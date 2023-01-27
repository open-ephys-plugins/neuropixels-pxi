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

#ifndef __NEUROPIXBASESTATIONV3_H_2C4C2D67__
#define __NEUROPIXBASESTATIONV3_H_2C4C2D67__

#include "../API/v3/NeuropixAPI.h"
#include "../NeuropixComponents.h"

# define SAMPLECOUNT 64

/* 
* 
	Thread for arming basestation immediately after acquisition
	ends. This takes a few seconds, so we should do it in a thread
	so acquisition stops promptly.

*/

class Basestation_v3;

class PortChecker : public ThreadPoolJob
{
public:
	PortChecker(int slot_, int port_, Basestation_v3* basestation_) :
		ThreadPoolJob("Port checker for " + String(slot_) + ":" + String(port_))
		, slot(slot_), port(port_), basestation(basestation_), headstage(nullptr) { }

	~PortChecker() {
		signalJobShouldExit();
	}

	JobStatus runJob();

	Headstage* headstage;

private:

	int slot;
	int port;
	Basestation_v3* basestation;
};

class ArmBasestation : public Thread
{
public:
	ArmBasestation(int slot_) : 
		Thread("Arm Basestation in Slot " + String(slot))
		, slot(slot_) { }

	~ArmBasestation() { 
		stopThread(2000);
	}
	
	void run()
	{
		LOGC("Arming PXI slot ", slot, "...");
		Neuropixels::arm(slot);
		LOGC("Arming complete.");
	}

private:

	int slot;
};

/*

	Standard Neuropixels PXI basestation
	running v3 firmware.

*/
class Basestation_v3 : public Basestation

{
public:
	/** Constructor */
	Basestation_v3(int slot);

	/** Destructor */
	~Basestation_v3();

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

	/** Activates a probe emission site (only works for Opto probes) */
	void selectEmissionSite(int port, int dock, String wavelength, int site);

	/** Returns true if the arm basestation thread is running */
	bool isBusy() override;

	/** Waits for the arm basestation thread to exit */
	void waitForThreadToExit() override;

private:
	std::unique_ptr<ArmBasestation> armBasestation;

	Neuropixels::NP_ErrorCode errorCode;

	bool invertOutput;

};

class BasestationConnectBoard_v3 : public BasestationConnectBoard
{
public:

	/** Constructor */
	BasestationConnectBoard_v3(Basestation*);

	/** Returns part number, firmware version, etc.*/
	void getInfo() override;

private:
	Neuropixels::NP_ErrorCode errorCode;
};


#endif  // __NEUROPIXBASESTATIONV3_H_2C4C2D67__