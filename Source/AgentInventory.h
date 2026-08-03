#pragma once

#include <string>
#include <vector>

namespace neuropix::agent
{
struct Locator
{
    int slot;
    int port;
    int dock;
};

struct ProbeInventory
{
    std::string name;
    std::string type;
    Locator locator;
    std::string partNumber;
    std::string serialNumber;
    bool calibrated;
    bool available;
    bool disabled;
};

struct BasestationInventory
{
    int slot;
    std::string type;
    std::string partNumber;
    std::string firmwareVersion;
};

struct Inventory
{
    std::string plugin;
    std::string version;
    std::vector<BasestationInventory> basestations;
    std::vector<ProbeInventory> probes;
};

std::string serializeInventory (const Inventory& inventory);
std::string basestationAutomationId (int slot);
std::string probeAutomationId (const Locator& locator);
std::string basestationStatusText (const BasestationInventory& basestation);
std::string probeStatusText (const ProbeInventory& probe);
}
