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

	apGainIndex = 3;
	lfpGainIndex = 2;
	referenceIndex = 0;
	apFilterState = true;

	

}

void SimulatedProbe::initialize()
{
	calibrate();
	ap_timestamp = 0;
	lfp_timestamp = 0;
	eventCode = 0;
	setStatus(ProbeStatus::CONNECTED);
	Sleep(200);
}

void SimulatedProbe::calibrate()
{
	Sleep(1);
	std::cout << "Calibrating simulated probe." << std::endl;
}

void SimulatedProbe::selectElectrodes(ProbeSettings settings, bool shouldWriteConfiguration)
{

	std::cout << "Selecting channels for simulated probe." << std::endl;

}

void SimulatedProbe::setApFilterState(bool disableHighPass, bool shouldWriteConfiguration)
{

	std::cout << "Wrote filter state for simulated probe." << std::endl;
}

void SimulatedProbe::setAllGains(int apGain, int lfpGain, bool shouldWriteConfiguration)
{
	//for (int channel = 0; channel < 384; channel++)
	//{
	//	apGains.set(channel, int(apGain));
	//	lfpGains.set(channel, int(lfpGain));
	//}

	std::cout << "Wrote gain state for simulated probe." << std::endl;
}


void SimulatedProbe::setAllReferences(int referenceIndex, bool shouldWriteConfiguration)
{
	std::cout << "Wrote reference state for simulated probe." << std::endl;
}

void SimulatedProbe::writeConfiguration()
{
	Sleep(1000);

	std::cout << "Wrote configuration for simulated probe." << std::endl;
	
}

void SimulatedProbe::startAcquisition() {

}

void SimulatedProbe::stopAcquisition()
{

}

void SimulatedProbe::run()
{

	while (!threadShouldExit())
	{

		if (1) // every 1/300 s
		{
			Sleep(3);

			float apSamples[384];
			float lfpSamples[384];

			for (int packetNum = 0; packetNum < 100; packetNum++)
			{
				for (int i = 0; i < 12; i++)
				{
					for (int j = 0; j < 384; j++)
					{
						apSamples[j] = 0;

						if (i == 0)
							lfpSamples[j] = 0;
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
	}

}
