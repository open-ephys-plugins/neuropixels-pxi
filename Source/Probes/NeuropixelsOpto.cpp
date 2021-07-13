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

#include "NeuropixelsOpto.h"
#include "Geometry.h"
#include "../Utils.h"

#define MAXLEN 50

void NeuropixelsOpto::getInfo()
{
	errorCode = Neuropixels::readProbeSN(basestation->slot, headstage->port, dock, &info.serial_number);

	char pn[MAXLEN];
	errorCode = Neuropixels::readProbePN(basestation->slot, headstage->port, dock, pn, MAXLEN);

	info.part_number = String(pn);
}

NeuropixelsOpto::NeuropixelsOpto(Basestation* bs, Headstage* hs, Flex* fl) : Probe(bs, hs, fl, 1)
{
	getInfo();

	setStatus(SourceStatus::DISCONNECTED);

	Geometry::forPartNumber(info.part_number, electrodeMetadata, probeMetadata);

	if (Geometry::forPartNumber(info.part_number, electrodeMetadata, probeMetadata))
	{

		name = probeMetadata.name;
		type = probeMetadata.type;

		settings.apGainIndex = 3;
		settings.lfpGainIndex = 2;
		settings.referenceIndex = 0;
		settings.apFilterState = true;

		channel_count = 384;
		lfp_sample_rate = 2500.0f;
		ap_sample_rate = 30000.0f;
		
		for (int i = 0; i < channel_count; i++)
		{
			settings.selectedBank.add(Bank::A);
			settings.selectedChannel.add(i);
			settings.selectedShank.add(0);
		}

		settings.availableApGains.add(50.0f);
		settings.availableApGains.add(125.0f);
		settings.availableApGains.add(250.0f);
		settings.availableApGains.add(500.0f);
		settings.availableApGains.add(1000.0f);
		settings.availableApGains.add(1500.0f);
		settings.availableApGains.add(2000.0f);
		settings.availableApGains.add(3000.0f);

		settings.availableLfpGains.add(50.0f);
		settings.availableLfpGains.add(125.0f);
		settings.availableLfpGains.add(250.0f);
		settings.availableLfpGains.add(500.0f);
		settings.availableLfpGains.add(1000.0f);
		settings.availableLfpGains.add(1500.0f);
		settings.availableLfpGains.add(2000.0f);
		settings.availableLfpGains.add(3000.0f);

		settings.availableReferences.add("Ext");
		settings.availableReferences.add("Tip");
		settings.availableReferences.add("192");
		settings.availableReferences.add("576");
		settings.availableReferences.add("960");

		settings.availableBanks = { Bank::A,
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

	isCalibrated = false;
	

}

bool NeuropixelsOpto::open()
{
	errorCode = Neuropixels::openProbe(basestation->slot, headstage->port, dock);
	LOGD("openProbe: slot: ", basestation->slot, " port: ", headstage->port, " dock: ", dock, " errorCode: ", errorCode);

	return errorCode == Neuropixels::SUCCESS;

}

bool NeuropixelsOpto::close()
{
	errorCode = Neuropixels::closeProbe(basestation->slot, headstage->port, dock);
	LOGD("closeProbe: slot: ", basestation->slot, " port: ", headstage->port, " dock: ", dock, " errorCode: ", errorCode);

	return errorCode == Neuropixels::SUCCESS;
}

void NeuropixelsOpto::initialize()
{

	if (open())
	{
		
		errorCode = Neuropixels::init(basestation->slot, headstage->port, dock);
		LOGD("Neuropixels::init: errorCode: ", errorCode);

		errorCode = Neuropixels::setHSLed(basestation->slot, headstage->port, false);
		LOGDD("Neuropixels::setHSLed: errorCode: ", errorCode);

		selectElectrodes();
		setAllReferences();
		setAllGains();
		setApFilterState();

		calibrate();

		writeConfiguration();

		ap_timestamp = 0;
		lfp_timestamp = 0;
		eventCode = 0;

		setStatus(SourceStatus::CONNECTED);

		apView = new ActivityView(384, 3000);
		lfpView = new ActivityView(384, 250);
	}
}


void NeuropixelsOpto::calibrate()
{

	LOGD("Calibrating probe...");

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

		if (!calibrationWarningShown)
		{
			// show popup notification window
			String message = "Missing calibration files for probe serial number " + String(info.serial_number);
			message += ". ADC and Gain calibration files must be located in 'CalibrationInfo\\<serial_number>' folder in the directory where the Open Ephys GUI was launched.";
			message += "The GUI will proceed without calibration.";
			message += "The plugin must be deleted and re-inserted once calibration files have been added";

			AlertWindow::showMessageBox(AlertWindow::AlertIconType::WarningIcon, "Calibration files missing", message, "OK");

			calibrationWarningShown = true;
		}

		return;
	}

	String adcFile = probeDirectory.getChildFile(String(info.serial_number) + "_ADCCalibration.csv").getFullPathName();
	String gainFile = probeDirectory.getChildFile(String(info.serial_number) + "_gainCalValues.csv").getFullPathName();
	String opticalFile = probeDirectory.getChildFile(String(info.serial_number) + "opticalCalValues.csv").getFullPathName();

	LOGDD("ADC file: ", adcFile);

	errorCode = Neuropixels::setADCCalibration(basestation->slot, headstage->port, adcFile.toRawUTF8());

	if (errorCode == 0) { LOGD("Successful ADC calibration."); }
	else { LOGD("!!! Unsuccessful ADC calibration, failed with error code: ", errorCode); return; }

	LOGDD("Gain file: ", gainFile);

	errorCode = Neuropixels::setGainCalibration(basestation->slot, headstage->port, dock, gainFile.toRawUTF8());

	if (errorCode == 0) { LOGD("Successful gain calibration."); }
	else { LOGD("!!! Unsuccessful gain calibration, failed with error code: ", errorCode); return; }

	LOGDD("Optical calibration file: ", adcFile);

	errorCode = Neuropixels::np_setOpticalCalibration(basestation->slot, headstage->port, dock, opticalFile.toRawUTF8());

	if (errorCode == 0) { LOGD("Successful optical calibration."); }
	else { LOGD("!!! Unsuccessful optical calibration, failed with error code: ", errorCode); return; }

	isCalibrated = true;

}

void NeuropixelsOpto::setEmissionSite(Neuropixels::wavelength_t wavelength, int site)
{
	errorCode = Neuropixels::np_setEmissionSite(basestation->slot, headstage->port, dock, wavelength, site);

	if (errorCode == 0) { LOGD("Selected site ", site); }
	else { LOGD("!!! Unsuccessful site selection, failed with error code: ", errorCode); return; }

}

void NeuropixelsOpto::getGain()
{
	int apGainIndex;
	int lfpGainIndex;

	Neuropixels::getGain(basestation->slot, headstage->port, dock, 32, &apGainIndex, &lfpGainIndex);

	LOGD("Current settings for probe on slot: ", basestation->slot,
		" port: ", headstage->port,
		" dock: ", dock,
		" AP=", settings.availableApGains[apGainIndex],
		" LFP=", settings.availableLfpGains[lfpGainIndex],
		" REF=", settings.availableReferences[settings.referenceIndex]);
}

void NeuropixelsOpto::selectElectrodes()
{

	Neuropixels::NP_ErrorCode ec;

	if (settings.selectedChannel.size() > 0)
	{
		for (int ch = 0; ch < settings.selectedChannel.size(); ch++)
		{

			//LOGD("Setting probe: ", headstage->port, " ch: ", settings.selectedChannel[ch], " to bank: ", settings.availableBanks.indexOf(settings.selectedBank[ch]));

			ec = Neuropixels::selectElectrode(basestation->slot,
				headstage->port,
				dock,
				settings.selectedChannel[ch],
				settings.selectedShank[ch],
				settings.availableBanks.indexOf(settings.selectedBank[ch]));

		}
	}

}

void NeuropixelsOpto::setApFilterState()
{

	for (int channel = 0; channel < 384; channel++)
		Neuropixels::setAPCornerFrequency(basestation->slot,
			headstage->port,
			dock,
			channel,
			!settings.apFilterState); // true if disabled
	
}

void NeuropixelsOpto::setAllGains()
{

	LOGDD("Setting gain AP=", settings.apGainIndex, " LFP=", settings.lfpGainIndex);

	for (int channel = 0; channel < 384; channel++)
	{
		Neuropixels::setGain(basestation->slot, headstage->port, dock,
			channel,
			settings.apGainIndex,
			settings.lfpGainIndex);
	}
	
}


void NeuropixelsOpto::setAllReferences()
{

	Neuropixels::channelreference_t refId;
	int refElectrodeBank = 0;

	switch (settings.referenceIndex)
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
		Neuropixels::setReference(basestation->slot, headstage->port, dock, channel, 0, refId, refElectrodeBank);

}

void NeuropixelsOpto::writeConfiguration()
{

	errorCode = Neuropixels::writeProbeConfiguration(basestation->slot, headstage->port, dock, false);

	if (errorCode == Neuropixels::SUCCESS)
	{
		LOGD("Succesfully wrote probe configuration");
		getGain();
	}
	else
	{
		LOGD("!!! FAILED TO WRITE PROBE CONFIGURATION !!! Slot: ", basestation->slot, " port: ", headstage->port, " error code: ", errorCode);
	}

}

void NeuropixelsOpto::startAcquisition()
{
	ap_timestamp = 0;
	lfp_timestamp = 0;
	//std::cout << "... and clearing buffers" << std::endl;
	apBuffer->clear();
	lfpBuffer->clear();

	apView->reset();
	lfpView->reset();
	

	LOGD("  Starting thread.");
	startThread();


}

void NeuropixelsOpto::stopAcquisition()
{
	stopThread(1000);
}

void NeuropixelsOpto::run()
{

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

		if (errorCode == Neuropixels::SUCCESS &&
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

						apSamples[j] = float(packet[packetNum].apData[i][j]) * 1.2f / 1024.0f * 1000000.0f / settings.availableApGains[settings.apGainIndex]; // convert to microvolts

						apView->addSample(apSamples[j], j);

						if (i == 0)
						{
							lfpSamples[j] = float(packet[packetNum].lfpData[j]) * 1.2f / 1024.0f * 1000000.0f / settings.availableLfpGains[settings.lfpGainIndex]; // convert to microvolts

							lfpView->addSample(lfpSamples[j], j);
						}
					}

					ap_timestamp += 1;

					if (sendSync)
						apSamples[384] = (float)eventCode;

					apBuffer->addToBuffer(apSamples, &ap_timestamp, &eventCode, 1);

					if (ap_timestamp % 30000 == 0)
					{
						int packetsAvailable;
						int headroom;

						Neuropixels::getElectrodeDataFifoState(
							basestation->slot,
							headstage->port,
							dock,
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
		else if (errorCode != Neuropixels::SUCCESS)
		{
			LOGD("readPackets error code: ", errorCode, " for Basestation ", int(basestation->slot), ", probe ", int(headstage->port));
		}
	}

}

bool NeuropixelsOpto::runBist(BIST bistType)
{

	close();
	open();

	int slot = basestation->slot;
	int port = headstage->port;

	bool returnValue = false;

	switch (bistType)
	{
	case BIST::SIGNAL:
	{
		if (Neuropixels::bistSignal(slot, port, dock) == Neuropixels::SUCCESS)
			returnValue = true;
		break;
	}
	case BIST::NOISE:
	{
		if (Neuropixels::bistNoise(slot, port, dock) == Neuropixels::SUCCESS)
			returnValue = true;
		break;
	}
	case BIST::PSB:
	{
		if (Neuropixels::bistPSB(slot, port, dock) == Neuropixels::SUCCESS)
			returnValue = true;
		break;
	}
	case BIST::SR:
	{
		if (Neuropixels::bistSR(slot, port, dock) == Neuropixels::SUCCESS)
			returnValue = true;
		break;
	}
	case BIST::EEPROM:
	{
		if (Neuropixels::bistEEPROM(slot, port) == Neuropixels::SUCCESS)
			returnValue = true;
		break;
	}
	case BIST::I2C:
	{
		if (Neuropixels::bistI2CMM(slot, port, dock) == Neuropixels::SUCCESS)
			returnValue = true;
		break;
	}
	case BIST::SERDES:
	{
		int errors;
		Neuropixels::bistStartPRBS(slot, port);
		Sleep(200);
		Neuropixels::bistStopPRBS(slot, port, &errors);

		if (errors == 0)
			returnValue = true;
		break;
	}
	case BIST::HB:
	{
		if (Neuropixels::bistHB(slot, port, dock) == Neuropixels::SUCCESS)
			returnValue = true;
		break;
	} case BIST::BS:
	{
		if (Neuropixels::bistBS(slot) == Neuropixels::SUCCESS)
			returnValue = true;
		break;
	} default:
		CoreServices::sendStatusMessage("Test not found.");
	}

	close();
	initialize();

	errorCode = Neuropixels::setSWTrigger(slot);
	errorCode = Neuropixels::arm(slot);

	return returnValue;
}
