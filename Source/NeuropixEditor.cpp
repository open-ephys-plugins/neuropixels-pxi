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


EditorBackground::EditorBackground(Array<Basestation*> basestations_, bool freqSelectEnabled)
	: basestations(basestations_), numBasestations(basestations_.size()), freqSelectEnabled(freqSelectEnabled) {}

void EditorBackground::setFreqSelectAvailable(bool isAvailable)
{
	freqSelectEnabled = isAvailable;
}

void EditorBackground::paint(Graphics& g)
{

	if (numBasestations > 0)
	{
		for (int i = 0; i < numBasestations; i++)
		{
			g.setColour(Colours::lightgrey);
			g.drawRoundedRectangle(90 * i + 32, 13, 32, 98, 4, 3);
			g.setColour(Colours::darkgrey);
			g.drawRoundedRectangle(90 * i + 32, 13, 32, 98, 4, 1);

			g.setColour(Colours::darkgrey);
			g.setFont(10);
			g.drawText("SLOT", 90 * i + 72, 15, 50, 12, Justification::centredLeft);
			g.setFont(26);
			g.drawText(String(basestations[i]->slot), 90 * i + 72, 28, 25, 26, Justification::centredLeft);
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
	else {
		g.setColour(Colours::darkgrey);
		g.setFont(15);
		g.drawText(String("NO BASESTATIONS DETECTED"), 0, 10, 250, 100, Justification::centred);
	}

}

FifoMonitor::FifoMonitor(int id_, Basestation* basestation_) : id(id_), basestation(basestation_), fillPercentage(0.0)
{
	startTimer(500); // update fill percentage every 0.5 seconds
}

void FifoMonitor::timerCallback()
{
	//std::cout << "Checking fill percentage for monitor " << id << ", slot " << int(slot) << std::endl;

	if (slot != 255)
	{
		setFillPercentage(basestation->getFillPercentage());
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

SourceButton::SourceButton(int id_, DataSource* source_) : id(id_), dataSource(source_), selected(false)
{
	status = SourceStatus::DISCONNECTED;

	if (dataSource != nullptr)
	{
		sourceType = dataSource->sourceType;
	}
	else {
		sourceType = DataSourceType::NONE;
	}

	setRadioGroupId(979);

	startTimer(500); // update probe status and fifo monitor every 500 ms
}


void SourceButton::setSelectedState(bool state)
{
	selected = state;
}

void SourceButton::paintButton(Graphics& g, bool isMouseOver, bool isButtonDown)
{
	if (isMouseOver && connected)
		g.setColour(Colours::antiquewhite);
	else
		g.setColour(Colours::darkgrey);
	g.fillEllipse(0, 0, 15, 15);

	Colour baseColour;

	if (sourceType == DataSourceType::PROBE)
	{
		baseColour = Colours::green;
	}
	else if (sourceType == DataSourceType::ADC)
	{
		baseColour = Colours::purple;
	}
	else if (sourceType == DataSourceType::DAC)
	{
		baseColour = Colours::blue;
	}
	else {
		baseColour = Colours::grey;
	}

	if (status == SourceStatus::CONNECTED)
	{
		if (selected)
		{
			if (isMouseOver)
				g.setColour(baseColour.brighter(0.9f));
			else
				g.setColour(baseColour.brighter(0.8f));
		}
		else {
			if (isMouseOver)
				g.setColour(baseColour.brighter(0.2f));
			else
				g.setColour(baseColour);
		}
	}
	else if (status == SourceStatus::CONNECTING || status == SourceStatus::UPDATING)
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

void SourceButton::setSourceStatus(SourceStatus status)
{
	if (dataSource != nullptr)
	{
		this->status = status;

		repaint();
	}

}

SourceStatus SourceButton::getSourceStatus()
{
	return status;
}

void SourceButton::timerCallback()
{

	if (dataSource != nullptr)
	{
		setSourceStatus(dataSource->getStatus());
		//std::cout << "Setting for slot: " << String(slot) << " port: " << String(port) << " status: " << String(status) << " selected: " << String(selected) << std::endl;
	}
}

BackgroundLoader::BackgroundLoader(NeuropixThread* thread_, NeuropixEditor* editor_) 
	: Thread("Neuropix Loader"), 
      thread(thread_), 
	  editor(editor_), 
	  isInitialized(false),
	  signalChainIsLoading(false)
{
}

BackgroundLoader::~BackgroundLoader()
{

	if (isThreadRunning())
	{
		waitForThreadToExit(30000);
	}
		
}

void BackgroundLoader::run()
{

	LOGC("Running background thread...");

	/* Initializes the NPX-PXI probe connections in the background to prevent this 
	   plugin from blocking the main GUI*/
	if (!isInitialized)
	{
		LOGC("Not initialized.");
		thread->initializeBasestations(signalChainIsLoading);
		isInitialized = true;

		//if (!signalChainIsLoading)
		//{
		LOGC("Updating settings for ", thread->getProbes().size(), " probes.");

		for (auto probe : thread->getProbes())
		{
			LOGC(" Updating queue for probe ", probe->name);
			thread->updateProbeSettingsQueue(ProbeSettings(probe->settings));
		}
				
		//}
	}

	LOGC("Initialized, applying probe settings...");

	/* Apply any saved settings */
	thread->applyProbeSettingsQueue();
	
}

void NeuropixEditor::initialize(bool signalChainIsLoading)
{
	uiLoader->signalChainIsLoading = signalChainIsLoading;
	uiLoader->startThread();
}


NeuropixEditor::NeuropixEditor(GenericProcessor* parentNode, NeuropixThread* t)
 : VisualizerEditor(parentNode, "Neuropix PXI")
{

    thread = t;
    canvas = nullptr;

	Array<Basestation*> basestations = t->getBasestations();

	bool foundFirst = false;

	int id = 0;

	for (int i = 0; i < basestations.size(); i++)
	{
		Array<Headstage*> headstages = basestations[i]->getHeadstages(); // can return null

		for (int j = 0; j < headstages.size(); j++)
		{

			int slotIndex = i;
			int portIndex = j;

			if (headstages[j] != nullptr)
			{
				Array<Probe*> probes = headstages[j]->getProbes();

				for (int k = 0; k < probes.size(); k++)
				{
					int offset;
					
					if (probes.size() == 2)
						offset = 20 * k;
					else
						offset = 10;

					int x_pos = slotIndex * 90 + 30 + offset;
					int y_pos = 125 - (portIndex + 1) * 22;

					SourceButton* p = new SourceButton(id++, probes[k]);
					p->setBounds(x_pos, y_pos, 15, 15);
					p->addListener(this);
					addAndMakeVisible(p);
					sourceButtons.add(p);
				}
			}
			else {
				int x_pos = slotIndex * 90 + 40;
				int y_pos = 125 - (portIndex + 1) * 22;

				SourceButton* p = new SourceButton(id++, nullptr);
				p->setBounds(x_pos, y_pos, 15, 15);
				p->addListener(this);
				addAndMakeVisible(p);
				sourceButtons.add(p);
			}
		}

		Array<DataSource*> additionalDataSources = basestations[i]->getAdditionalDataSources(); // can return null

		for (int j = 0; j < additionalDataSources.size(); j++)
		{
			LOGD("Creating source button for ADCs");

			int slotIndex = i;
			int portIndex = j + 3;

			int x_pos = slotIndex * 90 + 40;
			int y_pos = 125 - (portIndex + 1) * 22;

			SourceButton* p = new SourceButton(id++, additionalDataSources[j]);
			p->setBounds(x_pos, y_pos, 15, 15);
			p->addListener(this);
			addAndMakeVisible(p);
			sourceButtons.add(p);
		}
	}

	

	for (int i = 0; i < basestations.size(); i++)
	{
		int x_pos = i * 90 + 70;
		int y_pos = 50;

		UtilityButton* b = new UtilityButton("", Font("Small Text", 13, Font::plain));
		b->setBounds(x_pos, y_pos, 30, 20);
		b->addListener(this);
		//addAndMakeVisible(b);
		directoryButtons.add(b);

		savingDirectories.add(File());

		FifoMonitor* f = new FifoMonitor(i, basestations[i]);
		f->setBounds(x_pos + 2, 75, 12, 50);
		addAndMakeVisible(f);
		f->setSlot(basestations[i]->slot);
		fifoMonitors.add(f);
	}

	masterSelectBox = new ComboBox("MasterSelectComboBox");
	masterSelectBox->setBounds(90 * (basestations.size())+32, 39, 38, 20);
	for (int i = 0; i < basestations.size(); i++)
	{
		masterSelectBox->addItem(String(i + 1), i+1);
	}
	masterSelectBox->setSelectedItemIndex(0, dontSendNotification);
	masterSelectBox->addListener(this);
	addChildComponent(masterSelectBox);

	masterConfigBox = new ComboBox("MasterConfigComboBox");
	masterConfigBox->setBounds(90 * (basestations.size())+32, 72, 78, 20);
	masterConfigBox->addItem(String("INPUT"), 1);
	masterConfigBox->addItem(String("OUTPUT"), 2);
	masterConfigBox->setSelectedItemIndex(0, dontSendNotification);
	masterConfigBox->addListener(this);
	addChildComponent(masterConfigBox);

	Array<int> syncFrequencies = t->getSyncFrequencies();

	freqSelectBox = new ComboBox("FreqSelectComboBox");
	freqSelectBox->setBounds(90 * (basestations.size())+32, 105, 70, 20);
	for (int i = 0; i < syncFrequencies.size(); i++)
	{
		freqSelectBox->addItem(String(syncFrequencies[i])+String(" Hz"), i+1);
	}
	freqSelectBox->setSelectedItemIndex(0, dontSendNotification);
	freqSelectBox->addListener(this);
	addChildComponent(freqSelectBox);


	background = new EditorBackground(basestations, false);
	background->setBounds(0, 15, 500, 150);
	addAndMakeVisible(background);
	background->toBack();
	background->repaint();

	addSyncChannelButton = new UtilityButton("+", Font("Small Text", 13, Font::plain));
	addSyncChannelButton->setBounds(90 * basestations.size() + 78, 40, 20, 20);
	addSyncChannelButton->addListener(this);
	addSyncChannelButton->setTooltip("Add sync channel to the continuous data stream.");
	addSyncChannelButton->setClickingTogglesState(true);
	addChildComponent(addSyncChannelButton);

	if (basestations.size() > 0)
	{
		masterConfigBox->setVisible(true);
		masterSelectBox->setVisible(true);
		addSyncChannelButton->setVisible(true);
		desiredWidth = 100 * basestations.size() + 120;
	}
	else
	{
		desiredWidth = 250;
	}

	uiLoader = new BackgroundLoader(t, this);
	
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
		thread->setMainSync(slotIndex);
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
			thread->setMainSync(slotIndex);
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

void NeuropixEditor::startAcquisition()
{

	if (canvas)
		canvas->startAcquisition();
}

void NeuropixEditor::stopAcquisition()
{
	if (canvas)
		canvas->stopAcquisition();
}

void NeuropixEditor::buttonClicked(Button* button)
{

	if (sourceButtons.contains((SourceButton*) button))
	{
		for (auto button : sourceButtons)
		{
			button->setSelectedState(false);
		}

		SourceButton* sourceButton = (SourceButton*) button;

		sourceButton->setSelectedState(true);

		if (canvas != nullptr && sourceButton->dataSource != nullptr)
			canvas->setSelectedInterface(sourceButton->dataSource);

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
		else if (button == addSyncChannelButton)
		{
			thread->sendSyncAsContinuousChannel(addSyncChannelButton->getToggleState());
			CoreServices::updateSignalChain(this);
		}
    }
}


void NeuropixEditor::saveVisualizerEditorParameters(XmlElement* xml)
{
	LOGD("Saving Neuropix editor.");

	XmlElement* xmlNode = xml->createNewChildElement("NEUROPIXELS_EDITOR");

	for (int slot = 0; slot < thread->getBasestations().size(); slot++)
	{
		String directory_name = savingDirectories[slot].getFullPathName();
		if (directory_name.length() == 2)
			directory_name += "\\\\";

		xmlNode->setAttribute("Slot" + String(slot) + "Directory", directory_name);
	}

	xmlNode->setAttribute("MasterSlot", masterSelectBox->getSelectedItemIndex());

	xmlNode->setAttribute("SendSyncAsContinuous", addSyncChannelButton->getToggleState());

	xmlNode->setAttribute("SyncDirection", masterConfigBox->getSelectedItemIndex());

	xmlNode->setAttribute("SyncFreq", freqSelectBox->getSelectedItemIndex());


}

void NeuropixEditor::loadVisualizerEditorParameters(XmlElement* xml)
{

	forEachXmlChildElement(*xml, xmlNode)
	{
		if (xmlNode->hasTagName("NEUROPIXELS_EDITOR"))
		{
			LOGDD("Found parameters for Neuropixels editor");

			for (int slot = 0; slot < thread->getBasestations().size(); slot++)
			{
				File directory = File(xmlNode->getStringAttribute("Slot" + String(slot) + "Directory"));
				LOGDD("Setting thread directory for slot ", slot);
				thread->setDirectoryForSlot(slot, directory);
				directoryButtons[slot]->setLabel(directory.getFullPathName().substring(0, 2));
				savingDirectories.set(slot, directory);
			}

			int slotIndex = xmlNode->getIntAttribute("MasterSlot", masterSelectBox->getSelectedItemIndex());

			/*Configure master basestation */
			thread->setMainSync(slotIndex);
			masterConfigBox->setSelectedItemIndex(0,true);
			freqSelectBox->setSelectedItemIndex(0, true);

			/* Add sync as continuous channel */
			bool addSyncAsContinuousChannel = xmlNode->getBoolAttribute("SendSyncAsContinuous", false);
			addSyncChannelButton->setToggleState(addSyncAsContinuousChannel, false);
			thread->sendSyncAsContinuousChannel(addSyncAsContinuousChannel);

			/* Set SMA as input or output */
			bool setAsOutput = (bool)xmlNode->getIntAttribute("SyncDirection", false);

			if (setAsOutput)
			{
				masterConfigBox->setSelectedItemIndex(1,true);
				thread->setSyncOutput(slotIndex);
				freqSelectBox->setVisible(true);
				background->setFreqSelectAvailable(true);
				int freqIndex = xmlNode->getIntAttribute("SyncFreq", false);
				freqSelectBox->setSelectedItemIndex(freqIndex, true);
				thread->setSyncFrequency(slotIndex, freqIndex);
			}

		}
	}
}

Visualizer* NeuropixEditor::createNewCanvas(void)
{
    GenericProcessor* processor = (GenericProcessor*) getProcessor();
    canvas = new NeuropixCanvas(processor, this, thread);
    return canvas;
}

/********************************************/
