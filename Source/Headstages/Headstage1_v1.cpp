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

#include "Headstage1_v1.h"

#define MAXLEN 50

void Headstage1_v1::getInfo()
{

	unsigned char version_major;
	unsigned char version_minor;

	errorCode = np::getHSVersion(probe->basestation->slot, probe->port, &version_major, &version_minor);

	info.version = String(version_major) + "." + String(version_minor);

	errorCode = np::readHSSN(probe->basestation->slot, probe->port, &info.serial_number);

	char pn[MAXLEN];
	errorCode = np::readHSPN(probe->basestation->slot, probe->port, pn, MAXLEN);

	info.part_number = String(pn);

}


void Flex1_v1::getInfo()
{

	unsigned char version_major;
	unsigned char version_minor;

	errorCode = np::getFlexVersion(probe->basestation->slot, probe->port, &version_major, &version_minor);

	info.version = String(version_major) + "." + String(version_minor);

	char pn[MAXLEN];
	errorCode = np::readFlexPN(probe->basestation->slot, probe->port, pn, MAXLEN);

	info.part_number = String(pn);

}


Headstage1_v1::Headstage1_v1(Probe* probe_) : Headstage(probe_)
{
}

Flex1_v1::Flex1_v1(Probe* probe_) : Flex(probe_)
{
}


/****************Headstage Test Module**************************/

HeadstageTestModule_v1::HeadstageTestModule_v1(Basestation* bs, signed char port) : HeadstageTestModule(bs, port)
{

	slot = basestation->slot;

	this->port = port;

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

}

void HeadstageTestModule_v1::getInfo()
{
	//TODO?
}

void HeadstageTestModule_v1::runAll()
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

void HeadstageTestModule_v1::showResults()
{

	int numTests = sizeof(struct HST_Status)/sizeof(np::NP_ErrorCode);
	int resultLineTextLength = 30;
	String message = "Test results from HST module on slot: " + String(slot) + " port: " + String(port) + "\n\n"; 

	np::NP_ErrorCode *results = (np::NP_ErrorCode*)(&status->VDD_A1V2);
	for (int i = 0; i < numTests; i++)
	{
		message+=String(tests[i]);
		for (int ii = 0; ii < resultLineTextLength - tests[i].length(); ii++) message+="-"; 
		message+=results[i] == np::SUCCESS ? "PASSED" : "FAILED w/ error code: " + String(results[i]);
		message+= "\n";
	}

	AlertWindow::showMessageBox(AlertWindow::AlertIconType::InfoIcon, "HST Module Detected!", message, "OK");

}

np::NP_ErrorCode HeadstageTestModule_v1::test_VDD_A1V2()
{
	return np::HSTestVDDA1V2(slot, port);
}

np::NP_ErrorCode HeadstageTestModule_v1::test_VDD_A1V8()
{
	return np::HSTestVDDA1V8(slot, port);
}

np::NP_ErrorCode HeadstageTestModule_v1::test_VDD_D1V2()
{
	return np::HSTestVDDD1V2(slot, port);
}

np::NP_ErrorCode HeadstageTestModule_v1::test_VDD_D1V8()
{
	return np::HSTestVDDD1V8(slot, port);
}

np::NP_ErrorCode HeadstageTestModule_v1::test_MCLK()
{
	return np::HSTestMCLK(slot, port);
}

np::NP_ErrorCode HeadstageTestModule_v1::test_PCLK()
{
	return np::HSTestPCLK(slot, port);
}

np::NP_ErrorCode HeadstageTestModule_v1::test_PSB()
{
	return np::HSTestPSB(slot, port);
}

np::NP_ErrorCode HeadstageTestModule_v1::test_I2C()
{
	return np::HSTestI2C(slot, port);
}

np::NP_ErrorCode HeadstageTestModule_v1::test_NRST()
{
	return np::HSTestNRST(slot, port);
}

np::NP_ErrorCode HeadstageTestModule_v1::test_REC_NRESET()
{
	return np::HSTestREC_NRESET(slot, port);
}

np::NP_ErrorCode HeadstageTestModule_v1::test_SIGNAL()
{
	return np::HSTestOscillator(slot, port);
}