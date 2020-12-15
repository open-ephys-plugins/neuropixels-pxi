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

#include "Headstage1_v3.h"
#include "../Probes/Neuropixels1_v3.h"

#define MAXLEN 50

void Headstage1_v3::getInfo()
{

	int version_major;
	int version_minor;

	errorCode = Neuropixels::getHSVersion(basestation->slot, port, &version_major, &version_minor);

	info.version = String(version_major) + "." + String(version_minor);

	errorCode = Neuropixels::readHSSN(basestation->slot, port, &info.serial_number);

	char pn[MAXLEN];
	errorCode = Neuropixels::readHSPN(basestation->slot, port, pn, MAXLEN);

	info.part_number = String(pn);

}


void Flex1_v3::getInfo()
{

	int version_major;
	int version_minor;

	errorCode = Neuropixels::getFlexVersion(headstage->basestation->slot,
								   headstage->port, 
								   dock,
								   &version_major, 
								   &version_minor);

	info.version = String(version_major) + "." + String(version_minor);

	char pn[MAXLEN];
	errorCode = Neuropixels::readFlexPN(headstage->basestation->slot,
								headstage->port, 
								dock,
								pn, 
								MAXLEN);

	info.part_number = String(pn);

}


Headstage1_v3::Headstage1_v3(Basestation* bs_, int port) : Headstage(bs_, port)
{

	getInfo();

	testModule = nullptr;

	if (hasTestModule())
	{
		testModule = new HeadstageTestModule_v3(basestation, this);
		testModule->runAll();
		testModule->showResults();
	}
	else
	{
		flexCables.add(new Flex1_v3(this));
		probes.add(new Neuropixels1_v3(basestation, this, flexCables[0]));
		probes[0]->setStatus(ProbeStatus::CONNECTING);
	}

}

bool Headstage1_v3::hasTestModule()
{
	std::cout << "Checking for test module..." << std::endl;
	int vmajor;
	int vminor;
	return Neuropixels::HST_GetVersion(basestation->slot, port, &vmajor, &vminor) == Neuropixels::SUCCESS;
}

void Headstage1_v3::runTestModule()
{
	testModule->runAll();
	testModule->showResults();
}

Flex1_v3::Flex1_v3(Headstage* hs_) : Flex(hs_, 0)
{
	getInfo();

	errorCode = Neuropixels::SUCCESS;
}


/****************Headstage Test Module**************************/

HeadstageTestModule_v3::HeadstageTestModule_v3(Basestation* bs, Headstage* hs) : HeadstageTestModule(bs, hs)
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

	status = new HST_Status();

	status->VDD_A1V2 	= test_VDD_A1V2();
	status->VDD_A1V8 	= test_VDD_A1V8();
	status->VDD_D1V2 	= test_VDD_D1V2();
	status->VDD_D1V8 	= test_VDD_D1V8();
	status->MCLK 		= test_MCLK();
	status->PCLK 		= test_PCLK();
	status->PSB 		= test_PSB();
	status->I2C 		= test_I2C();
	status->NRST 		= test_NRST();
	status->REC_NRESET 	= test_REC_NRESET();
	status->SIGNAL 		= test_SIGNAL();

}

void HeadstageTestModule_v3::showResults()
{

	int numTests = sizeof(struct HST_Status)/sizeof(Neuropixels::NP_ErrorCode);
	int resultLineTextLength = 30;
	String message = "Test results from HST module on slot: " + String(basestation->slot) + " port: " + String(headstage->port) + "\n\n"; 

	Neuropixels::NP_ErrorCode *results = (Neuropixels::NP_ErrorCode*)(&status->VDD_A1V2);

	for (int i = 0; i < numTests; i++)
	{
		message+=String(tests[i]);
		for (int ii = 0; ii < resultLineTextLength - tests[i].length(); ii++) message+="-"; 
		message+=results[i] == Neuropixels::SUCCESS ? "PASSED" : "FAILED w/ error code: " + String(results[i]);
		message+= "\n";
	}

	AlertWindow::showMessageBox(AlertWindow::AlertIconType::InfoIcon, "HST Module Detected!", message, "OK");

}

Neuropixels::NP_ErrorCode HeadstageTestModule_v3::test_VDD_A1V2()
{
	return Neuropixels::HSTestVDDA1V2(basestation->slot, headstage->port);
}

Neuropixels::NP_ErrorCode HeadstageTestModule_v3::test_VDD_A1V8()
{
	return Neuropixels::HSTestVDDA1V8(basestation->slot, headstage->port);
}

Neuropixels::NP_ErrorCode HeadstageTestModule_v3::test_VDD_D1V2()
{
	return Neuropixels::HSTestVDDD1V2(basestation->slot, headstage->port);
}

Neuropixels::NP_ErrorCode HeadstageTestModule_v3::test_VDD_D1V8()
{
	return Neuropixels::HSTestVDDD1V8(basestation->slot, headstage->port);
}

Neuropixels::NP_ErrorCode HeadstageTestModule_v3::test_MCLK()
{
	return Neuropixels::HSTestMCLK(basestation->slot, headstage->port);
}

Neuropixels::NP_ErrorCode HeadstageTestModule_v3::test_PCLK()
{
	return Neuropixels::HSTestPCLK(basestation->slot, headstage->port);
}

Neuropixels::NP_ErrorCode HeadstageTestModule_v3::test_PSB()
{
	return Neuropixels::HSTestPSB(basestation->slot, headstage->port);
}

Neuropixels::NP_ErrorCode HeadstageTestModule_v3::test_I2C()
{
	return Neuropixels::HSTestI2C(basestation->slot, headstage->port);
}

Neuropixels::NP_ErrorCode HeadstageTestModule_v3::test_NRST()
{
	return Neuropixels::HSTestNRST(basestation->slot, headstage->port);
}

Neuropixels::NP_ErrorCode HeadstageTestModule_v3::test_REC_NRESET()
{
	return Neuropixels::HSTestREC_NRESET(basestation->slot, headstage->port);
}

Neuropixels::NP_ErrorCode HeadstageTestModule_v3::test_SIGNAL()
{
	return Neuropixels::HSTestOscillator(basestation->slot, headstage->port);
}