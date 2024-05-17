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

	setStatus(SourceStatus::DISCONNECTED);

	customName.probeSpecific = String(info.serial_number);

	CoreServices::sendStatusMessage("Probe part number: " + PN);

	if (PN == "NP1300")
	{
		Geometry::forPartNumber(PN, electrodeMetadata, emissionSiteMetadata, probeMetadata);
	}
	else {
		Geometry::forPartNumber(PN, electrodeMetadata, probeMetadata);
	}
	
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
        settings.selectedChannel.add(electrodeMetadata[i].channel);
        settings.selectedShank.add(0);
		settings.selectedElectrode.add(electrodeMetadata[i].global_index);
    }

	if (type == ProbeType::NP1 ||
		type == ProbeType::NHP10 ||
		type == ProbeType::NHP25 ||
		type == ProbeType::NHP45 ||
		type == ProbeType::UHD1 ||
		type == ProbeType::UHD2 ||
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

		std::cout << "Probe Type: " << (int) type << std::endl;

		if (type == ProbeType::UHD2)
		{
			for (int i = 0; i < 16; i++)
				settings.availableElectrodeConfigurations.add("8 x 48: Bank " + String(i));
			settings.availableElectrodeConfigurations.add("1 x 384: Tip Half");
			settings.availableElectrodeConfigurations.add("1 x 384: Base Half");
			settings.availableElectrodeConfigurations.add("2 x 192");
			settings.availableElectrodeConfigurations.add("4 x 96");
			settings.availableElectrodeConfigurations.add("2 x 2 x 96");

			LOGC("ADDING ELECTRODE CONFIGS");
		}
		else {
			if (type != ProbeType::UHD1 && type != ProbeType::NHP1)
			{
				settings.availableElectrodeConfigurations.add("Bank A");
				settings.availableElectrodeConfigurations.add("Bank B");
				settings.availableElectrodeConfigurations.add("Bank C");

				if (type == ProbeType::NHP25 || type == ProbeType::NHP45)
				{
					settings.availableElectrodeConfigurations.add("Bank D");
					settings.availableElectrodeConfigurations.add("Bank E");
					settings.availableElectrodeConfigurations.add("Bank F");
					settings.availableElectrodeConfigurations.add("Bank G");

					if (type == ProbeType::NHP45)
					{
						settings.availableElectrodeConfigurations.add("Bank H");
						settings.availableElectrodeConfigurations.add("Bank I");
						settings.availableElectrodeConfigurations.add("Bank J");
						settings.availableElectrodeConfigurations.add("Bank K");
						settings.availableElectrodeConfigurations.add("Bank L");
					}
				}

				settings.availableElectrodeConfigurations.add("Single Column");
				settings.availableElectrodeConfigurations.add("Tetrodes");
			}
		}

		settings.apGainIndex = 3;
		settings.lfpGainIndex = 2;
		settings.apFilterState = true;

		settings.availableReferences.add("Ext");
		settings.availableReferences.add("Tip");

	}
	else {

		settings.apGainIndex = -1;
		settings.lfpGainIndex = -1;

		apFilterSwitch = false;

		if (type == ProbeType::NP2_1)
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
			settings.availableReferences.add("1: Tip");
			//settings.availableReferences.add("1: 128");
			//settings.availableReferences.add("1: 512");
			//settings.availableReferences.add("1: 896");
			//settings.availableReferences.add("1: 1280");
			settings.availableReferences.add("2: Tip");
			//settings.availableReferences.add("2: 128");
			//settings.availableReferences.add("2: 512");
			//settings.availableReferences.add("2: 896");
			//settings.availableReferences.add("2: 1280");
			settings.availableReferences.add("3: Tip");
			//settings.availableReferences.add("3: 128");
			//settings.availableReferences.add("3: 512");
			//settings.availableReferences.add("3: 896");
			//settings.availableReferences.add("3: 1280");
			settings.availableReferences.add("4: Tip");
			//settings.availableReferences.add("4: 128");
			//settings.availableReferences.add("4: 512");
			//settings.availableReferences.add("4: 896");
			//settings.availableReferences.add("4: 1280");

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

Array<int> SimulatedProbe::selectElectrodeConfiguration(String config)
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
		if (type == ProbeType::NHP25 || type == ProbeType::NHP45 || type == ProbeType::NP2_1)
		{
			for (int i = 768; i < 1152; i++)
				selection.add(i);
		}
		else {
			for (int i = 576; i < 960; i++)
				selection.add(i);
		}
	}
	else if (config.equalsIgnoreCase("Bank D"))
	{
		if (type == ProbeType::NP2_1)
		{
			for (int i = 896; i < 1280; i++)
				selection.add(i);
		}
		else {
			for (int i = 1152; i < 1536; i++)
				selection.add(i);
		}
		
	}
	else if (config.equalsIgnoreCase("Bank E"))
	{
		for (int i = 1536; i < 1920; i++)
			selection.add(i);
	}
	else if (config.equalsIgnoreCase("Bank F"))
	{
		for (int i = 1920; i < 2304; i++)
			selection.add(i);
	}
	else if (config.equalsIgnoreCase("Bank G"))
	{
		if (type == ProbeType::NHP45)
		{
			for (int i = 2304; i < 2688; i++)
				selection.add(i);
		}
		else {
			for (int i = 2112; i < 2496; i++)
				selection.add(i);
		}
	}
	else if (config.equalsIgnoreCase("Bank H"))
	{
		for (int i = 2688; i < 3072; i++)
			selection.add(i);

	}
	else if (config.equalsIgnoreCase("Bank I"))
	{
		for (int i = 3072; i < 3456; i++)
			selection.add(i);

	}
	else if (config.equalsIgnoreCase("Bank J"))
	{
		for (int i = 3456; i < 3840; i++)
			selection.add(i);

	}
	else if (config.equalsIgnoreCase("Bank K"))
	{
		for (int i = 3840; i < 4224; i++)
			selection.add(i);

	}
	else if (config.equalsIgnoreCase("Bank L"))
	{
		for (int i = 4032; i < 4416; i++)
			selection.add(i);

	}
	else if (config.equalsIgnoreCase("Single Column"))
	{
		for (int i = 0; i < 384; i += 2)
			selection.add(i);

		for (int i = 385; i < 768; i += 2)
			selection.add(i);
	}
	else if (config.equalsIgnoreCase("Tetrodes"))
	{
		for (int i = 0; i < 384; i += 8)
		{
			for (int j = 0; j < 4; j++)
				selection.add(i + j);
		}

		for (int i = 388; i < 768; i += 8)
		{
			for (int j = 0; j < 4; j++)
				selection.add(i + j);
		}

	}
	else if (config.equalsIgnoreCase("Shank 1 Bank A"))
	{
		for (int i = 0; i < 384; i ++)
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
	} else if (config.equalsIgnoreCase("Shank 4 Bank A"))
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

	if (generatesLfpData())
		lfpBuffer->clear();

	apView->reset();

	if (generatesLfpData())
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
					apSamples[j * (12 * MAXPACKETS) + i + (packetNum * 12)] = (simulatedData.ap_band[ap_timestamp % 3000] + float(j * 2) - ap_offsets[j][0]) 
						* (float((ap_timestamp + j * 78) % 60000) / 60000.0f);
					apView->addSample(apSamples[j * (12 * MAXPACKETS) + i + (packetNum * 12)], j);
					
					if (i == 0)
					{
						lfpSamples[(j * MAXPACKETS) + packetNum] = simulatedData.lfp_band[lfp_timestamp % 250] * float(j % 24) / 24.0f - lfp_offsets[j][0];
						lfpView->addSample(lfpSamples[(j * MAXPACKETS) + packetNum], j);
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
					apSamples[384 * (12 * MAXPACKETS) + i + (packetNum * 12)] = (float)eventCode;

			}

			lfp_timestamps[packetNum] = lfp_timestamp++;
			lfp_event_codes[packetNum] = eventCode;

			if (sendSync)
				lfpSamples[(384 * MAXPACKETS) + packetNum] = (float)eventCode;
		}

		apBuffer->addToBuffer(apSamples, ap_timestamps, timestamp_s, event_codes, 12 * MAXPACKETS);

		if (generatesLfpData())
			lfpBuffer->addToBuffer(lfpSamples, lfp_timestamps, timestamp_s, lfp_event_codes, MAXPACKETS);

		if (ap_offsets[0][0] == 0)
		{
			updateOffsets(apSamples, ap_timestamp, true);

			if (generatesLfpData())
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
