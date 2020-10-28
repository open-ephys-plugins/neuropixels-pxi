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
#include "../Headstages/SimulatedHeadstage.h"

void SimulatedProbe::getInfo()
{
	info.part_number = "Simulated probe";
}



SimulatedProbe::SimulatedProbe(Basestation* bs,
	Headstage* hs,
	Flex* fl,
	int dock) : Probe(bs, hs, fl, dock)
{
	channel_count = 384;
	lfp_sample_rate = 2500.0f;
	ap_sample_rate = 30000.0f;

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
	std::cout << "Calibrating simulated probe." << std::endl;
}

void SimulatedProbe::setChannelStatus(Array<int> channelStatus)
{

	int electrode;
	BANK_SELECT electrode_bank;

	for (int channel = 0; channel < channelMap.size(); channel++)
	{

		if (channel != 191)
		{
			if (channelStatus[channel])
			{
				electrode = channel;
				electrode_bank = BANK_SELECT::BANK_0;
			}
			else if (channelStatus[channel + 384])
			{
				electrode = channel + 384;
				electrode_bank = BANK_SELECT::BANK_1;
			}
			else if (channelStatus[channel + 2 * 384])
			{
				electrode = channel + 2 * 384;
				electrode_bank = BANK_SELECT::BANK_2;
			}
			else
			{
				electrode_bank = BANK_SELECT::DISCONNECTED;
			}

			channelMap.set(channel, electrode_bank);

		}

	}

	std::cout << "Updating electrode settings for"
		<< " slot: " << static_cast<unsigned>(basestation->slot)
		<< " port: " << static_cast<unsigned>(headstage->port) << std::endl;

}

void SimulatedProbe::setApFilterState(bool disableHighPass)
{

	std::cout << "Wrote filter state for simulated probe." << std::endl;
}

void SimulatedProbe::setAllGains(int apGain, int lfpGain)
{
	for (int channel = 0; channel < 384; channel++)
	{
		apGains.set(channel, int(apGain));
		lfpGains.set(channel, int(lfpGain));
	}

	std::cout << "Wrote gain state for simulated probe." << std::endl;
}


void SimulatedProbe::setAllReferences(int referenceIndex)
{
	std::cout << "Wrote reference state for simulated probe." << std::endl;
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
