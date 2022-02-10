#pragma once

#include <ydb/core/kqp/common/kqp_transform.h>
#include <ydb/core/kqp/opt/logical/kqp_opt_log.h>
#include <ydb/core/kqp/opt/peephole/kqp_opt_peephole.h>
#include <ydb/core/kqp/opt/physical/kqp_opt_phy.h>

namespace NKikimr::NKqp::NOpt {

struct TKqpOptimizeContext : public TSimpleRefCount<TKqpOptimizeContext> { 
    TKqpOptimizeContext(const TString& cluster, const NYql::TKikimrConfiguration::TPtr& config,
        const TIntrusivePtr<NYql::TKikimrQueryContext> queryCtx, const TIntrusivePtr<NYql::TKikimrTablesData>& tables)
        : Cluster(cluster)
        , Config(config)
        , QueryCtx(queryCtx)
        , Tables(tables)
    {
        YQL_ENSURE(QueryCtx);
        YQL_ENSURE(Tables);
    }

    TString Cluster;
    const NYql::TKikimrConfiguration::TPtr Config;
    const TIntrusivePtr<NYql::TKikimrQueryContext> QueryCtx;
    const TIntrusivePtr<NYql::TKikimrTablesData> Tables;

    bool IsDataQuery() const {
        return QueryCtx->Type == NYql::EKikimrQueryType::Dml;
    }

    bool IsScanQuery() const {
        return QueryCtx->Type == NYql::EKikimrQueryType::Scan;
    }
};

struct TKqpBuildQueryContext : TThrRefBase {
    TKqpBuildQueryContext() {}

    TVector<NYql::NNodes::TKqpPhysicalTx> PhysicalTxs;

    void Reset() {
        PhysicalTxs.clear();
    }
};

bool IsKqpEffectsStage(const NYql::NNodes::TDqStageBase& stage);

TMaybe<NYql::NNodes::TKqlQuery> BuildKqlQuery(NYql::NNodes::TKiDataQuery query, const NYql::TKikimrTablesData& tablesData,
    NYql::TExprContext& ctx, bool withSystemColumns, const TIntrusivePtr<TKqpOptimizeContext>& kqpCtx);

TAutoPtr<NYql::IGraphTransformer> CreateKqpFinalizingOptTransformer(); 
TAutoPtr<NYql::IGraphTransformer> CreateKqpQueryPhasesTransformer();
TAutoPtr<NYql::IGraphTransformer> CreateKqpQueryEffectsTransformer(const TIntrusivePtr<TKqpOptimizeContext>& kqpCtx);
TAutoPtr<NYql::IGraphTransformer> CreateKqpCheckPhysicalQueryTransformer(); 

TAutoPtr<NYql::IGraphTransformer> CreateKqpBuildTxsTransformer(const TIntrusivePtr<TKqpOptimizeContext>& kqpCtx, 
    const TIntrusivePtr<TKqpBuildQueryContext>& buildCtx, TAutoPtr<NYql::IGraphTransformer>&& typeAnnTransformer, 
    NYql::TTypeAnnotationContext& typesCtx, NYql::TKikimrConfiguration::TPtr& config); 

} // namespace NKikimr::NKqp::NOpt
