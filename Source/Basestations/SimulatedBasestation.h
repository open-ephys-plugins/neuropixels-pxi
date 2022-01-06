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

#ifndef __SIMULATEDBASESTATION_H_2C4C2D67__
#define __SIMULATEDBASESTATION_H_2C4C2D67__

#include "../NeuropixComponents.h"

/** 

	Simulates a PXI basestation when none is connected

*/
class SimulatedBasestation : public Basestation
{
public:

	/** Constructor */
	SimulatedBasestation(int slot);

	/** Destructor */
	~SimulatedBasestation() { }

	/** Gets part number, firmware version, etc.*/
	void getInfo() override;

	/** Opens connection to the basestation */
	bool open() override;

	/** Closes connection to the basestation */
	void close() override;

	/** Initializes probes in a background thread */
	void initialize(bool signalChainIsLoading) override;

	/** Returns the total number of probes connected to this basestation*/
	int getProbeCount() override;

	/** Set basestation SMA connector as input*/
	void setSyncAsInput() override;

	/** Set basestation SMA connector as output (and set frequency)*/
	void setSyncAsOutput(int freqIndex) override;

	/** Starts probe data streaming */
	void startAcquisition() override;

	/** Stops probe data streaming*/
	void stopAcquisition() override;

	/** Returns an array of available frequencies when SMA is in "output" mode */
	Array<int> getSyncFrequencies() override;

	/** Returns the fraction of the basestation FIFO that is filled */
	float getFillPercentage() override;

	/** Updates the basestation firmware (simulated) */
	void updateBsFirmware(File file) override;

	/** Updates the basestation connect board firmware (simulated) */
	void updateBscFirmware(File file) override;

	/** Launches the firmware update thread*/
	void run() override;

};

class SimulatedBasestationConnectBoard : public BasestationConnectBoard
{
public:

	/** Constructor */
	SimulatedBasestationConnectBoard(Basestation* bs) : BasestationConnectBoard(bs) { getInfo(); }

	/** Returns part number, firmware version, etc.*/
	void getInfo() override;
};




#endif  // __SIMULATEDBASESTATION_H_2C4C2D67__