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

#include "NeuropixInterface.h"
#include "ProbeBrowser.h"

#include "../NeuropixThread.h"
#include "../NeuropixEditor.h"
#include "../NeuropixCanvas.h"

#include "../Basestations/Basestation_v3.h"

#include "../Formats/IMRO.h"
#include "../Formats/ProbeInterfaceJson.h"

NeuropixInterface::NeuropixInterface(DataSource* p,
                                     NeuropixThread* t, 
                                     NeuropixEditor* e,
                                     NeuropixCanvas* c,
                                     Basestation* b) : SettingsInterface(p, t, e, c),
                                     probe((Probe*) p), basestation(b), neuropix_info("INFO")
{

    ColourScheme::setColourScheme(ColourSchemeId::PLASMA);

    if (probe != nullptr)
    {

        type = SettingsInterface::PROBE_SETTINGS_INTERFACE;

        basestation = probe->basestation;

        probe->ui = this;

        // make a local copy
        electrodeMetadata = Array<ElectrodeMetadata>(probe->electrodeMetadata);

        // make a local copy
        probeMetadata = probe->probeMetadata;

        mode = VisualizationMode::ENABLE_VIEW;

        probeBrowser = new ProbeBrowser(this);
        probeBrowser->setBounds(0, 0, 800, 600);
        addAndMakeVisible(probeBrowser);

        int currentHeight = 55;

        electrodesLabel = new Label("ELECTRODES", "ELECTRODES");
        electrodesLabel->setFont(Font("Small Text", 13, Font::plain));
        electrodesLabel->setBounds(446, currentHeight - 20, 100, 20);
        electrodesLabel->setColour(Label::textColourId, Colours::grey);
        addAndMakeVisible(electrodesLabel);

        enableViewButton = new UtilityButton("VIEW", Font("Small Text", 12, Font::plain));
        enableViewButton->setRadius(3.0f);
        enableViewButton->setBounds(530, currentHeight + 2, 45, 18);
        enableViewButton->addListener(this);
        enableViewButton->setTooltip("View electrode enabled state");
        addAndMakeVisible(enableViewButton);
        
        enableButton = new UtilityButton("ENABLE", Font("Small Text", 13, Font::plain));
        enableButton->setRadius(3.0f);
        enableButton->setBounds(450, currentHeight, 65, 22);
        enableButton->addListener(this);
        enableButton->setTooltip("Enable selected electrodes");
        addAndMakeVisible(enableButton);

        currentHeight += 58;

        electrodePresetLabel = new Label("ELECTRODE PRESET", "ELECTRODE PRESET");
        electrodePresetLabel->setFont(Font("Small Text", 13, Font::plain));
        electrodePresetLabel->setBounds(446, currentHeight - 20, 150, 20);
        electrodePresetLabel->setColour(Label::textColourId, Colours::grey);
        addAndMakeVisible(electrodePresetLabel);

        electrodeConfigurationComboBox = new ComboBox("electrodeConfigurationComboBox");
        electrodeConfigurationComboBox->setBounds(450, currentHeight, 135, 22);
        electrodeConfigurationComboBox->addListener(this);
        electrodeConfigurationComboBox->setTooltip("Enable a pre-configured set of electrodes");

        electrodeConfigurationComboBox->addItem("Select a preset...", 1);
        electrodeConfigurationComboBox->setItemEnabled(1, false);
        electrodeConfigurationComboBox->addSeparator();

        LOGC("FOUND ", probe->settings.availableElectrodeConfigurations.size(), " electrode configurations");   
        
        for (int i = 0; i < probe->settings.availableElectrodeConfigurations.size(); i++)
        {
            electrodeConfigurationComboBox->addItem(probe->settings.availableElectrodeConfigurations[i], i + 2);
        }

        electrodeConfigurationComboBox->setSelectedId(1, dontSendNotification);

        addAndMakeVisible(electrodeConfigurationComboBox);

        currentHeight += 55;

        if (probe->settings.availableApGains.size() > 0)
        {

            apGainComboBox = new ComboBox("apGainComboBox");
            apGainComboBox->setBounds(450, currentHeight, 65, 22);
            apGainComboBox->addListener(this);

            for (int i = 0; i < probe->settings.availableApGains.size(); i++)
                apGainComboBox->addItem(String(probe->settings.availableApGains[i]) + "x", i + 1);

            apGainComboBox->setSelectedId(probe->settings.apGainIndex + 1, dontSendNotification);
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

            currentHeight += 55;

        }

        if (probe->settings.availableLfpGains.size() > 0)
        {


            lfpGainComboBox = new ComboBox("lfpGainComboBox");
            lfpGainComboBox->setBounds(450, currentHeight, 65, 22);
            lfpGainComboBox->addListener(this);

            for (int i = 0; i < probe->settings.availableLfpGains.size(); i++)
                lfpGainComboBox->addItem(String(probe->settings.availableLfpGains[i]) + "x", i + 1);

            lfpGainComboBox->setSelectedId(probe->settings.lfpGainIndex + 1, dontSendNotification);
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

            currentHeight += 55;
        }

        if (probe->settings.availableReferences.size() > 0)
        {


            referenceComboBox = new ComboBox("ReferenceComboBox");
            referenceComboBox->setBounds(450, currentHeight, 65, 22);
            referenceComboBox->addListener(this);

            for (int i = 0; i < probe->settings.availableReferences.size(); i++)
            {
                referenceComboBox->addItem(probe->settings.availableReferences[i], i + 1);
            }

            referenceComboBox->setSelectedId(probe->settings.referenceIndex + 1, dontSendNotification);
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

            currentHeight += 55;
        }

        if (probe->hasApFilterSwitch())
        {


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

        currentHeight += 55;

        activityViewButton = new UtilityButton("VIEW", Font("Small Text", 12, Font::plain));
        activityViewButton->setRadius(3.0f);

        activityViewButton->addListener(this);
        activityViewButton->setTooltip("View peak-to-peak amplitudes for each channel");
        addAndMakeVisible(activityViewButton);

        activityViewComboBox = new ComboBox("ActivityView Combo Box");

        if (probe->settings.availableLfpGains.size() > 0)
        {
            activityViewComboBox->setBounds(450, currentHeight, 65, 22);
            activityViewComboBox->addListener(this);
            activityViewComboBox->addItem("AP", 1);
            activityViewComboBox->addItem("LFP", 2);
            activityViewComboBox->setSelectedId(1, dontSendNotification);
            addAndMakeVisible(activityViewComboBox);
            activityViewButton->setBounds(530, currentHeight + 2, 45, 18);
        }
        else {
            activityViewButton->setBounds(450, currentHeight + 2, 45, 18);
        }

        activityViewLabel = new Label("PROBE SIGNAL", "PROBE SIGNAL");
        activityViewLabel->setFont(Font("Small Text", 13, Font::plain));
        activityViewLabel->setBounds(446, currentHeight - 20, 180, 20);
        activityViewLabel->setColour(Label::textColourId, Colours::grey);
        addAndMakeVisible(activityViewLabel);

        currentHeight += 55;

        if (probe->info.part_number == "NP1300") // Neuropixels Opto
        {
            redEmissionSiteLabel = new Label("RED EMISSION SITE", "RED EMISSION SITE");
            redEmissionSiteLabel->setFont(Font("Small Text", 13, Font::plain));
            redEmissionSiteLabel->setBounds(446, currentHeight - 20, 180, 20);
            redEmissionSiteLabel->setColour(Label::textColourId, Colours::red);
            addAndMakeVisible(redEmissionSiteLabel);

            redEmissionSiteComboBox = new ComboBox("Red Emission Site Combo Box");
            redEmissionSiteComboBox->addListener(this);
            redEmissionSiteComboBox->setBounds(450, currentHeight, 65, 22);
            redEmissionSiteComboBox->addItem("OFF", 1);

            for (int i = 0; i < 14; i++)
                redEmissionSiteComboBox->addItem(String(i + 1), i + 2);

            redEmissionSiteComboBox->setSelectedId(1, dontSendNotification);
            addAndMakeVisible(redEmissionSiteComboBox);

            currentHeight += 55;

            blueEmissionSiteLabel = new Label("BLUE EMISSION SITE", "BLUE EMISSION SITE");
            blueEmissionSiteLabel->setFont(Font("Small Text", 13, Font::plain));
            blueEmissionSiteLabel->setBounds(446, currentHeight - 20, 180, 20);
            blueEmissionSiteLabel->setColour(Label::textColourId, Colours::blue);
            addAndMakeVisible(blueEmissionSiteLabel);

            blueEmissionSiteComboBox = new ComboBox("Blue Emission Site Combo Box");
            blueEmissionSiteComboBox->addListener(this);
            blueEmissionSiteComboBox->setBounds(450, currentHeight, 65, 22);
            blueEmissionSiteComboBox->addItem("OFF", 1);

            for (int i = 0; i < 14; i++)
                blueEmissionSiteComboBox->addItem(String(i + 1), i + 2);

            blueEmissionSiteComboBox->setSelectedId(1, dontSendNotification);
            addAndMakeVisible(blueEmissionSiteComboBox);
        }

        // BIST
        bistComboBox = new ComboBox("BistComboBox");
        bistComboBox->setBounds(650, 500, 225, 22);
        bistComboBox->addListener(this);

        bistComboBox->addItem("Select a test...", 1);
        bistComboBox->setItemEnabled(1, false);
        bistComboBox->addSeparator();

        availableBists.add(BIST::EMPTY);
        availableBists.add(BIST::SIGNAL);
        bistComboBox->addItem("Test probe signal", 2);

        availableBists.add(BIST::NOISE);
        bistComboBox->addItem("Test probe noise", 3);

        availableBists.add(BIST::PSB);
        bistComboBox->addItem("Test PSB bus", 4);

        availableBists.add(BIST::SR);
        bistComboBox->addItem("Test shift registers", 5);

        availableBists.add(BIST::EEPROM);
        bistComboBox->addItem("Test EEPROM", 6);

        availableBists.add(BIST::I2C);
        bistComboBox->addItem("Test I2C", 7);

        availableBists.add(BIST::SERDES);
        bistComboBox->addItem("Test Serdes", 8);

        availableBists.add(BIST::HB);
        bistComboBox->addItem("Test Heartbeat", 9);

        availableBists.add(BIST::BS);
        bistComboBox->addItem("Test Basestation", 10);

        bistComboBox->setSelectedId(1, dontSendNotification);
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
        saveImroButton->setTooltip("Save settings map to .imro file");
        addAndMakeVisible(saveImroButton);

        loadImroButton = new UtilityButton("LOAD FROM IMRO", Font("Small Text", 12, Font::plain));
        loadImroButton->setRadius(3.0f);
        loadImroButton->setBounds(175, 672, 130, 22);
        loadImroButton->addListener(this);
        loadImroButton->setTooltip("Load settings map from .imro file");
        addAndMakeVisible(loadImroButton);

        saveJsonButton = new UtilityButton("SAVE TO JSON", Font("Small Text", 12, Font::plain));
        saveJsonButton->setRadius(3.0f);
        saveJsonButton->setBounds(45, 707, 120, 22);
        saveJsonButton->addListener(this);
        saveJsonButton->setTooltip("Save channel map to probeinterface .json file");
        addAndMakeVisible(saveJsonButton);

        loadJsonButton = new UtilityButton("LOAD FROM JSON", Font("Small Text", 12, Font::plain));
        loadJsonButton->setRadius(3.0f);
        loadJsonButton->setBounds(175, 707, 130, 22);
        loadJsonButton->addListener(this);
        loadJsonButton->setTooltip("Load channel map from probeinterface .json file");
        // addAndMakeVisible(loadJsonButton);

        loadImroComboBox = new ComboBox("Quick-load IMRO");
        loadImroComboBox->setBounds(175, 707, 130, 22);
        loadImroComboBox->addListener(this);
        loadImroComboBox->setTooltip("Load a favorite IMRO setting.");

        File baseDirectory = File::getSpecialLocation(File::currentExecutableFile).getParentDirectory();
        File imroDirectory = baseDirectory.getChildFile("IMRO");

        if (File(imroDirectory).findChildFiles(File::findFiles, false, "*.imro").size())
            loadImroComboBox->addItem("Select a preset...", 1);
        else
            loadImroComboBox->addItem("No pre-set IMRO found", 1);

        loadImroComboBox->addSeparator();

        for (const auto& filename : File(imroDirectory).findChildFiles(File::findFiles, false, "*.imro")) {
            imroFiles.add(filename.getFileNameWithoutExtension());
            imroLoadedFromFolder.add(true);
            loadImroComboBox->addItem(imroFiles.getLast(), imroFiles.size() + 1);
        }
        loadImroComboBox->setSelectedId(1, dontSendNotification);
        addAndMakeVisible(loadImroComboBox);
   
        probeSettingsLabel = new Label("Settings", "Probe settings:");
        probeSettingsLabel->setFont(Font("Small Text", 13, Font::plain));
        probeSettingsLabel->setBounds(40, 610, 300, 20);
        probeSettingsLabel->setColour(Label::textColourId, Colours::grey);
        addAndMakeVisible(probeSettingsLabel);
    }
    else {
       type = SettingsInterface::BASESTATION_SETTINGS_INTERFACE;
    }

    int verticalOffset = 550;

    if (probe == nullptr)
        verticalOffset = 250;

    // FIRMWARE
    firmwareToggleButton = new UtilityButton("UPDATE FIRMWARE...", Font("Small Text", 12, Font::plain));
    firmwareToggleButton->setRadius(3.0f);
    firmwareToggleButton->addListener(this);
    firmwareToggleButton->setBounds(650, verticalOffset, 150, 22);
    firmwareToggleButton->setClickingTogglesState(true);

    if (thread->type == PXI)
        addAndMakeVisible(firmwareToggleButton);

    bscFirmwareComboBox = new ComboBox("bscFirmwareComboBox");
    bscFirmwareComboBox->setBounds(550, verticalOffset + 70, 375, 22);
    bscFirmwareComboBox->addListener(this);
    bscFirmwareComboBox->addItem("Select file...", 1);

    if (thread->type == PXI)
        addChildComponent(bscFirmwareComboBox);

    bscFirmwareButton = new UtilityButton("UPLOAD", Font("Small Text", 12, Font::plain));
    bscFirmwareButton->setRadius(3.0f);
    bscFirmwareButton->setBounds(930, verticalOffset + 70, 60, 22);
    bscFirmwareButton->addListener(this);
    bscFirmwareButton->setTooltip("Upload firmware to selected basestation connect board");

    if (thread->type == PXI)
        addChildComponent(bscFirmwareButton);

    bscFirmwareLabel = new Label("BSC FIRMWARE", "1. Update basestation connect board firmware (QBSC_FPGA_B189.bin):");
    bscFirmwareLabel->setFont(Font("Small Text", 13, Font::plain));
    bscFirmwareLabel->setBounds(550, verticalOffset + 43, 500, 20);
    bscFirmwareLabel->setColour(Label::textColourId, Colours::orange);
    
    if (thread->type == PXI)
        addChildComponent(bscFirmwareLabel);

    bsFirmwareComboBox = new ComboBox("bscFirmwareComboBox");
    bsFirmwareComboBox->setBounds(550, verticalOffset + 140, 375, 22);
    bsFirmwareComboBox->addListener(this);
    bsFirmwareComboBox->addItem("Select file...", 1);

    if (thread->type == PXI)
        addChildComponent(bsFirmwareComboBox);

    bsFirmwareButton = new UtilityButton("UPLOAD", Font("Small Text", 12, Font::plain));
    bsFirmwareButton->setRadius(3.0f);
    bsFirmwareButton->setBounds(930, verticalOffset + 140, 60, 22);
    bsFirmwareButton->addListener(this);
    bsFirmwareButton->setTooltip("Upload firmware to selected basestation");

    if (thread->type == PXI)
        addChildComponent(bsFirmwareButton);

    bsFirmwareLabel = new Label("BS FIRMWARE", "2. Update basestation firmware (BS_FPGA_B169.bin):");
    bsFirmwareLabel->setFont(Font("Small Text", 13, Font::plain));
    bsFirmwareLabel->setBounds(550, verticalOffset + 113, 500, 20);
    bsFirmwareLabel->setColour(Label::textColourId, Colours::orange);

    if (thread->type == PXI)
        addChildComponent(bsFirmwareLabel);

    firmwareInstructionsLabel = new Label("FIRMWARE INSTRUCTIONS", "3. Power cycle computer and PXI chassis");
    firmwareInstructionsLabel->setFont(Font("Small Text", 13, Font::plain));
    firmwareInstructionsLabel->setBounds(550, verticalOffset + 183, 500, 20);
    firmwareInstructionsLabel->setColour(Label::textColourId, Colours::orange);

    if (thread->type == PXI)
        addChildComponent(firmwareInstructionsLabel);

    // PROBE INFO 
    mainLabel = new Label("MAIN", "MAIN");
    mainLabel->setFont(Font("Small Text", 40, Font::plain));
    mainLabel->setBounds(625, 20, 300, 45);
    mainLabel->setColour(Label::textColourId, Colours::darkkhaki);
    addAndMakeVisible(mainLabel);

    nameLabel = new Label("MAIN", "NAME");
    nameLabel->setFont(Font("Small Text", 20, Font::plain));
    nameLabel->setBounds(625, 70, 500, 45);
    nameLabel->setColour(Label::textColourId, Colours::pink);
    addAndMakeVisible(nameLabel);

    infoLabelView = new Viewport("INFO");
    if (probe != nullptr)
    {
        infoLabelView->setBounds(625, 128, 750, 400);
    }
    else {
        infoLabelView->setBounds(625, 80, 750, 150);
    }
    
    addAndMakeVisible(infoLabelView);

    infoLabel = new Label("INFO", "INFO");
    infoLabelView->setViewedComponent(infoLabel, false);
    infoLabel->setFont(Font("Fira Code", "Retina", 15));
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

    if (probe == nullptr)
        mainString += "Slot ";

    mainString += String(basestation->slot);

    if (probe != nullptr)
    {
        
        mainString += ":";
        mainString += String(probe->headstage->port);

        if (probe->type == ProbeType::NP2_1 || probe->type == ProbeType::NP2_4)
        {
            mainString += ":";
            mainString += String(probe->dock);
        }

        nameString = probe->name + "\n(" + probe->displayName + ")";
    }

    infoString += "API version: ";
    infoString += thread->getApiVersion();
    infoString += "\n";
    infoString += "\n";

    infoString += "Basestation";
    infoString += "\n Firmware version: " + basestation->info.boot_version;
    infoString += "\n";
    infoString += "\n";

    if (basestation->type != BasestationType::ONEBOX)
    {
        infoString += "Basestation connect board";
        infoString += "\n Hardware version: " + basestation->basestationConnectBoard->info.version;
        infoString += "\n Firmware version: " + basestation->basestationConnectBoard->info.boot_version;
        infoString += "\n";
        infoString += "\n";
    }
    
    if (probe != nullptr)
    {
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

    }
    
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

    probe->updateSettings(settings);

    int ch0index = settings.selectedChannel.indexOf(0);

    thread->updateProbeSettingsQueue(settings);

    LOGC("NeuropixInterface requesting thread start");

    editor->uiLoader->startThread();
}

void NeuropixInterface::comboBoxChanged(ComboBox* comboBox)
{

    if (!editor->acquisitionIsActive)
    {
        if (comboBox == electrodeConfigurationComboBox)
        {

            String preset = electrodeConfigurationComboBox->getText();
            
			if (probe->type == ProbeType::UHD2) // switchable probe
                updateProbeSettingsInBackground();
            else
            {
                Array<int> selection = probe->selectElectrodeConfiguration(preset);

                selectElectrodes(selection);
            }
        }
        else if ((comboBox == apGainComboBox) || (comboBox == lfpGainComboBox))
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
        } else if (comboBox == activityViewComboBox)
        {
            if (comboBox->getSelectedId() == 1)
            {
                probeBrowser->activityToView = ActivityToView::APVIEW;
                ColourScheme::setColourScheme(ColourSchemeId::PLASMA);
                probeBrowser->maxPeakToPeakAmplitude = 250.0f;
            }
            else {
                probeBrowser->activityToView = ActivityToView::LFPVIEW;
                ColourScheme::setColourScheme(ColourSchemeId::VIRIDIS);
                probeBrowser->maxPeakToPeakAmplitude = 500.0f;
            }

        }
        else if (comboBox == redEmissionSiteComboBox)
        {

            setEmissionSite("red", comboBox->getSelectedId() - 1);

        }
        else if (comboBox == blueEmissionSiteComboBox)
        {
            setEmissionSite("blue", comboBox->getSelectedId() - 1);
        }
        else if (comboBox == loadImroComboBox)
        {
            if (!imroFiles.size())
            {
                return;
            }

            int fileIndex = comboBox->getSelectedId() - 2;

            if (imroFiles[fileIndex].length())
            {
                ProbeSettings settings = getProbeSettings();

                settings.clearElectrodeSelection();

                bool success = IMRO::readSettingsFromImro(File(imroFiles[fileIndex]), settings);

                if (success)
                {
                    applyProbeSettings(settings);
                    loadImroComboBox->setSelectedId(comboBox->getSelectedId(), false);
                }
                else
                {
                    loadImroComboBox->setSelectedId(1, false);
                }
            }

        }

        repaint();
    }
    else {

        if (comboBox == activityViewComboBox)
        {
            if (comboBox->getSelectedId() == 1)
            {
                probeBrowser->activityToView = ActivityToView::APVIEW;
                ColourScheme::setColourScheme(ColourSchemeId::PLASMA);
                probeBrowser->maxPeakToPeakAmplitude = 250.0f;
            }
            else {
                probeBrowser->activityToView = ActivityToView::LFPVIEW;
                ColourScheme::setColourScheme(ColourSchemeId::VIRIDIS);
                probeBrowser->maxPeakToPeakAmplitude = 500.0f;
            }

            repaint();
        }
        else if (comboBox == redEmissionSiteComboBox)
        {
            LOGD("Select red emission site.");
            setEmissionSite("red", comboBox->getSelectedId() - 1);

        }
        else if (comboBox == blueEmissionSiteComboBox)
        {
            LOGD("Select blue emission site.");
            setEmissionSite("blue", comboBox->getSelectedId() - 1);
        }
        else {
            CoreServices::sendStatusMessage("Cannot update parameters while acquisition is active");// no parameter change while acquisition is active 
        }
    }

    MouseCursor::hideWaitCursor();

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
        probeBrowser->stopTimer();
        repaint();
    }
    else if (button == apGainViewButton)
    {
        mode = AP_GAIN_VIEW;
        probeBrowser->stopTimer();
        repaint();
    }
    else if (button == lfpGainViewButton)
    {
        mode = LFP_GAIN_VIEW;
        probeBrowser->stopTimer();
        repaint();
    }
    else if (button == referenceViewButton)
    {
        mode = REFERENCE_VIEW;
        probeBrowser->stopTimer();
        repaint();
    }
    else if (button == activityViewButton)
    {
        mode = ACTIVITY_VIEW;

        if (acquisitionIsActive)
            probeBrowser->startTimer(100);

        repaint();
    }
    else if (button == enableButton)
    {
        

        Array<int> selection = getSelectedElectrodes();

        if (selection.size() > 0)
        {
            electrodeConfigurationComboBox->setSelectedId(1);
            selectElectrodes(selection);
        }

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
            if (bistComboBox->getSelectedId() == 1)
            {
                CoreServices::sendStatusMessage("Please select a test to run.");
            }
            else {

                //Save current probe settings
                ProbeSettings settings = getProbeSettings();

                //Run test
                bool passed = probe->runBist(availableBists[bistComboBox->getSelectedId() - 1]);

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

            ProbeSettings settings = getProbeSettings();

            settings.clearElectrodeSelection();

            File selectedFile = fileChooser.getResult();

            bool success = IMRO::readSettingsFromImro(selectedFile, settings);

            if (success)
            {
                if (imroFiles.size() == 0)
                {
                    loadImroComboBox->clear();
                }

                imroFiles.add(selectedFile.getFullPathName());
                imroLoadedFromFolder.add(false);
                loadImroComboBox->addItem(selectedFile.getFullPathName(), imroFiles.size());

                applyProbeSettings(settings, true);
                CoreServices::updateSignalChain(editor);
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
    else if (button == loadJsonButton)
    {
    FileChooser fileChooser("Select an probeinterface JSON file to load.", File(), "*.json");

    if (fileChooser.browseForFileToOpen())
    {
        ProbeSettings settings = getProbeSettings();

        bool success = ProbeInterfaceJson::readProbeSettingsFromJson(fileChooser.getResult(), settings);

        if (success)
        {
            applyProbeSettings(settings);
        }

    }
    }
    else if (button == saveJsonButton)
    {
    FileChooser fileChooser("Save channel map to a probeinterface JSON file.", File(), "*.json");

    if (fileChooser.browseForFileToSave(true))
    {
        bool success = ProbeInterfaceJson::writeProbeSettingsToJson(fileChooser.getResult(), getProbeSettings());

        if (!success)
            CoreServices::sendStatusMessage("Failed to write probe channel map.");
        else
            CoreServices::sendStatusMessage("Successfully wrote probe channel map.");

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
        CoreServices::updateSignalChain(editor);
       
    }
    else if (button == applyToAllButton)
    {
        canvas->applyParametersToAllProbes(getProbeSettings());
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
            basestation->updateBsFirmware(File(bsFirmwareComboBox->getText()));
        }
        else {
            CoreServices::sendStatusMessage("No file selected.");
        }
       
    }
    else if (button == bscFirmwareButton)
    {
        if (bscFirmwareComboBox->getSelectedId() > 1)
        {
            basestation->updateBscFirmware(File(bscFirmwareComboBox->getText()));
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

void NeuropixInterface::setApGain(int index)
{
    apGainComboBox->setSelectedId(index + 1, true);
}

void NeuropixInterface::setLfpGain(int index)
{
    lfpGainComboBox->setSelectedId(index + 1, true);
}

void NeuropixInterface::setReference(int index)
{
    referenceComboBox->setSelectedId(index + 1, true);
}

void NeuropixInterface::setApFilterState(bool state)
{
    filterComboBox->setSelectedId(int(!state) + 1, true);
}

void NeuropixInterface::setEmissionSite(String wavelength, int site)
{

    LOGD("Emission site selection.");

    if (probe->basestation->type == BasestationType::OPTO)
    {

        Basestation_v3* optoBs = (Basestation_v3*)probe->basestation;

        optoBs->selectEmissionSite(probe->headstage->port,
            probe->dock,
            wavelength,
            site - 1);
    }
    else {
        LOGD("Wrong basestation type: ", int(probe->basestation->type));
    }
}

void NeuropixInterface::selectElectrodes(Array<int> electrodes)
{
    // update selection state

    for (int i = 0; i <electrodes.size(); i++)
    {

        Bank bank = electrodeMetadata[electrodes[i]].bank;
        int channel = electrodeMetadata[electrodes[i]].channel;
        int shank = electrodeMetadata[electrodes[i]].shank;

        for (int j = 0; j < electrodeMetadata.size(); j++)
        {
            if (electrodeMetadata[j].channel == channel)
            {
                if (electrodeMetadata[j].bank == bank && electrodeMetadata[j].shank == shank)
                {
                    electrodeMetadata.getReference(j).status = ElectrodeStatus::CONNECTED;
                }

                else
                {
                    electrodeMetadata.getReference(j).status = ElectrodeStatus::DISCONNECTED;
                }

            }
        }
    }

    repaint();

    updateProbeSettingsInBackground();

    CoreServices::updateSignalChain(editor);
}


void NeuropixInterface::startAcquisition()
{
    
    bool enabledState = false;
    acquisitionIsActive = true;

    if (enableButton != nullptr)
        enableButton->setEnabled(enabledState);

    if (electrodeConfigurationComboBox != nullptr)
        electrodeConfigurationComboBox->setEnabled(enabledState);

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

    if (copyButton != nullptr)
        copyButton->setEnabled(enabledState);
    
    if (pasteButton != nullptr)
        pasteButton->setEnabled(enabledState);

    if (applyToAllButton != nullptr)
        applyToAllButton->setEnabled(enabledState);

    if (loadImroButton != nullptr)
        loadImroButton->setEnabled(enabledState);

    if (loadJsonButton != nullptr)
        loadJsonButton->setEnabled(enabledState);

    if (firmwareToggleButton != nullptr)
        firmwareToggleButton->setEnabled(enabledState);

    if (bscFirmwareComboBox != nullptr)
        bscFirmwareComboBox->setEnabled(enabledState);

    if (bsFirmwareComboBox != nullptr)
        bsFirmwareComboBox->setEnabled(enabledState);

    if (bsFirmwareButton != nullptr)
        bsFirmwareButton->setEnabled(enabledState);

    if (bscFirmwareButton != nullptr)
        bscFirmwareButton->setEnabled(enabledState);

    if (mode == ACTIVITY_VIEW)
        probeBrowser->startTimer(100);


}

void NeuropixInterface::stopAcquisition()
{
    bool enabledState = true;
    acquisitionIsActive = false;

    if (enableButton != nullptr)
        enableButton->setEnabled(enabledState);

    if (electrodeConfigurationComboBox != nullptr)
        electrodeConfigurationComboBox->setEnabled(enabledState);

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

    if (copyButton != nullptr)
        copyButton->setEnabled(enabledState);

    if (pasteButton != nullptr)
        pasteButton->setEnabled(enabledState);

    if (applyToAllButton != nullptr)
        applyToAllButton->setEnabled(enabledState);

    if (loadImroButton != nullptr)
        loadImroButton->setEnabled(enabledState);

    if (loadJsonButton != nullptr)
        loadJsonButton->setEnabled(enabledState);

    if (firmwareToggleButton != nullptr)
        firmwareToggleButton->setEnabled(enabledState);

    if (bscFirmwareComboBox != nullptr)
        bscFirmwareComboBox->setEnabled(enabledState);

    if (bsFirmwareComboBox != nullptr)
        bsFirmwareComboBox->setEnabled(enabledState);

    if (bsFirmwareButton != nullptr)
        bsFirmwareButton->setEnabled(enabledState);

    if (bscFirmwareButton != nullptr)
        bscFirmwareButton->setEnabled(enabledState);

    //probeBrowser->stopTimer();
}


/*void NeuropixInterface::mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel)
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
    else {
        canvas->mouseWheelMove(event, wheel);
    }

}*/


void NeuropixInterface::paint(Graphics& g)
{

    if (probe != nullptr)
    {
        drawLegend(g);

        g.setColour(Colour(60, 60, 60));
        g.fillRoundedRectangle(30, 600, 290, 145, 8.0f);
    }

}


void NeuropixInterface::drawLegend(Graphics& g)
{
    g.setColour(Colour(55, 55, 55));
    g.setFont(15);

    int xOffset = 450;
    int yOffset = 440;

    switch (mode)
    {
    case ENABLE_VIEW:
        g.drawMultiLineText("ENABLED?", xOffset, yOffset, 200);
        g.drawMultiLineText("YES", xOffset + 30, yOffset + 22, 200);
        g.drawMultiLineText("NO", xOffset + 30, yOffset + 42, 200);
        
        if (probe->type == ProbeType::NP2_1 ||
            probe->type == ProbeType::NP2_4)
        {
            g.drawMultiLineText("SELECTABLE REFERENCE", xOffset + 30, yOffset + 62, 200);
        }
        else {
            g.drawMultiLineText("REFERENCE", xOffset + 30, yOffset + 62, 200);
        }
       

        g.setColour(Colours::yellow);
        g.fillRect(xOffset + 10, yOffset + 10, 15, 15);

        g.setColour(Colours::grey);
        g.fillRect(xOffset + 10, yOffset + 30, 15, 15);

        if (probe->type == ProbeType::NP2_1 ||
            probe->type == ProbeType::NP2_4)
        {
            g.setColour(Colours::purple);
        }
        else {
            g.setColour(Colours::black);
           
        }

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

    case ACTIVITY_VIEW:
        g.drawMultiLineText("AMPLITUDE", xOffset, yOffset, 200);

        for (int i = 0; i < 6; i++)
        {
            g.drawMultiLineText(String(float(probeBrowser->maxPeakToPeakAmplitude) / 5.0f * float(i)) + " uV", xOffset + 30, yOffset + 22 + 20 * i, 200);

        }

        for (int i = 0; i < 6; i++)
        {
            g.setColour(ColourScheme::getColourForNormalizedValue(float(i) / 5.0f));
            g.fillRect(xOffset + 10, yOffset + 10 + 20 * i, 15, 15);
        }

        break;
    }
}


bool NeuropixInterface::applyProbeSettings(ProbeSettings p, bool shouldUpdateProbe)
{
    LOGC("Apply probe settings for ", p.probe->name, " shouldUpdate: ", shouldUpdateProbe);

    if (p.probeType != probe->type)
    {
        CoreServices::sendStatusMessage("Probe types do not match.");
        return false;
    }

    //if (electrodeConfigurationComboBox != 0)
    //    electrodeConfigurationComboBox->setSelectedId(p.electrodeConfigurationIndex + 2, dontSendNotification);

    // update display
    if (apGainComboBox != 0)
        apGainComboBox->setSelectedId(p.apGainIndex + 1, dontSendNotification);

    if (lfpGainComboBox != 0)
        lfpGainComboBox->setSelectedId(p.lfpGainIndex + 1, dontSendNotification);

    if (filterComboBox != 0)
    {
        if (p.apFilterState)
            filterComboBox->setSelectedId(1, dontSendNotification);
        else
            filterComboBox->setSelectedId(2, dontSendNotification);
    }

    if (referenceComboBox != 0)
        referenceComboBox->setSelectedId(p.referenceIndex + 1, dontSendNotification);

    for (int i = 0; i < electrodeMetadata.size(); i++)
    {
        if (electrodeMetadata[i].status == ElectrodeStatus::CONNECTED)
            electrodeMetadata.getReference(i).status = ElectrodeStatus::DISCONNECTED;
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
                electrodeMetadata.getReference(j).status = ElectrodeStatus::CONNECTED;
            }
        }
    }

    // apply settings in background thread
    if (shouldUpdateProbe) 
    {
        
        thread->updateProbeSettingsQueue(p);
        updateProbeSettingsInBackground();
        CoreServices::saveRecoveryConfig();
        
    }

    repaint();

    return true;

}

ProbeSettings NeuropixInterface::getProbeSettings()
{

    ProbeSettings p;

    // Get probe constants
    p.availableApGains = probe->settings.availableApGains;
    p.availableLfpGains = probe->settings.availableLfpGains;
    p.availableReferences = probe->settings.availableReferences;
    p.availableBanks = probe->settings.availableBanks;

    // Set probe variables
    if (electrodeConfigurationComboBox != nullptr)
        p.electrodeConfigurationIndex = electrodeConfigurationComboBox->getSelectedId() - 2;
    else
        p.electrodeConfigurationIndex = -1;

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
        if (electrode.status == ElectrodeStatus::CONNECTED)
        {
            p.selectedChannel.add(electrode.channel);
            p.selectedBank.add(electrode.bank);
            p.selectedShank.add(electrode.shank);
            p.selectedElectrode.add(electrode.global_index);

           // std::cout << electrode.channel << " : " << electrode.global_index << std::endl;
        }
    }
    
    p.probe = probe;
    p.probeType = probe->type;

    return p;
}


void NeuropixInterface::saveParameters(XmlElement* xml)
{


    if (probe != nullptr)
    {
        LOGD("Saving Neuropix display.");

        XmlElement* xmlNode = xml->createNewChildElement("NP_PROBE");

        xmlNode->setAttribute("slot", probe->basestation->slot);
        xmlNode->setAttribute("bs_firmware_version", probe->basestation->info.boot_version);
        xmlNode->setAttribute("bs_hardware_version", probe->basestation->info.version);
        xmlNode->setAttribute("bs_serial_number", String(probe->basestation->info.serial_number));
        xmlNode->setAttribute("bs_part_number", probe->basestation->info.part_number);

        if (thread->type == PXI)
        {
            xmlNode->setAttribute("bsc_firmware_version", probe->basestation->basestationConnectBoard->info.boot_version);
            xmlNode->setAttribute("bsc_hardware_version", probe->basestation->basestationConnectBoard->info.version);
            xmlNode->setAttribute("bsc_serial_number", String(probe->basestation->basestationConnectBoard->info.serial_number));
            xmlNode->setAttribute("bsc_part_number", probe->basestation->basestationConnectBoard->info.part_number);
        }

        xmlNode->setAttribute("headstage_serial_number", String(probe->headstage->info.serial_number));
        xmlNode->setAttribute("headstage_part_number", probe->headstage->info.part_number);

        xmlNode->setAttribute("flex_version", probe->flex->info.version);
        xmlNode->setAttribute("flex_part_number", probe->headstage->info.part_number);

        xmlNode->setAttribute("port", probe->headstage->port);
        xmlNode->setAttribute("dock", probe->dock);
        xmlNode->setAttribute("probe_serial_number", String(probe->info.serial_number));
        xmlNode->setAttribute("probe_part_number", probe->info.part_number);
        xmlNode->setAttribute("probe_name", probe->name);
        xmlNode->setAttribute("num_adcs", probe->probeMetadata.num_adcs);
        xmlNode->setAttribute("custom_probe_name", probe->customName.probeSpecific);

        xmlNode->setAttribute("ZoomHeight", probeBrowser->getZoomHeight());
        xmlNode->setAttribute("ZoomOffset", probeBrowser->getZoomOffset());

        if (apGainComboBox != nullptr)
        {
            xmlNode->setAttribute("apGainValue", apGainComboBox->getText());
            xmlNode->setAttribute("apGainIndex", apGainComboBox->getSelectedId() - 1);
        }

        if (lfpGainComboBox != nullptr)
        {
            xmlNode->setAttribute("lfpGainValue", lfpGainComboBox->getText());
            xmlNode->setAttribute("lfpGainIndex", lfpGainComboBox->getSelectedId() - 1);
        }

        if (electrodeConfigurationComboBox != nullptr)
        {
            if (electrodeConfigurationComboBox->getSelectedId() > 1)
            {
                xmlNode->setAttribute("electrodeConfigurationPreset", electrodeConfigurationComboBox->getText());
            }
            else {
                xmlNode->setAttribute("electrodeConfigurationPreset", "NONE");
            }

        }

        if (referenceComboBox != nullptr)
        {
            if (referenceComboBox->getSelectedId() > 0)
            {
                xmlNode->setAttribute("referenceChannel", referenceComboBox->getText());
                xmlNode->setAttribute("referenceChannelIndex", referenceComboBox->getSelectedId() - 1);
            }
            else {
                xmlNode->setAttribute("referenceChannel", "Ext");
                xmlNode->setAttribute("referenceChannelIndex", 0);
            }
        }

        if (filterComboBox != nullptr)
        {
            xmlNode->setAttribute("filterCut", filterComboBox->getText());
            xmlNode->setAttribute("filterCutIndex", filterComboBox->getSelectedId());
        }

        XmlElement* channelNode = xmlNode->createNewChildElement("CHANNELS");
        XmlElement* xposNode = xmlNode->createNewChildElement("ELECTRODE_XPOS");
        XmlElement* yposNode = xmlNode->createNewChildElement("ELECTRODE_YPOS");

        ProbeSettings p = getProbeSettings();

        for (int i = 0; i < p.selectedChannel.size(); i++)
        {
            int bank = int(p.selectedBank[i]);
            int shank = p.selectedShank[i];
            int channel = p.selectedChannel[i];
            int elec = p.selectedElectrode[i];

            String chString = String(bank);

            if (probe->type == ProbeType::NP2_4)
                chString += ":" + String(shank);

            channelNode->setAttribute("CH" + String(channel), chString);
            xposNode->setAttribute("CH" + String(channel), String(probe->electrodeMetadata[elec].xpos + 250 * shank));
            yposNode->setAttribute("CH" + String(channel), String(probe->electrodeMetadata[elec].ypos));
        }

        if (probe->emissionSiteMetadata.size() > 0)
        {
            XmlElement* emissionSiteNode = xmlNode->createNewChildElement("EMISSION_SITES");

            for (int i = 0; i < probe->emissionSiteMetadata.size(); i++)
            {

                XmlElement* emissionSite = emissionSiteNode->createNewChildElement("SITE");

                const EmissionSiteMetadata& metadata = probe->emissionSiteMetadata[i];

                emissionSite->setAttribute("WAVELENGTH", metadata.wavelength_nm);
                emissionSite->setAttribute("SHANK_INDEX", metadata.shank_index);
                emissionSite->setAttribute("XPOS", metadata.xpos);
                emissionSite->setAttribute("YPOS", metadata.ypos);
            }
        }

        if (imroFiles.size() > 0)
        {
            XmlElement* imroFilesNode = xmlNode->createNewChildElement("IMRO_FILES");

            for (int i = 0; i < imroFiles.size(); i++)
            {
                if (!imroLoadedFromFolder[i])
                {
                    XmlElement* imroFileNode = imroFilesNode->createNewChildElement("FILE");
                    imroFileNode->setAttribute("PATH", imroFiles[i]);
                }

            }
        }

        xmlNode->setAttribute("visualizationMode", mode);
        xmlNode->setAttribute("activityToView", probeBrowser->activityToView);

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
    
}

void NeuropixInterface::loadParameters(XmlElement* xml)
{

    if (probe != nullptr)
    {
        String mySerialNumber = String(probe->info.serial_number);

        // first, set defaults
        ProbeSettings settings; // = ProbeSettings(probe->settings);
        settings.probe = probe;
        settings.probeType = probe->type;
        settings.apFilterState = probe->settings.apFilterState;
        settings.lfpGainIndex = probe->settings.lfpGainIndex;
        settings.apGainIndex = probe->settings.apGainIndex;
        settings.referenceIndex = probe->settings.referenceIndex;
        if (settings.referenceIndex >= referenceComboBox->getNumItems())
			settings.referenceIndex = 0;
        settings.availableApGains = probe->settings.availableApGains;
        settings.availableLfpGains = probe->settings.availableLfpGains;
        settings.availableBanks = probe->settings.availableBanks;
        settings.availableReferences = probe->settings.availableReferences;

        for (int i = 0; i < probe->channel_count; i++)
        {
            settings.selectedBank.add(Bank::A);
            settings.selectedChannel.add(probe->electrodeMetadata[i].channel);
            settings.selectedShank.add(0);
            settings.selectedElectrode.add(probe->electrodeMetadata[i].global_index);
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
                        ProbeType type = ProbeType::NP1;

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
                settings.selectedElectrode.clear();

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

                    for (int j = 0; j < electrodeMetadata.size(); j++)
                    {
                        if (electrodeMetadata[j].channel == i)
                        {
                            if (electrodeMetadata[j].bank == bank && electrodeMetadata[j].shank == shank)
                            {
                                settings.selectedElectrode.add(j);
                            }
                        }
                    }

                }

            }

            probeBrowser->setZoomHeightAndOffset(matchingNode->getIntAttribute("ZoomHeight"),
                matchingNode->getIntAttribute("ZoomOffset"));

            String customName = thread->getCustomProbeName(matchingNode->getStringAttribute("probe_serial_number"));

            if (customName.length() > 0)
            {
                probe->customName.probeSpecific = customName;
            }

            settings.apGainIndex = matchingNode->getIntAttribute("apGainIndex", 3);
            settings.lfpGainIndex = matchingNode->getIntAttribute("lfpGainIndex", 2);
            settings.referenceIndex = matchingNode->getIntAttribute("referenceChannelIndex", 0);
            if (settings.referenceIndex >= referenceComboBox->getNumItems())
                settings.referenceIndex = 0;

            String configurationName = matchingNode->getStringAttribute("electrodeConfigurationPreset", "NONE");

            //std::cout << "configurationName: " << configurationName << std::endl;
 			settings.electrodeConfigurationIndex = settings.availableElectrodeConfigurations.indexOf(configurationName);
           //std::cout << "electrodeConfigurationIndex: " << settings.electrodeConfigurationIndex << std::endl;

            for (int i = 0; i < electrodeConfigurationComboBox->getNumItems(); i++)
            {
                if (electrodeConfigurationComboBox->getItemText(i).equalsIgnoreCase(configurationName))
                {
                    electrodeConfigurationComboBox->setSelectedItemIndex(i, dontSendNotification);
                    break;
                }
                    
            }
            
            settings.apFilterState = matchingNode->getIntAttribute("filterCutIndex", 1) == 1;

            forEachXmlChildElement(*matchingNode, imroNode)
            {
                if (imroNode->hasTagName("IMRO_FILES"))
                {

                    forEachXmlChildElement(*imroNode, fileNode)
                    {

                        if (imroFiles.size() == 0)
                            loadImroComboBox->clear();

                        imroFiles.add(fileNode->getStringAttribute("PATH"));
                        imroLoadedFromFolder.add(false);
                        loadImroComboBox->addItem(imroFiles.getLast(), imroFiles.size());
                    }
                }
            }

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

        probe->updateSettings(settings);

        applyProbeSettings(settings, false);
    }

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