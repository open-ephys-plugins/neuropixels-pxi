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

#include "SimulatedHeadstage.h"
#include "../Probes/SimulatedProbe.h"

void SimulatedHeadstage::getInfo()
{
	info.version = "SIM0.0";
	info.part_number = "Simulated headstage";
}


void SimulatedFlex::getInfo()
{

	info.version = "SIM0.0";
	info.part_number = "Simulated flex";
}


SimulatedHeadstage::SimulatedHeadstage(Basestation* bs, int port, String PN, int SN) : Headstage(bs, port)
{

	getInfo();

	flexCables.add(new SimulatedFlex(this));

	probes.add(new SimulatedProbe(basestation, this, flexCables[0], 0, PN, SN));
	probes[0]->setStatus(SourceStatus::CONNECTING);
}