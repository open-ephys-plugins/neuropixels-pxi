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

#include <DspLib.h>
#include <VisualizerEditorHeaders.h>
#include <limits>
#include <memory>
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
    ActivityView (int numChannels, int updateInterval, std::vector<std::vector<int>> blocks = {}, int numAdcs = 0);
    ~ActivityView() {};

    const float* getPeakToPeakValues();

    void setBandpassFilterEnabled (bool enabled);

    void setCommonAverageReferencingEnabled (bool enabled);

    void addToBuffer (float* samples, int numSamples, int blockIndex = 0);

    void reset (int blockIndex = 0);

private:
    void calculatePeakToPeakValues();

    void applyCommonAverageReferencing (AudioBuffer<float>& buffer, int blockIndex, int numSamples);

    // Thread synchronization
    CriticalSection bufferMutex;

    int numChannels;
    std::vector<float> peakToPeakValues;

    // JUCE AudioBuffers for sample storage - one per block
    std::vector<AudioBuffer<float>> sampleBuffers;
    std::vector<AudioBuffer<float>> filteredBuffers;

    // AbstractFifo - one per block
    std::vector<std::unique_ptr<AbstractFifo>> abstractFifos;

    int bufferIndex;
    bool needsUpdate;

    std::vector<std::vector<int>> blocks;
    std::vector<int> counters;
    int updateInterval;

    // Bandpass filter state
    bool filterEnabled;
    Array<Dsp::Filter*> filters;

    // Common average referencing state
    bool carEnabled;
    int numAdcs;
    std::vector<std::vector<int>> adcGroups; // Maps global channel index to ADC group index
    std::vector<AudioBuffer<float>> adcBuffers; // One AudioBuffer per block, channels = ADC groups

    // 300 Hz low-pass filter
    const float lowCut = 300.0f;
    // 6000 Hz high-pass filter
    const float highCut = 6000.0f;
};

#endif // __ACTIVITYVIEW_H__
