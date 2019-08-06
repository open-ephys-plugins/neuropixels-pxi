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


# define SAMPLECOUNT 64

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

	void init();

	int getProbeCount();

	String boot_version;

	ScopedPointer<BasestationConnectBoard> basestationConnectBoard;

	OwnedArray<Probe> probes;

	void initializeProbes();

	float getTemperature();

	void setChannels(unsigned char slot, signed char port, Array<int> channelStatus);
	void setReferences(unsigned char slot, signed char port, np::channelreference_t refId, unsigned char electrodeBank);
	void setGains(unsigned char slot, signed char port, unsigned char apGain, unsigned char lfpGain);
	void setApFilterState(unsigned char slot, signed char port, bool filterState);

	void getInfo();

	void setSyncAsInput();
	void setSyncAsOutput(int freqIndex);

	Array<int> getSyncFrequencies();

	void startAcquisition();
	void stopAcquisition();

	void setSavingDirectory(File);
	File getSavingDirectory();

	float getFillPercentage();
	

private:
	bool probesInitialized;

	Array<int> syncFrequencies;

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
	Probe(Basestation* bs, signed char port);

	Basestation* basestation;
	signed char port;

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

	void init();

	void setChannels(Array<int> channelStatus);
	enum BANK_SELECT {
		BANK_0,
		BANK_1,
		BANK_2,
		DISCONNECTED = 0xFF
	};
	Array<BANK_SELECT> channelMap;

	Array<int> apGains;
	Array<int> lfpGains;

	void setApFilterState(bool);
	void setReferences(np::channelreference_t refId, unsigned char refElectrodeBank);
	void setGains(unsigned char apGain, unsigned char lfpGain);

	void calibrate();

	void setStatus(ProbeStatus);
	ProbeStatus status;

	void setSelected(bool isSelected_);
	bool isSelected;

	void getInfo();

	int channel_count;

	float fifoFillPercentage;

	String name;

	void run();

	uint64 eventCode;
	Array<int> gains;

	np::electrodePacket packet[SAMPLECOUNT];

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

typedef struct HST_Status {
	np::NP_ErrorCode VDD_A1V2;
	np::NP_ErrorCode VDD_A1V8;
	np::NP_ErrorCode VDD_D1V2;
	np::NP_ErrorCode VDD_D1V8;
	np::NP_ErrorCode MCLK;
	np::NP_ErrorCode PCLK;
	np::NP_ErrorCode PSB;
	np::NP_ErrorCode I2C;
	np::NP_ErrorCode NRST;
	np::NP_ErrorCode REC_NRESET;
	np::NP_ErrorCode SIGNAL;
};

class HeadstageTestModule : public NeuropixComponent
{
public:

	HeadstageTestModule::HeadstageTestModule(Basestation* bs, signed char port);

	void getInfo();

	np::NP_ErrorCode test_VDD_A1V2();
	np::NP_ErrorCode test_VDD_A1V8();
	np::NP_ErrorCode test_VDD_D1V2();
	np::NP_ErrorCode test_VDD_D1V8();
	np::NP_ErrorCode test_MCLK();
	np::NP_ErrorCode test_PCLK();
	np::NP_ErrorCode test_PSB();
	np::NP_ErrorCode test_I2C();
	np::NP_ErrorCode test_NRST();
	np::NP_ErrorCode test_REC_NRESET();
	np::NP_ErrorCode test_SIGNAL();

	void runAll();
	void showResults();

private:
	Basestation* basestation;
	Headstage* headstage;

	unsigned char slot;
	signed char port;

	std::vector<std::string> tests;

	ScopedPointer<HST_Status> status;

};

#endif  // __NEUROPIXCOMPONENTS_H_2C4C2D67__