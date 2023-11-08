/*
------------------------------------------------------------------

This file is part of the Open Ephys GUI
Copyright (C) 2020 Allen Institute for Brain Science and Open Ephys

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

#include "../API/v3/NeuropixAPI.h"

# define SAMPLECOUNT 64
# define NUM_ADCS 12

class OneBoxDAC;
class OneBoxInterface;

/** 

	Data source for OneBox ADC channels

	By default, 12 are available, but these can be repurposed
	as DAC channels if desired.

*/
class OneBoxADC : public DataSource
{
public:

	/** Constructor */
	OneBoxADC(Basestation*);

	/** Return info about part numbers, etc.; not currently used*/
	void getInfo() override { }

	/** Open connection to the ADCs */
	bool open() override { return true; }

	/** Close connection to the ADCs */
	bool close() override { return true; }

	/** Initialize all channels as ADCs */
	void initialize(bool signalChainIsLoading) override;

	/** Sets range for all ADC channels */
	void setAdcInputRange(AdcInputRange range);

	/** Start data acquisition thread */
	void startAcquisition() override;

	/** Stop data acquisition thread */
	void stopAcquisition() override;

	/** Read packets and add to buffer */
	void run() override; // acquire data

	/** Set channel type to ADC or DAC */
	void setChannelType(int chan, DataSourceType type);

	/** Gets channel type array */
	Array<DataSourceType> getChannelTypes() {return channelTypes;}

	/** Returns gain of each channel */
	float getChannelGain(int chan);

	/** Returns channel name */
	String getName() { return "ADC"; }

	/** Channel scaling */
	float bitVolts;

	/** Sample number */
	int64 timestamp;

	/** Holds incoming samples*/
	DataBuffer* sampleBuffer;

	/** Pointer to DAC control */
	OneBoxDAC* dac;

	/** Pointer to OneBox UI*/
	OneBoxInterface* ui;

private:

	/** Stores channel types (ADC or DAC) */
	Array<DataSourceType> channelTypes;

	/** Neuropixels API error code*/
	Neuropixels::NP_ErrorCode errorCode;

};


#endif  // __ONEBOX_ADC_H_2C4C2D67__