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

#include "Neuropixels_NHP_Active.h"

#include "../Headstages/Headstage1.h"

#define MAXLEN 50

void Neuropixels_NHP_Active::getInfo()
{
	errorCode = Neuropixels::readProbeSN(basestation->slot, headstage->port, dock, &info.serial_number);

	char pn[MAXLEN];
	errorCode = Neuropixels::readProbePN(basestation->slot_c, headstage->port_c, dock, pn, MAXLEN);

	info.part_number = String(pn);
}

Neuropixels_NHP_Active::Neuropixels_NHP_Active(Basestation* bs, Headstage* hs, Flex* fl) : Probe(bs, hs, fl, 0)
{

	setStatus(ProbeStatus::DISCONNECTED);

	int length = 0

	if (info.part_number == "NP1020" || info.part_number == "NP1021")
		length = 25;
	else if (info.part_number == "NP1030" || info.part_number == "NP1031")
		multi_shank = true;
	else
		CoreServices::sendStatusMessage("Unrecognized part number: " + info.part_number);

}