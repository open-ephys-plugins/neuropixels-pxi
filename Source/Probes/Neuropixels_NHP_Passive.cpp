/*
------------------------------------------------------------------

This file is part of the Open Ephys GUI
Copyright (C) 2018 Allen Institute for Brain Science and Open Ephys

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

#include "Neuropixels_NHP_Passive.h"
#include "Geometry.h"

#define MAXLEN 50

void Neuropixels_NHP_Passive::getInfo()
{
	errorCode = Neuropixels::readProbeSN(basestation->slot, headstage->port, dock, &info.serial_number);

	char pn[MAXLEN];
	errorCode = Neuropixels::readProbePN(basestation->slot_c, headstage->port_c, dock, pn, MAXLEN);

	info.part_number = String(pn);
}

Neuropixels_NHP_Passive::Neuropixels_NHP_Passive(Basestation* bs, Headstage* hs, Flex* fl) : Probe(bs, hs, fl, 0)
{

	setStatus(ProbeStatus::DISCONNECTED);

	channel_map = { 6,10,14,18,22,26,30,34,38,42,50,2,60,62,64,
		54,58,103,56,115,107,46,119,111,52,123,4,127,8,12,16,20,
		24,28,32,36,40,44,48,121,105,93,125,101,89,99,97,85,95,109,
		81,87,113,77,83,117,73,91,71,69,79,67,65,75,63,61,47,59,57,
		51,55,53,43,9,49,35,13,45,39,17,41,31,29,37,1,25,33,5,21,84,
		88,92,96,100,104,108,112,116,120,124,3,128,7,80,19,11,82,23,15,
		76,27,70,74,68,66,72,126,78,86,90,94,98,102,106,110,114,118,122 };


	Geometry::forPartNumber(info.part_number, electrodeMetadata, probeMetadata);

	apGainIndex = 3;
	lfpGainIndex = 2;
	referenceIndex = 0;

	availableApGains.add(50.0f);
	availableApGains.add(125.0f);
	availableApGains.add(250.0f);
	availableApGains.add(500.0f);
	availableApGains.add(1000.0f);
	availableApGains.add(1500.0f);
	availableApGains.add(2000.0f);
	availableApGains.add(3000.0f);

	availableLfpGains.add(50.0f);
	availableLfpGains.add(125.0f);
	availableLfpGains.add(250.0f);
	availableLfpGains.add(500.0f);
	availableLfpGains.add(1000.0f);
	availableLfpGains.add(1500.0f);
	availableLfpGains.add(2000.0f);
	availableLfpGains.add(3000.0f);

	availableReferences.add("REF_ELEC");
	availableReferences.add("TIP_REF");

	errorCode = Neuropixels::NP_ErrorCode::SUCCESS;

}

void Neuropixels_NHP_Passive::initialize()
{
	errorCode = Neuropixels::init(basestation->slot, headstage->port, dock);

	if (errorCode == Neuropixels::SUCCESS)
	{
		errorCode = Neuropixels::setOPMODE(basestation->slot, headstage->port, dock, Neuropixels::RECORDING);
		errorCode = Neuropixels::setHSLed(basestation->slot, headstage->port, false);

		calibrate();
		ap_timestamp = 0;
		lfp_timestamp = 0;
		eventCode = 0;
		setStatus(ProbeStatus::CONNECTED);
	}
}


void Neuropixels_NHP_Passive::calibrate()
{
	File baseDirectory = File::getSpecialLocation(File::currentExecutableFile).getParentDirectory();
	File calibrationDirectory = baseDirectory.getChildFile("CalibrationInfo");
	File probeDirectory = calibrationDirectory.getChildFile(String(info.serial_number));

	std::cout << probeDirectory.getFullPathName() << std::endl;

	if (probeDirectory.exists())
	{
		String adcFile = probeDirectory.getChildFile(String(info.serial_number) + "_ADCCalibration.csv").getFullPathName();
		String gainFile = probeDirectory.getChildFile(String(info.serial_number) + "_gainCalValues.csv").getFullPathName();
		std::cout << adcFile << std::endl;

		errorCode = Neuropixels::setADCCalibration(basestation->slot, headstage->port, adcFile.toRawUTF8());

		if (errorCode == 0)
			std::cout << "Successful ADC calibration." << std::endl;
		else
			std::cout << "Unsuccessful ADC calibration, failed with error code: " << errorCode << std::endl;

		std::cout << gainFile << std::endl;

		errorCode = Neuropixels::setGainCalibration(basestation->slot, headstage->port, dock, gainFile.toRawUTF8());

		if (errorCode == 0)
			std::cout << "Successful gain calibration." << std::endl;
		else
			std::cout << "Unsuccessful gain calibration, failed with error code: " << errorCode << std::endl;

		errorCode = Neuropixels::writeProbeConfiguration(basestation->slot, headstage->port, dock, false);
	}
	else {
		// show popup notification window
		String message = "Missing calibration files for probe serial number " + String(info.serial_number) + ". ADC and Gain calibration files must be located in 'CalibrationInfo\\<serial_number>' folder in the directory where the Open Ephys GUI was launched. The GUI will proceed without calibration.";
		AlertWindow::showMessageBox(AlertWindow::AlertIconType::WarningIcon, "Calibration files missing", message, "OK");
	}
}

void Neuropixels_NHP_Passive::selectElectrodes(ProbeSettings settings, bool shouldWriteConfiguration)
{

	// not available

}

void Neuropixels_NHP_Passive::setApFilterState(bool disableHighPass, bool shouldWriteConfiguration)
{
	// to implement
}

void Neuropixels_NHP_Passive::setAllGains(int apGain, int lfpGain, bool shouldWriteConfiguration)
{
	// to implement
}


void Neuropixels_NHP_Passive::setAllReferences(int referenceIndex, bool shouldWriteConfiguration)
{

	Neuropixels::channelreference_t refId;

	//for (int channel = 0; channel < channel_count; channel++)
	//	Neuropixels::setReference(basestation->slot, 
	//							  headstage->port, 
	//							  dock,
	//							  channel, 
	//							  refId, 
	//							  refElectrodeBank);

	if (shouldWriteConfiguration)
		errorCode = Neuropixels::writeProbeConfiguration(basestation->slot, headstage->port, dock, false);

	//std::cout << "Wrote reference " << int(refId) << ", " << int(refElectrodeBank) << " with error code " << errorCode << std::endl;
}

void Neuropixels_NHP_Passive::writeConfiguration()
{
	errorCode = Neuropixels::writeProbeConfiguration(basestation->slot, headstage->port, dock, false);
}

void Neuropixels_NHP_Passive::startAcquisition()
{
	ap_timestamp = 0;
	apBuffer->clear();

	std::cout << "  Starting thread." << std::endl;
	startThread();
}

void Neuropixels_NHP_Passive::stopAcquisition()
{
	stopThread(1000);
}

void Neuropixels_NHP_Passive::run()
{

	//std::cout << "Thread running." << std::endl;

	Neuropixels::streamsource_t source = Neuropixels::SourceAP;

	int16_t data[SAMPLECOUNT * 384];

	Neuropixels::PacketInfo packetInfo[SAMPLECOUNT];

	while (!threadShouldExit())
	{
		int count = SAMPLECOUNT;

		errorCode = Neuropixels::readPackets(
			basestation->slot,
			headstage->port,
			dock,
			source,
			&packetInfo[0],
			&data[0],
			384,
			count,
			&count);

		if (errorCode == np::SUCCESS && count > 0)
		{
			float apSamples[385];

			for (int packetNum = 0; packetNum < count; packetNum++)
			{

				eventCode = packetInfo[packetNum].Status >> 6; // AUX_IO<0:13>

				uint32_t npx_timestamp = packetInfo[packetNum].Timestamp;

				for (int j = 0; j < 384; j++)
				{
					apSamples[j] = float(data[packetNum * 384 + j]) * 1.2f / 1024.0f * 1000000.0f / 80.0f; // convert to microvolts
				}

				ap_timestamp += 1;

				if (sendSync)
					apSamples[384] = (float)eventCode;

				apBuffer->addToBuffer(apSamples, &ap_timestamp, &eventCode, 1);

				if (ap_timestamp % 30000 == 0)
				{
					size_t packetsAvailable;
					size_t headroom;

					np::getElectrodeDataFifoState(
						basestation->slot,
						headstage->port,
						&packetsAvailable,
						&headroom);

					//std::cout << "Basestation " << int(basestation->slot) << ", probe " << int(port) << ", packets: " << packetsAvailable << std::endl;

					fifoFillPercentage = float(packetsAvailable) / float(packetsAvailable + headroom);
				}

			}

		}
		else if (errorCode != np::SUCCESS)
		{
			std::cout << "Error code: " << errorCode << "for Basestation " << int(basestation->slot) << ", probe " << int(headstage->port) << std::endl;
		}
	}

}
