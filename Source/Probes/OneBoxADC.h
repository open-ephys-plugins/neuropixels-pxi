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

#ifndef __ONEBOX_ADC_H_2C4C2D67__
#define __ONEBOX_ADC_H_2C4C2D67__

#include "../NeuropixComponents.h"

#define MAXPACKETS 64
#define NUM_ADCS 12

class OneBoxInterface;
class OneBoxDAC;

/** 

	Data source for OneBox ADC channels

	By default, 12 are available, but these can be repurposed
	as DAC channels if desired.

*/
class OneBoxADC : public DataSource
{
public:
    /** Constructor */
    OneBoxADC (Basestation*, OneBoxDAC*);

    /** Returns channel name */
    String getName() { return "ADC"; }

    /** Return info about part numbers, etc. -- not used*/
    void getInfo() override {}

    /** Open connection to the ADCs -- not used */
    bool open() override { return true; }

    /** Close connection to the ADCs -- not used */
    bool close() override { return true; }

    /** Initialize all channels as ADCs */
    void initialize (bool signalChainIsLoading) override;

    /** Start data acquisition thread */
    void startAcquisition() override;

    /** Stop data acquisition thread */
    void stopAcquisition() override;

    /** Read packets and add to buffer */
    void run() override; // acquire data

    /** Map an ADC to a DAC, or turn it back into an ADC */
    void setAsOutput (int selected, int channel);

    /** Returns the connected DAC channel, or -1 if channel is an ADC */
    int getOutputChannel (int channel);

    /** Gets names of available channels */
    Array<int> getAvailableChannels (int sourceChannel);

    /** Sets input range for ADC channels */
    void setAdcInputRange (AdcInputRange range);

    /** Returns input range of all channels */
    AdcInputRange getAdcInputRange();

    /** Returns gain of a particular channel (depends on ADC input range) */
    float getChannelGain (int channel);

    /** Sets threshold for ADC channel */
    void setAdcThresholdLevel (AdcThresholdLevel level, int channel);

    /** Returns threshold level of a particular channel */
    AdcThresholdLevel getAdcThresholdLevel (int channel);

    /** Sets ADC comparator state */
    void setAdcComparatorState (AdcComparatorState state, int channel);

    /** Returns ADC comparator state for a particular channel */
    AdcComparatorState getAdcComparatorState (int channel);

    /** Sets waveplayer trigger state for a particular channel */
    void setTriggersWaveplayer (bool shouldTrigger, int channel);

    /** Returns waveplayer trigger state of a particular channel */
    bool getTriggersWaveplayer (int channel);

    /** Pointer to OneBox UI*/
    OneBoxInterface* ui;

    /** Pointer to DAC */
    OneBoxDAC* dac;

private:
    /** Sample number for acquisition */
    int64 sample_number;

    /** Holds incoming samples*/
    DataBuffer* sampleBuffer;

    /** Stores output mapping */
    Array<int> outputChannel;

    /** Stores channel gains */
    float bitVolts;

    /** Stores channel input range */
    AdcInputRange inputRange;

    /** Stores whether channel is ADC or DAC */
    bool isOutput[NUM_ADCS];

    /** Stores channel trigger thresholds */
    AdcThresholdLevel thresholdLevels[NUM_ADCS];

    /** Indicates whether to convert analog signal to a digital input */
    bool useAsDigitalInput[NUM_ADCS];

    /** Stores WaveplayerTrigger state */
    bool waveplayerTrigger[NUM_ADCS];

};

#endif // __ONEBOX_ADC_H_2C4C2D67__