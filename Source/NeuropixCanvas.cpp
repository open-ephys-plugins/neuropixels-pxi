/*
------------------------------------------------------------------

This file is part of the Open Ephys GUI
Copyright(C) 2020 Allen Institute for Brain Science and Open Ephys

------------------------------------------------------------------

This program is free software : you can redistribute itand /or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.If not, see < http://www.gnu.org/licenses/>.

*/

#include "NeuropixCanvas.h"
#include "NeuropixComponents.h"

#include "Formats/IMRO.h"

NeuropixCanvas::NeuropixCanvas(GenericProcessor* processor_, NeuropixEditor* editor_, NeuropixThread* thread_) : 
    processor(processor_),
    editor(editor_),
    thread(thread_)

{
    neuropixViewport = new Viewport();
    neuropixViewport->setScrollBarsShown(true, true);

    Array<Probe*> available_probes = thread->getProbes();

    for (auto probe : available_probes)
    {
        NeuropixInterface* neuropixInterface = new NeuropixInterface(probe, thread, editor, this);
        neuropixInterfaces.add(neuropixInterface);
        probes.add(probe);
    }

    neuropixViewport->setViewedComponent(neuropixInterfaces.getFirst(), false);
    addAndMakeVisible(neuropixViewport);

    resized();
    //update();

    savedSettings.probeType = ProbeType::NONE;
}

NeuropixCanvas::~NeuropixCanvas()
{

}

void NeuropixCanvas::paint(Graphics& g)
{
    g.fillAll(Colours::darkgrey);
}

void NeuropixCanvas::refresh()
{
    repaint();
}

void NeuropixCanvas::refreshState()
{
    resized();
}

void NeuropixCanvas::update()
{

}

void NeuropixCanvas::beginAnimation()
{
}

void NeuropixCanvas::endAnimation()
{
}

void NeuropixCanvas::resized()
{
    // why is this not working?

    neuropixViewport->setBounds(0, 0, getWidth(), getHeight());

    for (int i = 0; i < neuropixInterfaces.size(); i++)
        neuropixInterfaces[i]->setBounds(0, 0, getWidth(), 1000);
}

void NeuropixCanvas::setParameter(int x, float f)
{

}

void NeuropixCanvas::setParameter(int a, int b, int c, float d)
{
}

void NeuropixCanvas::buttonClicked(Button* button)
{

}

void NeuropixCanvas::startAcquisition()
{
    for (auto npInterface : neuropixInterfaces)
        npInterface->startAcquisition();
}

void NeuropixCanvas::stopAcquisition()
{
    for (auto npInterface : neuropixInterfaces)
        npInterface->stopAcquisition();
}

void NeuropixCanvas::setSelectedProbe(Probe* probe)
{
    if (probe != nullptr)
    {

        int index = probes.indexOf(probe);

        if (index > -1)
            neuropixViewport->setViewedComponent(neuropixInterfaces[index], false);
    }

}

void NeuropixCanvas::storeProbeSettings(ProbeSettings p)
{
    savedSettings = p;
}

ProbeSettings NeuropixCanvas::getProbeSettings()
{
    return savedSettings;
}

void NeuropixCanvas::applyParametersToAllProbes(ProbeSettings p)
{
    for (auto npInterface : neuropixInterfaces)
    {
        npInterface->applyProbeSettings(p);
    }
}

void NeuropixCanvas::saveVisualizerParameters(XmlElement* xml)
{
    editor->saveEditorParameters(xml);

    for (int i = 0; i < neuropixInterfaces.size(); i++)
        neuropixInterfaces[i]->saveParameters(xml);
}

void NeuropixCanvas::loadVisualizerParameters(XmlElement* xml)
{
    editor->loadEditorParameters(xml);

    for (int i = 0; i < neuropixInterfaces.size(); i++)
        neuropixInterfaces[i]->loadParameters(xml);
}

/*****************************************************/
NeuropixInterface::NeuropixInterface(Probe* p,
                                     NeuropixThread* t, 
                                     NeuropixEditor* e,
                                     NeuropixCanvas* c) : 
                                     probe(p), thread(t), editor(e), canvas(c), neuropix_info("INFO")
{
    cursorType = MouseCursor::NormalCursor;

   // std::cout << slot << "--" << port << std::endl;

    isOverZoomRegion = false;
    isOverUpperBorder = false;
    isOverLowerBorder = false;
    isSelectionActive = false;
    isOverElectrode = false;

    // make a local copy
    electrodeMetadata = Array<ElectrodeMetadata>(probe->electrodeMetadata);

    // make a local copy
    probeMetadata = probe->probeMetadata;

    // probe-specific
    shankPath = Path(probe->shankOutline);

    mode = VisualizationMode::ENABLE_VIEW;

    // PROBE SPECIFIC DRAWING SETTINGS
    minZoomHeight = 40;
    maxZoomHeight = 120;
    pixelHeight = 1;
    int defaultZoomHeight = 100;

    if (probeMetadata.columns_per_shank == 8)
    {
        maxZoomHeight = 450;
        minZoomHeight = 300;
        defaultZoomHeight = 400;
        pixelHeight = 10;
    } 

    if (probeMetadata.shank_count == 4)
    {
        maxZoomHeight = 250;
        minZoomHeight = 40;
        defaultZoomHeight = 100;
        pixelHeight = 1;
    }

    // ALSO CONFIGURE CHANNEL JUMP
    if (probeMetadata.electrodes_per_shank < 500)
        channelLabelSkip = 50;
    else if (probeMetadata.electrodes_per_shank >= 500
        && probeMetadata.electrodes_per_shank < 1500)
        channelLabelSkip = 100;
    else if (probeMetadata.electrodes_per_shank >= 1500
        && probeMetadata.electrodes_per_shank < 3000)
        channelLabelSkip = 200;
    else
        channelLabelSkip = 500;

    addMouseListener(this, true);

    zoomHeight = defaultZoomHeight; // number of rows
    lowerBound = 530; // bottom of interface
    //zoomAreaMinRow = 0;
    zoomOffset = 50;
    dragZoneWidth = 10;

    int currentHeight = 55;

    enableButton = new UtilityButton("ENABLE", Font("Small Text", 13, Font::plain));
    enableButton->setRadius(3.0f);
    enableButton->setBounds(450, currentHeight, 65, 22);
    enableButton->addListener(this);
    enableButton->setTooltip("Enable selected channel(s)");
    addAndMakeVisible(enableButton);

    enableViewButton = new UtilityButton("VIEW", Font("Small Text", 12, Font::plain));
    enableViewButton->setRadius(3.0f);
    enableViewButton->setBounds(530, currentHeight + 2, 45, 18);
    enableViewButton->addListener(this);
    enableViewButton->setTooltip("View channel enabled state");
    addAndMakeVisible(enableViewButton);

    if (probe->availableApGains.size() > 0)
    {
        currentHeight += 55;

        apGainComboBox = new ComboBox("apGainComboBox");
        apGainComboBox->setBounds(450, currentHeight, 65, 22);
        apGainComboBox->addListener(this);

        for (int i = 0; i < probe->availableApGains.size(); i++)
            apGainComboBox->addItem(String(probe->availableApGains[i]) + "x", i + 1);

        apGainComboBox->setSelectedId(probe->apGainIndex + 1, dontSendNotification);
        addAndMakeVisible(apGainComboBox);

        apGainViewButton = new UtilityButton("VIEW", Font("Small Text", 12, Font::plain));
        apGainViewButton->setRadius(3.0f);
        apGainViewButton->setBounds(530, currentHeight + 2, 45, 18);
        apGainViewButton->addListener(this);
        apGainViewButton->setTooltip("View AP gain of each channel");
        addAndMakeVisible(apGainViewButton);

        apGainLabel = new Label("AP GAIN", "AP GAIN");
        apGainLabel->setFont(Font("Small Text", 13, Font::plain));
        apGainLabel->setBounds(446, currentHeight - 20, 100, 20);
        apGainLabel->setColour(Label::textColourId, Colours::grey);
        addAndMakeVisible(apGainLabel);

    }

    if (probe->availableLfpGains.size() > 0)
    {
        currentHeight += 55;

        lfpGainComboBox = new ComboBox("lfpGainComboBox");
        lfpGainComboBox->setBounds(450, currentHeight, 65, 22);
        lfpGainComboBox->addListener(this);

        for (int i = 0; i < probe->availableLfpGains.size(); i++)
           lfpGainComboBox->addItem(String(probe->availableLfpGains[i]) + "x", i + 1);

        lfpGainComboBox->setSelectedId(probe->lfpGainIndex + 1, dontSendNotification);
        addAndMakeVisible(lfpGainComboBox);

        lfpGainViewButton = new UtilityButton("VIEW", Font("Small Text", 12, Font::plain));
        lfpGainViewButton->setRadius(3.0f);
        lfpGainViewButton->setBounds(530, currentHeight + 2, 45, 18);
        lfpGainViewButton->addListener(this);
        lfpGainViewButton->setTooltip("View LFP gain of each channel");
        addAndMakeVisible(lfpGainViewButton);

        lfpGainLabel = new Label("LFP GAIN", "LFP GAIN");
        lfpGainLabel->setFont(Font("Small Text", 13, Font::plain));
        lfpGainLabel->setBounds(446, currentHeight - 20, 100, 20);
        lfpGainLabel->setColour(Label::textColourId, Colours::grey);
        addAndMakeVisible(lfpGainLabel);
    }

    if (probe->availableReferences.size() > 0)
    {
        currentHeight += 55;

        referenceComboBox = new ComboBox("ReferenceComboBox");
        referenceComboBox->setBounds(450, currentHeight, 65, 22);
        referenceComboBox->addListener(this);

        for (int i = 0; i < probe->availableReferences.size(); i++)
        {
            referenceComboBox->addItem(probe->availableReferences[i], i+1);
        }

        referenceComboBox->setSelectedId(probe->referenceIndex + 1, dontSendNotification);
        addAndMakeVisible(referenceComboBox);

        referenceViewButton = new UtilityButton("VIEW", Font("Small Text", 12, Font::plain));
        referenceViewButton->setRadius(3.0f);
        referenceViewButton->setBounds(530, currentHeight + 2, 45, 18);
        referenceViewButton->addListener(this);
        referenceViewButton->setTooltip("View reference of each channel");
        addAndMakeVisible(referenceViewButton);

        referenceLabel = new Label("REFERENCE", "REFERENCE");
        referenceLabel->setFont(Font("Small Text", 13, Font::plain));
        referenceLabel->setBounds(446, currentHeight - 20, 100, 20);
        referenceLabel->setColour(Label::textColourId, Colours::grey);
        addAndMakeVisible(referenceLabel);
    }

    if (probe->hasApFilterSwitch())
    {
        currentHeight += 55;

        filterComboBox = new ComboBox("FilterComboBox");
        filterComboBox->setBounds(450, currentHeight, 75, 22);
        filterComboBox->addListener(this);
        filterComboBox->addItem("ON", 1);
        filterComboBox->addItem("OFF", 2);
        filterComboBox->setSelectedId(1, dontSendNotification);
        addAndMakeVisible(filterComboBox);

        filterLabel = new Label("FILTER", "AP FILTER CUT");
        filterLabel->setFont(Font("Small Text", 13, Font::plain));
        filterLabel->setBounds(446, currentHeight - 20, 200, 20);
        filterLabel->setColour(Label::textColourId, Colours::grey);
        addAndMakeVisible(filterLabel);
    }

    // BIST
    bistComboBox = new ComboBox("BistComboBox");
    bistComboBox->setBounds(650, 500, 225, 22);
    bistComboBox->addListener(this);

    availableBists.add(BIST::EMPTY);
    availableBists.add(BIST::SIGNAL);
    bistComboBox->addItem("Test probe signal", 1);

    availableBists.add(BIST::NOISE);
    bistComboBox->addItem("Test probe noise", 2);

    availableBists.add(BIST::PSB);
    bistComboBox->addItem("Test PSB bus", 3);

    availableBists.add(BIST::SR);
    bistComboBox->addItem("Test shift registers", 4);

    availableBists.add(BIST::EEPROM);
    bistComboBox->addItem("Test EEPROM", 5);

    availableBists.add(BIST::I2C);
    bistComboBox->addItem("Test I2C", 6);

    availableBists.add(BIST::SERDES);
    bistComboBox->addItem("Test Serdes", 7);

    availableBists.add(BIST::HB);
    bistComboBox->addItem("Test Heartbeat", 8);

    availableBists.add(BIST::BS);
    bistComboBox->addItem("Test Basestation", 9);
    addAndMakeVisible(bistComboBox);

    bistButton = new UtilityButton("RUN", Font("Small Text", 12, Font::plain));
    bistButton->setRadius(3.0f);
    bistButton->setBounds(880, 500, 50, 22);
    bistButton->addListener(this);
    bistButton->setTooltip("Run selected test");
    addAndMakeVisible(bistButton);

    bistLabel = new Label("BIST", "Built-in self tests:");
    bistLabel->setFont(Font("Small Text", 13, Font::plain));
    bistLabel->setBounds(650, 473, 200, 20);
    bistLabel->setColour(Label::textColourId, Colours::grey);
    addAndMakeVisible(bistLabel);

    // FIRMWARE
    firmwareToggleButton = new UtilityButton("UPDATE FIRMWARE...", Font("Small Text", 12, Font::plain));
    firmwareToggleButton->setRadius(3.0f);
    firmwareToggleButton->addListener(this);
    firmwareToggleButton->setBounds(650, 550, 150, 22);
    firmwareToggleButton->setClickingTogglesState(true);
    addAndMakeVisible(firmwareToggleButton);

    bscFirmwareComboBox = new ComboBox("bscFirmwareComboBox");
    bscFirmwareComboBox->setBounds(550, 620, 375, 22);
    bscFirmwareComboBox->addListener(this);
    bscFirmwareComboBox->addItem("Select file...", 1);
    addChildComponent(bscFirmwareComboBox);

    bscFirmwareButton = new UtilityButton("UPLOAD", Font("Small Text", 12, Font::plain));
    bscFirmwareButton->setRadius(3.0f);
    bscFirmwareButton->setBounds(930, 620, 60, 22);
    bscFirmwareButton->addListener(this);
    bscFirmwareButton->setTooltip("Upload firmware to selected basestation connect board");
    addChildComponent(bscFirmwareButton);

    bscFirmwareLabel = new Label("BSC FIRMWARE", "1. Update basestation connect board firmware:");
    bscFirmwareLabel->setFont(Font("Small Text", 13, Font::plain));
    bscFirmwareLabel->setBounds(550, 593, 500, 20);
    bscFirmwareLabel->setColour(Label::textColourId, Colours::orange);
    addChildComponent(bscFirmwareLabel);

    bsFirmwareComboBox = new ComboBox("bscFirmwareComboBox");
    bsFirmwareComboBox->setBounds(550, 690, 375, 22);
    bsFirmwareComboBox->addListener(this);
    bsFirmwareComboBox->addItem("Select file...", 1);
    addChildComponent(bsFirmwareComboBox);

    bsFirmwareButton = new UtilityButton("UPLOAD", Font("Small Text", 12, Font::plain));
    bsFirmwareButton->setRadius(3.0f);
    bsFirmwareButton->setBounds(930, 690, 60, 22);
    bsFirmwareButton->addListener(this);
    bsFirmwareButton->setTooltip("Upload firmware to selected basestation");
    addChildComponent(bsFirmwareButton);

    bsFirmwareLabel = new Label("BS FIRMWARE", "2. Update basestation firmware:");
    bsFirmwareLabel->setFont(Font("Small Text", 13, Font::plain));
    bsFirmwareLabel->setBounds(550, 663, 500, 20);
    bsFirmwareLabel->setColour(Label::textColourId, Colours::orange);
    addChildComponent(bsFirmwareLabel);

    firmwareInstructionsLabel = new Label("FIRMWARE INSTRUCTIONS", "3. Power cycle computer and PXI chassis");
    firmwareInstructionsLabel->setFont(Font("Small Text", 13, Font::plain));
    firmwareInstructionsLabel->setBounds(550, 733, 500, 20);
    firmwareInstructionsLabel->setColour(Label::textColourId, Colours::orange);
    addChildComponent(firmwareInstructionsLabel);

    // COPY / PASTE / UPLOAD
    copyButton = new UtilityButton("COPY", Font("Small Text", 12, Font::plain));
    copyButton->setRadius(3.0f);
    copyButton->setBounds(45, 637, 60, 22);
    copyButton->addListener(this);
    copyButton->setTooltip("Copy probe settings");
    addAndMakeVisible(copyButton);

    pasteButton = new UtilityButton("PASTE", Font("Small Text", 12, Font::plain));
    pasteButton->setRadius(3.0f);
    pasteButton->setBounds(115, 637, 60, 22);
    pasteButton->addListener(this);
    pasteButton->setTooltip("Paste probe settings");
    addAndMakeVisible(pasteButton);

    applyToAllButton = new UtilityButton("APPLY TO ALL", Font("Small Text", 12, Font::plain));
    applyToAllButton->setRadius(3.0f);
    applyToAllButton->setBounds(185, 637, 120, 22);
    applyToAllButton->addListener(this);
    applyToAllButton->setTooltip("Apply this probe's settings to all others");
    addAndMakeVisible(applyToAllButton);

    saveImroButton = new UtilityButton("SAVE TO IMRO", Font("Small Text", 12, Font::plain));
    saveImroButton->setRadius(3.0f);
    saveImroButton->setBounds(45, 672, 120, 22);
    saveImroButton->addListener(this);
    saveImroButton->setTooltip("Save channel map to .imro file");
    addAndMakeVisible(saveImroButton);

    loadImroButton = new UtilityButton("LOAD FROM IMRO", Font("Small Text", 12, Font::plain));
    loadImroButton->setRadius(3.0f);
    loadImroButton->setBounds(175, 672, 130, 22);
    loadImroButton->addListener(this);
    loadImroButton->setTooltip("Load channel map from .imro file");
    addAndMakeVisible(loadImroButton);

    probeSettingsLabel = new Label("Settings", "Probe settings:");
    probeSettingsLabel->setFont(Font("Small Text", 13, Font::plain));
    probeSettingsLabel->setBounds(40, 610, 300, 20);
    probeSettingsLabel->setColour(Label::textColourId, Colours::grey);
    addAndMakeVisible(probeSettingsLabel);


    // PROBE INFO 
    mainLabel = new Label("MAIN", "MAIN");
    mainLabel->setFont(Font("Small Text", 60, Font::plain));
    mainLabel->setBounds(625, 20, 200, 65);
    mainLabel->setColour(Label::textColourId, Colours::darkkhaki);
    addAndMakeVisible(mainLabel);

    nameLabel = new Label("MAIN", "NAME");
    nameLabel->setFont(Font("Small Text", 20, Font::plain));
    nameLabel->setBounds(625, 90, 500, 25);
    nameLabel->setColour(Label::textColourId, Colours::pink);
    addAndMakeVisible(nameLabel);

    infoLabelView = new Viewport("INFO");
    infoLabelView->setBounds(625, 128, 750, 400);
    addAndMakeVisible(infoLabelView);

    infoLabel = new Label("INFO", "INFO");
    infoLabelView->setViewedComponent(infoLabel, false);
    infoLabel->setFont(Font("Small Text", 13, Font::plain));
    infoLabel->setBounds(0, 0, 750, 350);
    infoLabel->setJustificationType(Justification::topLeft);
    infoLabel->setColour(Label::textColourId, Colours::lightgrey);

    
    // ANNOTATIONS
    annotationButton = new UtilityButton("ADD", Font("Small Text", 12, Font::plain));
    annotationButton->setRadius(3.0f);
    annotationButton->setBounds(400, 680, 40, 18);
    annotationButton->addListener(this);
    annotationButton->setTooltip("Add annotation to selected channels");
    //addAndMakeVisible(annotationButton);
   
    annotationLabel = new Label("ANNOTATION", "Custom annotation");
    annotationLabel->setBounds(396, 620, 200, 20);
    annotationLabel->setColour(Label::textColourId, Colours::white);
    annotationLabel->setEditable(true);
    annotationLabel->addListener(this);
   // addAndMakeVisible(annotationLabel);

    annotationLabelLabel = new Label("ANNOTATION_LABEL", "ANNOTATION");
    annotationLabelLabel->setFont(Font("Small Text", 13, Font::plain));
    annotationLabelLabel->setBounds(396, 600, 200, 20);
    annotationLabelLabel->setColour(Label::textColourId, Colours::grey);
   // addAndMakeVisible(annotationLabelLabel);

    colorSelector = new ColorSelector(this);
    colorSelector->setBounds(400, 650, 250, 20);
   // addAndMakeVisible(colorSelector);

    updateInfoString();

}

NeuropixInterface::~NeuropixInterface()
{

}

void NeuropixInterface::updateInfoString()
{

    String mainString, nameString, infoString;

    mainString += String(probe->basestation->slot + 1);
    mainString += ":";
    mainString += String(probe->headstage->port + 1);

    if (probe->type == ProbeType::NP2_1 || probe->type == ProbeType::NP2_4)
    {
        mainString += ":";
        mainString += String(probe->dock + 1);
    }
    
    nameString = probe->name;

    infoString += "API VERSION: ";
    infoString += thread->getApiVersion();
    infoString += "\n";
    infoString += "\n";

    infoString += "Basestation";
    infoString += "\n hardware version: " + probe->basestation->info.version;
    infoString += "\n firmware version: " + probe->basestation->info.boot_version;
    infoString += "\n";
    infoString += "\n";

    infoString += "Basestation connect board";
    infoString += "\n hardware version: " + probe->basestation->basestationConnectBoard->info.version;
    infoString += "\n firmware version: " + probe->basestation->basestationConnectBoard->info.boot_version;
    infoString += "\n";
    infoString += "\n";

    infoString += "Headstage: " + probe->headstage->info.part_number;
    infoString += "\n";
    infoString += "\n";

    infoString += "Flex: " + probe->flex->info.part_number;
    infoString += "\n";
    infoString += "\n";

    infoString += "Probe: ";
    infoString += "\n S/N: " + String(probe->info.serial_number);
    infoString += "\n Part number: " + probe->info.part_number;
    infoString += "\n";


    infoLabel->setText(infoString, dontSendNotification);
    nameLabel->setText(nameString, dontSendNotification);
    mainLabel->setText(mainString, dontSendNotification);

}

void NeuropixInterface::labelTextChanged(Label* label)
{
    if (label == annotationLabel)
    {
        colorSelector->updateCurrentString(label->getText());
    }
}

void NeuropixInterface::updateProbeSettingsInBackground()
{
    ProbeSettings settings = getProbeSettings();

    thread->updateProbeSettingsQueue(settings);
    editor->uiLoader->startThread();
}

void NeuropixInterface::comboBoxChanged(ComboBox* comboBox)
{

    if (!editor->acquisitionIsActive)
    {
        if ((comboBox == apGainComboBox) || (comboBox == lfpGainComboBox))
        {
            updateProbeSettingsInBackground();
        }
        else if (comboBox == referenceComboBox)
        {

            updateProbeSettingsInBackground();

        }
        else if (comboBox == filterComboBox)
        {
            updateProbeSettingsInBackground();
        }
        else if (comboBox == bscFirmwareComboBox)
        {
            if (comboBox->getSelectedId() == 1)
            {
                FileChooser fileChooser("Select a QBSC .bin file to load.", File(), "QBSC*.bin");

                if (fileChooser.browseForFileToOpen())
                {
                    comboBox->addItem(fileChooser.getResult().getFullPathName(), comboBox->getNumItems() + 1);
                    comboBox->setSelectedId(comboBox->getNumItems());
                }
                else {
                    comboBox->setSelectedId(0);
                }
            }
        }
        else if (comboBox == bsFirmwareComboBox)
        {
            if (comboBox->getSelectedId() == 1)
            {
                FileChooser fileChooser("Select a BS .bin file to load.", File(), "BS*.bin");

                if (fileChooser.browseForFileToOpen())
                {
                    comboBox->addItem(fileChooser.getResult().getFullPathName(), comboBox->getNumItems() + 1);
                    comboBox->setSelectedId(comboBox->getNumItems());
                }
                else {
                    comboBox->setSelectedId(0);
                }
            }
        }
        else if (comboBox == filterComboBox)
        {
            updateProbeSettingsInBackground();
        }


        repaint();
    }
    else {
        CoreServices::sendStatusMessage("Cannot update parameters while acquisition is active");// no parameter change while acquisition is active
    }

}

void NeuropixInterface::setAnnotationLabel(String s, Colour c)
{
    annotationLabel->setText(s, NotificationType::dontSendNotification);
    annotationLabel->setColour(Label::textColourId, c);
}

void NeuropixInterface::buttonClicked(Button* button)
{
    if (button == enableViewButton)
    {
        mode = ENABLE_VIEW;
        stopTimer();
        repaint();
    }
    else if (button == apGainViewButton)
    {
        mode = AP_GAIN_VIEW;
        stopTimer();
        repaint();
    }
    else if (button == lfpGainViewButton)
    {
        mode = LFP_GAIN_VIEW;
        stopTimer();
        repaint();
    }
    else if (button == referenceViewButton)
    {
        mode = REFERENCE_VIEW;
        stopTimer();
        repaint();
    }
    /*else if (button == activityViewButton)
    {
        mode = ACTIVITY_VIEW;
        startTimer();
        repaint();
    }*/
    else if (button == enableButton)
    {

        Array<int> selection = getSelectedElectrodes();

        // update selection state
        for (int i = 0; i < selection.size(); i++)
        {
            Bank bank = electrodeMetadata[selection[i]].bank;
            int channel = electrodeMetadata[selection[i]].channel;
            int shank = electrodeMetadata[selection[i]].shank;
            
            for (int j = 0; j < electrodeMetadata.size(); j++)
            {
                if (electrodeMetadata[j].channel == channel)
                {
                    if (electrodeMetadata[j].bank == bank && electrodeMetadata[j].shank == shank)
                    {
                        if (electrodeMetadata[j].status == ElectrodeStatus::OPTIONAL_REFERENCE ||
                            electrodeMetadata[j].status == ElectrodeStatus::CONNECTED_OPTIONAL_REFERENCE)
                            electrodeMetadata.getReference(j).status = ElectrodeStatus::CONNECTED_OPTIONAL_REFERENCE;
                        else if (electrodeMetadata[j].status == ElectrodeStatus::REFERENCE)
                            ; // skip
                        else
                            electrodeMetadata.getReference(j).status = ElectrodeStatus::CONNECTED;
                    }
                        
                    else
                    {
                        if (electrodeMetadata[j].status == ElectrodeStatus::CONNECTED_OPTIONAL_REFERENCE ||
                            electrodeMetadata[j].status == ElectrodeStatus::OPTIONAL_REFERENCE)
                            electrodeMetadata.getReference(j).status = ElectrodeStatus::OPTIONAL_REFERENCE;
                        else if (electrodeMetadata[j].status == ElectrodeStatus::REFERENCE)
                            ; // skipp
                        else
                            electrodeMetadata.getReference(j).status = ElectrodeStatus::DISCONNECTED;
                    }
                        
                }
            }
        }

        repaint();

        updateProbeSettingsInBackground();
    }
    else if (button == annotationButton)
    {
        String s = annotationLabel->getText();
        Array<int> a = getSelectedElectrodes();

        if (a.size() > 0)
            annotations.add(Annotation(s, a, colorSelector->getCurrentColour()));

        repaint();
    }
    else if (button == bistButton)
    {
        if (!editor->acquisitionIsActive)
        {
            if (bistComboBox->getSelectedId() == 0)
            {
                CoreServices::sendStatusMessage("Please select a test to run.");
            }
            else {
                bool passed = probe->basestation->runBist(probe->headstage->port, probe->dock,
                                                          availableBists[bistComboBox->getSelectedId()]);

                String testString = bistComboBox->getText();

                //Check if testString already has test result attached
                String result = testString.substring(testString.length() - 6);
                if (result.compare("PASSED") == 0 || result.compare("FAILED") == 0)
                {
                    testString = testString.dropLastCharacters(9);
                }

                if (passed)
                {
                    testString += " - PASSED";
                }
                else {
                    testString += " - FAILED";
                }
                //bistComboBox->setText(testString);
                bistComboBox->changeItemText(bistComboBox->getSelectedId(), testString);
                bistComboBox->setText(testString);
                //bistComboBox->setSelectedId(bistComboBox->getSelectedId(), NotificationType::sendNotification);
            }

        }
        else {
            CoreServices::sendStatusMessage("Cannot run test while acquisition is active.");
        }
    }
    else if (button == loadImroButton)
    {
        FileChooser fileChooser("Select an IMRO file to load.", File(), "*.imro");

        if (fileChooser.browseForFileToOpen())
        {
            ProbeSettings settings;
            bool success = IMRO::readSettingsFromImro(fileChooser.getResult(), settings);

            if (success)
            {
                applyProbeSettings(settings);
            }
                
        }
    }
    else if (button == saveImroButton)
    {
        FileChooser fileChooser("Save settings to an IMRO file.", File(), "*.imro");

        if (fileChooser.browseForFileToSave(true))
        {
            bool success = IMRO::writeSettingsToImro(fileChooser.getResult(), getProbeSettings());

            if (!success)
                CoreServices::sendStatusMessage("Failed to write probe settings.");
            else
                CoreServices::sendStatusMessage("Successfully wrote probe settings.");

        }
    }
    else if (button == copyButton)
    {
        canvas->storeProbeSettings(getProbeSettings());
        CoreServices::sendStatusMessage("Probe settings copied.");
    }
    else if (button == pasteButton)
    {
        applyProbeSettings(canvas->getProbeSettings());
       // CoreServices::sendStatusMessage("Applied saved settings to this probe.");
    }
    else if (button == applyToAllButton)
    {
        canvas->applyParametersToAllProbes(getProbeSettings());
        CoreServices::sendStatusMessage("Applied saved settings to all probes of same type.");
    }
    else if (button == firmwareToggleButton)
    {
        bool state = button->getToggleState();

        bscFirmwareButton->setVisible(state);
        bscFirmwareComboBox->setVisible(state);
        bscFirmwareLabel->setVisible(state);

        bsFirmwareButton->setVisible(state);
        bsFirmwareComboBox->setVisible(state);
        bsFirmwareLabel->setVisible(state);

        firmwareInstructionsLabel->setVisible(state);

        repaint();
    }
    else if (button == bsFirmwareButton)
    {
        if (bsFirmwareComboBox->getSelectedId() > 1)
        {
            probe->basestation->updateBsFirmware(File(bsFirmwareComboBox->getText()));
        }
        else {
            CoreServices::sendStatusMessage("No file selected.");
        }
       
    }
    else if (button == bscFirmwareButton)
    {
        if (bscFirmwareComboBox->getSelectedId() > 1)
        {
            probe->basestation->updateBscFirmware(File(bscFirmwareComboBox->getText()));
        }
        else {
            CoreServices::sendStatusMessage("No file selected.");
        }
    }

}

Array<int> NeuropixInterface::getSelectedElectrodes()
{
    Array<int> electrodeIndices;

    for (int i = 0; i < electrodeMetadata.size(); i++)
    {
        if (electrodeMetadata[i].isSelected)
        {
            electrodeIndices.add(i);
        }
    }

    return electrodeIndices;
}

void NeuropixInterface::mouseMove(const MouseEvent& event)
{
    float y = event.y;
    float x = event.x;

    //std::cout << x << " " << y << std::endl;

    bool isOverZoomRegionNew = false;
    bool isOverUpperBorderNew = false;
    bool isOverLowerBorderNew = false;

    // check for move into zoom region
    if ((y > lowerBound - zoomOffset - zoomHeight - dragZoneWidth / 2)
        && (y < lowerBound - zoomOffset + dragZoneWidth / 2)
        && (x > 9) 
        && (x < 54 + shankOffset))
    {
        isOverZoomRegionNew = true;
    }
    else {
        isOverZoomRegionNew = false;
    }

    // check for move over upper border or lower border
    if (isOverZoomRegionNew)
    {
        if (y > lowerBound - zoomHeight - zoomOffset - dragZoneWidth / 2
            && y < lowerBound - zoomHeight - zoomOffset + dragZoneWidth / 2)
        {
            isOverUpperBorderNew = true;

        }
        else if (y > lowerBound - zoomOffset - dragZoneWidth / 2
            && y < lowerBound - zoomOffset + dragZoneWidth / 2)
        {
            isOverLowerBorderNew = true;

        }
        else {
            isOverUpperBorderNew = false;
            isOverLowerBorderNew = false;
        }
    }

    // update cursor type
    if (isOverZoomRegionNew != isOverZoomRegion ||
        isOverLowerBorderNew != isOverLowerBorder ||
        isOverUpperBorderNew != isOverUpperBorder)
    {
        isOverZoomRegion = isOverZoomRegionNew;
        isOverUpperBorder = isOverUpperBorderNew;
        isOverLowerBorder = isOverLowerBorderNew;

        if (!isOverZoomRegion)
        {
            cursorType = MouseCursor::NormalCursor;
        }
        else {

            if (isOverUpperBorder)
                cursorType = MouseCursor::TopEdgeResizeCursor;
            else if (isOverLowerBorder)
                cursorType = MouseCursor::BottomEdgeResizeCursor;
            else
                cursorType = MouseCursor::NormalCursor;
        }

        repaint();
    }



    // check for movement over electrode
    if ((x > leftEdge)  // in electrode selection region
        && (x < rightEdge)
        && (y < lowerBound)
        && (y > 18)
        && (event.eventComponent->getWidth() > 800))
    {
        int index = getNearestElectrode(x, y);

        if (index > -1)
        {
            isOverElectrode = true;
            electrodeInfoString = getElectrodeInfoString(index);
        }
       
        repaint();
    }
    else {
        bool isOverChannelNew = false;

        if (isOverChannelNew != isOverElectrode)
        {
            isOverElectrode = isOverChannelNew;
            repaint();
        }
    }

}

int NeuropixInterface::getNearestElectrode(int x, int y)
{

    //int TOP_BORDER = 33;
    //int SHANK_HEIGHT = 480;

   // float xLoc = 220 + shankOffset - electrodeHeight * probeMetadata.columns_per_shank / 2
   //     + electrodeHeight * electrodeMetadata[i].column_index + electrodeMetadata[i].shank_index * electrodeHeight * 4
   //     - (probeMetadata.shank_count / 2 * electrodeHeight * 3);
   // float yLoc = lowerBound - ((electrodeMetadata[i].row_index - int(lowestRow)) * electrodeHeight);


    int row = (lowerBound - y) / electrodeHeight + zoomAreaMinRow + 1;

    int shankWidth = electrodeHeight * probeMetadata.columns_per_shank;
    int totalWidth = shankWidth * probeMetadata.shank_count + shankWidth * (probeMetadata.shank_count - 1);

    int shank = 0;
    int column = -1;

    for (shank = 0; shank < probeMetadata.shank_count; shank++)
    {
        int leftEdge = 220 + shankOffset - totalWidth / 2 + shankWidth * 2 * shank;
        int rightEdge = leftEdge + shankWidth;

        if (x >= leftEdge && x <= rightEdge)
        {
            column = (x - leftEdge) / electrodeHeight;
            break;
        }
    }

    //std::cout << "x: " << x << ", column: " << column << ", shank: " << shank << std::endl;
    //std::cout << "y: " << y <<  ", row: " << row << std::endl;

    for (int i = 0; i < electrodeMetadata.size(); i++)
    {
        if ((electrodeMetadata[i].row_index == row)
            && (electrodeMetadata[i].column_index == column)
            && (electrodeMetadata[i].shank == shank))
        {
            return i;
        }
    }

    return -1;
}

Array<int> NeuropixInterface::getElectrodesWithinBounds(int x, int y, int w, int h)
{
    int startrow = (lowerBound - y - h) / electrodeHeight + zoomAreaMinRow + 1;
    int endrow = (lowerBound - y) / electrodeHeight + zoomAreaMinRow + 1;

    int shankWidth = electrodeHeight * probeMetadata.columns_per_shank;
    int totalWidth = shankWidth * probeMetadata.shank_count + shankWidth * (probeMetadata.shank_count - 1);

    Array<int> selectedColumns;

    for (int i = 0; i < probeMetadata.shank_count * probeMetadata.columns_per_shank; i++)
    {
        int shank = i / probeMetadata.columns_per_shank;
        int column = i % probeMetadata.columns_per_shank;

        int l = leftEdge + shankWidth * 2 * shank + electrodeHeight * column;
        int r = l + electrodeHeight/2;

        if (x < l + electrodeHeight/2 && x + w > r)
            selectedColumns.add(i);
    }

    //int startcolumn = (x - (220 + shankOffset - electrodeHeight * probeMetadata.columns_per_shank / 2)) / electrodeHeight;
    //int endcolumn = (x + w - (220 + shankOffset - electrodeHeight * probeMetadata.columns_per_shank / 2)) / electrodeHeight;

    Array<int> inds; 

    //std::cout << startrow << " " << endrow << " " << startcolumn << " " << endcolumn << std::endl;

    for (int i = 0; i < electrodeMetadata.size(); i++)
    {
        if ( (electrodeMetadata[i].row_index >= startrow) && 
             (electrodeMetadata[i].row_index <= endrow))
             
        {
            int column_id = electrodeMetadata[i].shank * probeMetadata.columns_per_shank + electrodeMetadata[i].column_index;
            
            if (selectedColumns.indexOf(column_id) > -1)
            {
                    inds.add(i);
            }

        }
    }

    return inds;

}

String NeuropixInterface::getElectrodeInfoString(int index)
{
    String a;
    a += "Electrode ";
    a += String(electrodeMetadata[index].global_index + 1);

    a += "\n\Bank ";

    switch (electrodeMetadata[index].bank)
    {
        case Bank::A:
            a += "A";
            break;
        case Bank::B:
            a += "B";
            break;
        case Bank::C:
            a += "C";
            break;
        case Bank::D:
            a += "D";
            break;
        case Bank::E:
            a += "E";
            break;
        case Bank::F:
            a += "F";
            break;
        case Bank::G:
            a += "G";
            break;
        case Bank::H:
            a += "H";
            break;
        case Bank::I:
            a += "I";
            break;
        case Bank::J:
            a += "J";
            break;
        case Bank::K:
            a += "K";
            break;
        case Bank::L:
            a += "L";
            break;
        case Bank::M:
            a += "M";
            break;
        default:
            a += " NONE";
    }
    
    a += ", Channel ";
    a += String(electrodeMetadata[index].channel + 1);

    a += "\n\nType: ";

    if (electrodeMetadata[index].status == ElectrodeStatus::REFERENCE || 
        electrodeMetadata[index].status == ElectrodeStatus::OPTIONAL_REFERENCE ||
        electrodeMetadata[index].status == ElectrodeStatus::CONNECTED_OPTIONAL_REFERENCE)
    {
        a += "REFERENCE";
    }
    else
    {
        a += "SIGNAL";

        a += "\nEnabled: ";

        if (electrodeMetadata[index].status == ElectrodeStatus::CONNECTED || 
            electrodeMetadata[index].status == ElectrodeStatus::CONNECTED_OPTIONAL_REFERENCE)
            a += "YES";
        else
            a += "NO";
    }

    if (apGainComboBox != nullptr)
    {
        a += "\nAP Gain: ";
        a += String(apGainComboBox->getText());
    }
    
    if (lfpGainComboBox != nullptr)
    {
        a += "\nLFP Gain: ";
        a += String(lfpGainComboBox->getText());
    }
    
    if (referenceComboBox != nullptr)
    {
        a += "\nReference: ";
        a += String(referenceComboBox->getText());
    }
    
    return a;
}

void NeuropixInterface::startAcquisition()
{

    bool enabledState = false;

    if (enableButton != nullptr)
        enableButton->setEnabled(enabledState);

    if (apGainComboBox != nullptr)
        apGainComboBox->setEnabled(enabledState);

    if (lfpGainComboBox != nullptr)
        lfpGainComboBox->setEnabled(enabledState);

    if (filterComboBox != nullptr)
        filterComboBox->setEnabled(enabledState);

    if (referenceComboBox != nullptr)
        referenceComboBox->setEnabled(enabledState);

    if (bistComboBox != nullptr)
        bistComboBox->setEnabled(enabledState);

    if (bistButton != nullptr)
        bistButton->setEnabled(enabledState);
}

void NeuropixInterface::stopAcquisition()
{
    bool enabledState = true;

    if (enableButton != nullptr)
        enableButton->setEnabled(enabledState);

    if (apGainComboBox != nullptr)
        apGainComboBox->setEnabled(enabledState);

    if (lfpGainComboBox != nullptr)
        lfpGainComboBox->setEnabled(enabledState);

    if (filterComboBox != nullptr)
        filterComboBox->setEnabled(enabledState);

    if (referenceComboBox != nullptr)
        referenceComboBox->setEnabled(enabledState);

    if (bistComboBox != nullptr)
        bistComboBox->setEnabled(enabledState);

    if (bistButton != nullptr)
        bistButton->setEnabled(enabledState);
}

void NeuropixInterface::mouseUp(const MouseEvent& event)
{
    if (isSelectionActive)
    {

        isSelectionActive = false;
        repaint();
    }

}

void NeuropixInterface::mouseDown(const MouseEvent& event)
{
    initialOffset = zoomOffset;
    initialHeight = zoomHeight;

    //std::cout << event.x << std::endl;

    if (!event.mods.isRightButtonDown())
    {
        if (event.x > 150 && event.x < 400)
        {

            if (!event.mods.isShiftDown())
            {
                for (int i = 0; i < electrodeMetadata.size(); i++)
                    electrodeMetadata.getReference(i).isSelected = false;
            }

            if (event.x > leftEdge && event.x < rightEdge)
            {
                int chan = getNearestElectrode(event.x, event.y);

                //std::cout << chan << std::endl;

                if (chan >= 0 && chan < electrodeMetadata.size())
                {
                    electrodeMetadata.getReference(chan).isSelected = true;
                }

            }
            repaint();
        }
    }
    else {

        if (event.x > 225 + 10 && event.x < 225 + 150)
        {
            int currentAnnotationNum;

            for (int i = 0; i < annotations.size(); i++)
            {
                Annotation& a = annotations.getReference(i);
                float yLoc = a.currentYLoc;

                if (float(event.y) < yLoc && float(event.y) > yLoc - 12)
                {
                    currentAnnotationNum = i;
                    break;
                }
                else {
                    currentAnnotationNum = -1;
                }
            }

            if (currentAnnotationNum > -1)
            {
                PopupMenu annotationMenu;

                annotationMenu.addItem(1, "Delete annotation", true);

                const int result = annotationMenu.show();

                switch (result)
                {
                case 0:
                    break;
                case 1:
                    annotations.removeRange(currentAnnotationNum, 1);
                    repaint();
                    break;
                default:

                    break;
                }
            }

        }



        // if (event.x > 225 - channelHeight && event.x < 225 + channelHeight)
        // {
        //  PopupMenu annotationMenu;

     //        annotationMenu.addItem(1, "Add annotation", true);
        //  const int result = annotationMenu.show();

     //        switch (result)
     //        {
     //            case 1:
     //                std::cout << "Annotate!" << std::endl;
     //                break;
     //            default:
     //             break;
        //  }
        // }

    }


}

void NeuropixInterface::mouseDrag(const MouseEvent& event)
{
    if (isOverZoomRegion)
    {
        if (isOverUpperBorder)
        {
            zoomHeight = initialHeight - event.getDistanceFromDragStartY();

            if (zoomHeight > lowerBound - zoomOffset - 18)
                zoomHeight = lowerBound - zoomOffset - 18;
        }
        else if (isOverLowerBorder)
        {

            zoomOffset = initialOffset - event.getDistanceFromDragStartY();

            if (zoomOffset < 0)
            {
                zoomOffset = 0;
            }
            else {
                zoomHeight = initialHeight + event.getDistanceFromDragStartY();
            }

        }
        else {
            zoomOffset = initialOffset - event.getDistanceFromDragStartY();
        }
        //std::cout << zoomOffset << std::endl;
    }
    else if (event.x > 150 && event.x < 450)
    {
        int w = event.getDistanceFromDragStartX();
        int h = event.getDistanceFromDragStartY();
        int x = event.getMouseDownX();
        int y = event.getMouseDownY();

        if (w < 0)
        {
            x = x + w; w = -w;
        }

        if (h < 0)
        {
            y = y + h; h = -h;
        }

        //        selectionBox = Rectangle<int>(x, y, w, h);
        isSelectionActive = true;

        //if (x < 225)
        //{
        //int chanStart = getNearestRow(224, y + h) - 1;
       // int chanEnd = getNearestRow(224, y);

        Array<int> inBounds = getElectrodesWithinBounds(x, y, w, h);

        //std::cout << chanStart << " " << chanEnd << std::endl;

        if (x < rightEdge)
        {
            for (int i = 0; i < electrodeMetadata.size(); i++)
            {
                if (inBounds.indexOf(i) > -1)
                {
                    electrodeMetadata.getReference(i).isSelected = true;
                } else
                {
                    if (!event.mods.isShiftDown())
                        electrodeMetadata.getReference(i).isSelected = false;
                }
            }
        }

        repaint();
    }

    if (zoomHeight < minZoomHeight)
        zoomHeight = minZoomHeight;
    if (zoomHeight > maxZoomHeight)
        zoomHeight = maxZoomHeight;

    if (zoomOffset > lowerBound - zoomHeight - 15)
        zoomOffset = lowerBound - zoomHeight - 15;
    else if (zoomOffset < 0)
        zoomOffset = 0;

    repaint();
}

void NeuropixInterface::mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel)
{

    if (event.x > 100 && event.x < 350)
    {

        if (wheel.deltaY > 0)
            zoomOffset += 2;
        else
            zoomOffset -= 2;

        //std::cout << wheel.deltaY << " " << zoomOffset << std::endl;

        if (zoomOffset < 0)
        {
            zoomOffset = 0;
        }
        else if (zoomOffset + 18 + zoomHeight > lowerBound)
        {
            zoomOffset = lowerBound - zoomHeight - 18;
        }

        repaint();
    }
    //else {
    //    canvas->mouseWheelMove(event, wheel);
    //}

}

MouseCursor NeuropixInterface::getMouseCursor()
{
    MouseCursor c = MouseCursor(cursorType);

    return c;
}

void NeuropixInterface::paint(Graphics& g)
{

    int LEFT_BORDER = 30;
    int TOP_BORDER = 33;
    int SHANK_HEIGHT = 480;
    int INTERSHANK_DISTANCE = 30;

    // draw zoomed-out channels channels

    for (int i = 0; i < electrodeMetadata.size(); i++)
    {
        g.setColour(getElectrodeColour(i).withAlpha(0.5f));

        for (int px = 0; px < pixelHeight; px++)
            g.setPixel(LEFT_BORDER + electrodeMetadata[i].column_index * 2 + electrodeMetadata[i].shank * INTERSHANK_DISTANCE,
                       TOP_BORDER + SHANK_HEIGHT - float(electrodeMetadata[i].row_index) * float(SHANK_HEIGHT) / float(probeMetadata.rows_per_shank)- px);
    }

    // channel 1 = pixel 513
    // channel 960 = pixel 33
    // 480 pixels for 960 channels

    // draw channel numbers

    g.setColour(Colours::grey);
    g.setFont(12);

    int ch = 0;

    int ch_interval = SHANK_HEIGHT * channelLabelSkip / probeMetadata.electrodes_per_shank;

    // draw mark for every N channels

    //int WWW = 10;

    shankOffset = INTERSHANK_DISTANCE * (probeMetadata.shank_count - 1); // +WWW;
    for (int i = TOP_BORDER + SHANK_HEIGHT; i > TOP_BORDER; i -= ch_interval)
    {
        g.drawLine(6, i, 18, i);
        g.drawLine(44 + shankOffset, i, 54 + shankOffset, i);
        g.drawText(String(ch), 59 + shankOffset, int(i) - 6, 100, 12, Justification::left, false);
        ch += channelLabelSkip;
    }

   // g.drawLine(220 + shankOffset, 0, 220 + shankOffset, getHeight());

    // draw top channel
    g.drawLine(6, TOP_BORDER, 18, TOP_BORDER);
    g.drawLine(44 + shankOffset, TOP_BORDER, 54 + shankOffset, TOP_BORDER);
    g.drawText(String(probeMetadata.electrodes_per_shank), 
        59 + shankOffset, TOP_BORDER - 6, 100, 12, Justification::left, false);

    // draw shank outline
    g.setColour(Colours::lightgrey);
    

    for (int i = 0; i < probeMetadata.shank_count; i++)
    {
        Path shankPath = probeMetadata.shankOutline;
        shankPath.applyTransform(AffineTransform::translation(INTERSHANK_DISTANCE*i, 0.0f));
        g.strokePath(shankPath, PathStrokeType(1.0));
    }

    // draw zoomed channels
    float miniRowHeight =  float(SHANK_HEIGHT) / float(probeMetadata.rows_per_shank); // pixels per row

    float lowestRow = (zoomOffset-17) / miniRowHeight;
    float highestRow = lowestRow + (zoomHeight / miniRowHeight);

    zoomAreaMinRow = int(lowestRow);

    float totalHeight = float(lowerBound + 100);
    electrodeHeight = totalHeight / (highestRow - lowestRow);

    //std::cout << "Lowest row: " << lowestRow << ", highest row: " << highestRow << std::endl;
    //std::cout << "Zoom offset: " << zoomOffset << ", Zoom height: " << zoomHeight << std::endl;

    for (int i = 0; i < electrodeMetadata.size(); i++)
    {
        if (electrodeMetadata[i].row_index >= int(lowestRow) 
            &&
            electrodeMetadata[i].row_index < int(highestRow))
        {

            float xLoc = 220+ shankOffset - electrodeHeight * probeMetadata.columns_per_shank / 2 
                + electrodeHeight * electrodeMetadata[i].column_index + electrodeMetadata[i].shank * electrodeHeight * 4
                - (probeMetadata.shank_count / 2 * electrodeHeight * 3);
            float yLoc = lowerBound - ((electrodeMetadata[i].row_index - int(lowestRow)) * electrodeHeight);

            //std::cout << "Drawing electrode " << i << ", X: " << xLoc << ", Y:" << yLoc << std::endl;

            if (electrodeMetadata[i].isSelected)
            {
                g.setColour(Colours::white);
                g.fillRect(xLoc, yLoc, electrodeHeight, electrodeHeight);
            }

            g.setColour(getElectrodeColour(i));

            g.fillRect(xLoc + 1, yLoc + 1, electrodeHeight - 2, electrodeHeight - 2);

        }

    }

    // draw annotations
    //drawAnnotations(g);

    // draw borders around zoom area

    g.setColour(Colours::darkgrey.withAlpha(0.7f));
    g.fillRect(25, 0, 25 + shankOffset, lowerBound - zoomOffset - zoomHeight);
    g.fillRect(25, lowerBound - zoomOffset, 25 + shankOffset, zoomOffset + 10);

    g.setColour(Colours::darkgrey);
    g.fillRect(100, 0, 250 + shankOffset, 20);
    g.fillRect(100, lowerBound + 14, 250 +shankOffset, 100);

    if (isOverZoomRegion)
        g.setColour(Colour(25, 25, 25));
    else
        g.setColour(Colour(55, 55, 55));

    Path upperBorder;
    upperBorder.startNewSubPath(5, lowerBound - zoomOffset - zoomHeight);
    upperBorder.lineTo(54 + shankOffset, lowerBound - zoomOffset - zoomHeight);
    upperBorder.lineTo(100 + shankOffset, 16);
    upperBorder.lineTo(330 + shankOffset, 16);

    Path lowerBorder;
    lowerBorder.startNewSubPath(5, lowerBound - zoomOffset);
    lowerBorder.lineTo(54 + shankOffset, lowerBound - zoomOffset);
    lowerBorder.lineTo(100 + shankOffset, lowerBound + 16);
    lowerBorder.lineTo(330 + shankOffset, lowerBound + 16);

    g.strokePath(upperBorder, PathStrokeType(2.0));
    g.strokePath(lowerBorder, PathStrokeType(2.0));

    // draw selection zone

    int shankWidth = electrodeHeight * probeMetadata.columns_per_shank;
    int totalWidth = shankWidth * probeMetadata.shank_count + shankWidth * (probeMetadata.shank_count - 1);

    leftEdge = 220 + shankOffset - totalWidth / 2;
    rightEdge = 220 + shankOffset + totalWidth / 2;

    if (isSelectionActive)
    {
        g.setColour(Colours::white.withAlpha(0.5f));
        //        g.drawRect(selectionBox);
    }

    if (isOverElectrode)
    {
        //std::cout << "YES" << std::endl;
        g.setColour(Colour(55, 55, 55));
        g.setFont(15);
        g.drawMultiLineText(electrodeInfoString,
            280 + shankOffset + 80, 340, 250);
    } 

    drawLegend(g);

    g.setColour(Colour(60, 60, 60));
    g.fillRoundedRectangle(30, 600, 290, 110, 8.0f);

}

void NeuropixInterface::drawAnnotations(Graphics& g)
{
    for (int i = 0; i < annotations.size(); i++)
    {
        bool shouldAppear = false;

        Annotation& a = annotations.getReference(i);

        for (int j = 0; j < a.electrodes.size(); j++)
        {
            if (j > lowestElectrode || j < highestElectrode)
            {
                shouldAppear = true;
                break;
            }
        }

        if (shouldAppear)
        {
            float xLoc = 225 + 30;
            int ch = a.electrodes[0];

            float midpoint = lowerBound / 2 + 8;

            float yLoc = lowerBound - ((ch - lowestElectrode - (ch % 2)) / 2 * electrodeHeight) + 10;

            //if (abs(yLoc - midpoint) < 200)
            yLoc = (midpoint + 3 * yLoc) / 4;
            a.currentYLoc = yLoc;

            float alpha;

            if (yLoc > lowerBound - 250)
                alpha = (lowerBound - yLoc) / (250.f);
            else if (yLoc < 250)
                alpha = 1.0f - (250.f - yLoc) / 200.f;
            else
                alpha = 1.0f;

            if (alpha < 0)
                alpha = -alpha;

            if (alpha < 0)
                alpha = 0;

            if (alpha > 1.0f)
                alpha = 1.0f;

            //float distFromMidpoint = yLoc - midpoint;
            //float ratioFromMidpoint = pow(distFromMidpoint / midpoint,1);

            //if (a.isMouseOver)
            //  g.setColour(Colours::yellow.withAlpha(alpha));
            //else 
            g.setColour(a.colour.withAlpha(alpha));

            g.drawMultiLineText(a.text, xLoc + 2, yLoc, 150);

            float xLoc2 = 225 - electrodeHeight * (1 - (ch % 2)) + electrodeHeight / 2;
            float yLoc2 = lowerBound - ((ch - lowestElectrode - (ch % 2)) / 2 * electrodeHeight) + electrodeHeight / 2;

            g.drawLine(xLoc - 5, yLoc - 3, xLoc2, yLoc2);
            g.drawLine(xLoc - 5, yLoc - 3, xLoc, yLoc - 3);
        }
    }
}

void NeuropixInterface::drawLegend(Graphics& g)
{
    g.setColour(Colour(55, 55, 55));
    g.setFont(15);

    int xOffset = 365 + shankOffset;
    int yOffset = 480;

    switch (mode)
    {
    case ENABLE_VIEW: 
        g.drawMultiLineText("ENABLED?", xOffset, yOffset, 200);
        g.drawMultiLineText("YES", xOffset + 30, yOffset + 22, 200);
        g.drawMultiLineText("NO", xOffset + 30, yOffset + 42, 200);
        g.drawMultiLineText("REFERENCE", xOffset + 30, yOffset + 62, 200);

        g.setColour(Colours::yellow);
        g.fillRect(xOffset + 10, yOffset + 10, 15, 15);

        g.setColour(Colours::grey);
        g.fillRect(xOffset + 10, yOffset + 30, 15, 15);

        g.setColour(Colours::black);
        g.fillRect(xOffset + 10, yOffset + 50, 15, 15);

        break;

    case AP_GAIN_VIEW: 
        g.drawMultiLineText("AP GAIN", xOffset, yOffset, 200);

        for (int i = 0; i < 8; i++)
        {
            g.drawMultiLineText(apGainComboBox->getItemText(i), xOffset + 30, yOffset + 22 + 20 * i, 200);
        }

        for (int i = 0; i < 8; i++)
        {
            g.setColour(Colour(25 * i, 25 * i, 50));
            g.fillRect(xOffset + 10, yOffset + 10 + 20 * i, 15, 15);
        }



        break;

    case LFP_GAIN_VIEW: 
        g.drawMultiLineText("LFP GAIN", xOffset, yOffset, 200);

        for (int i = 0; i < 8; i++)
        {
            g.drawMultiLineText(lfpGainComboBox->getItemText(i), xOffset + 30, yOffset + 22 + 20 * i, 200);
        }

        for (int i = 0; i < 8; i++)
        {
            g.setColour(Colour(66, 25 * i, 35 * i));
            g.fillRect(xOffset + 10, yOffset + 10 + 20 * i, 15, 15);
        }

        break;

    case REFERENCE_VIEW: 
        g.drawMultiLineText("REFERENCE", xOffset, yOffset, 200);

        for (int i = 0; i < referenceComboBox->getNumItems(); i++)
        {
            g.drawMultiLineText(referenceComboBox->getItemText(i), xOffset + 30, yOffset + 22 + 20 * i, 200);
        }


        for (int i = 0; i < referenceComboBox->getNumItems(); i++)
        {
            String referenceDescription = referenceComboBox->getItemText(i);

            if (referenceDescription.contains("Ext"))
                g.setColour(Colours::pink);
            else if (referenceDescription.contains("Tip"))
                g.setColour(Colours::orange);
            else
                g.setColour(Colours::purple);

            g.fillRect(xOffset + 10, yOffset + 10 + 20 * i, 15, 15);
        }

        break;
    }
}

Colour NeuropixInterface::getElectrodeColour(int i)
{
    if (electrodeMetadata[i].status == ElectrodeStatus::DISCONNECTED) // not available
    {
        return Colours::grey;
    }
    else if (electrodeMetadata[i].status == ElectrodeStatus::REFERENCE)
        return Colours::black;
    else if (electrodeMetadata[i].status == ElectrodeStatus::OPTIONAL_REFERENCE)
        return Colours::purple;
    else {
        if (mode == VisualizationMode::ENABLE_VIEW) // ENABLED STATE
        {
            return Colours::yellow;
        }
        else if (mode == VisualizationMode::AP_GAIN_VIEW) // AP GAIN
        {
            return  Colour(25 * probe->apGainIndex, 25 * probe->apGainIndex, 50);
        }
        else if (mode == VisualizationMode::LFP_GAIN_VIEW) // LFP GAIN
        {
            return Colour(66, 25 * probe->lfpGainIndex, 35 * probe->lfpGainIndex);

        }
        else if (mode == VisualizationMode::REFERENCE_VIEW)
        {
            if (referenceComboBox != nullptr)
            {
                String referenceDescription = referenceComboBox->getText();

                if (referenceDescription.contains("Ext"))
                    return Colours::pink;
                else if (referenceDescription.contains("Tip"))
                    return Colours::orange;
                else
                    return Colours::purple;
            }
            else {
                return Colours::black;
            }
            
            
        }
        /*else if (mode == ACTIVITY_VIEW) // TODO
        {
            if (channelStatus[i] == -1) // not available
            {
                return Colours::grey;
            }
            else if (channelStatus[i] < -1) // reference
            {
                return Colours::black;
            }
            else
            {
                return Colour(200 - 10 * channelReference[i], 110 - 10 * channelReference[i], 20 * channelReference[i]);
            }
        }*/
    }

    
}

void NeuropixInterface::timerCallback()
{
    Random random;
    uint64 timestamp;
    uint64 eventCode;

    int numSamples;

    if (editor->acquisitionIsActive)
        numSamples = 10;
    else
        numSamples = 0;

    //

    if (numSamples > 0)
    {
        for (int i = 0; i < electrodeMetadata.size(); i++)
        {
            if (mode == VisualizationMode::ACTIVITY_VIEW)
                electrodeMetadata.getReference(i).colour = Colour(random.nextInt(256), random.nextInt(256), 0);
        }
    }
    else {
        for (int i = 0; i < electrodeMetadata.size(); i++)
        {
            electrodeMetadata.getReference(i).colour = Colour(20, 20, 20);
        }
    }

    // NOT WORKING:
    //{
    //  ScopedLock(*thread->getMutex());
    //  int numSamples2 = inputBuffer->readAllFromBuffer(displayBuffer, &timestamp, &eventCode, 10000);
    //}
    //

    repaint();
}


void NeuropixInterface::applyProbeSettings(ProbeSettings p, bool shouldUpdateProbe)
{
    if (p.probeType != probe->type)
    {
        CoreServices::sendStatusMessage("Probe types do not match.");
        return;
    }

    // update display
    if (apGainComboBox != 0)
        apGainComboBox->setSelectedId(p.apGainIndex + 1);

    if (lfpGainComboBox != 0)
        lfpGainComboBox->setSelectedId(p.lfpGainIndex + 1);

    if (filterComboBox != 0)
    {
        if (p.apFilterState)
            filterComboBox->setSelectedId(1);
        else
            filterComboBox->setSelectedId(2);
    }

    if (referenceComboBox != 0)
        referenceComboBox->setSelectedId(p.referenceIndex + 1);

    for (int i = 0; i < electrodeMetadata.size(); i++)
    {
        if (electrodeMetadata[i].status == ElectrodeStatus::CONNECTED)
            electrodeMetadata.getReference(i).status = ElectrodeStatus::DISCONNECTED;

        if (electrodeMetadata[i].status == ElectrodeStatus::CONNECTED_OPTIONAL_REFERENCE)
            electrodeMetadata.getReference(i).status = ElectrodeStatus::OPTIONAL_REFERENCE;
    
    }

        

    // update selection state
    for (int i = 0; i < p.selectedChannel.size(); i++)
    {
        Bank bank = p.selectedBank[i];
        int channel = p.selectedChannel[i];
        int shank = p.selectedShank[i];

        for (int j = 0; j < electrodeMetadata.size(); j++)
        {
            if (electrodeMetadata[j].channel == channel &&
                electrodeMetadata[j].bank == bank &&
                electrodeMetadata[j].shank == shank)
            {
                if (electrodeMetadata[j].status == ElectrodeStatus::OPTIONAL_REFERENCE)
                    electrodeMetadata.getReference(j).status = ElectrodeStatus::CONNECTED_OPTIONAL_REFERENCE;
                else if (electrodeMetadata[j].status == ElectrodeStatus::REFERENCE)
                    ;
                else
                    electrodeMetadata.getReference(j).status = ElectrodeStatus::CONNECTED;
            }
        }
    }

 
    // apply settings in background thread
    if (shouldUpdateProbe)
         updateProbeSettingsInBackground();

    repaint();

}

ProbeSettings NeuropixInterface::getProbeSettings()
{
    ProbeSettings p;

    if (apGainComboBox != 0)
        p.apGainIndex = apGainComboBox->getSelectedId() - 1;
    else
        p.apGainIndex = -1;

    if (lfpGainComboBox != 0)
        p.lfpGainIndex = lfpGainComboBox->getSelectedId() - 1;
    else
        p.lfpGainIndex = -1;

    if (filterComboBox != 0)
        p.apFilterState = (filterComboBox->getSelectedId()) == 1;
    else
        p.apFilterState = false;
        
    if (referenceComboBox != 0)
        p.referenceIndex = referenceComboBox->getSelectedId() - 1;
    else
        p.referenceIndex = -1;

    for (auto electrode : electrodeMetadata)
    {
        if (electrode.status == ElectrodeStatus::CONNECTED || electrode.status == ElectrodeStatus::CONNECTED_OPTIONAL_REFERENCE)
        {
            p.selectedChannel.add(electrode.channel);
            p.selectedBank.add(electrode.bank);
            p.selectedShank.add(electrode.shank);
            p.selectedElectrode.add(electrode.shank_local_index);
        }
    }
    
    p.probe = probe;
    p.probeType = probe->type;

    return p;
}


void NeuropixInterface::saveParameters(XmlElement* xml)
{

    std::cout << "Saving Neuropix display." << std::endl;

    XmlElement* xmlNode = xml->createNewChildElement("NP_PROBE");

    xmlNode->setAttribute("slot", probe->basestation->slot);
    xmlNode->setAttribute("bs_firmware_version", probe->basestation->info.boot_version);
    xmlNode->setAttribute("bs_hardware_version", probe->basestation->info.version);
    xmlNode->setAttribute("bs_serial_number", String(probe->basestation->info.serial_number));
    xmlNode->setAttribute("bs_part_number", probe->basestation->info.part_number);

    xmlNode->setAttribute("bsc_firmware_version", probe->basestation->basestationConnectBoard->info.boot_version);
    xmlNode->setAttribute("bsc_hardware_version", probe->basestation->basestationConnectBoard->info.version);
    xmlNode->setAttribute("bsc_serial_number", String(probe->basestation->basestationConnectBoard->info.serial_number));
    xmlNode->setAttribute("bsc_part_number", probe->basestation->basestationConnectBoard->info.part_number);

    xmlNode->setAttribute("headstage_serial_number", String(probe->headstage->info.serial_number));
    xmlNode->setAttribute("headstage_part_number", probe->headstage->info.part_number);

    xmlNode->setAttribute("flex_version", probe->flex->info.version);
    xmlNode->setAttribute("flex_part_number", probe->headstage->info.part_number);

    xmlNode->setAttribute("port", probe->headstage->port);
    xmlNode->setAttribute("dock", probe->dock);
    xmlNode->setAttribute("probe_serial_number", String(probe->info.serial_number));
    xmlNode->setAttribute("probe_part_number", probe->info.part_number);
    xmlNode->setAttribute("probe_name", probe->name);

    xmlNode->setAttribute("ZoomHeight", zoomHeight);
    xmlNode->setAttribute("ZoomOffset", zoomOffset);

    if (apGainComboBox != nullptr)
    {
        xmlNode->setAttribute("apGainValue", apGainComboBox->getText());
        xmlNode->setAttribute("apGainIndex", apGainComboBox->getSelectedId()-1);
    }
    
    if (lfpGainComboBox != nullptr)
    {
        xmlNode->setAttribute("lfpGainValue", lfpGainComboBox->getText());
        xmlNode->setAttribute("lfpGainIndex", lfpGainComboBox->getSelectedId()-1);
    }
    
    if (referenceComboBox != nullptr)
    {
        xmlNode->setAttribute("referenceChannel", referenceComboBox->getText());
        xmlNode->setAttribute("referenceChannelIndex", referenceComboBox->getSelectedId()-1);
    }
    
    if (filterComboBox != nullptr)
    {
        xmlNode->setAttribute("filterCut", filterComboBox->getText());
        xmlNode->setAttribute("filterCutIndex", filterComboBox->getSelectedId());
    }

    XmlElement* channelNode = xmlNode->createNewChildElement("CHANNELS");

    ProbeSettings p = getProbeSettings();

    for (int i = 0; i < p.selectedChannel.size(); i++)
    {
        int bank = p.selectedBank[i];
        int shank = p.selectedShank[i];
        int channel = p.selectedChannel[i];

        String chString = String(bank);

        if (probe->type == ProbeType::NP2_4)
            chString += ":" + String(shank);

        channelNode->setAttribute("CH" + String(channel), chString);
    }
    
    xmlNode->setAttribute("visualizationMode", mode);

    // annotations
    for (int i = 0; i < annotations.size(); i++)
    {
        Annotation& a = annotations.getReference(i);
        XmlElement* annotationNode = xmlNode->createNewChildElement("ANNOTATIONS");
        annotationNode->setAttribute("text", a.text);
        annotationNode->setAttribute("channel", a.electrodes[0]);
        annotationNode->setAttribute("R", a.colour.getRed());
        annotationNode->setAttribute("G", a.colour.getGreen());
        annotationNode->setAttribute("B", a.colour.getBlue());
    }
}

void NeuropixInterface::loadParameters(XmlElement* xml)
{
    String mySerialNumber = String(probe->info.serial_number);

    // first, set defaults
    ProbeSettings settings;
    settings.probe = probe;
    settings.probeType = probe->type;
    settings.apFilterState = probe->apFilterState;
    settings.lfpGainIndex = probe->lfpGainIndex;
    settings.apGainIndex = probe->apGainIndex;

    for (int i = 0; i < probe->channel_count; i++)
    {
        settings.selectedBank.add(Bank::A);
        settings.selectedChannel.add(i);
        settings.selectedShank.add(0);
    }

    XmlElement* matchingNode = nullptr;

    // find by serial number
    forEachXmlChildElement(*xml, xmlNode)
    {
        if (xmlNode->hasTagName("NP_PROBE"))
        {
            if (xmlNode->getStringAttribute("probe_serial_number").equalsIgnoreCase(mySerialNumber))
            {

                matchingNode = xmlNode;
                break;

            }
        }
    }

    // if not, search for matching port
    if (matchingNode == nullptr)
    {
        forEachXmlChildElement(*xml, xmlNode)
        {
            if (xmlNode->hasTagName("NP_PROBE"))
            {
                if (xmlNode->getIntAttribute("slot") == probe->basestation->slot &&
                    xmlNode->getIntAttribute("port") == probe->headstage->port &&
                    xmlNode->getIntAttribute("dock") == probe->dock)
                {

                    String PN = xmlNode->getStringAttribute("probe_part_number");
                    ProbeType type;

                    if (PN.equalsIgnoreCase("NP1010"))
                        type = ProbeType::NHP10;

                    else if (PN.equalsIgnoreCase("NP1020") || PN.equalsIgnoreCase("NP1021"))
                        type = ProbeType::NHP25;

                    else if (PN.equalsIgnoreCase("NP1030") || PN.equalsIgnoreCase("NP1031"))
                        type = ProbeType::NHP45;

                    else if (PN.equalsIgnoreCase("NP1200") || PN.equalsIgnoreCase("NP1210"))
                        type = ProbeType::NHP1;

                    else if (PN.equalsIgnoreCase("PRB2_1_2_0640_0") || PN.equalsIgnoreCase("NP2000"))
                        type = ProbeType::NP2_1;

                    else if (PN.equalsIgnoreCase("PRB2_4_2_0640_0") || PN.equalsIgnoreCase("NP2010"))
                        type = ProbeType::NP2_4;

                    else if (PN.equalsIgnoreCase("PRB_1_4_0480_1") || PN.equalsIgnoreCase("PRB_1_4_0480_1_C"))
                        type = ProbeType::NP1;

                    else if (PN.equalsIgnoreCase("NP1100"))
                        type = ProbeType::UHD1;

                    else if (PN.equalsIgnoreCase("NP1110"))
                        type = ProbeType::UHD2;

                    if (type == probe->type)
                    {
                        matchingNode = xmlNode;
                        break;
                    }

                }
            }
        }
    }

    if (matchingNode != nullptr)
    {

        if (matchingNode->getChildByName("CHANNELS"))
        {
            settings.selectedBank.clear();
            settings.selectedChannel.clear();
            settings.selectedShank.clear();

                XmlElement* status = matchingNode->getChildByName("CHANNELS");

            for (int i = 0; i < probe->channel_count; i++)
            {
                settings.selectedChannel.add(i);

                String bankInfo = status->getStringAttribute("CH" + String(i));
                Bank bank = static_cast<Bank> (bankInfo.substring(0, 1).getIntValue());
                int shank = 0;

                if (probe->type == ProbeType::NP2_4)
                {
                    shank = bankInfo.substring(2, 3).getIntValue();
                }

                settings.selectedBank.add(bank);
                settings.selectedShank.add(shank);

            }
   
        }

        zoomHeight = matchingNode->getIntAttribute("ZoomHeight");
        zoomOffset = matchingNode->getIntAttribute("ZoomOffset");

        settings.apGainIndex = matchingNode->getIntAttribute("apGainIndex", -1);
        settings.lfpGainIndex = matchingNode->getIntAttribute("lfpGainIndex", -1);
        settings.referenceIndex = matchingNode->getIntAttribute("referenceChannelIndex", 0);
        settings.apFilterState = matchingNode->getIntAttribute("filterCutIndex", 1) == 1;

        forEachXmlChildElement(*matchingNode, annotationNode)
        {
            if (annotationNode->hasTagName("ANNOTATIONS"))
            {
                Array<int> annotationChannels;
                annotationChannels.add(annotationNode->getIntAttribute("electrode"));
                annotations.add(Annotation(annotationNode->getStringAttribute("text"),
                    annotationChannels,
                    Colour(annotationNode->getIntAttribute("R"),
                        annotationNode->getIntAttribute("G"),
                        annotationNode->getIntAttribute("B"))));
            }
        }

        
    }

    thread->updateProbeSettingsQueue(settings);

    applyProbeSettings(settings, false);
}

// --------------------------------------

Annotation::Annotation(String t, Array<int> e, Colour c)
{
    text = t;
    electrodes = e;

    currentYLoc = -100.f;

    isMouseOver = false;
    isSelected = false;

    colour = c;
}

Annotation::~Annotation()
{

}

// ---------------------------------------

ColorSelector::ColorSelector(NeuropixInterface* np)
{
    npi = np;
    Path p;
    p.addRoundedRectangle(0, 0, 15, 15, 3);

    for (int i = 0; i < 6; i++)
    {
        standardColors.add(Colour(245, 245, 245 - 40 * i));
        hoverColors.add(Colour(215, 215, 215 - 40 * i));
    }


    for (int i = 0; i < 6; i++)
    {
        buttons.add(new ShapeButton(String(i), standardColors[i], hoverColors[i], hoverColors[i]));
        buttons[i]->setShape(p, true, true, false);
        buttons[i]->setBounds(18 * i, 0, 15, 15);
        buttons[i]->addListener(this);
        addAndMakeVisible(buttons[i]);

        strings.add("Annotation " + String(i + 1));
    }

    npi->setAnnotationLabel(strings[0], standardColors[0]);

    activeButton = 0;

}

ColorSelector::~ColorSelector()
{


}

void ColorSelector::buttonClicked(Button* b)
{
    activeButton = buttons.indexOf((ShapeButton*)b);

    npi->setAnnotationLabel(strings[activeButton], standardColors[activeButton]);
}

void ColorSelector::updateCurrentString(String s)
{
    strings.set(activeButton, s);
}

Colour ColorSelector::getCurrentColour()
{
    return standardColors[activeButton];
}