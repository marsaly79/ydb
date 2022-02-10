# Generated by the gRPC Python protocol compiler plugin. DO NOT EDIT!
"""Client and server classes corresponding to protobuf-defined services."""
import grpc

from ydb.public.api.protos.draft import yq_private_pb2 as ydb_dot_public_dot_api_dot_protos_dot_draft_dot_yq__private__pb2


class YqPrivateTaskServiceStub(object): 
    """Missing associated documentation comment in .proto file."""

    def __init__(self, channel):
        """Constructor.

        Args:
            channel: A grpc.Channel.
        """
        self.GetTask = channel.unary_unary(
                '/Yq.Private.V1.YqPrivateTaskService/GetTask', 
                request_serializer=ydb_dot_public_dot_api_dot_protos_dot_draft_dot_yq__private__pb2.GetTaskRequest.SerializeToString,
                response_deserializer=ydb_dot_public_dot_api_dot_protos_dot_draft_dot_yq__private__pb2.GetTaskResponse.FromString,
                )
        self.PingTask = channel.unary_unary(
                '/Yq.Private.V1.YqPrivateTaskService/PingTask', 
                request_serializer=ydb_dot_public_dot_api_dot_protos_dot_draft_dot_yq__private__pb2.PingTaskRequest.SerializeToString,
                response_deserializer=ydb_dot_public_dot_api_dot_protos_dot_draft_dot_yq__private__pb2.PingTaskResponse.FromString,
                )
        self.WriteTaskResult = channel.unary_unary(
                '/Yq.Private.V1.YqPrivateTaskService/WriteTaskResult', 
                request_serializer=ydb_dot_public_dot_api_dot_protos_dot_draft_dot_yq__private__pb2.WriteTaskResultRequest.SerializeToString,
                response_deserializer=ydb_dot_public_dot_api_dot_protos_dot_draft_dot_yq__private__pb2.WriteTaskResultResponse.FromString,
                )
        self.NodesHealthCheck = channel.unary_unary(
                '/Yq.Private.V1.YqPrivateTaskService/NodesHealthCheck', 
                request_serializer=ydb_dot_public_dot_api_dot_protos_dot_draft_dot_yq__private__pb2.NodesHealthCheckRequest.SerializeToString,
                response_deserializer=ydb_dot_public_dot_api_dot_protos_dot_draft_dot_yq__private__pb2.NodesHealthCheckResponse.FromString,
                )


class YqPrivateTaskServiceServicer(object): 
    """Missing associated documentation comment in .proto file."""

    def GetTask(self, request, context):
        """gets new task
        """
        context.set_code(grpc.StatusCode.UNIMPLEMENTED)
        context.set_details('Method not implemented!')
        raise NotImplementedError('Method not implemented!')

    def PingTask(self, request, context):
        """pings new task (also can update metadata)
        """
        context.set_code(grpc.StatusCode.UNIMPLEMENTED)
        context.set_details('Method not implemented!')
        raise NotImplementedError('Method not implemented!')

    def WriteTaskResult(self, request, context):
        """writes rows
        """
        context.set_code(grpc.StatusCode.UNIMPLEMENTED)
        context.set_details('Method not implemented!')
        raise NotImplementedError('Method not implemented!')

    def NodesHealthCheck(self, request, context):
        """Nodes
        """
        context.set_code(grpc.StatusCode.UNIMPLEMENTED)
        context.set_details('Method not implemented!')
        raise NotImplementedError('Method not implemented!')


def add_YqPrivateTaskServiceServicer_to_server(servicer, server): 
    rpc_method_handlers = {
            'GetTask': grpc.unary_unary_rpc_method_handler(
                    servicer.GetTask,
                    request_deserializer=ydb_dot_public_dot_api_dot_protos_dot_draft_dot_yq__private__pb2.GetTaskRequest.FromString,
                    response_serializer=ydb_dot_public_dot_api_dot_protos_dot_draft_dot_yq__private__pb2.GetTaskResponse.SerializeToString,
            ),
            'PingTask': grpc.unary_unary_rpc_method_handler(
                    servicer.PingTask,
                    request_deserializer=ydb_dot_public_dot_api_dot_protos_dot_draft_dot_yq__private__pb2.PingTaskRequest.FromString,
                    response_serializer=ydb_dot_public_dot_api_dot_protos_dot_draft_dot_yq__private__pb2.PingTaskResponse.SerializeToString,
            ),
            'WriteTaskResult': grpc.unary_unary_rpc_method_handler(
                    servicer.WriteTaskResult,
                    request_deserializer=ydb_dot_public_dot_api_dot_protos_dot_draft_dot_yq__private__pb2.WriteTaskResultRequest.FromString,
                    response_serializer=ydb_dot_public_dot_api_dot_protos_dot_draft_dot_yq__private__pb2.WriteTaskResultResponse.SerializeToString,
            ),
            'NodesHealthCheck': grpc.unary_unary_rpc_method_handler(
                    servicer.NodesHealthCheck,
                    request_deserializer=ydb_dot_public_dot_api_dot_protos_dot_draft_dot_yq__private__pb2.NodesHealthCheckRequest.FromString,
                    response_serializer=ydb_dot_public_dot_api_dot_protos_dot_draft_dot_yq__private__pb2.NodesHealthCheckResponse.SerializeToString,
            ),
    }
    generic_handler = grpc.method_handlers_generic_handler(
            'Yq.Private.V1.YqPrivateTaskService', rpc_method_handlers) 
    server.add_generic_rpc_handlers((generic_handler,))


 # This class is part of an EXPERIMENTAL API.
class YqPrivateTaskService(object): 
    """Missing associated documentation comment in .proto file."""

    @staticmethod
    def GetTask(request,
            target,
            options=(),
            channel_credentials=None,
            call_credentials=None,
            insecure=False,
            compression=None,
            wait_for_ready=None,
            timeout=None,
            metadata=None):
        return grpc.experimental.unary_unary(request, target, '/Yq.Private.V1.YqPrivateTaskService/GetTask', 
            ydb_dot_public_dot_api_dot_protos_dot_draft_dot_yq__private__pb2.GetTaskRequest.SerializeToString,
            ydb_dot_public_dot_api_dot_protos_dot_draft_dot_yq__private__pb2.GetTaskResponse.FromString,
            options, channel_credentials,
            insecure, call_credentials, compression, wait_for_ready, timeout, metadata)

    @staticmethod
    def PingTask(request,
            target,
            options=(),
            channel_credentials=None,
            call_credentials=None,
            insecure=False,
            compression=None,
            wait_for_ready=None,
            timeout=None,
            metadata=None):
        return grpc.experimental.unary_unary(request, target, '/Yq.Private.V1.YqPrivateTaskService/PingTask', 
            ydb_dot_public_dot_api_dot_protos_dot_draft_dot_yq__private__pb2.PingTaskRequest.SerializeToString,
            ydb_dot_public_dot_api_dot_protos_dot_draft_dot_yq__private__pb2.PingTaskResponse.FromString,
            options, channel_credentials,
            insecure, call_credentials, compression, wait_for_ready, timeout, metadata)

    @staticmethod
    def WriteTaskResult(request,
            target,
            options=(),
            channel_credentials=None,
            call_credentials=None,
            insecure=False,
            compression=None,
            wait_for_ready=None,
            timeout=None,
            metadata=None):
        return grpc.experimental.unary_unary(request, target, '/Yq.Private.V1.YqPrivateTaskService/WriteTaskResult', 
            ydb_dot_public_dot_api_dot_protos_dot_draft_dot_yq__private__pb2.WriteTaskResultRequest.SerializeToString,
            ydb_dot_public_dot_api_dot_protos_dot_draft_dot_yq__private__pb2.WriteTaskResultResponse.FromString,
            options, channel_credentials,
            insecure, call_credentials, compression, wait_for_ready, timeout, metadata)

    @staticmethod
    def NodesHealthCheck(request,
            target,
            options=(),
            channel_credentials=None,
            call_credentials=None,
            insecure=False,
            compression=None,
            wait_for_ready=None,
            timeout=None,
            metadata=None):
        return grpc.experimental.unary_unary(request, target, '/Yq.Private.V1.YqPrivateTaskService/NodesHealthCheck', 
            ydb_dot_public_dot_api_dot_protos_dot_draft_dot_yq__private__pb2.NodesHealthCheckRequest.SerializeToString,
            ydb_dot_public_dot_api_dot_protos_dot_draft_dot_yq__private__pb2.NodesHealthCheckResponse.FromString,
            options, channel_credentials,
            insecure, call_credentials, compression, wait_for_ready, timeout, metadata)
