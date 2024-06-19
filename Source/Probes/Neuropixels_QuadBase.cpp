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

	acquisitionThreads.clear();

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

		channel_count = 384;
		lfp_sample_rate = 2500.0f; // not used
		ap_sample_rate = 30000.0f;

		for (int shank = 0; shank < 4; shank++)
		{
			for (int i = 0; i < channel_count; i++)
			{
				settings.selectedBank.add(Bank::A);
				settings.selectedChannel.add(electrodeMetadata[i].channel);
				settings.selectedShank.add(shank);
				settings.selectedElectrode.add(electrodeMetadata[i].global_index);

			}
		}
		
		settings.availableReferences.add("Ext");
		settings.availableReferences.add("Tip");
		settings.availableReferences.add("Ground");

		settings.availableElectrodeConfigurations.add("Bank A");
		settings.availableElectrodeConfigurations.add("Bank B");
		settings.availableElectrodeConfigurations.add("Bank C");
		settings.availableElectrodeConfigurations.add("Bank D");
		settings.availableElectrodeConfigurations.add("Single column");
		settings.availableElectrodeConfigurations.add("Tetrodes");

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

			//AlertWindow::showMessageBox(AlertWindow::AlertIconType::WarningIcon, "Calibration files missing", message, "OK");

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
		for (int shank = 0; shank < 4; shank++)
		{
			for (int i = 0; i < 384; i++)
				selection.add(i + 1280 * shank);
		}
	}
	else if (config.equalsIgnoreCase("Bank B"))
	{
		for (int shank = 0; shank < 4; shank++)
		{
			for (int i = 384; i < 768; i++)
				selection.add(i + 1280 * shank);
		}
	}
	else if (config.equalsIgnoreCase("Bank C"))
	{
		for (int shank = 0; shank < 4; shank++)
		{
			for (int i = 768; i < 1152; i++)
				selection.add(i + 1280 * shank);
		}
	}

	else if (config.equalsIgnoreCase("Bank D"))
	{
		for (int shank = 0; shank < 4; shank++)
		{
			for (int i = 896; i < 1280; i++)
				selection.add(i + 1280 * shank);
		}
	}

	else if (config.equalsIgnoreCase("Single Column"))
	{
		for (int shank = 0; shank < 4; shank++)
		{
			for (int i = 0; i < 384; i += 2)
				selection.add(i + 1280 * shank);

			for (int i = 385; i < 768; i += 2)
				selection.add(i + 1280 * shank);
		}
		
	}
	else if (config.equalsIgnoreCase("Tetrodes"))
	{
		for (int shank = 0; shank < 4; shank++)
		{
			for (int i = 0; i < 384; i += 8)
			{
				for (int j = 0; j < 4; j++)
					selection.add(i + j + 1280 * shank);
			}

			for (int i = 388; i < 768; i += 8)
			{
				for (int j = 0; j < 4; j++)
					selection.add(i + j + 1280 * shank);
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

	switch (settings.referenceIndex)
	{
	case 0:
		refId = Neuropixels::EXT_REF;
		break;
	case 1:
		refId = Neuropixels::TIP_REF;
		break;
	case 2:
		refId = Neuropixels::GND_REF;
		break;
	
	default:
		refId = Neuropixels::EXT_REF;
	}

	for (int shank = 0; shank < 4; shank++)
	{
		for (int channel = 0; channel < channel_count; channel++)
			Neuropixels::setReference(basestation->slot,
				headstage->port,
				dock,
				channel,
				shank,
				refId,
				refElectrodeBank);
	}
	

	LOGD("Updated reference for slot: ", basestation->slot, " port: ", headstage->port, " dock: ", dock, " to ", refId);

}

void Neuropixels_QuadBase::writeConfiguration()
{

	errorCode = Neuropixels::writeProbeConfiguration(basestation->slot, headstage->port, dock, false);
}

void Neuropixels_QuadBase::startAcquisition()
{
	if (acquisitionThreads.size() == 0)
	{
		for (int shank = 0; shank < 4; shank++)
		{
			
			quadBaseBuffers[shank]->clear();

			acquisitionThreads.add(
				new AcquisitionThread(basestation->slot,
					headstage->port,
					dock,
					shank, 
					quadBaseBuffers[shank], 
					this));
		}
	
	}

	apView->reset();

	for (int shank = 0; shank < 4; shank++)
	{
		jassert(quadBaseBuffers[shank]->getNumSamples() == 0);

		acquisitionThreads[shank]->startThread();
	}
}

void Neuropixels_QuadBase::stopAcquisition()
{
	LOGC("Probe stopping thread.");

	for (int shank = 0; shank < 4; shank++)
	{
		acquisitionThreads[shank]->signalThreadShouldExit();
	}

}

AcquisitionThread::AcquisitionThread(
	int slot_,
	int port_,
	int dock_,
	int shank_, 
	DataBuffer* buffer_, 
	Probe* probe_) : 
	Thread("AcquisitionThread" + String(shank)), 
	slot(slot_),
	port(port_),
	dock(dock_),
	shank(shank_), 
	buffer(buffer_),
	probe(probe_),
	ap_sample_rate(30000.0f),
	ap_timestamp(0),
	last_npx_timestamp(0),
	sendSync(false),
	passedOneSecond(false),
	eventCode(0)
{
	if (shank == 0)
		stream_source = Neuropixels::streamsource_t::SourceAP;
	else if (shank == 1)
		stream_source = Neuropixels::streamsource_t::SourceLFP;
	else if (shank == 2)
		stream_source = Neuropixels::streamsource_t::SourceSt2;
	else if (shank == 3)
		stream_source = Neuropixels::streamsource_t::SourceSt3;

}

void AcquisitionThread::run()
{

	//apView->reset();

	ap_timestamp = 0;
	last_npx_timestamp = 0;
	passedOneSecond = false;

	SKIP = probe->sendSync ? 385 : 384;

	LOGD("  Starting thread for shank ", shank);

	while (!threadShouldExit())
	{
		int channel_count = 384;

		int packet_count = MAXPACKETS;

		errorCode = Neuropixels::readPackets(
			slot,
			port,
			dock,
			stream_source,
			&packetInfo[0],
			&data[0],
			channel_count,
			packet_count,
			&packet_count);

		if (errorCode == Neuropixels::SUCCESS && packet_count > 0)
		{
			for (int packetNum = 0; packetNum < packet_count; packetNum++)
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
							String(slot) + ", probe " + String(port) +
							" at sample number " + String(ap_timestamp);

						LOGC(msg);

						probe->basestation->neuropixThread->sendBroadcastMessage(msg);
					}
				}

				last_npx_timestamp = npx_timestamp;

				if (sendSync)
				{
					apSamples[4 * channel_count + SKIP * packetNum] = (float)eventCode;
				}

				for (int j = 0; j < channel_count; j++)
				{
					apSamples[j + packetNum * SKIP] =
						float(data[packetNum * channel_count + j]) * 1.0f / 4096.0f * 1000000.0f / 80.0f; // convert to microvolts

					//probe->apView->addSample(apSamples[j + packetNum * SKIP], j);

				}

				ap_timestamps[packetNum] = ap_timestamp++;
				event_codes[packetNum] = eventCode;

			}

		}
		else if (errorCode != Neuropixels::SUCCESS)
		{
			std::cout << "readPackets error code: " << errorCode << " for Basestation " << slot << ", probe " << port << std::endl;
		}

		buffer->addToBuffer(apSamples, ap_timestamps, timestamp_s, event_codes, packet_count);

		if (!passedOneSecond)
		{
			if (ap_timestamp > 30000)
				passedOneSecond = true;
		}

		int packetsAvailable;
		int headroom;

		Neuropixels::getPacketFifoStatus(
			slot,
			port,
			dock,
			static_cast<Neuropixels::streamsource_t>(0),
			&packetsAvailable,
			&headroom);

		if (shank == 0)
			probe->fifoFillPercentage = float(packetsAvailable) / float(packetsAvailable + headroom);

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
