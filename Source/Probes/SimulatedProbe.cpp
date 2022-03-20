/*
------------------------------------------------------------------

This file is part of the Open Ephys GUI
Copyright (C) 2020 Allen Institute for Brain Science and Open Ephys

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

#include "SimulatedProbe.h"
#include "Geometry.h"
#include "../Headstages/SimulatedHeadstage.h"

void SimulatedProbe::getInfo()
{
	info.part_number = "Simulated probe";
	info.serial_number = 123456789;

}

SimulatedProbe::SimulatedProbe(Basestation* bs,
	Headstage* hs,
	Flex* fl,
	int dock, String PN, int SN) : Probe(bs, hs, fl, dock)
{

	getInfo();
	info.serial_number = SN;
	info.part_number = PN;

	CoreServices::sendStatusMessage("Probe part number: " + PN);

	Geometry::forPartNumber(PN, electrodeMetadata, probeMetadata);

	name = probeMetadata.name;
	type = probeMetadata.type;

	settings.probe = this;

	settings.availableBanks = probeMetadata.availableBanks;

	channel_count = 384;
	lfp_sample_rate = 2500.0f;
	ap_sample_rate = 30000.0f;

	settings.referenceIndex = 0;

	apFilterSwitch = true;

	for (int i = 0; i < channel_count; i++)
    {
        settings.selectedBank.add(Bank::A);
        settings.selectedChannel.add(i);
        settings.selectedShank.add(0);
    }

	if (type == ProbeType::NP1 ||
		type == ProbeType::NHP10 ||
		type == ProbeType::NHP25 ||
		type == ProbeType::NHP45 ||
		type == ProbeType::UHD1 ||
		type == ProbeType::NHP1)
	{
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

		settings.apGainIndex = 3;
		settings.lfpGainIndex = 2;
		settings.apFilterState = true;

		settings.availableReferences.add("Ext");
		settings.availableReferences.add("Tip");
		settings.availableReferences.add("192");
		settings.availableReferences.add("576");
		settings.availableReferences.add("960");

	}
	else {

		settings.apGainIndex = -1;
		settings.lfpGainIndex = -1;

		apFilterSwitch = false;

		if (type == ProbeType::NP2_1)
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
	}

	open();

}

bool SimulatedProbe::generatesLfpData()
{
	return apFilterSwitch;
}

bool SimulatedProbe::open()
{
	LOGD("Opened connection to probe.");

	ap_timestamp = 0;
	lfp_timestamp = 0;
	eventCode = 0;

	apView = new ActivityView(384, 3000);
	lfpView = new ActivityView(384, 250);

	return true;
}

bool SimulatedProbe::close()
{
	LOGD("Closed connection to probe.");
	return true;
}

void SimulatedProbe::initialize(bool signalChainIsLoading)
{

	LOGD("Initializing probe...");
	Sleep(1000);

}

void SimulatedProbe::calibrate()
{
	Sleep(1);
	LOGC("Calibrating simulated probe.");
}

void SimulatedProbe::selectElectrodes()
{

	LOGC("Selecting channels for simulated probe.");

}

void SimulatedProbe::setApFilterState()
{

	LOGC("Wrote filter state for simulated probe.");
}

void SimulatedProbe::setAllGains()
{

	LOGC("Wrote gain state for simulated probe.");
}


void SimulatedProbe::setAllReferences()
{
	LOGC("Wrote reference state for simulated probe.");
}

void SimulatedProbe::writeConfiguration()
{
	Sleep(500);

	LOGC("Wrote configuration for simulated probe.");
	
}

void SimulatedProbe::startAcquisition() 
{

	ap_timestamp = 0;
	lfp_timestamp = 0;
	apBuffer->clear();
	lfpBuffer->clear();

	apView->reset();
	lfpView->reset();

	SKIP = sendSync ? 385 : 384;

	startThread();
}

void SimulatedProbe::stopAcquisition()
{
	signalThreadShouldExit();
}

bool SimulatedProbe::runBist(BIST bistType)
{
	//TODO: Output some meaningful simulated results.
	return true;
}

void SimulatedProbe::run()
{

	while (!threadShouldExit())
	{

		int64 start = Time::getHighResolutionTicks();

		for (int packetNum = 0; packetNum < MAXPACKETS; packetNum++)
		{
			for (int i = 0; i < 12; i++)
			{
				for (int j = 0; j < 384; j++)
				{
					apSamples[j + i * SKIP + packetNum * 12 * SKIP] = (simulatedData.ap_band[ap_timestamp % 3000] + float(j * 2) - ap_offsets[j][0]) 
						* (float((ap_timestamp + j * 78) % 60000) / 60000.0f);
					apView->addSample(apSamples[j + i * SKIP + packetNum * 12 * SKIP], j);
					
					if (i == 0)
					{
						lfpSamples[j + packetNum * SKIP] = simulatedData.lfp_band[lfp_timestamp % 250] * float(j % 24) / 24.0f - lfp_offsets[j][0];
						lfpView->addSample(lfpSamples[j + packetNum * SKIP], j);
					}
							
				}

				ap_timestamps[i + packetNum * 12] = ap_timestamp++;

				if (ap_timestamp % 15000 == 0)
				{
					if (eventCode == 0)
						eventCode = 1;
					else
						eventCode = 0;
				}

				event_codes[i + packetNum * 12] = eventCode;

				if (sendSync)
					apSamples[384 + i * SKIP + packetNum * 12 * SKIP] = (float)eventCode;

			}

			lfp_timestamps[packetNum] = lfp_timestamp++;
			lfp_event_codes[packetNum] = eventCode;

			if (sendSync)
				lfpSamples[384 + packetNum * SKIP] = (float)eventCode;
		}

		apBuffer->addToBuffer(apSamples, ap_timestamps, event_codes, 12 * MAXPACKETS);
		lfpBuffer->addToBuffer(lfpSamples, lfp_timestamps, lfp_event_codes, MAXPACKETS);

		if (ap_offsets[0][0] == 0)
		{
			updateOffsets(apSamples, ap_timestamp, true);
			updateOffsets(lfpSamples, lfp_timestamp, false);
		}
		

		fifoFillPercentage = 0;
		
		int64 uSecElapsed = int64 (Time::highResolutionTicksToSeconds(Time::getHighResolutionTicks() - start) * 1e6);

		if (uSecElapsed < 400 * MAXPACKETS)
		{
			std::this_thread::sleep_for(std::chrono::microseconds(400 * MAXPACKETS - uSecElapsed));
		}

	}

}
