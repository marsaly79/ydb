#include "datashard_active_transaction.h" 
#include "datashard_impl.h" 
#include "datashard_pipeline.h" 
#include "execution_unit_ctors.h" 
#include "setup_sys_locks.h" 
 
namespace NKikimr { 
namespace NDataShard { 
 
class TExecuteCommitWritesTxUnit : public TExecutionUnit { 
public: 
    TExecuteCommitWritesTxUnit(TDataShard& self, TPipeline& pipeline) 
        : TExecutionUnit(EExecutionUnitKind::ExecuteCommitWritesTx, false, self, pipeline) 
    { 
    } 
 
    bool IsReadyToExecute(TOperation::TPtr op) const override { 
        if (DataShard.IsStopping()) { 
            // Avoid doing any new work when datashard is stopping 
            return false; 
        } 
 
        return !op->HasRuntimeConflicts(); 
    } 
 
    EExecutionStatus Execute(TOperation::TPtr op, TTransactionContext& txc, const TActorContext&) override { 
        Y_VERIFY(op->IsCommitWritesTx()); 
 
        TActiveTransaction* tx = dynamic_cast<TActiveTransaction*>(op.Get()); 
        Y_VERIFY_S(tx, "cannot cast operation of kind " << op->GetKind()); 
 
        TSetupSysLocks guardLocks(op, DataShard); 
 
        const auto& commitTx = tx->GetCommitWritesTx()->GetBody(); 
        const auto versions = DataShard.GetReadWriteVersions(op.Get()); 
 
        const auto& tableId = commitTx.GetTableId(); 
        const auto& tableInfo = *DataShard.GetUserTables().at(tableId.GetTableId()); 
        const ui64 writeTxId = commitTx.GetWriteTxId(); 
 
        const TTableId fullTableId(tableId.GetOwnerId(), tableId.GetTableId()); 
 
        // FIXME: temporary break all locks, but we want to be smarter about which locks we break 
        DataShard.SysLocksTable().BreakAllLocks(fullTableId); 
        txc.DB.CommitTx(tableInfo.LocalTid, writeTxId, versions.WriteVersion); 
 
        BuildResult(op, NKikimrTxDataShard::TEvProposeTransactionResult::COMPLETE); 
        DataShard.SysLocksTable().ApplyLocks(); 
        Pipeline.AddCommittingOp(op); 
 
        return EExecutionStatus::ExecutedNoMoreRestarts; 
    } 
 
    void Complete(TOperation::TPtr, const TActorContext&) override { 
    } 
}; 
 
THolder<TExecutionUnit> CreateExecuteCommitWritesTxUnit(TDataShard& self, TPipeline& pipeline) { 
    return THolder(new TExecuteCommitWritesTxUnit(self, pipeline)); 
} 
 
} // namespace NDataShard 
} // namespace NKikimr 
