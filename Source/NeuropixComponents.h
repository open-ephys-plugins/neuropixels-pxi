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

#include "UI/ActivityView.h"

# define SAMPLECOUNT 64
# define MAX_HEADSTAGE_CLK_SAMPLE 3221225475
# define MAX_ALLOWABLE_TIMESTAMP_JUMP 4

class BasestationConnectBoard;
class Flex;
class Headstage;
class Probe;
class Basestation;
class NeuropixInterface;

struct ComponentInfo
{
	String version = "UNKNOWN";
	uint64_t serial_number = 0;
	int SN = 0;
	String part_number = "";
	String boot_version = "";
};

enum class DataSourceType {
	PROBE,
	ADC,
	DAC,
	NONE
};

enum class BasestationType {
	V1,
	V3,
	OPTO,
	ONEBOX,
	SIMULATED
};

enum class ProbeType {
	NONE,
	NP1,
	NHP10,
	NHP25,
	NHP45,
	NHP1,
	UHD1,
	UHD2,
	NP2_1,
	NP2_4
};

enum class SourceStatus {
	DISCONNECTED, //There is no communication between probe and computer
	CONNECTING,   //Computer has detected the probe and is attempting to connect
	CONNECTED,    //Computer has established a valid connection with probe
	UPDATING,	  //The probe is currently updating its settings
	ACQUIRING,    //The probe is currently streaming data to computer
	RECORDING,    //The probe is recording the streaming data
};

enum class Bank {
	NONE = -1,
	A = 0,
	B = 1,
	C = 2,
	D = 3,
	E = 4,
	F = 5,
	G = 6,
	H = 7,
	I = 8,
	J = 9,
	K = 10,
	L = 11,
	M = 12,
	OFF = 255 //used in v1 API
};

enum class ElectrodeStatus {
	CONNECTED,
	DISCONNECTED,
	REFERENCE,
	OPTIONAL_REFERENCE,
	CONNECTED_OPTIONAL_REFERENCE
};

enum class BIST {
	EMPTY = 0,
	SIGNAL = 1,
	NOISE = 2,
	PSB = 3,
	SR = 4,
	EEPROM = 5,
	I2C = 6,
	SERDES = 7,
	HB = 8,
	BS = 9
};

enum class FirmwareType {
	BS_FIRMWARE,
	BSC_FIRMWARE
};

enum class AdcInputRange {
	PLUSMINUS2PT5V = 0,
	PLUSMINUS5V = 1,
	PLUSMINUS10V = 2
};

struct ProbeMetadata {
	int shank_count;
	int electrodes_per_shank;
	Path shankOutline;
	int columns_per_shank;
	int rows_per_shank;
	ProbeType type;
	String name;
	Array<Bank> availableBanks;
};

struct ElectrodeMetadata {
	int global_index;
	int shank_local_index;
	int shank;
	int column_index;
	int channel;
	int row_index;
	float xpos; // position on shank, in microns
	float ypos; // position on shank, in microns
	float site_width; // in microns
	Bank bank;
	ElectrodeStatus status;
	bool isSelected;
	Colour colour;
};



struct ProbeSettings {

	Array<float> availableApGains; // Available AP gain values for each channel (if any)
	Array<float> availableLfpGains; // Available LFP gain values for each channel (if any)
	Array<String> availableReferences; // reference types
	Array<Bank> availableBanks; // bank inds

	int apGainIndex;
	int lfpGainIndex;
	int referenceIndex;
	bool apFilterState;

	Array<Bank> selectedBank;    // size = channels
	Array<int> selectedShank;    // size = channels
	Array<int> selectedChannel;  // size = channels
	Array<int> selectedElectrode; // size = channels

	void clearElectrodeSelection() {
		selectedBank.clear();
		selectedShank.clear();
		selectedChannel.clear();
		selectedElectrode.clear();
	}

	ProbeType probeType;

	Probe* probe; // pointer to the probe
};

/** Base class for all Neuropixels components, which must implement the "getInfo" method */
class NeuropixComponent
{
public:
	NeuropixComponent() { }

	ComponentInfo info;

	virtual void getInfo() = 0;
};

/** Holds info about APIv1, as well as a boolean value to indicate whether or not it is being used*/
class NeuropixAPIv1 : public NeuropixComponent
{
public:
	NeuropixAPIv1::NeuropixAPIv1()
	{
		getInfo();
		isActive = false;
	}

	void getInfo() override {

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
		getInfo();
		isActive = false;
	}

	void getInfo() override {

		int version_major;
		int version_minor;
		Neuropixels::getAPIVersion(&version_major, &version_minor);

		info.version = String(version_major) + "." + String(version_minor);
	}

	bool isActive;
};

class DataSource : public NeuropixComponent, public Thread
{
public:
	DataSource(Basestation* bs_) : NeuropixComponent(), Thread("DataSourceThread")
	{
		basestation = bs_;
	}

	/** Opens the connection to the probe */
	virtual bool open() = 0;

	/** Closes the connection to the probe */
	virtual bool close() = 0;

	/** Prepares the probe for data acquisition */
	virtual void initialize(bool signalChainIsLoading) = 0;

	/** Starts data streaming.*/
	virtual void startAcquisition() = 0;

	/** Stops data streaming.*/
	virtual void stopAcquisition() = 0;

	Basestation* basestation;

	int channel_count;

	float sample_rate;

	DataSourceType sourceType;

	DataBuffer* apBuffer;

	void setStatus(SourceStatus status_) {
		status = status_;
	}

	SourceStatus getStatus() {
		return status;
	}

protected:
	SourceStatus status;
};


/** Represents a Neuropixels probe of any type */
class Probe : public DataSource
{
public:
	Probe(Basestation* bs_, Headstage* hs_, Flex* fl_, int dock_) : DataSource(bs_)
	{
		dock = dock_;
		headstage = hs_;
		flex = fl_;
		isValid = false;

		isCalibrated = false;
		calibrationWarningShown = false;

		sourceType = DataSourceType::PROBE;

		for (int i = 0; i < 384; i++)
		{
			for (int j = 0; j < 100; j++)
			{
				ap_offsets[i][j] = 0;
				lfp_offsets[i][j] = 0;
			}
		}
	}

	Headstage* headstage; // owned by Basestation
	Flex* flex; // owned by Headstage

	bool isValid; //True if the PN is supported by the API

	bool isCalibrated = false;
	bool calibrationWarningShown;

	int port;
	int dock;
	
	DataBuffer* lfpBuffer;

	float ap_sample_rate;
	float lfp_sample_rate;

	float ap_offsets[384][100];
	float lfp_offsets[384][100];

	int64 ap_timestamp;
	int64 lfp_timestamp;

	Array<ElectrodeMetadata> electrodeMetadata;
	ProbeMetadata probeMetadata;

	ProbeSettings settings;

	Path shankOutline;
	
	NeuropixInterface* ui;

	/** VIRTUAL METHODS */

	/** Returns true if the probe generates LFP data */
	virtual bool generatesLfpData() = 0;

	/** Returns true if the probe has a selectable AP filter cut */
	virtual bool hasApFilterSwitch() = 0;

	/** Used to select channels. Returns the currently active electrodes*/
	virtual void selectElectrodes() = 0;

	/** Used to set references (same for all channels).*/
	virtual void setAllReferences() = 0;

	/** Used to set gains (same for all channels).*/
	virtual void setAllGains() = 0;

	/** Used to set AP filter cut (if available).*/
	virtual void setApFilterState() = 0;

	/** Writes probe configuration after calling setAllReferences / setAllGains / selectElectrodes */
	virtual void writeConfiguration() = 0;

	/** Applies calibration info from a file.*/
	virtual void calibrate() = 0;

	/** Runs a built-in self-test for a specified port */
	virtual bool runBist(BIST bistType) = 0;

	/** Main loop -- copies data from the probe into a DataBuffer object */
	virtual void run() = 0;

	/** NON-VIRTUAL METHODS */
	void updateSettings(ProbeSettings p)
	{
		settings = p;
	}

	void updateOffsets(float* samples, int64 timestamp, bool isApBand);

	int ap_offset_counter = 0;
	int lfp_offset_counter = 0;

	ProbeType type;
	
	int electrode_count;
	float ap_band_sample_rate;
	float lfp_band_sample_rate;

	int buffer_size;

	float fifoFillPercentage;

	/* Stores the generic probe model name e.g. Neuripixels 2.0 - Single Shank */
	String name;

	/* Stores the name assigned to the probe/stream (default is autoName) */
	String streamName;

	/* Assign a custom naming scheme to the probe */
	String autoName;
	String autoNumber;
	String customPort;
	String customProbe;

	StringArray autoProbeNames = { "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N",
						   "O" , "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z" };

	void sendSyncAsContinuousChannel(bool shouldSend) {
		sendSync = shouldSend;
	}

	bool sendSync;

	uint32_t last_npx_timestamp;
	bool passedOneSecond;

	const float* getPeakToPeakValues(ActivityToView currentView = ActivityToView::APVIEW)
	{
		if (currentView == ActivityToView::APVIEW)
			return apView->getPeakToPeakValues();
		else
			return lfpView->getPeakToPeakValues();
	}

protected:

	ScopedPointer<ActivityView> apView;
	ScopedPointer<ActivityView> lfpView;

	uint64 eventCode;
	Array<int> gains; // available gain values

};

class Basestation;

class FirmwareUpdater : public ThreadWithProgressWindow
{
public:

	/** Constructor */
	FirmwareUpdater(Basestation* basestation, File firmwareFile, FirmwareType type);

	/** Destructor */
	~FirmwareUpdater() {}

	/** Thread for firmware update */
	void run() override;

	/** Callback to update progress bar*/
	static int firmwareUpdateCallback(size_t bytes)
	{
		currentThread->setProgress(float(bytes) / totalFirmwareBytes);

		return 1;
	}

	static FirmwareUpdater* currentThread;
	static float totalFirmwareBytes;

	Basestation* basestation;
	FirmwareType firmwareType;
	String firmwareFilePath;

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

		bsFirmwarePath = "";
		bscFirmwarePath = "";
	}

	/** VIRTUAL METHODS */

	/** Opens the connection and retrieves info about available components; should be fast */
	/** Returns false if the API version does not match */
	virtual bool open() = 0;

	/** Closes the connection */
	virtual void close() = 0;

	/** Initializes all components for acquisition; may inclue some delays */
	virtual void initialize(bool signalChainIsLoading) = 0;

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

	/** Launches FirmwareUpdater to update the BSC firmware */
	void updateBscFirmware(File file);

	/** Launches FirmwareUpdater to update the BS firmware */
	void updateBsFirmware(File file);

	/** Checks the status of any initialization threads */
	virtual bool isBusy() { return false;  }

	/** Waits for initialization threads to exit */
	virtual void waitForThreadToExit() { }

	BasestationType type;
	
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

	virtual Array<DataSource*> getAdditionalDataSources() { 
		
		return Array<DataSource*>();

	}
	
	/** Returns an array of probes connected to this basestation (cannot include null values) */
	Array<Probe*> getProbes()
	{
		return probes;
	}

	/** Informs all probes to add the sync channel value to the continuous buffer */
	void sendSyncAsContinuousChannel(bool shouldSend) {

		for (auto probe : getProbes())
		{
			probe->sendSyncAsContinuousChannel(shouldSend);
		}
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

	void setNamingScheme(int schemeIdx) {
		namingSchemeIdx = schemeIdx;
	}

	int getNamingScheme() {
		return namingSchemeIdx;
	}
	
protected:

	bool probesInitialized;
	Array<int> syncFrequencies;
	File savingDirectory;
	int namingSchemeIdx = 0;
	
	String bscFirmwarePath;
	String bsFirmwarePath;
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

	HeadstageTestModule* testModule;

	Basestation* basestation;

	// ** Returns true if headstage test module is available */
	virtual bool hasTestModule() = 0;
	// ** Runs the headstage test module and shows the results in a pop-window */
	virtual void runTestModule() = 0;
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




#endif  // __NEUROPIXCOMPONENTS_H_2C4C2D67__