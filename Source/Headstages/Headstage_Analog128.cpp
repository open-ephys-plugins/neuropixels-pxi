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

#include "Headstage_Analog128.h"
#include "../Probes/Neuropixels_NHP_Passive.h"
#include "../Utils.h"

#define MAXLEN 50

void Headstage_Analog128::getInfo()
{

	int version_major;
	int version_minor;

	errorCode = Neuropixels::getHSVersion(basestation->slot_c, port_c, &version_major, &version_minor);

	info.version = String(version_major) + "." + String(version_minor);

	errorCode = Neuropixels::readHSSN(basestation->slot_c, port_c, &info.serial_number);

	char pn[MAXLEN];
	errorCode = Neuropixels::readHSPN(basestation->slot_c, port_c, pn, MAXLEN);

	info.part_number = String(pn);

}


void Flex1_NHP::getInfo()
{

	int version_major;
	int version_minor;

	errorCode = Neuropixels::getFlexVersion(headstage->basestation->slot_c, 
								   headstage->port_c, 
								   dock,
								   &version_major, 
								   &version_minor);

	info.version = String(version_major) + "." + String(version_minor);

	char pn[MAXLEN];
	errorCode = Neuropixels::readFlexPN(headstage->basestation->slot_c,
								headstage->port_c, 
								dock,
								pn, 
								MAXLEN);

	info.part_number = String(pn);

}


Headstage_Analog128::Headstage_Analog128(Basestation* bs_, int port) : Headstage(bs_, port)
{
	getInfo();

	flexCables.add(new Flex1_NHP(this));

	probes.add(new Neuropixels_NHP_Passive(basestation, this, flexCables[0]));
	probes[0]->setStatus(ProbeStatus::CONNECTING);
}


Flex1_NHP::Flex1_NHP(Headstage* hs_) : Flex(hs_, 0)
{
	getInfo();

	errorCode = Neuropixels::SUCCESS;
}

