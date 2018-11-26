/*
------------------------------------------------------------------

This file is part of the Open Ephys GUI
Copyright (C) 2018 Allen Institute for Brain Science and Open Ephys

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

#ifndef __NEUROPIXCOMPONENTS_H_2C4C2D67__
#define __NEUROPIXCOMPONENTS_H_2C4C2D67__

#include <DataThreadHeaders.h>
#include <stdio.h>
#include <string.h>

#include "neuropix-api/NeuropixAPI.h"

class BasestationConnectBoard;
class Flex;
class Headstage;
class Probe;

class NeuropixComponent
{
public:
	NeuropixComponent();

	String version;
	uint64_t serial_number;
	String part_number;

	virtual void getInfo() = 0;
};

class NeuropixAPI : public NeuropixComponent
{
public:
	void getInfo();
};

class Basestation : public NeuropixComponent
{
public:
	Basestation(int slot);
	~Basestation();

	unsigned char slot;

	int getProbeCount();

	String boot_version;

	ScopedPointer<BasestationConnectBoard> basestationConnectBoard;

	OwnedArray<Probe> probes;

	void initializeProbes();

	float getTemperature();

	void setReferences(channelreference_t refId, unsigned char electrodeBank);
	void setGains(unsigned char apGain, unsigned char lfpGain);
	void setApFilterState(bool filterState);

	void getInfo();
	void makeSyncMaster();

	void startAcquisition();
	void stopAcquisition();

	void setSavingDirectory(File);
	File getSavingDirectory();

	float getFillPercentage();

private:
	bool probesInitialized;

	File savingDirectory;
};

class BasestationConnectBoard : public NeuropixComponent
{
public:
	BasestationConnectBoard(Basestation*);
	String boot_version;

	Basestation* basestation;

	void getInfo();
};

class Probe : public NeuropixComponent
{
public:
	Probe(Basestation* bs, signed char port);

	Basestation* basestation;
	signed char port;

	ScopedPointer<Headstage> headstage;
	ScopedPointer<Flex> flex;

	int reference;
	int ap_gain;
	int lfp_gain;
	bool highpass_on;

	Array<bool> selectedElectrodes;

	void calibrate();

	void getInfo();

	int channel_count;

	float fifoFillPercentage;

	String name;

};

class Headstage : public NeuropixComponent
{
public:
	Headstage::Headstage(Probe*);
	Probe* probe;
	void getInfo();
};

class Flex : public NeuropixComponent
{
public:
	Flex::Flex(Probe*);
	Probe* probe;
	void getInfo();
};

#endif  // __NEUROPIXCOMPONENTS_H_2C4C2D67__