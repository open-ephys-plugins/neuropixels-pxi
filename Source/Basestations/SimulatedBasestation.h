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

#ifndef __SIMULATEDBASESTATION_H_2C4C2D67__
#define __SIMULATEDBASESTATION_H_2C4C2D67__

#include "../NeuropixComponents.h"

class SimulatedBasestation : public Basestation
{
public:
	SimulatedBasestation(int slot) : Basestation(slot) { }
	~SimulatedBasestation() { }

	void open() override;
	void close() override;

	void init() override;

	int getProbeCount() override;

	void initializeProbes() override;

	void setChannels(unsigned char slot, signed char port, Array<int> channelStatus) override;
	void setReferences(unsigned char slot, signed char port, np::channelreference_t refId, unsigned char electrodeBank) override;
	void setGains(unsigned char slot, signed char port, unsigned char apGain, unsigned char lfpGain) override;
	void setApFilterState(unsigned char slot, signed char port, bool filterState) override;

	void getInfo() override;

	void setSyncAsInput() override;
	void setSyncAsOutput(int freqIndex) override;

	void startAcquisition() override;
	void stopAcquisition() override;

	Array<int> getSyncFrequencies() override;

	float getFillPercentage() override;

	bool runBist(signed char port, int bistIndex) override
	{
		return true;
	}
};

class SimulatedBasestationConnectBoard : public BasestationConnectBoard
{
public:
	SimulatedBasestationConnectBoard(Basestation* bs): BasestationConnectBoard(bs) {}
	void getInfo() override;
};




#endif  // __SIMULATEDBASESTATION_H_2C4C2D67__