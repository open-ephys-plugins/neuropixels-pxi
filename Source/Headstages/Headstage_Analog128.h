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


#include "../API/v3/NeuropixAPI.h"
#include "../NeuropixComponents.h"


class Headstage_Analog128 : public Headstage
{
public:
	Headstage_Analog128::Headstage_Analog128(Basestation*, int port);
	void getInfo() override;
	bool hasTestModule() override { return false; }
	void runTestModule() override {}

	Neuropixels::NP_ErrorCode errorCode;
};

class Flex1_NHP : public Flex
{
public:
	Flex1_NHP::Flex1_NHP(Headstage*);
	void getInfo() override;

	Neuropixels::NP_ErrorCode errorCode;
};

#endif  // __NEUROPIXHSNP1V1_H_2C4C2D67__