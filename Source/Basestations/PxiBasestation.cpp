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

#include "PxiBasestation.h"
#include "../Headstages/Headstage1.h"
#include "../Headstages/Headstage2.h"
#include "../Headstages/Headstage_Analog128.h"
#include "../Headstages/Headstage_Custom384.h"
#include "../Headstages/Headstage_QuadBase.h"
#include "../Probes/Neuropixels1.h"

#define MAXLEN 50

void PxiBasestation::getInfo()
{
    Neuropixels::firmware_Info firmwareInfo;

    Neuropixels::bs_getFirmwareInfo (slot, &firmwareInfo);

    info.boot_version = String (firmwareInfo.major) + "." + String (firmwareInfo.minor) + String (firmwareInfo.build);

    info.part_number = String (firmwareInfo.name);
}

void BasestationConnectBoard_v3::getInfo()
{
    int version_major;
    int version_minor;

    errorCode = Neuropixels::getBSCVersion (basestation->slot, &version_major, &version_minor);

    info.version = String (version_major) + "." + String (version_minor);

    errorCode = Neuropixels::readBSCSN (basestation->slot, &info.serial_number);

    char pn[MAXLEN];
    Neuropixels::readBSCPN (basestation->slot, pn, MAXLEN);

    info.part_number = String (pn);

    Neuropixels::firmware_Info firmwareInfo;
    Neuropixels::bsc_getFirmwareInfo (basestation->slot, &firmwareInfo);
    info.boot_version = String (firmwareInfo.major) + "." + String (firmwareInfo.minor) + String (firmwareInfo.build);
}

BasestationConnectBoard_v3::BasestationConnectBoard_v3 (Basestation* bs) : BasestationConnectBoard (bs)
{
    getInfo();
}

PxiBasestation::PxiBasestation (NeuropixThread* neuropixThread, int slot_number) : Basestation (neuropixThread, slot_number)
{
    type = BasestationType::PXI;

    armBasestation = std::make_unique<ArmBasestation> (slot_number);

    getInfo();
}

ThreadPoolJob::JobStatus PortChecker::runJob()
{
    bool detected = false;

    Neuropixels::NP_ErrorCode errorCode = Neuropixels::openPort (slot, port);

    errorCode = Neuropixels::detectHeadStage (slot, port, &detected); // check for headstage on port

    if (detected && errorCode == Neuropixels::SUCCESS)
    {
        char pn[MAXLEN];
        Neuropixels::readHSPN (slot, port, pn, MAXLEN);

        String hsPartNumber = String (pn);

        LOGC ("Port ", port, " HS part #: ", hsPartNumber);

        if (hsPartNumber == "NP2_HS_30" || hsPartNumber == "OPTO_HS_00") // 1.0 headstage, only one dock
        {
            LOGC ("      Found 1.0 single-dock headstage on port: ", port);
            headstage = new Headstage1 (basestation, port);
            if (headstage->testModule != nullptr || ! headstage->probes.size())
            {
                headstage = nullptr;
            }
        }
        else if (hsPartNumber == "NPNH_HS_30" || hsPartNumber == "NPNH_HS_31") // 128-ch analog headstage
        {
            LOGC ("      Found 128-ch analog headstage on port: ", port);
            headstage = new Headstage_Analog128 (basestation, port);
        }
        else if (hsPartNumber == "NPNH_HS_00") // custom 384-ch headstage
        {
            LOGC ("      Found 384-ch custom headstage on port: ", port);
            headstage = new Headstage_Custom384 (basestation, port);
        }
        else if (hsPartNumber == "NPM_HS_30" || hsPartNumber == "NPM_HS_31" || hsPartNumber == "NPM_HS_01") // 2.0 headstage, 2 docks
        {
            LOGC ("      Found 2.0 dual-dock headstage on port: ", port);
            headstage = new Headstage2 (basestation, port);
        }
        else if (hsPartNumber == "NPM_HS_32") //QuadBase headstage
        {
            LOGC ("      Found 2.0 Phase 2C dual-dock headstage on port: ", port);
            headstage = new Headstage_QuadBase (basestation, port);
        }
        else
        {
            headstage = nullptr;
        }
    }
    else
    {
        if (errorCode != Neuropixels::SUCCESS)
        {
            LOGC ("***detectHeadstage failed w/ error code: ", errorCode);
        }
        else
        {
            LOGC ("  No headstage detected on port: ", port);
        }

        errorCode = Neuropixels::closePort (slot, port); // close port

        headstage = nullptr;
    }

    return jobHasFinished;
}

bool PxiBasestation::open()
{

    syncFrequencies.clear();
    syncFrequencies.add (1);

    errorCode = Neuropixels::openBS (slot);

    if (errorCode == Neuropixels::VERSION_MISMATCH)
    {
        LOGC ("Basestation at slot: ", slot, " API VERSION MISMATCH!");
        return false;
    }

    if (errorCode == Neuropixels::SUCCESS)
    {
        LOGC ("  Opened BS on slot ", slot);

        basestationConnectBoard = std::make_unique<BasestationConnectBoard_v3> (this);

        //Confirm v3 basestation by BS version 2.0 or greater.
        LOGC ("  BS firmware: ", info.boot_version);
        if (std::stod ((info.boot_version.toStdString())) < 2.0)
            return false;

        invertOutput = false;

        if (! info.boot_version.equalsIgnoreCase (BS_FIRMWARE_VERSION))
        {
            LOGC ("Found basestation firmware version ", info.boot_version);
            LOGC ("Required version is ", BS_FIRMWARE_VERSION);

            return true;
        }

        if (! basestationConnectBoard->info.boot_version.equalsIgnoreCase (BSC_FIRMWARE_VERSION))
        {
            LOGC ("Found basestation connect board firmware version ", basestationConnectBoard->info.boot_version);
            LOGC ("Required version is ", BSC_FIRMWARE_VERSION);

            return true;

        }

        LOGC ("    Searching for probes...");

        probes.clear();
        searchForProbes();

        LOGC ("    Found ", probes.size(), probes.size() == 1 ? " probe." : " probes.");
    }

    //LOGC("Initial switchmatrix status:");
    //print_switchmatrix();

    return true;
}

void PxiBasestation::searchForProbes()
{

    ThreadPool threadPool;
    OwnedArray<PortChecker> portCheckers;

    for (int port = 1; port <= 4; port++)
    {
        if (type == BasestationType::OPTO && port > 2)
            break;

        bool detected = false;

        portCheckers.add (new PortChecker (slot, port, this));
        threadPool.addJob (portCheckers.getLast(), false);
    }

    //LOGC("    Waiting for jobs to finish...");
    while (threadPool.getNumJobs() > 0)
        std::this_thread::sleep_for (std::chrono::milliseconds (100));
    //LOGC("    Jobs finished.");

    int portIndex = 0;

    headstages.clear();

    for (auto portChecker : portCheckers)
    {
        headstages.add (portChecker->headstage);

        if (portChecker->headstage != nullptr)
        {
            for (auto probe : portChecker->headstage->probes)
            {
                if (probe != nullptr)
                {
                    //check if probe serial number already exists
                    probes.add (probe);

                    if (probe->info.part_number.equalsIgnoreCase ("NP1300"))
                        type = BasestationType::OPTO;
                }
            }
        }
    }

    LOGC("*** Found ", probes.size(), " probes on slot ", slot);
}

void PxiBasestation::initialize (bool signalChainIsLoading)
{
    if (! probesInitialized)
    {
        for (auto probe : probes)
        {
            probe->initialize (signalChainIsLoading);
        }

        probesInitialized = true;
    }

    LOGC ("Arming basestation");
    Neuropixels::arm (slot);
    LOGC ("Arming complete");
}

void PxiBasestation::print_switchmatrix()
{
    bool isConnected;

    Neuropixels::switchmatrix_get (slot, Neuropixels::SM_Output_StatusBit, Neuropixels::SM_Input_PXISYNC, &isConnected);
    LOGC ("Slot ", slot, " connection between StatusBit and PXISYNC: ", isConnected);
    Neuropixels::switchmatrix_get (slot, Neuropixels::SM_Output_StatusBit, Neuropixels::SM_Input_SMA, &isConnected);
    LOGC ("Slot ", slot, " connection between StatusBit and SMA: ", isConnected);
    Neuropixels::switchmatrix_get (slot, Neuropixels::SM_Output_StatusBit, Neuropixels::SM_Input_SyncClk, &isConnected);
    LOGC ("Slot ", slot, " connection between StatusBit and SyncClk: ", isConnected);
    Neuropixels::switchmatrix_get (slot, Neuropixels::SM_Output_StatusBit, Neuropixels::SM_Input_None, &isConnected);
    LOGC ("Slot ", slot, " connection between StatusBit and None: ", isConnected);
    Neuropixels::switchmatrix_get (slot, Neuropixels::SM_Output_StatusBit, Neuropixels::SM_Input_TimeStampClk, &isConnected);
    LOGC ("Slot ", slot, " connection between StatusBit and TimestampClk: ", isConnected);
    Neuropixels::switchmatrix_get (slot, Neuropixels::SM_Output_StatusBit, Neuropixels::SM_Input_SMA1, &isConnected);
    LOGC ("Slot ", slot, " connection between StatusBit and SMA1: ", isConnected);
    Neuropixels::switchmatrix_get (slot, Neuropixels::SM_Output_StatusBit, Neuropixels::SM_Input_SWTrigger1, &isConnected);
    LOGC ("Slot ", slot, " connection between StatusBit and SWTrigger1: ", isConnected);

    Neuropixels::switchmatrix_get (slot, Neuropixels::SM_Output_PXISYNC, Neuropixels::SM_Input_PXISYNC, &isConnected);
    LOGC ("Slot ", slot, " connection between PXISYNC and PXISYNC: ", isConnected);
    Neuropixels::switchmatrix_get (slot, Neuropixels::SM_Output_PXISYNC, Neuropixels::SM_Input_SMA, &isConnected);
    LOGC ("Slot ", slot, " connection between PXISYNC and SMA: ", isConnected);
    Neuropixels::switchmatrix_get (slot, Neuropixels::SM_Output_PXISYNC, Neuropixels::SM_Input_SyncClk, &isConnected);
    LOGC ("Slot ", slot, " connection between PXISYNC and SyncClk: ", isConnected);
    Neuropixels::switchmatrix_get (slot, Neuropixels::SM_Output_PXISYNC, Neuropixels::SM_Input_None, &isConnected);
    LOGC ("Slot ", slot, " connection between PXISYNC and None: ", isConnected);
    Neuropixels::switchmatrix_get (slot, Neuropixels::SM_Output_PXISYNC, Neuropixels::SM_Input_TimeStampClk, &isConnected);
    LOGC ("Slot ", slot, " connection between PXISYNC and TimestampClk: ", isConnected);
    Neuropixels::switchmatrix_get (slot, Neuropixels::SM_Output_PXISYNC, Neuropixels::SM_Input_SMA1, &isConnected);
    LOGC ("Slot ", slot, " connection between PXISYNC and SMA1: ", isConnected);
    Neuropixels::switchmatrix_get (slot, Neuropixels::SM_Output_PXISYNC, Neuropixels::SM_Input_SWTrigger1, &isConnected);
    LOGC ("Slot ", slot, " connection between PXISYNC and SWTrigger1: ", isConnected);

    Neuropixels::switchmatrix_get (slot, Neuropixels::SM_Output_SMA, Neuropixels::SM_Input_PXISYNC, &isConnected);
    LOGC ("Slot ", slot, " connection between SMA and PXISYNC: ", isConnected);
    Neuropixels::switchmatrix_get (slot, Neuropixels::SM_Output_SMA, Neuropixels::SM_Input_SMA, &isConnected);
    LOGC ("Slot ", slot, " connection between SMA and SMA: ", isConnected);
    Neuropixels::switchmatrix_get (slot, Neuropixels::SM_Output_SMA, Neuropixels::SM_Input_SyncClk, &isConnected);
    LOGC ("Slot ", slot, " connection between SMA and SyncClk: ", isConnected);
    Neuropixels::switchmatrix_get (slot, Neuropixels::SM_Output_SMA, Neuropixels::SM_Input_None, &isConnected);
    LOGC ("Slot ", slot, " connection between SMA and None: ", isConnected);
    Neuropixels::switchmatrix_get (slot, Neuropixels::SM_Output_SMA, Neuropixels::SM_Input_TimeStampClk, &isConnected);
    LOGC ("Slot ", slot, " connection between SMA and TimestampClk: ", isConnected);
    Neuropixels::switchmatrix_get (slot, Neuropixels::SM_Output_SMA, Neuropixels::SM_Input_SMA1, &isConnected);
    LOGC ("Slot ", slot, " connection between SMA and SMA1: ", isConnected);
    Neuropixels::switchmatrix_get (slot, Neuropixels::SM_Output_SMA, Neuropixels::SM_Input_SWTrigger1, &isConnected);
    LOGC ("Slot ", slot, " connection between SMA and SWTrigger1: ", isConnected);
}

PxiBasestation::~PxiBasestation()
{
    /* As of API 3.31, closing a v3 basestation does not turn off the SMA output */
    setSyncAsPassive();

    close();
}

void PxiBasestation::close()
{
    for (auto probe : probes)
    {
        try {
            errorCode = Neuropixels::closeBS (slot);
            LOGC(" Closing probe ", probe->info.serial_number, " on slot ", slot, " w/ error code: ", errorCode);
        } catch (std::exception& e) {
            LOGC ("Error closing probe: ", e.what());
        }
    }
    probes.clear();
    headstages.clear();

    errorCode = Neuropixels::closeBS (slot);

    LOGC ("Closed basestation on slot: ", slot, " w/ error code: ", errorCode);
}

void PxiBasestation::checkFirmwareVersion()
{
    if (! info.boot_version.equalsIgnoreCase (BS_FIRMWARE_VERSION))
    {
        LOGC ("Found basestation firmware version ", info.boot_version);

        // show popup notification window
        String message = "The basestation on slot " + String (slot) + " has firmware version " + info.boot_version;
        message += ", but version " + String (BS_FIRMWARE_VERSION) + " is required for this plugin. ";
        message += "This is contained in the file named " + String(BS_FIRMWARE_FILENAME) + ". ";
        message += "Please see the Neuropixels PXI page on the Open Ephys GUI documentation site for information on how to perform a firmware update. ";

        AlertWindow::showMessageBox (AlertWindow::AlertIconType::WarningIcon, "Outdated basestation firmware on slot " + String (slot), message, "OK");
    }

    if (! basestationConnectBoard->info.boot_version.equalsIgnoreCase (BSC_FIRMWARE_VERSION))
    {
        LOGC ("Found basestation connect board firmware version ", basestationConnectBoard->info.boot_version);

        // show popup notification window
        String message = "The basestation on slot " + String (slot) + " has basestation firmware version " + basestationConnectBoard->info.boot_version;
        message += ", but version " + String (BSC_FIRMWARE_VERSION) + " is required for this plugin. ";
        message += "This is contained in the file named " + String(BSC_FIRMWARE_FILENAME) + ". ";
        message += "Please see the Neuropixels PXI page on the Open Ephys GUI documentation site for information on how to perform a firmware update.";

        AlertWindow::showMessageBox (AlertWindow::AlertIconType::WarningIcon, "Outdated basestation connect board firmware on slot " + String (slot), message, "OK");
    }
}


bool PxiBasestation::isBusy()
{
    return armBasestation->isThreadRunning();
}

void PxiBasestation::waitForThreadToExit()
{
    armBasestation->waitForThreadToExit (25000);
}

void PxiBasestation::setSyncAsPassive()
{
    LOGC ("Setting slot ", slot, " sync as passive.");

    errorCode = Neuropixels::switchmatrix_clear (slot, Neuropixels::SM_Output_StatusBit);
    errorCode = Neuropixels::switchmatrix_clear (slot, Neuropixels::SM_Output_SMA);
    errorCode = Neuropixels::switchmatrix_clear (slot, Neuropixels::SM_Output_PXISYNC);

    errorCode = Neuropixels::switchmatrix_set (slot, Neuropixels::SM_Output_StatusBit, Neuropixels::SM_Input_PXISYNC, true);
    errorCode = Neuropixels::switchmatrix_set (slot, Neuropixels::SM_Output_SMA, Neuropixels::SM_Input_PXISYNC, true);

    if (invertOutput)
    {
        for (auto probe : probes)
        {
            probe->invertSyncLine = true;
        }
    }

    //print_switchmatrix();
}

void PxiBasestation::setSyncAsInput()
{
    LOGC ("Setting slot ", slot, " sync as input.");

    errorCode = Neuropixels::switchmatrix_clear (slot, Neuropixels::SM_Output_StatusBit);
    errorCode = Neuropixels::switchmatrix_clear (slot, Neuropixels::SM_Output_SMA);
    errorCode = Neuropixels::switchmatrix_clear (slot, Neuropixels::SM_Output_PXISYNC);

    errorCode = Neuropixels::switchmatrix_set (slot, Neuropixels::SM_Output_StatusBit, Neuropixels::SM_Input_SMA, true);
    errorCode = Neuropixels::switchmatrix_set (slot, Neuropixels::SM_Output_PXISYNC, Neuropixels::SM_Input_SMA, true);

    if (errorCode != Neuropixels::SUCCESS)
    {
        LOGC ("Failed to set sync on SMA output on slot: ", slot);
    }

    //print_switchmatrix();
}

Array<int> PxiBasestation::getSyncFrequencies()
{
    return syncFrequencies;
}

void PxiBasestation::setSyncAsOutput (int freqIndex)
{
    LOGC ("Setting slot ", slot, " sync as output.");

    errorCode = Neuropixels::switchmatrix_clear (slot, Neuropixels::SM_Output_StatusBit);
    errorCode = Neuropixels::switchmatrix_clear (slot, Neuropixels::SM_Output_SMA);
    errorCode = Neuropixels::switchmatrix_clear (slot, Neuropixels::SM_Output_PXISYNC);

    errorCode = Neuropixels::switchmatrix_set (slot, Neuropixels::SM_Output_StatusBit, Neuropixels::SM_Input_SyncClk, true);
    errorCode = Neuropixels::switchmatrix_set (slot, Neuropixels::SM_Output_PXISYNC, Neuropixels::SM_Input_SyncClk, true);
    errorCode = Neuropixels::switchmatrix_set (slot, Neuropixels::SM_Output_SMA, Neuropixels::SM_Input_SyncClk, true);

    errorCode = Neuropixels::setSyncClockFrequency (slot, syncFrequencies[freqIndex]);

    if (errorCode != Neuropixels::SUCCESS)
    {
        LOGC ("Failed to set sync on SMA output on slot: ", slot);
    }

    //print_switchmatrix();
}

int PxiBasestation::getProbeCount()
{
    return probes.size();
}

float PxiBasestation::getFillPercentage()
{
    if (neuropixThread->isRefreshing) return 0.0;

    float perc = 0.0;

    for (int i = 0; i < getProbeCount(); i++)
    {
        //LOGDD("Percentage for probe ", i, ": ", probes[i]->fifoFillPercentage);

        if (probes[i]->fifoFillPercentage > perc)
            perc = probes[i]->fifoFillPercentage;
    }

    return perc;
}

void PxiBasestation::startAcquisition()
{
    if (armBasestation->isThreadRunning())
        armBasestation->waitForThreadToExit (25000);

    for (auto probe : probes)
    {
        if (probe->isEnabled)
            probe->startAcquisition();
    }

    errorCode = Neuropixels::setSWTrigger (slot);
}

void PxiBasestation::stopAcquisition()
{
    LOGC ("Basestation stopping acquisition.");

    for (auto probe : probes)
    {
        if (probe->isEnabled)
            probe->stopAcquisition();
    }

    armBasestation->startThread();
}

void PxiBasestation::selectEmissionSite (int port, int dock, String wavelength, int site)
{
    if (type == BasestationType::OPTO)
    {
        LOGD ("Opto basestation on slot ", slot, " selecting emission site on port ", port, ", dock ", dock);

        Neuropixels::wavelength_t wv;

        if (wavelength.equalsIgnoreCase ("red"))
        {
            wv = Neuropixels::wavelength_red;
        }
        else if (wavelength.equalsIgnoreCase ("blue"))
        {
            wv = Neuropixels::wavelength_blue;
        }
        else
        {
            LOGD ("Wavelength not recognized. No emission site selected.");
            return;
        }

        if (site < -1 || site > 13)
        {
            LOGD (site, ": invalid site number.");
            return;
        }

        errorCode = Neuropixels::setEmissionSite (slot, port, dock, wv, site);

        LOGD (wavelength, " site ", site, " selected with error code ", errorCode);

        errorCode = Neuropixels::getEmissionSite (slot, port, dock, wv, &site);

        LOGD (wavelength, " actual site: ", site, " selected with error code ", errorCode);
    }
}
