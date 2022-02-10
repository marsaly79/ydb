#pragma once

#include "defs.h"

#include <library/cpp/actors/core/actor.h>
#include <library/cpp/actors/interconnect/interconnect.h>
#include <ydb/core/base/domain.h>
#include <ydb/core/protos/node_broker.pb.h>

namespace NKikimr {
namespace NNodeBroker {

// Create nameservice for static node.
IActor *CreateDynamicNameserver(const TIntrusivePtr<TTableNameserverSetup> &setup,
                                ui32 poolId = 0);

// Create nameservice for dynamic node providing its info.
IActor *CreateDynamicNameserver(const TIntrusivePtr<TTableNameserverSetup> &setup,
                                const NKikimrNodeBroker::TNodeInfo &node,
                                const TDomainsInfo &domains, 
                                ui32 poolId = 0);

} // NNodeBroker
} // NKikimr
