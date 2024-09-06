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

#ifndef __NEUROPIXHS1V3_H_2C4C2D67__
#define __NEUROPIXHS1V3_H_2C4C2D67__

#include "../NeuropixComponents.h"

/** 
    
    Connects to a Neuropixels 1.0 probe

*/
class Headstage1 : public Headstage
{
public:
    /** Constructor */
    Headstage1::Headstage1 (Basestation*, int port);

    /** Reads headstage part number and serial number */
    void getInfo() override;

    /** Returns true if headstage tester is connected */
    bool hasTestModule() override;

    /** Runs the headstage tests */
    void runTestModule() override;
};

/** 
	
    Represents a Neuropixels 1.0 flex cable
        
*/
class Flex1 : public Flex
{
public:
    /** Constructor */
    Flex1::Flex1 (Headstage*);

    /** Reads flex part number */
    void getInfo() override;
};

/** Test module status codes */
typedef struct HST_Status
{
    Neuropixels::NP_ErrorCode VDD_A1V2;
    Neuropixels::NP_ErrorCode VDD_A1V8;
    Neuropixels::NP_ErrorCode VDD_D1V2;
    Neuropixels::NP_ErrorCode VDD_D1V8;
    Neuropixels::NP_ErrorCode MCLK;
    Neuropixels::NP_ErrorCode PCLK;
    Neuropixels::NP_ErrorCode PSB;
    Neuropixels::NP_ErrorCode I2C;
    Neuropixels::NP_ErrorCode NRST;
    Neuropixels::NP_ErrorCode REC_NRESET;
    Neuropixels::NP_ErrorCode SIGNAL;
};

/** 

    Interface to headstage test module

*/
class HeadstageTestModule_v3 : public HeadstageTestModule
{
public:

    /** Constructor */
    HeadstageTestModule_v3::HeadstageTestModule_v3 (Basestation* bs, Headstage* hs);

    /** Gets part info */
    void getInfo() override;

    /** Runs all tests */
    void runAll() override;

    /** Shows test results */
    void showResults() override;

private:
    Neuropixels::NP_ErrorCode test_VDD_A1V2();
    Neuropixels::NP_ErrorCode test_VDD_A1V8();
    Neuropixels::NP_ErrorCode test_VDD_D1V2();
    Neuropixels::NP_ErrorCode test_VDD_D1V8();
    Neuropixels::NP_ErrorCode test_MCLK();
    Neuropixels::NP_ErrorCode test_PCLK();
    Neuropixels::NP_ErrorCode test_PSB();
    Neuropixels::NP_ErrorCode test_I2C();
    Neuropixels::NP_ErrorCode test_NRST();
    Neuropixels::NP_ErrorCode test_REC_NRESET();
    Neuropixels::NP_ErrorCode test_SIGNAL();

    Basestation* basestation;
    Headstage* headstage;
    HeadstageTestModule* testModule;

    std::vector<std::string> tests;

    std::unique_ptr<HST_Status> status;
};

#endif // __NEUROPIXHS1V3_H_2C4C2D67__