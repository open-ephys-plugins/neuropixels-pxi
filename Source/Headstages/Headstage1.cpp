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

#include "Headstage1.h"
#include "../Probes/Neuropixels1.h"
#include "../Probes/NeuropixelsOpto.h"
#include "../Probes/Neuropixels_NHP_Active.h"
#include "../Probes/Neuropixels_UHD.h"

#define MAXLEN 50

void Headstage1::getInfo()
{
    errorCode = Neuropixels::getHeadstageHardwareID (basestation->slot,
                                                     port,
                                                     &info.hardwareID);

    info.version = String (info.hardwareID.version_Major)
                   + "." + String (info.hardwareID.version_Minor);

    info.part_number = String (info.hardwareID.ProductNumber);
}

void Flex1::getInfo()
{
    errorCode = Neuropixels::getFlexHardwareID (headstage->basestation->slot,
                                                headstage->port,
                                                dock,
                                                &info.hardwareID);

    info.version = String (info.hardwareID.version_Major)
                   + "." + String (info.hardwareID.version_Minor);

    info.part_number = String (info.hardwareID.ProductNumber);
}

Headstage1::Headstage1 (Basestation* bs_, int port) : Headstage (bs_, port)
{
    getInfo();

    testModule = nullptr;

    if (hasTestModule())
    {
        LOGD ("Test module detected");
        testModule = new HeadstageTestModule_v3 (basestation, this);
        testModule->runAll();
        testModule->showResults();
    }
    else
    {
        flexCables.add (new Flex1 (this));

        Neuropixels::HardwareID hardwareID;

        errorCode = Neuropixels::getProbeHardwareID (basestation->slot, 
            port, 
            1, 
            &hardwareID);

        String partNumber = String (hardwareID.ProductNumber);

        LOGC ("   Found probe part number: ", partNumber);

        if (~partNumber.length() > 0)
        {
            // invalid probe part number
            LOGC ("Headstage has no valid probes connected.");
            return;
        }

        if (String (partNumber).equalsIgnoreCase ("NP1300"))
            probes.add (new NeuropixelsOpto (basestation, this, flexCables[0]));
        else if (String (partNumber).equalsIgnoreCase ("NP1110"))
        {
            probes.add (new Neuropixels_UHD (basestation, this, flexCables[0]));
        }
        else if (String (partNumber).equalsIgnoreCase ("NP1010") || String (partNumber).equalsIgnoreCase ("NP1011") || String (partNumber).equalsIgnoreCase ("NP1012") || String (partNumber).equalsIgnoreCase ("NP1013") || String (partNumber).equalsIgnoreCase ("NP1015") || String (partNumber).equalsIgnoreCase ("NP1016") || String (partNumber).equalsIgnoreCase ("NP1020") || String (partNumber).equalsIgnoreCase ("NP1022") || String (partNumber).equalsIgnoreCase ("NP1030") || String (partNumber).equalsIgnoreCase ("NP1032"))
        {
            probes.add (new Neuropixels_NHP_Active (basestation, this, flexCables[0]));
        }
        else
        {
            probes.add (new Neuropixels1 (basestation, this, flexCables[0]));
        }

        if (probes[0]->isValid)
            probes[0]->setStatus (SourceStatus::CONNECTING);
        else
            probes.remove (0, true);

        if (probes.size() != 1)
        {
            LOGC ("Headstage has ", probes.size(), " valid probes connected.");
        }
        else
            LOGC ("Headstage has 1 valid probe connected.");
    }
}

bool Headstage1::hasTestModule()
{
    int vmajor;
    int vminor;
    return Neuropixels::HST_GetVersion (basestation->slot, port, &vmajor, &vminor) == Neuropixels::SUCCESS;
}

void Headstage1::runTestModule()
{
    testModule->runAll();
    testModule->showResults();
}

Flex1::Flex1 (Headstage* hs_) : Flex (hs_, 1)
{
    getInfo();

    errorCode = Neuropixels::SUCCESS;
}

/****************Headstage Test Module**************************/

HeadstageTestModule_v3::HeadstageTestModule_v3 (Basestation* bs, Headstage* hs) : HeadstageTestModule (bs, hs)
{
    tests = {
        "VDDA1V2",
        "VDDA1V8",
        "VDDD1V2",
        "VDDD1V8",
        "MCLK",
        "PCLK",
        "PSB",
        "I2C",
        "NRST",
        "REC_NRESET",
        "SIGNAL"
    };

    basestation = bs;
    headstage = hs;
}

void HeadstageTestModule_v3::getInfo()
{
    //TODO?
}

void HeadstageTestModule_v3::runAll()
{
    status = std::make_unique<HST_Status>();

    status->VDD_A1V2 = test_VDD_A1V2();
    status->VDD_A1V8 = test_VDD_A1V8();
    status->VDD_D1V2 = test_VDD_D1V2();
    status->VDD_D1V8 = test_VDD_D1V8();
    status->MCLK = test_MCLK();
    status->PCLK = test_PCLK();
    status->PSB = test_PSB();
    status->I2C = test_I2C();
    status->NRST = test_NRST();
    status->REC_NRESET = test_REC_NRESET();
    status->SIGNAL = test_SIGNAL();
}

void HeadstageTestModule_v3::showResults()
{
    int numTests = sizeof (struct HST_Status) / sizeof (Neuropixels::NP_ErrorCode);
    int resultLineTextLength = 30;
    String message = "Test results from HST module on slot: " + String (basestation->slot) + " port: " + String (headstage->port) + "\n\n";

    Neuropixels::NP_ErrorCode* results = (Neuropixels::NP_ErrorCode*) (&status->VDD_A1V2);

    for (int i = 0; i < numTests; i++)
    {
        message += String (tests[i]);
        for (int ii = 0; ii < resultLineTextLength - tests[i].length(); ii++)
            message += "-";
        message += results[i] == Neuropixels::SUCCESS ? "PASSED" : "FAILED w/ error code: " + String (results[i]);
        message += "\n";
    }

    //AlertWindow::showMessageBox(AlertWindow::AlertIconType::InfoIcon, "HST Module Detected!", message, "OK");
    LOGC ("Headstage Module Test Results: ", message);
}

Neuropixels::NP_ErrorCode HeadstageTestModule_v3::test_VDD_A1V2()
{
    return Neuropixels::HSTestVDDA1V2 (basestation->slot, headstage->port);
}

Neuropixels::NP_ErrorCode HeadstageTestModule_v3::test_VDD_A1V8()
{
    return Neuropixels::HSTestVDDA1V8 (basestation->slot, headstage->port);
}

Neuropixels::NP_ErrorCode HeadstageTestModule_v3::test_VDD_D1V2()
{
    return Neuropixels::HSTestVDDD1V2 (basestation->slot, headstage->port);
}

Neuropixels::NP_ErrorCode HeadstageTestModule_v3::test_VDD_D1V8()
{
    return Neuropixels::HSTestVDDD1V8 (basestation->slot, headstage->port);
}

Neuropixels::NP_ErrorCode HeadstageTestModule_v3::test_MCLK()
{
    return Neuropixels::HSTestMCLK (basestation->slot, headstage->port);
}

Neuropixels::NP_ErrorCode HeadstageTestModule_v3::test_PCLK()
{
    return Neuropixels::HSTestPCLK (basestation->slot, headstage->port);
}

Neuropixels::NP_ErrorCode HeadstageTestModule_v3::test_PSB()
{
    return Neuropixels::HSTestPSB (basestation->slot, headstage->port);
}

Neuropixels::NP_ErrorCode HeadstageTestModule_v3::test_I2C()
{
    return Neuropixels::HSTestI2C (basestation->slot, headstage->port);
}

Neuropixels::NP_ErrorCode HeadstageTestModule_v3::test_NRST()
{
    return Neuropixels::HSTestNRST (basestation->slot, headstage->port);
}

Neuropixels::NP_ErrorCode HeadstageTestModule_v3::test_REC_NRESET()
{
    return Neuropixels::HSTestREC_NRESET (basestation->slot, headstage->port);
}

Neuropixels::NP_ErrorCode HeadstageTestModule_v3::test_SIGNAL()
{
    return Neuropixels::HSTestOscillator (basestation->slot, headstage->port);
}