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

	sourceBuffers.add(new DataBuffer(384, 10000));  // AP band buffer
	sourceBuffers.add(new DataBuffer(384, 10000));  // LFP band buffer

    for (int i = 0; i < 384; i++)
    {
        lfpGains.add(0); // default setting = 50x
        apGains.add(4); // default setting = 1000x
        channelMap.add(i);
        outputOn.add(true);
    }

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
    timestampAp = 0;
	timestampLfp = 0;
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

	scanPXI(&availableslotmask);

	for (int slot = 0; slot < 32; slot++)
	{
		//std::cout << "Slot " << i << " available: " << ((availableslotmask >> i) & 1) << std::endl;
		if ((availableslotmask >> slot) & 1)
		{
			basestations.add(new Basestation(slot));
		}
	}

	for (int i = 0; i < basestations.size(); i++)
	{
		if (basestations[i]->getProbeCount() > 0)
		{
			baseStationAvailable = true;
			selectedSlot = basestations[0]->slot;
			selectedPort = basestations[0]->probes[0]->port;
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
}

unsigned char NeuropixThread::getSlotForIndex(int slotIndex, int portIndex)
{
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
	sourceBuffers[0]->clear();
	sourceBuffers[1]->clear();

    counter = 0;
    timestampAp = 0;
	timestampLfp = 0;
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
			File fullPath = savingDirectory.getChildFile(pathName);

			if (!fullPath.exists())
			{
				fullPath.createDirectory();
			}

			File npxFileName = fullPath.getChildFile("recording_slot" + String(basestations[i]->slot) + "_" + String(recordingNumber) + ".npx2");

			setFileStream(basestations[i]->slot, npxFileName.getFullPathName().getCharPointer());
			enableFileStream(basestations[i]->slot, true);
		}
	}

	std::cout << "NeuropixThread started recording." << std::endl;

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
	selectedSlot = slot;
	selectedPort = port;
}

void NeuropixThread::setDefaultChannelNames()
{

	//std::cout << "Setting channel bitVolts to 0.195" << std::endl;

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

bool NeuropixThread::usesCustomNames() const
{
	return true;
}


/** Returns the number of virtual subprocessors this source can generate */
unsigned int NeuropixThread::getNumSubProcessors() const
{
	return 2;
}

/** Returns the number of continuous headstage channels the data source can provide.*/
int NeuropixThread::getNumDataOutputs(DataChannel::DataChannelTypes type, int subProcessorIdx) const
{

	int numChans;

	if (type == DataChannel::DataChannelTypes::HEADSTAGE_CHANNEL && subProcessorIdx == 0)
		numChans = 384;
	else if (type == DataChannel::DataChannelTypes::HEADSTAGE_CHANNEL && subProcessorIdx == 1)
		numChans = 384;
	else
		numChans = 0;

	//std::cout << "Num chans for subprocessor " << subProcessorIdx << " = " << numChans << std::endl;
	
	return numChans;
}

/** Returns the number of TTL channels that each subprocessor generates*/
int NeuropixThread::getNumTTLOutputs(int subProcessorIdx) const 
{
	if (subProcessorIdx == 0)
	{
		return 16;
	}
	else {
		return 0;
	}
}

/** Returns the sample rate of the data source.*/
float NeuropixThread::getSampleRate(int subProcessorIdx) const
{

	float rate;

	if (subProcessorIdx == 0)
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

void NeuropixThread::setAllReferences(int refId)
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
		basestations[i]->setReferences(reference, intRefElectrodeBank);

}

void NeuropixThread::setAllGains(unsigned char apGain, unsigned char lfpGain)
{

	for (int i = 0; i < basestations.size(); i++)
		basestations[i]->setGains(apGain, lfpGain);

	for (int ch = 0; ch < 384; ch++)
	{
		apGains.set(ch, apGain);
		lfpGains.set(ch, lfpGain);
	}
   
}

void NeuropixThread::setFilter(bool filterState)
{
	for (int i = 0; i < basestations.size(); i++)
		basestations[i]->setApFilterState(filterState);
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
		//std::cout << " Checking slot " << int(connected_basestations[this_basestation]) << std::endl;

		for (int probe_num = 0; probe_num < basestations[bs]->getProbeCount(); probe_num++)
		{
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
				float data[384];
				float data2[384];

				for (int packetNum = 0; packetNum < count; packetNum++)
				{
					for (int i = 0; i < 12; i++)
					{
						uint64 oldeventcode = eventCode;
						eventCode = 1 - (packet[packetNum].Trigger[i] && 64); // AUX_IO<0:13>
						//if (ec != 64)
						//	std::cout << "Got nonzero event code: " << ec << std::endl;
						//std::cout << "Read event data. " << std::endl;

						if (eventCode != oldeventcode)
							std::cout << "event code: " << eventCode << std::endl;

						for (int j = 0; j < 384; j++)
						{
							data[j] = float(packet[packetNum].apData[i][j]) * 1.2f / 1024.0f * 1000000.0f / gains[apGains[j]]; // *-1000000.0f; // convert to microvolts

							if (i == 0) // && sendLfp)
								data2[j] = float(packet[packetNum].lfpData[j]) * 1.2f / 1024.0f * 1000000.0f / gains[lfpGains[j]]; // *-1000000.0f; // convert to microvolts
						}

						sourceBuffers[0]->addToBuffer(data, &timestampAp, &eventCode, 1);
						timestampAp += 1;
						//std::cout << "Added AP data to buffer. " << std::endl;
					}

					uint64 lfpeventCode = 0;

					sourceBuffers[1]->addToBuffer(data2, &timestampLfp, &lfpeventCode, 1);
					timestampLfp += 1;
				}

				//std::cout << "READ SUCCESS!" << std::endl;  

			}
			else if (errorCode != SUCCESS)
			{
				std::cout << "Error code: " << errorCode << std::endl;
			}// dump data
		} // loop through probes
	} //loop through basestations


	if (timestampAp % 60000 == 0)
	{
		size_t packetsAvailable;
		size_t headroom;

		for (int this_basestation = 0; this_basestation < basestations.size(); this_basestation++)
		{
			//std::cout << " Checking slot " << int(connected_basestations[this_basestation]) << std::endl;

			for (int this_probe = 0; this_probe < basestations[this_basestation]->getProbeCount(); this_probe++)
			{

				getElectrodeDataFifoState(
					basestations[this_basestation]->slot,
					basestations[this_basestation]->probes[this_probe]->port,
					&packetsAvailable,
					&headroom);

				basestations[this_basestation]->probes[this_probe]->fifoFillPercentage = float(packetsAvailable) / float(packetsAvailable + headroom);

				//std::cout << "  FIFO filling  " << this_probe << ": " << packetsAvailable << std::endl;
			}
			
		}

		//std::cout << "Current event code: " << eventCode << std::endl;

		//std::cout << "  " << std::endl;
	}

    return true;
}