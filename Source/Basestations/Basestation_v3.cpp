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
	Neuropixels::firmware_Info firmwareInfo;

	Neuropixels::bs_getFirmwareInfo(slot, &firmwareInfo);

	info.boot_version = String(firmwareInfo.major) + "." + String(firmwareInfo.minor) + String(firmwareInfo.build);
	 
	info.part_number = String(firmwareInfo.name);

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

	Neuropixels::firmware_Info firmwareInfo;
	Neuropixels::bsc_getFirmwareInfo(basestation->slot, &firmwareInfo);
	info.boot_version = String(firmwareInfo.major) + "." + String(firmwareInfo.minor) + String(firmwareInfo.build);

}

BasestationConnectBoard_v3::BasestationConnectBoard_v3(Basestation* bs) : BasestationConnectBoard(bs)
{
	getInfo();
}

Basestation_v3::Basestation_v3(int slot_number) : Basestation(slot_number)
{
	type = BasestationType::V3;

	armBasestation = std::make_unique<ArmBasestation>(slot_number);

	getInfo();
}

bool Basestation_v3::open()
{

	errorCode = Neuropixels::openBS(slot);

	if (errorCode == Neuropixels::VERSION_MISMATCH)
	{
		LOGC("Basestation at slot: ", slot, " API VERSION MISMATCH!");
		return false;
	}

	if (errorCode == Neuropixels::SUCCESS)
	{

		LOGC("  Opened BS on slot ", slot);

		basestationConnectBoard = new BasestationConnectBoard_v3(this);

		//Confirm v3 basestation by BS version 2.0 or greater.
		LOGC("  BS firmware: ", info.boot_version);
		if (std::stod((info.boot_version.toStdString())) < 2.0)
			return false;

		savingDirectory = File();

		LOGC("    Searching for probes...");

		for (int port = 1; port <= 4; port++)
		{

			if (type == BasestationType::OPTO && port > 2)
				break;

			bool detected = false;

			errorCode = Neuropixels::detectHeadStage(slot, port, &detected); // check for headstage on port

			LOGC("Port ", port, " errorCode: ", errorCode);
			LOGC("Detected: ", detected);

			if (detected && errorCode == Neuropixels::SUCCESS)
			{
				LOGC("HS Detected");
				char pn[MAXLEN];
				Neuropixels::readHSPN(slot, port, pn, MAXLEN);

				String hsPartNumber = String(pn);

				LOGC("Got HS part #: ", hsPartNumber);

				Headstage* headstage;

				if (hsPartNumber == "NP2_HS_30" || hsPartNumber == "OPTO_HS_00") // 1.0 headstage, only one dock
				{
					LOGC("      Found 1.0 single-dock headstage on port: ", port);
					headstage = new Headstage1_v3(this, port);
					if (headstage->testModule != nullptr)
					{
						headstage = nullptr;
					}
				}
				else if (hsPartNumber == "NPNH_HS_30") // 128-ch analog headstage
				{
					LOGC("      Found 128-ch analog headstage on port: ", port);
					headstage = new Headstage_Analog128(this, port);
				}
				else if (hsPartNumber == "NPM_HS_30" || hsPartNumber == "NPM_HS_01") // 2.0 headstage, 2 docks
				{
					LOGC("      Found 2.0 dual-dock headstage on port: ", port);
					headstage = new Headstage2(this, port);
				}
				else
				{
					headstage = nullptr;
				}
				
				headstages.add(headstage);

				if (headstage != nullptr)
				{
					for (auto probe : headstage->probes)
					{
						if (probe != nullptr)
						{
							probes.add(probe);

							if (probe->info.part_number.equalsIgnoreCase("NP1300"))
								type = BasestationType::OPTO;
						}
							
					}
				}
			
				continue;
			}
			else  
			{
				if (errorCode != Neuropixels::SUCCESS)
				{
					LOGC("***detectHeadstage failed w/ error code: ", errorCode);
				}
				else
				{
					LOGC("  No headstage detected on port: ", port);
				}
				headstages.add(nullptr);
			}


		}

		LOGC("    Found ", probes.size(), probes.size() == 1 ? " probe." : " probes.");

	}

	syncFrequencies.add(1);
	syncFrequencies.add(10);

	return true;
}

void Basestation_v3::initialize(bool signalChainIsLoading)
{

	if (!probesInitialized)
	{
		//errorCode = Neuropixels::setTriggerInput(slot, Neuropixels::TRIGIN_SW);

		for (auto probe : probes)
		{
			probe->initialize(signalChainIsLoading);
		}

		probesInitialized = true;
	}

	Neuropixels::arm(slot); //armBasestation->startThread();
}

Basestation_v3::~Basestation_v3()
{
	/* As of API 3.31, closing a v3 basestation does not turn off the SMA output */
	setSyncAsInput();
	close();
	
}

void Basestation_v3::close()
{
	for (auto probe : probes)
	{
		errorCode = Neuropixels::closeProbe(slot, probe->headstage->port, probe->dock);
	}

	errorCode = Neuropixels::closeBS(slot);
	LOGD("Closed basestation on slot: ", slot, "w/ error code: ", errorCode);
}

bool Basestation_v3::isBusy()
{
	return armBasestation->isThreadRunning();
}

void Basestation_v3::waitForThreadToExit()
{
	armBasestation->waitForThreadToExit(10000);
}

void Basestation_v3::setSyncAsInput()
{

	LOGD("Setting sync as input...");

	errorCode = Neuropixels::setParameter(Neuropixels::NP_PARAM_SYNCMASTER, slot);
	if (errorCode != Neuropixels::SUCCESS)
	{
		LOGD("Failed to set slot", slot, "as sync master!");
		return;
	}

	errorCode = Neuropixels::setParameter(Neuropixels::NP_PARAM_SYNCSOURCE, Neuropixels::SyncSource_SMA);
	if (errorCode != Neuropixels::SUCCESS)
		LOGD("Failed to set slot ", slot, "SMA as sync source!");

	errorCode = Neuropixels::switchmatrix_set(slot, Neuropixels::SM_Output_SMA, Neuropixels::SM_Input_PXISYNC, false);
	if (errorCode != Neuropixels::SUCCESS)
	{
		LOGD("Failed to set sync on SMA output on slot: ", slot);
	}

}


Array<int> Basestation_v3::getSyncFrequencies()
{
	return syncFrequencies;
}

void Basestation_v3::setSyncAsOutput(int freqIndex)
{

	LOGD("Setting sync as output...");
	
	errorCode = Neuropixels::setParameter(Neuropixels::NP_PARAM_SYNCMASTER, slot);
	if (errorCode != Neuropixels::SUCCESS)
	{
		LOGD("Failed to set slot ",  slot, " as sync master!");
		return;
	} 

	errorCode = Neuropixels::setParameter(Neuropixels::NP_PARAM_SYNCSOURCE, Neuropixels::SyncSource_Clock);
	if (errorCode != Neuropixels::SUCCESS)
	{
		LOGD("Failed to set slot ", slot, " internal clock as sync source!");
		return;
	}

	int freq = syncFrequencies[freqIndex];

	LOGD("Setting slot ", slot, " sync frequency to ", freq, " Hz...");
	errorCode = Neuropixels::setParameter(Neuropixels::NP_PARAM_SYNCFREQUENCY_HZ, freq);
	if (errorCode != Neuropixels::SUCCESS)
	{
		LOGD("Failed to set slot ", slot, " sync frequency to ", freq, " Hz!");
		return;
	}

	errorCode = Neuropixels::switchmatrix_set(slot, Neuropixels::SM_Output_SMA, Neuropixels::SM_Input_PXISYNC, true);
	if (errorCode != Neuropixels::SUCCESS)
	{
		LOGD("Failed to set sync on SMA output on slot: ", slot);
	}

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
		//LOGDD("Percentage for probe ", i, ": ", probes[i]->fifoFillPercentage);

		if (probes[i]->fifoFillPercentage > perc)
			perc = probes[i]->fifoFillPercentage;
	}

	return perc;
}

void Basestation_v3::startAcquisition()
{

	if (armBasestation->isThreadRunning())
		armBasestation->waitForThreadToExit(5000);

	for (auto probe : probes)
	{
		probe->startAcquisition();
	}

	errorCode = Neuropixels::setSWTrigger(slot);

}

void Basestation_v3::stopAcquisition()
{

	LOGC("Basestation stopping acquisition.");

	for (auto probe : probes)
	{
		probe->stopAcquisition();
	}
	
	armBasestation->startThread();
}




void Basestation_v3::selectEmissionSite(int port, int dock, String wavelength, int site)
{

	if (type == BasestationType::OPTO)
	{
		LOGD("Opto basestation on slot ", slot, " selecting emission site on port ", port, ", dock ", dock);

		Neuropixels::wavelength_t wv;

		if (wavelength.equalsIgnoreCase("red"))
		{
			wv = Neuropixels::wavelength_red;
		}
		else if (wavelength.equalsIgnoreCase("blue"))
		{
			wv = Neuropixels::wavelength_blue;
		}
		else {
			LOGD("Wavelength not recognized. No emission site selected.");
			return;
		}

		if (site < -1 || site > 13)
		{
			LOGD(site, ": invalid site number.");
			return;
		}

		errorCode = Neuropixels::setEmissionSite(slot, port, dock, wv, site);

		LOGD(wavelength, " site ", site, " selected with error code ", errorCode);

		errorCode = Neuropixels::getEmissionSite(slot, port, dock, wv, &site);

		LOGD(wavelength, " actual site: ", site, " selected with error code ", errorCode);
	}
}


