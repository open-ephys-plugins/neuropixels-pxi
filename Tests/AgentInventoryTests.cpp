#include "AgentInventory.h"

#include <iostream>
#include <set>
#include <string>
#include <utility>

namespace
{
int failures = 0;

void expectEqual (const std::string& actual,
                  const std::string& expected,
                  const char* testName)
{
    if (actual == expected)
        return;

    ++failures;
    std::cerr << "FAIL " << testName << "\nexpected: " << expected
              << "\nactual:   " << actual << "\n";
}

void expectTrue (bool condition, const char* testName)
{
    if (condition)
        return;

    ++failures;
    std::cerr << "FAIL " << testName << "\n";
}
}

int main()
{
    using namespace neuropix::agent;

    Inventory inventory;
    inventory.plugin = "Neuropix-PXI";
    inventory.version = "2.1.1";
    inventory.basestations.push_back ({ 2, "PXI", "BS-0001", "3.0226" });
    inventory.probes.push_back ({ "Probe-A",
                                  "Neuropixels 2.0 Single Shank",
                                  { 2, 1, 2 },
                                  "PRB2_1_2_0640_0",
                                  "123456789",
                                  true,
                                  true,
                                  ProbeStatus::CONNECTED,
                                  false });

    const std::string expectedJson =
        "{\"plugin\":\"Neuropix-PXI\",\"version\":\"2.1.1\",\"schema_version\":1,"
        "\"basestations\":[{\"slot\":2,\"type\":\"PXI\",\"part_number\":\"BS-0001\","
        "\"firmware_version\":\"3.0226\"}],\"probes\":[{\"name\":\"Probe-A\","
        "\"type\":\"Neuropixels 2.0 Single Shank\",\"slot\":2,\"port\":1,\"dock\":2,"
        "\"part_number\":\"PRB2_1_2_0640_0\",\"serial_number\":\"123456789\","
        "\"is_calibrated\":true,\"supported\":true,\"status\":\"CONNECTED\","
        "\"disabled\":false}]}";
    expectEqual (serializeInventory (inventory),
                 expectedJson,
                 "golden JSON preserves NP INFO and exposes read-only inventory");

    Inventory escaped;
    escaped.plugin = "Neuropix-\"PXI\"";
    escaped.version = "2.1.1\npreview";
    expectEqual (serializeInventory (escaped),
                 "{\"plugin\":\"Neuropix-\\\"PXI\\\"\",\"version\":\"2.1.1\\npreview\","
                 "\"schema_version\":1,\"basestations\":[],\"probes\":[]}",
                 "serializer JSON-escapes readable device strings");

    const Locator first { 2, 1, 1 };
    const Locator secondDock { 2, 1, 2 };
    const Locator secondPort { 2, 2, 1 };
    const Locator secondSlot { 3, 1, 1 };

    const auto firstId = probeAutomationId (first);
    expectEqual (firstId,
                 "oe.plugin.neuropix_pxi.inventory.probe.slot_2.port_1.dock_1",
                 "probe UIA ID is namespaced and locator based");
    expectEqual (probeAutomationId (first),
                 firstId,
                 "probe UIA ID is stable for the same locator");

    const std::set<std::string> probeIds {
        firstId,
        probeAutomationId (secondDock),
        probeAutomationId (secondPort),
        probeAutomationId (secondSlot)
    };
    expectTrue (probeIds.size() == 4,
                "probe UIA IDs are unique across slot port dock locators");

    expectEqual (basestationAutomationId (2),
                 "oe.plugin.neuropix_pxi.inventory.basestation.slot_2",
                 "basestation UIA ID is namespaced and slot based");
    expectTrue (basestationAutomationId (2) != basestationAutomationId (3),
                "basestation UIA IDs are unique across slots");
    expectTrue (basestationAutomationId (2) != firstId,
                "basestation and probe namespaces cannot collide");

    const auto status = probeStatusText (inventory.probes.front());
    expectEqual (status,
                 "slot=2; port=1; dock=2; type=Neuropixels 2.0 Single Shank; "
                 "part_number=PRB2_1_2_0640_0; serial_number=123456789; "
                 "supported=true; status=CONNECTED; disabled=false; calibrated=true",
                 "probe UIA dynamic status is readable separately from its stable ID");
    expectEqual (basestationStatusText (inventory.basestations.front()),
                 "slot=2; type=PXI; part_number=BS-0001; firmware_version=3.0226",
                 "basestation UIA status exposes only source-backed readable fields");
    expectTrue (firstId.find ("supported") == std::string::npos
                    && firstId.find ("status") == std::string::npos
                    && firstId.find ("calibrated") == std::string::npos,
                "probe UIA stable ID excludes dynamic status values");

    const std::pair<ProbeStatus, const char*> statuses[] {
        { ProbeStatus::DISCONNECTED, "DISCONNECTED" },
        { ProbeStatus::CONNECTING, "CONNECTING" },
        { ProbeStatus::CONNECTED, "CONNECTED" },
        { ProbeStatus::UPDATING, "UPDATING" },
        { ProbeStatus::ACQUIRING, "ACQUIRING" },
        { ProbeStatus::RECORDING, "RECORDING" },
        { ProbeStatus::DISABLED, "DISABLED" }
    };

    for (const auto& [value, expected] : statuses)
        expectEqual (probeStatusToString (value),
                     expected,
                     "every official SourceStatus has a stable string");

    if (failures != 0)
        return 1;

    std::cout << "PASS 18 inventory contract checks\n";
    return 0;
}
