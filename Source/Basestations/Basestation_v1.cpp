/*
------------------------------------------------------------------

This file is part of the Open Ephys GUI
Copyright (C) 2018 Allen Institute for Brain Science and Open Ephys

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

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

#include "Basestation_v1.h"
#include "../Probes/Neuropixels1_v1.h"
#include "../Headstages/Headstage1_v1.h"

#define MAXLEN 50



void Basestation_v1::getInfo()
{

	unsigned char version_major;
	unsigned char version_minor;
	uint16_t version_build;

	errorCode = np::getBSBootVersion(slot, &version_major, &version_minor, &version_build);

	info.boot_version = String(version_major) + "." + String(version_minor);

	if (version_build != NULL)
		info.boot_version += ".";
		info.boot_version += String(version_build);

}


void BasestationConnectBoard_v1::getInfo()
{

	unsigned char version_major;
	unsigned char version_minor;
	uint16_t version_build;

	errorCode = np::getBSCBootVersion(basestation->slot, &version_major, &version_minor, &version_build);

	info.boot_version = String(version_major) + "." + String(version_minor);

	if (version_build != NULL)
		info.boot_version += ".";
		info.boot_version += String(version_build);

	errorCode = np::getBSCVersion(basestation->slot, &version_major, &version_minor);

	info.version = String(version_major) + "." + String(version_minor);

	errorCode = np::readBSCSN(basestation->slot, &info.serial_number);

	char pn[MAXLEN];
	np::readBSCPN(basestation->slot, pn, MAXLEN);

	info.part_number = String(pn);

}

BasestationConnectBoard_v1::BasestationConnectBoard_v1(Basestation* bs) : BasestationConnectBoard(bs)
{
	getInfo();
}

Basestation_v1::Basestation_v1(int slot_number) : Basestation(slot_number)
{
	getInfo();
}

bool Basestation_v1::open()
{
	std::cout << "OPENING PARENT" << std::endl;

	errorCode = np::openBS(slot_c);

	if (errorCode == np::VERSION_MISMATCH)
	{
		return false;
	}

	if (errorCode == np::SUCCESS)
	{

		std::cout << "  Opened BSv1 on slot " << slot_c << std::endl;

		basestationConnectBoard = new BasestationConnectBoard_v1(this);

		savingDirectory = File();

		for (signed char port = 1; port <= 4; port++)
		{

			errorCode = np::openProbe(slot_c, port); // check for probe on slot

			if (errorCode == np::SUCCESS)
			{
				Headstage* headstage = new Headstage1_v1(this, port);
				headstages.add(headstage);
				probes.add(headstage->probes[0]);

				continue;
			}
			else {
				headstages.add(nullptr);
			}

			errorCode = np::openProbeHSTest(slot_c, port);

			if (errorCode == np::SUCCESS)
			{
				if (headstages.getLast() != nullptr)
				{
					ScopedPointer<HeadstageTestModule> hst = new HeadstageTestModule_v1(this, headstages.getLast());
					hst->runAll();
					hst->showResults();
				}
			}

		}

		std::cout << "Found " << String(probes.size()) << (probes.size() == 1 ? " probe." : " probes.") << std::endl;
	}

	syncFrequencies.add(1);
	syncFrequencies.add(10);

	return true;
}

void Basestation_v1::initialize()
{

	if (!probesInitialized)
	{
		errorCode = np::setTriggerInput(slot, np::TRIGIN_SW);

		for (auto probe : probes)
		{
			probe->initialize();
		}

		probesInitialized = true;
	}

	errorCode = np::arm(slot_c);
}

Basestation_v1::~Basestation_v1()
{
	close();
	
}

void Basestation_v1::close()
{
	for (int i = 0; i < probes.size(); i++)
	{
		errorCode = np::close(slot_c, probes[i]->headstage->port_c);
	}

	errorCode = np::closeBS(slot_c);
}

void Basestation_v1::setSyncAsInput()
{

	/*
	errorCode = np::setTriggerInput(slot_c, np::TRIGIN_SW);
	if (errorCode != np::SUCCESS)
	{
		printf("Failed to set slot %d trigger as input!\n");
		return;
	}
	*/

	errorCode = setParameter(np::NP_PARAM_SYNCMASTER, slot_c);
	if (errorCode != np::SUCCESS)
	{
		printf("Failed to set slot %d as sync master!\n");
		return;
	}

	errorCode = setParameter(np::NP_PARAM_SYNCSOURCE, np::TRIGIN_SMA);
	if (errorCode != np::SUCCESS)
		printf("Failed to set slot %d SMA as sync input!\n");

	/*
	errorCode = setTriggerOutput(slot, np::TRIGOUT_PXI1, np::TRIGIN_SW);
	if (errorCode != np::SUCCESS)
	{
		printf("Failed to reset sync on SMA output on slot: %d\n", slot_c);
	}
	*/

}

Array<int> Basestation_v1::getSyncFrequencies()
{
	return syncFrequencies;
}

void Basestation_v1::setSyncAsOutput(int freqIndex)
{

	errorCode = setParameter(np::NP_PARAM_SYNCMASTER, slot_c);
	if (errorCode != np::SUCCESS)
	{
		printf("Failed to set slot %d as sync master!\n", slot_c);
		return;
	} 

	errorCode = setParameter(np::NP_PARAM_SYNCSOURCE, np::TRIGIN_SYNCCLOCK);
	if (errorCode != np::SUCCESS)
	{
		printf("Failed to set slot %d internal clock as sync source!\n", slot_c);
		return;
	}

	int freq = syncFrequencies[freqIndex];

	printf("Setting slot %d sync frequency to %d Hz...\n", slot_c, freq);
	errorCode = setParameter(np::NP_PARAM_SYNCFREQUENCY_HZ, freq);
	if (errorCode != np::SUCCESS)
	{
		printf("Failed to set slot %d sync frequency to %d Hz!\n", slot_c, freq);
		return;
	}

	/*
	errorCode = setTriggerOutput(slot_c, np::TRIGOUT_SMA, np::TRIGIN_SHAREDSYNC);
	if (errorCode != np::SUCCESS)
	{
		printf("Failed to set sync on SMA output on slot: %d\n", slot);
	}
	*/

}

int Basestation_v1::getProbeCount()
{
	return probes.size();
}

float Basestation_v1::getFillPercentage()
{
	float perc = 0.0;

	for (int i = 0; i < getProbeCount(); i++)
	{
		//std::cout << "Percentage for probe " << i << ": " << probes[i]->fifoFillPercentage << std::endl;

		if (probes[i]->fifoFillPercentage > perc)
			perc = probes[i]->fifoFillPercentage;
	}

	return perc;
}

void Basestation_v1::startAcquisition()
{
	for (auto probe : probes)
	{
		probe->startAcquisition();
	}

	errorCode = np::setSWTrigger(slot_c);

}

void Basestation_v1::stopAcquisition()
{
	for (auto probe : probes)
	{
		probe->stopAcquisition();
	}

	errorCode = np::arm(slot_c);
}

void Basestation_v1::updateBscFirmware(File file)
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

	AlertWindow::showMessageBoxAsync(AlertWindow::InfoIcon, "Successful firmware update",
		String("Basestation connect board firmware updated successfully. Please update the basestation firmware now."));
}

void Basestation_v1::updateBsFirmware(File file)
{
	bsFirmwarePath = file.getFullPathName();
	Basestation::totalFirmwareBytes = (float)file.getSize();
	Basestation::currentBasestation = this;

	std::cout << bsFirmwarePath << std::endl;

	auto window = getAlertWindow();
	window->setColour(AlertWindow::textColourId, Colours::white);
	window->setColour(AlertWindow::backgroundColourId, Colour::fromRGB(50, 50, 50));

	this->setStatusMessage("Updating basestation firmware...");
	this->runThread(); //Upload firmware

	bsFirmwarePath = "";

	AlertWindow::showMessageBoxAsync(AlertWindow::InfoIcon, "Successful firmware update",
		String("Please restart your computer and power cycle the PXI chassis for the changes to take effect."));
}

void Basestation_v1::run()
{
	
	if (bscFirmwarePath.length() > 0)
		errorCode = np::qbsc_update(slot_c,
								bscFirmwarePath.getCharPointer(),
								firmwareUpdateCallback);

	if (bsFirmwarePath.length() > 0)
		errorCode = np::bs_update(slot_c,
					  bsFirmwarePath.getCharPointer(),
					  firmwareUpdateCallback);


}


bool Basestation_v1::runBist(int port, int dock, BIST bistType)
{
	signed char port_c = (signed char) port;

	bool returnValue = false;

	switch (bistType)
	{
	case BIST::SIGNAL:
	{
		np::NP_ErrorCode errorCode = np::bistSignal(slot_c, port_c, &returnValue, stats);
		break;
	}
	case BIST::NOISE:
	{
		if (np::bistNoise(slot_c, port_c) == np::SUCCESS)
			returnValue = true;
		break;
	}
	case BIST::PSB:
	{
		if (np::bistPSB(slot_c, port_c) == np::SUCCESS)
			returnValue = true;
		break;
	}
	case BIST::SR:
	{
		if (np::bistSR(slot_c, port_c) == np::SUCCESS)
			returnValue = true;
		break;
	}
	case BIST::EEPROM:
	{
		if (np::bistEEPROM(slot_c, port_c) == np::SUCCESS)
			returnValue = true;
		break;
	}
	case BIST::I2C:
	{
		if (np::bistI2CMM(slot_c, port_c) == np::SUCCESS)
			returnValue = true;
		break;
	}
	case BIST::SERDES:
	{
		unsigned char errors;
		np::bistStartPRBS(slot_c, port_c);
		Sleep(200);
		np::bistStopPRBS(slot_c, port_c, &errors);

		if (errors == 0)
			returnValue = true;
		break;
	}
	case BIST::HB:
	{
		if (np::bistHB(slot_c, port_c) == np::SUCCESS)
			returnValue = true;
		break;
	} case BIST::BS:
	{
		if (np::bistBS(slot_c) == np::SUCCESS)
			returnValue = true;
		break;
	} default:
		CoreServices::sendStatusMessage("Test not found.");
	}

	return returnValue;
}