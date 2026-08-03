#include "AgentInventory.h"

#include <iomanip>
#include <sstream>

namespace neuropix::agent
{
namespace
{
std::string escapeJson (const std::string& value)
{
    std::ostringstream escaped;

    for (const unsigned char character : value)
    {
        switch (character)
        {
            case '\"': escaped << "\\\""; break;
            case '\\': escaped << "\\\\"; break;
            case '\b': escaped << "\\b"; break;
            case '\f': escaped << "\\f"; break;
            case '\n': escaped << "\\n"; break;
            case '\r': escaped << "\\r"; break;
            case '\t': escaped << "\\t"; break;
            default:
                if (character < 0x20)
                    escaped << "\\u" << std::hex << std::setw (4)
                            << std::setfill ('0') << static_cast<int> (character)
                            << std::dec;
                else
                    escaped << character;
        }
    }

    return escaped.str();
}

void appendString (std::ostringstream& output,
                   const char* name,
                   const std::string& value)
{
    output << '\"' << name << "\":\"" << escapeJson (value) << '\"';
}
}

std::string serializeInventory (const Inventory& inventory)
{
    std::ostringstream output;
    output << '{';
    appendString (output, "plugin", inventory.plugin);
    output << ',';
    appendString (output, "version", inventory.version);
    output << ",\"schema_version\":1,\"basestations\":[";

    for (std::size_t index = 0; index < inventory.basestations.size(); ++index)
    {
        if (index != 0)
            output << ',';

        const auto& basestation = inventory.basestations[index];
        output << "{\"slot\":" << basestation.slot << ',';
        appendString (output, "type", basestation.type);
        output << ',';
        appendString (output, "part_number", basestation.partNumber);
        output << ',';
        appendString (output, "firmware_version", basestation.firmwareVersion);
        output << '}';
    }

    output << "],\"probes\":[";

    for (std::size_t index = 0; index < inventory.probes.size(); ++index)
    {
        if (index != 0)
            output << ',';

        const auto& probe = inventory.probes[index];
        output << '{';
        appendString (output, "name", probe.name);
        output << ',';
        appendString (output, "type", probe.type);
        output << ",\"slot\":" << probe.locator.slot
               << ",\"port\":" << probe.locator.port
               << ",\"dock\":" << probe.locator.dock << ',';
        appendString (output, "part_number", probe.partNumber);
        output << ',';
        appendString (output, "serial_number", probe.serialNumber);
        output << ",\"is_calibrated\":" << (probe.calibrated ? "true" : "false")
               << ",\"supported\":" << (probe.supported ? "true" : "false")
               << ",\"status\":\"" << probeStatusToString (probe.status) << '\"'
               << ",\"disabled\":" << (probe.disabled ? "true" : "false")
               << '}';
    }

    output << "]}";
    return output.str();
}

std::string basestationAutomationId (int slot)
{
    return "oe.plugin.neuropix_pxi.inventory.basestation.slot_"
           + std::to_string (slot);
}

std::string probeAutomationId (const Locator& locator)
{
    return "oe.plugin.neuropix_pxi.inventory.probe.slot_"
           + std::to_string (locator.slot)
           + ".port_" + std::to_string (locator.port)
           + ".dock_" + std::to_string (locator.dock);
}

std::string probeStatusToString (ProbeStatus status)
{
    switch (status)
    {
        case ProbeStatus::DISCONNECTED: return "DISCONNECTED";
        case ProbeStatus::CONNECTING: return "CONNECTING";
        case ProbeStatus::CONNECTED: return "CONNECTED";
        case ProbeStatus::UPDATING: return "UPDATING";
        case ProbeStatus::ACQUIRING: return "ACQUIRING";
        case ProbeStatus::RECORDING: return "RECORDING";
        case ProbeStatus::DISABLED: return "DISABLED";
    }

    return "UNKNOWN";
}

std::string basestationStatusText (const BasestationInventory& basestation)
{
    return "slot=" + std::to_string (basestation.slot)
           + "; type=" + basestation.type
           + "; part_number=" + basestation.partNumber
           + "; firmware_version=" + basestation.firmwareVersion;
}

std::string probeStatusText (const ProbeInventory& probe)
{
    return "slot=" + std::to_string (probe.locator.slot)
           + "; port=" + std::to_string (probe.locator.port)
           + "; dock=" + std::to_string (probe.locator.dock)
           + "; type=" + probe.type
           + "; part_number=" + probe.partNumber
           + "; serial_number=" + probe.serialNumber
           + "; supported=" + (probe.supported ? "true" : "false")
           + "; status=" + probeStatusToString (probe.status)
           + "; disabled=" + (probe.disabled ? "true" : "false")
           + "; calibrated=" + (probe.calibrated ? "true" : "false");
}
}
