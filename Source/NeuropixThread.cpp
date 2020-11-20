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
#include "Basestations/SimulatedBasestation.h"

#include <vector>

DataThread* NeuropixThread::createDataThread(SourceNode *sn)
{
	return new NeuropixThread(sn);
}

GenericEditor* NeuropixThread::createEditor(SourceNode* sn)
{
    return new NeuropixEditor(sn, this, true);
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

	uint32_t availableslotmask;

	std::vector<int> slotsToCheck;
	np::scanPXI(&availableslotmask);

	api_v1.isActive = false;
	api_v3.isActive = true;

	Neuropixels::scanBS();

	Neuropixels::basestationID list[16];
		
	int count = getDeviceList(&list[0], 16);

	std::cout << "Found " << count << " devices..." << std::endl;

	for (int i = 0; i < count; i++)
	{

		Neuropixels::NP_ErrorCode ec = getDeviceInfo(list[i].ID, &list[i]);

		int slotID;

		bool foundSlot = tryGetSlotID(&list[i], &slotID);

		if (foundSlot && list[i].platformid == Neuropixels::NPPlatform_PXI)
		{

			std::cout << "Got slot id: " << slotID << std::endl;

			Basestation* bs = new Basestation_v3(slotID); 

			//Only add a basestation if it has at least one probe connected
			if (bs->open() && bs->getProbeCount())
			{
				basestations.add(bs);
			}

		}
		else {
			CoreServices::sendStatusMessage("ONE Box not yet supported.");
		}
	}


	if (basestations.size() == 0) // no basestations with API version match
	{
		for (int slot = 0; slot < 32; slot++)
		{
			if ((availableslotmask >> slot) & 1)
			{

				Basestation* bs = new Basestation_v1(slot);

				if (bs->open()) // detects # of probes; returns true if API version matches
				{
					std::cout << "Setting active API to v1" << std::endl;
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


	std::cout << "Num basestations: " << basestations.size() << std::endl;

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

	subprocessorInfo.clear();
	sourceBuffers.clear();

	for (auto probe : getProbes() )
	{

		std::cout << "PROBE " << probe->headstage->port << std::endl;

		SubprocessorInfo spInfo;
		spInfo.num_channels = probe->sendSync ? probe->channel_count + 1 : probe->channel_count;
		spInfo.sample_rate = probe->ap_sample_rate;
		spInfo.type = AP_BAND;
		spInfo.sendSyncAsContinuousChannel = probe->sendSync;

		subprocessorInfo.add(spInfo);

		sourceBuffers.add(new DataBuffer(spInfo.num_channels, 10000));  // AP band buffer
		probe->apBuffer = sourceBuffers.getLast();

		std::cout << "spinfo: " << spInfo.num_channels << " ch, " << spInfo.sample_rate << " Hz" << std::endl;

		if (probe->generatesLfpData())
		{
			sourceBuffers.add(new DataBuffer(spInfo.num_channels, 10000));  // LFP band buffer
			probe->lfpBuffer = sourceBuffers.getLast();

			SubprocessorInfo spInfo;
			spInfo.num_channels = probe->channel_count;
			spInfo.sample_rate = probe->lfp_sample_rate;
			spInfo.type = LFP_BAND;

			std::cout << "spinfo: " << spInfo.num_channels << " ch, " << spInfo.sample_rate << " Hz" << std::endl;

			subprocessorInfo.add(spInfo);
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
		settings.probe->setStatus(ProbeStatus::UPDATING);
	}

	for (auto settings: probeSettingsUpdateQueue)
	{

		if (settings.probe != nullptr)
		{
			settings.probe->selectElectrodes(settings, false);
			settings.probe->setAllGains(settings.apGainIndex, settings.lfpGainIndex, false);
			settings.probe->setAllReferences(settings.referenceIndex, false);
			settings.probe->setApFilterState(settings.apFilterState, false);
			settings.probe->writeConfiguration();

			settings.probe->setStatus(ProbeStatus::CONNECTED);
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

	File rootFolder = CoreServices::RecordNode::getRecordingPath();
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
				

				std::cout << "Basestation " << i << " started recording." << std::endl;
			}
			
		}
	}

	

}

void NeuropixThread::setDirectoryForSlot(int slotIndex, File directory)
{

	setRecordMode(true);

	std::cout << "Thread setting directory for slot " << slotIndex << " to " << directory.getFileName() << std::endl;

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

	std::cout << "NeuropixThread stopped recording." << std::endl;
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

/*
void NeuropixThread::setSelectedProbe(unsigned char slot, signed char port)
{
	int currentSlot, currentPort;
	int newSlot, newPort;

	for (int i = 0; i < basestations.size(); i++)
	{
		if (basestations[i]->slot == slot)
		{
			for (int probe_num = 0; probe_num < basestations[i]->getProbeCount(); probe_num++)
			{
				if (basestations[i]->probes[probe_num]->port == port)
				{
					newSlot = i;
					newPort = probe_num;
					basestations[i]->probes[probe_num]->setSelected(true);
				}
			}
		}
	}

	for (int i = 0; i < basestations.size(); i++)
	{
		if (basestations[i]->slot == selectedSlot)
		{
			for (int probe_num = 0; probe_num < basestations[i]->getProbeCount(); probe_num++)
			{
				if (basestations[i]->probes[probe_num]->port == selectedPort)
				{
					currentSlot = i;
					currentPort = probe_num;
					basestations[i]->probes[probe_num]->setSelected(false);
				}
			}
		}
	}

	//basestations[newSlot]->probes[newPort]->ap_timestamp = basestations[currentSlot]->probes[currentPort]->ap_timestamp;
	//basestations[newSlot]->probes[newPort]->lfp_timestamp = basestations[currentSlot]->probes[currentPort]->lfp_timestamp;

	selectedSlot = slot;
	selectedPort = port;
}*/

/*ProbeStatus NeuropixThread::getProbeStatus(unsigned char slot, signed char port)
{
	for (int i = 0; i < basestations.size(); i++)
	{
		if (basestations[i]->slot == slot)
		{
			for (int probe_num = 0; probe_num < basestations[i]->getProbeCount(); probe_num++)
			{
				if (basestations[i]->probes[probe_num]->port == port)
				{
					return basestations[i]->probes[probe_num]->status;
				}
			}
		}

	}
	return ProbeStatus::DISCONNECTED;
}

bool NeuropixThread::isSelectedProbe(unsigned char slot, signed char port)
{
	for (int i = 0; i < basestations.size(); i++)
	{
		if (basestations[i]->slot == slot)
		{
			for (int probe_num = 0; probe_num < basestations[i]->getProbeCount(); probe_num++)
			{
				if (basestations[i]->probes[probe_num]->port == port)
				{
					return basestations[i]->probes[probe_num]->isSelected;
				}
			}
		}

	}
	return false;
}*/

void NeuropixThread::setDefaultChannelNames()
{

	if (subprocessorInfo.size() > 0)
	{
		
		int chan = 0;

		for (auto spInfo : subprocessorInfo)
		{
			for (int i = 0; i < spInfo.num_channels; i++)
			{
				ChannelCustomInfo info;

				if (spInfo.type == AP_BAND)
					info.name = "AP";
				else
					info.name = "LFP";


				if (spInfo.sendSyncAsContinuousChannel && (i == spInfo.num_channels - 1))
				{
					info.name += "_SYNC";
					info.gain = 1.0;
				}
				else {
					info.name += String(i + 1);
					info.gain = 0.1950000f;
				}

				channelInfo.set(chan, info);
				chan++;
			}

		}

	}

}

bool NeuropixThread::usesCustomNames() const
{
	return true;
}

void NeuropixThread::sendSyncAsContinuousChannel(bool shouldSend)
{
	for (auto probe : getProbes())
	{
		probe->sendSyncAsContinuousChannel(shouldSend);
	}

	updateSubprocessors();

}


/** Returns the number of virtual subprocessors this source can generate */
unsigned int NeuropixThread::getNumSubProcessors() const
{
	return subprocessorInfo.size();
}

/** Returns the number of continuous headstage channels the data source can provide.*/
int NeuropixThread::getNumDataOutputs(DataChannel::DataChannelTypes type, int subProcessorIdx) const
{

	int numChans;

	if (type == DataChannel::DataChannelTypes::HEADSTAGE_CHANNEL)
		return subprocessorInfo[subProcessorIdx].num_channels;
	else
		numChans = 0;

	//std::cout << "Num chans for subprocessor " << subProcessorIdx << " = " << numChans << std::endl;
	
	return numChans;
}

/** Returns the number of TTL channels that each subprocessor generates*/
int NeuropixThread::getNumTTLOutputs(int subProcessorIdx) const 
{
	return 1;
}

/** Returns the sample rate of the data source.*/
float NeuropixThread::getSampleRate(int subProcessorIdx) const
{

	return subprocessorInfo[subProcessorIdx].sample_rate;

}

/** Returns the volts per bit of the data source.*/
float NeuropixThread::getBitVolts(const DataChannel* chan) const
{
	//std::cout << "BIT VOLTS == 0.195" << std::endl;
	return 0.1950000f;
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
