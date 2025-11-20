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

#include "XDAQ.h"
#include <fmt/format.h>

#include "../Headstages/Headstage1.h"
#include "../Headstages/Headstage2.h"
#include "../Headstages/Headstage_Analog128.h"
#include "../Headstages/Headstage_Custom384.h"
#include "../Headstages/Headstage_QuadBase.h"

void XDAQ_BS::getInfo()
{
    Neuropixels::firmware_Info firmwareInfo;

    errorCode = Neuropixels::bs_getFirmwareInfo (slot, &firmwareInfo);

    info.boot_version = String (fmt::format ("{}/{}/{}", firmwareInfo.major, firmwareInfo.minor, firmwareInfo.build));

    info.part_number = String (firmwareInfo.name);
}

XDAQ_BS::XDAQ_BS (NeuropixThread* neuropixThread, int serial_number) : Basestation (neuropixThread, -1)
{
    type = BasestationType::XDAQ;
    for (int next_slot = 1; next_slot < 16; ++next_slot)
    {
        errorCode = Neuropixels::mapBS (serial_number, next_slot);
        if (errorCode != Neuropixels::SUCCESS)
            continue;
        slot_c = slot = next_slot;
        LOGD ("Successfully mapped XDAQ with serial number ", serial_number, " to slot ", next_slot);
        break;
    }
    if (slot == -1)
    {
        LOGE ("Failed to map XDAQ with serial number ", serial_number);
        return;
    }
    for (int port = 0; port < 4; ++port)
        headstages.add (nullptr);

    customPortNames.clear();

    for (int p = 0; p < 4; p++)
    {
        for (int d = 0; d < 2; d++)
        {
            customPortNames.add ("slot" + String (slot) + "-port" + String (p + 1) + "-" + String (d + 1));
        }
    }

    LOGD ("Stored slot number: ", slot);
}

XDAQ_BS::~XDAQ_BS()
{
    /* As of API 3.31, closing a v3 basestation does not turn off the SMA output */
    setSyncAsInput();
    close();
}

struct XDAQConnectBoard : public BasestationConnectBoard
{
    XDAQConnectBoard (Basestation* bs_) : BasestationConnectBoard (bs_) {}
    void getInfo() override
    {
        basestation->getInfo();
    }
};

bool XDAQ_BS::open()
{
    errorCode = Neuropixels::openBS (slot);

    if (errorCode == Neuropixels::VERSION_MISMATCH)
    {
        LOGC ("Basestation at slot: ", slot, " API VERSION MISMATCH!");
        return false;
    }
    else if (errorCode == Neuropixels::NO_SLOT)
    {
        LOGC ("No XDAQ found at slot ", slot);
        return false;
    }
    else if (errorCode == Neuropixels::SUCCESS)
    {
        basestationConnectBoard = std::make_unique<XDAQConnectBoard> (this);
        getInfo();

        LOGC ("  Opened XDAQ on slot ", slot);

        LOGD ("    Searching for probes...");

        searchForProbes();

        LOGD ("    Found ", probes.size(), probes.size() == 1 ? " probe." : " probes.");

        // adcSource = new OneBoxADC(this);
        // dacSource = new OneBoxDAC(this);
    }
    else
    {
        LOGC ("Failed to open XDAQ, error code: ", errorCode);
        return false;
    }

    setSyncAsInput();

    syncFrequencies.clear();
    syncFrequencies.add (1);

    return true;
}

void XDAQ_BS::searchForProbes()
{
    for (int port = 1; port <= 4; port++)
    {
        bool detected = false;

        errorCode = Neuropixels::detectHeadStage (slot, port, &detected);

        if (detected && errorCode == Neuropixels::SUCCESS)
        {
            Neuropixels::HardwareID hardwareID;
            if (auto r = Neuropixels::getHeadstageHardwareID (slot, port, &hardwareID); r != Neuropixels::SUCCESS)
            {
                LOGE ("Failed to get headstage hardware ID on slot ", slot, ", port ", port, ", error code: ", r);
                headstages.set (port - 1, nullptr, true);
                continue;
            }

            String hsPartNumber = String (hardwareID.ProductNumber);

            LOGDD ("Got part #: ", hsPartNumber);

            Headstage* headstage;

            if (hsPartNumber == "NP2_HS_30")
            { // 1.0 headstage, only one dock

                LOGD ("      Found 1.0 single-dock headstage on port: ", port);
                headstage = new Headstage1 (this, port);
                if (headstage->testModule != nullptr)
                {
                    headstage = nullptr;
                }
            }
            else if (hsPartNumber == "NPNH_HS_30" || hsPartNumber == "NPNH_HS_31")
            { // 128-ch analog headstage
                LOGD ("      Found 128-ch analog headstage on port: ", port);
                LOGE ("      This headstage type is not currently supported on XDAQ, contact KonetX for assistance.");
                // headstage = new Headstage_Analog128 (this, port);
                headstage = nullptr;
            }
            else if (hsPartNumber == "NPNH_HS_00")
            { // custom 384-ch headstage
                LOGC ("      Found 384-ch custom headstage on port: ", port);
                LOGE ("      This headstage type is not currently supported on XDAQ, contact KonetX for assistance.");
                // headstage = new Headstage_Custom384 (this, port);
                headstage = nullptr;
            }
            else if (hsPartNumber == "NPM_HS_30" || hsPartNumber == "NPM_HS_31" || hsPartNumber == "NPM_HS_01")
            { // 2.0 headstage, 2 docks
                LOGD ("      Found 2.0 dual-dock headstage on port: ", port);
                headstage = new Headstage2 (this, port);
            }
            else
            {
                headstage = nullptr;
            }
            headstages.set (port - 1, headstage, true);

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

            headstages.set (port - 1, nullptr, true);
        }
    }
}

Array<DataSource*> XDAQ_BS::getAdditionalDataSources()
{
    Array<DataSource*> sources;
    // sources.add((DataSource*) adcSource);
    //sources.add((DataSource*) dacSource);

    return sources;
}

void XDAQ_BS::initialize (bool signalChainIsLoading)
{
    LOGD ("Initializing probes on slot ", slot);
    if (! probesInitialized)
    {
        for (auto probe : probes)
        {
            probe->initialize (signalChainIsLoading);
        }

        probesInitialized = true;
    }

    // LOGD ("Initializing ADC source on slot ", slot);
    // adcSource->initialize (signalChainIsLoading);

    errorCode = checkError (Neuropixels::arm (slot), "arm slot " + String (slot));

    if (errorCode != Neuropixels::SUCCESS)
    {
        LOGC ("Failed to arm XDAQ on slot ", slot, ", error code = ", errorCode);
    }
    else
    {
        LOGC ("XDAQ initialized on slot ", slot);
    }
}

// bool XDAQ::np_initialize() {
//
//
// 		for (auto probe : probes) {
//
// 			probe->initialize(false);
// 		}
//
// 	//errorCode = Neuropixels::arm(slot);
//
// 	return true;
// }

void XDAQ_BS::close()
{
    for (auto probe : probes)
    {
        errorCode = Neuropixels::closeProbe (slot, probe->headstage->port, probe->dock);
    }

    errorCode = Neuropixels::closeBS (slot);
    probesInitialized = false;
    LOGD ("Closed basestation on slot: ", slot, " w/ error code: ", errorCode);
}

void XDAQ_BS::setSyncAsInput()
{
    return;
    LOGD ("Setting sync as input...");

    errorCode = Neuropixels::switchmatrix_set (slot, Neuropixels::SM_Output_SMA, Neuropixels::SM_Input_SyncClk, false);
    if (errorCode != Neuropixels::SUCCESS)
    {
        LOGC ("Failed to set sync on SMA output on slot: ", slot);
    }

    errorCode = Neuropixels::switchmatrix_set (slot, Neuropixels::SM_Output_StatusBit, Neuropixels::SM_Input_SyncClk, false);
    if (errorCode != Neuropixels::SUCCESS)
    {
        LOGC ("Failed to set sync on SMA input on slot: ", slot);
    }

    //errorCode = Neuropixels::setParameter(Neuropixels::NP_PARAM_SYNCMASTER, slot);
    //if (errorCode != Neuropixels::SUCCESS)
    //{
    //	LOGC("Failed to set slot", slot, "as sync master!");
    //	return;
    //}

    //errorCode = Neuropixels::setParameter(Neuropixels::NP_PARAM_SYNCSOURCE, Neuropixels::SyncSource_SMA);
    //if (errorCode != Neuropixels::SUCCESS)
    //{
    //	LOGC("Failed to set slot ", slot, "SMA as sync source!");
    //}

    errorCode = Neuropixels::switchmatrix_set (slot, Neuropixels::SM_Output_StatusBit, Neuropixels::SM_Input_SMA, true);
    if (errorCode != Neuropixels::SUCCESS)
    {
        LOGD ("Failed to set sync on SMA input on slot: ", slot);
    }
}

Array<int> XDAQ_BS::getSyncFrequencies()
{
    return syncFrequencies;
}
void XDAQ_BS::setSyncAsPassive()
{
}
void XDAQ_BS::setSyncAsOutput (int freqIndex)
{
    // errorCode = Neuropixels::setParameter(Neuropixels::NP_PARAM_SYNCMASTER, slot);
    // if (errorCode != Neuropixels::SUCCESS)
    // {
    // 	LOGC("Failed to set slot ", slot, " as sync master!");
    // 	return;
    // }

    // errorCode = Neuropixels::setParameter(Neuropixels::NP_PARAM_SYNCSOURCE, Neuropixels::SyncSource_Clock);
    // if (errorCode != Neuropixels::SUCCESS)
    // {
    // 	LOGC("Failed to set slot ", slot, " internal clock as sync source!");
    // 	return;
    // }

    // int freq = syncFrequencies[freqIndex];

    // LOGD("Setting slot ", slot, " sync frequency to ", freq, " Hz...");
    // errorCode = Neuropixels::setParameter(Neuropixels::NP_PARAM_SYNCFREQUENCY_HZ, freq);
    // if (errorCode != Neuropixels::SUCCESS)
    // {
    // 	LOGC("Failed to set slot ", slot, " sync frequency to ", freq, " Hz!");
    // 	return;
    // }

    LOGD ("Setting sync as output...");

    errorCode = Neuropixels::switchmatrix_set (slot, Neuropixels::SM_Output_SMA, Neuropixels::SM_Input_SyncClk, true);
    if (errorCode != Neuropixels::SUCCESS)
    {
        LOGC ("Failed to set sync on SMA output on slot: ", slot);
    }
}

int XDAQ_BS::getProbeCount()
{
    return probes.size();
}

float XDAQ_BS::getFillPercentage()
{
    float perc = 0.0;

    for (int i = 0; i < getProbeCount(); i++)
    {
        LOGDD ("Percentage for probe ", i, ": ", probes[i]->fifoFillPercentage);

        if (probes[i]->fifoFillPercentage > perc)
            perc = probes[i]->fifoFillPercentage;
    }

    return perc;
}

// void XDAQ::triggerWaveplayer(bool shouldStart)
//{
//
//	if (shouldStart)
//	{
//		LOGD("XDAQ starting waveplayer.");
//		// dacSource->playWaveform();
//	}
//	else
//	{
//		LOGD("XDAQ stopping waveplayer.");
//		// dacSource->stopWaveform();
//	}
//
//}

void XDAQ_BS::startAcquisition()
{
    for (auto probe : probes)
    {
        probe->startAcquisition();
    }

    //adcSource->startAcquisition();

    LOGD ("XDAQ software trigger");
    errorCode = Neuropixels::setSWTrigger (slot);
}

void XDAQ_BS::stopAcquisition()
{
    for (auto probe : probes)
    {
        probe->stopAcquisition();
    }

    // adcSource->stopAcquisition();

    errorCode = Neuropixels::arm (slot);
}
