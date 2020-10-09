/*
    ------------------------------------------------------------------

    This file is part of the Open Ephys GUI
    Copyright (C) 2016 Allen Institute for Brain Science and Open Ephys

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

#include "NeuropixEditor.h"

#include "NeuropixThread.h"
#include "NeuropixCanvas.h"

EditorBackground::EditorBackground(int numBasestations, bool freqSelectEnabled)
	: numBasestations(numBasestations), freqSelectEnabled(freqSelectEnabled) {}

void EditorBackground::setFreqSelectAvailable(bool isAvailable)
{
	freqSelectEnabled = isAvailable;
}

void EditorBackground::paint(Graphics& g)
{

	for (int i = 0; i < numBasestations; i++)
	{
		g.setColour(Colours::lightgrey);
		g.drawRoundedRectangle(90 * i + 32, 13, 32, 98, 4, 3);
		g.setColour(Colours::darkgrey);
		g.drawRoundedRectangle(90 * i + 32, 13, 32, 98, 4, 1);

		g.setColour(Colours::darkgrey);
		g.setFont(20);
		g.drawText(String(i + 1), 90 * i + 72, 15, 10, 10, Justification::centred);
		g.setFont(8);
		g.drawText(String("0"), 90 * i + 87, 100, 50, 10, Justification::centredLeft);
		g.drawText(String("100"), 90 * i + 87, 60, 50, 10, Justification::centredLeft);
		g.drawText(String("%"), 90 * i + 87, 80, 50, 10, Justification::centredLeft);

		for (int j = 0; j < 4; j++)
		{
			g.setFont(10);
			g.drawText(String(j + 1), 90 * i + 22, 90 - j * 22, 10, 10, Justification::centredLeft);
		}
	}

	g.setColour(Colours::darkgrey);
	g.setFont(10);
	g.drawText(String("MASTER SYNC"), 90 * (numBasestations)+32, 13, 100, 10, Justification::centredLeft);
	g.drawText(String("CONFIG AS"), 90 * (numBasestations)+32, 46, 100, 10, Justification::centredLeft);
	if (freqSelectEnabled)
		g.drawText(String("WITH FREQ"), 90 * (numBasestations)+32, 79, 100, 10, Justification::centredLeft);

}

FifoMonitor::FifoMonitor(int id_, NeuropixThread* thread_) : id(id_), thread(thread_), fillPercentage(0.0)
{
	startTimer(500); // update fill percentage every 0.5 seconds
}

void FifoMonitor::timerCallback()
{
	//std::cout << "Checking fill percentage for monitor " << id << ", slot " << int(slot) << std::endl;

	if (slot != 255)
	{
		setFillPercentage(thread->getFillPercentage(slot));
	}
}


void FifoMonitor::setSlot(unsigned char slot_)
{
	slot = slot_;
}

void FifoMonitor::setFillPercentage(float fill_)
{
	fillPercentage = fill_;

	repaint();
}

void FifoMonitor::paint(Graphics& g)
{
	g.setColour(Colours::grey);
	g.fillRoundedRectangle(0, 0, this->getWidth(), this->getHeight(), 4);
	g.setColour(Colours::lightslategrey);
	g.fillRoundedRectangle(2, 2, this->getWidth()-4, this->getHeight()-4, 2);
	
	g.setColour(Colours::yellow);
	float barHeight = (this->getHeight() - 4) * fillPercentage;
	g.fillRoundedRectangle(2, this->getHeight()-2-barHeight, this->getWidth() - 4, barHeight, 2);
}

ProbeButton::ProbeButton(int id_, NeuropixThread* thread_) : id(id_), thread(thread_), selected(false)
{
	status = ProbeStatus::DISCONNECTED;

	setRadioGroupId(979);

	startTimer(500);
}

void ProbeButton::setSlotAndPort(unsigned char slot_, signed char port_)
{
	slot = slot_;
	port = port_;

	std::cout << "Setting button " << id << " to " << int(slot) << ":" << int(port) << std::endl;

	if (slot == 255 || port == -1)
		connected = false;
	else
		connected = true;
}

void ProbeButton::setSelectedState(bool state)
{
	selected = state;
}

void ProbeButton::paintButton(Graphics& g, bool isMouseOver, bool isButtonDown)
{
	if (isMouseOver && connected)
		g.setColour(Colours::antiquewhite);
	else
		g.setColour(Colours::darkgrey);
	g.fillEllipse(0, 0, 15, 15);

	///g.setGradientFill(ColourGradient(Colours::lightcyan, 0, 0, Colours::lightskyblue, 10,10, true));

	if (status == ProbeStatus::CONNECTED)
	{
		if (selected)
		{
			if (isMouseOver)
				g.setColour(Colours::lightgreen);
			else
				g.setColour(Colours::lightgreen);
		}
		else {
			if (isMouseOver)
				g.setColour(Colours::green);
			else
				g.setColour(Colours::green);
		}
	}
	else if (status == ProbeStatus::CONNECTING)
	{
		if (selected)
		{
			if (isMouseOver)
				g.setColour(Colours::lightsalmon);
			else
				g.setColour(Colours::lightsalmon);
		}
		else {
			if (isMouseOver)
				g.setColour(Colours::orange);
			else
				g.setColour(Colours::orange);
		}
	}
	else {
		g.setColour(Colours::lightgrey);
	}
		
	g.fillEllipse(2, 2, 11, 11);
}

void ProbeButton::setProbeStatus(ProbeStatus status)
{
	this->status = status;

	repaint();

}

ProbeStatus ProbeButton::getProbeStatus()
{
	return status;
}

void ProbeButton::timerCallback()
{

	if (slot != 255)
	{
		setProbeStatus(thread->getProbeStatus(slot, port));
		//std::cout << "Setting for slot: " << String(slot) << " port: " << String(port) << " status: " << String(status) << " selected: " << String(selected) << std::endl;
	}
}

BackgroundLoader::BackgroundLoader(NeuropixThread* thread, NeuropixEditor* editor) 
	: Thread("Neuropix Loader"), np(thread), ed(editor)
{
}

BackgroundLoader::~BackgroundLoader()
{
}

void BackgroundLoader::run()
{
	/* Open the NPX-PXI probe connections in the background to prevent this plugin from blocking the main GUI*/
	np->openConnection();

	/* Apply any saved settings */
	CoreServices::sendStatusMessage("Restoring saved probe settings...");
	np->applyProbeSettingsQueue();

	 /* Remove button callback timers and select first avalable probe by default*/
	bool selectedFirstAvailableProbe = false;
	for (auto button : ed->probeButtons)
	{
		if (button->getProbeStatus() == ProbeStatus::CONNECTED && (!selectedFirstAvailableProbe))
		{
			ed->buttonEvent(button);
			selectedFirstAvailableProbe = true;
		}
		button->stopTimer();
	}

	/* Let the main GUI know the plugin is done initializing */
	MessageManagerLock mml;
	CoreServices::updateSignalChain(ed);
	CoreServices::sendStatusMessage("Neuropix-PXI plugin ready for acquisition!");

}


NeuropixEditor::NeuropixEditor(GenericProcessor* parentNode, NeuropixThread* t, bool useDefaultParameterEditors)
 : VisualizerEditor(parentNode, useDefaultParameterEditors)
{

    thread = t;
    canvas = nullptr;

    tabText = "Neuropix PXI";

	int numBasestations = t->getNumBasestations();

	bool foundFirst = false;

	for (int i = 0; i < 4*numBasestations; i++)
	{
		int slotIndex = i / 4;
		int portIndex = i % 4 + 1;
		int x_pos = slotIndex * 90 + 40;
		int y_pos = 125 - portIndex * 22;

		ProbeButton* p = new ProbeButton(i, thread);
		p->setBounds(x_pos, y_pos, 15, 15);
		p->addListener(this);
		addAndMakeVisible(p);
		probeButtons.add(p);

		p->setSlotAndPort(thread->getSlotForIndex(slotIndex, portIndex), thread->getPortForIndex(slotIndex, portIndex));

	}

	for (int i = 0; i < numBasestations; i++)
	{
		int x_pos = i * 90 + 70;
		int y_pos = 50;

		UtilityButton* b = new UtilityButton("", Font("Small Text", 13, Font::plain));
		b->setBounds(x_pos, y_pos, 30, 20);
		b->addListener(this);
		addAndMakeVisible(b);
		directoryButtons.add(b);

		savingDirectories.add(File());

		FifoMonitor* f = new FifoMonitor(i, thread);
		f->setBounds(x_pos + 2, 75, 12, 50);
		addAndMakeVisible(f);
		f->setSlot(thread->getSlotForIndex(i, -1));
		fifoMonitors.add(f);
	}

	masterSelectBox = new ComboBox("MasterSelectComboBox");
	masterSelectBox->setBounds(90 * (numBasestations)+32, 39, 38, 20);
	for (int i = 0; i < numBasestations; i++)
	{
		masterSelectBox->addItem(String(i + 1), i+1);
	}
	masterSelectBox->setSelectedItemIndex(0, dontSendNotification);
	masterSelectBox->addListener(this);
	addAndMakeVisible(masterSelectBox);

	masterConfigBox = new ComboBox("MasterConfigComboBox");
	masterConfigBox->setBounds(90 * (numBasestations)+32, 72, 78, 20);
	masterConfigBox->addItem(String("INPUT"), 1);
	masterConfigBox->addItem(String("OUTPUT"), 2);
	masterConfigBox->setSelectedItemIndex(0, dontSendNotification);
	masterConfigBox->addListener(this);
	addAndMakeVisible(masterConfigBox);

	Array<int> syncFrequencies = t->getSyncFrequencies();

	freqSelectBox = new ComboBox("FreqSelectComboBox");
	freqSelectBox->setBounds(90 * (numBasestations)+32, 105, 70, 20);
	for (int i = 0; i < syncFrequencies.size(); i++)
	{
		freqSelectBox->addItem(String(syncFrequencies[i])+String(" Hz"), i+1);
	}
	freqSelectBox->setSelectedItemIndex(0, dontSendNotification);
	freqSelectBox->addListener(this);
	addChildComponent(freqSelectBox);

	desiredWidth = 100 * numBasestations + 120;

	background = new EditorBackground(numBasestations, false);
	background->setBounds(0, 15, 500, 150);
	addAndMakeVisible(background);
	background->toBack();
	background->repaint();

	uiLoader = new BackgroundLoader(t, this);
	uiLoader->startThread();
	
}

NeuropixEditor::~NeuropixEditor()
{

}

void NeuropixEditor::collapsedStateChanged()
{
    if (masterConfigBox->getSelectedId() == 1)
        freqSelectBox->setVisible(false);
}

void NeuropixEditor::comboBoxChanged(ComboBox* comboBox)
{

	int slotIndex = masterSelectBox->getSelectedId() - 1;

	if (comboBox == masterSelectBox)
	{
		thread->setMasterSync(slotIndex);
        masterConfigBox->setSelectedItemIndex(0,true);
		freqSelectBox->setVisible(false);
		background->setFreqSelectAvailable(false);
		freqSelectBox->setSelectedItemIndex(0, true);
	}
	else if (comboBox == masterConfigBox)
	{
		bool asOutput = masterConfigBox->getSelectedId() == 2;
		if (asOutput)
		{
			thread->setSyncOutput(slotIndex);
			freqSelectBox->setVisible(true);
			background->setFreqSelectAvailable(true);
		}
		else
		{
			thread->setMasterSync(slotIndex);
			freqSelectBox->setVisible(false);
			background->setFreqSelectAvailable(false);
		}
		freqSelectBox->setSelectedItemIndex(0, true);
	}
	else /* comboBox == freqSelectBox */
	{
		int freqIndex = freqSelectBox->getSelectedId() - 1;
		thread->setSyncFrequency(slotIndex, freqIndex);
	}

	background->repaint();

}

void NeuropixEditor::buttonEvent(Button* button)
{

	if (probeButtons.contains((ProbeButton*) button))
	{
		for (auto button : probeButtons)
		{
			button->setSelectedState(false);
		}
		ProbeButton* probe = (ProbeButton*)button;
		probe->setSelectedState(true);
		thread->setSelectedProbe(probe->slot, probe->port);

		if (canvas != nullptr)
			canvas->setSelectedProbe(probe->slot, probe->port);

		repaint();
	}

    if (!acquisitionIsActive)
    {

		if (directoryButtons.contains((UtilityButton*)button))
		{
			// open file chooser to select the saving location for this basestation
			int slotIndex = directoryButtons.indexOf((UtilityButton*)button);
			File currentDirectory = thread->getDirectoryForSlot(slotIndex);
			FileChooser fileChooser("Select directory for NPX file.", currentDirectory);
			if (fileChooser.browseForDirectory())
			{
				File result = fileChooser.getResult();
				String pathName = result.getFullPathName();
				thread->setDirectoryForSlot(slotIndex, result);

				savingDirectories.set(slotIndex, result);
				UtilityButton* ub = (UtilityButton*)button;
				ub->setLabel(pathName.substring(0, 3));
			}

		}
        
    }
}


void NeuropixEditor::saveEditorParameters(XmlElement* xml)
{
	std::cout << "Saving Neuropix editor." << std::endl;

	XmlElement* xmlNode = xml->createNewChildElement("NEUROPIXELS_EDITOR");

	for (int slot = 0; slot < thread->getNumBasestations(); slot++)
	{
		String directory_name = savingDirectories[slot].getFullPathName();
		if (directory_name.length() == 2)
			directory_name += "\\\\";
		xmlNode->setAttribute("Slot" + String(slot) + "Directory", directory_name);
	}

}

void NeuropixEditor::loadEditorParameters(XmlElement* xml)
{
	forEachXmlChildElement(*xml, xmlNode)
	{
		if (xmlNode->hasTagName("NEUROPIXELS_EDITOR"))
		{
			std::cout << "Found parameters for Neuropixels editor" << std::endl;

			for (int slot = 0; slot < thread->getNumBasestations(); slot++)
			{
				File directory = File(xmlNode->getStringAttribute("Slot" + String(slot) + "Directory"));
				std::cout << "Setting thread directory for slot " << slot << std::endl;
				thread->setDirectoryForSlot(slot, directory);
				directoryButtons[slot]->setLabel(directory.getFullPathName().substring(0, 2));
				savingDirectories.set(slot, directory);
			}
		}
	}
}

Visualizer* NeuropixEditor::createNewCanvas(void)
{
    std::cout << "Button clicked..." << std::endl;
    GenericProcessor* processor = (GenericProcessor*) getProcessor();
    std::cout << "Got processor." << std::endl;
    canvas = new NeuropixCanvas(processor, this, thread);
    std::cout << "Created canvas." << std::endl;
    return canvas;
}

/********************************************/
