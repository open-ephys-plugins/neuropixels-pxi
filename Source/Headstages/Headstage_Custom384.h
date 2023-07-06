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

#ifndef __NEUROPIXHSCUSTOM_H_2C4C2D67__
#define __NEUROPIXHSCUSTOM_H_2C4C2D67__


#include "../API/v3/NeuropixAPI.h"
#include "../NeuropixComponents.h"


/**

	Custom headstage with 384-channel Neuropixels chip

*/
class Headstage_Custom384 : public Headstage
{
public:

	/** Constructor */
	Headstage_Custom384::Headstage_Custom384(NeuropixThread* neuropixThread, Basestation*, int port);

	/** Get part numbers*/
	void getInfo() override;

	/** Override hasTestModule() */
	bool hasTestModule() override { return false; }

	/** No test module available*/
	void runTestModule() override {}

	Neuropixels::NP_ErrorCode errorCode;
};

/** 

	Custom Flex cable 
*/
class Flex1_Custom : public Flex
{
public:

	/** Constructor */
	Flex1_Custom::Flex1_Custom(Headstage*);

	/** Get part number */
	void getInfo() override;

	Neuropixels::NP_ErrorCode errorCode;
};

#endif  // __NEUROPIXHSNP1V1_H_2C4C2D67__