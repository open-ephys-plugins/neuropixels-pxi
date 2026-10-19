#pragma once

#include <juce_core/juce_core.h>
#include <array>
#include <vector>

namespace QuadBaseMapping
{
    constexpr int groupCount = 4;
    constexpr int channelsPerGroup = 384;

    struct Channel
    {
        int channel;
        int shank;
        int bank;
        int electrode;
    };

    using Mapping = std::vector<Channel>;

    inline juce::String validate (const Mapping& selection, const Mapping& geometry)
    {
        if (selection.size() != groupCount * channelsPerGroup)
            return "Expected 1536 channel mappings (384 per group), found " + juce::String ((int) selection.size()) + ".";

        std::array<bool, groupCount * channelsPerGroup> seen {};
        for (const auto& entry : selection)
        {
            if (entry.shank < 0 || entry.shank >= groupCount
                || entry.channel < 0 || entry.channel >= channelsPerGroup)
                return "Channel or group index is out of range.";

            const int index = entry.shank * channelsPerGroup + entry.channel;
            if (seen[index])
                return "Duplicate mapping for group " + juce::String (entry.shank) + ", channel " + juce::String (entry.channel) + ".";
            seen[index] = true;

            if (entry.electrode < 0 || entry.electrode >= (int) geometry.size())
                return "Electrode index is out of range.";

            const auto& electrode = geometry[(size_t) entry.electrode];
            if (electrode.electrode != entry.electrode
                || electrode.channel != entry.channel
                || electrode.shank != entry.shank
                || electrode.bank != entry.bank)
                return "Electrode does not match its channel, bank, and group.";
        }

        return {};
    }

    inline juce::Array<juce::XmlElement*> findProbeNodes (juce::XmlElement& xml,
                                                       const juce::String& serial,
                                                       int slot,
                                                       int port,
                                                       int dock)
    {
        juce::Array<juce::XmlElement*> matches;
        for (auto* node : xml.getChildIterator())
        {
            if (node->hasTagName ("NP_PROBE")
                && node->getStringAttribute ("probe_serial_number").equalsIgnoreCase (serial))
                matches.add (node);
        }

        if (! matches.isEmpty())
            return matches;

        juce::String savedSerial;
        for (auto* node : xml.getChildIterator())
        {
            const auto part = node->getStringAttribute ("probe_part_number");
            if (node->hasTagName ("NP_PROBE")
                && node->getIntAttribute ("slot", -1) == slot
                && node->getIntAttribute ("port", -1) == port
                && node->getIntAttribute ("dock", -1) == dock
                && (part.equalsIgnoreCase ("NP2020")
                    || part.equalsIgnoreCase ("NP2021")
                    || part.equalsIgnoreCase ("NP2022")))
            {
                if (matches.isEmpty())
                    savedSerial = node->getStringAttribute ("probe_serial_number");

                // Do not combine groups belonging to different saved probes.
                if (node->getStringAttribute ("probe_serial_number").equalsIgnoreCase (savedSerial))
                    matches.add (node);
            }
        }
        return matches;
    }

    inline bool readParameters (const juce::Array<juce::XmlElement*>& nodes,
                                const Mapping& geometry,
                                Mapping& selection,
                                juce::String& error)
    {
        if (nodes.size() != groupCount)
        {
            error = "Expected four saved Quad Base groups, found " + juce::String (nodes.size()) + ".";
            return false;
        }

        Mapping candidate;
        std::array<bool, groupCount> seen {};
        for (auto* node : nodes)
        {
            const auto shankText = node->getStringAttribute ("shank");
            if (shankText.length() != 1 || ! shankText.containsOnly ("0123"))
            {
                error = "Missing or invalid saved group index.";
                return false;
            }

            const int shank = shankText.getIntValue();
            if (seen[shank])
            {
                error = "Duplicate saved group " + juce::String (shank) + ".";
                return false;
            }
            seen[shank] = true;

            auto* channels = node->getChildByName ("CHANNELS");
            if (channels == nullptr || channels->getNumAttributes() != channelsPerGroup)
            {
                error = "Group " + juce::String (shank) + " must contain 384 saved channels.";
                return false;
            }

            for (int channel = 0; channel < channelsPerGroup; ++channel)
            {
                const auto id = "CH" + juce::String (channel) + "_" + juce::String (shank);
                const auto bankText = channels->getStringAttribute (id);
                // Missing attributes must not silently become Bank A.
                if (bankText.length() != 1 || ! bankText.containsOnly ("0123"))
                {
                    error = "Missing or invalid bank for " + id + ".";
                    return false;
                }

                const int bank = bankText.getIntValue();
                int electrode = -1;
                for (const auto& entry : geometry)
                {
                    if (entry.channel == channel && entry.shank == shank && entry.bank == bank)
                    {
                        electrode = entry.electrode;
                        break;
                    }
                }
                candidate.push_back ({ channel, shank, bank, electrode });
            }
        }

        error = validate (candidate, geometry);
        if (error.isNotEmpty())
            return false;

        selection = std::move (candidate);
        return true;
    }
}
