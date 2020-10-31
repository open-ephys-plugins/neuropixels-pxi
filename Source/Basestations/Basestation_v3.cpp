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

#include "Basestation_v3.h"
#include "../Probes/Neuropixels1_v3.h"
#include "../Headstages/Headstage1_v3.h"
#include "../Headstages/Headstage2.h"
#include "../Headstages/Headstage_Analog128.h"

#define MAXLEN 50

void Basestation_v3::getInfo()
{
	// NEED TO FIX
	/*unsigned char version_major;
	unsigned char version_minor;
	uint16_t version_build;

	errorCode = Neuropixels::getBSBootVersion(slot, &version_major, &version_minor, &version_build);

	info.boot_version = String(version_major) + "." + String(version_minor);

	if (version_build != NULL)
		info.boot_version += ".";
		info.boot_version += String(version_build);*/

}


void BasestationConnectBoard_v3::getInfo()
{

	int version_major;
	int version_minor;

	errorCode = Neuropixels::getBSCVersion(basestation->slot, &version_major, &version_minor);

	info.version = String(version_major) + "." + String(version_minor);

	errorCode = Neuropixels::readBSCSN(basestation->slot, &info.serial_number);

	char pn[MAXLEN];
	Neuropixels::readBSCPN(basestation->slot, pn, MAXLEN);

	info.part_number = String(pn);

}

BasestationConnectBoard_v3::BasestationConnectBoard_v3(Basestation* bs) : BasestationConnectBoard(bs)
{
}

Basestation_v3::Basestation_v3(int slot_number) : Basestation(slot_number)
{

}

bool Basestation_v3::open()
{
	std::cout << "OPENING PARENT" << std::endl;

	errorCode = Neuropixels::openBS(slot_c);

	if (errorCode == np::VERSION_MISMATCH)
	{
		return false;
	}

	if (errorCode == Neuropixels::SUCCESS)
	{

		std::cout << "  Opened BS on slot " << slot << std::endl;

		basestationConnectBoard = new BasestationConnectBoard_v3(this);

		savingDirectory = File();

		for (signed char port = 1; port <= 4; port++)
		{
			bool detected;

			errorCode = Neuropixels::detectHeadStage(slot, port, &detected); // check for headstage on port

			if (errorCode == np::SUCCESS)
			{
				char pn[MAXLEN];
				Neuropixels::readHSPN(slot, port, pn, MAXLEN);

				String hsPartNumber = String(pn);

				Headstage* headstage;

				if (hsPartNumber == "NP2_HS_30") // 1.0 headstage, only one dock

					headstage = new Headstage1_v3(this, port);

				else if (hsPartNumber == "NPNH_HS_30") // 128-ch analog headstage

					headstage = new Headstage_Analog128(this, port);

				else if (hsPartNumber == "NPM_HS_30") // 2.0 headstage, 2 docks

					headstage = new Headstage2(this, port);

				else
					headstage = nullptr;
				
				headstages.add(headstage);

				for (auto probe : headstage->probes)
				{
					if (probe != nullptr)
						probes.add(probe);
				}
				
				continue;
			}
			else {
				headstages.add(nullptr);
			}

			/*int vmajor, vminor;

			errorCode = Neuropixels::HST_GetVersion(slot, port, &vmajor, &vminor); // is this right?

			if (errorCode == np::SUCCESS)
			{
				if (headstages.getLast() != nullptr)
				{
					ScopedPointer<HeadstageTestModule> hst = new HeadstageTestModule_v1(this, headstages.getLast());
					hst->runAll();
					hst->showResults();
				}
			}*/

		}

		std::cout << "Found " << String(probes.size()) << (probes.size() == 1 ? " probe." : " probes.") << std::endl;
	}

	syncFrequencies.add(1);
	syncFrequencies.add(10);

	return true;
}

void Basestation_v3::initialize()
{

	if (!probesInitialized)
	{
		//errorCode = Neuropixels::setTriggerInput(slot, np::TRIGIN_SW);

		for (auto probe : probes)
		{
			probe->initialize();
		}

		probesInitialized = true;
	}

	errorCode = Neuropixels::arm(slot);
}

Basestation_v3::~Basestation_v3()
{
	close();
	
}

void Basestation_v3::close()
{
	for (auto probe : probes)
	{
		errorCode = Neuropixels::closeProbe(slot, probe->headstage->port, probe->dock);
	}

	errorCode = Neuropixels::closeBS(slot);
}

void Basestation_v3::setSyncAsInput()
{

	/*errorCode = np::setTriggerInput(slot, np::TRIGIN_SW);
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
	}*/


}

Array<int> Basestation_v3::getSyncFrequencies()
{
	return syncFrequencies;
}

void Basestation_v3::setSyncAsOutput(int freqIndex)
{
	/*

	errorCode = setParameter(Neuropixels::NP_PARAM_SYNCMASTER, slot_c);
	if (errorCode != np::SUCCESS)
	{
		printf("Failed to set slot %d as sync master!\n", slot);
		return;
	} 

	errorCode = setParameter(Neuropixels::NP_PARAM_SYNCSOURCE, np::TRIGIN_SYNCCLOCK);
	if (errorCode != np::SUCCESS)
	{
		printf("Failed to set slot %d internal clock as sync source!\n", slot);
		return;
	}

	int freq = syncFrequencies[freqIndex];

	printf("Setting slot %d sync frequency to %d Hz...\n", slot, freq);
	errorCode = setParameter(Neuropixels::NP_PARAM_SYNCFREQUENCY_HZ, freq);
	if (errorCode != np::SUCCESS)
	{
		printf("Failed to set slot %d sync frequency to %d Hz!\n", slot, freq);
		return;
	}

	errorCode = setTriggerOutput(slot, Neuropixels::TRIGOUT_SMA, Neuropixels::TRIGIN_SHAREDSYNC);
	if (errorCode != np::SUCCESS)
	{
		printf("Failed to set sync on SMA output on slot: %d\n", slot);
	}*/

}

int Basestation_v3::getProbeCount()
{
	return probes.size();
}

float Basestation_v3::getFillPercentage()
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

void Basestation_v3::startAcquisition()
{
	for (auto probe : probes)
	{
		probe->startAcquisition();
	}

	errorCode = Neuropixels::setSWTrigger(slot);

}

void Basestation_v3::stopAcquisition()
{
	for (auto probe : probes)
	{
		probe->stopAcquisition();
	}

	errorCode = Neuropixels::arm(slot);
}

void Basestation_v3::updateBscFirmware(String filepath)
{
	//ProgressBar
	//errorCode = Neuropixels::bsc_updateFirmware(slot,
	//							filepath.getCharPointer(),
	//							firmwareUpdateCallback);

}

void Basestation_v3::updateBsFirmware(String filepath)
{
	// ProgressBar
	//errorCode = Neuropixels::bs_updateFirmware(slot,
	//						  filepath.getCharPointer(), 
	//						  firmwareUpdateCallback);

}



bool Basestation_v3::runBist(signed char port, BIST bistType)
{

	bool returnValue = false;
	/*

	switch (bistType)
	{
	case BIST::SIGNAL:
	{
		np::NP_ErrorCode errorCode = np::bistSignal(slot_c, port, &returnValue, stats);
		break;
	}
	case BIST::NOISE:
	{
		if (np::bistNoise(slot_c, port) == np::SUCCESS)
			returnValue = true;
		break;
	}
	case BIST::PSB:
	{
		if (np::bistPSB(slot_c, port) == np::SUCCESS)
			returnValue = true;
		break;
	}
	case BIST::SR:
	{
		if (np::bistSR(slot_c, port) == np::SUCCESS)
			returnValue = true;
		break;
	}
	case BIST::EEPROM:
	{
		if (np::bistEEPROM(slot_c, port) == np::SUCCESS)
			returnValue = true;
		break;
	}
	case BIST::I2C:
	{
		if (np::bistI2CMM(slot_c, port) == np::SUCCESS)
			returnValue = true;
		break;
	}
	case BIST::SERDES:
	{
		unsigned char errors;
		np::bistStartPRBS(slot_c, port);
		Sleep(200);
		np::bistStopPRBS(slot_c, port, &errors);

		if (errors == 0)
			returnValue = true;
		break;
	}
	case BIST::HB:
	{
		if (np::bistHB(slot_c, port) == np::SUCCESS)
			returnValue = true;
		break;
	} case BIST::BS:
	{
		if (np::bistBS(slot_c) == np::SUCCESS)
			returnValue = true;
		break;
	} default:
		CoreServices::sendStatusMessage("Test not found.");
	}*/

	return returnValue;
}