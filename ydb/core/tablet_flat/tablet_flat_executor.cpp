#include "tablet_flat_executor.h"
#include "flat_executor.h"

namespace NKikimr {
namespace NTabletFlatExecutor {

namespace NFlatExecutorSetup {
    IActor* CreateExecutor(ITablet *owner, const TActorId& ownerActorId) {
        return new TExecutor(owner, ownerActorId);
    }

    void ITablet::SnapshotComplete(TIntrusivePtr<TTableSnapshotContext> snapContext, const TActorContext &ctx) {
        Y_UNUSED(snapContext);
        Y_UNUSED(ctx);
        Y_FAIL("must be overriden if plan to use table snapshot completion");
    }

    void ITablet::CompactionComplete(ui32 tableId, const TActorContext &ctx) {
        Y_UNUSED(tableId);
        Y_UNUSED(ctx);
    }

    void ITablet::CompletedLoansChanged(const TActorContext &ctx) {
        Y_UNUSED(ctx);
    }

    void ITablet::ScanComplete(NTable::EAbort status, TAutoPtr<IDestructable> prod, ui64 cookie, const TActorContext &ctx)
    {
        Y_UNUSED(status);
        Y_UNUSED(prod);
        Y_UNUSED(cookie);
        Y_UNUSED(ctx);
    }

    bool ITablet::ReassignChannelsEnabled() const { 
        // By default channels are reassigned automatically 
        return true; 
    } 
 
    void ITablet::UpdateTabletInfo(TIntrusivePtr<TTabletStorageInfo> info, const TActorId& launcherID) {
        if (info)
            TabletInfo = info;

        if (launcherID)
            LauncherActorID = launcherID;
    }
}

}}
