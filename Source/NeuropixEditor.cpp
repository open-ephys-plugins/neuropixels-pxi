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

#include "NeuropixEditor.h"

#include "NeuropixCanvas.h"
#include "NeuropixComponents.h"
#include "NeuropixThread.h"

RefreshButton::RefreshButton() : Button ("Refresh")
{

    XmlDocument xmlDoc (R"(
        <svg width="800px" height="800px" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
<path d="M13 2L11 3.99545L11.0592 4.05474M11 18.0001L13 19.9108L12.9703 19.9417M11.0592 4.05474L13 6M11.0592 4.05474C11.3677 4.01859 11.6817 4 12 4C16.4183 4 20 7.58172 20 12C20 14.5264 18.8289 16.7793 17 18.2454M7 5.75463C5.17107 7.22075 4 9.47362 4 12C4 16.4183 7.58172 20 12 20C12.3284 20 12.6523 19.9802 12.9703 19.9417M11 22.0001L12.9703 19.9417" stroke="#000000" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/>
</svg>
    )");

    refreshIcon = Drawable::createFromSVG (*xmlDoc.getDocumentElement().get());

    setClickingTogglesState (false);
}

void RefreshButton::paintButton(Graphics& g, bool isMouseOver, bool isButtonDown)
{
    Colour buttonColour = Colours::darkgrey;

    if (isMouseOver && isEnabled())
        buttonColour = Colours::white;

    refreshIcon->replaceColour (Colours::black, buttonColour);

    refreshIcon->drawWithin (g, getLocalBounds().toFloat(), RectanglePlacement::centred, 1.0f);

    refreshIcon->replaceColour (buttonColour, Colours::black);
}

void RefreshButton::parentSizeChanged()
{
    setBounds (getParentWidth() - 65, 4, 16, 16);
}

SlotButton::SlotButton (Basestation* bs, NeuropixThread* thread_) : Button (String (bs->slot))
{
    isEnabled = true;
    thread = thread_;
    basestation = bs;
    slot = basestation->slot;
}

void SlotButton::paintButton (Graphics& g, bool isMouseOver, bool isButtonDown)
{
    g.setFont (26);

    if (isMouseOver && isEnabled)
        g.setColour (Colours::yellow);
    else
        g.setColour (findColour (ThemeColours::defaultText));

    g.drawText (String (slot), 0, 0, getWidth(), getHeight(), Justification::centredLeft);
}

void SlotButton::mouseUp (const MouseEvent& event)
{
    if (! isEnabled)
        return;

    ProbeNameConfig* probeNamingPopup = new ProbeNameConfig (basestation, thread);

    CallOutBox& myBox = CallOutBox::launchAsynchronously (std::unique_ptr<Component> (probeNamingPopup),
                                                          getScreenBounds(),
                                                          nullptr);

    myBox.addComponentListener (this);
    myBox.setDismissalMouseClicksAreAlwaysConsumed (true);
}

void SlotButton::componentBeingDeleted (Component& component)
{
    CoreServices::updateSignalChain ((GenericEditor*) (getParentComponent()->getParentComponent()));
}

EditorBackground::EditorBackground (NeuropixThread* t, bool freqSelectEnabled)
    : basestations (t->getBasestations()),
      type (t->type),
      freqSelectEnabled (freqSelectEnabled)
{
    numBasestations = basestations.size();

    for (int i = 0; i < numBasestations; i++)
    {
        LOGD ("Creating slot button.");
        slotButtons.push_back (std::make_unique<SlotButton> (basestations[i], t));
        slotButtons[slotButtons.size() - 1]->setBounds (90 * i + 72, 28, 35, 26);
        addAndMakeVisible (slotButtons[slotButtons.size() - 1].get());
    }
}

void EditorBackground::setFreqSelectAvailable (bool isAvailable)
{
    freqSelectEnabled = isAvailable;
}

void EditorBackground::paint (Graphics& g)
{
    if (numBasestations > 0)
    {
        for (int i = 0; i < numBasestations; i++)
        {
            g.setColour (findColour (ThemeColours::outline));
            g.drawRoundedRectangle (90 * i + 30-3, 13, 35+6, 98, 4, 1);

            g.setColour (findColour (ThemeColours::defaultText));
            
            g.setFont (10);
            g.drawText ("SLOT", 90 * i + 72, 15, 50, 12, Justification::centredLeft);

            g.setFont (8);
            g.drawText (String ("0"), 90 * i + 87, 100, 50, 10, Justification::centredLeft);
            g.drawText (String ("100"), 90 * i + 87, 60, 50, 10, Justification::centredLeft);
            g.drawText (String ("%"), 90 * i + 87, 80, 50, 10, Justification::centredLeft);

            for (int j = 0; j < 4; j++)
            {
                g.setFont (10);

                if (type == ONEBOX && j == 3)
                {
                    g.drawText (String ("ADC"), 90 * i + 20 - 12, 90 - j * 22 + 1, 20, 10, Justification::centredLeft);
				}
                else if (type == ONEBOX && j == 2)
                {
                    // skip
                }
                else
                {
                    g.drawText (String (j + 1), 90 * i + 20 - 3, 90 - j * 22 + 1, 10, 10, Justification::centredLeft);
				}
                
            }
        }

        g.setFont (10);
        if(type != ONEBOX)
            g.drawText (String ("MAIN SYNC SLOT"), 90 * (numBasestations) + 32, 13, 100, 10, Justification::centredLeft);
        g.drawText (String ("SMA CONFIGURATION"), 90 * (numBasestations) + 32, 48, 100, 10, Justification::centredLeft);
        if (freqSelectEnabled)
            g.drawText (String ("WITH FREQ"), 90 * (numBasestations) + 32, 82, 100, 10, Justification::centredLeft);
    }
    else
    {
        g.setColour (findColour (ThemeColours::defaultText));
        g.setFont (15);
        if (type == PXI)
        {
            g.drawText (String ("NO BASESTATIONS DETECTED"), 0, 10, 250, 100, Justification::centred);
        }
        else if (type == ONEBOX)
        {
            g.drawText (String ("NO ONEBOX DETECTED"), 0, 10, 250, 100, Justification::centred);
        }
    }
}

FifoMonitor::FifoMonitor (int id_, Basestation* basestation_) : id (id_),
                                                                basestation (basestation_),
                                                                fillPercentage (0.0)
{
    startTimer (500); // update fill percentage every 0.5 seconds
}

void FifoMonitor::timerCallback()
{
    if (slot != 255)
    {
        setFillPercentage (basestation->getFillPercentage());
    }
}

void FifoMonitor::setSlot (unsigned char slot_)
{
    slot = slot_;
}

void FifoMonitor::setFillPercentage (float fill_)
{
    fillPercentage = fill_;

    repaint();
}

void FifoMonitor::paint (Graphics& g)
{
    g.setColour (findColour (ThemeColours::outline));
    g.fillRoundedRectangle (0, 0, this->getWidth(), this->getHeight(), 4);
    g.setColour (findColour (ThemeColours::widgetBackground));
    g.fillRoundedRectangle (1, 1, this->getWidth() - 2, this->getHeight() - 2, 2);

    g.setColour (Colours::yellow);
    float barHeight = (this->getHeight() - 4) * fillPercentage;
    g.fillRoundedRectangle (2, this->getHeight() - 2 - barHeight, this->getWidth() - 4, barHeight, 2);
}

SourceButton::SourceButton (int id_, DataSource* source_, Basestation* basestation_)
    : id (id_),
      dataSource (source_),
      selected (false),
      basestation (basestation_)
{
    status = SourceStatus::DISCONNECTED;

    if (dataSource != nullptr)
    {
        sourceType = dataSource->sourceType;
    }
    else
    {
        sourceType = DataSourceType::NONE;
    }

    setRadioGroupId (979);

    startTimer (500); // update probe status and fifo monitor every 500 ms
}

void SourceButton::setSelectedState (bool state)
{
    selected = state;
}

void SourceButton::paintButton (Graphics& g, bool isMouseOver, bool isButtonDown)
{
    if (isMouseOver && connected)
        g.setColour (findColour (ThemeColours::highlightedFill));
    else
        g.setColour (findColour (ThemeColours::outline).withAlpha (0.75f));

    g.fillEllipse (0, 0, 15, 15);

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
    else
    {
        baseColour = findColour (ThemeColours::defaultFill);
    }

    if (status == SourceStatus::CONNECTED)
    {
        if (selected)
        {
            if (isMouseOver)
                g.setColour (baseColour.brighter (0.9f));
            else
                g.setColour (baseColour.brighter (0.8f));
        }
        else
        {
            if (isMouseOver)
                g.setColour (baseColour.brighter (0.2f));
            else
                g.setColour (baseColour);
        }
    }
    else if (status == SourceStatus::CONNECTING || status == SourceStatus::UPDATING)
    {
        if (selected)
        {
            if (isMouseOver)
                g.setColour (Colours::lightsalmon);
            else
                g.setColour (Colours::lightsalmon);
        }
        else
        {
            if (isMouseOver)
                g.setColour (Colours::orange);
            else
                g.setColour (Colours::orange);
        }
    }
    else if (status == SourceStatus::DISABLED) {
        if (selected)
        {
            if (isMouseOver)
                g.setColour (Colours::red);
            else
                g.setColour (Colours::red);
        }
        else
        {
            if (isMouseOver)
                g.setColour (Colours::red);
            else
                g.setColour (Colours::red);
        }

    }
    else
    {
        g.setColour (findColour (ThemeColours::widgetBackground));
    }

    g.fillEllipse (2, 2, 11, 11);
}

void SourceButton::setSourceStatus (SourceStatus status)
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
        setSourceStatus (dataSource->getStatus());
    }
}


BackgroundLoaderWithProgressWindow::BackgroundLoaderWithProgressWindow (NeuropixThread* thread_, NeuropixEditor* editor_)
    : ThreadWithProgressWindow("Re-scanning Neuropixels devices", true, false), 
      BackgroundLoader (thread_, editor_)
{
}

void BackgroundLoaderWithProgressWindow::run()
{

    setProgress (-1); // endless moving progress bar

    std::map<std::tuple<int, int, int>, std::pair<uint64, ProbeSettings>> updatedMap;

    ProbeSettings temp;

    setStatusMessage ("Checking for hardware changes...");
    LOGC ("Scanning for hardware changes...");
    //Assume basestation counts/slots do not change
    for (int i = 0; i < thread->getBasestations().size(); i++)
    {
        Basestation* bs = thread->getBasestations()[i];
        if (bs != nullptr)
        {
            bs->close();
            bs->open();

            for (auto hs : bs->getHeadstages())
            {
                if (hs != nullptr)
                {
                    for (auto probe : hs->getProbes())
                    {
                        if (probe != nullptr)
                        {
                            //Check for existing probe settings
                            std::tuple<int, int, int> current_location = std::make_tuple (bs->slot, hs->port, probe->dock);

                            LOGD ("Checking for probe at slot ", bs->slot, " port ", hs->port, " dock ", probe->dock);

                            if (thread->probeMap.find (current_location) != thread->probeMap.end())
                            {
                                LOGD ("Found matching probe.");

                                if (std::get<0> (thread->probeMap[current_location]) == probe->info.serial_number)
                                {
                                    temp = ProbeSettings (thread->probeMap[current_location].second);
                                    temp.probe = probe;
                                    updatedMap[current_location] = std::make_pair (probe->info.serial_number, temp);
                                    continue;
                                }
                            }

                            bool found = false;
                            std::tuple<int, int, int> old_location;

                            for (auto it = thread->probeMap.begin(); it != thread->probeMap.end(); it++)
                            {
                                uint64 old_serial = it->second.first;
                                if (old_serial == probe->info.serial_number)
                                {
                                    //Existing probe moved to new location
                                    found = true;
                                    old_location = it->first;
                                    temp = ProbeSettings (it->second.second);
                                    temp.probe = probe;
                                    updatedMap[current_location] = std::make_pair (probe->info.serial_number, temp);
                                    break;
                                }
                            }
                            if (! found)
                            {
                                //New probe connected
                                updatedMap[current_location] = std::make_pair (probe->info.serial_number, probe->settings);
                            }
                        }
                    }
                }
            }
        }
    }

    //Update the probe map
    LOGD ("Updating probe map...");
    thread->probeMap = updatedMap;

    setStatusMessage ("Initializing probes...");

    LOGD ("Initializing probes...");
    thread->initializeProbes();
    thread->updateStreamInfo();

    thread->isRefreshing = false;

}

BackgroundLoader::BackgroundLoader (NeuropixThread* thread_, NeuropixEditor* editor_)
    : Thread ("Neuropix Loader"),
      thread (thread_),
      editor (editor_)
{
}

BackgroundLoader::~BackgroundLoader()
{
    if (isThreadRunning())
    {
        waitForThreadToExit (30000);
    }
}

void BackgroundLoader::run()
{
    LOGC ("Running background thread...");

    /* Initializes the NPX-PXI probe connections in the background to prevent this 
	   plugin from blocking the main GUI*/

    if (!isInitialized)
    {
        LOGC ("Not initialized.");
        thread->initializeBasestations (signalChainIsLoading);
        isInitialized = true;

        LOGC ("Updating settings for ", thread->getProbes().size(), " probes.");

        //MessageManagerLock mml;
        //CoreServices::updateSignalChain (editor);

        bool updateStreamInfoRequired = false;

        for (auto probe : thread->getProbes())
        {
            LOGC (" Updating queue for probe ", probe->name);
            thread->updateProbeSettingsQueue (ProbeSettings (probe->settings));

            if (! probe->isEnabled)
                updateStreamInfoRequired = true;
        }

        if (updateStreamInfoRequired)
        {
            thread->updateStreamInfo(true);
            MessageManagerLock mml;
            CoreServices::updateSignalChain (editor);
        }

        //editor->checkCanvas();

    }
    

    LOGC ("Initialized, applying probe settings...");

    /* Apply any queued settings */
    thread->applyProbeSettingsQueue();
}

void NeuropixEditor::resetCanvas()
{
    if (canvas != nullptr)
    {
        VisualizerEditor::canvas.reset();

        if (isOpenInTab)
        {
            removeTab();
            addTab();
        }
        else
        {
            checkForCanvas();
            
            if (dataWindow != nullptr)
                dataWindow->setContentNonOwned (VisualizerEditor::canvas.get(), false);
        }
    }
}

void NeuropixEditor::initialize (bool signalChainIsLoading)
{
    uiLoader->signalChainIsLoading = signalChainIsLoading;
    uiLoader->startThread();

    checkCanvas();
}

NeuropixEditor::NeuropixEditor (GenericProcessor* parentNode, NeuropixThread* t)
    : VisualizerEditor (parentNode, t->type == ONEBOX ? "OneBox" : "Neuropix PXI")
{
    canvas = nullptr;

    thread = t;

    Array<Basestation*> basestations = t->getBasestations();

    drawBasestations(basestations);

    mainSyncSelector = std::make_unique<ComboBox> ("Basestation that acts as main synchronizer");
    mainSyncSelector->setBounds (90 * (basestations.size()) + 32, 39, 50, 20);
    for (int i = 0; i < basestations.size(); i++)
    {
        mainSyncSelector->addItem (String (basestations[i]->slot), i + 1);
    }
    mainSyncSelector->setSelectedItemIndex (0, dontSendNotification);
    mainSyncSelector->addListener (this);
    addChildComponent (mainSyncSelector.get());

    inputOutputSyncSelector = std::make_unique<ComboBox> ("Toggles the main synchronizer as input or output");
    inputOutputSyncSelector->setBounds (90 * (basestations.size()) + 32, 74, 78, 20);
    inputOutputSyncSelector->addItem (String ("INPUT"), 1);
    inputOutputSyncSelector->addItem (String ("OUTPUT"), 2);
    inputOutputSyncSelector->setSelectedItemIndex (0, dontSendNotification);
    inputOutputSyncSelector->addListener (this);
    addChildComponent (inputOutputSyncSelector.get());

    Array<int> syncFrequencies = t->getSyncFrequencies();

    // syncFrequencySelector = std::make_unique<ComboBox> ("Select the sync frequency (if in output mode)");
    // syncFrequencySelector->setBounds (90 * (basestations.size()) + 32, 105, 70, 20);
    // for (int i = 0; i < syncFrequencies.size(); i++)
    // {
    //     syncFrequencySelector->addItem (String (syncFrequencies[i]) + String (" Hz"), i + 1);
    // }
    // syncFrequencySelector->setSelectedItemIndex (0, dontSendNotification);
    // syncFrequencySelector->addListener (this);
    // addChildComponent (syncFrequencySelector.get());

    syncFrequencyLabel = std::make_unique<Label> ("Sync frequency label", String (syncFrequencies[0]) + " Hz");
    syncFrequencyLabel->setBounds (90 * (basestations.size()) + 32, 105, 70, 20);
    syncFrequencyLabel->setFont (FontOptions ("Inter", "Regular", 16.0f));
    addChildComponent (syncFrequencyLabel.get());

    background = std::make_unique<EditorBackground> (t, false);
    background->setBounds (0, 15, 500, 150);
    addAndMakeVisible (background.get());
    background->toBack();
    background->repaint();

    addSyncChannelButton = std::make_unique<UtilityButton> ("+");
    addSyncChannelButton->setBounds (90 * basestations.size() + 90, 40, 20, 20);
    addSyncChannelButton->addListener (this);
    addSyncChannelButton->setTooltip ("Add sync channel to the continuous data stream.");
    addSyncChannelButton->setClickingTogglesState (true);
    addChildComponent (addSyncChannelButton.get());

    refreshButton = std::make_unique<RefreshButton> ();
    refreshButton->setBounds (desiredWidth - 65, 4, 16, 16);
    refreshButton->addListener (this);
    refreshButton->setTooltip ("Re-scan basestation for hardware changes.");
    addChildComponent (refreshButton.get());

    if (basestations.size() > 0)
    {
        if (thread->type != ONEBOX)
        {
            mainSyncSelector->setVisible (true);   
            //addSyncChannelButton->setVisible (true);
            refreshButton->setVisible (true);
            
        }

        inputOutputSyncSelector->setVisible (true);
        desiredWidth = 100 * basestations.size() + 120;
    }
    else
    {
        desiredWidth = 250;
    }

    uiLoader = std::make_unique<BackgroundLoader> (t, this);
    uiLoaderWithProgressWindow = std::make_unique<BackgroundLoaderWithProgressWindow> (t, this);
}

void NeuropixEditor::drawBasestations(Array<Basestation*> basestations) {
    
    //Clear any existing source buttons from sourceButtons vector of unique ptrs
    for (auto& button : sourceButtons)
    {
        removeChildComponent (button.get());
    }
    sourceButtons.clear();

    int id = 0;

    for (int i = 0; i < basestations.size(); i++)
    {
        Array<Headstage*> headstages = basestations[i]->getHeadstages(); // can return null

        int probeCount = basestations[i]->getProbeCount();

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

                    LOGD("### Adding new source button for probe at slot ", slotIndex, " port ", portIndex, " dock ", k);
                    sourceButtons.emplace_back (std::make_unique<SourceButton> (id++, probes[k]));

                    SourceButton* sourceButton = sourceButtons[sourceButtons.size() - 1].get();
                    sourceButton->setBounds (x_pos, y_pos, 15, 15);
                    sourceButton->addListener (this);
                    addAndMakeVisible (sourceButton);
                }
            }
            else
            {
                int x_pos = slotIndex * 90 + 40;
                int y_pos = 125 - (portIndex + 1) * 22;

                if (probeCount == 0)
                    sourceButtons.emplace_back (std::make_unique<SourceButton> (id++, nullptr, basestations[i]));
                else
                    sourceButtons.emplace_back (std::make_unique<SourceButton> (id++, nullptr, nullptr));

                SourceButton* p = sourceButtons[sourceButtons.size() - 1].get();
                p->setBounds (x_pos, y_pos, 15, 15);
                p->addListener (this);
                addAndMakeVisible (p);
            }
        }

        Array<DataSource*> additionalDataSources = basestations[i]->getAdditionalDataSources(); // can return null

        for (int j = 0; j < additionalDataSources.size(); j++)
        {
            LOGD ("Creating source button for ADCs");

            int slotIndex = i;
            int portIndex = j + 3;

            int x_pos = slotIndex * 90 + 40;
            int y_pos = 125 - (portIndex + 1) * 22;

            sourceButtons.emplace_back( std::make_unique<SourceButton> (id++, additionalDataSources[j]));
            SourceButton* p = sourceButtons[sourceButtons.size() - 1].get();
            p->setBounds (x_pos, y_pos, 15, 15);
            p->addListener (this);
            addAndMakeVisible (p);
        }
    }

    for (int i = 0; i < basestations.size(); i++)
    {
        int x_pos = i * 90 + 70;
        int y_pos = 50;

        UtilityButton* b = new UtilityButton ("");
        b->setBounds (x_pos, y_pos, 30, 20);
        b->addListener (this);
        //addAndMakeVisible(b);
        directoryButtons.add (b);

        savingDirectories.add (File());
        slotNamingSchemes.add (0);

        FifoMonitor* f = new FifoMonitor (i, basestations[i]);
        f->setBounds (x_pos + 2, 75, 12, 50);
        addAndMakeVisible (f);
        f->setSlot (basestations[i]->slot);
        fifoMonitors.add (f);
    }

}

void NeuropixEditor::collapsedStateChanged()
{
    if (inputOutputSyncSelector->getSelectedId() == 1)
        syncFrequencyLabel->setVisible (false);
}

void NeuropixEditor::update()
{
    if (canvas != nullptr)
        canvas->updateSettings();
}

void NeuropixEditor::comboBoxChanged (ComboBox* comboBox)
{
    int slotIndex = mainSyncSelector->getSelectedId() - 1;

    if (comboBox == mainSyncSelector.get())
    {
        thread->setMainSync (slotIndex);
        inputOutputSyncSelector->setSelectedItemIndex (0, true);
        syncFrequencyLabel->setVisible (false);
        background->setFreqSelectAvailable (false);
        // syncFrequencySelector->setSelectedItemIndex (0, true);
    }
    else if (comboBox == inputOutputSyncSelector.get())
    {
        bool asOutput = inputOutputSyncSelector->getSelectedId() == 2;

        if (asOutput)
        {
            thread->setSyncOutput (slotIndex);
            syncFrequencyLabel->setVisible (true);
            background->setFreqSelectAvailable (true);
        }
        else
        {
            thread->setMainSync (slotIndex);
            syncFrequencyLabel->setVisible (false);
            background->setFreqSelectAvailable (false);
        }

        // syncFrequencySelector->setSelectedItemIndex (0, true);
    }
    // else /* comboBox == freqSelectBox */
    // {
    //     int freqIndex = syncFrequencySelector->getSelectedId() - 1;
    //     thread->setSyncFrequency (slotIndex, freqIndex);
    // }

    background->repaint();
}

void NeuropixEditor::startAcquisition()
{
    if (canvas)
        canvas->startAcquisition();

    addSyncChannelButton->setEnabled (false);
    background->setEnabled (false);
    refreshButton->setVisible (false);
}

void NeuropixEditor::stopAcquisition()
{
    if (canvas)
        canvas->stopAcquisition();

    addSyncChannelButton->setEnabled (true);
    background->setEnabled (true);
    refreshButton->setVisible (true);
}

void NeuropixEditor::buttonClicked (Button* button)
{
    for (auto & source : sourceButtons)
    {
        if (source.get() == button)
        {
            for (auto & button : sourceButtons)
            {
                button->setSelectedState (false);
            }

            SourceButton* sourceButton = (SourceButton*) button;

            sourceButton->setSelectedState (true);

            if (canvas != nullptr && sourceButton->dataSource != nullptr)
            {
                canvas->setSelectedInterface (sourceButton->dataSource);
            }
            else if (canvas != nullptr && sourceButton->dataSource == nullptr)
            {
                canvas->setSelectedBasestation (sourceButton->basestation);
            }

            repaint();
        }
    }

    if (! acquisitionIsActive)
    {
        if (directoryButtons.contains ((UtilityButton*) button))
        {
            // open file chooser to select the saving location for this basestation
            int slotIndex = directoryButtons.indexOf ((UtilityButton*) button);
            File currentDirectory = thread->getDirectoryForSlot (slotIndex);
            FileChooser fileChooser ("Select directory for NPX file.", currentDirectory);
            if (fileChooser.browseForDirectory())
            {
                File result = fileChooser.getResult();
                String pathName = result.getFullPathName();
                thread->setDirectoryForSlot (slotIndex, result);

                savingDirectories.set (slotIndex, result);
                UtilityButton* ub = (UtilityButton*) button;
                ub->setLabel (pathName.substring (0, 3));
            }
        }
        else if (button == addSyncChannelButton.get())
        {
            thread->sendSyncAsContinuousChannel (addSyncChannelButton->getToggleState());
            CoreServices::updateSignalChain (this);
        }
        else if (button == refreshButton.get())
        {
            for (auto & btn : sourceButtons)
            {
                btn->setSourceStatus (SourceStatus::DISCONNECTED);
                btn->stopTimer();
            }
            thread->isRefreshing = true;
            uiLoaderWithProgressWindow->runThread();
            LOGD ("Updating signal chain...");

            LOGD ("Resetting canvas...");
            drawBasestations (thread->getBasestations());
            resetCanvas();

            LOGD ("Updating settings interfaces...");
            for (auto& interface : canvas->settingsInterfaces)
            {
                for (auto probe : thread->getProbes())
                {
                    if (interface->dataSource != nullptr && interface->dataSource->getName() == probe->getName())
                    {
                        ProbeSettings settingsToRestore = ProbeSettings (thread->probeMap[std::make_tuple (probe->basestation->slot, probe->headstage->port, probe->dock)].second);
                        interface->applyProbeSettings (settingsToRestore, true);
                    }
                }
            }

            CoreServices::updateSignalChain (this);
        }
    }
}

void NeuropixEditor::selectSource (DataSource* source)
{
    for (auto & button : sourceButtons)
    {
        if (source == button->dataSource)
        {
            button->setSelectedState (true);
        }
        else
        {
            button->setSelectedState (false);
        }
    }

    repaint();
}

void NeuropixEditor::saveVisualizerEditorParameters (XmlElement* xml)
{
    LOGD ("Saving Neuropix editor.");

    XmlElement* xmlNode = xml->createNewChildElement ("NEUROPIXELS_EDITOR");

    for (int slotIdx = 0; slotIdx < thread->getBasestations().size(); slotIdx++)
    {
        Basestation* bs = thread->getBasestations()[slotIdx];
        int slot = bs->slot;

        String directory_name = savingDirectories[slot].getFullPathName();
        if (directory_name.length() == 2)
            directory_name += "\\\\";

        XmlElement* basestationXml = xmlNode->createNewChildElement ("BASESTATION");

        basestationXml->setAttribute ("Directory", directory_name);
        basestationXml->setAttribute ("Slot", slot);
        basestationXml->setAttribute ("NamingScheme", (int) bs->getNamingScheme());

        for (int port = 1; port < 5; port++)
        {
            for (int dock = 1; dock < 3; dock++)
            {
                basestationXml->setAttribute ("port" + String (port) + "dock" + String (dock), bs->getCustomPortName (port, dock));
            }
        }
    }

    xmlNode->setAttribute ("MainSyncSlot", mainSyncSelector->getSelectedItemIndex());

    xmlNode->setAttribute ("SendSyncAsContinuous", addSyncChannelButton->getToggleState());

    xmlNode->setAttribute ("SyncDirection", inputOutputSyncSelector->getSelectedItemIndex());

    xmlNode->setAttribute ("SyncFreq", 0);

    XmlElement* customNamesXml = xmlNode->createNewChildElement ("CUSTOM_PROBE_NAMES");

    auto iter = thread->customProbeNames.begin();

    while (iter != thread->customProbeNames.end())
    {
        customNamesXml->setAttribute ("SN" + iter->first, iter->second);
        ++iter;
    }
}

void NeuropixEditor::loadVisualizerEditorParameters (XmlElement* xml)
{
    forEachXmlChildElement (*xml, xmlNode)
    {
        if (xmlNode->hasTagName ("NEUROPIXELS_EDITOR"))
        {
            LOGDD ("Found parameters for Neuropixels editor");

            int slotIdx = -1;

            forEachXmlChildElement (*xmlNode, basestationXml)
            {
                if (basestationXml->hasTagName ("BASESTATION"))
                {
                    slotIdx++;

                    if (slotIdx < thread->getBasestations().size())
                    {
                        File directory = File (basestationXml->getStringAttribute ("Directory"));
                        LOGDD ("Setting thread directory for slot ", slotIdx);
                        thread->setDirectoryForSlot (slotIdx, directory);
                        directoryButtons[slotIdx]->setLabel (directory.getFullPathName().substring (0, 2));
                        savingDirectories.set (slotIdx, directory);

                        Basestation* bs = thread->getBasestations()[slotIdx];

                        bs->setNamingScheme ((ProbeNameConfig::NamingScheme) basestationXml->getIntAttribute ("NamingScheme", 0));

                        for (int port = 1; port < 5; port++)
                        {
                            for (int dock = 1; dock < 3; dock++)
                            {
                                String key = "port" + String (port) + "dock" + String (dock);
                                if (basestationXml->hasAttribute (key))
                                    bs->setCustomPortName (basestationXml->getStringAttribute (key), port, dock);
                            }
                        }
                    }
                }
                else if (basestationXml->hasTagName ("CUSTOM_PROBE_NAMES"))
                {
                    for (int i = 0; i < basestationXml->getNumAttributes(); i++)
                    {
                        thread->setCustomProbeName (basestationXml->getAttributeName (i).substring (2),
                                                    basestationXml->getAttributeValue (i));
                    }
                }
            }

            int mainSyncSlotIndex = xmlNode->getIntAttribute ("MainSyncSlot", mainSyncSelector->getSelectedItemIndex());
            int frequencyIndex = 0;

            if (mainSyncSlotIndex >= thread->getBasestations().size())
                mainSyncSlotIndex = 0;

            /*Configure main basestation */
            thread->setMainSync (mainSyncSlotIndex);
            mainSyncSelector->setSelectedItemIndex (mainSyncSlotIndex, dontSendNotification);
            // syncFrequencySelector->setSelectedItemIndex (frequencyIndex, dontSendNotification);

            /* Add sync as continuous channel */
            bool addSyncAsContinuousChannel = xmlNode->getBoolAttribute ("SendSyncAsContinuous", false);
            addSyncChannelButton->setToggleState (addSyncAsContinuousChannel, dontSendNotification);
            thread->sendSyncAsContinuousChannel (addSyncAsContinuousChannel);

            /* Set SMA as input or output */
            bool setAsOutput = (bool) xmlNode->getIntAttribute ("SyncDirection", false);

            if (setAsOutput)
            {
                inputOutputSyncSelector->setSelectedItemIndex (1, dontSendNotification);
                thread->setSyncOutput (mainSyncSlotIndex);
                syncFrequencyLabel->setVisible (true);
                background->setFreqSelectAvailable (true);
                // syncFrequencySelector->setSelectedItemIndex (frequencyIndex, dontSendNotification);
                thread->setSyncFrequency (mainSyncSlotIndex, frequencyIndex);
            }
        }
    }
}

Visualizer* NeuropixEditor::createNewCanvas (void)
{
    GenericProcessor* processor = (GenericProcessor*) getProcessor();
    canvas = new NeuropixCanvas (processor, this, thread);

    if (acquisitionIsActive)
    {
        canvas->startAcquisition();
    }

    return canvas;
}

/********************************************/
