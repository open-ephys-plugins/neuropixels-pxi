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

#include "UI/NeuropixInterface.h"

#include <vector>

//Helpful for debugging when PXI system is connected but don't want to connect to real probes
#define FORCE_SIMULATION_MODE false

DataThread* NeuropixThread::createDataThread(SourceNode *sn, DeviceType type)
{
	return new NeuropixThread(sn, type);
}

std::unique_ptr<GenericEditor> NeuropixThread::createEditor(SourceNode* sn)
{
	std::unique_ptr<NeuropixEditor> ed = std::make_unique<NeuropixEditor>(sn, this);

	editor = ed.get();

	return ed;
}

void Initializer::run()
{

	Neuropixels::scanBS();
	Neuropixels::basestationID list[16];
	int count = getDeviceList(&list[0], 16);

	LOGC("  Found ", count, " device", count == 1 ? "." : "s.");

	Array<int> slotIDs;

	if (!FORCE_SIMULATION_MODE)
	{

		for (int i = 0; i < count; i++)
		{

			int slotID;

			bool foundSlot = Neuropixels::tryGetSlotID(&list[i], &slotID);

			Neuropixels::NP_ErrorCode ec = Neuropixels::getDeviceInfo(list[i].ID, &list[i]);

			LOGD("Slot ID: ", slotID, "Platform ID : ", list[i].platformid);

			if (foundSlot && list[i].platformid == Neuropixels::NPPlatform_PXI && type == PXI)
			{

				LOGC("  Opening device on slot ", slotID);

				Basestation* bs = new Basestation_v3(neuropixThread, slotID);

				if (bs->open()) //returns true if Basestation firmware >= 2.0
				{

					int insertionIndex = 0;

					if (slotIDs.size() > 0)
					{

						insertionIndex = slotIDs.size();

						LOGC("  Checking ", insertionIndex, ": ", slotIDs[insertionIndex - 1]);

						while (insertionIndex > 0 && slotIDs[insertionIndex - 1] > slotID)
						{
							LOGC("Moving backward...");
							insertionIndex--;
							LOGC("  Checking ", insertionIndex, ": ", slotIDs[insertionIndex - 1]);
						}
					}

					LOGC("Insertion index:", insertionIndex);

					basestations.insert(insertionIndex, bs);
					slotIDs.insert(insertionIndex, slotID);

					LOGC("  Adding basestation");

				}
				else
				{
					LOGC("  Could not open basestation");
					delete bs;
				}

			}
			else if (list[i].platformid == Neuropixels::NPPlatform_USB && type == ONEBOX) {

				Basestation* bs = new OneBox(neuropixThread, list[i].ID);

				if (bs->open())
				{
						
					basestations.add(bs);

					if (!bs->getProbeCount())
						CoreServices::sendStatusMessage("OneBox found, no probes connected.");
				}
				else {
					delete bs;
				}
			}
			else {
				LOGC("   Slot ", slotID, " did not match desired platform.");
			}
		}


		if (basestations.size() == 0 && type == PXI) // no basestations with API version match
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

					Basestation* bs = new Basestation_v1(neuropixThread, slot);

					if (bs->open()) // detects # of probes; returns true if API version matches
					{
						api_v1.isActive = true;
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

	}

	if (basestations.size() == 0) // no basestations at all
	{


		bool response = true;

		if (!FORCE_SIMULATION_MODE)
		{

			if (type == PXI)
			{
				response = AlertWindow::showOkCancelBox(AlertWindow::NoIcon,
					"No basestations detected",
					"No Neuropixels PXI basestations were detected. Do you want to run this plugin in simulation mode?",
					"Yes", "No", 0, 0);
			}
			else if (type == ONEBOX) 
			{
				response = AlertWindow::showOkCancelBox(AlertWindow::NoIcon,
					"No OneBox detected",
					"No OneBox was detected. Do you want to run this plugin in simulation mode?",
					"Yes", "No", 0, 0);
			}
			
		}

		if (response)
		{
			if (type == PXI)
			{
				basestations.add(new SimulatedBasestation(neuropixThread, type, 2));
				basestations.getLast()->open(); // detects # of probes
			}
			else if (type == ONEBOX)
			{
				basestations.add(new SimulatedBasestation(neuropixThread, type, 16));
				basestations.getLast()->open(); // detects # of probes
			}
		}

	}

}

NeuropixThread::NeuropixThread(SourceNode* sn, DeviceType type_) :
	DataThread(sn),
	type(type_),
	baseStationAvailable(false),
	probesInitialized(false),
	initializationComplete(false)
{

	defaultSyncFrequencies.add(1);
	defaultSyncFrequencies.add(10);

	api_v1.isActive = false;
	api_v3.isActive = true;

	LOGC("Scanning for devices...");

	LOGD("Setting debug level to 0");
	Neuropixels::np_dbg_setlevel(0);

	initializer = std::make_unique<Initializer>(this, basestations, type, api_v1, api_v3);

	initializer->run();

	bool foundSync = false;

	int probeIndex = 0;
	int streamIndex = 0;

	for (auto probe : getProbes())
	{
		baseStationAvailable = true;

		if (!foundSync)
		{
			probe->basestation->setSyncAsInput();
			foundSync = true;
		}

		/* Generate names for probes based on order of appearance in chassis */
		probe->customName.automatic = generateProbeName(probeIndex, ProbeNameConfig::AUTO_NAMING);
		probe->displayName = probe->customName.automatic;
		probe->streamIndex = streamIndex;
		probe->customName.streamSpecific = generateProbeName(probeIndex, ProbeNameConfig::STREAM_INDICES);

		if (probe->generatesLfpData())
			streamIndex += 2;
		else
			streamIndex += 1;

		probeIndex++;

	}

	updateStreamInfo();

}

String NeuropixThread::generateProbeName(int probeIndex, ProbeNameConfig::NamingScheme namingScheme)
{
	StringArray probeNames = { "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N",
						   "O" , "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z" };

	Array<Probe*> probes = getProbes();

	Probe* probe = probes[probeIndex];

	String name;

	switch (namingScheme)
	{
	case ProbeNameConfig::AUTO_NAMING:
		name = "Probe" + probeNames[probeIndex];
		break;
	case ProbeNameConfig::PROBE_SPECIFIC_NAMING:
		name = probe->customName.probeSpecific;
		break;
	case ProbeNameConfig::PORT_SPECIFIC_NAMING:
		name = probe->basestation->getCustomPortName(probe->headstage->port, probe->dock);
		break;
	case ProbeNameConfig::STREAM_INDICES:
		name = String(probe->streamIndex);
		if (probe->generatesLfpData())
			name += "," + String(probe->streamIndex + 1);
		break;
	default:
		name = "Probe" + probeNames[probeIndex];
	}

	return name;
	
}

void NeuropixThread::updateStreamInfo()
{

	streamInfo.clear();
	sourceBuffers.clear();

	int probe_index = 0;

	for (auto source : getDataSources() )
	{

		if (source->sourceType == DataSourceType::PROBE)
		{
			StreamInfo apInfo;

			Probe* probe = (Probe*) source;

			apInfo.num_channels = probe->sendSync ? probe->channel_count + 1 : probe->channel_count;
			apInfo.sample_rate = probe->ap_sample_rate;
			apInfo.probe = probe;
			apInfo.probe_index = probe_index++;

			if (probe->generatesLfpData())
				apInfo.type = AP_BAND;
			else
				apInfo.type = BROAD_BAND;

			apInfo.sendSyncAsContinuousChannel = probe->sendSync;

			streamInfo.add(apInfo);

			sourceBuffers.add(new DataBuffer(apInfo.num_channels, 460800));  // AP band buffer
			probe->apBuffer = sourceBuffers.getLast();

			if (probe->generatesLfpData())
			{

				StreamInfo lfpInfo;
				lfpInfo.num_channels = probe->sendSync ? probe->channel_count + 1 : probe->channel_count;
				lfpInfo.sample_rate = probe->lfp_sample_rate;
				lfpInfo.type = LFP_BAND;
				lfpInfo.probe = probe;
				lfpInfo.sendSyncAsContinuousChannel = probe->sendSync;

				sourceBuffers.add(new DataBuffer(lfpInfo.num_channels, 38400));  // LFP band buffer
				probe->lfpBuffer = sourceBuffers.getLast();

				streamInfo.add(lfpInfo);
			}

			LOGD("Probe (slot=", probe->basestation->slot, ", port=", probe->headstage->port, ") CH=", apInfo.num_channels, " SR=", apInfo.sample_rate, " Hz");
		}
		else {

			if (true)
			{
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

		if (settings.probe->basestation->isBusy())
			settings.probe->basestation->waitForThreadToExit();

		LOGC("Applying probe settings for ", settings.probe->name);

		if (settings.probe != nullptr)
		{

			settings.probe->selectElectrodes();
			settings.probe->setAllGains();
			settings.probe->setAllReferences();
			settings.probe->setApFilterState();

			settings.probe->calibrate();

			settings.probe->writeConfiguration();

			settings.probe->setStatus(SourceStatus::CONNECTED);

			LOGC("Wrote configuration");
		}
	}

	probeSettingsUpdateQueue.clear();
}

void NeuropixThread::initialize(bool signalChainIsLoading)
{
	editor->initialize(signalChainIsLoading);

}

void NeuropixThread::initializeBasestations(bool signalChainIsLoading)
{
	// slower task, run in background thread

	LOGD("NeuropixThread::initializeBasestations");

	for (auto basestation : basestations)
	{
		basestation->initialize(signalChainIsLoading); // prepares probes for acquisition; may be slow
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

Array<OneBox*> NeuropixThread::getOneBoxes()
{
	Array<OneBox*> bs;

	for (auto bs_ : basestations)
	{
		if (bs_->type == BasestationType::ONEBOX)
			bs.add((OneBox*)bs_);
	}

	return bs;
}

Array<Basestation_v3*> NeuropixThread::getOptoBasestations()
{
	Array<Basestation_v3*> bs;

	for (auto bs_ : basestations)
	{
		if (bs_->type == BasestationType::OPTO)
			bs.add((Basestation_v3*) bs_);
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

String NeuropixThread::getProbeInfoString()
{
	DynamicObject output;

	output.setProperty(Identifier("plugin"),
		var("Neuropix-PXI"));
	output.setProperty(Identifier("version"),
		var(PLUGIN_VERSION));

	Array<var> probes;

	for (auto probe : getProbes())
	{
		DynamicObject::Ptr p = new DynamicObject();

		p->setProperty(Identifier("name"), probe->displayName);
		p->setProperty(Identifier("type"), probe->probeMetadata.name);
		p->setProperty(Identifier("slot"), probe->basestation->slot);
		p->setProperty(Identifier("port"), probe->headstage->port);
		p->setProperty(Identifier("dock"), probe->dock);
		
		p->setProperty(Identifier("part_number"), probe->info.part_number);
		p->setProperty(Identifier("serial_number"), String(probe->info.serial_number));

		p->setProperty(Identifier("is_calibrated"), probe->isCalibrated);

		probes.add(p.get());
	}

	output.setProperty(Identifier("probes"), probes);

	MemoryOutputStream f;
	output.writeAsJSON(f, 0, true, 4);

	return f.toString();

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
	if (foundInputSource() && slotIndex > -1)
		basestations[slotIndex]->setSyncAsInput();
}

void NeuropixThread::setSyncOutput(int slotIndex)
{
	if (basestations.size() && slotIndex > -1)
		basestations[slotIndex]->setSyncAsOutput(0);
}

Array<int> NeuropixThread::getSyncFrequencies()
{
	if (foundInputSource())
		return basestations[0]->getSyncFrequencies();
	else
		return defaultSyncFrequencies;
		
}

void NeuropixThread::setSyncFrequency(int slotIndex, int freqIndex)
{
	if (foundInputSource() && slotIndex > -1)
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
	startTimer(100);
	
    return true;
}


void NeuropixThread::timerCallback()
{
	LOGD("Timer callback.");

	if (editor->uiLoader->isThreadRunning())
	{
		LOGD("Waiting for Neuropixels settings thread to exit.");
		editor->uiLoader->waitForThreadToExit(20000);
		LOGD("Neuropixels settings thread finished.");
	}
		

	for (int i = 0; i < basestations.size(); i++)
	{
		basestations[i]->startAcquisition();
	}

	startThread();

    stopTimer();

}


void NeuropixThread::setDirectoryForSlot(int slotIndex, File directory)
{

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

void NeuropixThread::setNamingSchemeForSlot(int slot, ProbeNameConfig::NamingScheme namingScheme)
{
	for (auto bs : getBasestations())
		if (bs->slot == slot)
			bs->setNamingScheme(namingScheme);
}

ProbeNameConfig::NamingScheme NeuropixThread::getNamingSchemeForSlot(int slot)
{
	for (auto bs : getBasestations())
		if (bs->slot == slot)
			return bs->getNamingScheme();
}


/** Stops data transfer.*/
bool NeuropixThread::stopAcquisition()
{

	LOGC("Stopping Neuropixels thread.");

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

		String lastName;

		for (auto info : streamInfo)
		{
			String streamName, description, identifier;

			if (info.type == stream_type::ADC)
			{
				streamName = "OneBox-ADC";
				description = "OneBox ADC data stream";
				identifier = "onebox.adc";
			}
				
			else if (info.type == stream_type::AP_BAND)
			{
				lastName = generateProbeName(info.probe_index, info.probe->namingScheme);
				
				if (info.probe->namingScheme != ProbeNameConfig::STREAM_INDICES)
					streamName = lastName + "-AP";
				else
					streamName = String(info.probe->streamIndex);

				description = "Neuropixels AP band data stream";
				identifier = "neuropixels.data.ap";
			}

			else if (info.type == stream_type::BROAD_BAND)
			{
				streamName = generateProbeName(info.probe_index, info.probe->namingScheme);
				description = "Neuropixels data stream";
				identifier = "neuropixels.data";
			}
			
			else if (info.type == stream_type::LFP_BAND)
			{
				if (info.probe->namingScheme != ProbeNameConfig::STREAM_INDICES)
					streamName = lastName + "-LFP";
				else
					streamName = String(info.probe->streamIndex + 1);

				description = "Neuropixels LFP band data stream";
				identifier = "neuropixels.data.lfp";
			}

			DataStream::Settings settings
			{
				streamName,
				"description",
				"identifier",

				info.sample_rate

			};

			sourceStreams.add(new DataStream(settings));
		}
	}

	dataStreams->clear();
	eventChannels->clear();
	continuousChannels->clear();
	spikeChannels->clear();
	devices->clear();
	configurationObjects->clear();

	int probeIdx = 0;

	for (int i = 0; i < sourceStreams.size(); i++)
	{

		DataStream* currentStream = sourceStreams[i];

		String streamName;

		StreamInfo info = streamInfo[i];

		if (info.type == stream_type::AP_BAND)
		{
			Probe* p = getProbes()[probeIdx];
			p->updateNamingScheme(p->namingScheme); // update displayName

			streamName = generateProbeName(probeIdx, p->namingScheme);

			if (p->namingScheme != ProbeNameConfig::STREAM_INDICES)
				streamName += "-AP";
			else
				streamName = String(p->streamIndex);
		}
		else if (info.type == stream_type::LFP_BAND)
		{
			Probe* p = getProbes()[probeIdx];
			p->updateNamingScheme(p->namingScheme); // update displayName

			streamName = generateProbeName(probeIdx, p->namingScheme);

			if (p->namingScheme != ProbeNameConfig::STREAM_INDICES)
				streamName += "-LFP";
			else
				streamName = String(p->streamIndex + 1);

			probeIdx++;

		}
		else if (info.type == stream_type::BROAD_BAND)
		{
			Probe* p = getProbes()[probeIdx];
			p->updateNamingScheme(p->namingScheme); // update displayName

			streamName = generateProbeName(probeIdx, p->namingScheme);

			probeIdx++;
		}
		else {
			streamName = currentStream->getName();
		}

		currentStream->setName(streamName);

		ContinuousChannel::Type type;

		String description, identifier;

		if (info.type == stream_type::ADC)
		{
			type = ContinuousChannel::Type::ADC;
			description = "OneBox ADC channel";
			identifier = "neuropixels.adc";
		}
		else
		{
			type = ContinuousChannel::Type::ELECTRODE;
			description = "Neuropixels electrode";
			identifier = "neuropixels.electrode";
		}

		currentStream->clearChannels();

		for (int ch = 0; ch < info.num_channels; ch++)
		{

			float bitVolts;

			if (info.type == stream_type::ADC) {
				bitVolts = info.adc->getChannelGain(ch);
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
			else if (info.type == stream_type::LFP_BAND) {
				name = "LFP";
			}
			else
			{
				name = "CH";
			}

			float depth = -1.0f;

			if (info.sendSyncAsContinuousChannel && (ch == info.num_channels - 1))
			{
				type = ContinuousChannel::Type::ADC;
				name += "_SYNC";
				bitVolts = 1.0;
				description = "Neuropixels sync line (continuously sampled)";
				identifier = "neuropixels.sync";
			}
			else {
				name += String(ch + 1);
			}

			ContinuousChannel::Settings settings{
				type,
				name,
				description,
				identifier,

				bitVolts,

				currentStream
			};

			continuousChannels->add(new ContinuousChannel(settings));

			if (type == ContinuousChannel::Type::ELECTRODE)
			{
				int chIndex = info.probe->settings.selectedChannel.indexOf(ch);

				Array<Bank> availableBanks = info.probe->settings.availableBanks;

				int selectedBank = availableBanks.indexOf(info.probe->settings.selectedBank[chIndex]);

				int selectedElectrode = info.probe->settings.selectedElectrode[chIndex];
				int shank = info.probe->settings.selectedShank[chIndex];

				float depth = float(info.probe->electrodeMetadata[selectedElectrode].ypos)
					+ shank * 10000.0f
					+ float(ch % 2)
					+ 0.0001f * ch; // each channel must have a unique depth value

				continuousChannels->getLast()->position.y = depth;

				MetadataDescriptor descriptor(MetadataDescriptor::MetadataType::UINT16,
					1, "electrode_index",
					"Electrode index for this channel", "neuropixels.electrode_index");

				MetadataValue value(MetadataDescriptor::MetadataType::UINT16, 1);
				value.setValue((uint16) info.probe->settings.selectedElectrode[chIndex]);

				//LOGD("Setting channel ", ch, " electrode_index metadata value to ", (uint16)info.probe->settings.selectedElectrode[chIndex]);

				continuousChannels->getLast()->addMetadata(descriptor, value);
			}
			
		} // end channel loop

		EventChannel::Settings settings{
			EventChannel::Type::TTL,
			"Neuropixels PXI Sync",
			"Status of SMA sync line on PXI card",
			"neuropixels.sync",
			currentStream,
			1
		};

		eventChannels->add(new EventChannel(settings));

		dataStreams->add(new DataStream(*currentStream)); // copy existing stream

		if (info.probe != nullptr)
		{
			DeviceInfo::Settings deviceSettings{
			info.probe->name,
			"Neuropixels probe",
			info.probe->info.part_number,

			String(info.probe->info.serial_number),
			"imec"
			};

			DeviceInfo* device = new DeviceInfo(deviceSettings);

			MetadataDescriptor descriptor(MetadataDescriptor::MetadataType::UINT16,
				1, "num_adcs",
				"Number of analog-to-digital converter for this probe", "neuropixels.adcs");

			MetadataValue value(MetadataDescriptor::MetadataType::UINT16, 1);
			value.setValue((uint16)info.probe->probeMetadata.num_adcs);

			device->addMetadata(descriptor, value);

			devices->add(device); // unique device object owned by SourceNode

			dataStreams->getLast()->device = device; // DataStream object just gets a pointer
		}
		
	} // end source stream loop

	editor->update();

}


void NeuropixThread::sendSyncAsContinuousChannel(bool shouldSend)
{
	for (auto probe : getProbes())
	{
		LOGD("Setting sendSyncAsContinuousChannel to: ", shouldSend);
		probe->sendSyncAsContinuousChannel(shouldSend);
	}

	updateStreamInfo();

}

void NeuropixThread::setTriggerMode(bool trigger)
{
    internalTrigger = trigger;
}


void NeuropixThread::setAutoRestart(bool restart)
{
	autoRestart = restart;
}


void NeuropixThread::handleBroadcastMessage(String msg)
{
	// Available commands:
	//
	// NP OPTO <bs> <port> <probe> <wavelength> <site>
	// NP WAVEPLAYER <bs> <"start"/"stop">
	//

	LOGD("Neuropix-PXI received ", msg);

	StringArray parts = StringArray::fromTokens(msg, " ", "");

	if (parts[0].equalsIgnoreCase("NP"))
	{

		LOGD("Found NP command: ", msg);

		if (parts.size() > 0)
		{

			String command = parts[1];

			if (command.equalsIgnoreCase("OPTO"))
			{
				if (parts.size() == 7)
				{
					int slot = parts[2].getIntValue();
					int port = parts[3].getIntValue();
					int dock = parts[4].getIntValue();
					String wavelength = parts[5];
					int emitter = parts[6].getIntValue();

					if (emitter < 0 || emitter > 14)
					{
						LOGD("Invalid site number, must be between 0 and 14, got ", emitter);
						return;
					}
						

					for (auto bs : getOptoBasestations())
					{
						if (bs->slot == slot)
						{
							for (auto probe : getProbes())
							{

								if (probe->basestation->slot == slot &&
									probe->headstage->port == port &&
									probe->dock == dock)
								{
									probe->ui->setEmissionSite(wavelength, emitter);
								}

							}
						}
							

					}
				}
				else {
					LOGD("Incorrect number of argument for OPTO command. Found ", parts.size(), ", requires 7.");
				}
			}
			else if (command.equalsIgnoreCase("OPTO"))

			{
				if (parts.size() == 4)
				{
					int slot = parts[2].getIntValue();
					bool shouldStart = parts[3].equalsIgnoreCase("start");

					for (auto bs : getOneBoxes())
					{
						if (bs->slot == slot)
							bs->triggerWaveplayer(shouldStart);

					}
				}
				else {
					LOGD("Incorrect number of argument for WAVEPLAYER message. Found ", parts.size(), ", requires 4.");
				}
			}
		}
		
	}


}

String NeuropixThread::handleConfigMessage(String msg)
{
	// Available commands:
	// NP SELECT <bs> <port> <dock> <electrode> <electrode> <electrode> ...
	// NP SELECT "<preset>"
	// NP GAIN <bs> <port> <dock> <AP/LFP> <gainval>
	// NP REFERENCE <bs> <port> <dock> <EXT/TIP>
	// NP FILTER <bs> <port> <dock> <ON/OFF>
	// NP INFO

	LOGD("Neuropix-PXI received ", msg);

	if (CoreServices::getAcquisitionStatus())
	{
		return "Neuropixels plugin cannot update settings while acquisition is active.";
	}

	StringArray parts = StringArray::fromTokens(msg, " ", "");

	if (parts[0].equalsIgnoreCase("NP"))
	{

		LOGD("Found NP command.");

		if (parts.size() > 0)
		{

			String command = parts[1];

			LOGD("Command: ", command);

			if (command.equalsIgnoreCase("SELECT") ||
				command.equalsIgnoreCase("GAIN") || 
				command.equalsIgnoreCase("REFERENCE") || 
				command.equalsIgnoreCase("FILTER"))
			{
				if (parts.size() > 5)
				{
					int slot = parts[2].getIntValue();
					int port = parts[3].getIntValue();
					int dock = parts[4].getIntValue();

					LOGD("Slot: ", slot, ", Port: ", port, ", Dock: ", dock);

					for (auto probe : getProbes())
					{

						if (probe->basestation->slot == slot &&
							probe->headstage->port == port &&
							probe->dock == dock)
						{
							if (command.equalsIgnoreCase("GAIN"))
							{
								bool isApBand = parts[5].equalsIgnoreCase("AP");
								float gain = parts[6].getFloatValue();

								if (isApBand)
								{
									if (probe->settings.availableApGains.size() > 0)
									{
										int gainIndex = probe->settings.availableApGains.indexOf(gain);

										if (gainIndex > -1)
										{
											probe->ui->setApGain(gainIndex);
										}
									}
								}
								else {
									if (probe->settings.availableLfpGains.size() > 0)
									{
										int gainIndex = probe->settings.availableLfpGains.indexOf(gain);

										if (gainIndex > -1)
										{
											probe->ui->setLfpGain(gainIndex);
										}
									}
								}
							}
							else if (command.equalsIgnoreCase("REFERENCE"))
							{

								int referenceIndex;

								if (parts[5].equalsIgnoreCase("EXT"))
								{
									referenceIndex = 0;
								}
								else if (parts[5].equalsIgnoreCase("TIP"))
								{
									referenceIndex = 1;
								}

								probe->ui->setReference(referenceIndex);
							}
							else if (command.equalsIgnoreCase("FILTER"))
							{
								if (probe->hasApFilterSwitch())
								{
									probe->ui->setApFilterState(parts[5].equalsIgnoreCase("ON"));
								}
							}
							else if (command.equalsIgnoreCase("SELECT"))
							{
								Array<int> electrodes;

								if (parts[5].substring(0, 1) == "\"")
								{
									String presetName = msg.fromFirstOccurrenceOf("\"", false, false).upToFirstOccurrenceOf("\"", false, false);

									LOGD("Selecting preset: ", presetName);

									electrodes = probe->selectElectrodeConfiguration(presetName);
									
									probe->ui->selectElectrodes(electrodes);
								}
								else {

									LOGD("Selecting electrodes: ")

									for (int i = 5; i < parts.size(); i++)
									{
										int electrode = parts[i].getIntValue();

										//std::cout << electrode << std::endl;

										if (electrode > 0 && electrode < probe->electrodeMetadata.size() + 1)
											electrodes.add(electrode - 1);
									}

									probe->ui->selectElectrodes(electrodes);
								}
							}
						}
					}

					return "SUCCESS";
				}
				else {
					return "Incorrect number of argument for " + command + ". Found " + String(parts.size()) + ", requires 6.";
				}
			}
			else if (command.equalsIgnoreCase("INFO"))
			{
				return getProbeInfoString();
			}
			else
			{
				return "NP command " + command + " not recognized.";
			}
		}
	}

	return "Command not recognized.";

}

String NeuropixThread::getCustomProbeName(String serialNumber)
{
	if (customProbeNames.count(serialNumber) > 0)
	{
		return customProbeNames[serialNumber];
	}
	else {
		return "";
	}
}


void NeuropixThread::setCustomProbeName(String serialNumber, String customName)
{
	customProbeNames[serialNumber] = customName;
}

bool NeuropixThread::updateBuffer()
{

	Sleep(500);

    return true;
}
