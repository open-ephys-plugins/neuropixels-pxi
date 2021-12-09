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

#ifndef __NEUROPIXBASESTATIONV1_H_2C4C2D67__
#define __NEUROPIXBASESTATIONV1_H_2C4C2D67__

#include "../API/v1/NeuropixAPI.h"
#include "../NeuropixComponents.h"

# define SAMPLECOUNT 64


class Basestation_v1 : public Basestation
{
public:
	Basestation_v1(int slot);
	~Basestation_v1();

	int slot;

	bool open() override;
	void close() override;
	void initialize(bool signalChainIsLoading) override;

	int getProbeCount() override;

	//float getTemperature() override;

	void getInfo() override;

	void setSyncAsInput() override;
	void setSyncAsOutput(int freqIndex) override;

	Array<int> getSyncFrequencies() override;

	void startAcquisition() override;
	void stopAcquisition() override;

	float getFillPercentage() override;

	void updateBsFirmware(File file) override;
	void updateBscFirmware(File file) override;

	void run() override;

	np::bistElectrodeStats stats[960];

	np::NP_ErrorCode errorCode;

};

class BasestationConnectBoard_v1 : public BasestationConnectBoard
{
public:
	BasestationConnectBoard_v1(Basestation*);

	void getInfo() override;

	np::NP_ErrorCode errorCode;
};




#endif  // __NEUROPIXBASESTATIONV1_H_2C4C2D67__