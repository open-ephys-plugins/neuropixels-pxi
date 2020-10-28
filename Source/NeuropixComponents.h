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
#include "API/v3/NeuropixAPI.h"

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

struct ElectrodeMetadata {
	int index;
	int xpos;
	int ypos;

};

/** Base class for all Neuropixels components, which must implement the "getInfo" method */
class NeuropixComponent
{
public:
	NeuropixComponent() {
		getInfo();
	}

	ComponentInfo info;

	virtual void getInfo() { }
};

/** Holds info about APIv1, as well as a boolean value to indicate whether or not it is being used*/
class NeuropixAPIv1 : public NeuropixComponent
{
public:
	NeuropixAPIv1::NeuropixAPIv1()
	{
		isActive = false;
	}

	void getInfo() {

		unsigned char version_major;
		unsigned char version_minor;
		np::getAPIVersion(&version_major, &version_minor);

		info.version = String(version_major) + "." + String(version_minor);
	}

	bool isActive;
};

/** Holds info about APIv3, as well as a boolean value to indicate whether or not it is being used*/
class NeuropixAPIv3 : public NeuropixComponent
{
public:

	NeuropixAPIv3::NeuropixAPIv3()
	{
		isActive = false;
	}

	void getInfo() {

		int version_major;
		int version_minor;
		Neuropixels::getAPIVersion(&version_major, &version_minor);

		info.version = String(version_major) + "." + String(version_minor);
	}

	bool isActive;
};

/** Represents a PXI basestation card */
class Basestation : public NeuropixComponent
{
public:

	/** Sets the slot values. */
	Basestation(int slot_) : NeuropixComponent() {
		probesInitialized = false;
		slot = slot_;
		slot_c = (unsigned char) slot_;
	}

	/** VIRTUAL METHODS */

	/** Opens the connection and retrieves info about available components; should be fast */
	virtual void open() = 0;

	/** Closes the connection */
	virtual void close() = 0;

	/** Initializes all components for acquisition; may inclue some delays */
	virtual void initialize() = 0;

	/** Runs a built-in self-test for a specified port */
	virtual bool runBist(signed char port, int bistIndex) = 0;

	/** Sets the sync channel as an "input" (for external sync) */
	virtual void setSyncAsInput() = 0;

	/** Sets the sync channel as an "output" (and specifies the frequency index) */
	virtual void setSyncAsOutput(int freqIndex) = 0;

	/** Returns an array of available sync frequencies for this basestation */
	virtual Array<int> getSyncFrequencies() = 0;

	/** Starts data streaming */
	virtual void startAcquisition() = 0;

	/** Stops data streaming */
	virtual void stopAcquisition() = 0;

	/** Returns the percentage of the FIFO buffer that is filled */
	virtual float getFillPercentage() = 0;

	/** Returns the total number of probes connected to this basestation */
	virtual int getProbeCount() = 0;

	/** Returns an array of probes connected to this basestation (cannot include null values) */
	Array<Probe*> getProbes()
	{
		return probes;
	}

	/** NON-VIRTUAL METHODS */

	/** Returns an array of headstages connected to this basestation 
		(can include null values for disconnected headstages) */
	Array<Headstage*> getHeadstages() {

		Array<Headstage*> headstage_array;

		for (auto h : headstages)
		{
			headstage_array.add(h);
		}

		return headstage_array;
	}

	unsigned char slot_c;
	int slot;

	ScopedPointer<BasestationConnectBoard> basestationConnectBoard;

	OwnedArray<Headstage> headstages;
	Array<Probe*> probes;

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

/** Represents a basestation connect board */
class BasestationConnectBoard : public NeuropixComponent
{
public:
	BasestationConnectBoard(Basestation* bs_) : NeuropixComponent() {
		basestation = bs_;
	}

	Basestation* basestation;
};


/** Represents a Neuropixels probe of any type */
class Probe : public NeuropixComponent, public Thread
{
public:
	Probe(Basestation* bs_, Headstage* hs_, Flex* fl_, int dock_) : NeuropixComponent(), Thread("ProbeThread")
	{
		dock = dock_;
		basestation = bs_;
		headstage = hs_;
		flex = fl_;
	}

	Basestation* basestation; // owned by NeuropixThread
	Headstage* headstage; // owned by Basestation
	Flex* flex; // owned by Headstage

	int dock;

	DataBuffer* apBuffer;
	DataBuffer* lfpBuffer;

	float ap_sample_rate;
	float lfp_sample_rate;

	int64 ap_timestamp;
	int64 lfp_timestamp;

	int reference;
	int ap_gain;
	int lfp_gain;
	bool highpass_on;

	enum BANK_SELECT {
		BANK_0,
		BANK_1,
		BANK_2,
		DISCONNECTED = 0xFF
	};

	Array<BANK_SELECT> channelMap;
	Array<int> apGains; // AP gain values for each channel
	Array<int> lfpGains; // LFP gain values for each channel

	/** VIRTUAL METHODS */

	/** Prepares the probe for data acquisition */
	virtual void initialize() = 0;

	/** Returns true if the probe generates LFP data */
	virtual bool generatesLfpData() = 0;

	/** Used to select channels.*/
	virtual void setChannelStatus(Array<int> channelStatus) = 0;

	/** Used to set references (same for all channels).*/
	virtual void setAllReferences(int referenceIndex) = 0;

	/** Used to set gains (same for all channels).*/
	virtual void setAllGains(int apGainIndex, int lfpGainIndex) = 0;

	/** Used to set AP filter cut (if available).*/
	virtual void setApFilterState(bool disableHighPass) = 0;

	/** Applies calibration info from a file.*/
	virtual void calibrate() = 0;

	/** Starts data streaming.*/
	virtual void startAcquisition() = 0;

	/** Stops data streaming.*/
	virtual void stopAcquisition() = 0;

	/** Main loop -- copies data from the probe into a DataBuffer object */
	virtual void run() = 0;

	/** NON-VIRTUAL METHODS */

	void setStatus(ProbeStatus status_) {
		status = status_;
	}

	ProbeStatus getStatus() {
		return status;
	}

	int channel_count;
	float ap_band_sample_rate;
	float lfp_band_sample_rate;

	float fifoFillPercentage;

	String name;

protected:

	ProbeStatus status;

	uint64 eventCode;
	Array<int> gains; // available gain values

};

/** Represents a Neuropixels headstage of any type */
class Headstage : public NeuropixComponent
{
public:
	Headstage::Headstage(Basestation* bs_, int port_) : NeuropixComponent() {
		basestation = bs_;
		port_c = (signed char) port_;
		port = port_;
	}

	Array<Probe*> getProbes() {

		Array<Probe*> probe_array;

		for (auto p : probes)
		{
			probe_array.add(p);
		}

		return probe_array;
	}
	Array<Flex*> getFlexCables()
	{
		Array<Flex*> flex_array;

		for (auto f : flexCables)
		{
			flex_array.add(f);
		}

		return flex_array;
	}
	
	signed char port_c;
	int port;

	OwnedArray<Probe> probes;
	OwnedArray<Flex> flexCables;

	Basestation* basestation;

	// ** Returns true if headstage test module is available */
	virtual bool hasTestModule() = 0;
};

/** Represents a Neuropixels flex cable */
class Flex : public NeuropixComponent
{
public:
	Flex::Flex(Headstage* hs_, int dock_)
	{
		headstage = hs_;
		dock = dock_;
	}

	Headstage* headstage;
	int dock;
};

/** Represents a Headstage Test Module */
class HeadstageTestModule : public NeuropixComponent
{
public:

	HeadstageTestModule::HeadstageTestModule(Basestation* bs_, Headstage* hs_) : NeuropixComponent()
	{
		basestation = bs_;
		headstage = hs_;
	}

	/** Run all available headstage tests */
	virtual void runAll() = 0;

	/** Show test results */
	virtual void showResults() = 0;

private:
	Basestation* basestation;
	Headstage* headstage;

};

#endif  // __NEUROPIXCOMPONENTS_H_2C4C2D67__