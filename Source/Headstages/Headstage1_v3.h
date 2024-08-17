/*
------------------------------------------------------------------

This file is part of the Open Ephys GUI
Copyright (C) 2020 Allen Institute for Brain Science and Open Ephys

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


#include "../API/v3/NeuropixAPI.h"
#include "../NeuropixComponents.h"


class Headstage1_v3 : public Headstage
{
public:
	Headstage1_v3(Basestation*, int port);
	void getInfo() override;
	bool hasTestModule() override;
	void runTestModule() override;

	Neuropixels::NP_ErrorCode errorCode;
};

class Flex1_v3 : public Flex
{
public:
	Flex1_v3(Headstage*);
	void getInfo() override;

	Neuropixels::NP_ErrorCode errorCode;
};

typedef struct HST_Status {
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

class HeadstageTestModule_v3 : public HeadstageTestModule
{
public:

	HeadstageTestModule_v3(Basestation* bs, Headstage* hs);

	void getInfo() override;

	void runAll() override;
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

	Neuropixels::NP_ErrorCode errorCode;

	ScopedPointer<HST_Status> status;

};

#endif  // __NEUROPIXHS1V3_H_2C4C2D67__