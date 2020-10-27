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

#ifndef __NEUROPIXHSNP1V1_H_2C4C2D67__
#define __NEUROPIXHSNP1V1_H_2C4C2D67__


#include "../API/v1/NeuropixAPI.h"
#include "../NeuropixComponents.h"


class Headstage1_v1 : public Headstage
{
public:
	Headstage1_v1::Headstage1_v1(Probe*);
	void getInfo() override;
	bool hasTestModule() override { return true; }

	np::NP_ErrorCode errorCode;
};

class Flex1_v1 : public Flex
{
public:
	Flex1_v1::Flex1_v1(Probe*);
	void getInfo() override;

	np::NP_ErrorCode errorCode;
};

typedef struct HST_Status {
	np::NP_ErrorCode VDD_A1V2;
	np::NP_ErrorCode VDD_A1V8;
	np::NP_ErrorCode VDD_D1V2;
	np::NP_ErrorCode VDD_D1V8;
	np::NP_ErrorCode MCLK;
	np::NP_ErrorCode PCLK;
	np::NP_ErrorCode PSB;
	np::NP_ErrorCode I2C;
	np::NP_ErrorCode NRST;
	np::NP_ErrorCode REC_NRESET;
	np::NP_ErrorCode SIGNAL;
};

class HeadstageTestModule_v1 : public HeadstageTestModule
{
public:

	HeadstageTestModule_v1::HeadstageTestModule_v1(Basestation* bs, signed char port);

	void getInfo() override;

	np::NP_ErrorCode test_VDD_A1V2();
	np::NP_ErrorCode test_VDD_A1V8();
	np::NP_ErrorCode test_VDD_D1V2();
	np::NP_ErrorCode test_VDD_D1V8();
	np::NP_ErrorCode test_MCLK();
	np::NP_ErrorCode test_PCLK();
	np::NP_ErrorCode test_PSB();
	np::NP_ErrorCode test_I2C();
	np::NP_ErrorCode test_NRST();
	np::NP_ErrorCode test_REC_NRESET();
	np::NP_ErrorCode test_SIGNAL();

	void runAll() override;
	void showResults() override;

	np::NP_ErrorCode errorCode;

private:
	Basestation* basestation;
	Headstage* headstage;

	unsigned char slot;
	signed char port;

	std::vector<std::string> tests;

	ScopedPointer<HST_Status> status;

};

#endif  // __NEUROPIXHSNP1V1_H_2C4C2D67__