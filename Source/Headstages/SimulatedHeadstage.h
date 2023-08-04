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

#ifndef __SIMULATEDHEADSTAGE_H_2C4C2D67__
#define __SIMULATEDHEADSTAGE_H_2C4C2D67__

#include <stdio.h>
#include <string.h>

#include "../NeuropixComponents.h"

class SimulatedBasestationConnectBoard;
class SimulatedFlex;
class SimulatedHeadstage;
class SimulatedProbe;

class SimulatedHeadstage : public Headstage
{
public:
	SimulatedHeadstage(Basestation* bs, int port, String PN, int SN);
	void getInfo() override;
	bool hasTestModule() override { return false; }
	void runTestModule() override {}
};

class SimulatedFlex : public Flex
{
public:
	SimulatedFlex(Headstage* headstage) : Flex(headstage, 0) { getInfo(); }
	void getInfo() override;
};


#endif  // __SIMULATEDCOMPONENTS_H_2C4C2D67__