/*
------------------------------------------------------------------

This file is part of the Open Ephys GUI
Copyright (C) 2024 Allen Institute for Brain Science and Open Ephys

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

#include "Headstage_Ph2C.h"
#include "../Probes/Neuropixels_Ph2C.h"

#define MAXLEN 50

void Headstage_Ph2C::getInfo()
{

	int version_major;
	int version_minor;

	errorCode = Neuropixels::getHSVersion(basestation->slot, port, &version_major, &version_minor);

	info.version = String(version_major) + "." + String(version_minor);

	errorCode = Neuropixels::readHSSN(basestation->slot, port, &info.serial_number);

	char pn[MAXLEN];
	errorCode = Neuropixels::readHSPN(basestation->slot, port, pn, MAXLEN);

	info.part_number = String(pn);

}


void Flex_Ph2C::getInfo()
{

	int version_major;
	int version_minor;

	errorCode = Neuropixels::getFlexVersion(headstage->basestation->slot, 
								   headstage->port, 
								   dock,
								   &version_major, 
								   &version_minor);

	info.version = String(version_major) + "." + String(version_minor);

	char pn[MAXLEN];
	errorCode = Neuropixels::readFlexPN(headstage->basestation->slot, 
								headstage->port,
							    dock,
								pn, 
								MAXLEN);

	info.part_number = String(pn);

}


Headstage_Ph2C::Headstage_Ph2C(Basestation* bs_, int port) : Headstage(bs_, port)
{
	getInfo();

	int count;

	Neuropixels::getHSSupportedProbeCount(basestation->slot, port, &count);

	for (int dock = 1; dock <= count; dock++)
	{
		bool flexDetected;

		Neuropixels::detectFlex(basestation->slot, port, dock, &flexDetected);

		if (flexDetected)
		{
			flexCables.add(new Flex_Ph2C(this, dock));
			Neuropixels_Ph2C* probe = new Neuropixels_Ph2C(basestation, this, flexCables.getLast(), dock);

			if (probe->isValid)
			{
				probe->setStatus(SourceStatus::CONNECTING);
				probes.add(probe);
			}
			else
			{
				delete probe;
				probes.add(nullptr);
			}
				
		}
		else {
			probes.add(nullptr);
		}

	}
	
}

Flex_Ph2C::Flex_Ph2C(Headstage* hs_, int dock) : Flex(hs_, dock)
{
	getInfo();

	errorCode = Neuropixels::SUCCESS;
}