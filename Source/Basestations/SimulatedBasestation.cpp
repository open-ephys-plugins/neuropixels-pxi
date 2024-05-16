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

#include "SimulatedBasestation.h"

#include "../Probes/SimulatedProbe.h"
#include "../Headstages/SimulatedHeadstage.h"

SimulatedBasestationConfigWindow::SimulatedBasestationConfigWindow(SimulatedBasestation* bs_)
	: bs(bs_)
{
	
	for (int i = 0; i < bs->headstage_count; i++)
	{
		ComboBox* comboBox = new ComboBox("Port " + String(i) + " Combo Box");

		comboBox->addItem("Empty", (int) ProbeType::NONE);
		comboBox->addItem("Neuropixels 1.0", (int) ProbeType::NP1);
		comboBox->addItem("Neuropixels NHP (45 mm)", (int)ProbeType::NHP45);
		comboBox->addItem("Neuropixels UHD - Active", (int)ProbeType::UHD2);
		comboBox->addItem("Neuropixels 2.0 1-shank", (int)ProbeType::NP2_1);
		comboBox->addItem("Neuropixels 2.0 4-shank", (int)ProbeType::NP2_4);
		comboBox->addItem("Neuropixels Opto", (int) ProbeType::OPTO);

		if (i == 0)
			comboBox->setSelectedId((int)ProbeType::NP1, dontSendNotification);
		else
			comboBox->setSelectedId((int)ProbeType::NONE, dontSendNotification);

		portComboBoxes.add(comboBox);
		addAndMakeVisible(comboBox);
		comboBox->setBounds(65, 50 + 35 * i, 200, 20);
	}

	acceptButton = std::make_unique<UtilityButton>("LAUNCH", Font("Small Text", 13, Font::plain));
	acceptButton->setBounds(120, 200, 80, 20);
	acceptButton->addListener(this);
	addAndMakeVisible(acceptButton.get());
}

void SimulatedBasestationConfigWindow::paint(Graphics& g) 
{
	g.setColour(Colours::lightgrey);

	g.drawText("PORT", 22, 22, 50, 25, Justification::centred);
	g.drawText("PROBE TYPE", 62, 22, 200, 20, Justification::centred);

	for (int port_index = 0; port_index < bs->headstage_count; port_index++)
	{
		g.drawText(String(port_index + 1), 25, 50 + 35 * port_index, 25, 20, Justification::right);
	}
}

void SimulatedBasestationConfigWindow::buttonClicked(Button* button)
{

	for (int i = 0; i < bs->headstage_count; i++)
		bs->simulatedProbeTypes[i] = (ProbeType)portComboBoxes[i]->getSelectedId();

	if (DialogWindow* dw = findParentComponentOfClass<DialogWindow>())
		dw->exitModalState(0);
}

void SimulatedBasestationConnectBoard::getInfo()
{
	info.boot_version = "SIM 0.0";
	info.version = "SIM 0.0";
	info.part_number = "Simulated BSC";
}


void SimulatedBasestation::getInfo()
{
	info.boot_version = "SIM 0.0";
	info.version = "SIM 0.0";
	info.part_number = "Simulated BS";
}

/// ###########################################

SimulatedBasestation::SimulatedBasestation(NeuropixThread* neuropixThread, 
	DeviceType deviceType, 
	int slot_number) : Basestation(neuropixThread, slot_number)
{
	
	type = BasestationType::SIMULATED;

	if (deviceType == PXI)
		headstage_count = 4;
	else
		headstage_count = 2;

	simulatedProbeTypes[0] = ProbeType::NP1;
	simulatedProbeTypes[1] = ProbeType::NONE;
	simulatedProbeTypes[2] = ProbeType::NONE;
	simulatedProbeTypes[3] = ProbeType::NONE;

	DialogWindow::LaunchOptions options;

	configComponent = std::make_unique<SimulatedBasestationConfigWindow>(this);
	options.content.setOwned(configComponent.get());
	configComponent.release();

	options.content->setSize(320, 250);

	options.dialogTitle = "Configure basestation";
	options.dialogBackgroundColour = Colours::darkgrey;
	options.escapeKeyTriggersCloseButton = true;
	options.useNativeTitleBar = false;
	options.resizable = false;

	int result = options.runModal();

	getInfo();
}

bool SimulatedBasestation::open()
{

	savingDirectory = File();

	basestationConnectBoard = new SimulatedBasestationConnectBoard(this);
	basestationConnectBoard->getInfo();

	for (int i = 0; i < headstage_count; i++)
	{
		switch (simulatedProbeTypes[i])
		{
		case ProbeType::NONE:
			headstages.add(nullptr);
			break;
		case ProbeType::NP1:
			headstages.add(new SimulatedHeadstage(this, i + 1, "PRB_1_4_0480_1", 28948291 + i));
			break;
		case ProbeType::NHP45:
			headstages.add(new SimulatedHeadstage(this, i + 1, "NP1031", 38948291 + i));
			break;
		case ProbeType::UHD1:
			headstages.add(new SimulatedHeadstage(this, i + 1, "NP1100", 48948291 + i));
			break;
		case ProbeType::UHD2:
			headstages.add(new SimulatedHeadstage(this, i + 1, "NP1110", 48948211 + i));
			break;
		case ProbeType::NP2_1:
			headstages.add(new SimulatedHeadstage(this, i + 1, "NP2000", 58948291 + i));
			break;
		case ProbeType::NP2_4:
			headstages.add(new SimulatedHeadstage(this, i + 1, "NP2010", 68948291 + i));
			break;
		case ProbeType::OPTO:
			headstages.add(new SimulatedHeadstage(this, i + 1, "NP1300", 78948291 + i));
			break;
		default:
			headstages.add(new SimulatedHeadstage(this, i + 1, "PRB_1_4_0480_1", 28948291 + i));
		}
	}

	for (auto headstage : headstages)
	{
		if (headstage != nullptr)
		{
			probes.add(headstage->getProbes()[0]);
		}
	}

	LOGD(probes.size(), " total probes ");

	syncFrequencies.add(1);
	syncFrequencies.add(10);

	return true;
}

void SimulatedBasestation::close()
{

}


void SimulatedBasestation::setSyncAsInput()
{

}


void SimulatedBasestation::setSyncAsOutput(int freqIndex)
{

}


void SimulatedBasestation::setSyncAsPassive()
{

}

int SimulatedBasestation::getProbeCount()
{
	return probes.size();
}


void SimulatedBasestation::initialize(bool signalChainIsLoading)
{
	if (!probesInitialized)
	{
		LOGD("Basestation initializing probes...");

		for (auto probe: probes)
		{
			probe->initialize(signalChainIsLoading);
			
		}

		probesInitialized = true;
	}
}


Array<int> SimulatedBasestation::getSyncFrequencies()
{
	return syncFrequencies;
}

float SimulatedBasestation::getFillPercentage()
{
	return 0.0f;
}

void SimulatedBasestation::startAcquisition()
{
	for (int i = 0; i < probes.size(); i++)
	{
		probes[i]->startAcquisition();
	}

}

void SimulatedBasestation::stopAcquisition()
{
	for (int i = 0; i < probes.size(); i++)
	{
		probes[i]->stopAcquisition();
	}

}
