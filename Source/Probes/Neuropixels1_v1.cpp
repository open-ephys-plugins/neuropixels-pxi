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

#include "Neuropixels1_v1.h"
#include "Geometry.h"

#include "../Headstages/Headstage1_v1.h"

#define MAXLEN 50

void Neuropixels1_v1::getInfo()
{
	errorCode = np::readId(basestation->slot_c, headstage->port_c, &info.serial_number);

	char pn[MAXLEN];
	errorCode = np::readProbePN(basestation->slot_c, headstage->port_c, pn, MAXLEN);

	info.part_number = String(pn);
}

Neuropixels1_v1::Neuropixels1_v1(Basestation* bs, Headstage* hs, Flex* fl) : Probe(bs, hs, fl, 0)
{

	getInfo();

	setStatus(SourceStatus::DISCONNECTED);

	Geometry::forPartNumber(info.part_number, electrodeMetadata, probeMetadata);

	name = probeMetadata.name;
	type = probeMetadata.type;

	settings.probe = this;

	settings.availableBanks = probeMetadata.availableBanks; 
	
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
	//settings.availableReferences.add("192");
	//settings.availableReferences.add("576");
	//settings.availableReferences.add("960");

	shankOutline.startNewSubPath(27, 31);
	shankOutline.lineTo(27, 514);
	shankOutline.lineTo(27 + 5, 522);
	shankOutline.lineTo(27 + 10, 514);
	shankOutline.lineTo(27 + 10, 31);
	shankOutline.closeSubPath();

	open();

}

bool Neuropixels1_v1::open()
{
	errorCode = np::openProbe(basestation->slot_c, headstage->port_c);
	LOGD("openProbe: slot: ", (int) basestation->slot_c, " port: ", (int) headstage->port_c, " errorCode: ", errorCode);
	
	ap_timestamp = 0;
	lfp_timestamp = 0;
	eventCode = 0;

	apView = new ActivityView(384, 3000);
	lfpView = new ActivityView(384, 250);

	return true;

}

bool Neuropixels1_v1::close()
{
	errorCode = np::close(basestation->slot_c, headstage->port_c);
	LOGD("close: slot: ", (int) basestation->slot_c, " port: ", (int) headstage->port_c, " errorCode: ", errorCode);

	return errorCode == np::SUCCESS;
}

void Neuropixels1_v1::initialize(bool signalChainIsLoading)
{

	errorCode = np::init(basestation->slot_c, headstage->port_c);
	LOGD("init: slot: ", (int) basestation->slot_c, " port: ", (int) headstage->port_c, " errorCode: ", errorCode);

	errorCode = np::setOPMODE(basestation->slot_c, headstage->port_c, np::RECORDING);
	LOGD("setOPMODE: slot: ", (int) basestation->slot_c, " port: ", (int) headstage->port_c, " errorCode: ", errorCode);

	errorCode = np::setHSLed(basestation->slot_c, headstage->port_c, false);
	LOGD("setHSLed: slot: ", (int) basestation->slot_c, " port: ", (int) headstage->port_c, " errorCode: ", errorCode);

}


void Neuropixels1_v1::calibrate()
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
		
	errorCode = np::setADCCalibration(basestation->slot_c, headstage->port_c, adcFile.toRawUTF8());

	if (errorCode == 0) { LOGD("Successful ADC calibration."); }
	else { LOGD("Unsuccessful ADC calibration, failed with error code: ", errorCode); }

	LOGD("Gain file: ", gainFile);
		
	errorCode = np::setGainCalibration(basestation->slot_c, headstage->port_c, gainFile.toRawUTF8());

	if (errorCode == 0) { LOGD("Successful gain calibration."); }
	else { LOGD("Unsuccessful gain calibration, failed with error code: ", errorCode); }

	errorCode = np::writeProbeConfiguration(basestation->slot_c, headstage->port_c, false);

	if (!errorCode == np::SUCCESS) { LOGD("Failed to write probe config w/ error code: ", errorCode); }
	else { LOGD("Successfully wrote probe config "); }

}

void Neuropixels1_v1::selectElectrodes()
{
	if (settings.selectedChannel.size() == 0)
		return;

	for (int ch = 0; ch < 384; ch++)
	{
		if (ch != 191)
			errorCode = np::selectElectrode(basestation->slot_c, headstage->port_c, ch, 0xFF);
	}

	for (int ch = 0; ch < settings.selectedChannel.size(); ch++)
	{

		errorCode = np::selectElectrode(basestation->slot_c,
			headstage->port_c,
			settings.selectedChannel[ch],
			settings.availableBanks.indexOf(settings.selectedBank[ch]));

	}

	LOGD("Updating electrode settings for slot: ", basestation->slot, " port: ", headstage->port, " dock: ", dock);

}

void Neuropixels1_v1::setApFilterState()
{

	for (int channel = 0; channel < 384; channel++)
		np::setAPCornerFrequency(basestation->slot_c, headstage->port_c, channel, !settings.apFilterState); // true if disabled

}

void Neuropixels1_v1::setAllGains()
{

	for (int channel = 0; channel < 384; channel++)
	{
		np::setGain(basestation->slot_c, headstage->port_c,
			channel,
			(unsigned char) settings.apGainIndex,
			(unsigned char) settings.lfpGainIndex);
	}

}


void Neuropixels1_v1::setAllReferences()
{

	np::channelreference_t refId;
	uint8_t refElectrodeBank = 0;

	switch (settings.referenceIndex)
	{
	case 0:
		refId = np::EXT_REF;
		break;
	case 1:
		refId = np::TIP_REF;
		break;
	case 2:
		refId = np::INT_REF;
		break;
	case 3:
		refId = np::INT_REF;
		refElectrodeBank = 1;
		break;
	case 4:
		refId = np::INT_REF;
		refElectrodeBank = 2;
		break;
	default:
		refId = np::EXT_REF;
	}

	for (int channel = 0; channel < 384; channel++)
		np::setReference(basestation->slot_c, headstage->port_c, channel, refId, refElectrodeBank);

}

void Neuropixels1_v1::writeConfiguration()
{

	LOGD("************WRITE PROBE CONFIGURATION****************");
	LOGD("AP Gain: ", settings.availableApGains[settings.apGainIndex]);
	LOGD("LFP Gain: ", settings.availableLfpGains[settings.lfpGainIndex]);
	LOGD("REF: ", settings.availableReferences[settings.referenceIndex]);
	LOGD("FILTER: ", settings.apFilterState);

	errorCode = np::writeProbeConfiguration(basestation->slot_c, headstage->port_c, false);
}

void Neuropixels1_v1::startAcquisition()
{
	ap_timestamp = 0;
	lfp_timestamp = 0;

	apBuffer->clear();
	lfpBuffer->clear();

	apView->reset();
	lfpView->reset();

	last_npx_timestamp = 0;
	passedOneSecond = false;

	SKIP = sendSync ? 385 : 384;

	LOGD("  Starting thread.");
	startThread();
}

void Neuropixels1_v1::stopAcquisition()
{
	LOGC("Probe stopping thread.");
	signalThreadShouldExit();
}

void Neuropixels1_v1::run()
{

	while (!threadShouldExit())
	{
		
		size_t count = MAXPACKETS;

		errorCode = readElectrodeData(
			basestation->slot,
			headstage->port,
			&packet[0],
			&count,
			count);

		if (errorCode == np::SUCCESS &&
			count > 0)
		{

			for (int packetNum = 0; packetNum < count; packetNum++)
			{
				for (int i = 0; i < 12; i++)
				{
					eventCode = packet[packetNum].Status[i] >> 6;

					uint32_t npx_timestamp = packet[packetNum].timestamp[i];

					uint32_t timestamp_jump = npx_timestamp - last_npx_timestamp;

					if (timestamp_jump > MAX_ALLOWABLE_TIMESTAMP_JUMP)
					{
						if (passedOneSecond && timestamp_jump < MAX_HEADSTAGE_CLK_SAMPLE)
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
		else if (errorCode != np::SUCCESS)
		{
			LOGD("readPackets error code: ", errorCode, " for Basestation ", int(basestation->slot), ", probe ", int(headstage->port));
		}

		if (ap_timestamp % 30000 == 0)
			passedOneSecond = true;

		size_t packetsAvailable;
		size_t headroom;

		np::getElectrodeDataFifoState(
			basestation->slot,
			headstage->port,
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

bool Neuropixels1_v1::runBist(BIST bistType)
{

	signed char slot_c = (signed char) basestation->slot;
	signed char port_c = (signed char) headstage->port;

	bool returnValue = false;

	close();
	open();

	switch (bistType)
	{
	case BIST::SIGNAL:
	{
		np::NP_ErrorCode errorCode = np::bistSignal(slot_c, port_c, &returnValue, stats);
		break;
	}
	case BIST::NOISE:
	{
		if (np::bistNoise(slot_c, port_c) == np::SUCCESS)
			returnValue = true;
		break;
	}
	case BIST::PSB:
	{
		if (np::bistPSB(slot_c, port_c) == np::SUCCESS)
			returnValue = true;
		break;
	}
	case BIST::SR:
	{
		if (np::bistSR(slot_c, port_c) == np::SUCCESS)
			returnValue = true;
		break;
	}
	case BIST::EEPROM:
	{
		if (np::bistEEPROM(slot_c, port_c) == np::SUCCESS)
			returnValue = true;
		break;
	}
	case BIST::I2C:
	{
		if (np::bistI2CMM(slot_c, port_c) == np::SUCCESS)
			returnValue = true;
		break;
	}
	case BIST::SERDES:
	{
		unsigned char errors;
		np::bistStartPRBS(slot_c, port_c);
		Sleep(200);
		np::bistStopPRBS(slot_c, port_c, &errors);

		if (errors == 0)
			returnValue = true;
		break;
	}
	case BIST::HB:
	{
		if (np::bistHB(slot_c, port_c) == np::SUCCESS)
			returnValue = true;
		break;
	} case BIST::BS:
	{
		if (np::bistBS(slot_c) == np::SUCCESS)
			returnValue = true;
		break;
	} default:
		CoreServices::sendStatusMessage("Test not found.");
	}

	close();
	open();
	initialize(false);

	np::setTriggerInput(basestation->slot_c, np::TRIGIN_SW);
	np::arm(basestation->slot_c);

	return returnValue;
}
