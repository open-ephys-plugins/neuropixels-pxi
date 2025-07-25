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

#include "Headstage2.h"
#include "../Probes/Neuropixels2.h"

#define MAXLEN 50

void Headstage2::getInfo()
{
    errorCode = Neuropixels::getHeadstageHardwareID (basestation->slot,
                                                     port,
                                                     &info.hardwareID);

    info.version = String (info.hardwareID.version_Major)
                   + "." + String (info.hardwareID.version_Minor);

    info.part_number = String (info.hardwareID.ProductNumber);

}

void Flex2::getInfo()
{
    errorCode = Neuropixels::getFlexHardwareID (headstage->basestation->slot,
                                                headstage->port,
                                                dock,
                                                &info.hardwareID);

    info.version = String (info.hardwareID.version_Major)
                   + "." + String (info.hardwareID.version_Minor);

    info.part_number = String (info.hardwareID.ProductNumber);
}

Headstage2::Headstage2 (Basestation* bs_, int port) : Headstage (bs_, port)
{
    getInfo();

    int count;

    Neuropixels::getHSSupportedProbeCount (basestation->slot, port, &count);

    for (int dock = 1; dock <= count; dock++)
    {
        bool flexDetected;

        Neuropixels::detectFlex (basestation->slot, port, dock, &flexDetected);

        if (flexDetected)
        {
            flexCables.add (new Flex2 (this, dock));
            Neuropixels2* probe = new Neuropixels2 (basestation, this, flexCables.getLast(), dock);

            if (probe->isValid)
            {
                probe->setStatus (SourceStatus::CONNECTING);
                probes.add (probe);
            }
            else
            {
                delete probe;
                probes.add (nullptr);
            }
        }
        else
        {
            probes.add (nullptr);
        }
    }
}

Flex2::Flex2 (Headstage* hs_, int dock) : Flex (hs_, dock)
{
    getInfo();

    errorCode = Neuropixels::SUCCESS;
}
