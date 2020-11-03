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
	info.boot_version = "SIM 0.0";
	info.version = "SIM 0.0";
	info.part_number = "Simulated BSC";
}


void SimulatedBasestation::getInfo()
{
	info.boot_version = "SIM 0.0";
	info.version = "SIM 0.0";
	info.part_number = "Simulated BS";
}

/// ###########################################

bool SimulatedBasestation::open()
{
	std::cout << "OPENING CHILD" << std::endl;
	savingDirectory = File();

	basestationConnectBoard = new SimulatedBasestationConnectBoard(this);
	basestationConnectBoard->getInfo();

	headstages.add(new SimulatedHeadstage(this, 0, "NP2000", 12395));
	headstages.add(new SimulatedHeadstage(this, 1, "NP2010", 45678));
	headstages.add(new SimulatedHeadstage(this, 2, "PRB_1_4_0480_1", 3222395));
	headstages.add(new SimulatedHeadstage(this, 3, "PRB_1_4_0480_1", 3225678));

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

	return true;
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



void SimulatedBasestation::run()
{

	for (int i = 0; i < 20; i++)
	{
		setProgress(0.05 * i);
		Sleep(100);
	}

}


void SimulatedBasestation::updateBscFirmware(File file)
{
	bscFirmwarePath = file.getFullPathName();
	Basestation::totalFirmwareBytes = (float)file.getSize();
	Basestation::currentBasestation = this;

	std::cout << bscFirmwarePath << std::endl;

	auto window = getAlertWindow();
	window->setColour(AlertWindow::textColourId, Colours::white);
	window->setColour(AlertWindow::backgroundColourId, Colour::fromRGB(50, 50, 50));

	this->setStatusMessage("Updating BSC firmware...");
	this->runThread(); //Upload firmware

	bscFirmwarePath = "";
}

void SimulatedBasestation::updateBsFirmware(File file)
{

	bsFirmwarePath = file.getFullPathName();
	Basestation::totalFirmwareBytes = (float) file.getSize();
	Basestation::currentBasestation = this;

	std::cout << bsFirmwarePath << std::endl;

	auto window = getAlertWindow();
	window->setColour(AlertWindow::textColourId, Colours::white);
	window->setColour(AlertWindow::backgroundColourId, Colour::fromRGB(50, 50, 50));

	this->setStatusMessage("Updating basestation firmware...");
	this->runThread(); //Upload firmware

	bsFirmwarePath = "";
}
