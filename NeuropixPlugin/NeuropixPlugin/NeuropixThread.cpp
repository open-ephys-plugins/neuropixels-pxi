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

NeuropixThread::NeuropixThread(SourceNode* sn) : DataThread(sn), baseStationAvailable(false), probesInitialized(false)
{

	slotID = 0; // for testing only
	port = 0; // for testing only

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


void NeuropixThread::checkSlots()
{
	for (unsigned char basestation = 0; basestation < 7; basestation++)
	{
		errorCode = openBS(basestation); // establishes a data connection with the basestation

		std::cout << "Opening basestation " << int(basestation) << ", error code : " << errorCode << std::endl;

		if (errorCode == SUCCESS)
		{
			connected_basestations.push_back(basestation);

			std::cout << "  Success." << std::endl;
		}
	}

	std::cout << " " << std::endl;
}


void NeuropixThread::checkProbes()
{

	for (int this_basestation = 0; this_basestation < connected_basestations.size(); this_basestation++)
	{
		std::cout << "Checking for probes on slot " << int(connected_basestations[this_basestation]) << std::endl;

		std::vector<int> probes_for_basestation;

		for (signed char port = 1; port < 5; port++)
		{
			errorCode = openProbe(connected_basestations[this_basestation], port);

			std::cout << "Opening probe " << int(port) << ", error code : " << errorCode << std::endl;

			if (errorCode == SUCCESS)
			{
				probes_for_basestation.push_back(port);
				baseStationAvailable = true;

				std::cout << "  Success." << std::endl;
			}
		}

		std::cout << " " << std::endl;

		connected_probes.push_back(probes_for_basestation);
	}

	std::cout << " " << std::endl;
}

void NeuropixThread::openConnection()
{

	// open up the basestation connection and checks for probes
	// - does not initialize the probe or headstage

	checkSlots();

	checkProbes();

}


void NeuropixThread::closeProbes()
{
	// closing things
	for (int this_basestation = 0; this_basestation < connected_basestations.size(); this_basestation++)
	{
		std::cout << "Closing slot " << int(connected_basestations[this_basestation]) << std::endl;

		for (int this_probe = 0; this_probe < connected_probes[this_basestation].size(); this_probe++)
		{
			std::cout << " Closing probe " << int(connected_probes[this_basestation][this_probe]) << std::endl;
			errorCode = close(connected_basestations[this_basestation], connected_probes[this_basestation][this_probe]);
		}

		errorCode = closeBS(connected_basestations[this_basestation]);
	}

	std::cout << " " << std::endl;

}

void NeuropixThread::closeConnection()
{
	closeProbes();
}

/** Returns true if the data source is connected, false otherwise.*/
bool NeuropixThread::foundInputSource()
{
    return baseStationAvailable;
}

void NeuropixThread::getInfo(String& probeInfo, String& hsInfo, String& bscInfo, String& bsInfo, String& apiInfo)
{

	NP_ErrorCode errorCode;

	uint64_t probe_id;
	uint64_t hs_id;
	uint64_t bsc_id;
	char probe_part_number[50];
	char hs_part_number[50];
	char bsc_part_number[50];
	unsigned char flex_version_major;
	unsigned char flex_version_minor;
	unsigned char hs_version_major;
	unsigned char hs_version_minor;
	unsigned char bsc_version_major;
	unsigned char bsc_version_minor;
	unsigned char api_version_major;
	unsigned char api_version_minor;
	unsigned char bsc_fpga_version_major;
	unsigned char bsc_fpga_version_minor;
	unsigned char bs_fpga_version_major;
	unsigned char bs_fpga_version_minor;

	//errorCode = neuropix.readId(slotID, port, probe_id);
	//errorCode = neuropix.readProbePN(slotID, port, probe_part_number);
	//errorCode = neuropix.getFlexVersion(slotID, port, flex_version_major, flex_version_minor);

	//errorCode = neuropix.readHSSN(slotID, port, hs_id);
	//errorCode = neuropix.readHSPN(slotID, port, hs_part_number);
	//errorCode = neuropix.getHSVersion(slotID, port, hs_version_major, hs_version_minor);

	//errorCode = neuropix.readBSCSN(slotID, bsc_id);
	//errorCode = neuropix.readBSCPN(slotID, bsc_part_number);
	//errorCode = neuropix.getBSCVersion(slotID, bsc_version_major, bsc_version_major);

	//errorCode = neuropix.getBSCBootVersion(slotID, bsc_fpga_version_major, bsc_fpga_version_minor);
	//errorCode = neuropix.getBSBootVersion(slotID, bs_fpga_version_major, bs_fpga_version_minor);

	//errorCode = neuropix.getAPIVersion(api_version_major, api_version_minor);

	probeInfo = "SN" + String(probe_id) + "\n" + String(probe_part_number) + ", v" + String(flex_version_minor) + "." + String(flex_version_minor) + "\n";
	hsInfo = "SN" + String(hs_id) + "\n" + String(hs_part_number) + ", v" + String(hs_version_major) + "." + String(hs_version_minor) + "\n";
	bscInfo = "SN" + String(bsc_id) + "\n" + String(bsc_part_number) + ", v" + String(bsc_version_major) + "." + String(bsc_version_major)
		+ ", firmware v" + String(bsc_fpga_version_major) + "." + String(bsc_fpga_version_minor) + "\n";
	bsInfo = "firmware v" + String(bs_fpga_version_major) + "." + String(bs_fpga_version_minor) + "\n";
	apiInfo = "v" + String(api_version_major) + "." + String(api_version_minor);

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
    
    startTimer(100);
   
    return true;
}

void NeuropixThread::timerCallback()
{

    stopTimer();

	// loop through probes
	for (int this_basestation = 0; this_basestation < connected_basestations.size(); this_basestation++)
	{
		std::cout << " Checking slot " << int(connected_basestations[this_basestation]) << std::endl;

		const size_t probe_count = connected_probes[this_basestation].size();

		if (probe_count == 0)
			break;

		if (!probesInitialized)
		{

			errorCode = setTriggerInput(connected_basestations[this_basestation], TRIGIN_SW);

			for (int this_probe = 0; this_probe < probe_count; this_probe++)
			{
				std::cout << " Initializing probe " << int(connected_probes[this_basestation][this_probe]) << std::endl;

				errorCode = init(connected_basestations[this_basestation], connected_probes[this_basestation][this_probe]);
				errorCode = setOPMODE(connected_basestations[this_basestation], connected_probes[this_basestation][this_probe], RECORDING);
				errorCode = setHSLed(connected_basestations[this_basestation], connected_probes[this_basestation][this_probe], false);

				if (errorCode == SUCCESS)
				{
					std::cout << "     Probe initialized." << std::endl;
				}
				else {
					std::cout << "     Failed with error code " << errorCode << std::endl;
				}

			}

			std::cout << " Arming basestation " << int(connected_basestations[this_basestation]) << std::endl;

			//setFileStream(connected_basestations[this_basestation], "test.npx2");

			errorCode = arm(connected_basestations[this_basestation]);

			std::cout << "     Basestation armed." << std::endl;

			probesInitialized = true;
		}

		//enableFileStream(connected_basestations[this_basestation], true);
		errorCode = setSWTrigger(connected_basestations[this_basestation]);

	}

    startThread();

}


/** Stops data transfer.*/
bool NeuropixThread::stopAcquisition()
{

    if (isThreadRunning())
    {
        signalThreadShouldExit();
    }

	for (int this_basestation = 0; this_basestation < connected_basestations.size(); this_basestation++)
		errorCode = arm(connected_basestations[this_basestation]);

    return true;
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
	//ChannelReference reference;
	unsigned char intRefElectrodeBank;

	if (refId == 0) // external reference
	{
		//reference = EXT_REF;
		intRefElectrodeBank = 0;
	}
	else if (refId == 1) // tip reference
	{
		//reference = TIP_REF;
		intRefElectrodeBank = 0;
	}
	else if (refId > 1) // internal reference
	{
		//reference = INT_REF;
		intRefElectrodeBank = refId - 2;
	}

	for (int i = 0; i < 384; i++)
	{
		//ec = neuropix.setReference(slotID, port, i, reference, intRefElectrodeBank);
	}

	//neuropix.writeProbeConfiguration(slotID, port, false);

	std::cout << "Set all references to " << refId << "; error code = " << ec << std::endl;
}

void NeuropixThread::setAllGains(unsigned char apGain, unsigned char lfpGain)
{
	NP_ErrorCode ec;

	for (int i = 0; i < 384; i++)
	{
		//ec = neuropix.setGain(slotID, port, i, apGain, lfpGain);
		apGains.set(i, apGain);
		lfpGains.set(i, lfpGain);
	}

	//neuropix.writeProbeConfiguration(slotID, port, false);

    std::cout << "Set gains to " << gains[int(apGain)] << " and " << gains[int(lfpGain)] << "; error code = " << ec << std::endl;
   
}

void NeuropixThread::setFilter(bool filterState)
{
	NP_ErrorCode ec;

	//for (int i = 0; i < 384; i++)
	//  ec = neuropix.setAPCornerFrequency(slotID, port, i, !filterState); // true = disabled, false = enabled

	//neuropix.writeProbeConfiguration(slotID, port, false);

    std::cout << "Set filter to " << filterState << "; error code = " << ec << std::endl;
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


void NeuropixThread::calibrateProbe()
{

    std::cout << "Applying ADC calibration..." << std::endl;
    //neuropix.neuropix_applyAdcCalibrationFromEeprom();
    std::cout << "Applying gain correction settings..." << std::endl;
    //neuropix.neuropix_applyGainCalibrationFromEeprom();
    std::cout << "Done." << std::endl;

}

void NeuropixThread::calibrateADCs()
{

	//std::cout << "Applying ADC calibration..." << std::endl;
	//ErrorCode e = neuropix.neuropix_applyAdcCalibrationFromEeprom();
	//std::cout << "Finished with error code " << e << std::endl;

}

void NeuropixThread::calibrateGains()
{

	//std::cout << "Applying gain correction settings..." << std::endl;
	//ErrorCode e = neuropix.neuropix_applyGainCalibrationFromEeprom();
	//std::cout << "Finished with error code " << e << std::endl;

}

void NeuropixThread::calibrateFromCsv()
{

	//Read from csv and apply to API and read from API
	//std::cout << "Reading files from " << directory.getFullPathName() << std::endl;

	//File comparatorCsv = directory.getChildFile("Comparator_calibration.csv");
	//File offsetCsv = directory.getChildFile("Offset_calibration.csv");
	//File slopeCsv = directory.getChildFile("Slope_calibration.csv");
	//File gainCsv = directory.getChildFile("Gain_calibration.csv");

	File baseDirectory = File::getCurrentWorkingDirectory().getFullPathName();
	File calibrationDirectory = baseDirectory.getChildFile("CalibrationInfo");
	File probeDirectory = calibrationDirectory.getChildFile(String(probeId));
	String adcFile = probeDirectory.getChildFile(String(probeId) + "_ADCCalibration.csv").getFullPathName();
	String gainFile = probeDirectory.getChildFile(String(probeId) + "_gainCalValues.csv").getFullPathName();


    std::cout << adcFile << std::endl;
	std::cout << gainFile << std::endl;

	NP_ErrorCode errorCode;

	//errorCode = neuropix.setADCCalibration(slotID, port, adcFile.toRawUTF8());

	if (errorCode == SUCCESS)
	{
		std::cout << "Successful ADC calibration" << std::endl;
	}
	else {
		std::cout << "Problem with ADC calibration, error code = " << errorCode << std::endl;
	}
	
	//errorCode = neuropix.setGainCalibration(slotID, port, gainFile.toRawUTF8());

	if (errorCode == SUCCESS)
	{
		std::cout << "Successful gain calibration" << std::endl;
	}
	else {
		std::cout << "Problem with gain calibration, error code = " << errorCode << std::endl;
	}

}

bool NeuropixThread::updateBuffer()
{

	unsigned int actualNumPackets;
	unsigned int requestedNumPackets = 250;

	//std::cout << "Attempting data read. " << std::endl;

	for (int this_basestation = 0; this_basestation < connected_basestations.size(); this_basestation++)
	{
		//std::cout << " Checking slot " << int(connected_basestations[this_basestation]) << std::endl;

		const size_t probe_count = connected_probes[this_basestation].size();

		for (int this_probe = 0; this_probe < probe_count; this_probe++)
		{
			size_t count = SAMPLECOUNT;

			errorCode = readElectrodeData(
				connected_basestations[this_basestation],
				connected_probes[this_basestation][this_probe],
				&packet[0],
				&count,
				count);

			if (errorCode == SUCCESS && count > 0 && this_probe == 0)
			{

				//std::cout << "Got data. " << std::endl;
				float data[384];
				float data2[384];

				for (int packetNum = 0; packetNum < count; packetNum++)
				{
					for (int i = 0; i < 12; i++)
					{
						eventCode = 0; packet[packetNum].SYNC[i]; // AUX_IO<0:13>
						//std::cout << "Read event data. " << std::endl;

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

					eventCode = 0;

					sourceBuffers[1]->addToBuffer(data2, &timestampLfp, &eventCode, 1);
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

		for (int this_basestation = 0; this_basestation < connected_basestations.size(); this_basestation++)
		{
			//std::cout << " Checking slot " << int(connected_basestations[this_basestation]) << std::endl;

			const size_t probe_count = connected_probes[this_basestation].size();

			for (int this_probe = 0; this_probe < probe_count; this_probe++)
			{

				getElectrodeDataFifoState(
					connected_basestations[this_basestation],
					connected_probes[this_basestation][this_probe],
					&packetsAvailable,
					&headroom);

				std::cout << "  FIFO filling  " << this_probe << ": " << packetsAvailable << std::endl;
			}
		}

		std::cout << "  " << std::endl;
	}

    return true;
}