#pragma once

#include "defs.h"
#include "config.h"
#include "downtime.h"
#include "services.h"

#include <library/cpp/actors/interconnect/interconnect.h>
#include <ydb/core/base/blobstorage.h>
#include <ydb/core/node_whiteboard/node_whiteboard.h>
#include <ydb/core/blobstorage/base/blobstorage_vdiskid.h>
#include <ydb/core/mind/tenant_pool.h>
#include <ydb/core/protos/cms.pb.h>

#include <util/generic/hash.h>
#include <util/generic/maybe.h>
#include <util/generic/set.h>

namespace NKikimr {
namespace NCms {

// Forward declarations.
class TClusterInfo;
using TClusterInfoPtr = TIntrusivePtr<TClusterInfo>;
struct TCmsState;
using TCmsStatePtr = TIntrusivePtr<TCmsState>;

struct TErrorInfo {
    NKikimrCms::TStatus::ECode Code = NKikimrCms::TStatus::ALLOW;
    TString Reason;
    TInstant Deadline;
    ui64 RollbackPoint = 0;
};

/**
 * Structure to hold info about issued permission. A set of
 * all issued permissions is a part of CMS persistent state.
 * Info is used for permissions management and to identify
 * locked items (nodes and devices) in cluster.
 */
struct TPermissionInfo {
    TPermissionInfo() = default;
    TPermissionInfo(const TPermissionInfo &other) = default;
    TPermissionInfo(TPermissionInfo &&other) = default;

    TPermissionInfo(const NKikimrCms::TPermission &permission, const TString &requestId,
                    const TString &owner)
        : PermissionId(permission.GetId())
        , RequestId(requestId)
        , Owner(owner)
        , Action(permission.GetAction())
        , Deadline(TInstant::MicroSeconds(permission.GetDeadline())) 
    {
    }

    TPermissionInfo &operator=(const TPermissionInfo &other) = default;
    TPermissionInfo &operator=(TPermissionInfo &&other) = default;

    void CopyTo(NKikimrCms::TPermission &permission) const
    {
        permission.SetId(PermissionId);
        permission.MutableAction()->CopyFrom(Action);
        permission.SetDeadline(Deadline.GetValue());
    }

    TString PermissionId;
    TString RequestId;
    TString Owner;
    NKikimrCms::TAction Action;
    TInstant Deadline;
};

/**
 * Structure to hold information about scheduled request.
 */
struct TRequestInfo {
    TRequestInfo() = default;
    TRequestInfo(const TRequestInfo &other) = default;
    TRequestInfo(TRequestInfo &&other) = default;

    TRequestInfo &operator=(const TRequestInfo &other) = default;
    TRequestInfo &operator=(TRequestInfo &&other) = default;

    void CopyTo(NKikimrCms::TManageRequestResponse::TScheduledRequest &request) const
    {
        request.SetRequestId(RequestId);
        request.SetOwner(Owner);
        request.MutableActions()->CopyFrom(Request.GetActions());
        request.SetPartialPermissionAllowed(Request.GetPartialPermissionAllowed());
        request.SetReason(Request.GetReason());
    }

    TString RequestId;
    TString Owner;
    ui64 Order = 0;
    NKikimrCms::TPermissionRequest Request;
};

/**
 * Structure to hold information about notification.
 */
struct TNotificationInfo {
    TNotificationInfo() = default;
    TNotificationInfo(const TNotificationInfo &other) = default;
    TNotificationInfo(TNotificationInfo &&other) = default;

    TNotificationInfo &operator=(const TNotificationInfo &other) = default;
    TNotificationInfo &operator=(TNotificationInfo &&other) = default;

    void CopyTo(NKikimrCms::TManageNotificationResponse::TStoredNotification &notification) const
    {
        notification.SetNotificationId(NotificationId);
        notification.SetOwner(Owner);
        notification.MutableActions()->CopyFrom(Notification.GetActions());
        notification.SetTime(Notification.GetTime());
        notification.SetReason(Notification.GetReason());
    }

    TString NotificationId;
    TString Owner;
    NKikimrCms::TNotification Notification;
};

/**
 * Base class for entity which can be locked by CMS user. There are three
 * types of locks used.
 *
 * TLock - lock by issued permission. Only one such lock is possible per item.
 *
 * TExternalLock - lock caused by some external activity (not permitted by CMS)
 * reported via CMS notifications. This lock may have delayed effect which allows
 * to notify about some actions in the future.
 *
 * TScheduledLock - lock by scheduled request. Multiple scheduled locks are allowed.
 * Scheduled locks are ordered and request cannot get permission for an item if
 * it has a scheduled lock with lower order.
 *
 * TTemporaryLock - temporary lock used for action processing. Used to identify
 * conflicts within a single action and between actions in a single request.
 */
class TLockableItem : public TThrRefBase {
public:
    struct TBaseLock
    {
        TBaseLock(const TString &owner, const NKikimrCms::TAction &action)
            : Owner(owner)
            , Action(action)
        {
        }
        TBaseLock(const TBaseLock &other) = default;
        TBaseLock(TBaseLock &&other) = default;

        TBaseLock &operator=(const TBaseLock &other) = default;
        TBaseLock &operator=(TBaseLock &&other) = default;

        TString Owner;
        NKikimrCms::TAction Action;
    };

    struct TLock : public TBaseLock {
        TLock(const TPermissionInfo &permission)
            : TBaseLock(permission.Owner, permission.Action)
        {
            PermissionId = permission.PermissionId;
            LockDeadline = permission.Deadline;
            ActionDeadline = LockDeadline + TDuration::MicroSeconds(Action.GetDuration()); 
        }
        TLock(const TLock &other) = default;
        TLock(TLock &&other) = default;

        TLock &operator=(const TLock &other) = default;
        TLock &operator=(TLock &&other) = default;

        TString PermissionId;
        TInstant LockDeadline;
        TInstant ActionDeadline;
    };

    struct TExternalLock : TBaseLock {
        TExternalLock(const TNotificationInfo &notification, const NKikimrCms::TAction &action)
            : TBaseLock(notification.Owner, action)
        {
            NotificationId = notification.NotificationId;
            if (notification.Notification.HasTime())
                LockStart = TInstant::MicroSeconds(notification.Notification.GetTime()); 
            else
                LockStart = TActivationContext::Now();
            LockDeadline = LockStart + TDuration::MicroSeconds(action.GetDuration()); 
        }

        TString NotificationId;
        TInstant LockStart;
        TInstant LockDeadline;
    };

    struct TScheduledLock : TBaseLock {
        TScheduledLock(const NKikimrCms::TAction &action, const TString &owner, const TString &requestId, ui64 order)
            : TBaseLock(owner, action)
            , RequestId(requestId)
            , Order(order)
        {
        }

        TScheduledLock(const TScheduledLock &other) = default;
        TScheduledLock(TScheduledLock &&other) = default;

        TScheduledLock &operator=(const TScheduledLock &other) = default;
        TScheduledLock &operator=(TScheduledLock &&other) = default;

        TString RequestId;
        ui64 Order = 0;
    };

    struct TTemporaryLock : TBaseLock {
        TTemporaryLock(ui64 point, const NKikimrCms::TAction &action)
            : TBaseLock("", action)
            , RollbackPoint(point)
        {
        }
        TTemporaryLock(const TTemporaryLock &other) = default;
        TTemporaryLock(TTemporaryLock &&other) = default;

        TTemporaryLock &operator=(const TTemporaryLock &other) = default;
        TTemporaryLock &operator=(TTemporaryLock &&other) = default;

        ui64 RollbackPoint;
    };

    TLockableItem() = default;
    TLockableItem(NKikimrCms::EState state)
        : State(state)
    {
    }

    virtual ~TLockableItem()
    {
    }

    virtual TString ItemName() const = 0;
    virtual TString PrettyItemName() const
    {
        return ItemName();
    }

    void AddLock(const TPermissionInfo &permission)
    {
        Y_VERIFY(Lock.Empty());
        Lock.ConstructInPlace(permission);
    }

    void AddExternalLock(const TNotificationInfo &notification,
                         const NKikimrCms::TAction &action)
    {
        TExternalLock lock(notification, action);
        auto pos = LowerBound(ExternalLocks.begin(), ExternalLocks.end(), lock, [](auto &l, auto &r) {
                return l.LockStart < r.LockStart;
            });
        ExternalLocks.insert(pos, std::move(lock));
    }

    void ScheduleLock(TScheduledLock &&lock)
    {
        auto pos = LowerBound(ScheduledLocks.begin(), ScheduledLocks.end(), lock, [](auto &l, auto &r) {
                return l.Order < r.Order;
            });
        ScheduledLocks.insert(pos, lock);
    }

    bool IsLocked(TErrorInfo &error, TDuration defaultRetryTime, TInstant no, TDuration durationw) const;
    bool IsDown(TErrorInfo &error, TInstant defaultDeadline) const;

    void RollbackLocks(ui64 point);

    void DeactivateScheduledLocks(ui64 order);
    void ReactivateScheduledLocks();
    void RemoveScheduledLocks(const TString &requestId);

    // Fill some item info (e.g. Downtime) basing on previous item state.
    virtual void MigrateOldInfo(const TLockableItem &old);

    void DebugLocksDump(IOutputStream &ss, const TString &prefix = "") const;

    TInstant Timestamp;
    NKikimrCms::EState State = NKikimrCms::UNKNOWN;
    // Recent item downtimes.
    TDowntime Downtime = TDowntime(TDuration::Zero());

    TMaybe<TLock> Lock;
    std::list<TExternalLock> ExternalLocks;
    std::list<TScheduledLock> ScheduledLocks;
    TVector<TTemporaryLock> TempLocks; 
    ui64 DeactivatedLocksOrder = Max<ui64>();
};
using TLockableItemPtr = TIntrusivePtr<TLockableItem>;

/**
 * Structure to hold info and state for a single node. It holds basic
 * info received from NodeWhiteboard and references to tablet and
 * disk structures.
 */
class TNodeInfo : public TLockableItem {
public:
    TNodeInfo() = default;
    TNodeInfo(const TNodeInfo &other) = default;
    TNodeInfo(TNodeInfo &&other) = default;

    TNodeInfo &operator=(const TNodeInfo &other) = default;
    TNodeInfo &operator=(TNodeInfo &&other) = default;

    TString ItemName() const override
    {
        return Sprintf("Host %s:%" PRIu16 " (%" PRIu32 ")", Host.data(), IcPort, NodeId);
    }

    void MigrateOldInfo(const TLockableItem &old) override;

    ui32 NodeId = 0;
    TString Host;
    TString Address;
    ui16 IcPort = 0;
    TNodeLocation Location;
    TString Version;
    TSet<ui64> Tablets; 
    TSet<TPDiskID> PDisks;
    TSet<TVDiskID> VDisks; 
    bool HasTenantInfo = false;
    TString Tenant;
    TString PreviousTenant;
    TServices Services;
};
using TNodeInfoPtr = TIntrusivePtr<TNodeInfo>;

/**
 * Structure to hold tablet info received from NodeWhiteboard.
 */
struct TTabletInfo {
    using EState = NKikimrWhiteboard::TTabletStateInfo::ETabletState;
    using EType = TTabletTypes::EType;

    TTabletInfo() = default;
    TTabletInfo(const TTabletInfo &other) = default;
    TTabletInfo(TTabletInfo &&other) = default;

    TTabletInfo &operator=(const TTabletInfo &other) = default;
    TTabletInfo &operator=(TTabletInfo &&other) = default;

    ui64 TabletId = 0;
    EType Type = TTabletTypes::Unknown;
    EState State = NKikimrWhiteboard::TTabletStateInfo::Created;
    bool Leader = false;
    ui32 NodeId = 0;
};

/**
 * Structure to describe PDisk state. Holds state received from
 * NodeWhiteboard and references to VDisk structures.
 */
class TPDiskInfo : public TLockableItem {
public:
    TPDiskInfo(TPDiskID id = TPDiskID())
        : TLockableItem(NKikimrCms::DOWN)
        , PDiskId(id)
        , NodeId(id.NodeId)
        , Path("")
    {
    }

    TPDiskInfo(const TPDiskInfo &other) = default;
    TPDiskInfo(TPDiskInfo &&other) = default;

    TPDiskInfo &operator=(const TPDiskInfo &other) = default;
    TPDiskInfo &operator=(TPDiskInfo &&other) = default;

    TString ItemName() const override;
    TString PrettyItemName() const override;
    TString GetDeviceName() const;
    static bool IsDeviceName(const TString &name);
    static TPDiskID NameToId(const TString &name);

    void MigrateOldInfo(const TLockableItem &old) override;

    TPDiskID PDiskId;
    ui32 NodeId;
    TString Host;
    TString Path;
    TSet<TVDiskID> VDisks; 
    // SlotIdx -> VDiskID
    THashMap<ui32, TVDiskID> VSlots;

private:
    static bool NameToId(const TString &name, TPDiskID &id);
};
using TPDiskInfoPtr = TIntrusivePtr<TPDiskInfo>;

/**
 * Structure to describe VDisk state. Holds state received from
 * NodeWhiteboard and references to BSGroups VDisk is a part of.
 */
class TVDiskInfo : public TLockableItem {
public:
    TVDiskInfo(TVDiskID id = TVDiskID(), TPDiskID pdiskId = TPDiskID(), NKikimrCms::EState state = NKikimrCms::DOWN, ui32 nodeId = 0)
        : TLockableItem(state)
        , VDiskId(id)
        , PDiskId(pdiskId)
        , NodeId(nodeId)
        , SlotId(0)
    {
    }

    TVDiskInfo(const TVDiskInfo &other) = default;
    TVDiskInfo(TVDiskInfo &&other) = default;

    TVDiskInfo &operator=(const TVDiskInfo &other) = default;
    TVDiskInfo &operator=(TVDiskInfo &&other) = default;

    TString ItemName() const override;
    TString PrettyItemName() const override;
    TString GetDeviceName() const;
    static bool IsDeviceName(const TString &name);
    static TVDiskID NameToId(const TString &name);

    void MigrateOldInfo(const TLockableItem &old) override;

    TVDiskID VDiskId;
    TPDiskID PDiskId;
    TString Path;
    ui32 NodeId;
    TString Host;
    ui32 SlotId;
    TSet<ui32> BSGroups; 

private:
    static bool NameToId(const TString &name, TVDiskID &id);
};
using TVDiskInfoPtr = TIntrusivePtr<TVDiskInfo>;

/**
 * Describes type and structure of BS group.
 */
struct TBSGroupInfo {
    TBSGroupInfo() = default;
    TBSGroupInfo(const TBSGroupInfo &other) = default;
    TBSGroupInfo(TBSGroupInfo &&other) = default;

    TBSGroupInfo &operator=(const TBSGroupInfo &other) = default;
    TBSGroupInfo &operator=(TBSGroupInfo &&other) = default;

    ui32 GroupId = 0;
    TErasureType Erasure;
    TSet<TVDiskID> VDisks; 
};

/**
 * Main class to hold current cluster state.
 *
 * State is built by merging pieces of information from NodeWhiteboard through
 * AddNode/AddTablet/AddPDisk/AddVDisk/AddBSGroup methods.
 *
 * CMS state affects cluster state via locks added by
 * AddLocks/AddExternalLocks/AddTempLocks/ScheduleActions methods.
 */
class TClusterInfo : public TThrRefBase {
public:
    using TNodes = THashMap<ui32, TNodeInfoPtr>; 
    using TTablets = THashMap<ui64, TTabletInfo>; 
    using TPDisks = THashMap<TPDiskID, TPDiskInfoPtr, TPDiskIDHash>;
    using TVDisks = THashMap<TVDiskID, TVDiskInfoPtr>; 
    using TBSGroups = THashMap<ui32, TBSGroupInfo>; 

    TClusterInfo() = default;
    TClusterInfo(const TClusterInfo &other) = default;
    TClusterInfo(TClusterInfo &&other) = default;

    TClusterInfo &operator=(const TClusterInfo &other) = default;
    TClusterInfo &operator=(TClusterInfo &&other) = default;

    bool HasNode(ui32 nodeId) const
    {
        return Nodes.contains(nodeId);
    }

    bool HasNode(const TString &hostName) const
    {
        ui32 nodeId;
        if (TryFromString(hostName, nodeId))
            return HasNode(nodeId);
        return HostNameToNodeId.contains(hostName);
    }

    const TNodeInfo &Node(ui32 nodeId) const
    {
        Y_VERIFY(HasNode(nodeId));
        return *Nodes.find(nodeId)->second;
    }

    TVector<const TNodeInfo *> HostNodes(const TString &hostName) const
    {
        TVector<const TNodeInfo *> nodes;

        ui32 nodeId;
        if (TryFromString(hostName, nodeId)) {
            if (HasNode(nodeId))
                nodes.push_back(&NodeRef(nodeId));
            return nodes;
        }

        auto pr = HostNameToNodeId.equal_range(hostName);
        for (auto it = pr.first; it != pr.second; ++it) {
            nodeId = it->second;
            Y_VERIFY(HasNode(nodeId));
            nodes.push_back(Nodes.find(nodeId)->second.Get());
        }

        return nodes;
    }

    TVector<const TNodeInfo *> TenantNodes(const TString &tenant) const
    {
        TVector<const TNodeInfo *> nodes;

        auto pr = TenantToNodeId.equal_range(tenant);
        for (auto it = pr.first; it != pr.second; ++it) {
            const ui32 nodeId = it->second;
            Y_VERIFY(HasNode(nodeId));
            nodes.push_back(Nodes.find(nodeId)->second.Get());
        }

        return nodes;
    }

    size_t NodesCount() const
    {
        return Nodes.size();
    }

    size_t NodesCount(const TString &hostName) const
    {
        ui32 nodeId;
        if (TryFromString(hostName, nodeId))
            return HasNode(nodeId) ? 1 : 0;

        return HostNameToNodeId.count(hostName);
    }

    const TNodes &AllNodes() const
    {
        return Nodes;
    }

    bool HasTablet(ui64 id) const
    {
        return Tablets.contains(id);
    }

    const TTabletInfo &Tablet(ui64 id) const
    {
        Y_VERIFY(HasTablet(id));
        return Tablets.find(id)->second;
    }

    const TTablets &AllTablets() const
    {
        return Tablets;
    }

    bool HasPDisk(TPDiskID pdId) const
    {
        return PDisks.contains(pdId);
    }

    bool HasPDisk(const TString &name) const
    {
        if (!TPDiskInfo::IsDeviceName(name))
            return false;
        auto id = TPDiskInfo::NameToId(name);
        return PDisks.contains(id);
    }

    bool HasPDisk(const TString &hostName, const TString &path) const
    {
        return !!HostNamePathToPDiskId(hostName, path);
    }

    const TPDiskInfo &PDisk(TPDiskID pdId) const
    {
        Y_VERIFY(HasPDisk(pdId));
        return *PDisks.find(pdId)->second;
    }

    const TPDiskInfo &PDisk(const TString &name) const
    {
        auto id = TPDiskInfo::NameToId(name);
        return PDisk(id);
    }

    const TPDiskInfo &PDisk(const TString &hostName, const TString &path) const
    {
        return PDisk(HostNamePathToPDiskId(hostName, path));
    }

    size_t PDisksCount() const
    {
        return PDisks.size();
    }

    const TPDisks &AllPDisks() const
    {
        return PDisks;
    }

    bool HasVDisk(const TVDiskID &vdId) const
    {
        return VDisks.contains(vdId);
    }

    bool HasVDisk(const TString &name) const
    {
        if (!TVDiskInfo::IsDeviceName(name))
            return false;
        auto id = TVDiskInfo::NameToId(name);
        return VDisks.contains(id);
    }

    const TVDiskInfo &VDisk(const TVDiskID &vdId) const
    {
        Y_VERIFY(HasVDisk(vdId));
        return *VDisks.find(vdId)->second;
    }

    const TVDiskInfo &VDisk(const TString &name) const
    {
        auto id = TVDiskInfo::NameToId(name);
        return VDisk(id);
    }

    size_t VDisksCount() const
    {
        return VDisks.size();
    }

    const TVDisks &AllVDisks() const
    {
        return VDisks;
    }

    bool HasBSGroup(ui32 groupId) const
    {
        return BSGroups.contains(groupId);
    }

    const TBSGroupInfo &BSGroup(ui32 groupId) const
    {
        Y_VERIFY(HasBSGroup(groupId));
        return BSGroups.find(groupId)->second;
    }

    size_t BSGroupsCount() const
    {
        return BSGroups.size();
    }

    const TBSGroups &AllBSGroups() const
    {
        return BSGroups;
    }

    TInstant GetTimestamp() const
    {
        return Timestamp;
    }

    void SetTimestamp(TInstant timestamp);

    void AddNode(const TEvInterconnect::TNodeInfo &info, const TActorContext *ctx);
    void SetNodeState(ui32 nodeId, NKikimrCms::EState state, const NKikimrWhiteboard::TSystemStateInfo &info);
    void ClearNode(ui32 nodeId);
    void AddTablet(ui32 nodeId, const NKikimrWhiteboard::TTabletStateInfo &info);
    void AddPDisk(const NKikimrBlobStorage::TBaseConfig::TPDisk &info);
    void UpdatePDiskState(const TPDiskID &id, const NKikimrWhiteboard::TPDiskStateInfo &info);
    void AddVDisk(const NKikimrBlobStorage::TBaseConfig::TVSlot &info);
    void UpdateVDiskState(const TVDiskID &id, const NKikimrWhiteboard::TVDiskStateInfo &info);
    void AddBSGroup(const NKikimrBlobStorage::TBaseConfig::TGroup &info);
    void AddNodeTenants(ui32 nodeId, const NKikimrTenantPool::TTenantPoolStatus &info);

    void AddNodeTempLock(ui32 nodeId, const NKikimrCms::TAction &action);
    void AddPDiskTempLock(TPDiskID pdiskId, const NKikimrCms::TAction &action);
    void AddVDiskTempLock(TVDiskID vdiskId, const NKikimrCms::TAction &action);

    ui64 AddLocks(const TPermissionInfo &permission, const TActorContext *ctx);

    ui64 AddLocks(const NKikimrCms::TPermission &permission, const TString &requestId,
                  const TString &owner, const TActorContext *ctx)
    {
        return AddLocks({permission, requestId, owner}, ctx);
    }

    ui64 AddExternalLocks(const TNotificationInfo &notification, const TActorContext *ctx);

    void ApplyDowntimes(const TDowntimes &downtimes);
    void UpdateDowntimes(TDowntimes &downtimes, const TActorContext &ctx);

    ui64 AddTempLocks(const NKikimrCms::TAction &action, const TActorContext *ctx);
    ui64 ScheduleActions(const TRequestInfo &request, const TActorContext *ctx);
    void UnscheduleActions(const TString &requestId);
    void DeactivateScheduledLocks(ui64 order);
    void ReactivateScheduledLocks();

    void RollbackLocks(ui64 point);
    ui64 PushRollbackPoint()
    {
        return ++RollbackPoint;
    }

    void MigrateOldInfo(TClusterInfoPtr old);
    void ApplyInitialNodeTenants(const TActorContext& ctx, const THashMap<ui32, TString>& nodeTenants);

    void DebugDump(const TActorContext &ctx) const;

    void SetTenantsInfo(bool value) { HasTenantsInfo = value; }

    bool IsOutdated() const { return Outdated; }
    void SetOutdated(bool val) { Outdated = val; }

    static EGroupConfigurationType VDiskConfigurationType(const TVDiskID &vdId) {
        return TGroupID(vdId.GroupID).ConfigurationType();
    }

    static bool IsStaticGroupVDisk(const TVDiskID &vdId) { return GroupConfigurationTypeStatic == VDiskConfigurationType(vdId); }
    static bool IsDynamicGroupVDisk(const TVDiskID &vdId) { return GroupConfigurationTypeDynamic == VDiskConfigurationType(vdId); }

private:
    TNodeInfo &NodeRef(ui32 nodeId) const
    {
        Y_VERIFY(HasNode(nodeId));
        return *Nodes.find(nodeId)->second;
    }

    TVector<TNodeInfo *> NodePtrs(const TString &hostName, const TServices &filterByServices = {})
    {
        TVector<TNodeInfo *> nodes;

        ui32 nodeId;
        if (TryFromString(hostName, nodeId)) {
            if (HasNode(nodeId))
                nodes.push_back(&NodeRef(nodeId));
            return nodes;
        }

        auto range = HostNameToNodeId.equal_range(hostName);
        for (auto it = range.first; it != range.second; ++it) {
            nodeId = it->second;

            Y_VERIFY(HasNode(nodeId));
            auto &node = NodeRef(nodeId);

            if (filterByServices && !(node.Services & filterByServices)) {
                continue;
            }

            nodes.push_back(&node);
        }

        return nodes;
    }

    TPDiskInfo &PDiskRef(TPDiskID pdId)
    {
        Y_VERIFY(HasPDisk(pdId));
        return *PDisks.find(pdId)->second;
    }

    TPDiskInfo &PDiskRef(const TString &name)
    {
        TPDiskID id = TPDiskInfo::NameToId(name);
        return PDiskRef(id);
    }

    TVDiskInfo &VDiskRef(const TVDiskID &vdId)
    {
        Y_VERIFY(HasVDisk(vdId));
        return *VDisks.find(vdId)->second;
    }

    TVDiskInfo &VDiskRef(const TString &name)
    {
        TVDiskID id = TVDiskInfo::NameToId(name);
        return VDiskRef(id);
    }

    TBSGroupInfo &BSGroupRef(ui32 groupId)
    {
        Y_VERIFY(HasBSGroup(groupId));
        return BSGroups.find(groupId)->second;
    }

    TPDiskID HostNamePathToPDiskId(const TString &hostName, const TString &path) const {
        auto pr = HostNameToNodeId.equal_range(hostName);
        for (auto it = pr.first; it != pr.second; ++it) {
            const ui32 nodeId = it->second;
            Y_VERIFY(HasNode(nodeId));
            const auto &node = Node(nodeId);
            for (const auto &id : node.PDisks) {
                Y_VERIFY(HasPDisk(id));
                if (PDisk(id).Path == path) {
                    return id;
                }
            }
        }
        return TPDiskID();
    }

    TSet<TLockableItem *> FindLockedItems(const NKikimrCms::TAction &action, const TActorContext *ctx); 

    TNodes Nodes;
    TTablets Tablets;
    TPDisks PDisks;
    TVDisks VDisks;
    TBSGroups BSGroups;
    TInstant Timestamp;
    ui64 RollbackPoint = 0;
    bool HasTenantsInfo = false;
    bool Outdated = false;

    // Fast access structures.
    TMultiMap<TString, ui32> HostNameToNodeId; 
    TMultiMap<TString, ui32> TenantToNodeId;
    THashMap<TString, TLockableItemPtr> LockableItems; 
};

inline bool ActionRequiresHost(NKikimrCms::TAction::EType type)
{
    return type != NKikimrCms::TAction::ADD_HOST
        && type != NKikimrCms::TAction::ADD_DEVICES
        && type != NKikimrCms::TAction::REPLACE_DEVICES
        && type != NKikimrCms::TAction::REMOVE_DEVICES;
}

inline bool ActionRequiresHost(const NKikimrCms::TAction &action)
{
    return ActionRequiresHost(action.GetType());
}


} // NCms
} // NKikimr
