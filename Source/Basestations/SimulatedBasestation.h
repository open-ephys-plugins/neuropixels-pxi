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

class SimulatedBasestation;

/** 

	Interface for configuring the types of probes in the
	Simulated Basestation

*/

class SimulatedBasestationConfigWindow : 
	public Component,
	public Button::Listener
{
public:
	/** Constructor */
	SimulatedBasestationConfigWindow(SimulatedBasestation* bs);

	/** Destructor */
	~SimulatedBasestationConfigWindow() { }

	/** Render the component */
	void paint(Graphics& g) override;

	/** Accepts the configuration and closes the window */
	void buttonClicked(Button* button);

private:

	OwnedArray<ComboBox> portComboBoxes;

	std::unique_ptr<UtilityButton> acceptButton;

	SimulatedBasestation* bs;

};

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

	/** Probes for each slot */
	ProbeType simulatedProbeTypes[4];

private:

	std::unique_ptr<SimulatedBasestationConfigWindow> configComponent;

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