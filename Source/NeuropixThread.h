/*
    ------------------------------------------------------------------

    This file is part of the Open Ephys GUI
    Copyright (C) 2024 Open Ephys

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

#define PLUGIN_VERSION "1.0.0-dev"

#define MINSTREAMBUFFERSIZE (1024 * 32)
#define MAXSTREAMBUFFERSIZE (1024 * 1024 * 32)
#define MINSTREAMBUFFERCOUNT (2)
#define MAXSTREAMBUFFERCOUNT (1024)

class SourceNode;
class NeuropixThread;
class NeuropixEditor;
class OneBoxADC;
class OneBox;
class Basestation;

typedef enum
{
    AP_BAND,
    LFP_BAND,
    BROAD_BAND,
    QUAD_BASE,
    ADC
} stream_type;

struct StreamInfo
{
    int num_channels;
    int probe_index;
    float sample_rate;
    stream_type type;
    int shank = -1;
    bool sendSyncAsContinuousChannel;
    Probe* probe;
    OneBoxADC* adc;
};

/** 
	
	Shows a progress window while searching for probes.

*/
class Initializer : public ThreadWithProgressWindow
{
public:
    /** Constructor */
    Initializer (NeuropixThread* neuropixThread_,
                 OwnedArray<Basestation>& basestations_,
                 DeviceType type_,
                 NeuropixAPIv3& api_v3_)
        : ThreadWithProgressWindow ("Scanning for connected Neuropixels devices", true, false),
          neuropixThread (neuropixThread_),
          basestations (basestations_),
          type (type_),
          api_v3 (api_v3_) {}

    ~Initializer() {}

    void run() override;

private:
    NeuropixThread* neuropixThread;
    OwnedArray<Basestation>& basestations;
    NeuropixAPIv3& api_v3;
    DeviceType type;
};

/**

	Communicates with imec Neuropixels probes.

	@see DataThread, SourceNode

*/

class NeuropixThread : public DataThread, public Timer
{
public:
    /** Constructor */
    NeuropixThread (SourceNode* sn, DeviceType type);

    /** Destructor */
    ~NeuropixThread();

    /** Static method for creating the DataThread object */
    static DataThread* createDataThread (SourceNode* sn, DeviceType type);

    /** Creates the custom editor */
    std::unique_ptr<GenericEditor> createEditor (SourceNode* sn);

    /** Not used -- data is acquired by the individual Probe objects*/
    bool updateBuffer() { return true; }

    /** Returns true if the data source is connected, false otherwise.*/
    bool foundInputSource();

    /** Returns version and serial number info for hardware and API as a string.*/
    String getInfoString();

    /** Returns version and serial number info for hardware and API as XML.*/
    XmlElement getInfoXml();

    /** Called by ProcessorGraph to inform the thread whether the signal chain is loading */
    void initialize (bool signalChainIsLoading) override;

    /** Prepares probes for data acquisition*/
    void initializeBasestations (bool signalChainIsLoading);

    /** Starts data transfer.*/
    bool startAcquisition() override;

    /** Stops data transfer.*/
    bool stopAcquisition() override;

    /** Update settings */
    void updateSettings (OwnedArray<ContinuousChannel>* continuousChannels,
                         OwnedArray<EventChannel>* eventChannels,
                         OwnedArray<SpikeChannel>* spikeChannels,
                         OwnedArray<DataStream>* dataStreams,
                         OwnedArray<DeviceInfo>* devices,
                         OwnedArray<ConfigurationObject>* configurationObjects) override;

    /** Toggles between internal and external triggering. */
    void setTriggerMode (bool trigger);

    /** Select directory for saving NPX files. */
    void setDirectoryForSlot (int slotIndex, File directory);

    /** Select directory for saving NPX files. */
    File getDirectoryForSlot (int slotIndex);

    /** Sets the probe naming scheme to use for this slot */
    void setNamingSchemeForSlot (int slot, ProbeNameConfig::NamingScheme namingScheme);

    /** Gets the probe naming scheme for this slot */
    ProbeNameConfig::NamingScheme getNamingSchemeForSlot (int slot);

    /** Generates a unique name for each probe */
    String generateProbeName (int probeIndex, ProbeNameConfig::NamingScheme scheme);

    /** Toggles between auto-restart setting. */
    void setAutoRestart (bool restart);

    /** Starts data acquisition after a certain time.*/
    void timerCallback();

    /** Returns a mutex for live rendering of data */
    CriticalSection* getMutex()
    {
        return &displayMutex;
    }

    /** Returns pointers to the active Basestation objects */
    Array<Basestation*> getBasestations();

    /** Returns pointers to active OneBox objects */
    Array<OneBox*> getOneBoxes();

    /** Returns pointers to opto-compatible basestations */
    Array<Basestation*> getOptoBasestations();

    /** Returns pointers to active probes */
    Array<Probe*> getProbes();

    /** Returns a JSON-formatted string with info about all connected probes*/
    String getProbeInfoString();

    /** Returns pointer to active DataSources (probes + ADCs)*/
    Array<DataSource*> getDataSources();

    /** Determines which PXI slot is the primary sync */
    void setMainSync (int slotIndex);

    /** Sets the sync for a PXI slot to output mode */
    void setSyncOutput (int slotIndex);

    /** Returns an array of available sync frequencies */
    Array<int> getSyncFrequencies();

    /** Sets the sync frequency for a particular slot*/
    void setSyncFrequency (int slotIndex, int freqIndex);

    /* Helper for loading probes in the background */
    Array<ProbeSettings> probeSettingsUpdateQueue;

    /** Returns true if initialization process if finished */
    bool isInitialized() { return initializationComplete; }

    /** Adds a settings object to the background queue */
    void updateProbeSettingsQueue (ProbeSettings settings);

    /** Applies all the settings in the current queue */
    void applyProbeSettingsQueue();

    /** Sets whether the sync line is added as an extra (385th) continuous channel */
    void sendSyncAsContinuousChannel (bool shouldSend);

    /** Adds info about all the available data streams */
    void updateStreamInfo();

    /** Returns the current API version as a string */
    String getApiVersion();

    /** Responds to broadcast messages sent during acquisition */
    void handleBroadcastMessage (String msg) override;

    /** Responds to config messages sent while acquisition is not active*/
    String handleConfigMessage (String msg) override;

    /** Returns the custom name for a given probe serial number, if it exists*/
    String getCustomProbeName (String serialNumber);

    /** Sets the custom name for a given probe serial number*/
    void setCustomProbeName (String serialNumber, String customName);

    /** Allows probes to send broadcast messages to downstream processors */
    void sendBroadcastMessage (String msg) { broadcastMessage (msg); }

    std::map<String, String> customProbeNames;

    DeviceType type;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NeuropixThread);

private:
    bool baseStationAvailable;
    bool probesInitialized;
    bool internalTrigger;
    bool autoRestart;

    bool calibrationWarningShown;

    bool initializationComplete;
    bool configurationComplete;

    long int counter;

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

    std::unique_ptr<Initializer> initializer;

    NeuropixAPIv3 api_v3;

    NeuropixEditor* editor;
    GenericProcessor* processor;
};

#endif // __NEUROPIXTHREAD_H_2C4CBD67__
