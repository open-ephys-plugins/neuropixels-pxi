/*
------------------------------------------------------------------

This file is part of the Open Ephys GUI
Copyright (C) 2023 Allen Institute for Brain Science and Open Ephys

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

#include "Neuropixels_QuadBase.h"
#include "Geometry.h"

#include "../NeuropixThread.h"

#define MAXLEN 50

void Neuropixels_QuadBase::getInfo()
{
	errorCode = Neuropixels::readProbeSN(basestation->slot, headstage->port, dock, &info.serial_number);

	char pn[MAXLEN];
	errorCode = Neuropixels::readProbePN(basestation->slot_c, headstage->port_c, dock, pn, MAXLEN);

	LOGC("   Found probe part number: ", pn);
	LOGC("   Found probe serial number: ", info.serial_number);

	info.part_number = String(pn);
}

Neuropixels_QuadBase::Neuropixels_QuadBase(Basestation* bs, Headstage* hs, Flex* fl, int dock) : Probe(bs, hs, fl, dock)
{
	getInfo();

	setStatus(SourceStatus::DISCONNECTED);

	customName.probeSpecific = String(info.serial_number);

	LOGC("Trying to open probe, slot: ", basestation->slot, " port: ", headstage->port, " dock: ", dock);

	if (Geometry::forPartNumber(info.part_number, electrodeMetadata, probeMetadata))
	{
		name = probeMetadata.name;
		type = probeMetadata.type;

		settings.probe = this;

		settings.availableBanks = probeMetadata.availableBanks;

		settings.apGainIndex = -1;
		settings.lfpGainIndex = -1;
		settings.referenceIndex = 0;
		settings.apFilterState = false;

		channel_count = 384 * 4;
		lfp_sample_rate = 2500.0f; // not used
		ap_sample_rate = 30000.0f;

		for (int i = 0; i < channel_count; i++)
		{
			settings.selectedBank.add(Bank::A);
			settings.selectedChannel.add(electrodeMetadata[i].channel);
			settings.selectedShank.add(0);
			settings.selectedElectrode.add(electrodeMetadata[i].global_index);

		}

		if (probeMetadata.shank_count == 1)
		{
			settings.availableReferences.add("Ext");
			settings.availableReferences.add("Tip");
			
			settings.availableElectrodeConfigurations.add("Bank A");
			settings.availableElectrodeConfigurations.add("Bank B");
			settings.availableElectrodeConfigurations.add("Bank C");
			settings.availableElectrodeConfigurations.add("Bank D");
		}
		else {
			settings.availableReferences.add("Ext");
			settings.availableReferences.add("Tip");
			settings.availableReferences.add("Ground");

			settings.availableElectrodeConfigurations.add("Shank 1 Bank A");
			settings.availableElectrodeConfigurations.add("Shank 1 Bank B");
			settings.availableElectrodeConfigurations.add("Shank 1 Bank C");
			settings.availableElectrodeConfigurations.add("Shank 2 Bank A");
			settings.availableElectrodeConfigurations.add("Shank 2 Bank B");
			settings.availableElectrodeConfigurations.add("Shank 2 Bank C");
			settings.availableElectrodeConfigurations.add("Shank 3 Bank A");
			settings.availableElectrodeConfigurations.add("Shank 3 Bank B");
			settings.availableElectrodeConfigurations.add("Shank 3 Bank C");
			settings.availableElectrodeConfigurations.add("Shank 4 Bank A");
			settings.availableElectrodeConfigurations.add("Shank 4 Bank B");
			settings.availableElectrodeConfigurations.add("Shank 4 Bank C");
			settings.availableElectrodeConfigurations.add("All Shanks 1-96");
			settings.availableElectrodeConfigurations.add("All Shanks 97-192");
			settings.availableElectrodeConfigurations.add("All Shanks 193-288");
			settings.availableElectrodeConfigurations.add("All Shanks 289-384");
			settings.availableElectrodeConfigurations.add("All Shanks 385-480");
			settings.availableElectrodeConfigurations.add("All Shanks 481-576");
			settings.availableElectrodeConfigurations.add("All Shanks 577-672");
			settings.availableElectrodeConfigurations.add("All Shanks 673-768");
			settings.availableElectrodeConfigurations.add("All Shanks 769-864");
			settings.availableElectrodeConfigurations.add("All Shanks 865-960");
			settings.availableElectrodeConfigurations.add("All Shanks 961-1056");
			settings.availableElectrodeConfigurations.add("All Shanks 1057-1152");
			settings.availableElectrodeConfigurations.add("All Shanks 1153-1248");
		}

		open();
	}
	else {
		LOGC("Unable to open probe!");
		isValid = false;
	}

	

}

bool Neuropixels_QuadBase::open()
{
	errorCode = Neuropixels::openProbe(basestation->slot, headstage->port, dock);
	LOGC("openProbe: slot: ", basestation->slot, " port: ", headstage->port, " dock: ", dock, " errorCode: ", errorCode);
	
	ap_timestamp = 0;
	lfp_timestamp = 0;
	eventCode = 0;

	apView = new ActivityView(channel_count, 3000);

	return errorCode == Neuropixels::SUCCESS;

}

bool Neuropixels_QuadBase::close()
{
	errorCode = Neuropixels::closeProbe(basestation->slot, headstage->port, dock);
	LOGD("closeProbe: slot: ", basestation->slot, " port: ", headstage->port, " dock: ", dock, " errorCode: ", errorCode);

	return errorCode == Neuropixels::SUCCESS;
}

void Neuropixels_QuadBase::initialize(bool signalChainIsLoading)
{
	errorCode = Neuropixels::init(basestation->slot, headstage->port, dock);
	LOGD("init: slot: ", basestation->slot, " port: ", headstage->port, " dock: ", dock, " errorCode: ", errorCode);

}


void Neuropixels_QuadBase::calibrate()
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

	String gainFile = probeDirectory.getChildFile(String(info.serial_number) + "_gainCalValues.csv").getFullPathName();

	LOGD("Gain file: ", gainFile);

	errorCode = Neuropixels::setGainCalibration(basestation->slot, headstage->port, dock, gainFile.toRawUTF8());

	if (errorCode == 0) { LOGD("Successful gain calibration."); }
	else { LOGD("Unsuccessful gain calibration, failed with error code: ", errorCode); }

	errorCode = Neuropixels::writeProbeConfiguration(basestation->slot, headstage->port, dock, false);

	if (!errorCode == Neuropixels::SUCCESS) { LOGD("Failed to write probe config w/ error code: ", errorCode); }
	else { LOGD("Successfully wrote probe config "); }

	errorCode = Neuropixels::np_setHSLed(basestation->slot, headstage->port, false);


}

void Neuropixels_QuadBase::selectElectrodes()
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

	LOGD("Updated electrode settings for slot: ", basestation->slot, " port: ", headstage->port, " dock: ", dock);

}

Array<int> Neuropixels_QuadBase::selectElectrodeConfiguration(String config)
{
	Array<int> selection;

	if (config.equalsIgnoreCase("Bank A"))
	{
		for (int i = 0; i < 384; i++)
			selection.add(i);
	}
	else if (config.equalsIgnoreCase("Bank B"))
	{
		for (int i = 384; i < 768; i++)
			selection.add(i);
	}
	else if (config.equalsIgnoreCase("Bank C"))
	{

		for (int i = 768; i < 1152; i++)
			selection.add(i);

	}
	else if (config.equalsIgnoreCase("Bank D"))
	{

		for (int i = 896; i < 1280; i++)
			selection.add(i);

	}
	else if (config.equalsIgnoreCase("Shank 1 Bank A"))
	{
		for (int i = 0; i < 384; i++)
		{
			selection.add(i);
		}
	}
	else if (config.equalsIgnoreCase("Shank 1 Bank B"))
	{
		for (int i = 384; i < 768; i++)
		{
			selection.add(i);
		}
	}
	else if (config.equalsIgnoreCase("Shank 1 Bank C"))
	{
		for (int i = 768; i < 1152; i++)
		{
			selection.add(i);
		}
	}
	else if (config.equalsIgnoreCase("Shank 2 Bank A"))
	{
		int startElectrode = 1280;

		for (int i = startElectrode; i < startElectrode + 384; i++)
		{
			selection.add(i);
		}
	}
	else if (config.equalsIgnoreCase("Shank 2 Bank B"))
	{
		int startElectrode = 1280 + 384;

		for (int i = startElectrode; i < startElectrode + 384; i++)
		{
			selection.add(i);
		}
	}
	else if (config.equalsIgnoreCase("Shank 2 Bank C"))
	{
		int startElectrode = 1280 + 384 * 2;

		for (int i = startElectrode; i < startElectrode + 384; i++)
		{
			selection.add(i);
		}
	}
	else if (config.equalsIgnoreCase("Shank 3 Bank A"))
	{
		int startElectrode = 1280 * 2;

		for (int i = startElectrode; i < startElectrode + 384; i++)
		{
			selection.add(i);
		}
	}
	else if (config.equalsIgnoreCase("Shank 3 Bank B"))
	{
		int startElectrode = 1280 * 2 + 384;

		for (int i = startElectrode; i < startElectrode + 384; i++)
		{
			selection.add(i);
		}
	}
	else if (config.equalsIgnoreCase("Shank 3 Bank C"))
	{
		int startElectrode = 1280 * 2 + 384 * 2;

		for (int i = startElectrode; i < startElectrode + 384; i++)
		{
			selection.add(i);
		}
	}
	else if (config.equalsIgnoreCase("Shank 4 Bank A"))
	{
		int startElectrode = 1280 * 3;

		for (int i = startElectrode; i < startElectrode + 384; i++)
		{
			selection.add(i);
		}
	}
	else if (config.equalsIgnoreCase("Shank 4 Bank B"))
	{
		int startElectrode = 1280 * 3 + 384;

		for (int i = startElectrode; i < startElectrode + 384; i++)
		{
			selection.add(i);
		}
	}
	else if (config.equalsIgnoreCase("Shank 4 Bank C"))
	{
		int startElectrode = 1280 * 3 + 384 * 2;

		for (int i = startElectrode; i < startElectrode + 384; i++)
		{
			selection.add(i);
		}
	}
	else if (config.equalsIgnoreCase("All Shanks 1-96"))
	{

		int startElectrode = 0;

		for (int shank = 0; shank < 4; shank++)
		{
			for (int i = startElectrode + 1280 * shank; i < startElectrode + 96 + 1280 * shank; i++)
			{
				selection.add(i);
			}
		}
	}
	else if (config.equalsIgnoreCase("All Shanks 97-192"))
	{

		int startElectrode = 96;

		for (int shank = 0; shank < 4; shank++)
		{
			for (int i = startElectrode + 1280 * shank; i < startElectrode + 96 + 1280 * shank; i++)
			{
				selection.add(i);
			}
		}
	}
	else if (config.equalsIgnoreCase("All Shanks 193-288"))
	{

		int startElectrode = 192;

		for (int shank = 0; shank < 4; shank++)
		{
			for (int i = startElectrode + 1280 * shank; i < startElectrode + 96 + 1280 * shank; i++)
			{
				selection.add(i);
			}
		}
	}
	else if (config.equalsIgnoreCase("All Shanks 289-384"))
	{

		int startElectrode = 288;

		for (int shank = 0; shank < 4; shank++)
		{
			for (int i = startElectrode + 1280 * shank; i < startElectrode + 96 + 1280 * shank; i++)
			{
				selection.add(i);
			}
		}
	}
	else if (config.equalsIgnoreCase("All Shanks 385-480"))
	{

		int startElectrode = 384;

		for (int shank = 0; shank < 4; shank++)
		{
			for (int i = startElectrode + 1280 * shank; i < startElectrode + 96 + 1280 * shank; i++)
			{
				selection.add(i);
			}
		}
	}
	else if (config.equalsIgnoreCase("All Shanks 481-576"))
	{

		int startElectrode = 480;

		for (int shank = 0; shank < 4; shank++)
		{
			for (int i = startElectrode + 1280 * shank; i < startElectrode + 96 + 1280 * shank; i++)
			{
				selection.add(i);
			}
		}
	}
	else if (config.equalsIgnoreCase("All Shanks 577-672"))
	{

		int startElectrode = 576;

		for (int shank = 0; shank < 4; shank++)
		{
			for (int i = startElectrode + 1280 * shank; i < startElectrode + 96 + 1280 * shank; i++)
			{
				selection.add(i);
			}
		}
	}
	else if (config.equalsIgnoreCase("All Shanks 673-768"))
	{

		int startElectrode = 672;

		for (int shank = 0; shank < 4; shank++)
		{
			for (int i = startElectrode + 1280 * shank; i < startElectrode + 96 + 1280 * shank; i++)
			{
				selection.add(i);
			}
		}
	}
	else if (config.equalsIgnoreCase("All Shanks 769-864"))
	{

		int startElectrode = 768;

		for (int shank = 0; shank < 4; shank++)
		{
			for (int i = startElectrode + 1280 * shank; i < startElectrode + 96 + 1280 * shank; i++)
			{
				selection.add(i);
			}
		}
	}
	else if (config.equalsIgnoreCase("All Shanks 865-960"))
	{

		int startElectrode = 864;

		for (int shank = 0; shank < 4; shank++)
		{
			for (int i = startElectrode + 1280 * shank; i < startElectrode + 96 + 1280 * shank; i++)
			{
				selection.add(i);
			}
		}
	}
	else if (config.equalsIgnoreCase("All Shanks 961-1056"))
	{

		int startElectrode = 960;

		for (int shank = 0; shank < 4; shank++)
		{
			for (int i = startElectrode + 1280 * shank; i < startElectrode + 96 + 1280 * shank; i++)
			{
				selection.add(i);
			}
		}
	}
	else if (config.equalsIgnoreCase("All Shanks 1057-1152"))
	{

		int startElectrode = 1056;

		for (int shank = 0; shank < 4; shank++)
		{
			for (int i = startElectrode + 1280 * shank; i < startElectrode + 96 + 1280 * shank; i++)
			{
				selection.add(i);
			}
		}
	}
	else if (config.equalsIgnoreCase("All Shanks 1153-1248"))
	{

		int startElectrode = 1152;

		for (int shank = 0; shank < 4; shank++)
		{
			for (int i = startElectrode + 1280 * shank; i < startElectrode + 96 + 1280 * shank; i++)
			{
				selection.add(i);
			}
		}
	}

	return selection;
}


void Neuropixels_QuadBase::setApFilterState()
{
	// no filter cut available
}

void Neuropixels_QuadBase::setAllGains()
{
	// no gain available
}


void Neuropixels_QuadBase::setAllReferences()
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
		shank = 0;
		break;
	case 2:
		refId = Neuropixels::TIP_REF;
		shank = 1;
		break;
	case 3:
		refId = Neuropixels::TIP_REF;
		shank = 2;
		break;
	case 4:
		refId = Neuropixels::TIP_REF;
		shank = 3;
		break;
	case 5:
		refId = Neuropixels::INT_REF;
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

	LOGD("Updated reference for slot: ", basestation->slot, " port: ", headstage->port, " dock: ", dock, " to ", refId);

}

void Neuropixels_QuadBase::writeConfiguration()
{

	errorCode = Neuropixels::writeProbeConfiguration(basestation->slot, headstage->port, dock, false);
}

void Neuropixels_QuadBase::startAcquisition()
{
	ap_timestamp = 0;
	apBuffer->clear();

	apView->reset();

	last_npx_timestamp = 0;
	passedOneSecond = false;

	SKIP = sendSync ? 384 * 4 + 1 : 384 * 4;

	LOGD("  Starting thread.");
	startThread();
}

void Neuropixels_QuadBase::stopAcquisition()
{
	LOGC("Probe stopping thread.");
	signalThreadShouldExit();
}

void Neuropixels_QuadBase::run()
{

	while (!threadShouldExit())
	{
		int channel_count = 384;

		int packet_count = MAXPACKETS;

		for (int base = 0; base < 4; base++) {

			int count = MAXPACKETS;

			errorCode = Neuropixels::readPackets(
				basestation->slot,
				headstage->port,
				dock,
				static_cast<Neuropixels::streamsource_t>(base),
				&packetInfo[0],
				&data[0],
				channel_count,
				packet_count,
				&packet_count);

			if (errorCode == Neuropixels::SUCCESS && packet_count > 0)
			{
				for (int packetNum = 0; packetNum < packet_count; packetNum++)
				{
					if (base == 0)
					{
						eventCode = packetInfo[packetNum].Status >> 6;

						if (invertSyncLine)
							eventCode = ~eventCode;

						uint32_t npx_timestamp = packetInfo[packetNum].Timestamp;

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

						if (sendSync)
							apSamples[4 * channel_count + SKIP * packetNum] = (float)eventCode;

					}

					for (int j = base * 384; j < (base + 1) * channel_count; j++)
					{
						apSamples[j + packetNum * SKIP] =
							float(data[packetNum * channel_count + j]) * 1.0f / 16384.0f * 1000000.0f / 80.0f; // convert to microvolts

						apView->addSample(apSamples[j + packetNum * SKIP], j);

					}

					ap_timestamps[packetNum] = ap_timestamp++;
					event_codes[packetNum] = eventCode;

				}

			}
			else if (errorCode != Neuropixels::SUCCESS)
			{
				LOGD("readPackets error code: ", errorCode, " for Basestation ", int(basestation->slot), ", probe ", int(headstage->port));
			}

		}

		apBuffer->addToBuffer(apSamples, ap_timestamps, timestamp_s, event_codes, packet_count);

		if (!passedOneSecond)
		{
			if (ap_timestamp > 30000)
				passedOneSecond = true;
		}

		int packetsAvailable;
		int headroom;

		Neuropixels::getPacketFifoStatus(
			basestation->slot,
			headstage->port,
			dock,
			static_cast<Neuropixels::streamsource_t>(0),
			&packetsAvailable,
			&headroom);

		fifoFillPercentage = float(packetsAvailable) / float(packetsAvailable + headroom);

		if (packetsAvailable < MAXPACKETS)
		{
			int uSecToWait = (MAXPACKETS - packetsAvailable) * 30;

			std::this_thread::sleep_for(std::chrono::microseconds(uSecToWait));
		}
	}

}

bool Neuropixels_QuadBase::runBist(BIST bistType)
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

	return returnValue;
}
