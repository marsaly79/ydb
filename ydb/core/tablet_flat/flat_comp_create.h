#pragma once 
 
#include "flat_comp.h" 
#include "util_fmt_line.h" 
 
#include <library/cpp/time_provider/time_provider.h>
 
namespace NKikimr { 
namespace NTable { 
 
    THolder<ICompactionStrategy> CreateGenCompactionStrategy( 
            ui32 table, 
            ICompactionBackend* backend, 
            IResourceBroker* broker, 
            ITimeProvider* time, 
            TString taskNameSuffix); 
 
    THolder<ICompactionStrategy> CreateShardedCompactionStrategy( 
            ui32 table, 
            ICompactionBackend* backend, 
            IResourceBroker* broker, 
            NUtil::ILogger* logger, 
            ITimeProvider* time, 
            TString taskNameSuffix); 
 
} 
} 
