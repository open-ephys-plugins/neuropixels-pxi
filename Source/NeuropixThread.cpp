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

#include "NeuropixThread.h"
#include "NeuropixEditor.h"

#include "Basestations/Basestation_v1.h"
#include "Basestations/Basestation_v3.h"
#include "Basestations/OneBox.h"
#include "Basestations/SimulatedBasestation.h"
#include "Probes/OneBoxADC.h"

#include <vector>

#include "Utils.h"

DataThread* NeuropixThread::createDataThread(SourceNode *sn)
{
	return new NeuropixThread(sn);
}

std::unique_ptr<GenericEditor> NeuropixThread::createEditor(SourceNode* sn)
{
	std::unique_ptr<NeuropixEditor> editor = std::make_unique<NeuropixEditor>(sn, this, true);

	return editor;
}

NeuropixThread::NeuropixThread(SourceNode* sn) :
	DataThread(sn),
	baseStationAvailable(false),
	probesInitialized(false),
	recordingNumber(0),
	isRecording(false),
	recordingTimer(this),
	recordToNpx(false),
	initializationComplete(false)
{

	progressBar = new ProgressBar(initializationProgress);

	defaultSyncFrequencies.add(1);
	defaultSyncFrequencies.add(10);

	api_v1.isActive = false;
	api_v3.isActive = true;

	LOGD("Scanning for devices...");

	Neuropixels::scanBS();
	Neuropixels::basestationID list[16];
	int count = getDeviceList(&list[0], 16);

	LOGD("  Found ", count, " device", count == 1 ? "." : "s.");

	for (int i = 0; i < count; i++)
	{

		Neuropixels::NP_ErrorCode ec = getDeviceInfo(list[i].ID, &list[i]);

		int slotID;

		bool foundSlot = tryGetSlotID(&list[i], &slotID);

		if (foundSlot && list[i].platformid == Neuropixels::NPPlatform_PXI)
		{

			Basestation* bs = new Basestation_v3(slotID); 

			if (bs->open()) //returns true if Basestation firmware >= 2.0
			{
				basestations.add(bs);

				if (!bs->getProbeCount())
					CoreServices::sendStatusMessage("Neuropixels PXI basestation found, no probes connected.");
			}
			else
			{
				delete bs;
			}

		}
		else {

			Basestation* bs = new OneBox(list[i].ID);

			if (bs->open())
			{

				basestations.add(bs);

				if (!bs->getProbeCount())
					CoreServices::sendStatusMessage("OneBox found, no probes connected.");
			}
			else
			{
				delete bs;
			}
		}
	}


	if (basestations.size() == 0) // no basestations with API version match
	{
		LOGD("Checking for V1 basestations...");

		uint32_t availableslotmask;

		std::vector<int> slotsToCheck;
		np::scanPXI(&availableslotmask);

		for (int slot = 0; slot < 32; slot++)
		{
			if ((availableslotmask >> slot) & 1)
			{

				LOGD("  Found V1 Basestation");

				Basestation* bs = new Basestation_v1(slot);

				if (bs->open()) // detects # of probes; returns true if API version matches
				{
					api_v1.isActive = true ;
					api_v3.isActive = false;
					basestations.add(bs);
				}
				else {
					delete bs;
				}

			}
		}
	}
	else
	{
		LOGD("Found ", basestations.size(), " V3 basestation", basestations.size() > 1 ? "s" : "");
	}

	if (basestations.size() == 0) // no basestations at all
	{
		bool response = AlertWindow::showOkCancelBox(AlertWindow::NoIcon,
			"No basestations detected", 
			"No Neuropixels PXI basestations were detected. Do you want to run this plugin in simulation mode?",
			"Yes", "No", 0, 0);

		if (response)
		{
			basestations.add(new SimulatedBasestation(0));
			basestations.getLast()->open(); // detects # of probes
		}
		
	}

	bool foundSync = false;

	for (auto probe : getProbes())
	{
		baseStationAvailable = true;

		if (!foundSync)
		{
			probe->basestation->setSyncAsInput();
			foundSync = true;
		}
	}

	updateSubprocessors();
}

void NeuropixThread::updateSubprocessors()
{

	streamInfo.clear();
	sourceBuffers.clear();

	for (auto source : getDataSources() )
	{

		if (source->sourceType == DataSourceType::PROBE)
		{
			StreamInfo apInfo;

			Probe* probe = (Probe*) source;

			apInfo.num_channels = probe->sendSync ? probe->channel_count + 1 : probe->channel_count;
			apInfo.sample_rate = probe->ap_sample_rate;
			apInfo.probe = probe;

			apInfo.type = AP_BAND;
			apInfo.sendSyncAsContinuousChannel = probe->sendSync;

			streamInfo.add(apInfo);

			sourceBuffers.add(new DataBuffer(apInfo.num_channels, 10000));  // AP band buffer
			probe->apBuffer = sourceBuffers.getLast();

			if (probe->generatesLfpData())
			{

				StreamInfo lfpInfo;
				lfpInfo.num_channels = probe->sendSync ? probe->channel_count + 1 : probe->channel_count;
				lfpInfo.sample_rate = probe->lfp_sample_rate;
				lfpInfo.type = LFP_BAND;
				lfpInfo.probe = probe;
				lfpInfo.sendSyncAsContinuousChannel = probe->sendSync;

				sourceBuffers.add(new DataBuffer(lfpInfo.num_channels, 10000));  // LFP band buffer
				probe->lfpBuffer = sourceBuffers.getLast();

				streamInfo.add(lfpInfo);
			}

			LOGD("Probe (slot=", probe->basestation->slot, ", port=", probe->headstage->port, ") CH=", apInfo.num_channels, " SR=", apInfo.sample_rate, " Hz");
		}
		else {

			StreamInfo adcInfo;

			adcInfo.num_channels = source->channel_count;
			adcInfo.sample_rate = source->sample_rate;
			adcInfo.type = ADC;
			adcInfo.sendSyncAsContinuousChannel = false;
			adcInfo.probe = nullptr;
			adcInfo.adc = (OneBoxADC*)source;

			sourceBuffers.add(new DataBuffer(adcInfo.num_channels, 10000));  // data buffer
			source->apBuffer = sourceBuffers.getLast();

			streamInfo.add(adcInfo);

		}
		
	}

}

NeuropixThread::~NeuropixThread()
{
    closeConnection();
}

void NeuropixThread::updateProbeSettingsQueue(ProbeSettings settings)
{
	probeSettingsUpdateQueue.add(settings);
}

void NeuropixThread::applyProbeSettingsQueue()
{

	for (auto settings : probeSettingsUpdateQueue)
	{
		settings.probe->setStatus(SourceStatus::UPDATING);
	}

	for (auto settings: probeSettingsUpdateQueue)
	{

		if (settings.probe != nullptr)
		{

			settings.probe->selectElectrodes();
			settings.probe->setAllGains();
			settings.probe->setAllReferences();
			settings.probe->setApFilterState();

			settings.probe->calibrate();

			settings.probe->writeConfiguration();

			settings.probe->setStatus(SourceStatus::CONNECTED);
		}
	}

	probeSettingsUpdateQueue.clear();
}

void NeuropixThread::initialize()
{
	// slower task, run in background thread

	for (auto basestation : basestations)
	{
		basestation->initialize(); // prepares probes for acquisition; may be slow
	}

	if (api_v1.isActive)
	{
		np::setParameter(np::NP_PARAM_BUFFERSIZE, MAXSTREAMBUFFERSIZE);
		np::setParameter(np::NP_PARAM_BUFFERCOUNT, MAXSTREAMBUFFERCOUNT);
	}
	else {
		Neuropixels::setParameter(Neuropixels::NP_PARAM_BUFFERSIZE, MAXSTREAMBUFFERSIZE);
		Neuropixels::setParameter(Neuropixels::NP_PARAM_BUFFERCOUNT, MAXSTREAMBUFFERCOUNT);
	}

	initializationComplete = true;
	
}

Array<Basestation*> NeuropixThread::getBasestations()
{
	Array<Basestation*> bs;

	for (auto bs_ : basestations)
	{
		bs.add(bs_);
	}

	return bs;
}

Array<Probe*> NeuropixThread::getProbes()
{
	Array<Probe*> probes;

	for (auto bs : basestations)
	{
		for (auto probe : bs->getProbes())
		{
			probes.add(probe);
		}
	}

	return probes;
}


Array<DataSource*> NeuropixThread::getDataSources()
{
	Array<DataSource*> sources;

	for (auto bs : basestations)
	{
		for (auto probe : bs->getProbes())
		{
			sources.add(probe);
		}

		for (auto additionalSource : bs->getAdditionalDataSources())
		{
			sources.add(additionalSource);
		}
	}

	return sources;
}

String NeuropixThread::getApiVersion()
{
	if (api_v1.isActive)
		return api_v1.info.version;
	else
		return api_v3.info.version;
}

void NeuropixThread::setMainSync(int slotIndex)
{
	basestations[slotIndex]->setSyncAsInput();
}

void NeuropixThread::setSyncOutput(int slotIndex)
{
	basestations[slotIndex]->setSyncAsOutput(0);
}

Array<int> NeuropixThread::getSyncFrequencies()
{
	if (basestations.size() > 0)
		return basestations[0]->getSyncFrequencies();
	else
		return defaultSyncFrequencies;
		
}


void NeuropixThread::setSyncFrequency(int slotIndex, int freqIndex)
{
	basestations[slotIndex]->setSyncAsOutput(freqIndex);
}


void NeuropixThread::closeConnection()
{

}

/** Returns true if the data source is connected, false otherwise.*/
bool NeuropixThread::foundInputSource()
{
    return baseStationAvailable;
}


XmlElement NeuropixThread::getInfoXml()
{

	XmlElement neuropix_info("NEUROPIX-PXI");

	XmlElement* api_info = new XmlElement("API");

	if (api_v1.isActive)
		api_info->setAttribute("version", api_v1.info.version);
	else
		api_info->setAttribute("version", api_v3.info.version);

	neuropix_info.addChildElement(api_info);

	for (int i = 0; i < basestations.size(); i++)
	{
		XmlElement* basestation_info = new XmlElement("BASESTATION");
		basestation_info->setAttribute("index", i + 1);
		basestation_info->setAttribute("slot", int(basestations[i]->slot));
		basestation_info->setAttribute("firmware_version", basestations[i]->info.boot_version);
		basestation_info->setAttribute("bsc_firmware_version", basestations[i]->basestationConnectBoard->info.boot_version);
		basestation_info->setAttribute("bsc_part_number", basestations[i]->basestationConnectBoard->info.part_number);
		basestation_info->setAttribute("bsc_serial_number", String(basestations[i]->basestationConnectBoard->info.serial_number));

		for (auto probe : basestations[i]->getProbes())
		{
			XmlElement* probe_info = new XmlElement("PROBE");
			probe_info->setAttribute("port", probe->headstage->port);
			probe_info->setAttribute("port", probe->dock);
			probe_info->setAttribute("probe_serial_number", String(probe->info.serial_number));
			probe_info->setAttribute("hs_serial_number", String(probe->headstage->info.serial_number));
			probe_info->setAttribute("hs_part_number", probe->info.part_number);
			probe_info->setAttribute("hs_version", probe->headstage->info.version);
			probe_info->setAttribute("flex_part_number", probe->flex->info.part_number);
			probe_info->setAttribute("flex_version", probe->flex->info.version);

			basestation_info->addChildElement(probe_info);

		}

		neuropix_info.addChildElement(basestation_info);

	}

	return neuropix_info;

}


String NeuropixThread::getInfoString()
{

	String infoString;

	infoString += "API Version: ";
	if (api_v1.isActive)
		infoString += api_v1.info.version;
	else
		infoString += api_v3.info.version;
	infoString += "\n";
	infoString += "\n";
	infoString += "\n";

	for (int i = 0; i < basestations.size(); i++)
	{
		infoString += "Basestation ";
		infoString += String(i + 1);
		infoString += "\n";
		infoString += "  Firmware version: ";
		infoString += basestations[i]->info.boot_version;
		infoString += "\n";
		infoString += "  BSC firmware version: ";
		infoString += basestations[i]->basestationConnectBoard->info.boot_version;
		infoString += "\n";
		infoString += "  BSC part number: ";
		infoString += basestations[i]->basestationConnectBoard->info.part_number;
		infoString += "\n";
		infoString += "  BSC serial number: ";
		infoString += String(basestations[i]->basestationConnectBoard->info.serial_number);
		infoString += "\n";
		infoString += "\n";

		for (auto probe : basestations[i]->getProbes())
		{
			infoString += "    Port ";
			infoString += String(probe->headstage->port);
			infoString += "\n";
			infoString += "\n";
			infoString += "    Probe serial number: ";
			infoString += String(probe->info.serial_number);
			infoString += "\n";
			infoString += "\n";
			infoString += "    Headstage serial number: ";
			infoString += String(probe->headstage->info.serial_number);
			infoString += "\n";
			infoString += "    Headstage part number: ";
			infoString += probe->headstage->info.part_number;
			infoString += "\n";
			infoString += "    Headstage version: ";
			infoString += probe->headstage->info.version;
			infoString += "\n";
			infoString += "\n";
			infoString += "    Flex part number: ";
			infoString += probe->flex->info.part_number;
			infoString += "\n";
			infoString += "    Flex version: ";
			infoString += probe->flex->info.version;
			infoString += "\n";
			infoString += "\n";
			infoString += "\n";

		}
		infoString += "\n";
		infoString += "\n";
	}


	return infoString;

}

/** Initializes data transfer.*/
bool NeuropixThread::startAcquisition()
{

	initializationProgress = 0;

	progressBar->setVisible(true);

	startTimer(500); // wait for signal chain to be built
	
    return true;
}

void NeuropixThread::timerCallback()
{
	for (int i = 0; i < basestations.size(); i++)
	{
		basestations[i]->startAcquisition();
	}

	startThread();

    stopTimer();

	progressBar->setVisible(false);

}

void NeuropixThread::startRecording()
{
	recordingNumber++;

	File rootFolder = CoreServices::getDefaultRecordingDirectory();
	String pathName = rootFolder.getFileName();
	
	for (int i = 0; i < basestations.size(); i++)
	{
		if (basestations[i]->getProbeCount() > 0)
		{
			File savingDirectory = basestations[i]->getSavingDirectory();

			if (!savingDirectory.getFullPathName().isEmpty())
			{
				File fullPath = savingDirectory.getChildFile(pathName);

				if (!fullPath.exists())
				{
					fullPath.createDirectory();
				}

				File npxFileName = fullPath.getChildFile("recording_slot" + String(basestations[i]->slot) + "_" + String(recordingNumber) + ".npx2");

				if (api_v1.isActive)
				{
					np::setFileStream(basestations[i]->slot, npxFileName.getFullPathName().getCharPointer());
					np::enableFileStream(basestations[i]->slot, true);
				}
				else {
					Neuropixels::setFileStream(basestations[i]->slot, npxFileName.getFullPathName().getCharPointer());
					Neuropixels::enableFileStream(basestations[i]->slot, true);
				}
				

				LOGD("Basestation ", i, " started recording.");
			}
			
		}
	}

	

}

void NeuropixThread::setDirectoryForSlot(int slotIndex, File directory)
{

	setRecordMode(true);

	LOGD("Thread setting directory for slot ", slotIndex, " to ", directory.getFileName());

	if (slotIndex < basestations.size())
	{
		basestations[slotIndex]->setSavingDirectory(directory);
	}
}

File NeuropixThread::getDirectoryForSlot(int slotIndex)
{
	if (slotIndex < basestations.size())
	{
		return basestations[slotIndex]->getSavingDirectory();
	}
	else {
		return File::getCurrentWorkingDirectory();
	}
}

void NeuropixThread::stopRecording()
{
	for (int i = 0; i < basestations.size(); i++)
	{
		if (api_v1.isActive)
			np::enableFileStream(basestations[i]->slot_c, false);
		else
			Neuropixels::enableFileStream(basestations[i]->slot, false);
	}

	LOGD("NeuropixThread stopped recording.");
}

/** Stops data transfer.*/
bool NeuropixThread::stopAcquisition()
{

    if (isThreadRunning())
    {
        signalThreadShouldExit();
    }

	for (int i = 0; i < basestations.size(); i++)
	{
		basestations[i]->stopAcquisition();
	}

    return true;
}

void NeuropixThread::updateSettings(OwnedArray<ContinuousChannel>* continuousChannels,
	OwnedArray<EventChannel>* eventChannels,
	OwnedArray<SpikeChannel>* spikeChannels,
	OwnedArray<DataStream>* dataStreams,
	OwnedArray<DeviceInfo>* devices,
	OwnedArray<ConfigurationObject>* configurationObjects)
{

	if (sourceStreams.size() == 0) // initialize data streams
	{
		for (auto info : streamInfo)
		{
			DataStream::Settings settings
			{
				"name",
				"description",
				"identifier",

				info.sample_rate

			};

			sourceStreams.add(new DataStream(settings));
		}
	}

	for (int i = 0; i < sourceStreams.size(); i++)
	{
		DataStream* currentStream = sourceStreams[i];

		StreamInfo info = streamInfo[i];

		ContinuousChannel::Type type;

		if (info.type == stream_type::ADC)
			type = ContinuousChannel::Type::ADC;
		else
			type = ContinuousChannel::Type::ELECTRODE;

		for (int ch = 0; ch < info.num_channels; ch++)
		{
			float bitVolts;

			if (info.type == stream_type::ADC) {
				bitVolts = info.adc->getChannelGain(i);
			}
			else {
				bitVolts = 0.1950000f;
			}

			String name;

			if (info.type == stream_type::ADC) 
			{
				name = "ADC";
			}
			else if (info.type == stream_type::AP_BAND) {
				name = "AP";
			}
			else {
				name = "LFP";
			}

			if (info.sendSyncAsContinuousChannel && (ch == info.num_channels - 1))
			{
				name += "_SYNC";
				bitVolts = 1.0;
			}
			else {
				name += String(ch + 1);
			}

			ContinuousChannel::Settings settings{
				type,
				name,
				"description",
				"identifier",

				bitVolts,

				currentStream
			};

			int chIndex = info.probe->settings.selectedChannel.indexOf(i);

			int selectedBank = info.probe->settings.availableBanks.indexOf(info.probe->settings.selectedBank[chIndex]);

			int selectedElectrode = i + selectedBank * 384;
			int shank = info.probe->settings.selectedShank[chIndex];

			float depth = float(info.probe->electrodeMetadata[selectedElectrode].ypos)
				+ shank * 1000.0f
				+ float(i % 2)
				+ 0.0001f * i; // each channel must have a unique depth value

			continuousChannels->add(new ContinuousChannel(settings));
			continuousChannels->getLast()->position.y = depth;

		}

		EventChannel::Settings settings{
			EventChannel::Type::TTL,
			"name",
			"description",
			"identifier",
			currentStream,
			1
		};

		eventChannels->add(new EventChannel(settings));

		dataStreams->add(new DataStream(*currentStream)); // copy existing stream

	}

}


void NeuropixThread::sendSyncAsContinuousChannel(bool shouldSend)
{
	for (auto probe : getProbes())
	{
		LOGD("Setting sendSyncAsContinuousChannel to: ", shouldSend);
		probe->sendSyncAsContinuousChannel(shouldSend);
	}

	updateSubprocessors();

}

void NeuropixThread::setTriggerMode(bool trigger)
{
    //ConfigAccessErrorCode caec = neuropix.neuropix_triggerMode(trigger);
    
    internalTrigger = trigger;
}

void NeuropixThread::setRecordMode(bool record)
{
    recordToNpx = record;
}

void NeuropixThread::setAutoRestart(bool restart)
{
	autoRestart = restart;
}


void NeuropixThread::handleMessage(String msg)
{
	std::cout << "Neuropix-PXI received " << msg << std::endl;

	StringArray parts = StringArray::fromTokens(msg, " ", "");

	// NP <bs> <port> <probe> OPTO <wavelength> <site>
	// NP <bs> WAVEPLAYER <start/stop>
}

String NeuropixThread::handleConfigMessage(String msg)
{
	std::cout << "Neuropix-PXI received " << msg << std::endl;

	StringArray parts = StringArray::fromTokens(msg, " ", "");

	// NP <bs> <port> <probe> SELECT <electrode> <electrode> <electrode>
	// NP <bs> <port> <probe> GAIN <gainval>
	// NP <bs> <port> <probe> REFERENCE <refval>
	// NP <bs> <port> <probe> FILTER <filterval>

	return " ";

}

bool NeuropixThread::updateBuffer()
{

	if (recordToNpx)
	{

		bool shouldRecord = CoreServices::getRecordingStatus();

		if (!isRecording && shouldRecord)
		{
			isRecording = true;
			recordingTimer.startTimer(1000);
		}
		else if (isRecording && !shouldRecord)
		{
			isRecording = false;
			stopRecording();
		}

	}

    return true;
}


RecordingTimer::RecordingTimer(NeuropixThread* t_)
{
	thread = t_;
}

void RecordingTimer::timerCallback()
{
	thread->startRecording();
	stopTimer();
}
