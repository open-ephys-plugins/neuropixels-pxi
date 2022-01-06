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

#ifndef __NEUROPIXBASESTATIONV1_H_2C4C2D67__
#define __NEUROPIXBASESTATIONV1_H_2C4C2D67__

#include "../API/v1/NeuropixAPI.h"
#include "../NeuropixComponents.h"

# define SAMPLECOUNT 64

/*
*
	Thread for arming basestation immediately after acquisition
	ends. This takes a few seconds, so we should do it in a thread
	so acquisition stops promptly.

*/
class ArmBasestationV1 : public Thread
{
public:
	ArmBasestationV1(unsigned char slot_) :
		Thread("Arm Basestation in Slot " + String(int(slot)))
		, slot(slot_) { }

	~ArmBasestationV1() { }

	void run()
	{
		LOGC("Arming PXI slot ", int(slot), "...");
		np::arm(slot);
		LOGC("Arming complete.");
	}

private:

	unsigned char slot;
};

/*

	Standard Neuropixels PXI basestation
	running v1 firmware.

*/
class Basestation_v1 : public Basestation
{
public:
	/** Constructor */
	Basestation_v1(int slot);

	/** Destructor */
	~Basestation_v1();

	/** Opens connection to the basestation */
	bool open() override;

	/** Closes connection to the basestation */
	void close() override;

	/** Initializes probes in a background thread */
	void initialize(bool signalChainIsLoading) override;

	/** Returns the total number of probes connected to this basestation*/
	int getProbeCount() override;

	/** Gets part number, firmware version, etc.*/
	void getInfo() override;

	/** Set basestation SMA connector as input*/
	void setSyncAsInput() override;

	/** Set basestation SMA connector as output (and set frequency)*/
	void setSyncAsOutput(int freqIndex) override;

	/** Returns an array of available frequencies when SMA is in "output" mode */
	Array<int> getSyncFrequencies() override;

	/** Starts probe data streaming */
	void startAcquisition() override;

	/** Stops probe data streaming*/
	void stopAcquisition() override;

	/** Returns the fraction of the basestation FIFO that is filled */
	float getFillPercentage() override;

	/** Updates the basestation firmware */
	void updateBsFirmware(File file) override;

	/** Updates the basestation connect board firmware */
	void updateBscFirmware(File file) override;

	/** Launches the firmware update thread*/
	void run() override;

	bool isBusy() override;

	void waitForThreadToExit() override;

private:
	np::bistElectrodeStats stats[960];

	np::NP_ErrorCode errorCode;

	std::unique_ptr<ArmBasestationV1> armBasestation;

};

class BasestationConnectBoard_v1 : public BasestationConnectBoard
{
public:

	/** Constructor */
	BasestationConnectBoard_v1(Basestation*);

	/** Returns part number, firmware version, etc.*/
	void getInfo() override;

private:
	np::NP_ErrorCode errorCode;
};




#endif  // __NEUROPIXBASESTATIONV1_H_2C4C2D67__