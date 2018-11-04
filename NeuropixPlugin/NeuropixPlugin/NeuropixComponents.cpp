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

#include "NeuropixComponents.h"

#define MAXLEN 50

NP_ErrorCode errorCode;

NeuropixComponent::NeuropixComponent() : serial_number(-1), part_number(""), version("")
{
}

void NeuropixAPI::getInfo()
{
	unsigned char version_major;
	unsigned char version_minor;
	getAPIVersion(&version_major, &version_minor);

	version = String(version_major) + "." + String(version_minor);
}

void Basestation::getInfo()
{

	unsigned char version_major;
	unsigned char version_minor;
	uint16_t version_build;

	errorCode = getBSBootVersion(slot, &version_major, &version_minor, &version_build);

	boot_version = String(version_major) + "." + String(version_minor);

	if (version_build != NULL)
		boot_version += ".";
		boot_version += String(version_build);

}

void BasestationConnectBoard::getInfo()
{

	unsigned char version_major;
	unsigned char version_minor;
	uint16_t version_build;

	errorCode = getBSCBootVersion(basestation.slot, &version_major, &version_minor, &version_build);

	boot_version = String(version_major) + "." + String(version_minor);

	if (version_build != NULL)
		boot_version += ".";
		boot_version += String(version_build);

	errorCode = getBSCVersion(basestation.slot, &version_major, &version_minor);

	version = String(version_major) + "." + String(version_minor);

	errorCode = readBSCSN(basestation.slot, &serial_number);

	char pn[MAXLEN];
	readBSCPN(basestation.slot, pn, MAXLEN);

	part_number = String(pn);

}

void Headstage::getInfo()
{

	unsigned char version_major;
	unsigned char version_minor;

	errorCode = getHSVersion(probe.basestation.slot, probe.port, &version_major, &version_minor);

	version = String(version_major) + "." + String(version_minor);

	errorCode = readHSSN(probe.basestation.slot, probe.port, &serial_number);

	char pn[MAXLEN];
	errorCode = readHSPN(probe.basestation.slot, probe.port, pn, MAXLEN);

	part_number = String(pn);

}


void Flex::getInfo()
{

	unsigned char version_major;
	unsigned char version_minor;

	errorCode = getFlexVersion(probe.basestation.slot, probe.port, &version_major, &version_minor);

	version = String(version_major) + "." + String(version_minor);

	char pn[MAXLEN];
	errorCode = readFlexPN(probe.basestation.slot, probe.port, pn, MAXLEN);

	part_number = String(pn);

}


void Probe::getInfo()
{

	errorCode = readId(basestation.slot, port, &serial_number);

	char pn[MAXLEN];
	errorCode = readHSPN(basestation.slot, port, pn, MAXLEN);

	part_number = String(pn);
}

Probe::Probe(Basestation bs, signed char port_) : basestation(bs), port(port_)
{

}

Basestation::Basestation(int slot_number)
{
	slot = (unsigned char)slot_number;

	errorCode = openBS(slot);

	if (errorCode == SUCCESS)
	{
		std::cout << "  Opened BS on slot " << int(slot) << std::endl;

		for (signed char port = 1; port < 5; port++)
		{
			errorCode = openProbe(slot, port);

			std::cout << "    Opening probe " << int(port) << ", error code : " << errorCode << std::endl;

			if (errorCode == SUCCESS)
			{
				probes.add(new Probe(port));
				std::cout << "  Success." << std::endl;
			}
		}

	}
	else {
		std::cout << "  Opening BS on slot " << int(slot) << " failed with error code : " << errorCode << std::endl;
	}


}

void Basestation::makeSyncMaster()
{
	errorCode = setParameter(NP_PARAM_SYNCSOURCE, TRIGIN_SMA);
	errorCode = setParameter(NP_PARAM_SYNCMASTER, slot);
}