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

#include "ActivityView.h"

#include <algorithm>

/**

Helper class for viewing real-time activity across the probe.

*/

ActivityView::ActivityView (int numChannels_,
                            int updateInterval_,
                            std::vector<std::vector<int>> blocks_,
                            int numAdcs_,
                            int totalElectrodes_)
    : numChannels (numChannels_),
      totalElectrodes (totalElectrodes_ > 0 ? totalElectrodes_ : numChannels_),
      updateInterval (updateInterval_),
      filterEnabled (true),
      carEnabled (true),
      bufferIndex (0),
      needsUpdate (false),
      numAdcs (numAdcs_),
      surveyMode (false)
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

    peakToPeakValues.assign (totalElectrodes, -1.0f);
    channelToElectrode.resize (numChannels, -1);
    for (int i = 0; i < numChannels; ++i)
    {
        if (i < totalElectrodes)
            channelToElectrode[i] = i;
    }

    surveyAccumulation.assign (totalElectrodes, 0.0);
    surveySampleCount.assign (totalElectrodes, 0);

    // Initialize the sample buffers and FIFOs for each block
    sampleBuffers.resize (blocks.size());
    filteredBuffers.resize (blocks.size());
    abstractFifos.resize (blocks.size());

    int bufferSize = updateInterval * 2; // Buffer size to hold 2x the update interval

    for (int blockIndex = 0; blockIndex < blocks.size(); ++blockIndex)
    {
        int blockSize = blocks[blockIndex].size();
        sampleBuffers[blockIndex].setSize (blockSize, bufferSize);
        sampleBuffers[blockIndex].clear();

        filteredBuffers[blockIndex].setSize (blockSize, bufferSize);
        filteredBuffers[blockIndex].clear();

        abstractFifos[blockIndex] = std::make_unique<AbstractFifo> (bufferSize);
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

    // Initialize ADC grouping for CAR
    if (numAdcs == 32) // Neuropixels 1.0
    {
        // Create ADC buffers for each block (12 ADC groups each)
        adcGroups.resize (blocks.size());
        adcBuffers.resize (blocks.size());

        for (int blockIndex = 0; blockIndex < blocks.size(); ++blockIndex)
        {
            for (int i = 0; i < blocks[blockIndex].size(); i++)
            {
                adcGroups[blockIndex].push_back ((i / 2) % 12);
            }
            adcBuffers[blockIndex].setSize (12, updateInterval * 2);
            adcBuffers[blockIndex].clear();
        }
    }
    else if (numAdcs == 24) // Neuropixels 2.0
    {
        // Create ADC buffers for each block (16 ADC groups each)
        adcGroups.resize (blocks.size());
        adcBuffers.resize (blocks.size());

        for (int blockIndex = 0; blockIndex < blocks.size(); ++blockIndex)
        {
            for (int i = 0; i < blocks[blockIndex].size(); i++)
            {
                adcGroups[blockIndex].push_back ((i / 2) % 16);
            }
            adcBuffers[blockIndex].setSize (16, updateInterval * 2);
            adcBuffers[blockIndex].clear();
        }
    }
    else
    {
        // Default to simple CAR for unknown probe types (numAdcs = 0));

        // Create single ADC group per block
        adcGroups.resize (blocks.size());
        adcBuffers.resize (blocks.size());

        for (int blockIndex = 0; blockIndex < blocks.size(); ++blockIndex)
        {
            adcGroups[blockIndex].resize (blocks[blockIndex].size(), 0);
            adcBuffers[blockIndex].setSize (1, updateInterval * 2);
            adcBuffers[blockIndex].clear();
        }
    }
}

const float* ActivityView::getPeakToPeakValues()
{
    calculatePeakToPeakValues();
    return peakToPeakValues.data();
}

void ActivityView::setBandpassFilterEnabled (bool enabled)
{
    const ScopedLock lock (bufferMutex);
    filterEnabled = enabled;
}

void ActivityView::setCommonAverageReferencingEnabled (bool enabled)
{
    const ScopedLock lock (bufferMutex);
    carEnabled = enabled;
}

void ActivityView::addToBuffer (float* samples, int numSamples, int blockIndex)
{
    if (blockIndex >= abstractFifos.size())
        return;

    int startIndex1, blockSize1, startIndex2, blockSize2;

    abstractFifos[blockIndex]->prepareToWrite (numSamples, startIndex1, blockSize1, startIndex2, blockSize2);

    int bs[2] = { blockSize1, blockSize2 };
    int si[2] = { startIndex1, startIndex2 };
    int numWritten = 0;

    // for each of the dest blocks we can write to...
    for (int i = 0; i < 2; ++i)
    {
        for (int chanIdx = 0; chanIdx < blocks[blockIndex].size(); ++chanIdx) // write that much, per channel in block
        {
            sampleBuffers[blockIndex].copyFrom (chanIdx, // (int destChannel - local block channel index)
                                                si[i], // (int destStartSample)
                                                samples + (chanIdx * numSamples) + numWritten, // (const float* source)
                                                bs[i]); // (int num samples)
        }

        numWritten += bs[i];
    }

    // finish write
    abstractFifos[blockIndex]->finishedWrite (numWritten);
}

void ActivityView::reset (int blockIndex)
{
    const ScopedLock lock (bufferMutex);

    if (blockIndex < blocks.size())
    {
        for (auto channelId : blocks[blockIndex])
        {
            if (! isPositiveAndBelow (channelId, (int) channelToElectrode.size()))
                continue;

            const int electrodeIdx = channelToElectrode[(size_t) channelId];

            if (! isPositiveAndBelow (electrodeIdx, (int) peakToPeakValues.size()))
                continue;

            peakToPeakValues[(size_t) electrodeIdx] = -1.0f;
        }

        counters[blockIndex] = 0;
        sampleBuffers[blockIndex].clear();
        filteredBuffers[blockIndex].clear();
        abstractFifos[blockIndex]->reset();
    }

    bufferIndex = 0;
    needsUpdate = false;
}

void ActivityView::setChannelToElectrodeMapping (const std::vector<int>& mapping)
{
    const ScopedLock lock (bufferMutex);

    if ((int) mapping.size() != numChannels)
        return;

    channelToElectrode = mapping;
}

void ActivityView::setSurveyMode (bool enabled, bool reset)
{
    const ScopedLock lock (bufferMutex);
    surveyMode = enabled;

    if (reset)
        resetSurveyData();
}

void ActivityView::resetSurveyData()
{
    std::fill (surveyAccumulation.begin(), surveyAccumulation.end(), 0.0);
    std::fill (surveySampleCount.begin(), surveySampleCount.end(), 0);
}

ActivityView::SurveyStatistics ActivityView::getSurveyStatistics()
{
    const ScopedLock lock (bufferMutex);

    SurveyStatistics stats;
    stats.totals = surveyAccumulation;
    stats.sampleCounts = surveySampleCount;
    stats.averages.resize (stats.totals.size(), 0.0f);

    for (size_t i = 0; i < stats.totals.size(); ++i)
    {
        const auto count = i < stats.sampleCounts.size() ? stats.sampleCounts[i] : 0;
        stats.averages[i] = count > 0 ? static_cast<float> (stats.totals[i] / static_cast<double> (count)) : 0.0f;
    }

    return stats;
}

void ActivityView::calculatePeakToPeakValues()
{
    const ScopedLock lock (bufferMutex);

    for (int blockIndex = 0; blockIndex < blocks.size(); ++blockIndex)
    {
        int numReady = abstractFifos[blockIndex]->getNumReady();
        int numItems = jmin (numReady, updateInterval);
        int startIndex1, blockSize1, startIndex2, blockSize2;
        abstractFifos[blockIndex]->prepareToRead (numItems, startIndex1, blockSize1, startIndex2, blockSize2);

        if (numItems <= 0)
        {
            abstractFifos[blockIndex]->finishedRead (numItems);
            continue;
        }

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

        // Apply common average referencing if enabled (before filtering and analysis)
        if (carEnabled)
        {
            applyCommonAverageReferencing (filteredBuffers[blockIndex], blockIndex, numItems);
        }

        for (int chanIdx = 0; chanIdx < blocks[blockIndex].size(); ++chanIdx)
        {
            int globalChan = blocks[blockIndex][chanIdx];

            if (! isPositiveAndBelow (globalChan, numChannels))
                continue;

            // Apply bandpass filter if enabled
            if (filterEnabled && isPositiveAndBelow (globalChan, filters.size()))
            {
                float* channelData = filteredBuffers[blockIndex].getWritePointer (chanIdx);

                filters[globalChan]->process (numItems, &channelData);
            }

            Range<float> minMax = filteredBuffers[blockIndex].findMinMax (chanIdx, 0, numItems);
            const float amplitude = minMax.getEnd() - minMax.getStart();

            int electrodeIdx = -1;
            if (isPositiveAndBelow (globalChan, (int) channelToElectrode.size()))
                electrodeIdx = channelToElectrode[(size_t) globalChan];

            if (! isPositiveAndBelow (electrodeIdx, (int) peakToPeakValues.size()))
                continue;

            if (surveyMode && isPositiveAndBelow (electrodeIdx, (int) surveyAccumulation.size()))
            {
                surveyAccumulation[(size_t) electrodeIdx] += amplitude;
                surveySampleCount[(size_t) electrodeIdx] += 1;

                const double count = static_cast<double> (surveySampleCount[(size_t) electrodeIdx]);
                peakToPeakValues[(size_t) electrodeIdx] = count > 0.0 ? (float) (surveyAccumulation[(size_t) electrodeIdx] / count) : amplitude;
            }
            else
            {
                peakToPeakValues[(size_t) electrodeIdx] = amplitude;
            }
        }
    }
}

void ActivityView::applyCommonAverageReferencing (AudioBuffer<float>& buffer, int blockIndex, int numSamples)
{
    if (numAdcs == 0 || adcGroups.empty() || adcBuffers.empty() || blockIndex >= adcBuffers.size())
    {
        // Fallback to simple CAR if ADC information is not available
        int numChannels = blocks[blockIndex].size();

        for (int sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
        {
            float commonAverage = 0.0f;

            for (int chanIdx = 0; chanIdx < numChannels; ++chanIdx)
            {
                commonAverage += buffer.getSample (chanIdx, sampleIndex);
            }

            commonAverage /= numChannels;

            for (int chanIdx = 0; chanIdx < numChannels; ++chanIdx)
            {
                float currentSample = buffer.getSample (chanIdx, sampleIndex);
                buffer.setSample (chanIdx, sampleIndex, currentSample - commonAverage);
            }
        }
        return;
    }

    // ADC-aware CAR
    int numChannelsInBlock = blocks[blockIndex].size();
    int numAdcGroups = adcBuffers[blockIndex].getNumChannels();

    // Clear ADC buffers and reset counts for this block
    adcBuffers[blockIndex].clear (0, numSamples);

    // Sum samples by ADC group
    for (int chanIdx = 0; chanIdx < numChannelsInBlock; ++chanIdx)
    {
        int adcGroup = adcGroups[blockIndex][chanIdx];
        if (adcGroup < numAdcGroups)
        {
            adcBuffers[blockIndex].addFrom (adcGroup, // destChan
                                            0, // destStartSample
                                            buffer, // source
                                            chanIdx, // sourceChan
                                            0, // sourceStartSample
                                            numSamples); // numSamples
        }
    }

    float count = static_cast<float> (numAdcs);

    // Calculate averages for each ADC group
    for (int g = 0; g < numAdcGroups; ++g)
    {
        adcBuffers[blockIndex].applyGain (g, // channel
                                          0, // startSample
                                          numSamples, // numSamples
                                          1.0f / count); // gain
    }

    // Subtract ADC group averages from channels
    for (int chanIdx = 0; chanIdx < numChannelsInBlock; ++chanIdx)
    {
        int adcGroup = adcGroups[blockIndex][chanIdx];
        if (adcGroup < numAdcGroups)
        {
            buffer.addFrom (chanIdx, // destChan
                            0, // destStartSample
                            adcBuffers[blockIndex], // source
                            adcGroup, // sourceChan
                            0, // sourceStartSample
                            numSamples, // numSamples
                            -1.0f); // gain
        }
    }
}
