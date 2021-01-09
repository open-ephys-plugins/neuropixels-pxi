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

#ifndef __SIMULATEDCOMPONENTS_H_2C4C2D67__
#define __SIMULATEDCOMPONENTS_H_2C4C2D67__

#include <stdio.h>
#include <string.h>

#include "../NeuropixComponents.h"

class SimulatedBasestationConnectBoard;
class SimulatedFlex;
class SimulatedHeadstage;
class SimulatedProbe;


class SimulatedProbe : public Probe
{
public:
	SimulatedProbe(Basestation* bs,
		Headstage* hs,
		Flex* fl,
		int dock,
		String partNumber,
		int serialNumber);

	bool open() override;
	bool close() override;

	void initialize() override;

	void selectElectrodes(ProbeSettings settings, bool shouldWriteConfiguration = true) override;
	void setAllReferences(int referenceIndex, bool shouldWriteConfiguration = true) override;
	void setAllGains(int apGainIndex, int lfpGainIndex, bool shouldWriteConfiguration = true) override;
	void setApFilterState(bool disableHighPass, bool shouldWriteConfiguration = true) override;
	void writeConfiguration() override;

	void startAcquisition() override;
	void stopAcquisition() override;

	bool runBist(BIST bistType) override;

	void calibrate() override;

	void getInfo() override;

	bool generatesLfpData() { return true; }
	bool hasApFilterSwitch() {
		return apFilterSwitch;
	}

	void run() override;

	bool apFilterSwitch;

};



#endif  // __SIMULATEDCOMPONENTS_H_2C4C2D67__