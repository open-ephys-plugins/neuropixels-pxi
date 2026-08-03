#pragma once

#include "AgentInventory.h"
#include "NeuropixComponents.h"

namespace neuropix::agent
{
inline ProbeStatus probeStatusFromSource (SourceStatus status)
{
    switch (status)
    {
        case SourceStatus::DISCONNECTED: return ProbeStatus::DISCONNECTED;
        case SourceStatus::CONNECTING: return ProbeStatus::CONNECTING;
        case SourceStatus::CONNECTED: return ProbeStatus::CONNECTED;
        case SourceStatus::UPDATING: return ProbeStatus::UPDATING;
        case SourceStatus::ACQUIRING: return ProbeStatus::ACQUIRING;
        case SourceStatus::RECORDING: return ProbeStatus::RECORDING;
        case SourceStatus::DISABLED: return ProbeStatus::DISABLED;
    }

    return ProbeStatus::DISCONNECTED;
}
}
