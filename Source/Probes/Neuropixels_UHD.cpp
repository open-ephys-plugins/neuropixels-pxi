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

#include "Neuropixels_UHD.h"
#include "Geometry.h"

#include "../NeuropixThread.h"

#define MAXLEN 50

void Neuropixels_UHD::getInfo()
{
	errorCode = Neuropixels::readProbeSN(basestation->slot, headstage->port, dock, &info.serial_number);

	char pn[MAXLEN];
	errorCode = Neuropixels::readProbePN(basestation->slot, headstage->port, dock, pn, MAXLEN);

	info.part_number = String(pn);
}

Neuropixels_UHD::Neuropixels_UHD(Basestation* bs, Headstage* hs, Flex* fl) : Probe(bs, hs, fl, 1)
{
	getInfo();

	setStatus(SourceStatus::DISCONNECTED);

	customName.probeSpecific = String(info.serial_number);

	if (Geometry::forPartNumber(info.part_number, electrodeMetadata, probeMetadata))
	{

		name = probeMetadata.name;
		type = probeMetadata.type;
		switchable = probeMetadata.switchable;

		settings.probe = this;
		settings.availableBanks = probeMetadata.availableBanks;

		settings.apGainIndex = 3;
		settings.lfpGainIndex = 2;
		settings.referenceIndex = 0;
		settings.apFilterState = true;

		channel_count = 384;
		lfp_sample_rate = 2500.0f;
		ap_sample_rate = 30000.0f;

		for (int i = 0; i < 16; i++)
			settings.availableElectrodeConfigurations.add("Bank: " + String(i));
		settings.availableElectrodeConfigurations.add("Tip Half");
		settings.availableElectrodeConfigurations.add("Base Half");

		settings.electrodeConfigurationIndex = 1;

		for (int i = 0; i < channel_count; i++)
		{
			settings.selectedBank.add(Bank::A);
			settings.selectedChannel.add(i);
			settings.selectedShank.add(0);
			settings.selectedElectrode.add(i);
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

		open();

	}
	else
	{
		isValid = false;
	}

}

bool Neuropixels_UHD::open()
{
	LOGC("Opening probe...");
	errorCode = Neuropixels::openProbe(basestation->slot, headstage->port, dock);

	LOGC("openProbe: slot: ", basestation->slot, " port: ", headstage->port, " dock: ", dock, " errorCode: ", errorCode);

	ap_timestamp = 0;
	lfp_timestamp = 0;
	eventCode = 0;

	apView = new ActivityView(384, 3000);
	lfpView = new ActivityView(384, 250);


	return errorCode == Neuropixels::SUCCESS;

}

bool Neuropixels_UHD::close()
{
	errorCode = Neuropixels::closeProbe(basestation->slot, headstage->port, dock);
	LOGD("closeProbe: slot: ", basestation->slot, " port: ", headstage->port, " dock: ", dock, " errorCode: ", errorCode);

	return errorCode == Neuropixels::SUCCESS;
}

void Neuropixels_UHD::initialize(bool signalChainIsLoading)
{

	errorCode = Neuropixels::init(basestation->slot, headstage->port, dock);
	LOGD("Neuropixels::init: errorCode: ", errorCode);

	errorCode = Neuropixels::setOPMODE(basestation->slot, headstage->port, dock, Neuropixels::RECORDING);
	LOGD("Neuropixels::setOPMODE: errorCode: ", errorCode);

	errorCode = Neuropixels::setHSLed(basestation->slot, headstage->port, false);
	LOGDD("Neuropixels::setHSLed: errorCode: ", errorCode);

}


void Neuropixels_UHD::calibrate()
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
	LOGDD("ADC file: ", adcFile);

	errorCode = Neuropixels::setADCCalibration(basestation->slot, headstage->port, adcFile.toRawUTF8());

	if (errorCode == 0) { LOGD("Successful ADC calibration."); }
	else { LOGD("!!! Unsuccessful ADC calibration, failed with error code: ", errorCode); return; }

	LOGDD("Gain file: ", gainFile);

	errorCode = Neuropixels::setGainCalibration(basestation->slot, headstage->port, dock, gainFile.toRawUTF8());

	if (errorCode == 0) { LOGD("Successful gain calibration."); }
	else { LOGD("!!! Unsuccessful gain calibration, failed with error code: ", errorCode); return; }

	isCalibrated = true;

}

void Neuropixels_UHD::printSettings()
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

void Neuropixels_UHD::selectElectrodes()
{
	if (settings.electrodeConfigurationIndex >= 0 && switchable) {
		selectElectrodeConfiguration(settings.availableElectrodeConfigurations[settings.electrodeConfigurationIndex]);
	}
}

Array<int> Neuropixels_UHD::selectElectrodeConfiguration(String electrodeConfiguration)
{

	Array<int> returnValue;

	/*
	*           ---------BANK 0--------|---------BANK 1--------
	*         /| 0 | 4 | 8 | 12| 16| 20| 2 | 6 | 10| 14| 18| 22|         |
	* PROBE  / | 2 | 6 | 10| 14| 18| 22| 0 | 4 | 8 | 12| 16| 20| ... x 8 | PROBE
	*  TIP   \ | 3 | 7 | 11| 15| 19| 23| 1 | 5 | 9 | 13| 17| 21|         | BASE
	*         \| 1 | 5 | 9 | 13| 17| 21| 3 | 7 | 11| 15| 19| 23|         |
	*           --------24 GROUPS------|-------24 GROUPS-------
	*/

	int banksPerProbe		= 16;
	int groupsPerBank		= 24;
	int groupsPerBankColumn	= 6;
	int electrodesPerGroup	= 16;

    Neuropixels::NP_ErrorCode ec;

	int index = settings.availableElectrodeConfigurations.indexOf(electrodeConfiguration);

	if (index == -1)
		return returnValue;

	if (index < banksPerProbe)
	{
		LOGC("Selecting column pattern: ALL");
		// select columnar configuration
		ec = Neuropixels::selectColumnPattern(
			basestation->slot,
			headstage->port,
			dock,
			Neuropixels::ALL

		);
	}
	else {
		LOGC("Selecting column pattern: OUTER");
		// select columnar configuration
		ec = Neuropixels::selectColumnPattern(
			basestation->slot,
			headstage->port,
			dock,
			Neuropixels::OUTER

		);
	}
	
	// Select all groups in a particular bank
    if (index < banksPerProbe)
    {
		LOGC("Selecting bank: ", index);

		// Select all groups at this bank index
		for (int group = 0; group < groupsPerBank; group++)
			ec = Neuropixels::selectElectrodeGroup(
				basestation->slot,	// slot
				headstage->port,	// port
				dock,				// dock
				group,				// group number
				index);				// bank index

		return returnValue;
    }

	// Select a subset of groups in multiple banks to span half of the probe
	//   0 = Select groups from middle to tip of probe (Banks 0-7)
	//   1 = Select groups from middle to base of probe (Banks 8-15)
	int verticalConfiguration = index - banksPerProbe;

	int offset; //controls offset of bank index

	if (verticalConfiguration == 0)
	{
		offset = 0;
	}
	else {
		offset = 8;
	}
	// Vertical line in banks 0-7
	for (int bank = offset; bank < offset + 4; bank++) {

		// Direct vs. Group Cross numbering based on bank index
		int start_group = bank % 4 < 2 ? 0 : 1;
		start_group = bank % 2 == 0 ? start_group + 2 : start_group;

		LOGC("Selecting bank: ", bank, ", start group: ", start_group);

		Neuropixels::electrodebanks_t bank1 = (Neuropixels::electrodebanks_t) (1 << bank);
		Neuropixels::electrodebanks_t bank2 = (Neuropixels::electrodebanks_t) (1 << (bank+4));

		for (int group = 0; group < groupsPerBankColumn; group++)
		{
			int G = start_group + group * 4;

			ec = Neuropixels::selectElectrodeGroupMask(
				basestation->slot,			// slot
				headstage->port,			// port
				dock,						// dock
				G,							// group number
				(Neuropixels::electrodebanks_t) (bank1 + bank2));				// bank mask
		}

	}

	return returnValue;

}

void Neuropixels_UHD::setApFilterState()
{

	for (int channel = 0; channel < 384; channel++)
		Neuropixels::setAPCornerFrequency(basestation->slot,
			headstage->port,
			dock,
			channel,
			!settings.apFilterState); // true if disabled

}

void Neuropixels_UHD::setAllGains()
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


void Neuropixels_UHD::setAllReferences()
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

void Neuropixels_UHD::writeConfiguration()
{

	if (basestation->isBusy())
		basestation->waitForThreadToExit();

	errorCode = Neuropixels::writeProbeConfiguration(basestation->slot, headstage->port, dock, false);

	if (errorCode == Neuropixels::SUCCESS)
	{
		LOGD("Succesfully wrote probe configuration");
		printSettings();
	}
	else
	{
		LOGD("!!! FAILED TO WRITE PROBE CONFIGURATION !!! Slot: ", basestation->slot, " port: ", headstage->port, " error code: ", errorCode);
	}

}

void Neuropixels_UHD::startAcquisition()
{
	ap_timestamp = 0;
	lfp_timestamp = 0;

	apBuffer->clear();
	lfpBuffer->clear();

	apView->reset();
	lfpView->reset();

	SKIP = sendSync ? 385 : 384;

	LOGD("  Starting thread.");
	startThread();

}

void Neuropixels_UHD::stopAcquisition()
{
	LOGC("Probe stopping thread.");
	signalThreadShouldExit();
}

void Neuropixels_UHD::run()
{

	while (!threadShouldExit())
	{

		int count = MAXPACKETS;

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

			for (int packetNum = 0; packetNum < count; packetNum++)
			{
				for (int i = 0; i < 12; i++)
				{
					eventCode = packet[packetNum].Status[i] >> 6; // AUX_IO<0:13>

					if (invertSyncLine)
						eventCode = ~eventCode;

					uint32_t npx_timestamp = packet[packetNum].timestamp[i];

					uint32_t timestamp_jump = npx_timestamp - last_npx_timestamp;

					if (timestamp_jump > MAX_ALLOWABLE_TIMESTAMP_JUMP)
					{
						if (passedOneSecond && timestamp_jump < MAX_HEADSTAGE_CLK_SAMPLE)
						{
							String msg = "NPX TIMESTAMP JUMP: " + String(timestamp_jump) +
								", expected 3 or 4...Possible data loss on slot " +
								String(basestation->slot_c) + ", probe " + String(headstage->port_c) +
								" at sample number " + String(ap_timestamp);

							LOGC(msg);

							basestation->neuropixThread->sendBroadcastMessage(msg);
						}
					}

					last_npx_timestamp = npx_timestamp;

					for (int j = 0; j < 384; j++)
					{

						apSamples[j + i * SKIP + packetNum * 12 * SKIP] =
							float(packet[packetNum].apData[i][j]) * 1.2f / 1024.0f * 1000000.0f
							      / settings.availableApGains[settings.apGainIndex]
								  - ap_offsets[j][0]; // convert to microvolts

						apView->addSample(apSamples[j + i * SKIP + packetNum * 12 * SKIP], j);

						if (i == 0)
						{
							lfpSamples[j + packetNum * SKIP] =
								float(packet[packetNum].lfpData[j]) * 1.2f / 1024.0f * 1000000.0f 
								/ settings.availableLfpGains[settings.lfpGainIndex]
								- lfp_offsets[j][0]; // convert to microvolts

							lfpView->addSample(lfpSamples[j + packetNum * SKIP], j);
						}
					}

					ap_timestamps[i + packetNum * 12] = ap_timestamp++;
					event_codes[i + packetNum * 12] = eventCode;

					if (sendSync)
						apSamples[384 + i * SKIP + packetNum * 12 * SKIP] = (float)eventCode;

				}

				lfp_timestamps[packetNum] = lfp_timestamp++;
				lfp_event_codes[packetNum] = eventCode;

				if (sendSync)
					lfpSamples[384 + packetNum * SKIP] = (float)eventCode;

			}

			apBuffer->addToBuffer(apSamples, ap_timestamps, timestamp_s, event_codes, 12 * count);
			lfpBuffer->addToBuffer(lfpSamples, lfp_timestamps, timestamp_s, lfp_event_codes, count);

			if (ap_offsets[0][0] == 0)
			{
				updateOffsets(apSamples, ap_timestamp, true);
				updateOffsets(lfpSamples, lfp_timestamp, false);
			}

		}
		else if (errorCode != Neuropixels::SUCCESS)
		{
			LOGD("readPackets error code: ", errorCode, " for Basestation ", int(basestation->slot), ", probe ", int(headstage->port));
		}

		if (!passedOneSecond)
		{
			if (ap_timestamp > 30000)
				passedOneSecond = true;
		}

		int packetsAvailable;
		int headroom;

		Neuropixels::getElectrodeDataFifoState(
			basestation->slot,
			headstage->port,
			dock,
			&packetsAvailable,
			&headroom);

		fifoFillPercentage = float(packetsAvailable) / float(packetsAvailable + headroom);

		if (packetsAvailable < MAXPACKETS)
		{
			int uSecToWait = (MAXPACKETS - packetsAvailable) * 400;

			std::this_thread::sleep_for(std::chrono::microseconds(uSecToWait));
		}

	}

}

bool Neuropixels_UHD::runBist(BIST bistType)
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
	open();
	initialize(false);

	errorCode = Neuropixels::setSWTrigger(slot);
	errorCode = Neuropixels::arm(slot);

	return returnValue;
}
