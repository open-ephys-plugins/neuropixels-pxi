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
    ActivityView (int numChannels, int updateInterval_, std::vector<std::vector<int>> blocks_ = {}, bool filterEnabled_ = false)
        : numChannels (numChannels),
          updateInterval (updateInterval_),
          filterEnabled (filterEnabled_),
          bufferIndex (0),
          needsUpdate (false)
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

        peakToPeakValues.resize (numChannels, 0.0f);

        // Initialize the sample buffers and FIFOs for each block
        sampleBuffers.resize (blocks.size());
        filteredBuffers.resize (blocks.size());
        abstractFifos.resize (blocks.size());

        for (int blockIndex = 0; blockIndex < blocks.size(); ++blockIndex)
        {
            int blockSize = blocks[blockIndex].size();
            sampleBuffers[blockIndex].setSize (blockSize, updateInterval);
            sampleBuffers[blockIndex].clear();

            filteredBuffers[blockIndex].setSize (blockSize, updateInterval);
            filteredBuffers[blockIndex].clear();

            abstractFifos[blockIndex] = std::make_unique<AbstractFifo> (updateInterval);
        }

        counters.resize (blocks.size(), 0);

        filters.clear();

        for (int n = 0; n < numChannels; ++n)
        {
            filters.add (new Dsp::SmoothedFilterDesign<Dsp::Butterworth::Design::BandPass // design type
                                                       <2>, // order
                                                       1, // number of channels (must be const)
                                                       Dsp::DirectFormII> (1)); // realization

            Dsp::Params params;
            params[0] = updateInterval * 10.0f; // sample rate
            params[1] = 2; // order
            params[2] = (highCut + lowCut) / 2; // center frequency
            params[3] = highCut - lowCut; // bandwidth

            filters.getLast()->setParams (params);
        }
    }

    const float* getPeakToPeakValues()
    {
        calculatePeakToPeakValues();
        return peakToPeakValues.data();
    }

    void setBandpassFilterEnabled (bool enabled)
    {
        const ScopedLock lock (bufferMutex);
        filterEnabled = enabled;
    }

    void addToBuffer (float* samples, int numSamples, int blockIndex = 0)
    {
        if (blockIndex >= abstractFifos.size())
            return;

        int startIndex1, blockSize1, startIndex2, blockSize2;

        abstractFifos[blockIndex]->prepareToWrite (numSamples, startIndex1, blockSize1, startIndex2, blockSize2);

        int bs[2] = { blockSize1, blockSize2 };
        int si[2] = { startIndex1, startIndex2 };
        int cSize = 0;
        int numWritten = 0;

        // for each of the dest blocks we can write to...
        for (int i = 0; i < 2; ++i)
        {
            cSize = bs[i];
            for (int chanIdx = 0; chanIdx < blocks[blockIndex].size(); ++chanIdx) // write that much, per channel in block
            {
                int globalChan = blocks[blockIndex][chanIdx];
                sampleBuffers[blockIndex].copyFrom (chanIdx, // (int destChannel - local block channel index)
                                                    si[i], // (int destStartSample)
                                                    samples + (globalChan * numSamples) + numWritten, // (const float* source)
                                                    cSize); // (int num samples)
            }

            numWritten += cSize;
        }

        // finish write
        abstractFifos[blockIndex]->finishedWrite (numWritten);
    }

    void reset (int blockIndex = 0)
    {
        const ScopedLock lock (bufferMutex);

        if (blockIndex < blocks.size())
        {
            for (auto ch : blocks[blockIndex])
            {
                peakToPeakValues[ch] = 0.0f;
            }

            counters[blockIndex] = 0;
            sampleBuffers[blockIndex].clear();
            filteredBuffers[blockIndex].clear();
            abstractFifos[blockIndex]->reset();
        }

        bufferIndex = 0;
        needsUpdate = false;
    }

private:
    void calculatePeakToPeakValues()
    {
        for (int blockIndex = 0; blockIndex < blocks.size(); ++blockIndex)
        {
            int numReady = abstractFifos[blockIndex]->getNumReady();
            int numItems = jmin (numReady, updateInterval);
            int startIndex1, blockSize1, startIndex2, blockSize2;
            abstractFifos[blockIndex]->prepareToRead (numItems, startIndex1, blockSize1, startIndex2, blockSize2);

            if (blockSize1 > 0)
            {
                for (int chanIdx = 0; chanIdx < blocks[blockIndex].size(); ++chanIdx)
                {
                    filteredBuffers[blockIndex].copyFrom (chanIdx, // destChan
                                                          0, // destStartSample
                                                          sampleBuffers[blockIndex], // source
                                                          chanIdx, // sourceChannel
                                                          startIndex1, // sourceStartSample
                                                          blockSize1); // numSamples
                }
            }

            if (blockSize2 > 0)
            {
                for (int chanIdx = 0; chanIdx < blocks[blockIndex].size(); ++chanIdx)
                {
                    filteredBuffers[blockIndex].copyFrom (chanIdx, // destChan
                                                          blockSize1, // destStartSample
                                                          sampleBuffers[blockIndex], // source
                                                          chanIdx, // sourceChannel
                                                          startIndex2, // sourceStartSample
                                                          blockSize2); // numSamples
                }
            }

            abstractFifos[blockIndex]->finishedRead (numItems);

            for (int chanIdx = 0; chanIdx < blocks[blockIndex].size(); ++chanIdx)
            {
                int globalChan = blocks[blockIndex][chanIdx];

                // Apply bandpass filter if enabled
                if (filterEnabled)
                {
                    float* channelData = filteredBuffers[blockIndex].getWritePointer (chanIdx);

                    filters[globalChan]->process (numItems, &channelData);
                }

                Range<float> minMax = filteredBuffers[blockIndex].findMinMax (chanIdx, 0, numItems);
                peakToPeakValues[globalChan] = minMax.getEnd() - minMax.getStart();
            }

            filteredBuffers[blockIndex].clear();
        }
    }

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

    // 300 Hz low-pass filter
    const float lowCut = 300.0f;
    // 6000 Hz high-pass filter
    const float highCut = 6000.0f;
};

#endif // __ACTIVITYVIEW_H__
