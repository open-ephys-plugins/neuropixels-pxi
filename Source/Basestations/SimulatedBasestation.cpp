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
#include "../Headstages/SimulatedHeadstage.h"

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

	headstages.add(new SimulatedHeadstage(this, 0));
	headstages.add(new SimulatedHeadstage(this, 1));
	headstages.add(nullptr);
	headstages.add(nullptr);

	for (auto headstage : headstages)
	{
		if (headstage != nullptr)
		{
			probes.add(headstage->getProbes()[0]);
		}
	}

	std::cout << probes.size() << " total probes " << std::endl;

	syncFrequencies.add(1);
	syncFrequencies.add(10);
}

void SimulatedBasestation::close()
{

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


void SimulatedBasestation::initialize()
{
	if (!probesInitialized)
	{
		
		for (auto probe: probes)
		{
			probe->initialize();
			
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
