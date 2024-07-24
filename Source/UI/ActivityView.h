/*
    ------------------------------------------------------------------

    This file is part of the Open Ephys GUI
    Copyright (C) 2024 Open Ephys

    ------------------------------------------------------------------

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef __ACTIVITYVIEW_H__
#define __ACTIVITYVIEW_H__

#include <VisualizerEditorHeaders.h>
#include <limits>
#include <unordered_map>
#include <vector>

enum ActivityToView
{
    APVIEW = 0,
    LFPVIEW = 1
};

/**

Helper class for viewing real-time activity across the probe.

*/

class ActivityView
{
public:
    ActivityView (int numChannels, int updateInterval_, std::vector<std::vector<int>> blocks_ = {})
        : updateInterval (updateInterval_)
    {
        if (blocks_.empty())
        {
            std::vector<int> block;
            for (int i = 0; i < numChannels; i++)
            {
                block.push_back (i);
            }
            blocks.push_back (block);
        }
        else
        {
            blocks = blocks_;
        }

        minChannelValues.resize (numChannels, std::numeric_limits<int>::max());
        maxChannelValues.resize (numChannels, std::numeric_limits<int>::min());
        peakToPeakValues.resize (numChannels, 0.0f);

        counters.resize (blocks.size(), 0);
    }

    const float* getPeakToPeakValues()
    {
        return peakToPeakValues.data();
    }

    void addSample (float sample, int channel, int block = 0)
    {
        int blockChannel = blocks[block][0];
        int& counter = counters[block];

        if (channel == blockChannel)
        {
            if (counter == updateInterval)
            {
                reset (block);
                counter = 0;
            }
            counter++;
        }

        if (counter % 10 == 0)
        {
            if (sample < minChannelValues[channel])
            {
                minChannelValues[channel] = sample;
                return;
            }

            if (sample > maxChannelValues[channel])
            {
                maxChannelValues[channel] = sample;
            }
        }
    }

    void reset (int blockIndex = 0)
    {
        for (auto ch : blocks[blockIndex])
        {
            peakToPeakValues[ch] = maxChannelValues[ch] - minChannelValues[ch];
            minChannelValues[ch] = std::numeric_limits<int>::max();
            maxChannelValues[ch] = std::numeric_limits<int>::min();
        }

        counters[blockIndex] = 0;
    }

private:
    std::vector<float> minChannelValues;
    std::vector<float> maxChannelValues;
    std::vector<float> peakToPeakValues;

    std::vector<std::vector<int>> blocks;
    std::vector<int> counters;
    int updateInterval;
};

#endif // __ACTIVITYVIEW_H__
