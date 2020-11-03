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

#ifndef __NEUROPIXBASESTATIONV3_H_2C4C2D67__
#define __NEUROPIXBASESTATIONV3_H_2C4C2D67__

#include "../API/v3/NeuropixAPI.h"
#include "../NeuropixComponents.h"

# define SAMPLECOUNT 64

class Basestation_v3 : public Basestation

{
public:
	Basestation_v3(int slot);
	~Basestation_v3();

	int slot;

	bool open() override;
	void close() override;
	void initialize() override;

	int getProbeCount() override;

	//float getTemperature() override;

	void getInfo() override;

	void setSyncAsInput() override;
	void setSyncAsOutput(int freqIndex) override;

	Array<int> getSyncFrequencies() override;

	void startAcquisition() override;
	void stopAcquisition() override;

	float getFillPercentage() override;

	bool runBist(signed char port, BIST bistType) override;

	void updateBsFirmware(File file) override;
	void updateBscFirmware(File file) override;

	void run() override;

	//Neuropixels::bistElectrodeStats stats[960];

	Neuropixels::NP_ErrorCode errorCode;

};

class BasestationConnectBoard_v3 : public BasestationConnectBoard
{
public:
	BasestationConnectBoard_v3(Basestation*);

	void getInfo() override;

	Neuropixels::NP_ErrorCode errorCode;
};


#endif  // __NEUROPIXBASESTATIONV3_H_2C4C2D67__