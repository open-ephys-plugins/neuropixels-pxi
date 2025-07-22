/*
------------------------------------------------------------------

This file is part of the Open Ephys GUI
Copyright (C) 2020 Allen Institute for Brain Science and Open Ephys

------------------------------------------------------------------

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

#ifndef __NEUROPIXHS2_H_2C4C2D67__
#define __NEUROPIXHS2_H_2C4C2D67__

#include "../NeuropixComponents.h"

/** 

	Connects to a Neuropixels 2.0 probe

*/
class Headstage2 : public Headstage
{
public:

    /** Constructor */
    Headstage2 (Basestation*, int port);

    /** Gets headstage info */
    void getInfo() override;

    /** No test module available for 2.0 probes, so this is always false */
    bool hasTestModule() override { return false; }

    /** No tests can be run */
    void runTestModule() override {}
};

/** 

	Represents a Neuropixels 2.0 probe flex cable

*/
class Flex2 : public Flex
{
public:
    /** Constructor */
    Flex2 (Headstage*, int dock);

    /** Gets flex info */
    void getInfo() override;
};

#endif // __NEUROPIXHS2_H_2C4C2D67__