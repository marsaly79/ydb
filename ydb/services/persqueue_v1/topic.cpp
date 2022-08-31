#include "topic.h"

#include <ydb/core/base/appdata.h>
#include <ydb/core/base/counters.h>
#include <ydb/core/grpc_services/rpc_calls.h>
#include <ydb/core/grpc_services/grpc_helper.h>
#include <ydb/core/tx/scheme_board/cache.h>

#include "grpc_pq_read.h"
#include "grpc_pq_write.h"
#include "grpc_pq_schema.h"

namespace NKikimr {
namespace NGRpcService {
namespace V1 {

static const ui32 TopicWriteSessionsMaxCount = 1000000;
static const ui32 TopicReadSessionsMaxCount = 100000;

TGRpcTopicService::TGRpcTopicService(NActors::TActorSystem *system, TIntrusivePtr<::NMonitoring::TDynamicCounters> counters, const NActors::TActorId& schemeCache,const NActors::TActorId& grpcRequestProxy, bool rlAllowed)
    : TGrpcServiceBase<Ydb::Topic::V1::TopicService>(system, counters, grpcRequestProxy, rlAllowed)
    , SchemeCache(schemeCache)
{ }

void TGRpcTopicService::InitService(grpc::ServerCompletionQueue *cq, NGrpc::TLoggerPtr logger) {
    CQ_ = cq;
    InitNewSchemeCacheActor();

    if (ActorSystem_->AppData<TAppData>()->PQConfig.GetEnabled()) {

        IActor* writeSvc = NGRpcProxy::V1::CreatePQWriteService(SchemeCache, Counters_, TopicWriteSessionsMaxCount);
        TActorId actorId = ActorSystem_->Register(writeSvc, TMailboxType::HTSwap, ActorSystem_->AppData<TAppData>()->UserPoolId);
        ActorSystem_->RegisterLocalService(NGRpcProxy::V1::GetPQWriteServiceActorID(), actorId);

        IActor* readSvc = NGRpcProxy::V1::CreatePQReadService(SchemeCache, NewSchemeCache, Counters_, TopicReadSessionsMaxCount);
        actorId = ActorSystem_->Register(readSvc, TMailboxType::HTSwap, ActorSystem_->AppData<TAppData>()->UserPoolId);
        ActorSystem_->RegisterLocalService(NGRpcProxy::V1::GetPQReadServiceActorID(), actorId);

        IActor* schemaSvc = NGRpcProxy::V1::CreatePQSchemaService(SchemeCache, Counters_);
        actorId = ActorSystem_->Register(schemaSvc, TMailboxType::HTSwap, ActorSystem_->AppData<TAppData>()->UserPoolId);
        ActorSystem_->RegisterLocalService(NGRpcProxy::V1::GetPQSchemaServiceActorID(), actorId);

        SetupIncomingRequests(std::move(logger));
    }
}

void TGRpcTopicService::InitNewSchemeCacheActor() {
    auto appData = ActorSystem_->AppData<TAppData>();
    auto cacheCounters = GetServiceCounters(Counters_, "pqproxy|schemecache");
    auto cacheConfig = MakeIntrusive<NSchemeCache::TSchemeCacheConfig>(appData, cacheCounters);
    NewSchemeCache = ActorSystem_->Register(CreateSchemeBoardSchemeCache(cacheConfig.Get()),
        TMailboxType::HTSwap, ActorSystem_->AppData<TAppData>()->UserPoolId);
}

void TGRpcTopicService::SetupIncomingRequests(NGrpc::TLoggerPtr logger) {

    auto getCounterBlock = NKikimr::NGRpcService::CreateCounterCb(Counters_, ActorSystem_);

    {
        using TBiRequest = Ydb::Topic::StreamWriteMessage::FromClient;

        using TBiResponse = Ydb::Topic::StreamWriteMessage::FromServer;

        using TStreamGRpcRequest = NGRpcServer::TGRpcStreamingRequest<
                    TBiRequest,
                    TBiResponse,
                    TGRpcTopicService,
                    NKikimrServices::GRPC_SERVER>;


        TStreamGRpcRequest::Start(this, this->GetService(), CQ_, &Ydb::Topic::V1::TopicService::AsyncService::RequestStreamWrite,
                    [this](TIntrusivePtr<TStreamGRpcRequest::IContext> context) {
                        ActorSystem_->Send(GRpcRequestProxyId_, new NKikimr::NGRpcService::TEvStreamTopicWriteRequest(context));
                    },
                    *ActorSystem_, "TopicService/StreamWrite", getCounterBlock("topic", "StreamWrite", true, true), nullptr
                );
    }

    {
        using TBiRequest = Ydb::Topic::StreamReadMessage::FromClient;

        using TBiResponse = Ydb::Topic::StreamReadMessage::FromServer;

        using TStreamGRpcRequest = NGRpcServer::TGRpcStreamingRequest<
                    TBiRequest,
                    TBiResponse,
                    TGRpcTopicService,
                    NKikimrServices::GRPC_SERVER>;


        TStreamGRpcRequest::Start(this, this->GetService(), CQ_, &Ydb::Topic::V1::TopicService::AsyncService::RequestStreamRead,
                    [this](TIntrusivePtr<TStreamGRpcRequest::IContext> context) {
                        ActorSystem_->Send(GRpcRequestProxyId_, new NKikimr::NGRpcService::TEvStreamTopicReadRequest(context));
                    },
                    *ActorSystem_, "TopicService/StreamRead", getCounterBlock("topic", "StreamRead", true, true), nullptr
                );
    }

#ifdef ADD_REQUEST
#error ADD_REQUEST macro already defined
#endif
#define ADD_REQUEST(NAME, SVC, IN, OUT, ACTION) \
    MakeIntrusive<TGRpcRequest<Ydb::Topic::IN, Ydb::Topic::OUT, NGRpcService::V1::TGRpcTopicService>>(this, this->GetService(), CQ_, \
        [this](NGrpc::IRequestContextBase *ctx) { \
            NGRpcService::ReportGrpcReqToMon(*ActorSystem_, ctx->GetPeer()); \
            ACTION; \
        }, &Ydb::Topic::V1::SVC::AsyncService::Request ## NAME, \
        "TopicService/"#NAME, logger, getCounterBlock("topic", #NAME))->Run();

    ADD_REQUEST(DropTopic, TopicService, DropTopicRequest, DropTopicResponse, {
            ActorSystem_->Send(GRpcRequestProxyId_, new NGRpcService::TEvDropTopicRequest(ctx));
        })
    ADD_REQUEST(CreateTopic, TopicService, CreateTopicRequest, CreateTopicResponse, {
            ActorSystem_->Send(GRpcRequestProxyId_, new NGRpcService::TEvCreateTopicRequest(ctx));
        })
    ADD_REQUEST(AlterTopic, TopicService, AlterTopicRequest, AlterTopicResponse, {
            ActorSystem_->Send(GRpcRequestProxyId_, new NGRpcService::TEvAlterTopicRequest(ctx));
        })
    ADD_REQUEST(DescribeTopic, TopicService, DescribeTopicRequest, DescribeTopicResponse, {
            ActorSystem_->Send(GRpcRequestProxyId_, new NGRpcService::TEvDescribeTopicRequest(ctx));
        })

#undef ADD_REQUEST


}

void TGRpcTopicService::StopService() noexcept {
    TGrpcServiceBase::StopService();
}

} // V1
} // namespace NGRpcService
} // namespace NKikimr
