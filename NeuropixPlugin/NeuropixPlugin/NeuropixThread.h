/*
    ------------------------------------------------------------------

    This file is part of the Open Ephys GUI
    Copyright (C) 2017 Allen Institute for Brain Science and Open Ephys

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


#ifndef __NEUROPIXTHREAD_H_2C4CBD67__
#define __NEUROPIXTHREAD_H_2C4CBD67__

#include <DataThreadHeaders.h>
#include <stdio.h>
#include <string.h>

#include "neuropix-api/NeuropixAPI.h"
#include "NeuropixComponents.h"

# define SAMPLECOUNT 64

class SourceNode;

/**

	Communicates with imec Neuropixels probes.

	@see DataThread, SourceNode

*/

class NeuropixThread : public DataThread, public Timer
{

public:

	NeuropixThread(SourceNode* sn);
	~NeuropixThread();

	bool updateBuffer();

	void updateChannels();

	/** Returns true if the data source is connected, false otherwise.*/
	bool foundInputSource();

	/** Returns version info for hardware and API.*/
	String getInfoString();

	/** Initializes data transfer.*/
	bool startAcquisition() override;

	/** Stops data transfer.*/
	bool stopAcquisition() override;

	// DataThread Methods

	/** Returns the number of virtual subprocessors this source can generate */
	unsigned int getNumSubProcessors() const override;

	/** Returns the number of continuous headstage channels the data source can provide.*/
	int getNumDataOutputs(DataChannel::DataChannelTypes type, int subProcessorIdx) const override;

	/** Returns the number of TTL channels that each subprocessor generates*/
	int getNumTTLOutputs(int subProcessorIdx) const override;

	/** Returns the sample rate of the data source.*/
	float getSampleRate(int subProcessorIdx) const override;

	/** Returns the volts per bit of the data source.*/
	float getBitVolts(const DataChannel* chan) const override;

	/** Used to set default channel names.*/
	void setDefaultChannelNames() override;

	/** Used to override default channel names.*/
	bool usesCustomNames() const override;

	/** Selects which electrode is connected to each channel. */
	void selectElectrode(int chNum, int connection, bool transmit);

	/** Selects which reference is used for each channel. */
	void setAllReferences(int refId);

	/** Sets the gain for each channel. */
	void setAllGains(unsigned char apGain, unsigned char lfpGain);

	/** Sets the filter for all channels. */
	void setFilter(bool filterState);

	/** Toggles between internal and external triggering. */
	void setTriggerMode(bool trigger);

	/** Toggles between saving to NPX file. */
	void setRecordMode(bool record);

	/** Select directory for saving NPX files. */
	void setDirectoryForSlot(int slotIndex, File directory);

	/** Select directory for saving NPX files. */
	File getDirectoryForSlot(int slotIndex);

	/** Toggles between auto-restart setting. */
	void setAutoRestart(bool restart);

	/** Starts data acquisition after a certain time.*/
	void timerCallback();

	/** Starts recording.*/
	void startRecording();

	/** Stops recording.*/
	void stopRecording();

	CriticalSection* getMutex()
	{
		return &displayMutex;
	}

	static DataThread* createDataThread(SourceNode* sn);

	GenericEditor* createEditor(SourceNode* sn);

	void setSelectedProbe(unsigned char slot, signed char probe);

	bool checkSlotAndPortCombo(int slotIndex, int portIndex);
	unsigned char getSlotForIndex(int slotIndex, int portIndex);
	signed char getPortForIndex(int slotIndex, int portIndex);

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NeuropixThread);

private:
	bool baseStationAvailable;
	bool probesInitialized;
	bool internalTrigger;
	bool recordToNpx;
	bool autoRestart;

	bool isRecording;

	long int counter;
	int recordingNumber;

	CriticalSection displayMutex;

	Array<int> gains;
	Array<int> apGains;
	Array<int> lfpGains;
	Array<int> channelMap;
	Array<bool> outputOn;
	Array<int> refs;

	uint64_t probeId;

	void openConnection();
	void closeConnection();

	int64 timestampAp;
	int64 timestampLfp;
	uint64 eventCode;
	int maxCounter;
	int numRefs;
	int totalChans;

	OwnedArray<Basestation> basestations;

	NP_ErrorCode errorCode;
	NeuropixAPI api;

	unsigned char selectedSlot;
	signed char selectedPort;

	//std::vector<unsigned char> connected_basestations;
	//std::vector<std::vector<int>> connected_probes;
	electrodePacket packet[SAMPLECOUNT];

};

#endif  // __NEUROPIXTHREAD_H_2C4CBD67__
