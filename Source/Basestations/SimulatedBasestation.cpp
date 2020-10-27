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

#include "SimulatedBasestation.h"

#include "../Probes/SimulatedProbe.h"

void SimulatedBasestationConnectBoard::getInfo()
{
	info.boot_version = "XX.XX";
	info.version = "XX.XX";
	info.part_number = "Simulated BSC";
}


void SimulatedBasestation::getInfo()
{
	info.boot_version = "XX.XX";
}

/// ###########################################

void SimulatedBasestation::open()
{
	std::cout << "OPENING CHILD" << std::endl;
	savingDirectory = File();

	basestationConnectBoard = new SimulatedBasestationConnectBoard(this);
	basestationConnectBoard->getInfo();

	probes.add(new SimulatedProbe(this, 1));
	probes.getLast()->init();

	//probes.add(new SimulatedProbe(this, 2));
	//probes.getLast()->init();

	syncFrequencies.add(1);
	syncFrequencies.add(10);
}

void SimulatedBasestation::close()
{

}

void SimulatedBasestation::init()
{

	for (int i = 0; i < probes.size(); i++)
	{
		setGains(this->slot, probes[i]->port, 3, 2);
		probes[i]->setStatus(ProbeStatus::CONNECTED);
	}

}


void SimulatedBasestation::setSyncAsInput()
{

}


void SimulatedBasestation::setSyncAsOutput(int freqIndex)
{

}

int SimulatedBasestation::getProbeCount()
{
	return probes.size();
}


void SimulatedBasestation::initializeProbes()
{
	if (!probesInitialized)
	{
		
		for (int i = 0; i < probes.size(); i++)
		{

			probes[i]->calibrate();
			probes[i]->ap_timestamp = 0;
			probes[i]->lfp_timestamp = 0;
			probes[i]->eventCode = 0;
			probes[i]->setStatus(ProbeStatus::CONNECTED);

		}

		probesInitialized = true;
	}
}


Array<int> SimulatedBasestation::getSyncFrequencies()
{
	return syncFrequencies;
}

float SimulatedBasestation::getFillPercentage()
{
	return 0.0f;
}

void SimulatedBasestation::startAcquisition()
{
	for (int i = 0; i < probes.size(); i++)
	{

		probes[i]->ap_timestamp = 0;
		probes[i]->lfp_timestamp = 0;
		probes[i]->apBuffer->clear();
		probes[i]->lfpBuffer->clear();
		probes[i]->startThread();
	}

}

void SimulatedBasestation::stopAcquisition()
{
	for (int i = 0; i < probes.size(); i++)
	{
		probes[i]->stopThread(1000);
	}

}

void SimulatedBasestation::setChannels(unsigned char slot_, signed char port, Array<int> channelMap)
{
	if (slot == slot_)
	{
		for (int i = 0; i < probes.size(); i++)
		{
			if (probes[i]->port == port)
			{
				probes[i]->setChannels(channelMap);
				std::cout << "Set electrode-channel connections " << std::endl;
			}
		}
	}
}

void SimulatedBasestation::setApFilterState(unsigned char slot_, signed char port, bool disableHighPass)
{
	if (slot == slot_)
	{
		for (int i = 0; i < probes.size(); i++)
		{
			if (probes[i]->port == port)
			{
				probes[i]->setApFilterState(disableHighPass);
				std::cout << "Set all filters to " << int(disableHighPass) << std::endl;
			}
		}
	}

}

void SimulatedBasestation::setGains(unsigned char slot_, signed char port, unsigned char apGain, unsigned char lfpGain)
{
	if (slot == slot_)
	{
		for (int i = 0; i < probes.size(); i++)
		{
			if (probes[i]->port == port)
			{
				probes[i]->setGains(apGain, lfpGain);
				std::cout << "Set all gains to " << int(apGain) << ":" << int(lfpGain) << std::endl;
			}
		}
	}

}

void SimulatedBasestation::setReferences(unsigned char slot_, signed char port, np::channelreference_t refId, unsigned char refElectrodeBank)
{
	if (slot == slot_)
	{
		for (int i = 0; i < probes.size(); i++)
		{
			if (probes[i]->port == port)
			{
				probes[i]->setReferences(refId, refElectrodeBank);
				std::cout << "Set all references to " << refId << ":" << int(refElectrodeBank) << std::endl;
			}
		}
	}

}
