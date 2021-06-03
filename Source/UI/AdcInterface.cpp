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

#include "AdcInterface.h"

AdcInterface::AdcInterface(DataSource* dataSource_, NeuropixThread* thread_, NeuropixEditor* editor_, NeuropixCanvas* canvas_) :
    SettingsInterface(dataSource_, thread_, editor_, canvas_)
{

}

AdcInterface::~AdcInterface() {}

void AdcInterface::startAcquisition()
{

}

void AdcInterface::stopAcquisition()
{

}

void AdcInterface::saveParameters(XmlElement* xml)
{

}

void AdcInterface::loadParameters(XmlElement* xml)
{

}

void AdcInterface::paint(Graphics& g)
{
    g.fillAll(Colours::purple);
}