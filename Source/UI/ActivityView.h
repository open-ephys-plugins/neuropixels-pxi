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
    ActivityView (int numChannels, int updateInterval_, Array<Array<int>> blocks_ = Array<Array<int>>())
    {

        if (blocks_.size() == 0)
        {
            Array<int> block;

            for (int i = 0; i < numChannels; i++)
            {
				block.add (i);
			}
            blocks.add (block);
        }
        else
        {
        	blocks = blocks_;
        }
        
        updateInterval = updateInterval_;

        for (int i = 0; i < numChannels; i++)
        {
            minChannelValues.add (999999.9f);
			maxChannelValues.add (-999999.9f);
            peakToPeakValues.add (0);
        }

        for (int i = 0; i < blocks.size(); i++)
        {
            counters.add (0);
		}
    }

    const float* getPeakToPeakValues()
    {
        return peakToPeakValues.getRawDataPointer();
    }

    void addSample (float sample, int channel, int block=0)
    {
        if (channel == blocks[block][0])
        {
            if (counters[block] == updateInterval)
                reset(block);

            counters.set (block, counters[block] + 1);
        }

        if (counters[block] % 10 == 0)
        {
            if (sample < minChannelValues[channel])
            {
                minChannelValues.set (channel, sample);
                return;
            }

            if (sample > maxChannelValues[channel])
            {
                maxChannelValues.set (channel, sample);
            }
        }
    }

    void reset(int blockIndex = 0)
    {

        for (auto ch : blocks[blockIndex])
        {
            peakToPeakValues.set (ch, maxChannelValues[ch] - minChannelValues[ch]);
            minChannelValues.set (ch, 999999.9f);
            maxChannelValues.set (ch, -999999.9f);
        }

        counters.set (blockIndex, 0);

        
    }

private:
    Array<float, CriticalSection> minChannelValues;
    Array<float, CriticalSection> maxChannelValues;
    Array<float, CriticalSection> peakToPeakValues;

    Array<Array<int>> blocks;
    Array<int> counters;
    int updateInterval;
};

#endif // __ACTIVITYVIEW_H__
