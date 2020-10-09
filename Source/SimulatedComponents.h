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

#ifndef __SIMULATEDCOMPONENTS_H_2C4C2D67__
#define __SIMULATEDCOMPONENTS_H_2C4C2D67__

#include <stdio.h>
#include <string.h>

#include "NeuropixComponents.h"

class SimulatedBasestationConnectBoard;
class SimulatedFlex;
class SimulatedHeadstage;
class SimulatedProbe;

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
};

class SimulatedBasestationConnectBoard : public BasestationConnectBoard
{
public:
	SimulatedBasestationConnectBoard(Basestation* bs): BasestationConnectBoard(bs) {}
	void getInfo() override;
};


class SimulatedProbe : public Probe
{
public:
	SimulatedProbe(Basestation* bs, signed char port) : Probe(bs, port) { }

	void init() override;

	void setChannels(Array<int> channelStatus) override;

	void setApFilterState(bool) override;
	void setReferences(np::channelreference_t refId, unsigned char refElectrodeBank) override;
	void setGains(unsigned char apGain, unsigned char lfpGain) override;

	void calibrate() override;

	void getInfo() override;

	void run() override;

};

class SimulatedHeadstage : public Headstage
{
public:
	SimulatedHeadstage(Probe* probe) : Headstage(probe) {}
	void getInfo() override;
};

class SimulatedFlex : public Flex
{
public:
	SimulatedFlex(Probe* probe) : Flex(probe) {}
	void getInfo() override;
};


#endif  // __SIMULATEDCOMPONENTS_H_2C4C2D67__