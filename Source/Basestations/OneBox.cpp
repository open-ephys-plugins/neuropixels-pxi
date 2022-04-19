/*
------------------------------------------------------------------

This file is part of the Open Ephys GUI
Copyright (C) 2021 Allen Institute for Brain Science and Open Ephys

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

#include "OneBox.h"
#include "../Probes/Neuropixels1_v3.h"
#include "../Headstages/Headstage1_v3.h"
#include "../Headstages/Headstage2.h"
#include "../Headstages/Headstage_Analog128.h"
#include "../Probes/OneBoxADC.h"
#include "../Probes/OneBoxDAC.h"

#define MAXLEN 50

int OneBox::box_count = 0;

void OneBox::getInfo()
{
	Neuropixels::firmware_Info firmwareInfo;

	errorCode = Neuropixels::bs_getFirmwareInfo(slot, &firmwareInfo);

	info.boot_version = String(firmwareInfo.major) + "." + String(firmwareInfo.minor) + String(firmwareInfo.build);
	 
	info.part_number = String(firmwareInfo.name);

}

OneBox::OneBox(int ID) : Basestation(16)
{

	type = BasestationType::ONEBOX;

	errorCode = Neuropixels::mapBS(ID, first_available_slot + box_count); // assign to slot ID

	if (errorCode == Neuropixels::NO_SLOT || errorCode == Neuropixels::IO_ERROR)
	{
		LOGD("NO_SLOT error");
		return;
	}
		
	LOGD("Mapped basestation ", ID, " to slot ", first_available_slot + box_count, ", error code: ", errorCode);

	LOGD("Stored slot number: ", slot);
	slot = 16;
	LOGD("Stored slot number: ", slot);

	getInfo();

	box_count++;
}

OneBox::~OneBox()
{
	/* As of API 3.31, closing a v3 basestation does not turn off the SMA output */
	setSyncAsInput();
	close();

}

bool OneBox::open()
{

	if (box_count == 0)
		return false;

	errorCode = Neuropixels::openBS(slot);

	if (errorCode == Neuropixels::VERSION_MISMATCH)
	{
		LOGD("Basestation at slot: ", slot, " API VERSION MISMATCH!");
		return false;
	}
	else if (errorCode != Neuropixels::SUCCESS)
	{
		LOGD("Opening OneBox, error code: ", errorCode);
	}

	if (errorCode == Neuropixels::SUCCESS)
	{

		//Confirm v3 basestation by BS version 2.0 or greater.
		LOGD("  BS firmware: ", info.boot_version);
		if (std::stod((info.boot_version.toStdString())) < 2.0)
			return false;

		savingDirectory = File();

		LOGD("    Searching for probes...");

		for (int port = 1; port <= 2; port++)
		{
			bool detected = false;

			errorCode = Neuropixels::detectHeadStage(slot, port, &detected); // check for headstage on port

			if (detected && errorCode == Neuropixels::SUCCESS)
			{

				char pn[MAXLEN];
				Neuropixels::readHSPN(slot, port, pn, MAXLEN);

				String hsPartNumber = String(pn);

				LOGDD("Got part #: ", hsPartNumber);

				Headstage* headstage;

				if (hsPartNumber == "NP2_HS_30") // 1.0 headstage, only one dock
				{
					LOGD("      Found 1.0 single-dock headstage on port: ", port);
					headstage = new Headstage1_v3(this, port);
					if (headstage->testModule != nullptr)
					{
						headstage = nullptr;
					}
				}
				else if (hsPartNumber == "NPNH_HS_30") // 128-ch analog headstage
				{
					LOGD("      Found 128-ch analog headstage on port: ", port);
					headstage = new Headstage_Analog128(this, port);
				}
				else if (hsPartNumber == "NPM_HS_30" || hsPartNumber == "NPM_HS_01") // 2.0 headstage, 2 docks
				{
					LOGD("      Found 2.0 dual-dock headstage on port: ", port);
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
							probes.add(probe);
					}
				}
			
				continue;
			}
			else  
			{
				if (errorCode != Neuropixels::SUCCESS)
				{
					LOGD("***detectHeadstage failed w/ error code: ", errorCode);
				}
				else if (!detected)
				{
					LOGDD("  No headstage detected on port: ", port);
				}
				headstages.add(nullptr);
			}


		}

		LOGD("    Found ", probes.size(), probes.size() == 1 ? " probe." : " probes.");

		adcSource = new OneBoxADC(this);
		dacSource = new OneBoxDAC(this);

		adcSource->dac = dacSource;
		dacSource->adc = adcSource;
		
		headstages.add(nullptr);
		headstages.add(nullptr);
	}

	syncFrequencies.add(1);
	syncFrequencies.add(10);

	return true;
}

Array<DataSource*> OneBox::getAdditionalDataSources()
{
	Array<DataSource*> sources;
	sources.add((DataSource*) adcSource);
	//sources.add((DataSource*) dacSource);

	return sources;
}

void OneBox::initialize(bool signalChainIsLoading)
{

	Neuropixels::switchmatrix_set(slot, Neuropixels::SM_Output_AcquisitionTrigger, Neuropixels::SM_Input_SWTrigger1, true);

	if (!probesInitialized)
	{
		//errorCode = Neuropixels::setTriggerInput(slot, Neuropixels::TRIGIN_SW);

		for (auto probe : probes)
		{
			probe->initialize(signalChainIsLoading);
		}

		probesInitialized = true;
	}

	adcSource->initialize(signalChainIsLoading);

	errorCode = Neuropixels::arm(slot);
	LOGD("One box is armed");
}



void OneBox::close()
{
	for (auto probe : probes)
	{
		errorCode = Neuropixels::closeProbe(slot, probe->headstage->port, probe->dock);
	}

	errorCode = Neuropixels::closeBS(slot);
	LOGD("Closed basestation on slot: ", slot, "w/ error code: ", errorCode);
}

void OneBox::setSyncAsInput()
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

	errorCode = Neuropixels::switchmatrix_set(slot, Neuropixels::SM_Output_StatusBit, Neuropixels::SM_Input_SMA, false);

	if (errorCode != Neuropixels::SUCCESS)
	{
		LOGD("Failed to set sync on SMA output on slot: ", slot);
	}

}

Array<int> OneBox::getSyncFrequencies()
{
	return syncFrequencies;
}

void OneBox::setSyncAsOutput(int freqIndex)
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

	errorCode = Neuropixels::switchmatrix_set(slot, Neuropixels::SM_Output_SMA1, Neuropixels::SM_Input_SyncClk, true);

	if (errorCode != Neuropixels::SUCCESS)
	{
		LOGD("Failed to set sync on SMA output on slot: ", slot);
	}

}

int OneBox::getProbeCount()
{
	return probes.size();
}

float OneBox::getFillPercentage()
{
	float perc = 0.0;

	for (int i = 0; i < getProbeCount(); i++)
	{
		LOGDD("Percentage for probe ", i, ": ", probes[i]->fifoFillPercentage);

		if (probes[i]->fifoFillPercentage > perc)
			perc = probes[i]->fifoFillPercentage;
	}

	return perc;
}

void OneBox::triggerWaveplayer(bool shouldStart)
{

	if (shouldStart)
	{
		LOGD("OneBox starting waveplayer.");
		dacSource->playWaveform();
	}
	else
	{
		LOGD("OneBox stopping waveplayer.");
		dacSource->stopWaveform();
	}
		
}

void OneBox::startAcquisition()
{
	for (auto probe : probes)
	{
		probe->startAcquisition();
	}

	adcSource->startAcquisition();

	LOGD("OneBox software trigger");
	errorCode = Neuropixels::setSWTrigger(slot);

}

void OneBox::stopAcquisition()
{
	for (auto probe : probes)
	{
		probe->stopAcquisition();
	}

	adcSource->stopAcquisition();

	errorCode = Neuropixels::arm(slot);
}

