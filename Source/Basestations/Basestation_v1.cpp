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

	boot_version = String(version_major) + "." + String(version_minor);

	if (version_build != NULL)
		boot_version += ".";
		boot_version += String(version_build);

}


void BasestationConnectBoard_v1::getInfo()
{

	unsigned char version_major;
	unsigned char version_minor;
	uint16_t version_build;

	errorCode = np::getBSCBootVersion(basestation->slot, &version_major, &version_minor, &version_build);

	info.boot_version = String(version_major) + "." + String(version_minor);

	if (version_build != NULL)
		boot_version += ".";
		boot_version += String(version_build);

	errorCode = np::getBSCVersion(basestation->slot, &version_major, &version_minor);

	info.version = String(version_major) + "." + String(version_minor);

	errorCode = np::readBSCSN(basestation->slot, &info.serial_number);

	char pn[MAXLEN];
	np::readBSCPN(basestation->slot, pn, MAXLEN);

	info.part_number = String(pn);

}

BasestationConnectBoard_v1::BasestationConnectBoard_v1(Basestation* bs) : BasestationConnectBoard(bs)
{
}

Basestation_v1::Basestation_v1(int slot_number) : Basestation(slot_number)
{

	slot = (unsigned char) slot_number;

}

void Basestation_v1::open()
{
	std::cout << "OPENING PARENT" << std::endl;

	errorCode = np::openBS(slot);

	if (errorCode == np::SUCCESS)
	{

		std::cout << "  Opened BS on slot " << int(slot) << std::endl;

		getInfo();
		basestationConnectBoard = new BasestationConnectBoard_v1(this);
		basestationConnectBoard->getInfo();

		savingDirectory = File();

		for (signed char port = 1; port <= 4; port++)
		{

			errorCode = np::openProbe(slot, port);

			if (errorCode == np::SUCCESS)
			{
				probes.add(new Neuropixels1_v1(this, port));
				probes.getLast()->init();
				probes.getLast()->setStatus(ProbeStatus::CONNECTING);
				continue;
			}

			errorCode = np::openProbeHSTest(slot, port);

			if (errorCode == np::SUCCESS)
			{
				ScopedPointer<HeadstageTestModule> hst = new HeadstageTestModule_v1(this, port);
				hst->runAll();
				hst->showResults();
			}

		}
		std::cout << "Found " << String(probes.size()) << (probes.size() == 1 ? " probe." : " probes.") << std::endl;
	}

	syncFrequencies.add(1);
	syncFrequencies.add(10);
}

void Basestation_v1::init()
{

	for (int i = 0; i < probes.size(); i++)
	{
		std::cout << "Initializing probe " << String(i + 1) << "/" << String(probes.size()) << "...";

		errorCode = np::init(this->slot, probes[i]->port);
		if (errorCode != np::SUCCESS)
			std::cout << "  FAILED!." << std::endl;
		else
		{
			setGains(this->slot, probes[i]->port, 3, 2);
			probes[i]->setStatus(ProbeStatus::CONNECTED);
		}

	}

}

Basestation_v1::~Basestation_v1()
{
	close();
	
}

void Basestation_v1::close()
{
	for (int i = 0; i < probes.size(); i++)
	{
		errorCode = np::close(slot, probes[i]->port);
	}

	errorCode = np::closeBS(slot);
}

void Basestation_v1::setSyncAsInput()
{

	errorCode = np::setTriggerInput(slot, np::TRIGIN_SW);
	if (errorCode != np::SUCCESS)
	{
		printf("Failed to set slot %d trigger as input!\n");
		return;
	}

	errorCode = setParameter(np::NP_PARAM_SYNCMASTER, slot);
	if (errorCode != np::SUCCESS)
	{
		printf("Failed to set slot %d as sync master!\n");
		return;
	}

	errorCode = setParameter(np::NP_PARAM_SYNCSOURCE, np::TRIGIN_SMA);
	if (errorCode != np::SUCCESS)
		printf("Failed to set slot %d SMA as sync source!\n");

	errorCode = setTriggerOutput(slot, np::TRIGOUT_PXI1, np::TRIGIN_SW);
	if (errorCode != np::SUCCESS)
	{
		printf("Failed to reset sync on SMA output on slot: %d\n", slot);
	}


}

Array<int> Basestation_v1::getSyncFrequencies()
{
	return syncFrequencies;
}

void Basestation_v1::setSyncAsOutput(int freqIndex)
{

	errorCode = setParameter(np::NP_PARAM_SYNCMASTER, slot);
	if (errorCode != np::SUCCESS)
	{
		printf("Failed to set slot %d as sync master!\n", slot);
		return;
	} 

	errorCode = setParameter(np::NP_PARAM_SYNCSOURCE, np::TRIGIN_SYNCCLOCK);
	if (errorCode != np::SUCCESS)
	{
		printf("Failed to set slot %d internal clock as sync source!\n", slot);
		return;
	}

	int freq = syncFrequencies[freqIndex];

	printf("Setting slot %d sync frequency to %d Hz...\n", slot, freq);
	errorCode = setParameter(np::NP_PARAM_SYNCFREQUENCY_HZ, freq);
	if (errorCode != np::SUCCESS)
	{
		printf("Failed to set slot %d sync frequency to %d Hz!\n", slot, freq);
		return;
	}

	errorCode = setTriggerOutput(slot, np::TRIGOUT_SMA, np::TRIGIN_SHAREDSYNC);
	if (errorCode != np::SUCCESS)
	{
		printf("Failed to set sync on SMA output on slot: %d\n", slot);
	}

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

void Basestation_v1::initializeProbes()
{
	if (!probesInitialized)
	{
		errorCode = np::setTriggerInput(slot, np::TRIGIN_SW);

		for (int i = 0; i < probes.size(); i++)
		{
			errorCode = np::setOPMODE(slot, probes[i]->port, np::RECORDING);
			errorCode = np::setHSLed(slot, probes[i]->port, false);

			probes[i]->calibrate();

			if (errorCode == np::SUCCESS)
			{
				std::cout << "     Probe initialized." << std::endl;
				probes[i]->ap_timestamp = 0;
				probes[i]->lfp_timestamp = 0;
				probes[i]->eventCode = 0;
				probes[i]->setStatus(ProbeStatus::CONNECTED);
			}
			else {
				std::cout << "     Failed with error code " << errorCode << std::endl;
			}

		}

		probesInitialized = true;
	}

	errorCode = np::arm(slot);

	
}

void Basestation_v1::startAcquisition()
{
	for (int i = 0; i < probes.size(); i++)
	{
		std::cout << "Probe " << int(probes[i]->port) << " setting timestamp to 0" << std::endl;
		probes[i]->ap_timestamp = 0;
		probes[i]->lfp_timestamp = 0;
		//std::cout << "... and clearing buffers" << std::endl;
		probes[i]->apBuffer->clear();
		probes[i]->lfpBuffer->clear();
		std::cout << "  Starting thread." << std::endl;
		probes[i]->startThread();
	}

	errorCode = np::setSWTrigger(slot);

}

void Basestation_v1::stopAcquisition()
{
	for (int i = 0; i < probes.size(); i++)
	{
		probes[i]->stopThread(1000);
	}

	errorCode = np::arm(slot);
}

void Basestation_v1::setChannels(unsigned char slot_, signed char port, Array<int> channelMap)
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

void Basestation_v1::setApFilterState(unsigned char slot_, signed char port, bool disableHighPass)
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

void Basestation_v1::setGains(unsigned char slot_, signed char port, unsigned char apGain, unsigned char lfpGain)
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

void Basestation_v1::setReferences(unsigned char slot_, signed char port, np::channelreference_t refId, unsigned char refElectrodeBank)
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

bool Basestation_v1::runBist(signed char port, int bistIndex)
{

	bool returnValue = false;

	switch (bistIndex)
	{
	case BIST_SIGNAL:
	{
		np::NP_ErrorCode errorCode = np::bistSignal(slot, port, &returnValue, stats);
		break;
	}
	case BIST_NOISE:
	{
		if (np::bistNoise(slot, port) == np::SUCCESS)
			returnValue = true;
		break;
	}
	case BIST_PSB:
	{
		if (np::bistPSB(slot, port) == np::SUCCESS)
			returnValue = true;
		break;
	}
	case BIST_SR:
	{
		if (np::bistSR(slot, port) == np::SUCCESS)
			returnValue = true;
		break;
	}
	case BIST_EEPROM:
	{
		if (np::bistEEPROM(slot, port) == np::SUCCESS)
			returnValue = true;
		break;
	}
	case BIST_I2C:
	{
		if (np::bistI2CMM(slot, port) == np::SUCCESS)
			returnValue = true;
		break;
	}
	case BIST_SERDES:
	{
		unsigned char errors;
		np::bistStartPRBS(slot, port);
		Sleep(200);
		np::bistStopPRBS(slot, port, &errors);

		if (errors == 0)
			returnValue = true;
		break;
	}
	case BIST_HB:
	{
		if (np::bistHB(slot, port) == np::SUCCESS)
			returnValue = true;
		break;
	} case BIST_BS:
	{
		if (np::bistBS(slot) == np::SUCCESS)
			returnValue = true;
		break;
	} default:
		CoreServices::sendStatusMessage("Test not found.");
	}

	return returnValue;
}