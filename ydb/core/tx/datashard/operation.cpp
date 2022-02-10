#include "operation.h"
#include "key_conflicts.h"
#include "datashard_impl.h"

namespace NKikimr {
namespace NDataShard {

namespace {
void PrintDepTx(const TOperation *op,
                IOutputStream &os)
{
    os << " " << op->GetTxId();
    if (op->IsWaitingDependencies())
        os << "(W)";
    if (op->IsExecuting())
        os << "(E)";
    if (op->IsCompleted())
        os << "(C)";
}

void PrintDeps(const TOperation *op,
               IOutputStream &os)
{
    os << "OPERATION " << *op << " DEPS IN";
    for (auto &dep: op->GetDependencies()) 
        PrintDepTx(dep.Get(), os); 
    os << " OUT";
    for (auto &dep: op->GetDependents()) 
        PrintDepTx(dep.Get(), os); 
    os << Endl;
}
}

void TBasicOpInfo::Serialize(NKikimrTxDataShard::TBasicOpInfo &info) const
{
    info.SetTxId(TxId);
    info.SetStep(Step);
    info.SetKind(ToString(Kind));
    info.SetReceivedAt(ReceivedAt.GetValue());
    info.SetMinStep(MinStep);
    info.SetMaxStep(MaxStep);
    for (ui64 i = 1; i && i <= TTxFlags::LastFlag; i <<= 1) {
        if (HasFlag(static_cast<TTxFlags::Flags>(i)))
            info.AddFlags(ToString(static_cast<TTxFlags::Flags>(i)));
    }
}

NMiniKQL::IEngineFlat::TValidationInfo TOperation::EmptyKeysInfo;

void TOperation::AddInReadSet(const NKikimrTx::TEvReadSet& rs)
{
    AddInReadSet(TReadSetKey(rs), rs.GetBalanceTrackList(), rs.GetReadSet());
}

// Adjust incomplete readsets counter (track incapsulates logic required to handle merge/split of datashards)
void TOperation::AddInReadSet(const TReadSetKey &rsKey,
                              const NKikimrTx::TBalanceTrackList &btList,
                              TString readSet)
{
    auto it = CoverageBuilders().find(std::make_pair(rsKey.From, rsKey.To));
    if (it != CoverageBuilders().end()) {
        if (it->second->AddResult(btList)) {
            LOG_TRACE_S(TActivationContext::AsActorContext(), NKikimrServices::TX_DATASHARD,
                        "Filled readset for " << *this << " from=" << rsKey.From
                        << " to=" << rsKey.To << "origin=" << rsKey.Origin);
            InReadSets()[it->first].emplace_back(TRSData(readSet, rsKey.Origin));
            if (it->second->IsComplete()) {
                Y_VERIFY(InputDataRef().RemainReadSets > 0, "RemainReadSets counter underflow");
                --InputDataRef().RemainReadSets;
            }
        }
    } else {
        LOG_NOTICE_S(TActivationContext::AsActorContext(), NKikimrServices::TX_DATASHARD,
                     "Discarded readset for " << *this << " from=" << rsKey.From
                     << " to=" << rsKey.To << "origin=" << rsKey.Origin);
    }
}

void TOperation::AddDependency(const TOperation::TPtr &op) { 
    Y_VERIFY(this != op.Get()); 
 
    if (Dependencies.insert(op).second) { 
        op->Dependents.insert(this); 
    } 
} 
 
void TOperation::AddSpecialDependency(const TOperation::TPtr &op) { 
    Y_VERIFY(this != op.Get()); 
 
    if (SpecialDependencies.insert(op).second) { 
        op->SpecialDependents.insert(this); 
    } 
} 
 
void TOperation::AddImmediateConflict(const TOperation::TPtr &op) { 
    Y_VERIFY(this != op.Get()); 
    Y_VERIFY_DEBUG(!IsImmediate()); 
    Y_VERIFY_DEBUG(op->IsImmediate()); 
 
    if (HasFlag(TTxFlags::BlockingImmediateOps) || 
        HasFlag(TTxFlags::BlockingImmediateWrites) && !op->IsReadOnly()) 
    { 
        return op->AddDependency(this); 
    } 
 
    if (ImmediateConflicts.insert(op).second) { 
        op->PlannedConflicts.insert(this); 
    }
}

void TOperation::PromoteImmediateConflicts() { 
    for (auto& op : ImmediateConflicts) { 
        Y_VERIFY_DEBUG(op->PlannedConflicts.contains(this)); 
        op->PlannedConflicts.erase(this); 
        op->AddDependency(this); 
    } 
    ImmediateConflicts.clear(); 
}

void TOperation::PromoteImmediateWriteConflicts() { 
    for (auto it = ImmediateConflicts.begin(); it != ImmediateConflicts.end();) { 
        auto& op = *it; 
        if (op->IsReadOnly()) { 
            ++it; 
            continue; 
        } 
        Y_VERIFY_DEBUG(op->PlannedConflicts.contains(this)); 
        op->PlannedConflicts.erase(this); 
        op->AddDependency(this); 
        auto last = it; 
        ++it; 
        ImmediateConflicts.erase(last); 
    } 
} 
 
void TOperation::ClearDependents() { 
    for (auto &op : Dependents) { 
        Y_VERIFY_DEBUG(op->Dependencies.contains(this)); 
        op->Dependencies.erase(this); 
    } 
    Dependents.clear(); 
} 
 
void TOperation::ClearDependencies() { 
    for (auto &op : Dependencies) { 
        Y_VERIFY_DEBUG(op->Dependents.contains(this)); 
        op->Dependents.erase(this); 
    }
    Dependencies.clear();
}

void TOperation::ClearSpecialDependents() { 
    for (auto &op : SpecialDependents) { 
        Y_VERIFY_DEBUG(op->SpecialDependencies.contains(this)); 
        op->SpecialDependencies.erase(this); 
    }
    SpecialDependents.clear(); 
}

void TOperation::ClearSpecialDependencies() { 
    for (auto &op : SpecialDependencies) { 
        Y_VERIFY_DEBUG(op->SpecialDependents.contains(this)); 
        op->SpecialDependents.erase(this); 
    } 
    SpecialDependencies.clear(); 
}

void TOperation::ClearPlannedConflicts() { 
    for (auto &op : PlannedConflicts) { 
        Y_VERIFY_DEBUG(op->ImmediateConflicts.contains(this)); 
        op->ImmediateConflicts.erase(this); 
    }
    PlannedConflicts.clear(); 
}

void TOperation::ClearImmediateConflicts() { 
    for (auto &op : ImmediateConflicts) { 
        Y_VERIFY_DEBUG(op->PlannedConflicts.contains(this)); 
        op->PlannedConflicts.erase(this); 
    } 
    ImmediateConflicts.clear(); 
}

TString TOperation::DumpDependencies() const
{
    TStringStream ss;
    PrintDeps(this, ss);
    return ss.Str();
}

EExecutionUnitKind TOperation::GetCurrentUnit() const
{
    if (IsExecutionPlanFinished())
        return EExecutionUnitKind::Unspecified;
    return ExecutionPlan[CurrentUnit];
}

const TVector<EExecutionUnitKind> &TOperation::GetExecutionPlan() const
{
    return ExecutionPlan;
}

void TOperation::RewriteExecutionPlan(const TVector<EExecutionUnitKind> &plan)
{
    if (IsExecutionPlanFinished())
        ExecutionProfile.StartUnitAt = AppData()->TimeProvider->Now();
    else
        ExecutionPlan.resize(CurrentUnit + 1);
    ExecutionPlan.insert(ExecutionPlan.end(), plan.begin(), plan.end());
}

void TOperation::RewriteExecutionPlan(EExecutionUnitKind unit)
{
    if (IsExecutionPlanFinished())
        ExecutionProfile.StartUnitAt = AppData()->TimeProvider->Now();
    else
        ExecutionPlan.resize(CurrentUnit + 1);
    ExecutionPlan.push_back(unit);
}

bool TOperation::IsExecutionPlanFinished() const
{
    return CurrentUnit >= ExecutionPlan.size();
}

void TOperation::AdvanceExecutionPlan()
{
    TInstant now = AppData()->TimeProvider->Now();
    auto &profile = ExecutionProfile.UnitProfiles[ExecutionPlan[CurrentUnit]];

    profile.WaitTime = now - ExecutionProfile.StartUnitAt - profile.ExecuteTime
        - profile.CommitTime - profile.CompleteTime - profile.DelayedCommitTime;

    Y_VERIFY(!IsExecutionPlanFinished());
    ++CurrentUnit;

    ExecutionProfile.StartUnitAt = now;
}

void TOperation::Abort() 
{ 
    SetAbortedFlag(); 
} 
 
void TOperation::Abort(EExecutionUnitKind unit)
{
    SetAbortedFlag();
    RewriteExecutionPlan(unit);
}

TString TOperation::ExecutionProfileLogString(ui64 tabletId) const
{
    TStringStream ss;
    ss << "Execution profile for slow op " << *this << " at " << tabletId
       << " (W - Wait, E - Execution, C - commit, CP - Complete):";
    for (auto &unit : ExecutionPlan) {
        if (ExecutionProfile.UnitProfiles.contains(unit)) {
            auto &profile = ExecutionProfile.UnitProfiles.at(unit);
            ss << " " << unit << " W:" << profile.WaitTime
               << " E:" << profile.ExecuteTime << " C:" << profile.CommitTime
               << " CP:" << profile.CompleteTime;
        }
    }
    return ss.Str();
}

bool TOperation::HasRuntimeConflicts() const noexcept 
{ 
    // We may acquire some new dependencies at runtime 
    return !Dependencies.empty(); 
} 
 
} // namespace NDataShard
} // namespace NKikimr
