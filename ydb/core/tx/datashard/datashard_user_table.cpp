#include "datashard_user_table.h"

#include <ydb/core/base/path.h>
#include <ydb/core/tablet_flat/flat_cxx_database.h>
#include <ydb/core/tablet_flat/tablet_flat_executed.h>
#include <ydb/core/tablet_flat/tablet_flat_executor.h>

namespace NKikimr {

using NTabletFlatExecutor::TTransactionContext;

namespace NDataShard {

TUserTable::TUserTable(ui32 localTid, const NKikimrSchemeOp::TTableDescription& descr, ui32 shadowTid)
    : LocalTid(localTid)
    , ShadowTid(shadowTid) 
{
    Y_PROTOBUF_SUPPRESS_NODISCARD descr.SerializeToString(&Schema);
    Name = descr.GetName();
    Path = descr.GetPath();
    ParseProto(descr);
}

TUserTable::TUserTable(const TUserTable& table, const NKikimrSchemeOp::TTableDescription& descr)
    : TUserTable(table)
{
    Y_VERIFY_S(Name == descr.GetName(), "Name: " << Name << " descr.Name: " << descr.GetName());
    ParseProto(descr);
    AlterSchema();
}

void TUserTable::SetPath(const TString &path)
{
    auto name = ExtractBase(path);
    if (!name) {
        return;
    }

    Name = name;
    Path = path;
    AlterSchema();
}

void TUserTable::SetTableSchemaVersion(ui64 schemaVersion)
{
    NKikimrSchemeOp::TTableDescription schema;
    GetSchema(schema);
    schema.SetTableSchemaVersion(schemaVersion);
    SetSchema(schema);

    TableSchemaVersion = schemaVersion;
}

bool TUserTable::ResetTableSchemaVersion() 
{ 
    if (TableSchemaVersion) { 
        NKikimrSchemeOp::TTableDescription schema;
        GetSchema(schema); 
 
        schema.ClearTableSchemaVersion(); 
        TableSchemaVersion = 0; 
 
        SetSchema(schema); 
        return true; 
    } 
 
    return false; 
} 
 
void TUserTable::AddIndex(const NKikimrSchemeOp::TIndexDescription& indexDesc) {
    Y_VERIFY(indexDesc.HasPathOwnerId() && indexDesc.HasLocalPathId());
    const auto addIndexPathId = TPathId(indexDesc.GetPathOwnerId(), indexDesc.GetLocalPathId());

    if (Indexes.contains(addIndexPathId)) {
        return;
    }

    Indexes.emplace(addIndexPathId, TTableIndex(indexDesc, Columns));
    AsyncIndexCount += ui32(indexDesc.GetType() == TTableIndex::EIndexType::EIndexTypeGlobalAsync);

    NKikimrSchemeOp::TTableDescription schema;
    GetSchema(schema);

    schema.MutableTableIndexes()->Add()->CopyFrom(indexDesc);
    SetSchema(schema);
}

void TUserTable::DropIndex(const TPathId& indexPathId) {
    auto it = Indexes.find(indexPathId);
    if (it == Indexes.end()) {
        return;
    }

    AsyncIndexCount -= ui32(it->second.Type == TTableIndex::EIndexType::EIndexTypeGlobalAsync);
    Indexes.erase(it);

    NKikimrSchemeOp::TTableDescription schema;
    GetSchema(schema);

    for (auto it = schema.GetTableIndexes().begin(); it != schema.GetTableIndexes().end(); ++it) {
        if (indexPathId != TPathId(it->GetPathOwnerId(), it->GetLocalPathId())) {
            continue;
        }

        schema.MutableTableIndexes()->erase(it);
        SetSchema(schema);

        return;
    }

    Y_FAIL("unreachable");
}

bool TUserTable::HasAsyncIndexes() const {
    return AsyncIndexCount > 0;
}

void TUserTable::AddCdcStream(const NKikimrSchemeOp::TCdcStreamDescription& streamDesc) {
    Y_VERIFY(streamDesc.HasPathId());
    const auto streamPathId = TPathId(streamDesc.GetPathId().GetOwnerId(), streamDesc.GetPathId().GetLocalId());

    if (CdcStreams.contains(streamPathId)) {
        return;
    }

    CdcStreams.emplace(streamPathId, TCdcStream(streamDesc));

    NKikimrSchemeOp::TTableDescription schema;
    GetSchema(schema);

    schema.MutableCdcStreams()->Add()->CopyFrom(streamDesc);
    SetSchema(schema);
}

void TUserTable::DisableCdcStream(const TPathId& streamPathId) {
    auto it = CdcStreams.find(streamPathId);
    if (it == CdcStreams.end()) {
        return;
    }

    it->second.State = TCdcStream::EState::ECdcStreamStateDisabled;

    NKikimrSchemeOp::TTableDescription schema;
    GetSchema(schema);

    for (auto it = schema.MutableCdcStreams()->begin(); it != schema.MutableCdcStreams()->end(); ++it) {
        if (streamPathId != TPathId(it->GetPathId().GetOwnerId(), it->GetPathId().GetLocalId())) {
            continue;
        }

        it->SetState(TCdcStream::EState::ECdcStreamStateDisabled);
        SetSchema(schema);

        return;
    }

    Y_FAIL("unreachable");
}

void TUserTable::DropCdcStream(const TPathId& streamPathId) {
    auto it = CdcStreams.find(streamPathId);
    if (it == CdcStreams.end()) {
        return;
    }

    CdcStreams.erase(it);

    NKikimrSchemeOp::TTableDescription schema;
    GetSchema(schema);

    for (auto it = schema.GetCdcStreams().begin(); it != schema.GetCdcStreams().end(); ++it) {
        if (streamPathId != TPathId(it->GetPathId().GetOwnerId(), it->GetPathId().GetLocalId())) {
            continue;
        }

        schema.MutableCdcStreams()->erase(it);
        SetSchema(schema);

        return;
    }

    Y_FAIL("unreachable");
}

bool TUserTable::HasCdcStreams() const {
    return !CdcStreams.empty();
}

void TUserTable::ParseProto(const NKikimrSchemeOp::TTableDescription& descr)
{
    // We expect schemeshard to send us full list of storage rooms 
    if (descr.GetPartitionConfig().StorageRoomsSize()) { 
        Rooms.clear(); 
    } 
 
    for (const auto& roomDescr : descr.GetPartitionConfig().GetStorageRooms()) {
        TStorageRoom::TPtr room = new TStorageRoom(roomDescr);
        Rooms[room->GetId()] = room;
    }

    for (const auto& family : descr.GetPartitionConfig().GetColumnFamilies()) {
        auto it = Families.find(family.GetId());
        if (it == Families.end()) {
            it = Families.emplace(std::make_pair(family.GetId(), TUserFamily(family))).first;
        } else {
            it->second.Update(family);
        }
    } 

    for (auto& kv : Families) { 
        auto roomIt = Rooms.find(kv.second.GetRoomId()); 
        if (roomIt != Rooms.end()) {
            kv.second.Update(roomIt->second); 
        }
    }

    for (const auto& col : descr.GetColumns()) {
        TUserColumn& column = Columns[col.GetId()];
        if (column.Name.empty()) {
            column = TUserColumn(col.GetTypeId(), col.GetName());
        }
        column.Family = col.GetFamily();
        column.NotNull = col.GetNotNull();
    }

    for (const auto& col : descr.GetDropColumns()) {
        ui32 colId = col.GetId();
        auto it = Columns.find(colId);
        Y_VERIFY(it != Columns.end());
        Y_VERIFY(!it->second.IsKey);
        Columns.erase(it);
    }

    if (descr.KeyColumnIdsSize()) {
        Y_VERIFY(descr.KeyColumnIdsSize() >= KeyColumnIds.size());
        for (ui32 i = 0; i < KeyColumnIds.size(); ++i) {
            Y_VERIFY(KeyColumnIds[i] == descr.GetKeyColumnIds(i));
        }

        KeyColumnIds.clear();
        KeyColumnIds.reserve(descr.KeyColumnIdsSize());
        KeyColumnTypes.resize(descr.KeyColumnIdsSize());
        for (size_t i = 0; i < descr.KeyColumnIdsSize(); ++i) {
            ui32 keyColId = descr.GetKeyColumnIds(i);
            KeyColumnIds.push_back(keyColId);

            TUserColumn * col = Columns.FindPtr(keyColId);
            Y_VERIFY(col);
            col->IsKey = true;
            KeyColumnTypes[i] = col->Type;
        }

        Y_VERIFY(KeyColumnIds.size() == KeyColumnTypes.size());
    }

    if (descr.HasPartitionRangeBegin()) {
        Y_VERIFY(descr.HasPartitionRangeEnd());
        Range = TSerializedTableRange(descr.GetPartitionRangeBegin(),
                                      descr.GetPartitionRangeEnd(),
                                      descr.GetPartitionRangeBeginIsInclusive(),
                                      descr.GetPartitionRangeEndIsInclusive());
    }

    TableSchemaVersion = descr.GetTableSchemaVersion();
    IsBackup = descr.GetIsBackup(); 

    CheckSpecialColumns();

    for (const auto& indexDesc : descr.GetTableIndexes()) {
        Y_VERIFY(indexDesc.HasPathOwnerId() && indexDesc.HasLocalPathId());
        Indexes.emplace(TPathId(indexDesc.GetPathOwnerId(), indexDesc.GetLocalPathId()), TTableIndex(indexDesc, Columns));
        AsyncIndexCount += ui32(indexDesc.GetType() == TTableIndex::EIndexType::EIndexTypeGlobalAsync);
    }

    for (const auto& streamDesc : descr.GetCdcStreams()) {
        Y_VERIFY(streamDesc.HasPathId());
        CdcStreams.emplace(TPathId(streamDesc.GetPathId().GetOwnerId(), streamDesc.GetPathId().GetLocalId()), TCdcStream(streamDesc));
    }
}

void TUserTable::CheckSpecialColumns() {
    SpecialColTablet = Max<ui32>();
    SpecialColEpoch = Max<ui32>();
    SpecialColUpdateNo = Max<ui32>();

    for (const auto &xpair : Columns) {
        const ui32 colId = xpair.first;
        const auto &column = xpair.second;

        if (column.IsKey || column.Type != NScheme::NTypeIds::Uint64)
            continue;

        if (column.Name == "__tablet")
            SpecialColTablet = colId;
        else if (column.Name == "__updateEpoch")
            SpecialColEpoch = colId;
        else if (column.Name == "__updateNo")
            SpecialColUpdateNo = colId;
    }
}

void TUserTable::AlterSchema() {
    NKikimrSchemeOp::TTableDescription schema;
    GetSchema(schema);

    auto& partConfig = *schema.MutablePartitionConfig();
    partConfig.ClearStorageRooms();
    for (const auto& room : Rooms) {
        partConfig.AddStorageRooms()->CopyFrom(*room.second);
    }

    // FIXME: these generated column families are incorrect! 
    partConfig.ClearColumnFamilies();
    for (const auto& f : Families) {
        const TUserFamily& family = f.second;
        auto columnFamily = partConfig.AddColumnFamilies();
        columnFamily->SetId(f.first);
        columnFamily->SetName(family.GetName());
        columnFamily->SetStorage(family.Storage);
        columnFamily->SetColumnCodec(family.ColumnCodec);
        columnFamily->SetColumnCache(family.ColumnCache);
        columnFamily->SetRoom(family.GetRoomId()); 
    }

    schema.ClearColumns();
    for (const auto& col : Columns) {
        const TUserColumn& column = col.second;

        auto descr = schema.AddColumns();
        descr->SetName(column.Name);
        descr->SetId(col.first);
        descr->SetTypeId(column.Type);
        descr->SetFamily(column.Family);
        descr->SetNotNull(column.NotNull);
    }

    schema.SetPartitionRangeBegin(Range.From.GetBuffer());
    schema.SetPartitionRangeBeginIsInclusive(Range.FromInclusive);
    schema.SetPartitionRangeEnd(Range.To.GetBuffer());
    schema.SetPartitionRangeEndIsInclusive(Range.ToInclusive);

    schema.SetPath(Name);
    schema.SetPath(Path);

    SetSchema(schema);
}

void TUserTable::ApplyCreate( 
        TTransactionContext& txc, const TString& tableName, 
        const NKikimrSchemeOp::TPartitionConfig& partConfig) const
{
    DoApplyCreate(txc, tableName, false, partConfig); 
} 
 
void TUserTable::ApplyCreateShadow( 
        TTransactionContext& txc, const TString& tableName, 
        const NKikimrSchemeOp::TPartitionConfig& partConfig) const
{ 
    DoApplyCreate(txc, tableName, true, partConfig); 
} 
 
void TUserTable::DoApplyCreate( 
        TTransactionContext& txc, const TString& tableName, bool shadow, 
        const NKikimrSchemeOp::TPartitionConfig& partConfig) const
{ 
    const ui32 tid = shadow ? ShadowTid : LocalTid; 
 
    Y_VERIFY(tid != 0 && tid != Max<ui32>(), "Creating table %s with bad id %" PRIu32, tableName.c_str(), tid); 
 
    auto &alter = txc.DB.Alter();
    alter.AddTable(tableName, tid); 

    THashSet<ui32> appliedRooms; 
    for (const auto& fam : Families) {
        ui32 familyId = fam.first;
        const TUserFamily& family = fam.second;

        alter.AddFamily(tid, familyId, family.GetRoomId()); 
        alter.SetFamily(tid, familyId, family.Cache, family.Codec); 
        alter.SetFamilyBlobs(tid, familyId, family.GetOuterThreshold(), family.GetExternalThreshold()); 
        if (appliedRooms.insert(family.GetRoomId()).second) { 
            // Call SetRoom once per room 
            alter.SetRoom(tid, family.GetRoomId(), family.MainChannel(), family.ExternalChannel(), family.OuterChannel()); 
        } 
    }

    for (const auto& col : Columns) {
        ui32 columnId = col.first;
        const TUserColumn& column = col.second;

        alter.AddColumn(tid, column.Name, columnId, column.Type, column.NotNull);
        alter.AddColumnToFamily(tid, columnId, column.Family); 
    }

    for (size_t i = 0; i < KeyColumnIds.size(); ++i) {
        alter.AddColumnToKey(tid, KeyColumnIds[i]); 
    }

    if (partConfig.HasCompactionPolicy()) {
        NLocalDb::TCompactionPolicyPtr policy = new NLocalDb::TCompactionPolicy(partConfig.GetCompactionPolicy());
        alter.SetCompactionPolicy(tid, *policy); 
    } else {
        alter.SetCompactionPolicy(tid, *NLocalDb::CreateDefaultUserTablePolicy()); 
    }

    if (partConfig.HasEnableFilterByKey()) {
        alter.SetByKeyFilter(tid, partConfig.GetEnableFilterByKey()); 
    }

    // N.B. some settings only apply to the main table 

    if (!shadow) { 
        if (partConfig.HasExecutorCacheSize()) { 
            alter.SetExecutorCacheSize(partConfig.GetExecutorCacheSize()); 
        } 
 
        if (partConfig.HasResourceProfile() && partConfig.GetResourceProfile()) { 
            alter.SetExecutorResourceProfile(partConfig.GetResourceProfile()); 
        } 
 
        if (partConfig.HasExecutorFastLogPolicy()) { 
            alter.SetExecutorFastLogPolicy(partConfig.GetExecutorFastLogPolicy()); 
        } 
 
        alter.SetEraseCache(tid, partConfig.GetEnableEraseCache(), partConfig.GetEraseCacheMinRows(), partConfig.GetEraseCacheMaxBytes()); 
 
        if (IsBackup) { 
            alter.SetColdBorrow(tid, true); 
        } 
    } 
}

void TUserTable::ApplyAlter( 
        TTransactionContext& txc, const TUserTable& oldTable, 
        const NKikimrSchemeOp::TTableDescription& delta, TString& strError)
{
    const auto& configDelta = delta.GetPartitionConfig();
    NKikimrSchemeOp::TTableDescription schema;
    GetSchema(schema);
    auto& config = *schema.MutablePartitionConfig();

    auto &alter = txc.DB.Alter();

    // Check if we need to drop shadow table first 
    if (configDelta.HasShadowData()) { 
        if (configDelta.GetShadowData()) { 
            if (!ShadowTid) { 
                // Alter is creating shadow data 
                strError = "Alter cannot create new shadow data"; 
            } 
        } else { 
            if (ShadowTid) { 
                // Alter is removing shadow data 
                alter.DropTable(ShadowTid); 
                ShadowTid = 0; 
            } 
            config.ClearShadowData(); 
        } 
    } 
 
    // Most settings are applied to both main and shadow table 
    TStackVec<ui32> tids; 
    tids.push_back(LocalTid); 
    if (ShadowTid) { 
        tids.push_back(ShadowTid); 
    } 
 
    THashSet<ui32> appliedRooms; 
    for (const auto& f : Families) {
        ui32 familyId = f.first;
        const TUserFamily& family = f.second;

        for (ui32 tid : tids) { 
            alter.AddFamily(tid, familyId, family.GetRoomId()); 
            alter.SetFamily(tid, familyId, family.Cache, family.Codec); 
            alter.SetFamilyBlobs(tid, familyId, family.GetOuterThreshold(), family.GetExternalThreshold()); 
        } 
 
        if (appliedRooms.insert(family.GetRoomId()).second) { 
            // Call SetRoom once per room 
            for (ui32 tid : tids) { 
                alter.SetRoom(tid, family.GetRoomId(), family.MainChannel(), family.ExternalChannel(), family.OuterChannel()); 
            } 
        } 
    }

    for (const auto& col : Columns) {
        ui32 colId = col.first;
        const TUserColumn& column = col.second;

        if (!oldTable.Columns.contains(colId)) {
            for (ui32 tid : tids) { 
                alter.AddColumn(tid, column.Name, colId, column.Type, column.NotNull);
            } 
        }
 
        for (ui32 tid : tids) { 
            alter.AddColumnToFamily(tid, colId, column.Family); 
        } 
    }

    for (const auto& col : delta.GetDropColumns()) {
        ui32 colId = col.GetId();
        const TUserTable::TUserColumn * oldCol = oldTable.Columns.FindPtr(colId);
        Y_VERIFY(oldCol);
        Y_VERIFY(oldCol->Name == col.GetName());
        Y_VERIFY(!Columns.contains(colId));

        for (ui32 tid : tids) { 
            alter.DropColumn(tid, colId); 
        } 
    }

    for (size_t i = 0; i < KeyColumnIds.size(); ++i) {
        for (ui32 tid : tids) { 
            alter.AddColumnToKey(tid, KeyColumnIds[i]); 
        } 
    }

    if (configDelta.HasCompactionPolicy()) {
        TIntrusiveConstPtr<NLocalDb::TCompactionPolicy> oldPolicy =
                txc.DB.GetScheme().Tables.find(LocalTid)->second.CompactionPolicy;
        NLocalDb::TCompactionPolicy newPolicy(configDelta.GetCompactionPolicy());

        if (NLocalDb::ValidateCompactionPolicyChange(*oldPolicy, newPolicy, strError)) {
            for (ui32 tid : tids) { 
                alter.SetCompactionPolicy(tid, newPolicy); 
            } 
            config.ClearCompactionPolicy();
            newPolicy.Serialize(*config.MutableCompactionPolicy());
        } else {
            strError = TString("cannot change compaction policy: ") + strError;
        }
    }

    if (configDelta.HasEnableFilterByKey()) { 
        config.SetEnableFilterByKey(configDelta.GetEnableFilterByKey()); 
        for (ui32 tid : tids) { 
            alter.SetByKeyFilter(tid, configDelta.GetEnableFilterByKey()); 
        } 
    } 
 
    // N.B. some settings only apply to the main table 
 
    if (configDelta.HasExecutorCacheSize()) {
        config.SetExecutorCacheSize(configDelta.GetExecutorCacheSize());
        alter.SetExecutorCacheSize(configDelta.GetExecutorCacheSize());
    }

    if (configDelta.HasResourceProfile() && configDelta.GetResourceProfile()) { 
        config.SetResourceProfile(configDelta.GetResourceProfile()); 
        alter.SetExecutorResourceProfile(configDelta.GetResourceProfile());
    }

    if (configDelta.HasExecutorFastLogPolicy()) {
        config.SetExecutorFastLogPolicy(configDelta.GetExecutorFastLogPolicy());
        alter.SetExecutorFastLogPolicy(configDelta.GetExecutorFastLogPolicy());
    }

    if (configDelta.HasEnableEraseCache() || configDelta.HasEraseCacheMinRows() || configDelta.HasEraseCacheMaxBytes()) { 
        if (configDelta.HasEnableEraseCache()) { 
            config.SetEnableEraseCache(configDelta.GetEnableEraseCache()); 
        } 
        if (configDelta.HasEraseCacheMinRows()) { 
            config.SetEraseCacheMinRows(configDelta.GetEraseCacheMinRows()); 
        } 
        if (configDelta.HasEraseCacheMaxBytes()) { 
            config.SetEraseCacheMaxBytes(configDelta.GetEraseCacheMaxBytes()); 
        } 
        alter.SetEraseCache(LocalTid, config.GetEnableEraseCache(), config.GetEraseCacheMinRows(), config.GetEraseCacheMaxBytes()); 
    } 
 
    schema.SetTableSchemaVersion(delta.GetTableSchemaVersion());

    SetSchema(schema);
}

void TUserTable::ApplyDefaults(TTransactionContext& txc) const 
{ 
    const auto* tableInfo = txc.DB.GetScheme().GetTableInfo(LocalTid); 
    if (!tableInfo) { 
        // Local table does not exist, no need to apply any defaults 
        return; 
    } 
 
    NKikimrSchemeOp::TTableDescription schema;
    GetSchema(schema); 
    const auto& config = schema.GetPartitionConfig(); 
 
    if ((!config.HasEnableEraseCache() && config.GetEnableEraseCache() != tableInfo->EraseCacheEnabled) || 
        (config.GetEnableEraseCache() && !config.HasEraseCacheMinRows() && config.GetEraseCacheMinRows() != tableInfo->EraseCacheMinRows) || 
        (config.GetEnableEraseCache() && !config.HasEraseCacheMaxBytes() && config.GetEraseCacheMaxBytes() != tableInfo->EraseCacheMaxBytes)) 
    { 
        // Protobuf defaults for erase cache changed, apply to local database 
        txc.DB.Alter().SetEraseCache(LocalTid, config.GetEnableEraseCache(), config.GetEraseCacheMinRows(), config.GetEraseCacheMaxBytes()); 
    } 
} 
 
}}
