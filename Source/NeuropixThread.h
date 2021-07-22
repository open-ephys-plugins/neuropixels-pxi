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

#include "NeuropixComponents.h"


class SourceNode;
class NeuropixThread;
class OneBoxADC;

class RecordingTimer : public Timer
{

public:

	RecordingTimer(NeuropixThread* t_);
	void timerCallback();

	NeuropixThread* thread;
};

typedef enum {
	AP_BAND,
	LFP_BAND,
	ADC
} stream_type;

struct StreamInfo {
	int num_channels;
	float sample_rate;
	stream_type type;
	bool sendSyncAsContinuousChannel;
	Probe* probe;
	OneBoxADC* adc;
};

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

	/** Returns true if the data source is connected, false otherwise.*/
	bool foundInputSource();

	/** Returns version and serial number info for hardware and API as a string.*/
	String getInfoString();

	/** Returns version and serial number info for hardware and API as XML.*/
	XmlElement getInfoXml();

	/** Prepares probes for data acquisition */
	void initialize();

	/** Starts data transfer.*/
	bool startAcquisition() override;

	/** Stops data transfer.*/
	bool stopAcquisition() override;

	/** Update settings */
	void updateSettings(OwnedArray<ContinuousChannel>* continuousChannels,
		OwnedArray<EventChannel>* eventChannels,
		OwnedArray<SpikeChannel>* spikeChannels,
		OwnedArray<DataStream>* dataStreams,
		OwnedArray<DeviceInfo>* devices,
		OwnedArray<ConfigurationObject>* configurationObjects) override;

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

	std::unique_ptr<GenericEditor> createEditor(SourceNode* sn);

	Array<Basestation*> getBasestations();
	Array<Probe*> getProbes();
	Array<DataSource*> getDataSources();

	void setMainSync(int slotIndex);
	void setSyncOutput(int slotIndex);

	Array<int> getSyncFrequencies();

	void setSyncFrequency(int slotIndex, int freqIndex);

	/* Helper for loading probes in the background */
	Array<ProbeSettings> probeSettingsUpdateQueue;

	bool isInitialized() { return initializationComplete;  }

	void updateProbeSettingsQueue(ProbeSettings settings);
	void applyProbeSettingsQueue();

	void sendSyncAsContinuousChannel(bool shouldSend);
	void updateSubprocessors();

	String getApiVersion();

	void handleMessage(String msg) override;

	String handleConfigMessage(String msg) override;

	ScopedPointer<ProgressBar> progressBar;
	double initializationProgress;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NeuropixThread);

private:
	bool baseStationAvailable;
	bool probesInitialized;
	bool internalTrigger;
	bool recordToNpx;
	bool autoRestart;

	bool isRecording;

	bool initializationComplete;

	long int counter;
	int recordingNumber;

	CriticalSection displayMutex;

	void closeConnection();

	Array<int> defaultSyncFrequencies;

	Array<StreamInfo> streamInfo;

	int maxCounter;

	int totalChans;
	int totalProbes;

	uint32_t last_npx_timestamp;

	Array<float> fillPercentage;

	OwnedArray<Basestation> basestations;
	OwnedArray<DataStream> sourceStreams;

	NeuropixAPIv1 api_v1;
	NeuropixAPIv3 api_v3;

	RecordingTimer recordingTimer;

};





#endif  // __NEUROPIXTHREAD_H_2C4CBD67__
