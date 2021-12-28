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

#ifndef __NEUROPIX1V3_H_2C4C2D67__
#define __NEUROPIX1V3_H_2C4C2D67__

#include "../NeuropixComponents.h"

#include "../API/v3/NeuropixAPI.h"

# define MAXPACKETS 64

class Neuropixels1_v3 : public Probe
{
public:
	Neuropixels1_v3(Basestation* bs, Headstage* hs, Flex* fl);

	void getInfo() override;

	bool open() override;
	bool close() override;

	void initialize(bool signalChainIsLoading) override;

	void selectElectrodes() override;
	void setAllReferences() override;
	void setAllGains() override;
	void setApFilterState() override;
	
	void writeConfiguration() override;

	void startAcquisition() override;
	void stopAcquisition() override;

	bool runBist(BIST bistType) override;

	void calibrate() override;

	void run() override; // acquire data

	bool generatesLfpData() { return true; }
	bool hasApFilterSwitch() { return true; }

	void getGain();

	Neuropixels::electrodePacket packet[SAMPLECOUNT];
	Neuropixels::NP_ErrorCode errorCode;

	float apSamples[385 * 12 * MAXPACKETS];
	float lfpSamples[385 * MAXPACKETS];
	int64 ap_timestamps[12 * MAXPACKETS];
	uint64 event_codes[12 * MAXPACKETS];
	int64 lfp_timestamps[MAXPACKETS];
	uint64 lfp_event_codes[MAXPACKETS];

};


#endif  // _NEUROPIX1V3_H_2C4C2D67__