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

#include "Neuropixels1_v3.h"
#include "Geometry.h"

#define MAXLEN 50

void Neuropixels1_v3::getInfo()
{
	errorCode = Neuropixels::readProbeSN(basestation->slot, headstage->port, dock, &info.serial_number);

	char pn[MAXLEN];
	errorCode = Neuropixels::readProbePN(basestation->slot, headstage->port, dock, pn, MAXLEN);

	info.part_number = String(pn);
}

Neuropixels1_v3::Neuropixels1_v3(Basestation* bs, Headstage* hs, Flex* fl) : Probe(bs, hs, fl, 1)
{
	getInfo();

	setStatus(ProbeStatus::DISCONNECTED);

	Geometry::forPartNumber(info.part_number, electrodeMetadata, probeMetadata);

	if (Geometry::forPartNumber(info.part_number, electrodeMetadata, probeMetadata))
	{

		name = probeMetadata.name;
		type = probeMetadata.type;

		apGainIndex = 3;
		lfpGainIndex = 2;
		referenceIndex = 0;
		apFilterState = true;

		channel_count = 384;
		lfp_sample_rate = 2500.0f;
		ap_sample_rate = 30000.0f;


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

		availableReferences.add("Ext");
		availableReferences.add("Tip");
		availableReferences.add("192");
		availableReferences.add("576");
		availableReferences.add("960");

		availableBanks = { Bank::A,
			Bank::B,
			Bank::C,
			Bank::D,
			Bank::E,
			Bank::F,
			Bank::G,
			Bank::H,
			Bank::I,
			Bank::J,
			Bank::K,
			Bank::L};

		errorCode = Neuropixels::NP_ErrorCode::SUCCESS;

	}
	else
	{
		/* code */
	}
	

}

void Neuropixels1_v3::initialize()
{

	errorCode = Neuropixels::openProbe(basestation->slot, headstage->port, dock);
	std::cout << "openProbe: slot: " << basestation->slot << " port: " << headstage->port << " dock: " << dock << " errorCode: " << errorCode << std::endl;

	if (errorCode == np::SUCCESS)
	{
		std::cout << "Configuring probe..." << std::endl;
		errorCode = Neuropixels::setOPMODE(basestation->slot, headstage->port, dock, Neuropixels::RECORDING);
		errorCode = Neuropixels::setHSLed(basestation->slot, headstage->port, false);

		calibrate();
		ap_timestamp = 0;
		lfp_timestamp = 0;
		eventCode = 0;
		setStatus(ProbeStatus::CONNECTED);
	}
}


void Neuropixels1_v3::calibrate()
{
	File baseDirectory = File::getSpecialLocation(File::currentExecutableFile).getParentDirectory();
	File calibrationDirectory = baseDirectory.getChildFile("CalibrationInfo");
	File probeDirectory = calibrationDirectory.getChildFile(String(info.serial_number));


	if (!probeDirectory.exists())
	{
		// check alternate location
		baseDirectory = CoreServices::getSavedStateDirectory();
		calibrationDirectory = baseDirectory.getChildFile("CalibrationInfo");
		probeDirectory = calibrationDirectory.getChildFile(String(info.serial_number));
	}

	if (!probeDirectory.exists())
	{
		// show popup notification window
		String message = "Missing calibration files for probe serial number " + String(info.serial_number) + ". ADC and Gain calibration files must be located in 'CalibrationInfo\\<serial_number>' folder in the directory where the Open Ephys GUI was launched. The GUI will proceed without calibration.";
		AlertWindow::showMessageBox(AlertWindow::AlertIconType::WarningIcon, "Calibration files missing", message, "OK");
		return;
	}

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

void Neuropixels1_v3::selectElectrodes(ProbeSettings settings, bool shouldWriteConfiguration)
{

	Neuropixels::NP_ErrorCode ec;

	if (settings.selectedChannel.size() > 0)
	{
		for (int ch = 0; ch < settings.selectedChannel.size(); ch++)
		{

			ec = Neuropixels::selectElectrode(basestation->slot,
				headstage->port,
				dock,
				settings.selectedChannel[ch],
				settings.selectedShank[ch],
				availableBanks.indexOf(settings.selectedBank[ch]));

		}

		std::cout << "Updating electrode settings for"
			<< " slot: " << basestation->slot
			<< " port: " << headstage->port << std::endl;

		if (shouldWriteConfiguration)
		{
			ec = Neuropixels::writeProbeConfiguration(basestation->slot, headstage->port, dock, false);

			if (!ec == np::SUCCESS)
				std::cout << "Failed to write channel config " << std::endl;
			else
				std::cout << "Successfully wrote channel config " << std::endl;
		}
	}

}

void Neuropixels1_v3::setApFilterState(bool filterIsOn, bool shouldWriteConfiguration)
{
	if (apFilterState != filterIsOn)
	{
		for (int channel = 0; channel < 384; channel++)
			np::setAPCornerFrequency(basestation->slot,
				headstage->port,
				dock,
				!filterIsOn); // true if disabled

		if (shouldWriteConfiguration)
			errorCode = Neuropixels::writeProbeConfiguration(basestation->slot, headstage->port, dock, false);

		std::cout << "Wrote filter " << int(filterIsOn) << " with error code " << errorCode << std::endl;

		apFilterState = filterIsOn;
	}
	
}

void Neuropixels1_v3::setAllGains(int apGain, int lfpGain, bool shouldWriteConfiguration)
{
	if (apGain != apGainIndex || lfpGain != lfpGainIndex)
	{
		for (int channel = 0; channel < 384; channel++)
		{
			Neuropixels::setGain(basestation->slot, headstage->port, dock,
				channel,
				apGain,
				lfpGain);
		}

		if (shouldWriteConfiguration)
			errorCode = Neuropixels::writeProbeConfiguration(basestation->slot, headstage->port, dock, false);

		std::cout << "Wrote gain " << int(apGain) << ", " << int(lfpGain) << " with error code " << errorCode << std::endl;

		apGainIndex = apGain;
		lfpGainIndex = lfpGain;
	}
	
}


void Neuropixels1_v3::setAllReferences(int refIndex, bool shouldWriteConfiguration)
{

	if (refIndex != referenceIndex)
	{
		Neuropixels::channelreference_t refId;
		uint8_t refElectrodeBank = 0;

		switch (referenceIndex)
		{
		case 0:
			refId = Neuropixels::EXT_REF;
			break;
		case 1:
			refId = Neuropixels::TIP_REF;
			break;
		case 2:
			refId = Neuropixels::INT_REF;
			break;
		case 3:
			refId = Neuropixels::INT_REF;
			refElectrodeBank = 1;
			break;
		case 4:
			refId = Neuropixels::INT_REF;
			refElectrodeBank = 2;
			break;
		default:
			refId = Neuropixels::EXT_REF;
		}

		for (int channel = 0; channel < 384; channel++)
			Neuropixels::setReference(basestation->slot, headstage->port, dock, 0, channel, refId, refElectrodeBank);

		if (shouldWriteConfiguration)
			errorCode = Neuropixels::writeProbeConfiguration(basestation->slot, headstage->port, dock, false);

		std::cout << "Wrote reference " << int(refId) << ", " << int(refElectrodeBank) << " with error code " << errorCode << std::endl;

		referenceIndex = refIndex;
	}


}

void Neuropixels1_v3::writeConfiguration()
{
	errorCode = Neuropixels::writeProbeConfiguration(basestation->slot, headstage->port, dock, false);
}

void Neuropixels1_v3::startAcquisition()
{
	ap_timestamp = 0;
	lfp_timestamp = 0;
	//std::cout << "... and clearing buffers" << std::endl;
	apBuffer->clear();
	lfpBuffer->clear();
	std::cout << "  Starting thread." << std::endl;
	startThread();
}

void Neuropixels1_v3::stopAcquisition()
{
	stopThread(1000);
}

void Neuropixels1_v3::run()
{

	//std::cout << "Thread running." << std::endl;

	while (!threadShouldExit())
	{

		int count = SAMPLECOUNT;

		errorCode = Neuropixels::readElectrodeData(
			basestation->slot,
			headstage->port,
			dock,
			&packet[0],
			&count,
			count);

		if (errorCode == np::SUCCESS &&
			count > 0)
		{
			float apSamples[385];
			float lfpSamples[385];

			for (int packetNum = 0; packetNum < count; packetNum++)
			{
				for (int i = 0; i < 12; i++)
				{
					eventCode = packet[packetNum].Status[i] >> 6; // AUX_IO<0:13>

					uint32_t npx_timestamp = packet[packetNum].timestamp[i];

					for (int j = 0; j < 384; j++)
					{

						apSamples[j] = float(packet[packetNum].apData[i][j]) * 1.2f / 1024.0f * 1000000.0f / availableApGains[apGainIndex]; // convert to microvolts

						if (i == 0)
							lfpSamples[j] = float(packet[packetNum].lfpData[j]) * 1.2f / 1024.0f * 1000000.0f / availableLfpGains[lfpGainIndex]; // convert to microvolts
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
				lfp_timestamp += 1;

				if (sendSync)
					lfpSamples[384] = (float)eventCode;

				lfpBuffer->addToBuffer(lfpSamples, &lfp_timestamp, &eventCode, 1);

			}

		}
		else if (errorCode != np::SUCCESS)
		{
			std::cout << "Error code: " << errorCode << "for Basestation " << int(basestation->slot) << ", probe " << int(headstage->port) << std::endl;
		}
	}

}
