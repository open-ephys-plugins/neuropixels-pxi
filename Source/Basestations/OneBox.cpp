/*
    ------------------------------------------------------------------

    This file is part of the Open Ephys GUI
    Copyright (C) 2024 Open Ephys

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

#include "OneBox.h"
#include "../Headstages/Headstage1.h"
#include "../Headstages/Headstage2.h"
#include "../Headstages/Headstage_Analog128.h"
#include "../Headstages/Headstage_Custom384.h"
#include "../Headstages/Headstage_QuadBase.h"
#include "../Probes/Neuropixels1.h"
#include "../Probes/OneBoxADC.h"
#include "../Probes/OneBoxDAC.h"

#define MAXLEN 50

Array<int> OneBox::existing_oneboxes = Array<int>();

void OneBox::getInfo()
{
    Neuropixels::firmware_Info firmwareInfo;

    errorCode = Neuropixels::bs_getFirmwareInfo (slot, &firmwareInfo);

    info.boot_version = String (firmwareInfo.major) + "." + String (firmwareInfo.minor) + String (firmwareInfo.build);

    info.part_number = String (firmwareInfo.name);
}

OneBox::OneBox (NeuropixThread* neuropixThread, int serial_number_) : Basestation (neuropixThread, serial_number)
{
    type = BasestationType::ONEBOX;

    serial_number = serial_number_;

    if (! existing_oneboxes.contains (serial_number))
    {
        existing_oneboxes.add (serial_number);
        LOGC ("Stored OneBox serial number ", serial_number);
    }
    else
    {
        LOGC ("OneBox with serial number ", serial_number, " already connected!");
        return;
    }

    int next_slot = first_available_slot + existing_oneboxes.size() - 1;

    LOGD ("Mapping OneBox with serial number ", serial_number, " to slot ", next_slot);

    errorCode = Neuropixels::mapBS (serial_number, next_slot); // assign to slot ID
    errorCode = Neuropixels::openBS (next_slot);

    if (errorCode == Neuropixels::NO_SLOT)
    {
        LOGD ("NO_SLOT error");
        return;
    }
    else if (errorCode == Neuropixels::IO_ERROR)
    {
        LOGD ("IO_ERROR");
        return;
    }
    else if (errorCode == Neuropixels::WRONG_SLOT)
    {
        LOGD ("WRONG_SLOT error");
        return;
    }

    LOGD ("Successfully mapped OneBox with serial number ", serial_number, " to slot ", next_slot, ", error code: ", errorCode);

    slot = next_slot;
    slot_c = next_slot;

    customPortNames.clear();

    for (int p = 0; p < 4; p++)
    {
        for (int d = 0; d < 2; d++)
        {
            customPortNames.add ("slot" + String (slot) + "-port" + String (p + 1) + "-" + String (d + 1));
        }
    }
}

OneBox::~OneBox()
{
    /* As of API 3.31, closing a v3 basestation does not turn off the SMA output */
    setSyncAsInput();
    close();

    existing_oneboxes.removeFirstMatchingValue (serial_number);
}

bool OneBox::open()
{
    if (serial_number == -1)
        return false;

    errorCode = Neuropixels::openBS (slot);

    if (errorCode == Neuropixels::VERSION_MISMATCH)
    {
        LOGC ("Basestation at slot: ", slot, " API VERSION MISMATCH!");
        return false;
    }
    else if (errorCode == Neuropixels::NO_SLOT)
    {
        LOGC ("No OneBox found at slot ", slot);
        return false;
    }
    else if (errorCode != Neuropixels::SUCCESS)
    {
        LOGC ("Opening OneBox, error code: ", errorCode);
        return false;
    }


    if (errorCode == Neuropixels::SUCCESS)
    {
        getInfo();

        LOGC ("  Opened OneBox on slot ", slot);

        LOGD ("    Searching for probes...");

        searchForProbes();

        LOGD ("    Found ", probes.size(), probes.size() == 1 ? " probe." : " probes.");

        dacSource = std::make_unique<OneBoxDAC> (this);
        adcSource = std::make_unique<OneBoxADC> (this, dacSource.get());
    }

    setSyncAsInput();

    syncFrequencies.clear();
    syncFrequencies.add (1);

    return true;
}

void OneBox::searchForProbes() {

    probes.clear();
    headstages.clear();

    for (int port = 1; port <= 2; port++)
    {
        bool detected = false;

        errorCode = Neuropixels::detectHeadStage (slot, port, &detected); // check for headstage on port

        if (detected && errorCode == Neuropixels::SUCCESS)
        {
            char pn[MAXLEN];
            Neuropixels::readHSPN (slot, port, pn, MAXLEN);

            String hsPartNumber = String (pn);

            LOGDD ("Got part #: ", hsPartNumber);

            Headstage* headstage;

            if (hsPartNumber == "NP2_HS_30") // 1.0 headstage, only one dock
            {
                LOGD ("      Found 1.0 single-dock headstage on port: ", port);
                headstage = new Headstage1 (this, port);
                if (headstage->testModule != nullptr)
                {
                    headstage = nullptr;
                }
            }
            else if (hsPartNumber == "NPNH_HS_30" || hsPartNumber == "NPNH_HS_31") // 128-ch analog headstage
            {
                LOGD ("      Found 128-ch analog headstage on port: ", port);
                headstage = new Headstage_Analog128 (this, port);
            }
            else if (hsPartNumber == "NPNH_HS_00") // custom 384-ch headstage
            {
                LOGC ("      Found 384-ch custom headstage on port: ", port);
                headstage = new Headstage_Custom384 (this, port);
            }
            else if (hsPartNumber == "NPM_HS_30" || hsPartNumber == "NPM_HS_31" || hsPartNumber == "NPM_HS_01") // 2.0 headstage, 2 docks
            {
                LOGD ("      Found 2.0 dual-dock headstage on port: ", port);
                headstage = new Headstage2 (this, port);
            }
            else if (hsPartNumber == "NPM_HS_32") //QuadBase headstage
            {
                LOGC ("      Found 2.0 Phase 2C dual-dock headstage on port: ", port);
                headstage = new Headstage_QuadBase (this, port);
            }
            else
            {
                headstage = nullptr;
            }

            headstages.add (headstage);

            if (headstage != nullptr)
            {
                for (auto probe : headstage->probes)
                {
                    if (probe != nullptr)
                        probes.add (probe);
                }
            }

            continue;
        }
        else
        {
            if (errorCode != Neuropixels::SUCCESS)
            {
                LOGD ("***detectHeadstage failed w/ error code: ", errorCode);
            }
            else if (! detected)
            {
                LOGDD ("  No headstage detected on port: ", port);
            }

            errorCode = Neuropixels::closePort (slot, port); // close port

            headstages.add (nullptr);
        }
    }

}

Array<DataSource*> OneBox::getAdditionalDataSources()
{
    Array<DataSource*> sources;
    sources.add ((DataSource*) adcSource.get());

    return sources;
}

void OneBox::initialize (bool signalChainIsLoading)
{

    LOGD ("Initializing OneBox on slot ", slot);
    Neuropixels::switchmatrix_set (slot, Neuropixels::SM_Output_AcquisitionTrigger, Neuropixels::SM_Input_SWTrigger1, true);

    LOGD ("Initializing probes on slot ", slot);
    if (! probesInitialized)
    {
        for (auto probe : probes)
        {
            probe->initialize (signalChainIsLoading);
        }

        probesInitialized = true;
    }

    LOGD ("Initializing ADC source on slot ", slot);
    adcSource->initialize (signalChainIsLoading);

    errorCode = checkError(Neuropixels::arm (slot), "arm slot " + String(slot));

    if (errorCode != Neuropixels::SUCCESS)
    {
		LOGC ("Failed to arm OneBox on slot ", slot, ", error code = ", errorCode);
	}
    else
    {
		LOGC ("OneBox initialized on slot ", slot);
	}

}

void OneBox::close()
{
    LOGD ("Closing OneBox on slot: ", slot);
    for (auto probe : probes)
    {
        checkError (Neuropixels::closeProbe (slot, probe->headstage->port, probe->dock), "closeProbe");
    }

    checkError(Neuropixels::closeBS (slot), "closeBS slot " + String(slot));

}

void OneBox::setSyncAsInput()
{
    LOGC ("Setting slot ", slot, " sync as input.");

    errorCode = Neuropixels::switchmatrix_clear (slot, Neuropixels::SM_Output_StatusBit);

    if (errorCode != Neuropixels::SUCCESS)
    {
        LOGC ("Failed to clear SM_Output_StatusBit on slot ", slot, ", error code = ", errorCode);
    }

    errorCode = Neuropixels::switchmatrix_clear (slot, Neuropixels::SM_Output_SMA1);

    if (errorCode != Neuropixels::SUCCESS)
    {
        LOGC ("Failed to clear SM_Output_SMA1 on slot ", slot, ", error code = ", errorCode);
    }

    errorCode = Neuropixels::switchmatrix_set (slot, Neuropixels::SM_Output_StatusBit, Neuropixels::SM_Input_SMA1, true);

    if (errorCode != Neuropixels::SUCCESS)
    {
        LOGC ("Failed to connect SM_Output_StatusBit and SM_Input_SMA1 on slot ", slot, ", error code = ", errorCode);
    }
}

Array<int> OneBox::getSyncFrequencies()
{
    return syncFrequencies;
}

void OneBox::setSyncAsOutput (int freqIndex)
{
    LOGC ("Setting slot ", slot, " sync as output.");

    errorCode = Neuropixels::switchmatrix_clear (slot, Neuropixels::SM_Output_StatusBit);

    if (errorCode != Neuropixels::SUCCESS)
    {
        LOGC ("Failed to clear SM_Output_StatusBit on slot ", slot, ", error code = ", errorCode);
    }

    errorCode = Neuropixels::switchmatrix_clear (slot, Neuropixels::SM_Output_SMA1);

    if (errorCode != Neuropixels::SUCCESS)
    {
        LOGC ("Failed to clear SM_Output_SMA1 on slot ", slot, ", error code = ", errorCode);
    }

    errorCode = Neuropixels::switchmatrix_set (slot, Neuropixels::SM_Output_StatusBit, Neuropixels::SM_Input_SyncClk, true);
    
    if (errorCode != Neuropixels::SUCCESS)
    {
        LOGC ("Failed to connect SM_Output_StatusBit and SM_Input_SyncClk on slot ", slot, ", error code = ", errorCode);
    }
    
    errorCode = Neuropixels::switchmatrix_set (slot, Neuropixels::SM_Output_SMA1, Neuropixels::SM_Input_SyncClk, true);

    if (errorCode != Neuropixels::SUCCESS)
    {
        LOGC ("Failed to connect SM_Output_SMA1 and SM_Input_SyncClk on slot ", slot, ", error code = ", errorCode);
    }

    errorCode = Neuropixels::setSyncClockFrequency (slot, syncFrequencies[freqIndex]);

    if (errorCode != Neuropixels::SUCCESS)
    {
        LOGC ("Failed to set SyncClockFrequency slot ", slot, ", error code = ", errorCode);
    }
}

void OneBox::setSyncAsPassive()
{
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

        if (probes[i]->fifoFillPercentage > perc)
            perc = probes[i]->fifoFillPercentage;
    }

    return perc;
}

void OneBox::triggerWaveplayer (bool shouldStart)
{
    if (shouldStart)
    {
        LOGD ("OneBox starting waveplayer.");
        dacSource->playWaveform();
    }
    else
    {
        LOGD ("OneBox stopping waveplayer.");
        dacSource->stopWaveform();
    }
}

void OneBox::startAcquisition()
{
    for (auto probe : probes)
    {
        if (probe->isEnabled)
            probe->startAcquisition();
    }

    adcSource->startAcquisition();

    errorCode = Neuropixels::switchmatrix_set (slot, Neuropixels::SM_Output_AcquisitionTrigger, Neuropixels::SM_Input_SWTrigger1, true);

    LOGD ("OneBox software trigger");
    checkError (Neuropixels::setSWTrigger (slot), "setSWTrigger slot " + String (slot));
    checkError (Neuropixels::arm (slot), "arm slot " + String (slot));
    checkError(Neuropixels::setSWTrigger (slot));

    if (errorCode != Neuropixels::SUCCESS)
    {
        LOGC ("Failed to set SWTrigger slot ", slot, ", error code = ", errorCode);
    }
}

void OneBox::stopAcquisition()
{
    for (auto probe : probes)
    {
        if (probe->isEnabled)
            probe->stopAcquisition();
    }

    adcSource->stopAcquisition();

    errorCode = Neuropixels::arm (slot);
}
