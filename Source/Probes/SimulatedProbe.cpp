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

#include "../Utils.h"

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

	channel_count = 384;
	lfp_sample_rate = 2500.0f;
	ap_sample_rate = 30000.0f;

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

		apFilterSwitch = true;

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

	settings.referenceIndex = 0;

	isCalibrated = false;
	
}

bool SimulatedProbe::open()
{
	LOGD("Opened connection to probe.");
	return true;
}

bool SimulatedProbe::close()
{
	LOGD("Closed connection to probe.");
	return true;
}

void SimulatedProbe::initialize()
{
	calibrate();
	ap_timestamp = 0;
	lfp_timestamp = 0;
	eventCode = 0;

	apView = new ActivityView(384, 3000);
	lfpView = new ActivityView(384, 250);


	setStatus(ProbeStatus::CONNECTED);
	Sleep(200);
}

void SimulatedProbe::calibrate()
{
	Sleep(1);
	LOGD("Calibrating simulated probe.");
}

void SimulatedProbe::selectElectrodes()
{

	LOGD("Selecting channels for simulated probe.");

}

void SimulatedProbe::setApFilterState()
{

	LOGD("Wrote filter state for simulated probe.");
}

void SimulatedProbe::setAllGains()
{
	//for (int channel = 0; channel < 384; channel++)
	//{
	//	apGains.set(channel, int(apGain));
	//	lfpGains.set(channel, int(lfpGain));
	//}

	LOGD("Wrote gain state for simulated probe.");
}


void SimulatedProbe::setAllReferences()
{
	LOGD("Wrote reference state for simulated probe.");
}

void SimulatedProbe::writeConfiguration()
{
	Sleep(1000);

	LOGD("Wrote configuration for simulated probe.");
	
}

void SimulatedProbe::startAcquisition() {

	apView->reset();
	lfpView->reset();

}

void SimulatedProbe::stopAcquisition()
{

}

bool SimulatedProbe::runBist(BIST bistType)
{
	//TODO: Output some meaningful simulated results.
	return true;
}

#define INITIATION_POTENTIAL_START_TIME_IN_MS 0
#define DEPOLARIZATION_START_TIME_IN_MS 0.8f
#define REPOLARIZATION_START_TIME_IN_MS 1.3f
#define HYPERPOLARIZATION_START_TIME_IN_MS 1.8f
#define REFRACTORY_PERIOD_DURATION_IN_MS 5.0f

#define RESTING_MEMBRANE_POTENTIAL_IN_MV 10.0f
#define THRESHOLD_POTENTIAL_IN_MV -25.0f
#define PEAK_DEPOLARIZATION_POTENTIAL_IN_MV -100.0f
#define REFRACTORY_POTENTIAL_RANGE_IN_MV 50

std::random_device dev;
std::mt19937 rng(dev());

void SimulatedProbe::run()
{

	while (!threadShouldExit())
	{

		float sampleRate = 30000.0f;

		if (info.part_number == "PRB_1_4_0480_1")
		{

			std::uniform_int_distribution<std::mt19937::result_type> data(1,REFRACTORY_POTENTIAL_RANGE_IN_MV); 

			Sleep(25); //TODO: This is just an estimate on my machine for now, should be a function of nSamples

			float apSamples[384];
			float lfpSamples[384];

			for (int packetNum = 0; packetNum < 100; packetNum++)
			{

				for (int i = 0; i < 12; i++)
				{

					float time = 1000.0f * float(ap_timestamp % int(sampleRate)) / sampleRate;
					float sample_out;

					if (time < DEPOLARIZATION_START_TIME_IN_MS) 
					{
						sample_out = RESTING_MEMBRANE_POTENTIAL_IN_MV + (THRESHOLD_POTENTIAL_IN_MV) * time / (DEPOLARIZATION_START_TIME_IN_MS);
						eventCode = 1;
					}
					else if (time < REPOLARIZATION_START_TIME_IN_MS)
					{
						time -= DEPOLARIZATION_START_TIME_IN_MS;
						sample_out = THRESHOLD_POTENTIAL_IN_MV + (PEAK_DEPOLARIZATION_POTENTIAL_IN_MV - THRESHOLD_POTENTIAL_IN_MV) * time / (REPOLARIZATION_START_TIME_IN_MS - DEPOLARIZATION_START_TIME_IN_MS);
					}
					else if (time < HYPERPOLARIZATION_START_TIME_IN_MS)
					{ 
						time -= REPOLARIZATION_START_TIME_IN_MS;
						sample_out = PEAK_DEPOLARIZATION_POTENTIAL_IN_MV + (RESTING_MEMBRANE_POTENTIAL_IN_MV - PEAK_DEPOLARIZATION_POTENTIAL_IN_MV) * time / (HYPERPOLARIZATION_START_TIME_IN_MS - REPOLARIZATION_START_TIME_IN_MS);
					}
					else
					{
						eventCode = 0;
						sample_out = data(rng) - float(REFRACTORY_POTENTIAL_RANGE_IN_MV) / 2.0f;
					}

					for (int j = 0; j < 384; j++)
					{

						apSamples[j] = abs(sin(j))*sample_out;
						apView->addSample(sin(j)*sample_out, j);

						if (i == 0)
						{
							lfpSamples[j] = 1000.0f*sin(ap_timestamp);
							lfpView->addSample(lfpSamples[j], j);
						}
						
					}

					ap_timestamp += 1;

					apBuffer->addToBuffer(apSamples, &ap_timestamp, &eventCode, 1);

					if (ap_timestamp % 30000 == 0)
					{
						fifoFillPercentage = 0;
					}
				}
				lfp_timestamp += 1;

				lfpBuffer->addToBuffer(lfpSamples, &lfp_timestamp, &eventCode, 1);

			}	
		}
		/* Only generate simulated data for one probe model to minimize CPU usage.
		else 
		{
			Sleep(25);

			float apSamples[384];
			float lfpSamples[384];

			for (int packetNum = 0; packetNum < 100; packetNum++)
			{

				for (int i = 0; i < 12; i++)
				{

					for (int j = 0; j < 384; j++)
					{

						apSamples[j] = 0;
						//apView->addSample(sin(j)*sample_out, j); //No reason to update view if just generating 0s

						if (i == 0)
						{
							lfpSamples[j] = 0;
							//lfpView->addSample(sin(j)*sample_out, j);
						}
						
					}

					ap_timestamp += 1;

					apBuffer->addToBuffer(apSamples, &ap_timestamp, &eventCode, 1);

					if (ap_timestamp % 30000 == 0)
					{
						fifoFillPercentage = 0;
					}
				}
				lfp_timestamp += 1;

				lfpBuffer->addToBuffer(lfpSamples, &lfp_timestamp, &eventCode, 1);

			}	

		}
		*/
	}
}
