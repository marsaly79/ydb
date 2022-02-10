#include "kqp_runtime_impl.h"

#include <ydb/core/kqp/common/kqp_resolve.h>
#include <ydb/core/scheme_types/scheme_type_registry.h>

#include <ydb/library/yql/minikql/computation/mkql_computation_node_holders.h>

namespace NKikimr {
namespace NKqp {

using namespace NMiniKQL;

namespace {

using namespace NYql; 
using namespace NDq; 
using namespace NUdf; 
 
 
class TKqpOutputRangePartitionConsumer : public IDqOutputConsumer { 
public:
    TKqpOutputRangePartitionConsumer(const TTypeEnvironment& typeEnv, 
        TVector<NYql::NDq::IDqOutput::TPtr>&& outputs, TVector<TKqpRangePartition>&& partitions,
        TVector<TDataTypeId>&& keyColumnTypes, TVector<ui32>&& keyColumnIndices) 
        : TypeEnv(typeEnv) 
        , Outputs(std::move(outputs))
        , Partitions(std::move(partitions))
        , KeyColumnTypes(std::move(keyColumnTypes))
        , KeyColumnIndices(std::move(keyColumnIndices))
    { 
        MKQL_ENSURE_S(!Partitions.empty());
        MKQL_ENSURE_S(KeyColumnTypes.size() == KeyColumnIndices.size());

        SortPartitions(Partitions, KeyColumnTypes, [](const auto& partition) { return partition.Range; });
    }

    bool IsFull() const override { 
        return AnyOf(Outputs, [](const auto& output) { return output->IsFull(); });
    } 

    void Consume(TUnboxedValue&& value) final { 
        ui32 partitionIndex = FindKeyPartitionIndex(TypeEnv, value, Partitions, KeyColumnTypes, KeyColumnIndices, 
                [](const auto& partition) { return partition.Range; }); 

        Outputs[partitionIndex]->Push(std::move(value));
    } 

    void Finish() final { 
        for (auto& output : Outputs) {
            output->Finish();
        }
    }

private:
    const TTypeEnvironment& TypeEnv; 
    TVector<NYql::NDq::IDqOutput::TPtr> Outputs;
    TVector<TKqpRangePartition> Partitions;
    TVector<TDataTypeId> KeyColumnTypes; 
    TVector<ui32> KeyColumnIndices;
};

} // namespace

NYql::NDq::IDqOutputConsumer::TPtr CreateOutputRangePartitionConsumer( 
    TVector<NYql::NDq::IDqOutput::TPtr>&& outputs, TVector<TKqpRangePartition>&& partitions,
    TVector<NUdf::TDataTypeId>&& keyColumnTypes, TVector<ui32>&& keyColumnIndices, 
    const NMiniKQL::TTypeEnvironment& typeEnv) 
{ 
    return MakeIntrusive<TKqpOutputRangePartitionConsumer>(typeEnv, std::move(outputs), std::move(partitions),
            std::move(keyColumnTypes), std::move(keyColumnIndices)); 
}

} // namespace NKqp 
} // namespace NKikimr 
