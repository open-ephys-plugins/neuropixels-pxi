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
	recordToNpx(false)
{
	progressBar = new ProgressBar(initializationProgress);

	api.getInfo();

    for (int i = 0; i < 384; i++)
    {
        channelMap.add(i);
        outputOn.add(true);
    }

	for (int i = 0; i < 16; i++)
		fillPercentage.add(0.0);

    refs.add(0);
    refs.add(1);
    refs.add(192);
    refs.add(576);
    refs.add(960);

    counter = 0;

    maxCounter = 0;

    uint32_t availableslotmask;
	totalProbes = 0;

	np::scanPXI(&availableslotmask);

	for (int slot = 0; slot < 32; slot++)
	{
		if ((availableslotmask >> slot) & 1)
		{
			basestations.add(new Basestation(slot));
		}
	}

}

NeuropixThread::~NeuropixThread()
{
    closeConnection();
}

void NeuropixThread::updateProbeSettingsQueue()
{
	probeSettingsUpdateQueue.add(this->p_settings);
}

void NeuropixThread::applyProbeSettingsQueue()
{
	for (auto settings : probeSettingsUpdateQueue)
	{
		selectElectrodes(settings.slot, settings.port, settings.channelStatus);
		setAllGains(settings.slot, settings.port, settings.apGainIndex, settings.lfpGainIndex);
		setAllReferences(settings.slot, settings.port, settings.refChannelIndex);
		setFilter(settings.slot, settings.port, settings.disableHighPass);
	}
}

void NeuropixThread::openConnection()
{

	bool foundSync = false;

	for (int i = 0; i < basestations.size(); i++)
	{

		basestations[i]->init();

		if (basestations[i]->getProbeCount() > 0)
		{
			totalProbes += basestations[i]->getProbeCount();

			baseStationAvailable = true;

			if (!foundSync)
			{
				basestations[i]->setSyncAsInput();
				selectedSlot = basestations[i]->slot;
				selectedPort = basestations[i]->probes[0]->port;
				foundSync = true;
			}

			for (int probe_num = 0; probe_num < basestations[i]->getProbeCount(); probe_num++)
			{
				std::cout << "Creating buffers for slot " << int(basestations[i]->slot) << ", probe " << int(basestations[i]->probes[probe_num]->port) << std::endl;
				sourceBuffers.add(new DataBuffer(384, 10000));  // AP band buffer

				basestations[i]->probes[probe_num]->apBuffer = sourceBuffers.getLast();

				sourceBuffers.add(new DataBuffer(384, 10000));  // LFP band buffer

				basestations[i]->probes[probe_num]->lfpBuffer = sourceBuffers.getLast();

				CoreServices::sendStatusMessage("Initializing probe " + String(probe_num + 1) + "/" + String(basestations[i]->getProbeCount()) + 
					" on Basestation " + String(i + 1) + "/" + String(basestations.size()));
				
				basestations[i]->initializeProbes();
			}
				
		}
			
	}

	np::setParameter(np::NP_PARAM_BUFFERSIZE, MAXSTREAMBUFFERSIZE);
	np::setParameter(np::NP_PARAM_BUFFERCOUNT, MAXSTREAMBUFFERCOUNT);
}

int NeuropixThread::getNumBasestations()
{
	return basestations.size();
}

void NeuropixThread::setMasterSync(int slotIndex)
{
	basestations[slotIndex]->setSyncAsInput();
}

void NeuropixThread::setSyncOutput(int slotIndex)
{
	basestations[slotIndex]->setSyncAsOutput(0);
}

Array<int> NeuropixThread::getSyncFrequencies()
{
	return basestations[0]->getSyncFrequencies();
}


void NeuropixThread::setSyncFrequency(int slotIndex, int freqIndex)
{
	basestations[slotIndex]->setSyncAsOutput(freqIndex);
}

bool NeuropixThread::checkSlotAndPortCombo(int slotIndex, int portIndex)
{
	if (basestations.size() <= slotIndex)
	{
		return false;
	}
	else {

		if (portIndex > -1)
		{
			bool foundPort = false;
			for (int i = 0; i < basestations[slotIndex]->probes.size(); i++)
			{
				if (basestations[slotIndex]->probes[i]->port == (signed char)portIndex)
				{
					foundPort = true;
				}

			}
			return foundPort;
		}
		else {
			return true;
		}
		
	}
}

unsigned char NeuropixThread::getSlotForIndex(int slotIndex, int portIndex)
{
	return basestations[slotIndex]->slot;
}

signed char NeuropixThread::getPortForIndex(int slotIndex, int portIndex)
{
	return (signed char)portIndex;
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
	api_info->setAttribute("version", api.version);
	neuropix_info.addChildElement(api_info);

	for (int i = 0; i < basestations.size(); i++)
	{
		XmlElement* basestation_info = new XmlElement("BASESTATION");
		basestation_info->setAttribute("index", i + 1);
		basestation_info->setAttribute("slot", int(basestations[i]->slot));
		basestation_info->setAttribute("firmware_version", basestations[i]->boot_version);
		basestation_info->setAttribute("bsc_firmware_version", basestations[i]->basestationConnectBoard->boot_version);
		basestation_info->setAttribute("bsc_part_number", basestations[i]->basestationConnectBoard->part_number);
		basestation_info->setAttribute("bsc_serial_number", String(basestations[i]->basestationConnectBoard->serial_number));

		for (int j = 0; j < basestations[i]->getProbeCount(); j++)
		{
			XmlElement* probe_info = new XmlElement("PROBE");
			probe_info->setAttribute("port", int(basestations[i]->probes[j]->port));
			probe_info->setAttribute("probe_serial_number", String(basestations[i]->probes[j]->serial_number));
			probe_info->setAttribute("hs_serial_number", String(basestations[i]->probes[j]->headstage->serial_number));
			probe_info->setAttribute("hs_part_number", basestations[i]->probes[j]->headstage->part_number);
			probe_info->setAttribute("hs_version", basestations[i]->probes[j]->headstage->version);
			probe_info->setAttribute("flex_part_number", basestations[i]->probes[j]->flex->part_number);
			probe_info->setAttribute("flex_version", basestations[i]->probes[j]->flex->version);

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
	infoString += api.version;
	infoString += "\n";
	infoString += "\n";
	infoString += "\n";

	for (int i = 0; i < basestations.size(); i++)
	{
		infoString += "Basestation ";
		infoString += String(i + 1);
		infoString += "\n";
		infoString += "  Firmware version: ";
		infoString += basestations[i]->boot_version;
		infoString += "\n";
		infoString += "  BSC firmware version: ";
		infoString += basestations[i]->basestationConnectBoard->boot_version;
		infoString += "\n";
		infoString += "  BSC part number: ";
		infoString += basestations[i]->basestationConnectBoard->part_number;
		infoString += "\n";
		infoString += "  BSC serial number: ";
		infoString += String(basestations[i]->basestationConnectBoard->serial_number);
		infoString += "\n";
		infoString += "\n";

		for (int j = 0; j < basestations[i]->getProbeCount(); j++)
		{
			infoString += "    Port ";
			infoString += String(basestations[i]->probes[j]->port);
			infoString += "\n";
			infoString += "\n";
			infoString += "    Probe serial number: ";
			infoString += String(basestations[i]->probes[j]->serial_number);
			infoString += "\n";
			infoString += "\n";
			infoString += "    Headstage serial number: ";
			infoString += String(basestations[i]->probes[j]->headstage->serial_number);
			infoString += "\n";
			infoString += "    Headstage part number: ";
			infoString += basestations[i]->probes[j]->headstage->part_number;
			infoString += "\n";
			infoString += "    Headstage version: ";
			infoString += basestations[i]->probes[j]->headstage->version;
			infoString += "\n";
			infoString += "\n";
			infoString += "    Flex part number: ";
			infoString += basestations[i]->probes[j]->flex->part_number;
			infoString += "\n";
			infoString += "    Flex version: ";
			infoString += basestations[i]->probes[j]->flex->version;
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

    // clear the internal buffer (happens in initializeProbe)
	//for (int i = 0; i < sourceBuffers.size(); i++)
	//{
	//	sourceBuffers[i]->clear();
	//}

	initializationProgress = 0;

	progressBar->setVisible(true);

    counter = 0;
    maxCounter = 0;

	last_npx_timestamp = 0;

	startTimer(500 * totalProbes); // wait for signal chain to be built
	
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

				np::setFileStream(basestations[i]->slot, npxFileName.getFullPathName().getCharPointer());
				np::enableFileStream(basestations[i]->slot, true);

				std::cout << "Basestation " << i << " started recording." << std::endl;
			}
			
		}
	}

	

}

void NeuropixThread::stopRecording()
{
	for (int i = 0; i < basestations.size(); i++)
	{
		np::enableFileStream(basestations[i]->slot, false);
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
}

ProbeStatus NeuropixThread::getProbeStatus(unsigned char slot, signed char port)
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
}

void NeuropixThread::setDefaultChannelNames()
{

	//std::cout << "Setting channel bitVolts to 0.195" << std::endl;
	if (totalProbes > 0)
	{
		
		int chan = 0;

		for (int probe_num = 0; probe_num < totalProbes; probe_num++)
		{
			for (int i = 0; i < 384; i++)
			{
				ChannelCustomInfo info;
				info.name = "AP" + String(i + 1);
				info.gain = 0.1950000f;
				channelInfo.set(chan, info);
				chan++;
			}

			for (int i = 0; i < 384; i++)
			{
				ChannelCustomInfo info;
				info.name = "LFP" + String(i + 1);
				info.gain = 0.1950000f;
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


/** Returns the number of virtual subprocessors this source can generate */
unsigned int NeuropixThread::getNumSubProcessors() const
{
	return totalProbes > 0 ? 2 * totalProbes : 2;
}

/** Returns the number of continuous headstage channels the data source can provide.*/
int NeuropixThread::getNumDataOutputs(DataChannel::DataChannelTypes type, int subProcessorIdx) const
{

	int numChans;

	if (type == DataChannel::DataChannelTypes::HEADSTAGE_CHANNEL && subProcessorIdx % 2 == 0)
		numChans = 384;
	else if (type == DataChannel::DataChannelTypes::HEADSTAGE_CHANNEL && subProcessorIdx % 2 == 1)
		numChans = 384;
	else
		numChans = 0;

	//std::cout << "Num chans for subprocessor " << subProcessorIdx << " = " << numChans << std::endl;
	
	return numChans;
}

/** Returns the number of TTL channels that each subprocessor generates*/
int NeuropixThread::getNumTTLOutputs(int subProcessorIdx) const 
{
	if (subProcessorIdx % 2 == 0)
	{
		return 1;
	}
	else {
		return 0;
	}
}

/** Returns the sample rate of the data source.*/
float NeuropixThread::getSampleRate(int subProcessorIdx) const
{

	float rate;

	if (subProcessorIdx % 2 == 0)
		rate = 30000.0f;
	else
		rate = 2500.0f;


	//std::cout << "Sample rate for subprocessor " << subProcessorIdx << " = " << rate << std::endl;

	return rate;
}

/** Returns the volts per bit of the data source.*/
float NeuropixThread::getBitVolts(const DataChannel* chan) const
{
	//std::cout << "BIT VOLTS == 0.195" << std::endl;
	return 0.1950000f;
}


void NeuropixThread::selectElectrodes(unsigned char slot, signed char port, Array<int> channelStatus)
{

	for (int i = 0; i < basestations.size(); i++)
	{
		basestations[i]->setChannels(slot, port, channelStatus);
	}

}

void NeuropixThread::setAllReferences(unsigned char slot, signed char port, int refId)
{
 
	np::NP_ErrorCode ec;
	np::channelreference_t reference;
	unsigned char intRefElectrodeBank;

	if (refId == 0) // external reference
	{
		reference = np::EXT_REF;
		intRefElectrodeBank = 0;
	}
	else if (refId == 1) // tip reference
	{
		reference = np::TIP_REF;
		intRefElectrodeBank = 0;
	}
	else if (refId > 1) // internal reference
	{
		reference = np::INT_REF;
		intRefElectrodeBank = refId - 2;
	}

	for (int i = 0; i < basestations.size(); i++)
	{
		basestations[i]->setReferences(slot, port, reference, intRefElectrodeBank);
	}
}

void NeuropixThread::setAllGains(unsigned char slot, signed char port, unsigned char apGain, unsigned char lfpGain)
{

	for (int i = 0; i < basestations.size(); i++)
		basestations[i]->setGains(slot, port, apGain, lfpGain);
   
}

void NeuropixThread::setFilter(unsigned char slot, signed char port, bool disableHighPass)
{
	for (int i = 0; i < basestations.size(); i++)
		basestations[i]->setApFilterState(slot, port, disableHighPass);
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

bool NeuropixThread::runBist(unsigned char slot, signed char port, int bistIndex)
{
	bool returnValue = false;

	switch (bistIndex)
	{
	case BIST_SIGNAL:
	{
		np::NP_ErrorCode errorCode = bistSignal(slot, port, &returnValue, stats);
		break;
	}
	case BIST_NOISE:
	{
		if (np::bistNoise(slot, port) == np::SUCCESS)
			returnValue = true;
		break;
	}
	case BIST_PSB:
	{
		if (np::bistPSB(slot, port) == np::SUCCESS)
			returnValue = true;
		break;
	}
	case BIST_SR:
	{
		if (np::bistSR(slot, port) == np::SUCCESS)
			returnValue = true;
		break;
	}
	case BIST_EEPROM:
	{
		if (np::bistEEPROM(slot, port) == np::SUCCESS)
			returnValue = true;
		break;
	}
	case BIST_I2C:
	{
		if (np::bistI2CMM(slot, port) == np::SUCCESS)
			returnValue = true;
		break;
	}
	case BIST_SERDES:
	{
		unsigned char errors;
		np::bistStartPRBS(slot, port);
		sleep(200);
		np::bistStopPRBS(slot, port, &errors);

		if (errors == 0)
			returnValue = true;
		break;
	}
	case BIST_HB:
	{
		if (np::bistHB(slot, port) == np::SUCCESS)
			returnValue = true;
		break;
	} case BIST_BS:
	{
		if (np::bistBS(slot) == np::SUCCESS)
			returnValue = true;
		break;
	} default :
		CoreServices::sendStatusMessage("Test not found.");
	}

	return returnValue;
}

float NeuropixThread::getFillPercentage(unsigned char slot)
{

	//std::cout << "  Thread checking fill percentage for slot " << int(slot) << std::endl;

	for (int i = 0; i < basestations.size(); i++)
	{
		if (basestations[i]->slot == slot)
		{
			//std::cout << "  Found match!" << std::endl;
			return basestations[i]->getFillPercentage();
		}
			
	}

	return 0.0f;
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
