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

NeuropixThread::NeuropixThread(SourceNode* sn) : DataThread(sn), baseStationAvailable(false), probesInitialized(false), recordingNumber(0), isRecording(false)
{

	api.getInfo();

    for (int i = 0; i < 384; i++)
    {
        channelMap.add(i);
        outputOn.add(true);
    }

	for (int i = 0; i < 16; i++)
		fillPercentage.add(0.0);

    gains.add(50.0f);
    gains.add(125.0f);
    gains.add(250.0f);
    gains.add(500.0f);
    gains.add(1000.0f);
    gains.add(1500.0f);
    gains.add(2000.0f);
    gains.add(3000.0f);

    refs.add(0);
    refs.add(1);
    refs.add(192);
    refs.add(576);
    refs.add(960);

    counter = 0;

    eventCode = 0;
    maxCounter = 0;

    openConnection();

}

NeuropixThread::~NeuropixThread()
{
    closeConnection();
}

void NeuropixThread::openConnection()
{

	uint32_t availableslotmask;
	totalProbes = 0;

	scanPXI(&availableslotmask);

	for (int slot = 0; slot < 32; slot++)
	{
		//std::cout << "Slot " << i << " available: " << ((availableslotmask >> i) & 1) << std::endl;
		if ((availableslotmask >> slot) & 1)
		{
			basestations.add(new Basestation(slot));
		}
	}

	std::cout << "Basestations initialized" << std::endl;

	bool foundSync = false;

	for (int i = 0; i < basestations.size(); i++)
	{
		if (basestations[i]->getProbeCount() > 0)
		{
			totalProbes += basestations[i]->getProbeCount();

			baseStationAvailable = true;

			if (!foundSync)
			{
				basestations[i]->makeSyncMaster();
				selectedSlot = basestations[i]->slot;
				selectedPort = basestations[i]->probes[0]->port;
				foundSync = true;
			}

			// set up data buffers
			sourceBuffers.add(new DataBuffer(384, 10000));
			sourceBuffers.add(new DataBuffer(384, 10000));

			//for (int probe_num = 0; probe_num < basestations[i]->getProbeCount(); probe_num++)
			//{
			//	std::cout << "Creating buffers for slot " << int(basestations[i]->slot) << ", probe " << int(basestations[i]->probes[probe_num]->port) << std::endl;
			//	sourceBuffers.add(new DataBuffer(384, 10000));  // AP band buffer

			//	basestations[i]->probes[probe_num]->apBuffer = sourceBuffers.getLast();

			//	sourceBuffers.add(new DataBuffer(384, 10000));  // LFP band buffer

			//	basestations[i]->probes[probe_num]->lfpBuffer = sourceBuffers.getLast();
			//}
				
		}
			
	}
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
	// if portIndex = -1, return the slot number; otherwise, only return if a probe is connected

	if (checkSlotAndPortCombo(slotIndex, portIndex))
	{
		return basestations[slotIndex]->slot;
	}
	else {
		return -1;
	}
}

signed char NeuropixThread::getPortForIndex(int slotIndex, int portIndex)
{
	if (checkSlotAndPortCombo(slotIndex, portIndex))
	{
		return (signed char)portIndex;
	}
	else {
		return -1;
	}
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

    // clear the internal buffer
	//for (int i = 0; i < sourceBuffers.size(); i++)
	//{
	//	sourceBuffers[i]->clear();
	//}

    counter = 0;
    eventCode = 0;
    maxCounter = 0;
    
	for (int i = 0; i < basestations.size(); i++)
	{
		basestations[i]->initializeProbes();
	}

	for (int i = 0; i < basestations.size(); i++)
	{
		basestations[i]->startAcquisition();
	}

	last_npx_timestamp = 0;

	startThread();
   
    return true;
}

void NeuropixThread::timerCallback()
{

    stopTimer();

	startRecording();

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

				setFileStream(basestations[i]->slot, npxFileName.getFullPathName().getCharPointer());
				enableFileStream(basestations[i]->slot, true);

				std::cout << "Basestation " << i << " started recording." << std::endl;
			}
			
		}
	}

	

}

void NeuropixThread::stopRecording()
{
	for (int i = 0; i < basestations.size(); i++)
	{
		enableFileStream(basestations[i]->slot, false);
	}

	isRecording = false;

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
				}
			}
		}
	}

	basestations[newSlot]->probes[newPort]->ap_timestamp = basestations[currentSlot]->probes[currentPort]->ap_timestamp;
	basestations[newSlot]->probes[newPort]->lfp_timestamp = basestations[currentSlot]->probes[currentPort]->lfp_timestamp;

	selectedSlot = slot;
	selectedPort = port;
}

void NeuropixThread::setDefaultChannelNames()
{

	//std::cout << "Setting channel bitVolts to 0.195" << std::endl;

	for (int bs_num = 0; bs_num < basestations.size(); bs_num++)
	{
		for (int i = 0; i < 384; i++)
		{
			ChannelCustomInfo info;
			info.name = "AP" + String(i + 1);
			info.gain = 0.1950000f;
			channelInfo.set(i, info);
		}

		for (int i = 0; i < 384; i++)
		{
			ChannelCustomInfo info;
			info.name = "LFP" + String(i + 1);
			info.gain = 0.1950000f;
			channelInfo.set(384 + i, info);
		}
	}

	
}

bool NeuropixThread::usesCustomNames() const
{
	return false;
}


/** Returns the number of virtual subprocessors this source can generate */
unsigned int NeuropixThread::getNumSubProcessors() const
{

	return 2; // *totalProbes;
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


//	std::cout << "Sample rate for subprocessor " << subProcessorIdx << " = " << rate << std::endl;

	return rate;
}

/** Returns the volts per bit of the data source.*/
float NeuropixThread::getBitVolts(const DataChannel* chan) const
{
	//std::cout << "BIT VOLTS == 0.195" << std::endl;
	return 0.1950000f;
}


void NeuropixThread::selectElectrode(int chNum, int connection, bool transmit)
{

    //neuropix.selectElectrode(slotID, port, chNum, connection);
  
    //std::cout << "Connecting input " << chNum << " to channel " << connection << "; error code = " << scec << std::endl;

}

void NeuropixThread::setAllReferences(unsigned char slot, signed char port, int refId)
{
 
	NP_ErrorCode ec;
	channelreference_t reference;
	unsigned char intRefElectrodeBank;

	if (refId == 0) // external reference
	{
		reference = EXT_REF;
		intRefElectrodeBank = 0;
	}
	else if (refId == 1) // tip reference
	{
		reference = TIP_REF;
		intRefElectrodeBank = 0;
	}
	else if (refId > 1) // internal reference
	{
		reference = INT_REF;
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

void NeuropixThread::setFilter(unsigned char slot, signed char port, bool filterState)
{
	for (int i = 0; i < basestations.size(); i++)
		basestations[i]->setApFilterState(slot, port, filterState);
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
		NP_ErrorCode errorCode = bistSignal(slot, port, &returnValue, stats);
		break;
	}
	case BIST_NOISE:
	{
		if (bistNoise(slot, port) == SUCCESS)
			returnValue = true;
		break;
	}
	case BIST_PSB:
	{
		if (bistPSB(slot, port) == SUCCESS)
			returnValue = true;
		break;
	}
	case BIST_SR:
	{
		if (bistSR(slot, port) == SUCCESS)
			returnValue = true;
		break;
	}
	case BIST_EEPROM:
	{
		if (bistEEPROM(slot, port) == SUCCESS)
			returnValue = true;
		break;
	}
	case BIST_I2C:
	{
		if (bistI2CMM(slot, port) == SUCCESS)
			returnValue = true;
		break;
	}
	case BIST_SERDES:
	{
		unsigned char errors;
		bistStartPRBS(slot, port);
		sleep(200);
		bistStopPRBS(slot, port, &errors);

		if (errors == 0)
			returnValue = true;
		break;
	}
	case BIST_HB:
	{
		if (bistHB(slot, port) == SUCCESS)
			returnValue = true;
		break;
	} case BIST_BS:
	{
		if (bistBS(slot) == SUCCESS)
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

	bool shouldRecord = CoreServices::getRecordingStatus();

	if (!isRecording && shouldRecord)
	{
		isRecording = true;
		startTimer(500);
	}
	else if (isRecording && !shouldRecord)
	{
		stopRecording();
	}

	//std::cout << "Attempting data read. " << std::endl;

	for (int bs = 0; bs < basestations.size(); bs++)
	{
		//std::cout << " Checking slot " << int(basestations[bs]->slot) << std::endl;

		for (int probe_num = 0; probe_num < basestations[bs]->getProbeCount(); probe_num++)
		{
			//std::cout << " Checking probe " << int(basestations[bs]->probes[probe_num]->port) << std::endl;

			size_t count = SAMPLECOUNT;

			errorCode = readElectrodeData(
				basestations[bs]->slot,
				basestations[bs]->probes[probe_num]->port,
				&packet[0],
				&count,
				count);

			if (errorCode == SUCCESS && 
				count > 0 && 
				basestations[bs]->slot == selectedSlot &&
				basestations[bs]->probes[probe_num]->port == selectedPort)
			{

				//std::cout << "Got data. " << std::endl;
				float apSamples[384];
				float lfpSamples[384];

				int64* ap_timestamp = &basestations[bs]->probes[probe_num]->ap_timestamp;
				int64* lfp_timestamp = &basestations[bs]->probes[probe_num]->lfp_timestamp;

				for (int packetNum = 0; packetNum < count; packetNum++)
				{
					for (int i = 0; i < 12; i++)
					{
						uint64 oldeventcode = eventCode;

						eventCode = packet[packetNum].Status[i] >> 6; // AUX_IO<0:13>

						uint32_t npx_timestamp = packet[packetNum].timestamp[i];

						if (npx_timestamp == last_npx_timestamp)
						{
							std::cout << "Got repeated timestamp at sample " << *ap_timestamp << std::endl;
							std::cout << npx_timestamp << std::endl;
						}

						if (*ap_timestamp % 3000 == 0) // check fifo filling
						{
							size_t packetsAvailable;
							size_t headroom;

							getElectrodeDataFifoState(
								basestations[bs]->slot,
								basestations[bs]->probes[probe_num]->port,
								&packetsAvailable,
								&headroom);

							basestations[bs]->probes[probe_num]->fifoFillPercentage = float(packetsAvailable) / float(packetsAvailable + headroom);
						}
						

						last_npx_timestamp = npx_timestamp;

						for (int j = 0; j < 384; j++)
						{
							apSamples[j] = float(packet[packetNum].apData[i][j]) * 1.2f / 1024.0f * 1000000.0f / gains[basestations[bs]->probes[probe_num]->apGains[j]]; // convert to microvolts

							if (i == 0) // && sendLfp)
								lfpSamples[j] = float(packet[packetNum].lfpData[j]) * 1.2f / 1024.0f * 1000000.0f / gains[basestations[bs]->probes[probe_num]->lfpGains[j]]; // convert to microvolts
						}

						*ap_timestamp += 1;

						sourceBuffers[0]->addToBuffer(apSamples, ap_timestamp, &eventCode, 1);
						
					}
					*lfp_timestamp += 1;

					sourceBuffers[1]->addToBuffer(lfpSamples, lfp_timestamp, &eventCode, 1);

				}

				

				//std::cout << "READ SUCCESS!" << std::endl;  

			}
			else if (errorCode != SUCCESS)
			{
				std::cout << "Error code: " << errorCode << std::endl;
			}// dump data
		} // loop through probes
	} //loop through basestations

    return true;
}