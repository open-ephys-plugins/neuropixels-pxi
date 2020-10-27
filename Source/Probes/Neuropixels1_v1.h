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

#ifndef __NEUROPIX1V1_H_2C4C2D67__
#define __NEUROPIX1V1_H_2C4C2D67__

#include "../NeuropixComponents.h"

#include "../API/v1/NeuropixAPI.h"

# define SAMPLECOUNT 64

class Neuropixels1_v1 : public Probe
{
public:
	Neuropixels1_v1(Basestation* bs, signed char port);

	void init() override;

	void setChannels(Array<int> channelStatus) override;

	void setApFilterState(bool) override;
	void setReferences(np::channelreference_t refId, unsigned char refElectrodeBank) override;
	void setGains(unsigned char apGain, unsigned char lfpGain) override;

	void calibrate() override;

	void getInfo() override;

	void run() override;

	np::electrodePacket packet[SAMPLECOUNT];

	np::NP_ErrorCode errorCode;

};


#endif  // _NEUROPIX1V1_H_2C4C2D67__