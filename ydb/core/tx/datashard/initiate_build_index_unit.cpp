#include "datashard_impl.h"
#include "datashard_pipeline.h"
#include "execution_unit_ctors.h"

namespace NKikimr {
namespace NDataShard {

class TInitiateBuildIndexUnit : public TExecutionUnit {
    THolder<TEvChangeExchange::TEvAddSender> AddSender;

public:
    TInitiateBuildIndexUnit(TDataShard& dataShard, TPipeline& pipeline)
        : TExecutionUnit(EExecutionUnitKind::InitiateBuildIndex, false, dataShard, pipeline)
    { }

    bool IsReadyToExecute(TOperation::TPtr) const override {
        return true;
    }

    EExecutionStatus Execute(TOperation::TPtr op, TTransactionContext& txc, const TActorContext& ctx) override {
        Y_VERIFY(op->IsSchemeTx());

        TActiveTransaction* tx = dynamic_cast<TActiveTransaction*>(op.Get());
        Y_VERIFY_S(tx, "cannot cast operation of kind " << op->GetKind());

        auto& schemeTx = tx->GetSchemeTx();
        if (!schemeTx.HasInitiateBuildIndex()) {
            return EExecutionStatus::Executed;
        }

        const auto& params = schemeTx.GetInitiateBuildIndex();

        auto pathId = TPathId(params.GetPathId().GetOwnerId(), params.GetPathId().GetLocalId());
        Y_VERIFY(pathId.OwnerId == DataShard.GetPathOwnerId());

        TUserTable::TPtr tableInfo;
        if (params.HasIndexDescription()) {
            const auto& indexDesc = params.GetIndexDescription();

            if (indexDesc.GetType() == NKikimrSchemeOp::EIndexType::EIndexTypeGlobalAsync) {
                auto indexPathId = TPathId(indexDesc.GetPathOwnerId(), indexDesc.GetLocalPathId());
                AddSender.Reset(new TEvChangeExchange::TEvAddSender(
                    pathId, TEvChangeExchange::ESenderType::AsyncIndex, indexPathId
                ));
            }

            tableInfo = DataShard.AlterTableAddIndex(ctx, txc, pathId, params.GetTableSchemaVersion(), indexDesc);
        } else {
            tableInfo = DataShard.AlterTableSchemaVersion(ctx, txc, pathId, params.GetTableSchemaVersion());
        }

        Y_VERIFY(tableInfo);
        DataShard.AddUserTable(pathId, tableInfo); 

        ui64 step = tx->GetStep();
        ui64 txId = tx->GetTxId();
        Y_VERIFY(step != 0);

        const TSnapshotKey key(pathId.OwnerId, pathId.LocalPathId, step, txId);

        ui64 flags = TSnapshot::FlagScheme;

        bool added = DataShard.GetSnapshotManager().AddSnapshot(
            txc.DB, key, params.GetSnapshotName(), flags, TDuration::Zero());

        BuildResult(op, NKikimrTxDataShard::TEvProposeTransactionResult::COMPLETE);
        op->Result()->SetStepOrderId(op->GetStepOrder().ToPair());

        if (added) {
            return EExecutionStatus::DelayCompleteNoMoreRestarts;
        } else {
            return EExecutionStatus::DelayComplete;
        }
    }

    void Complete(TOperation::TPtr, const TActorContext& ctx) override {
        if (AddSender) {
            ctx.Send(DataShard.GetChangeSender(), AddSender.Release());
        }
    }
};

THolder<TExecutionUnit> CreateInitiateBuildIndexUnit(
    TDataShard& dataShard,
    TPipeline& pipeline)
{
    return THolder(new TInitiateBuildIndexUnit(dataShard, pipeline));
}

} // namespace NDataShard
} // namespace NKikimr
