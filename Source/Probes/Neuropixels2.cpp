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

#include "Neuropixels2.h"
#include "Geometry.h"
#include "../Utils.h"

#define MAXLEN 50

void Neuropixels2::getInfo()
{
	errorCode = Neuropixels::readProbeSN(basestation->slot, headstage->port, dock, &info.serial_number);

	char pn[MAXLEN];
	errorCode = Neuropixels::readProbePN(basestation->slot_c, headstage->port_c, dock, pn, MAXLEN);

	info.part_number = String(pn);
}

Neuropixels2::Neuropixels2(Basestation* bs, Headstage* hs, Flex* fl, int dock) : Probe(bs, hs, fl, dock)
{

	getInfo();

	setStatus(ProbeStatus::DISCONNECTED);

	Geometry::forPartNumber(info.part_number, electrodeMetadata, probeMetadata);

	name = probeMetadata.name;
	type = probeMetadata.type;

	settings.apGainIndex = -1;
	settings.lfpGainIndex = -1;
	settings.referenceIndex = 0;
	settings.apFilterState = false;

	channel_count = 384;
	lfp_sample_rate = 2500.0f; // not used
	ap_sample_rate = 30000.0f;

	for (int i = 0; i < channel_count; i++)
    {
        settings.selectedBank.add(Bank::A);
        settings.selectedChannel.add(i);
        settings.selectedShank.add(0);
    }

	if (probeMetadata.shank_count == 1)
	{
		settings.availableReferences.add("Ext");
		settings.availableReferences.add("Tip");
		settings.availableReferences.add("128");
		settings.availableReferences.add("508");
		settings.availableReferences.add("888");
		settings.availableReferences.add("1252");
	}
	else {
		settings.availableReferences.add("Ext");
		settings.availableReferences.add("1: Tip");
		settings.availableReferences.add("1: 128");
		settings.availableReferences.add("1: 512");
		settings.availableReferences.add("1: 896");
		settings.availableReferences.add("1: 1280");
		settings.availableReferences.add("2: Tip");
		settings.availableReferences.add("2: 128");
		settings.availableReferences.add("2: 512");
		settings.availableReferences.add("2: 896");
		settings.availableReferences.add("2: 1280");
		settings.availableReferences.add("3: Tip");
		settings.availableReferences.add("3: 128");
		settings.availableReferences.add("3: 512");
		settings.availableReferences.add("3: 896");
		settings.availableReferences.add("3: 1280");
		settings.availableReferences.add("4: Tip");
		settings.availableReferences.add("4: 128");
		settings.availableReferences.add("4: 512");
		settings.availableReferences.add("4: 896");
		settings.availableReferences.add("4: 1280");
	}

	settings.availableBanks = { Bank::A,
		Bank::B,
		Bank::C,
		Bank::D,
		Bank::E,
		Bank::F,
		Bank::G,
		Bank::H };
	
	errorCode = Neuropixels::NP_ErrorCode::SUCCESS;

	isCalibrated = false;

}

bool Neuropixels2::open()
{
	errorCode = Neuropixels::openProbe(basestation->slot, headstage->port, dock);
	LOGD("openProbe: slot: ", basestation->slot, " port: ", headstage->port, " dock: ", dock, " errorCode: ", errorCode);
	return errorCode == Neuropixels::SUCCESS;

}

bool Neuropixels2::close()
{
	errorCode = Neuropixels::closeProbe(basestation->slot, headstage->port, dock);
	LOGD("closeProbe: slot: ", basestation->slot, " port: ", headstage->port, " dock: ", dock, " errorCode: ", errorCode);
	return errorCode == Neuropixels::SUCCESS;
}

void Neuropixels2::initialize()
{

	if (open())
	{

		errorCode = Neuropixels::setOPMODE(basestation->slot, headstage->port, dock, Neuropixels::RECORDING);
		errorCode = Neuropixels::setHSLed(basestation->slot, headstage->port, false);

		selectElectrodes();
		setAllReferences();

		calibrate();

		writeConfiguration();

		ap_timestamp = 0;
		lfp_timestamp = 0;
		eventCode = 0;
		setStatus(ProbeStatus::CONNECTED);

		apView = new ActivityView(384, 3000);
		
	}

}


void Neuropixels2::calibrate()
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
	LOGD("ADC file: ", adcFile);

	errorCode = Neuropixels::setADCCalibration(basestation->slot, headstage->port, adcFile.toRawUTF8());

	if (errorCode == 0) { LOGD("Successful ADC calibration."); }
	else { LOGD("Unsuccessful ADC calibration, failed with error code: ", errorCode); }

	LOGD("Gain file: ", gainFile);

	errorCode = Neuropixels::setGainCalibration(basestation->slot, headstage->port, dock, gainFile.toRawUTF8());

	if (errorCode == 0) { LOGD("Successful gain calibration."); }
	else { LOGD("Unsuccessful gain calibration, failed with error code: ", errorCode); }

	errorCode = Neuropixels::writeProbeConfiguration(basestation->slot, headstage->port, dock, false);

	if (!errorCode == Neuropixels::SUCCESS) { LOGD("Failed to write probe config w/ error code: ", errorCode); }
	else { LOGD("Successfully wrote probe config "); }

}

void Neuropixels2::selectElectrodes()
{

	Neuropixels::NP_ErrorCode ec;

	if (settings.selectedBank.size() == 0)
		return;

	for (int ch = 0; ch < settings.selectedChannel.size(); ch++)
	{

		ec = Neuropixels::selectElectrode(basestation->slot,
			headstage->port,
			dock,
			settings.selectedChannel[ch],
			settings.selectedShank[ch],
			settings.availableBanks.indexOf(settings.selectedBank[ch]));

	}

	LOGD("Updating electrode settings for slot: ", basestation->slot, " port: ", headstage->port, " dock: ", dock);

}

void Neuropixels2::setApFilterState()
{
	// no filter cut available
}

void Neuropixels2::setAllGains()
{
	// no gain available
}


void Neuropixels2::setAllReferences()
{

	Neuropixels::channelreference_t refId;
	int refElectrodeBank = 0;
	int shank = 0;

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
	case 5:
		refId = Neuropixels::INT_REF;
		refElectrodeBank = 3;
		break;

	case 6:
		refId = Neuropixels::TIP_REF;
		shank = 1;
		break;
	case 7:
		refId = Neuropixels::INT_REF;
		shank = 1;
		break;
	case 8:
		refId = Neuropixels::INT_REF;
		shank = 1;
		refElectrodeBank = 1;
		break;
	case 9:
		refId = Neuropixels::INT_REF;
		shank = 1;
		refElectrodeBank = 2;
		break;
	case 10:
		refId = Neuropixels::INT_REF;
		shank = 1;
		refElectrodeBank = 3;
		break;


	case 11:
		refId = Neuropixels::TIP_REF;
		shank = 2;
		break;
	case 12:
		refId = Neuropixels::INT_REF;
		shank = 2;
		break;
	case 13:
		refId = Neuropixels::INT_REF;
		shank = 2;
		refElectrodeBank = 1;
		break;
	case 14:
		refId = Neuropixels::INT_REF;
		shank = 2;
		refElectrodeBank = 2;
		break;
	case 15:
		refId = Neuropixels::INT_REF;
		shank = 2;
		refElectrodeBank = 3;
		break;

	case 16:
		refId = Neuropixels::TIP_REF;
		shank = 3;
		break;
	case 17:
		refId = Neuropixels::INT_REF;
		shank = 3;
		break;
	case 18:
		refId = Neuropixels::INT_REF;
		refElectrodeBank = 1;
		shank = 3;
		break;
	case 19:
		refId = Neuropixels::INT_REF;
		refElectrodeBank = 2;
		shank = 3;
		break;
	case 20:
		refId = Neuropixels::INT_REF;
		refElectrodeBank = 3;
		shank = 3;
		break;


	default:
		refId = Neuropixels::EXT_REF;
	}

	for (int channel = 0; channel < channel_count; channel++)
		Neuropixels::setReference(basestation->slot, 
									headstage->port, 
									dock,
									channel,
									shank, 
									refId, 
									refElectrodeBank);

}

void Neuropixels2::writeConfiguration()
{
	errorCode = Neuropixels::writeProbeConfiguration(basestation->slot, headstage->port, dock, false);
}

void Neuropixels2::startAcquisition()
{
	ap_timestamp = 0;

	last_npx_timestamp = 0;
	passedOneSecond = false;

	apBuffer->clear();

	apView->reset();

	LOGD("  Starting thread.");
	startThread();
}

void Neuropixels2::stopAcquisition()
{
	stopThread(1000);
}

void Neuropixels2::run()
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

		if (errorCode == Neuropixels::SUCCESS && count > 0)
		{
			float apSamples[385];

			for (int packetNum = 0; packetNum < count; packetNum++)
			{

				eventCode = packetInfo[packetNum].Status >> 6; // AUX_IO<0:13>

				uint32_t npx_timestamp = packetInfo[packetNum].Timestamp;

				if ((npx_timestamp - last_npx_timestamp) > 4)
				{
					if (passedOneSecond)
					{
						LOGD("NPX TIMESTAMP JUMP: ", npx_timestamp - last_npx_timestamp,
							", expected 3 or 4...Possible data loss on slot ",
							int(basestation->slot_c), ", probe ", int(headstage->port_c),
							" at sample number ", ap_timestamp);
					}
				}

				last_npx_timestamp = npx_timestamp;

				for (int j = 0; j < 384; j++)
				{
					apSamples[j] = float(data[packetNum * 384 + j]) * 1.0f / 16384.0f * 1000000.0f / 80.0f; // convert to microvolts
					apView->addSample(apSamples[j], j);
				}

				ap_timestamp += 1;

				if (sendSync)
					apSamples[384] = (float) eventCode;

				apBuffer->addToBuffer(apSamples, &ap_timestamp, &eventCode, 1);

				if (ap_timestamp % 30000 == 0)
				{
					passedOneSecond = true;

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

		}
		else if (errorCode != Neuropixels::SUCCESS)
		{
			LOGD("readPackets error code: ", errorCode, " for Basestation ", int(basestation->slot), ", probe ", int(headstage->port));
		}
	}

}

bool Neuropixels2::runBist(BIST bistType)
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

	return returnValue;
}
