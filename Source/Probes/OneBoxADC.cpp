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

#include "OneBoxADC.h"
#include "OneBoxDAC.h"
#include "Geometry.h"

#include "../UI/OneBoxInterface.h"

OneBoxADC::OneBoxADC (Basestation* bs, OneBoxDAC* dac_) : DataSource (bs),
                                                          dac (dac_)
{
    ui = nullptr;

    channel_count = NUM_ADCS;
    sample_rate = 30300.5;

    sourceType = DataSourceType::ADC;
    status = SourceStatus::CONNECTED;

    LOGD ("Initializing OneBoxADC");

    errorCode = Neuropixels::ADC_enableProbe (basestation->slot, true);

    if (errorCode != Neuropixels::SUCCESS)
    {
        LOGD ("Error enabling ADCs: ", errorCode);
    }

    setAdcInputRange (AdcInputRange::PLUSMINUS5V);

    for (int i = 0; i < channel_count; i++)
    {
        outputChannel.add (-1);
        isOutput[i] = false;
        useAsDigitalInput[i] = false;
        waveplayerTrigger[i] = false;

        Neuropixels::DAC_enableOutput (basestation->slot, i, false);
        setAdcThresholdLevel (AdcThresholdLevel::ONE_VOLT, i);
        setAdcComparatorState (AdcComparatorState::COMPARATOR_OFF, i);
    }
}

void OneBoxADC::initialize (bool signalChainIsLoading)
{

}

void OneBoxADC::startAcquisition()
{
    sample_number = 0;
    apBuffer->clear();

    LOGD ("  Starting thread.");
    startThread();
}

void OneBoxADC::stopAcquisition()
{
    stopThread (1000);
}

void OneBoxADC::setAsOutput (int selectedOutput, int channel)
{
    if (channel < 0 || channel >= channel_count)
        return;

    if (outputChannel[channel] != -1)
    {
        isOutput[outputChannel[channel]] = false;
    }

    isOutput[selectedOutput] = true;
    outputChannel.set (channel, selectedOutput);

    // TODO: actually set the output
}

int OneBoxADC::getOutputChannel (int channel)
{
    if (channel < 0 || channel >= channel_count)
        return false;

    return outputChannel[channel];
}

Array<int> OneBoxADC::getAvailableChannels (int sourceChannel)
{
    Array<int> availableChannels;

    for (int i = 0; i < channel_count; i++)
    {
        if (! isOutput[i] && i != sourceChannel)
        {
            availableChannels.add (i);
        }
    }

    return availableChannels;
}

void OneBoxADC::setAdcInputRange (AdcInputRange range)
{
    switch (range)
    {
        case AdcInputRange::PLUSMINUS2PT5V:
            Neuropixels::ADC_setVoltageRange (basestation->slot, Neuropixels::ADC_RANGE_2_5V);
            bitVolts = 2.5f / float (pow (2, 15));
            inputRange = AdcInputRange::PLUSMINUS2PT5V;
            break;

        case AdcInputRange::PLUSMINUS5V:
            Neuropixels::ADC_setVoltageRange (basestation->slot, Neuropixels::ADC_RANGE_5V);
            bitVolts = 5.0f / float (pow (2, 15));
            inputRange = AdcInputRange::PLUSMINUS5V;
            break;

        case AdcInputRange::PLUSMINUS10V:
            Neuropixels::ADC_setVoltageRange (basestation->slot, Neuropixels::ADC_RANGE_10V);
            bitVolts = 10.0f / float (pow (2, 15));
            inputRange = AdcInputRange::PLUSMINUS10V;
            break;

        default:
            Neuropixels::ADC_setVoltageRange (basestation->slot, Neuropixels::ADC_RANGE_5V);
            bitVolts = 5.0f / float (pow (2, 15));
            inputRange = AdcInputRange::PLUSMINUS5V;
            break;
    }
}

AdcInputRange OneBoxADC::getAdcInputRange()
{
    return inputRange;
}

float OneBoxADC::getChannelGain (int channel)
{
    if (channel < 0 || channel >= channel_count)
        return -1;

    return bitVolts;
}

void OneBoxADC::setAdcThresholdLevel (AdcThresholdLevel level, int channel)
{
    if (channel < 0 || channel >= channel_count)
        return;

    switch (level)
    {
        case AdcThresholdLevel::ONE_VOLT:
            Neuropixels::ADC_setComparatorThreshold (basestation->slot,
                                                     channel,
                                                     0.5f,
                                                     1.0f);
            thresholdLevels[channel] = AdcThresholdLevel::ONE_VOLT;
            break;
        case AdcThresholdLevel::THREE_VOLTS:
            Neuropixels::ADC_setComparatorThreshold (basestation->slot,
                                                     channel,
                                                     1.5f,
                                                     3.0f);
            thresholdLevels[channel] = AdcThresholdLevel::THREE_VOLTS;
            break;
        default:
            Neuropixels::ADC_setComparatorThreshold (basestation->slot,
                                                     channel,
                                                     0.5f,
                                                     1.0f);
            thresholdLevels[channel] = AdcThresholdLevel::ONE_VOLT;
    }
}

AdcThresholdLevel OneBoxADC::getAdcThresholdLevel (int channel)
{
    if (channel < 0 || channel >= channel_count)
        return AdcThresholdLevel::ONE_VOLT;

    return thresholdLevels[channel];
}

void OneBoxADC::setAdcComparatorState (AdcComparatorState state, int channel)
{
    if (channel < 0 || channel >= channel_count)
        return;

    switch (state)
    {
        case AdcComparatorState::COMPARATOR_ON:
            useAsDigitalInput[channel] = true;
            break;
        case AdcComparatorState::COMPARATOR_OFF:
            useAsDigitalInput[channel] = false;
            break;
        default:
            useAsDigitalInput[channel] = false;
    }
}

AdcComparatorState OneBoxADC::getAdcComparatorState (int channel)
{
    if (channel < 0 || channel >= channel_count)
        return AdcComparatorState::COMPARATOR_OFF;

    return useAsDigitalInput[channel] ? AdcComparatorState::COMPARATOR_ON : AdcComparatorState::COMPARATOR_OFF;
}

void OneBoxADC::setTriggersWaveplayer (bool shouldTrigger, int channel)
{
    if (channel < 0 || channel >= channel_count)
        return;

    waveplayerTrigger[channel] = shouldTrigger;

    LOGD ("Setting channel ", channel, " to trigger waveplayer: ", shouldTrigger);

    // configure trigger
}

bool OneBoxADC::getTriggersWaveplayer (int channel)
{
    if (channel < 0 || channel >= channel_count)
        return false;

    return waveplayerTrigger[channel];
}

void OneBoxADC::run()
{
    const int NUM_ADCS_AND_COMPARATORS = NUM_ADCS * 2;

    int16_t data[MAXPACKETS * NUM_ADCS_AND_COMPARATORS];

    double ts_s;

    Neuropixels::PacketInfo packetInfo[MAXPACKETS];

    int packetsAvailable;
    int headroom;

    while (! threadShouldExit())
    {
        int count = MAXPACKETS;

        float adcSamples[NUM_ADCS_AND_COMPARATORS * MAXPACKETS];
        int64 sample_numbers[MAXPACKETS];
        double timestamps[MAXPACKETS];
        uint64 event_codes[MAXPACKETS];

        errorCode = Neuropixels::ADC_readPackets (basestation->slot,
                                                  &packetInfo[0],
                                                  &data[0],
                                                  NUM_ADCS_AND_COMPARATORS,
                                                  count,
                                                  &count);

        if (errorCode == Neuropixels::SUCCESS && count > 0)
        {
            for (int packetNum = 0; packetNum < count; packetNum++)
            {
                uint64 eventCode = packetInfo[packetNum].Status >> 6;

                uint32_t npx_timestamp = packetInfo[packetNum].Timestamp;

                for (int j = 0; j < NUM_ADCS; j++)
                {
                    adcSamples[j + packetNum * NUM_ADCS] = float (data[packetNum * NUM_ADCS_AND_COMPARATORS + j]) * bitVolts; // convert to volts
                }

                sample_numbers[packetNum] = sample_number++;

                for (int j = 0; j < NUM_ADCS; j++)
                {
                    if (useAsDigitalInput[j])
                        eventCode |= (data[packetNum * NUM_ADCS_AND_COMPARATORS + NUM_ADCS + j]) << j; // extract comparator states
                }

                event_codes[packetNum] = eventCode;
            }

            apBuffer->addToBuffer (adcSamples,
                                   sample_numbers,
                                   timestamps,
                                   event_codes,
                                   count);
        }
        else if (errorCode != Neuropixels::SUCCESS)
        {
            LOGD ("readPackets error code: ", errorCode, " for ADCs");
        }

        Neuropixels::ADC_getPacketFifoStatus (basestation->slot, &packetsAvailable, &headroom);

        if (packetsAvailable < MAXPACKETS)
        {
            int uSecToWait = (MAXPACKETS - packetsAvailable) * 30;

            std::this_thread::sleep_for (std::chrono::microseconds (uSecToWait));
        }
    }
}
