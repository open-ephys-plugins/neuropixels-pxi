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

#ifndef __NEUROPIXCOMPONENTS_H_2C4C2D67__
#define __NEUROPIXCOMPONENTS_H_2C4C2D67__

#include <DataThreadHeaders.h>
#include <stdio.h>
#include <string.h>

#include "API/v1/NeuropixAPI.h"


# define SAMPLECOUNT 64

class BasestationConnectBoard;
class Flex;
class Headstage;
class Probe;

struct ComponentInfo
{
	String version = "";
	uint64_t serial_number = 0;
	String part_number = "";
	String boot_version = "";
};

class NeuropixComponent
{
public:
	NeuropixComponent() {
		getInfo();
	}

	ComponentInfo info;

	virtual void getInfo() { }
};

class NeuropixAPIv1 : public NeuropixComponent
{
public:
	void getInfo() {

		unsigned char version_major;
		unsigned char version_minor;
		np::getAPIVersion(&version_major, &version_minor);

		info.version = String(version_major) + "." + String(version_minor);
	}
};

class Basestation : public NeuropixComponent
{
public:
	Basestation(int slot_) : NeuropixComponent() {
		probesInitialized = false;
		slot = slot_;
	}

	virtual void open() = 0;
	virtual void close() = 0;
	virtual void init() = 0;

	virtual int getProbeCount() = 0;

	virtual void initializeProbes() = 0;

//	virtual float getTemperature() = 0;

	virtual bool runBist(signed char port, int bistIndex) = 0;

	virtual void setChannels(unsigned char slot, 
							 signed char port, 
							 Array<int> channelStatus) = 0;

	virtual void setReferences(unsigned char slot, 
							   signed char port, 
							   np::channelreference_t refId, 
						       unsigned char electrodeBank) = 0;

	virtual void setGains(unsigned char slot, 
						  signed char port, 
						  unsigned char apGain, 
						  unsigned char lfpGain) = 0;

	virtual void setApFilterState(unsigned char slot, 
								  signed char port, 
								  bool filterState) = 0;

	virtual void setSyncAsInput() = 0;
	virtual void setSyncAsOutput(int freqIndex) = 0;

	virtual Array<int> getSyncFrequencies() = 0;

	virtual void startAcquisition() = 0;
	virtual void stopAcquisition() = 0;

	virtual float getFillPercentage() = 0;

	unsigned char slot;

	String boot_version;

	ScopedPointer<BasestationConnectBoard> basestationConnectBoard;

	OwnedArray<Probe> probes;

	void setSavingDirectory(File directory) {
		savingDirectory = directory;
	}
	File getSavingDirectory() {
		return savingDirectory;
	}
	
protected:
	bool probesInitialized;

	Array<int> syncFrequencies;

	File savingDirectory;
};

class BasestationConnectBoard : public NeuropixComponent
{
public:
	BasestationConnectBoard(Basestation* bs_) : NeuropixComponent() {
		basestation = bs_;
	}
	String boot_version;

	Basestation* basestation;
};

enum BISTS {
	BIST_SIGNAL = 1,
	BIST_NOISE = 2,
	BIST_PSB = 3,
	BIST_SR = 4,
	BIST_EEPROM = 5,
	BIST_I2C = 6,
	BIST_SERDES = 7,
	BIST_HB = 8,
	BIST_BS = 9

};

typedef enum {
	DISCONNECTED, //There is no communication between probe and computer
	CONNECTING,   //Computer has detected the probe and is attempting to connect
	CONNECTED,    //Computer has established a valid connection with probe
	ACQUIRING,    //The probe is currently streaming data to computer
	RECORDING     //The probe is recording the streaming data
} ProbeStatus;

class Probe : public NeuropixComponent, public Thread
{
public:
	Probe(Basestation* bs_, int port_, int dock_) : NeuropixComponent(), Thread("ProbeThread")
	{
		port = port_;
		dock = dock_;
		basestation = bs_;
	}

	Basestation* basestation;
	int port;
	int dock;

	DataBuffer* apBuffer;
	DataBuffer* lfpBuffer;

	int64 ap_timestamp;
	int64 lfp_timestamp;

	ScopedPointer<Headstage> headstage;
	ScopedPointer<Flex> flex;

	int reference;
	int ap_gain;
	int lfp_gain;
	bool highpass_on;

	virtual void init() = 0;

	virtual void setChannels(Array<int> channelStatus) = 0;

	enum BANK_SELECT {
		BANK_0,
		BANK_1,
		BANK_2,
		DISCONNECTED = 0xFF
	};

	Array<BANK_SELECT> channelMap;

	Array<int> apGains;
	Array<int> lfpGains;

	virtual void setApFilterState(bool) = 0;
	virtual void setReferences(np::channelreference_t refId, unsigned char refElectrodeBank) = 0;
	virtual void setGains(unsigned char apGain, unsigned char lfpGain) = 0;

	virtual void calibrate() = 0;

	void setStatus(ProbeStatus status_) {
		status = status_;
	}
	ProbeStatus status;

	void setSelected(bool isSelected_) {
		isSelected = isSelected_;
	}
	bool isSelected;

	virtual void run() = 0;

	int channel_count;

	float fifoFillPercentage;

	String name;

	uint64 eventCode;
	Array<int> gains;

	np::electrodePacket packet[SAMPLECOUNT];

};

class Headstage : public NeuropixComponent
{
public:
	Headstage::Headstage(Probe* probe_) : NeuropixComponent() {
		probe = probe_;
	}
	Probe* probe;

	virtual bool hasTestModule() = 0;
};

class Flex : public NeuropixComponent
{
public:
	Flex::Flex(Probe* probe_)
	{
		probe = probe_;
	}
	Probe* probe;

};

class HeadstageTestModule : public NeuropixComponent
{
public:

	HeadstageTestModule::HeadstageTestModule(Basestation* bs_, signed char port_) : NeuropixComponent()
	{
		basestation = bs_;
		port = port_;
	}

	virtual void runAll() = 0;
	virtual void showResults() = 0;

private:
	Basestation* basestation;
	Headstage* headstage;

	unsigned char slot;
	signed char port;

	std::vector<std::string> tests;

};

#endif  // __NEUROPIXCOMPONENTS_H_2C4C2D67__