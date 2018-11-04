#ifndef __NEUROPIXCOMPONENTS_H_2C4C2D67__
#define __NEUROPIXCOMPONENTS_H_2C4C2D67__

#include <DataThreadHeaders.h>
#include <stdio.h>
#include <string.h>

#include "neuropix-api/NeuropixAPI.h"

class NeuropixComponent
{
public:
	NeuropixComponent();
	~NeuropixComponent();

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

	BasestationConnectBoard basestationConnectBoard;

	OwnedArray<Probe> probes;

	float getTemperature();

	void getInfo();
	void makeSyncMaster();
};

class BasestationConnectBoard : public NeuropixComponent
{
public:
	
	String boot_version;

	Basestation basestation;

	void getInfo();
};

class Probe : public NeuropixComponent
{
public:
	Probe(Basestation bs, signed char port);
	~Probe();

	Basestation basestation;
	signed char port;

	Headstage headstage;
	Flex flex;

	int reference;
	int ap_gain;
	int lfp_gain;
	bool highpass_on;

	Array<bool> selectedElectrodes;

	void calibrate();

	void getInfo();

	int channel_count;

	String name;

};

class Headstage : public NeuropixComponent
{
public:
	Probe probe;
	void getInfo();
};

class Flex : public NeuropixComponent
{
public:
	Probe probe;
	void getInfo();
};

#endif  // __NEUROPIXCOMPONENTS_H_2C4C2D67__