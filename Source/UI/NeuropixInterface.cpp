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

#include "NeuropixInterface.h"
#include "ProbeBrowser.h"

#include "../NeuropixCanvas.h"
#include "../NeuropixEditor.h"
#include "../NeuropixThread.h"

#include "../Basestations/PxiBasestation.h"

#include "../Formats/IMRO.h"
#include "../Formats/ProbeInterfaceJson.h"

#include "../Basestations/PxiBasestation.h"

NeuropixInterface::NeuropixInterface (DataSource* p,
                                      NeuropixThread* t,
                                      NeuropixEditor* e,
                                      NeuropixCanvas* c,
                                      Basestation* b) : SettingsInterface (p, t, e, c),
                                                        probe ((Probe*) p),
                                                        basestation (b),
                                                        neuropix_info ("INFO")
{
    ColourScheme::setColourScheme (ColourSchemeId::PLASMA);

    if (probe != nullptr)
    {
        type = SettingsInterface::PROBE_SETTINGS_INTERFACE;

        basestation = probe->basestation;

        probe->ui = this;

        // make a local copy
        electrodeMetadata = Array<ElectrodeMetadata> (probe->electrodeMetadata);

        // make a local copy
        probeMetadata = probe->probeMetadata;

        mode = VisualizationMode::ENABLE_VIEW;

        probeBrowser = std::make_unique<ProbeBrowser> (this);
        probeBrowser->setBounds (0, 5, 450, 550);
        addAndMakeVisible (probeBrowser.get());

        int currentHeight = 55;

        probeEnableButton = std::make_unique<UtilityButton> ("ENABLED");
        probeEnableButton->setRadius (3.0f);
        probeEnableButton->setBounds (680, currentHeight + 25, 100, 22);
        probeEnableButton->setClickingTogglesState (true);
        probeEnableButton->setToggleState (probe->settings.isEnabled, dontSendNotification);
        probeEnableButton->setTooltip ("If disabled, probe will not stream data during acquisition");
        probeEnableButton->addListener (this);
        addAndMakeVisible (probeEnableButton.get());

        electrodesLabel = std::make_unique<Label> ("ELECTRODES", "ELECTRODES");
        electrodesLabel->setFont (FontOptions ("Inter", "Regular", 13.0f));
        electrodesLabel->setBounds (496, currentHeight - 20, 100, 20);
        addAndMakeVisible (electrodesLabel.get());

        enableViewButton = std::make_unique<UtilityButton> ("VIEW");
        enableViewButton->setRadius (3.0f);
        enableViewButton->setBounds (580, currentHeight + 2, 45, 18);
        enableViewButton->addListener (this);
        enableViewButton->setTooltip ("View electrode enabled state");
        addAndMakeVisible (enableViewButton.get());

        enableButton = std::make_unique<UtilityButton> ("ENABLE");
        enableButton->setRadius (3.0f);
        enableButton->setBounds (500, currentHeight, 65, 22);
        enableButton->addListener (this);
        enableButton->setTooltip ("Enable selected electrodes");
        addAndMakeVisible (enableButton.get());

        currentHeight += 58;

        electrodePresetLabel = std::make_unique<Label> ("ELECTRODE PRESET", "ELECTRODE PRESET");
        electrodePresetLabel->setFont (FontOptions ("Inter", "Regular", 13.0f));
        electrodePresetLabel->setBounds (496, currentHeight - 20, 150, 20);
        addAndMakeVisible (electrodePresetLabel.get());

        electrodeConfigurationComboBox = std::make_unique<ComboBox> ("electrodeConfigurationComboBox");
        electrodeConfigurationComboBox->setBounds (500, currentHeight, 135, 22);
        electrodeConfigurationComboBox->addListener (this);
        electrodeConfigurationComboBox->setTooltip ("Enable a pre-configured set of electrodes");

        electrodeConfigurationComboBox->addItem ("Select a preset...", 1);
        electrodeConfigurationComboBox->setItemEnabled (1, false);
        electrodeConfigurationComboBox->addSeparator();

        for (int i = 0; i < probe->settings.availableElectrodeConfigurations.size(); i++)
        {
            electrodeConfigurationComboBox->addItem (probe->settings.availableElectrodeConfigurations[i], i + 2);
        }

        electrodeConfigurationComboBox->setSelectedId (1, dontSendNotification);

        addAndMakeVisible (electrodeConfigurationComboBox.get());

        currentHeight += 55;

        if (probe->settings.availableApGains.size() > 0)
        {
            apGainComboBox = std::make_unique<ComboBox> ("apGainComboBox");
            apGainComboBox->setBounds (500, currentHeight, 65, 22);
            apGainComboBox->addListener (this);

            for (int i = 0; i < probe->settings.availableApGains.size(); i++)
                apGainComboBox->addItem (String (probe->settings.availableApGains[i]) + "x", i + 1);

            apGainComboBox->setSelectedId (probe->settings.apGainIndex + 1, dontSendNotification);
            addAndMakeVisible (apGainComboBox.get());

            apGainViewButton = std::make_unique<UtilityButton> ("VIEW");
            apGainViewButton->setRadius (3.0f);
            apGainViewButton->setBounds (580, currentHeight + 2, 45, 18);
            apGainViewButton->addListener (this);
            apGainViewButton->setTooltip ("View AP gain of each channel");
            addAndMakeVisible (apGainViewButton.get());

            apGainLabel = std::make_unique<Label> ("AP GAIN", "AP GAIN");
            apGainLabel->setFont (FontOptions ("Inter", "Regular", 13.0f));
            apGainLabel->setBounds (496, currentHeight - 20, 100, 20);
            addAndMakeVisible (apGainLabel.get());

            currentHeight += 55;
        }

        if (probe->settings.availableLfpGains.size() > 0)
        {
            lfpGainComboBox = std::make_unique<ComboBox> ("lfpGainComboBox");
            lfpGainComboBox->setBounds (500, currentHeight, 65, 22);
            lfpGainComboBox->addListener (this);

            for (int i = 0; i < probe->settings.availableLfpGains.size(); i++)
                lfpGainComboBox->addItem (String (probe->settings.availableLfpGains[i]) + "x", i + 1);

            lfpGainComboBox->setSelectedId (probe->settings.lfpGainIndex + 1, dontSendNotification);
            addAndMakeVisible (lfpGainComboBox.get());

            lfpGainViewButton = std::make_unique<UtilityButton> ("VIEW");
            lfpGainViewButton->setRadius (3.0f);
            lfpGainViewButton->setBounds (580, currentHeight + 2, 45, 18);
            lfpGainViewButton->addListener (this);
            lfpGainViewButton->setTooltip ("View LFP gain of each channel");
            addAndMakeVisible (lfpGainViewButton.get());

            lfpGainLabel = std::make_unique<Label> ("LFP GAIN", "LFP GAIN");
            lfpGainLabel->setFont (FontOptions ("Inter", "Regular", 13.0f));
            lfpGainLabel->setBounds (496, currentHeight - 20, 100, 20);
            addAndMakeVisible (lfpGainLabel.get());

            currentHeight += 55;
        }

        if (probe->settings.availableReferences.size() > 0)
        {
            referenceComboBox = std::make_unique<ComboBox> ("ReferenceComboBox");
            referenceComboBox->setBounds (500, currentHeight, 65, 22);
            referenceComboBox->addListener (this);

            for (int i = 0; i < probe->settings.availableReferences.size(); i++)
            {
                referenceComboBox->addItem (probe->settings.availableReferences[i], i + 1);
            }

            referenceComboBox->setSelectedId (probe->settings.referenceIndex + 1, dontSendNotification);
            addAndMakeVisible (referenceComboBox.get());

            referenceViewButton = std::make_unique<UtilityButton> ("VIEW");
            referenceViewButton->setRadius (3.0f);
            referenceViewButton->setBounds (580, currentHeight + 2, 45, 18);
            referenceViewButton->addListener (this);
            referenceViewButton->setTooltip ("View reference of each channel");
            addAndMakeVisible (referenceViewButton.get());

            referenceLabel = std::make_unique<Label> ("REFERENCE", "REFERENCE");
            referenceLabel->setFont (FontOptions ("Inter", "Regular", 13.0f));
            referenceLabel->setBounds (496, currentHeight - 20, 100, 20);
            addAndMakeVisible (referenceLabel.get());

            currentHeight += 55;
        }

        if (probe->hasApFilterSwitch())
        {
            filterComboBox = std::make_unique<ComboBox> ("FilterComboBox");
            filterComboBox->setBounds (500, currentHeight, 75, 22);
            filterComboBox->addListener (this);
            filterComboBox->addItem ("ON", 1);
            filterComboBox->addItem ("OFF", 2);
            filterComboBox->setSelectedId (1, dontSendNotification);
            addAndMakeVisible (filterComboBox.get());

            filterLabel = std::make_unique<Label> ("FILTER", "AP FILTER CUT");
            filterLabel->setFont (FontOptions ("Inter", "Regular", 13.0f));
            filterLabel->setBounds (496, currentHeight - 20, 200, 20);
            addAndMakeVisible (filterLabel.get());
        }

        currentHeight += 55;

        activityViewButton = std::make_unique<UtilityButton> ("VIEW");
        activityViewButton->setRadius (3.0f);

        activityViewButton->addListener (this);
        activityViewButton->setTooltip ("View peak-to-peak amplitudes for each channel");
        addAndMakeVisible (activityViewButton.get());

        activityViewComboBox = std::make_unique<ComboBox> ("ActivityView Combo Box");

        if (probe->settings.availableLfpGains.size() > 0)
        {
            activityViewComboBox->setBounds (500, currentHeight, 65, 22);
            activityViewComboBox->addListener (this);
            activityViewComboBox->addItem ("AP", 1);
            activityViewComboBox->addItem ("LFP", 2);
            activityViewComboBox->setSelectedId (1, dontSendNotification);
            addAndMakeVisible (activityViewComboBox.get());
            activityViewButton->setBounds (580, currentHeight + 2, 45, 18);
        }
        else
        {
            activityViewButton->setBounds (500, currentHeight + 2, 45, 18);
        }

        activityViewLabel = std::make_unique<Label> ("PROBE SIGNAL", "PROBE SIGNAL");
        activityViewLabel->setFont (FontOptions ("Inter", "Regular", 13.0f));
        activityViewLabel->setBounds (496, currentHeight - 20, 180, 20);
        addAndMakeVisible (activityViewLabel.get());

        activityViewFilterButton = std::make_unique<UtilityButton> ("BP FILTER");
        activityViewFilterButton->addListener (this);
        activityViewFilterButton->setTooltip ("View bandpass filtered signal");
        activityViewFilterButton->setClickingTogglesState (true);
        activityViewFilterButton->setToggleState (true, dontSendNotification);
        activityViewFilterButton->setBounds (500, currentHeight + 24, 70, 18);
        addAndMakeVisible (activityViewFilterButton.get());

        activityViewCARButton = std::make_unique<UtilityButton> ("CAR");
        activityViewCARButton->addListener (this);
        activityViewCARButton->setTooltip ("View common average referenced signal");
        activityViewCARButton->setClickingTogglesState (true);
        activityViewCARButton->setToggleState (true, dontSendNotification);
        activityViewCARButton->setBounds (500, currentHeight + 44, 70, 18);
        addAndMakeVisible (activityViewCARButton.get());

        currentHeight += 105;

        if (probe->info.part_number == "NP1300") // Neuropixels Opto
        {
            redEmissionSiteLabel = std::make_unique<Label> ("RED EMISSION SITE", "RED EMISSION SITE");
            redEmissionSiteLabel->setFont (FontOptions ("Inter", "Regular", 13.0f));
            redEmissionSiteLabel->setBounds (496, currentHeight - 20, 180, 20);
            addAndMakeVisible (redEmissionSiteLabel.get());

            redEmissionSiteComboBox = std::make_unique<ComboBox> ("Red Emission Site Combo Box");
            redEmissionSiteComboBox->addListener (this);
            redEmissionSiteComboBox->setBounds (500, currentHeight, 65, 22);
            redEmissionSiteComboBox->addItem ("OFF", 1);

            for (int i = 0; i < 14; i++)
                redEmissionSiteComboBox->addItem (String (i + 1), i + 2);

            redEmissionSiteComboBox->setSelectedId (1, dontSendNotification);
            addAndMakeVisible (redEmissionSiteComboBox.get());

            currentHeight += 55;

            blueEmissionSiteLabel = std::make_unique<Label> ("BLUE EMISSION SITE", "BLUE EMISSION SITE");
            blueEmissionSiteLabel->setFont (FontOptions ("Inter", "Regular", 13.0f));
            blueEmissionSiteLabel->setBounds (496, currentHeight - 20, 180, 20);
            addAndMakeVisible (blueEmissionSiteLabel.get());

            blueEmissionSiteComboBox = std::make_unique<ComboBox> ("Blue Emission Site Combo Box");
            blueEmissionSiteComboBox->addListener (this);
            blueEmissionSiteComboBox->setBounds (500, currentHeight, 65, 22);
            blueEmissionSiteComboBox->addItem ("OFF", 1);

            for (int i = 0; i < 14; i++)
                blueEmissionSiteComboBox->addItem (String (i + 1), i + 2);

            blueEmissionSiteComboBox->setSelectedId (1, dontSendNotification);
            addAndMakeVisible (blueEmissionSiteComboBox.get());
        }

        // BIST
        bistComboBox = std::make_unique<ComboBox> ("BistComboBox");
        bistComboBox->setBounds (700, 500, 225, 22);
        bistComboBox->addListener (this);

        bistComboBox->addItem ("Select a test...", 1);
        bistComboBox->setItemEnabled (1, false);
        bistComboBox->addSeparator();

        availableBists.add (BIST::EMPTY);
        availableBists.add (BIST::SIGNAL);
        bistComboBox->addItem ("Test probe signal", 2);

        availableBists.add (BIST::NOISE);
        bistComboBox->addItem ("Test probe noise", 3);

        availableBists.add (BIST::PSB);
        bistComboBox->addItem ("Test PSB bus", 4);

        availableBists.add (BIST::SR);
        bistComboBox->addItem ("Test shift registers", 5);

        availableBists.add (BIST::EEPROM);
        bistComboBox->addItem ("Test EEPROM", 6);

        availableBists.add (BIST::I2C);
        bistComboBox->addItem ("Test I2C", 7);

        availableBists.add (BIST::SERDES);
        bistComboBox->addItem ("Test Serdes", 8);

        availableBists.add (BIST::HB);
        bistComboBox->addItem ("Test Heartbeat", 9);

        availableBists.add (BIST::BS);
        bistComboBox->addItem ("Test Basestation", 10);

        bistComboBox->setSelectedId (1, dontSendNotification);
        addAndMakeVisible (bistComboBox.get());

        bistButton = std::make_unique<UtilityButton> ("RUN");
        bistButton->setRadius (3.0f);
        bistButton->setBounds (930, 500, 50, 22);
        bistButton->addListener (this);
        bistButton->setTooltip ("Run selected test");
        addAndMakeVisible (bistButton.get());

        bistLabel = std::make_unique<Label> ("BIST", "Built-in self tests:");
        bistLabel->setFont (FontOptions ("Inter", "Regular", 15.0f));
        bistLabel->setBounds (700, 473, 200, 20);
        addAndMakeVisible (bistLabel.get());

        // COPY / PASTE / UPLOAD
        copyButton = std::make_unique<UtilityButton> ("COPY");
        copyButton->setRadius (3.0f);
        copyButton->setBounds (45, 637, 60, 22);
        copyButton->addListener (this);
        copyButton->setTooltip ("Copy probe settings");
        addAndMakeVisible (copyButton.get());

        pasteButton = std::make_unique<UtilityButton> ("PASTE");
        pasteButton->setRadius (3.0f);
        pasteButton->setBounds (115, 637, 60, 22);
        pasteButton->addListener (this);
        pasteButton->setTooltip ("Paste probe settings");
        addAndMakeVisible (pasteButton.get());

        applyToAllButton = std::make_unique<UtilityButton> ("APPLY TO ALL");
        applyToAllButton->setRadius (3.0f);
        applyToAllButton->setBounds (185, 637, 120, 22);
        applyToAllButton->addListener (this);
        applyToAllButton->setTooltip ("Apply this probe's settings to all others");
        addAndMakeVisible (applyToAllButton.get());

        saveImroButton = std::make_unique<UtilityButton> ("SAVE TO IMRO");
        saveImroButton->setRadius (3.0f);
        saveImroButton->setBounds (45, 672, 120, 22);
        saveImroButton->addListener (this);
        saveImroButton->setTooltip ("Save settings map to .imro file");
        if (probe->type == ProbeType::UHD1 || probe->type == ProbeType::UHD2)
        {
            addChildComponent (saveImroButton.get());
        }
        else
        {
            addAndMakeVisible (saveImroButton.get());
        }

        loadImroButton = std::make_unique<UtilityButton> ("LOAD FROM IMRO");
        loadImroButton->setRadius (3.0f);
        loadImroButton->setBounds (175, 672, 130, 22);
        loadImroButton->addListener (this);
        loadImroButton->setTooltip ("Load settings map from .imro file");
        if (probe->type == ProbeType::UHD1 || probe->type == ProbeType::UHD2)
        {
            addChildComponent (loadImroButton.get());
        }
        else
        {
            addAndMakeVisible (loadImroButton.get());
        }

        saveJsonButton = std::make_unique<UtilityButton> ("SAVE TO JSON");
        saveJsonButton->setRadius (3.0f);
        saveJsonButton->setBounds (45, 707, 120, 22);
        saveJsonButton->addListener (this);
        saveJsonButton->setTooltip ("Save channel map to probeinterface .json file");
        addAndMakeVisible (saveJsonButton.get());

        loadJsonButton = std::make_unique<UtilityButton> ("LOAD FROM JSON");
        loadJsonButton->setRadius (3.0f);
        loadJsonButton->setBounds (175, 707, 130, 22);
        loadJsonButton->addListener (this);
        loadJsonButton->setTooltip ("Load channel map from probeinterface .json file");
        // addAndMakeVisible(loadJsonButton);

        loadImroComboBox = std::make_unique<ComboBox> ("Quick-load IMRO");
        loadImroComboBox->setBounds (175, 707, 130, 22);
        loadImroComboBox->addListener (this);
        loadImroComboBox->setTooltip ("Load settings from a stored IMRO file.");

        File baseDirectory = File::getSpecialLocation (File::currentExecutableFile).getParentDirectory();
        File imroDirectory = baseDirectory.getChildFile ("IMRO");

        loadImroComboBox->addItem ("Quick-load IMRO...", 1);
        loadImroComboBox->setItemEnabled (1, false);
        loadImroComboBox->addSeparator();

        for (const auto& filename : File (imroDirectory).findChildFiles (File::findFiles, false, "*.imro"))
        {
            imroFiles.add (filename.getFileNameWithoutExtension());
            imroLoadedFromFolder.add (true);
            loadImroComboBox->addItem (File (imroFiles.getLast()).getFileName(),
                                       imroFiles.size() + 1);
        }
        loadImroComboBox->setSelectedId (1, dontSendNotification);

        if (probe->type == ProbeType::UHD1 || probe->type == ProbeType::UHD2)
        {
            addChildComponent (loadImroComboBox.get());
        }
        else
        {
            addAndMakeVisible (loadImroComboBox.get());
        }

        probeSettingsLabel = std::make_unique<Label> ("Settings", "Probe settings:");
        probeSettingsLabel->setFont (FontOptions ("Inter", "Regular", 13.0f));
        probeSettingsLabel->setBounds (40, 610, 300, 20);
        addAndMakeVisible (probeSettingsLabel.get());
    }
    else
    {
        type = SettingsInterface::BASESTATION_SETTINGS_INTERFACE;
    }

    int verticalOffset = 550;

    if (probe == nullptr)
        verticalOffset = 250;

    // FIRMWARE
    firmwareToggleButton = std::make_unique<UtilityButton> ("UPDATE FIRMWARE...");
    firmwareToggleButton->setRadius (3.0f);
    firmwareToggleButton->addListener (this);
    firmwareToggleButton->setBounds (700, verticalOffset, 160, 24);
    firmwareToggleButton->setClickingTogglesState (true);
    firmwareToggleButton->setEnabled (true);

    if (thread->type == PXI)
        addAndMakeVisible (firmwareToggleButton.get());

    bscFirmwareComboBox = std::make_unique<ComboBox> ("bscFirmwareComboBox");
    bscFirmwareComboBox->setBounds (610, verticalOffset + 70, 375, 22);
    bscFirmwareComboBox->addListener (this);
    bscFirmwareComboBox->addItem ("Select file...", 1);

    if (thread->type == PXI)
        addChildComponent (bscFirmwareComboBox.get());

    bscFirmwareButton = std::make_unique<UtilityButton> ("UPLOAD");
    bscFirmwareButton->setRadius (3.0f);
    bscFirmwareButton->setBounds (990, verticalOffset + 70, 60, 22);
    bscFirmwareButton->addListener (this);
    bscFirmwareButton->setTooltip ("Upload firmware to selected basestation connect board");

    if (thread->type == PXI)
        addChildComponent (bscFirmwareButton.get());

    if (basestation->type == BasestationType::OPTO)
        bscFirmwareLabel = std::make_unique<Label> ("BSC FIRMWARE", "1. Update basestation connect board firmware (" + String (OPTO_BSC_FIRMWARE_FILENAME) + ") : ");
    else
        bscFirmwareLabel = std::make_unique<Label> ("BSC FIRMWARE", "1. Update basestation connect board firmware (" + String (BSC_FIRMWARE_FILENAME) + ") : ");
    bscFirmwareLabel->setFont (FontOptions ("Inter", "Medium", 15.0f));
    bscFirmwareLabel->setBounds (610, verticalOffset + 43, 500, 20);

    if (thread->type == PXI)
        addChildComponent (bscFirmwareLabel.get());

    bsFirmwareComboBox = std::make_unique<ComboBox> ("bscFirmwareComboBox");
    bsFirmwareComboBox->setBounds (610, verticalOffset + 140, 375, 22);
    bsFirmwareComboBox->addListener (this);
    bsFirmwareComboBox->addItem ("Select file...", 1);

    if (thread->type == PXI)
        addChildComponent (bsFirmwareComboBox.get());

    bsFirmwareButton = std::make_unique<UtilityButton> ("UPLOAD");
    bsFirmwareButton->setRadius (3.0f);
    bsFirmwareButton->setBounds (990, verticalOffset + 140, 60, 22);
    bsFirmwareButton->addListener (this);
    bsFirmwareButton->setTooltip ("Upload firmware to selected basestation");

    if (thread->type == PXI)
        addChildComponent (bsFirmwareButton.get());

    if (basestation->type == BasestationType::OPTO)
        bsFirmwareLabel = std::make_unique<Label> ("BS FIRMWARE", "2. Update basestation firmware (" + String (OPTO_BS_FIRMWARE_FILENAME) + "): ");
    else
        bsFirmwareLabel = std::make_unique<Label> ("BS FIRMWARE", "2. Update basestation firmware (" + String (BS_FIRMWARE_FILENAME) + "): ");
    bsFirmwareLabel->setFont (FontOptions ("Inter", "Medium", 15.0f));
    bsFirmwareLabel->setBounds (610, verticalOffset + 113, 500, 20);

    if (thread->type == PXI)
        addChildComponent (bsFirmwareLabel.get());

    firmwareInstructionsLabel = std::make_unique<Label> ("FIRMWARE INSTRUCTIONS", "3. Power cycle computer and PXI chassis");
    firmwareInstructionsLabel->setFont (FontOptions ("Inter", "Medium", 15.0f));
    firmwareInstructionsLabel->setBounds (610, verticalOffset + 183, 500, 20);

    if (thread->type == PXI)
        addChildComponent (firmwareInstructionsLabel.get());

    // PROBE INFO
    nameLabel = std::make_unique<Label> ("MAIN", "NAME");
    nameLabel->setFont (FontOptions ("Fira Code", "Medium", 30.0f));
    nameLabel->setBounds (675, 40, 500, 45);
    addAndMakeVisible (nameLabel.get());

    infoLabelView = std::make_unique<Viewport> ("INFO");
    infoLabelView->setBounds (675, 110, 750, 400);

    addAndMakeVisible (infoLabelView.get());
    infoLabelView->toBack();

    infoLabel = std::make_unique<Label> ("INFO", "INFO");
    infoLabelView->setViewedComponent (infoLabel.get(), false);
    infoLabel->setFont (FontOptions (15.0f));
    infoLabel->setBounds (0, 0, 750, 350);
    infoLabel->setJustificationType (Justification::topLeft);

    // ANNOTATIONS
    annotationButton = std::make_unique<UtilityButton> ("ADD");
    annotationButton->setRadius (3.0f);
    annotationButton->setBounds (500, 680, 40, 18);
    annotationButton->addListener (this);
    annotationButton->setTooltip ("Add annotation to selected channels");
    //addAndMakeVisible(annotationButton);

    annotationLabel = std::make_unique<Label> ("ANNOTATION", "Custom annotation");
    annotationLabel->setBounds (496, 620, 200, 20);
    annotationLabel->setEditable (true);
    annotationLabel->addListener (this);
    // addAndMakeVisible(annotationLabel);

    annotationLabelLabel = std::make_unique<Label> ("ANNOTATION_LABEL", "ANNOTATION");
    annotationLabelLabel->setFont (FontOptions ("Inter", "Regular", 13.0f));
    annotationLabelLabel->setBounds (496, 600, 200, 20);
    // addAndMakeVisible(annotationLabelLabel);

    annotationColourSelector = std::make_unique<AnnotationColourSelector> (this);
    annotationColourSelector->setBounds (500, 650, 250, 20);
    // addAndMakeVisible(annotationColourSelector);

    updateInfoString();
}

NeuropixInterface::~NeuropixInterface()
{
}

void NeuropixInterface::updateInfoString()
{
    String nameString, infoString;

    if (probe == nullptr)
    {
        nameString += "Slot ";
        nameString += String (basestation->slot);
    }
    else
    {
        nameString = probe->displayName;

        infoString += "Probe Type: " + String (probeTypeToString (probe->type));
        infoString += "\nPart Number: " + probe->info.part_number;
        infoString += "\nS/N: " + String (probe->info.serial_number);
        infoString += "\n";

        infoString += "\nSlot: " + String (basestation->slot);
        infoString += "\nPort: " + String (probe->headstage->port);

        if (probe->type == ProbeType::NP2_1 || probe->type == ProbeType::NP2_4)
        {
            infoString += "\nDock: " + String (probe->dock);
        }

        infoString += "\n";
        infoString += "\n";
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
        infoString += "\n Serial number: " + String (basestation->basestationConnectBoard->info.serial_number);
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
    }

    infoLabel->setText (infoString, dontSendNotification);
    nameLabel->setText (nameString, dontSendNotification);
}

void NeuropixInterface::labelTextChanged (Label* label)
{
    if (label == annotationLabel.get())
    {
        annotationColourSelector->updateCurrentString (label->getText());
    }
}

void NeuropixInterface::updateProbeSettingsInBackground()
{
    ProbeSettings settings = getProbeSettings();

    probe->updateSettings (settings);

    LOGD ("NeuropixInterface requesting thread start");

    editor->uiLoader->waitForThreadToExit (5000);
    thread->updateProbeSettingsQueue (settings);
    editor->uiLoader->startThread();
}

void NeuropixInterface::comboBoxChanged (ComboBox* comboBox)
{
    if (! editor->acquisitionIsActive)
    {
        if (comboBox == electrodeConfigurationComboBox.get())
        {
            String preset = electrodeConfigurationComboBox->getText();

            Array<int> selection = probe->selectElectrodeConfiguration (preset);

            selectElectrodes (selection);
        }
        else if ((comboBox == apGainComboBox.get()) || (comboBox == lfpGainComboBox.get()))
        {
            updateProbeSettingsInBackground();
        }
        else if (comboBox == referenceComboBox.get())
        {
            updateProbeSettingsInBackground();
        }
        else if (comboBox == filterComboBox.get())
        {
            updateProbeSettingsInBackground();
        }
        else if (comboBox == bscFirmwareComboBox.get())
        {
            if (comboBox->getSelectedId() == 1)
            {
                String dialogBoxTitle;
                String filePatternsAllowed;

                if (basestation->type == BasestationType::OPTO)
                {
                    dialogBoxTitle = "Select an OPTO_QBSC .bin file to load.";
                    filePatternsAllowed = OPTO_BSC_FIRMWARE_FILENAME;
                }
                else
                {
                    dialogBoxTitle = "Select a QBSC .bin file to load.";
                    filePatternsAllowed = BSC_FIRMWARE_FILENAME;
                }

                FileChooser fileChooser (dialogBoxTitle, File(), filePatternsAllowed);

                if (fileChooser.browseForFileToOpen())
                {
                    comboBox->addItem (fileChooser.getResult().getFullPathName(), comboBox->getNumItems() + 1);
                    comboBox->setSelectedId (comboBox->getNumItems());
                }
                else
                {
                    comboBox->setSelectedId (0);
                }
            }
        }
        else if (comboBox == bsFirmwareComboBox.get())
        {
            if (comboBox->getSelectedId() == 1)
            {
                String dialogBoxTitle;
                String filePatternsAllowed;

                if (basestation->type == BasestationType::OPTO)
                {
                    dialogBoxTitle = "Select a BS .bin file to load.";
                    filePatternsAllowed = OPTO_BS_FIRMWARE_FILENAME;
                }
                else
                {
                    dialogBoxTitle = "Select a BS .bin file to load.";
                    filePatternsAllowed = BS_FIRMWARE_FILENAME;
                }

                FileChooser fileChooser (dialogBoxTitle, File(), filePatternsAllowed);

                if (fileChooser.browseForFileToOpen())
                {
                    comboBox->addItem (fileChooser.getResult().getFullPathName(), comboBox->getNumItems() + 1);
                    comboBox->setSelectedId (comboBox->getNumItems());
                }
                else
                {
                    comboBox->setSelectedId (0);
                }
            }
        }
        else if (comboBox == filterComboBox.get())
        {
            updateProbeSettingsInBackground();
        }
        else if (comboBox == activityViewComboBox.get())
        {
            if (comboBox->getSelectedId() == 1)
            {
                probeBrowser->activityToView = ActivityToView::APVIEW;
                ColourScheme::setColourScheme (ColourSchemeId::PLASMA);
                probeBrowser->maxPeakToPeakAmplitude = 250.0f;
            }
            else
            {
                probeBrowser->activityToView = ActivityToView::LFPVIEW;
                ColourScheme::setColourScheme (ColourSchemeId::VIRIDIS);
                probeBrowser->maxPeakToPeakAmplitude = 500.0f;
            }
        }
        else if (comboBox == redEmissionSiteComboBox.get())
        {
            setEmissionSite ("red", comboBox->getSelectedId() - 1);
        }
        else if (comboBox == blueEmissionSiteComboBox.get())
        {
            setEmissionSite ("blue", comboBox->getSelectedId() - 1);
        }
        else if (comboBox == loadImroComboBox.get())
        {
            if (! imroFiles.size())
            {
                return;
            }

            int fileIndex = comboBox->getSelectedId() - 2;

            if (imroFiles[fileIndex].length())
            {
                applyProbeSettingsFromImro (imroFiles[fileIndex]);
            }
        }

        repaint();
    }
    else
    {
        if (comboBox == activityViewComboBox.get())
        {
            if (comboBox->getSelectedId() == 1)
            {
                probeBrowser->activityToView = ActivityToView::APVIEW;
                ColourScheme::setColourScheme (ColourSchemeId::PLASMA);
                probeBrowser->maxPeakToPeakAmplitude = 250.0f;
            }
            else
            {
                probeBrowser->activityToView = ActivityToView::LFPVIEW;
                ColourScheme::setColourScheme (ColourSchemeId::VIRIDIS);
                probeBrowser->maxPeakToPeakAmplitude = 500.0f;
            }

            repaint();
        }
        else if (comboBox == redEmissionSiteComboBox.get())
        {
            LOGD ("Select red emission site.");
            setEmissionSite ("red", comboBox->getSelectedId() - 1);
        }
        else if (comboBox == blueEmissionSiteComboBox.get())
        {
            LOGD ("Select blue emission site.");
            setEmissionSite ("blue", comboBox->getSelectedId() - 1);
        }
        else
        {
            CoreServices::sendStatusMessage ("Cannot update parameters while acquisition is active"); // no parameter change while acquisition is active
        }
    }

    MouseCursor::hideWaitCursor();
}

void NeuropixInterface::setAnnotationLabel (String s, Colour c)
{
    annotationLabel->setText (s, NotificationType::dontSendNotification);
    annotationLabel->setColour (Label::textColourId, c);
}

void NeuropixInterface::buttonClicked (Button* button)
{
    if (button == probeEnableButton.get())
    {
        probe->isEnabled = probeEnableButton->getToggleState();

        if (probe->isEnabled)
        {
            probeEnableButton->setLabel ("ENABLED");
        }
        else
        {
            probeEnableButton->setLabel ("DISABLED");
        }

        probe->settings.isEnabled = probe->isEnabled;
        probe->setStatus (probe->isEnabled ? SourceStatus::CONNECTED : SourceStatus::DISABLED);
        thread->updateStreamInfo (true);
        CoreServices::updateSignalChain (editor);
    }
    else if (button == enableViewButton.get())
    {
        mode = ENABLE_VIEW;
        probeBrowser->stopTimer();
        repaint();
    }
    else if (button == apGainViewButton.get())
    {
        mode = AP_GAIN_VIEW;
        probeBrowser->stopTimer();
        repaint();
    }
    else if (button == lfpGainViewButton.get())
    {
        mode = LFP_GAIN_VIEW;
        probeBrowser->stopTimer();
        repaint();
    }
    else if (button == referenceViewButton.get())
    {
        mode = REFERENCE_VIEW;
        probeBrowser->stopTimer();
        repaint();
    }
    else if (button == activityViewButton.get())
    {
        mode = ACTIVITY_VIEW;

        if (acquisitionIsActive)
            probeBrowser->startTimer (100);

        repaint();
    }
    else if (button == activityViewFilterButton.get())
    {
        probe->setActivityViewFilterState (activityViewFilterButton->getToggleState());
    }
    else if (button == activityViewCARButton.get())
    {
        probe->setActivityViewCARState (activityViewCARButton->getToggleState());
    }
    else if (button == enableButton.get())
    {
        Array<int> selection = getSelectedElectrodes();

        if (selection.size() > 0)
        {
            electrodeConfigurationComboBox->setSelectedId (1);
            selectElectrodes (selection);
        }
    }
    else if (button == annotationButton.get())
    {
        String s = annotationLabel->getText();
        Array<int> a = getSelectedElectrodes();

        if (a.size() > 0)
            annotations.add (Annotation (s, a, annotationColourSelector->getCurrentColour()));

        repaint();
    }
    else if (button == bistButton.get())
    {
        if (! editor->acquisitionIsActive)
        {
            if (bistComboBox->getSelectedId() == 1)
            {
                CoreServices::sendStatusMessage ("Please select a test to run.");
            }
            else
            {
                //Save current probe settings
                ProbeSettings settings = getProbeSettings();

                //Run test
                bool passed = probe->runBist (availableBists[bistComboBox->getSelectedId() - 1]);

                String testString = bistComboBox->getText();

                //Check if testString already has test result attached
                String result = testString.substring (testString.length() - 6);
                if (result.compare ("PASSED") == 0 || result.compare ("FAILED") == 0)
                {
                    testString = testString.dropLastCharacters (9);
                }

                if (passed)
                {
                    testString += " - PASSED";
                }
                else
                {
                    testString += " - FAILED";
                }
                //bistComboBox->setText(testString);
                bistComboBox->changeItemText (bistComboBox->getSelectedId(), testString);
                bistComboBox->setText (testString);
                //bistComboBox->setSelectedId(bistComboBox->getSelectedId(), NotificationType::sendNotification);
            }
        }
        else
        {
            CoreServices::sendStatusMessage ("Cannot run test while acquisition is active.");
        }
    }
    else if (button == loadImroButton.get())
    {
        FileChooser fileChooser ("Select an IMRO file to load.", File(), "*.imro");

        if (fileChooser.browseForFileToOpen())
        {
            File selectedFile = fileChooser.getResult();

            applyProbeSettingsFromImro (selectedFile);
        }
    }
    else if (button == saveImroButton.get())
    {
        FileChooser fileChooser ("Save settings to an IMRO file.", File(), "*.imro");

        if (fileChooser.browseForFileToSave (true))
        {
            bool success = IMRO::writeSettingsToImro (fileChooser.getResult(), getProbeSettings());

            if (! success)
                CoreServices::sendStatusMessage ("Failed to write probe settings.");
            else
                CoreServices::sendStatusMessage ("Successfully wrote probe settings.");
        }
    }
    else if (button == loadJsonButton.get())
    {
        FileChooser fileChooser ("Select an probeinterface JSON file to load.", File(), "*.json");

        if (fileChooser.browseForFileToOpen())
        {
            ProbeSettings settings = getProbeSettings();

            bool success = ProbeInterfaceJson::readProbeSettingsFromJson (fileChooser.getResult(), settings);

            if (success)
            {
                applyProbeSettings (settings);
            }
        }
    }
    else if (button == saveJsonButton.get())
    {
        FileChooser fileChooser ("Save channel map to a probeinterface JSON file.", File(), "*.json");

        if (fileChooser.browseForFileToSave (true))
        {
            bool success = ProbeInterfaceJson::writeProbeSettingsToJson (fileChooser.getResult(), getProbeSettings());

            if (! success)
                CoreServices::sendStatusMessage ("Failed to write probe channel map.");
            else
                CoreServices::sendStatusMessage ("Successfully wrote probe channel map.");
        }
    }
    else if (button == copyButton.get())
    {
        canvas->storeProbeSettings (getProbeSettings());
        CoreServices::sendStatusMessage ("Probe settings copied.");
    }
    else if (button == pasteButton.get())
    {
        applyProbeSettings (canvas->getProbeSettings());
        CoreServices::updateSignalChain (editor);
    }
    else if (button == applyToAllButton.get())
    {
        canvas->applyParametersToAllProbes (getProbeSettings());
    }
    else if (button == firmwareToggleButton.get())
    {
        bool state = button->getToggleState();

        bscFirmwareButton->setVisible (state);
        bscFirmwareComboBox->setVisible (state);
        bscFirmwareLabel->setVisible (state);

        bsFirmwareButton->setVisible (state);
        bsFirmwareComboBox->setVisible (state);
        bsFirmwareLabel->setVisible (state);

        firmwareInstructionsLabel->setVisible (state);

        repaint();
    }
    else if (button == bsFirmwareButton.get())
    {
        if (bsFirmwareComboBox->getSelectedId() > 1)
        {
            basestation->updateBsFirmware (File (bsFirmwareComboBox->getText()));
        }
        else
        {
            CoreServices::sendStatusMessage ("No file selected.");
        }
    }
    else if (button == bscFirmwareButton.get())
    {
        if (bscFirmwareComboBox->getSelectedId() > 1)
        {
            basestation->updateBscFirmware (File (bscFirmwareComboBox->getText()));
        }
        else
        {
            CoreServices::sendStatusMessage ("No file selected.");
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
            electrodeIndices.add (i);
        }
    }

    return electrodeIndices;
}

void NeuropixInterface::setApGain (int index)
{
    apGainComboBox->setSelectedId (index + 1, true);
}

void NeuropixInterface::setLfpGain (int index)
{
    lfpGainComboBox->setSelectedId (index + 1, true);
}

void NeuropixInterface::setReference (int index)
{
    referenceComboBox->setSelectedId (index + 1, true);
}

void NeuropixInterface::setApFilterState (bool state)
{
    filterComboBox->setSelectedId (int (! state) + 1, true);
}

void NeuropixInterface::setEmissionSite (String wavelength, int site)
{
    LOGD ("Emission site selection.");

    if (probe->basestation->type == BasestationType::OPTO)
    {
        PxiBasestation* optoBs = (PxiBasestation*) probe->basestation;

        optoBs->selectEmissionSite (probe->headstage->port,
                                    probe->dock,
                                    wavelength,
                                    site - 1);
    }
    else
    {
        LOGD ("Wrong basestation type: ", int (probe->basestation->type));
    }
}

void NeuropixInterface::selectElectrodes (Array<int> electrodes)
{
    // update selection state

    if (probe->type == ProbeType::UHD2)
    {
        LOGD ("UHD2 SELECTING ELECTRODES");

        for (int i = 0; i < electrodeMetadata.size(); i++)
        {
            electrodeMetadata.getReference (i).status = ElectrodeStatus::DISCONNECTED;
        }

        for (int i = 0; i < electrodes.size(); i++)
        {
            electrodeMetadata.getReference (electrodes[i]).status = ElectrodeStatus::CONNECTED;
            //std::cout << "Electrode " << electrodes[i] << " selected, CH=" << electrodeMetadata.getReference(electrodes[i]).channel << std::endl;
        }

        probe->settings.selectedBank.clear();
        probe->settings.selectedChannel.clear();
        probe->settings.selectedElectrode.clear();
        probe->settings.selectedShank.clear();

        // update selection state
        for (int i = 0; i < electrodeMetadata.size(); i++)
        {
            if (electrodeMetadata[i].status == ElectrodeStatus::CONNECTED)
            {
                probe->settings.selectedBank.add (electrodeMetadata[i].bank);
                probe->settings.selectedChannel.add (electrodeMetadata[i].channel);
                probe->settings.selectedElectrode.add (electrodeMetadata[i].global_index);
                probe->settings.selectedShank.add (electrodeMetadata[i].shank);
            }
        }
    }
    else
    {
        for (int i = 0; i < electrodes.size(); i++)
        {
            Bank bank = electrodeMetadata[electrodes[i]].bank;
            int channel = electrodeMetadata[electrodes[i]].channel;
            int shank = electrodeMetadata[electrodes[i]].shank;
            int global_index = electrodeMetadata[electrodes[i]].global_index;

            for (int j = 0; j < electrodeMetadata.size(); j++)
            {
                if (probe->type == ProbeType::QUAD_BASE)
                {
                    if (electrodeMetadata[j].channel == channel && electrodeMetadata[j].shank == shank)
                    {
                        if (electrodeMetadata[j].bank == bank)
                        {
                            electrodeMetadata.getReference (j).status = ElectrodeStatus::CONNECTED;
                        }
                        else
                        {
                            electrodeMetadata.getReference (j).status = ElectrodeStatus::DISCONNECTED;
                        }
                    }
                }
                else
                {
                    if (electrodeMetadata[j].channel == channel)
                    {
                        if (electrodeMetadata[j].bank == bank && electrodeMetadata[j].shank == shank)
                        {
                            electrodeMetadata.getReference (j).status = ElectrodeStatus::CONNECTED;
                        }

                        else
                        {
                            electrodeMetadata.getReference (j).status = ElectrodeStatus::DISCONNECTED;
                        }
                    }
                }
            }
        }
    }

    repaint();

    updateProbeSettingsInBackground();

    CoreServices::updateSignalChain (editor);
}

void NeuropixInterface::startAcquisition()
{
    bool enabledState = false;
    acquisitionIsActive = true;

    if (enableButton != nullptr)
        enableButton->setEnabled (enabledState);

    if (probeEnableButton != nullptr)
        probeEnableButton->setEnabled (enabledState);

    if (electrodeConfigurationComboBox != nullptr)
        electrodeConfigurationComboBox->setEnabled (enabledState);

    if (apGainComboBox != nullptr)
        apGainComboBox->setEnabled (enabledState);

    if (lfpGainComboBox != nullptr)
        lfpGainComboBox->setEnabled (enabledState);

    if (filterComboBox != nullptr)
        filterComboBox->setEnabled (enabledState);

    if (referenceComboBox != nullptr)
        referenceComboBox->setEnabled (enabledState);

    if (bistComboBox != nullptr)
        bistComboBox->setEnabled (enabledState);

    if (bistButton != nullptr)
        bistButton->setEnabled (enabledState);

    if (copyButton != nullptr)
        copyButton->setEnabled (enabledState);

    if (pasteButton != nullptr)
        pasteButton->setEnabled (enabledState);

    if (applyToAllButton != nullptr)
        applyToAllButton->setEnabled (enabledState);

    if (loadImroButton != nullptr)
        loadImroButton->setEnabled (enabledState);

    if (loadJsonButton != nullptr)
        loadJsonButton->setEnabled (enabledState);

    if (loadImroComboBox != nullptr)
        loadImroComboBox->setEnabled (enabledState);

    if (firmwareToggleButton != nullptr)
        firmwareToggleButton->setEnabled (enabledState);

    if (bscFirmwareComboBox != nullptr)
        bscFirmwareComboBox->setEnabled (enabledState);

    if (bsFirmwareComboBox != nullptr)
        bsFirmwareComboBox->setEnabled (enabledState);

    if (bsFirmwareButton != nullptr)
        bsFirmwareButton->setEnabled (enabledState);

    if (bscFirmwareButton != nullptr)
        bscFirmwareButton->setEnabled (enabledState);

    if (mode == ACTIVITY_VIEW)
        probeBrowser->startTimer (100);
}

void NeuropixInterface::stopAcquisition()
{
    bool enabledState = true;
    acquisitionIsActive = false;

    if (enableButton != nullptr)
        enableButton->setEnabled (enabledState);

    if (probeEnableButton != nullptr)
        probeEnableButton->setEnabled (enabledState);

    if (electrodeConfigurationComboBox != nullptr)
        electrodeConfigurationComboBox->setEnabled (enabledState);

    if (apGainComboBox != nullptr)
        apGainComboBox->setEnabled (enabledState);

    if (lfpGainComboBox != nullptr)
        lfpGainComboBox->setEnabled (enabledState);

    if (filterComboBox != nullptr)
        filterComboBox->setEnabled (enabledState);

    if (referenceComboBox != nullptr)
        referenceComboBox->setEnabled (enabledState);

    if (bistComboBox != nullptr)
        bistComboBox->setEnabled (enabledState);

    if (bistButton != nullptr)
        bistButton->setEnabled (enabledState);

    if (copyButton != nullptr)
        copyButton->setEnabled (enabledState);

    if (pasteButton != nullptr)
        pasteButton->setEnabled (enabledState);

    if (applyToAllButton != nullptr)
        applyToAllButton->setEnabled (enabledState);

    if (loadImroButton != nullptr)
        loadImroButton->setEnabled (enabledState);

    if (loadJsonButton != nullptr)
        loadJsonButton->setEnabled (enabledState);

    if (loadImroComboBox != nullptr)
        loadImroComboBox->setEnabled (enabledState);

    if (firmwareToggleButton != nullptr)
        firmwareToggleButton->setEnabled (enabledState);

    if (bscFirmwareComboBox != nullptr)
        bscFirmwareComboBox->setEnabled (enabledState);

    if (bsFirmwareComboBox != nullptr)
        bsFirmwareComboBox->setEnabled (enabledState);

    if (bsFirmwareButton != nullptr)
        bsFirmwareButton->setEnabled (enabledState);

    if (bscFirmwareButton != nullptr)
        bscFirmwareButton->setEnabled (enabledState);
}

void NeuropixInterface::paint (Graphics& g)
{
    if (probe != nullptr)
    {
        if (probe->info.part_number != "NP1300") // Neuropixels Opto
            drawLegend (g);

        g.setColour (findColour (ThemeColours::componentParentBackground).withAlpha (0.5f));
        g.fillRoundedRectangle (30, 600, 290, 145, 8.0f);
    }
}

void NeuropixInterface::drawLegend (Graphics& g)
{
    if (thread->isRefreshing)
        return;
    g.setColour (findColour (ThemeColours::defaultText).withAlpha (0.75f));
    g.setFont (15);

    int xOffset = 500;
    int yOffset = 485;

    switch (mode)
    {
        case ENABLE_VIEW:
            g.drawMultiLineText ("ENABLED?", xOffset, yOffset, 200);
            g.drawMultiLineText ("YES", xOffset + 30, yOffset + 22, 200);
            g.drawMultiLineText ("NO", xOffset + 30, yOffset + 42, 200);

            if (probe->type == ProbeType::NP2_1 || probe->type == ProbeType::NP2_4)
            {
                g.drawMultiLineText ("SELECTABLE REFERENCE", xOffset + 30, yOffset + 62, 200);
            }
            else
            {
                g.drawMultiLineText ("REFERENCE", xOffset + 30, yOffset + 62, 200);
            }

            g.setColour (Colours::yellow);
            g.fillRect (xOffset + 10, yOffset + 10, 15, 15);

            g.setColour (Colour (180, 180, 180));
            g.fillRect (xOffset + 10, yOffset + 30, 15, 15);

            if (probe->type == ProbeType::NP2_1 || probe->type == ProbeType::NP2_4)
            {
                g.setColour (Colours::purple);
            }
            else
            {
                g.setColour (Colours::black);
            }

            g.fillRect (xOffset + 10, yOffset + 50, 15, 15);

            break;

        case AP_GAIN_VIEW:
            g.drawMultiLineText ("AP GAIN", xOffset, yOffset, 200);

            for (int i = 0; i < 8; i++)
            {
                g.drawMultiLineText (apGainComboBox->getItemText (i), xOffset + 30, yOffset + 22 + 20 * i, 200);
            }

            for (int i = 0; i < 8; i++)
            {
                g.setColour (Colour (25 * i, 25 * i, 50));
                g.fillRect (xOffset + 10, yOffset + 10 + 20 * i, 15, 15);
            }

            break;

        case LFP_GAIN_VIEW:
            g.drawMultiLineText ("LFP GAIN", xOffset, yOffset, 200);

            for (int i = 0; i < 8; i++)
            {
                g.drawMultiLineText (lfpGainComboBox->getItemText (i), xOffset + 30, yOffset + 22 + 20 * i, 200);
            }

            for (int i = 0; i < 8; i++)
            {
                g.setColour (Colour (66, 25 * i, 35 * i));
                g.fillRect (xOffset + 10, yOffset + 10 + 20 * i, 15, 15);
            }

            break;

        case REFERENCE_VIEW:
            g.drawMultiLineText ("REFERENCE", xOffset, yOffset, 200);

            for (int i = 0; i < referenceComboBox->getNumItems(); i++)
            {
                g.drawMultiLineText (referenceComboBox->getItemText (i), xOffset + 30, yOffset + 22 + 20 * i, 200);
            }

            for (int i = 0; i < referenceComboBox->getNumItems(); i++)
            {
                String referenceDescription = referenceComboBox->getItemText (i);

                if (referenceDescription.contains ("Ext"))
                    g.setColour (Colours::pink);
                else if (referenceDescription.contains ("Tip"))
                    g.setColour (Colours::orange);
                else
                    g.setColour (Colours::purple);

                g.fillRect (xOffset + 10, yOffset + 10 + 20 * i, 15, 15);
            }

            break;

        case ACTIVITY_VIEW:
            g.drawMultiLineText ("AMPLITUDE", xOffset, yOffset, 200);

            for (int i = 0; i < 6; i++)
            {
                g.drawMultiLineText (String (float (probeBrowser->maxPeakToPeakAmplitude) / 5.0f * float (i)) + " uV", xOffset + 30, yOffset + 22 + 20 * i, 200);
            }

            for (int i = 0; i < 6; i++)
            {
                g.setColour (ColourScheme::getColourForNormalizedValue (float (i) / 5.0f));
                g.fillRect (xOffset + 10, yOffset + 10 + 20 * i, 15, 15);
            }

            break;
    }
}

void NeuropixInterface::applyProbeSettingsFromImro (File imroFile)
{
    ProbeSettings settings = getProbeSettings();

    settings.clearElectrodeSelection();

    bool success = IMRO::readSettingsFromImro (imroFile, settings);

    if (! success)
    {
        loadImroComboBox->setSelectedId (1);
        return;
    }

    if (settings.probeType == probe->type)
    {
        if (! imroFiles.contains (imroFile.getFullPathName()))
        {
            imroFiles.add (imroFile.getFullPathName());
            imroLoadedFromFolder.add (false);
            loadImroComboBox->addItem (imroFile.getFileName(), imroFiles.size() + 1);
        }

        electrodeConfigurationComboBox->setSelectedId (1);
        applyProbeSettings (settings);
        CoreServices::updateSignalChain (editor);
    }
    else
    {
        CoreServices::sendStatusMessage ("Probe types do not match.");
        // show popup notification window
        String message = "The IMRO file you have selected is for a " + probeTypeToString (settings.probeType);
        message += " probe, but the current probe is a " + String (probeTypeToString (probe->type)) + " probe.";

        AlertWindow::showMessageBox (AlertWindow::AlertIconType::WarningIcon,
                                     "Probe types do not match",
                                     message,
                                     "OK");
    }

    loadImroComboBox->setSelectedId (1);
}

bool NeuropixInterface::applyProbeSettings (ProbeSettings p, bool shouldUpdateProbe)
{
    LOGD ("NeuropixInterface applying probe settings for ", p.probe->name, " shouldUpdate: ", shouldUpdateProbe);

    if (p.probeType != probe->type)
    {
        CoreServices::sendStatusMessage ("Probe types do not match.");
        return false;
    }

    // update display
    if (apGainComboBox != 0)
        apGainComboBox->setSelectedId (p.apGainIndex + 1, dontSendNotification);

    if (lfpGainComboBox != 0)
        lfpGainComboBox->setSelectedId (p.lfpGainIndex + 1, dontSendNotification);

    if (filterComboBox != 0)
    {
        if (p.apFilterState)
            filterComboBox->setSelectedId (1, dontSendNotification);
        else
            filterComboBox->setSelectedId (2, dontSendNotification);
    }

    if (referenceComboBox != 0)
        referenceComboBox->setSelectedId (p.referenceIndex + 1, dontSendNotification);

    for (int i = 0; i < electrodeMetadata.size(); i++)
    {
        if (electrodeMetadata[i].status == ElectrodeStatus::CONNECTED)
            electrodeMetadata.getReference (i).status = ElectrodeStatus::DISCONNECTED;
    }

    if (probe->type == ProbeType::UHD2)
    {
        Array<int> selection = probe->selectElectrodeConfiguration (electrodeConfigurationComboBox->getText());

        selectElectrodes (selection);

        probe->settings.clearElectrodeSelection();

        for (auto const electrode : electrodeMetadata)
        {
            if (electrode.status == ElectrodeStatus::CONNECTED)
            {
                probe->settings.selectedChannel.add (electrode.channel);
                probe->settings.selectedBank.add (electrode.bank);
                probe->settings.selectedShank.add (electrode.shank);
                probe->settings.selectedElectrode.add (electrode.global_index);

                // std::cout << electrode.channel << " : " << electrode.global_index << std::endl;
            }
        }
    }
    else
    {
        // update selection state
        for (int i = 0; i < p.selectedChannel.size(); i++)
        {
            Bank bank = p.selectedBank[i];
            int channel = p.selectedChannel[i];
            int shank = p.selectedShank[i];

            for (int j = 0; j < electrodeMetadata.size(); j++)
            {
                if (electrodeMetadata[j].channel == channel && electrodeMetadata[j].bank == bank && electrodeMetadata[j].shank == shank)
                {
                    electrodeMetadata.getReference (j).status = ElectrodeStatus::CONNECTED;
                }
            }
        }
    }

    probe->updateNamingScheme (basestation->getNamingScheme());
    updateInfoString();

    // apply settings in background thread
    if (shouldUpdateProbe)
    {
        //thread->updateProbeSettingsQueue (p);
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

    LOGD ("Getting probe settings");
    int numElectrodes = 0;

    for (auto const electrode : electrodeMetadata)
    {
        if (electrode.status == ElectrodeStatus::CONNECTED)
        {
            p.selectedChannel.add (electrode.channel);
            p.selectedBank.add (electrode.bank);
            p.selectedShank.add (electrode.shank);
            p.selectedElectrode.add (electrode.global_index);
            numElectrodes++;

            // std::cout << electrode.channel << " : " << electrode.global_index << std::endl;
        }
    }

    LOGD ("Found ", numElectrodes, " connected electrodes.");

    p.probe = probe;
    p.probeType = probe->type;

    return p;
}

void NeuropixInterface::saveParameters (XmlElement* xml)
{
    if (probe != nullptr)
    {
        LOGD ("Saving Neuropix display.");

        int numElectrodeGroups = probe->type == ProbeType::QUAD_BASE ? 4 : 1;

        for (int electrodeGroupIndex = 0; electrodeGroupIndex < numElectrodeGroups; electrodeGroupIndex++)
        {
            XmlElement* xmlNode = xml->createNewChildElement ("NP_PROBE");

            xmlNode->setAttribute ("slot", probe->basestation->slot);
            xmlNode->setAttribute ("bs_firmware_version", probe->basestation->info.boot_version);
            xmlNode->setAttribute ("bs_hardware_version", probe->basestation->info.version);
            xmlNode->setAttribute ("bs_serial_number", String (probe->basestation->info.serial_number));
            xmlNode->setAttribute ("bs_part_number", probe->basestation->info.part_number);

            if (thread->type == PXI)
            {
                xmlNode->setAttribute ("bsc_firmware_version", probe->basestation->basestationConnectBoard->info.boot_version);
                xmlNode->setAttribute ("bsc_hardware_version", probe->basestation->basestationConnectBoard->info.version);
                xmlNode->setAttribute ("bsc_serial_number", String (probe->basestation->basestationConnectBoard->info.serial_number));
                xmlNode->setAttribute ("bsc_part_number", probe->basestation->basestationConnectBoard->info.part_number);
            }

            xmlNode->setAttribute ("headstage_serial_number", String (probe->headstage->info.serial_number));
            xmlNode->setAttribute ("headstage_part_number", probe->headstage->info.part_number);

            xmlNode->setAttribute ("flex_version", probe->flex->info.version);
            xmlNode->setAttribute ("flex_part_number", probe->headstage->info.part_number);

            xmlNode->setAttribute ("port", probe->headstage->port);
            xmlNode->setAttribute ("dock", probe->dock);

            if (probe->type == ProbeType::QUAD_BASE)
            {
                xmlNode->setAttribute ("shank", electrodeGroupIndex);
            }
            xmlNode->setAttribute ("probe_serial_number", String (probe->info.serial_number));
            xmlNode->setAttribute ("probe_part_number", probe->info.part_number);
            xmlNode->setAttribute ("probe_name", probe->name);
            xmlNode->setAttribute ("num_adcs", probe->probeMetadata.num_adcs);
            xmlNode->setAttribute ("custom_probe_name", probe->customName.probeSpecific);

            xmlNode->setAttribute ("ZoomHeight", probeBrowser->getZoomHeight());
            xmlNode->setAttribute ("ZoomOffset", probeBrowser->getZoomOffset());

            if (apGainComboBox != nullptr)
            {
                xmlNode->setAttribute ("apGainValue", apGainComboBox->getText());
                xmlNode->setAttribute ("apGainIndex", apGainComboBox->getSelectedId() - 1);
            }

            if (lfpGainComboBox != nullptr)
            {
                xmlNode->setAttribute ("lfpGainValue", lfpGainComboBox->getText());
                xmlNode->setAttribute ("lfpGainIndex", lfpGainComboBox->getSelectedId() - 1);
            }

            if (electrodeConfigurationComboBox != nullptr)
            {
                if (electrodeConfigurationComboBox->getSelectedId() > 1)
                {
                    xmlNode->setAttribute ("electrodeConfigurationPreset", electrodeConfigurationComboBox->getText());
                }
                else
                {
                    xmlNode->setAttribute ("electrodeConfigurationPreset", "NONE");
                }
            }

            if (referenceComboBox != nullptr)
            {
                if (referenceComboBox->getSelectedId() > 0)
                {
                    xmlNode->setAttribute ("referenceChannel", referenceComboBox->getText());
                    xmlNode->setAttribute ("referenceChannelIndex", referenceComboBox->getSelectedId() - 1);
                }
                else
                {
                    xmlNode->setAttribute ("referenceChannel", "Ext");
                    xmlNode->setAttribute ("referenceChannelIndex", 0);
                }
            }

            if (filterComboBox != nullptr)
            {
                xmlNode->setAttribute ("filterCut", filterComboBox->getText());
                xmlNode->setAttribute ("filterCutIndex", filterComboBox->getSelectedId());
            }

            XmlElement* channelNode = xmlNode->createNewChildElement ("CHANNELS");
            XmlElement* xposNode = xmlNode->createNewChildElement ("ELECTRODE_XPOS");
            XmlElement* yposNode = xmlNode->createNewChildElement ("ELECTRODE_YPOS");

            ProbeSettings p = getProbeSettings();

            for (int i = 0; i < p.selectedChannel.size(); i++)
            {
                int bank = int (p.selectedBank[i]);
                int shank = p.selectedShank[i];
                int channel = p.selectedChannel[i];
                int elec = p.selectedElectrode[i];

                String chString = String (bank);

                if (probe->type == ProbeType::NP2_4)
                    chString += ":" + String (shank);

                String chId = "CH" + String (channel);
                if (probe->type == ProbeType::QUAD_BASE)
                    chId += "_" + String (shank);

                if (probe->type == ProbeType::QUAD_BASE)
                {
                    if (shank == electrodeGroupIndex)
                    {
                        channelNode->setAttribute (chId, chString);
                        xposNode->setAttribute (chId, String (probe->electrodeMetadata[elec].xpos + 250 * shank));
                        yposNode->setAttribute (chId, String (probe->electrodeMetadata[elec].ypos));
                    }
                }
                else
                {
                    channelNode->setAttribute (chId, chString);
                    xposNode->setAttribute (chId, String (probe->electrodeMetadata[elec].xpos + 250 * shank));
                    yposNode->setAttribute (chId, String (probe->electrodeMetadata[elec].ypos));
                }
            }

            if (probe->emissionSiteMetadata.size() > 0)
            {
                XmlElement* emissionSiteNode = xmlNode->createNewChildElement ("EMISSION_SITES");

                for (int i = 0; i < probe->emissionSiteMetadata.size(); i++)
                {
                    XmlElement* emissionSite = emissionSiteNode->createNewChildElement ("SITE");

                    EmissionSiteMetadata& metadata = probe->emissionSiteMetadata[i];

                    emissionSite->setAttribute ("WAVELENGTH", metadata.wavelength_nm);
                    emissionSite->setAttribute ("SHANK_INDEX", metadata.shank_index);
                    emissionSite->setAttribute ("XPOS", metadata.xpos);
                    emissionSite->setAttribute ("YPOS", metadata.ypos);
                }
            }

            if (imroFiles.size() > 0)
            {
                XmlElement* imroFilesNode = xmlNode->createNewChildElement ("IMRO_FILES");

                for (int i = 0; i < imroFiles.size(); i++)
                {
                    if (! imroLoadedFromFolder[i])
                    {
                        XmlElement* imroFileNode = imroFilesNode->createNewChildElement ("FILE");
                        imroFileNode->setAttribute ("PATH", imroFiles[i]);
                    }
                }
            }

            xmlNode->setAttribute ("visualizationMode", mode);
            xmlNode->setAttribute ("activityToView", probeBrowser->activityToView);

            // annotations
            for (int i = 0; i < annotations.size(); i++)
            {
                Annotation& a = annotations.getReference (i);
                XmlElement* annotationNode = xmlNode->createNewChildElement ("ANNOTATIONS");
                annotationNode->setAttribute ("text", a.text);
                annotationNode->setAttribute ("channel", a.electrodes[0]);
                annotationNode->setAttribute ("R", a.colour.getRed());
                annotationNode->setAttribute ("G", a.colour.getGreen());
                annotationNode->setAttribute ("B", a.colour.getBlue());
            }

            xmlNode->setAttribute ("isEnabled", bool (probe->isEnabled));
        }
    }
}

void NeuropixInterface::loadParameters (XmlElement* xml)
{
    if (probe != nullptr)
    {
        String mySerialNumber = String (probe->info.serial_number);

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

        if (probe->type != ProbeType::QUAD_BASE)
        {
            for (int i = 0; i < probe->channel_count; i++)
            {
                settings.selectedBank.add (Bank::A);
                settings.selectedChannel.add (probe->electrodeMetadata[i].channel);
                settings.selectedShank.add (0);
                settings.selectedElectrode.add (probe->electrodeMetadata[i].global_index);
            }
        }
        else
        {
            for (int shank = 0; shank < 4; shank++)
            {
                for (int i = 0; i < 384; i++)
                {
                    settings.selectedBank.add (Bank::A);
                    settings.selectedChannel.add (i);
                    settings.selectedShank.add (shank);
                    settings.selectedElectrode.add (i + shank * 1280);
                }
            }
        }

        Array<XmlElement*> matchingNodes;

        // find by serial number
        forEachXmlChildElement (*xml, xmlNode)
        {
            if (xmlNode->hasTagName ("NP_PROBE"))
            {
                if (xmlNode->getStringAttribute ("probe_serial_number").equalsIgnoreCase (mySerialNumber))
                {
                    LOGC ("Found matching serial number: ", mySerialNumber);
                    matchingNodes.add (xmlNode);
                }
            }
        }

        // if not, search for matching port
        if (matchingNodes.size() == 0)
        {
            forEachXmlChildElement (*xml, xmlNode)
            {
                if (xmlNode->hasTagName ("NP_PROBE"))
                {
                    if (xmlNode->getIntAttribute ("slot") == probe->basestation->slot && xmlNode->getIntAttribute ("port") == probe->headstage->port && xmlNode->getIntAttribute ("dock") == probe->dock)
                    {
                        String PN = xmlNode->getStringAttribute ("probe_part_number");
                        ProbeType type = ProbeType::NP1;

                        if (PN.equalsIgnoreCase ("NP1010") || PN.equalsIgnoreCase ("NP1011") || PN.equalsIgnoreCase ("NP1012") || PN.equalsIgnoreCase ("NP1013")
                            || PN.equalsIgnoreCase ("NP1015") || PN.equalsIgnoreCase ("NP1016"))
                            type = ProbeType::NHP10;

                        else if (PN.equalsIgnoreCase ("NP1020") || PN.equalsIgnoreCase ("NP1021") || PN.equalsIgnoreCase ("NP1022"))
                            type = ProbeType::NHP25;

                        else if (PN.equalsIgnoreCase ("NP1030") || PN.equalsIgnoreCase ("NP1031") || PN.equalsIgnoreCase ("NP1032"))
                            type = ProbeType::NHP45;

                        else if (PN.equalsIgnoreCase ("NP1200") || PN.equalsIgnoreCase ("NP1210"))
                            type = ProbeType::NHP1;

                        else if (PN.equalsIgnoreCase ("PRB2_1_2_0640_0") || PN.equalsIgnoreCase ("NP2000") || PN.equalsIgnoreCase ("NP2003") || PN.equalsIgnoreCase ("NP2004"))
                            type = ProbeType::NP2_1;

                        else if (PN.equalsIgnoreCase ("PRB2_4_2_0640_0") || PN.equalsIgnoreCase ("NP2010") || PN.equalsIgnoreCase ("NP2013") || PN.equalsIgnoreCase ("NP2014"))
                            type = ProbeType::NP2_4;

                        else if (PN.equalsIgnoreCase ("NP2020"))
                            type = ProbeType::QUAD_BASE;

                        else if (PN.equalsIgnoreCase ("PRB_1_4_0480_1") || PN.equalsIgnoreCase ("PRB_1_4_0480_1_C") || PN.equalsIgnoreCase ("PRB_1_2_0480_2"))
                            type = ProbeType::NP1;

                        else if (PN.equalsIgnoreCase ("NP1100") || PN.equalsIgnoreCase ("NP1120") || PN.equalsIgnoreCase ("NP1121") || PN.equalsIgnoreCase ("NP1122") || PN.equalsIgnoreCase ("NP1123"))
                            type = ProbeType::UHD1;

                        else if (PN.equalsIgnoreCase ("NP1110"))
                            type = ProbeType::UHD2;

                        if (type == probe->type)
                        {
                            matchingNodes.add (xmlNode);

                            break;
                        }
                    }
                }
            }
        }

        for (int nodeIndex = 0; nodeIndex < matchingNodes.size(); nodeIndex++)
        {
            XmlElement* matchingNode = matchingNodes[nodeIndex];

            if (matchingNode->getChildByName ("CHANNELS"))
            {
                if (nodeIndex == 0)
                {
                    settings.selectedBank.clear();
                    settings.selectedChannel.clear();
                    settings.selectedShank.clear();
                    settings.selectedElectrode.clear();
                }

                XmlElement* status = matchingNode->getChildByName ("CHANNELS");

                if (probe->type != ProbeType::QUAD_BASE)
                {
                    for (int i = 0; i < probe->channel_count; i++)
                    {
                        settings.selectedChannel.add (i);

                        String bankInfo = status->getStringAttribute ("CH" + String (i));
                        Bank bank = static_cast<Bank> (bankInfo.substring (0, 1).getIntValue());
                        int shank = 0;

                        if (probe->type == ProbeType::NP2_4)
                        {
                            shank = bankInfo.substring (2, 3).getIntValue();
                        }

                        settings.selectedBank.add (bank);
                        settings.selectedShank.add (shank);

                        for (int j = 0; j < electrodeMetadata.size(); j++)
                        {
                            if (electrodeMetadata[j].channel == i)
                            {
                                if (electrodeMetadata[j].bank == bank && electrodeMetadata[j].shank == shank)
                                {
                                    settings.selectedElectrode.add (j);
                                }
                            }
                        }
                    }
                }
                else
                {
                    for (int i = 0; i < 384; i++)
                    {
                        settings.selectedChannel.add (i);

                        String bankInfo = status->getStringAttribute ("CH" + String (i) + "_" + String (nodeIndex));
                        Bank bank = static_cast<Bank> (bankInfo.substring (0, 1).getIntValue());

                        settings.selectedBank.add (bank);
                        settings.selectedShank.add (nodeIndex);

                        for (int j = 0; j < electrodeMetadata.size(); j++)
                        {
                            if (electrodeMetadata[j].channel == i && electrodeMetadata[j].bank == bank && electrodeMetadata[j].shank == nodeIndex)
                            {
                                settings.selectedElectrode.add (j);
                            }
                        }
                    }
                }
            }

            if (nodeIndex == 0)
            {
                probeBrowser->setZoomHeightAndOffset (matchingNode->getIntAttribute ("ZoomHeight"),
                                                      matchingNode->getIntAttribute ("ZoomOffset"));

                String customName = thread->getCustomProbeName (matchingNode->getStringAttribute ("probe_serial_number"));

                if (customName.length() > 0)
                {
                    probe->customName.probeSpecific = customName;
                }

                settings.apGainIndex = matchingNode->getIntAttribute ("apGainIndex", 3);
                settings.lfpGainIndex = matchingNode->getIntAttribute ("lfpGainIndex", 2);
                settings.referenceIndex = matchingNode->getIntAttribute ("referenceChannelIndex", 0);
                if (settings.referenceIndex >= referenceComboBox->getNumItems())
                    settings.referenceIndex = 0;

                String configurationName = matchingNode->getStringAttribute ("electrodeConfigurationPreset", "NONE");

                for (int i = 0; i < electrodeConfigurationComboBox->getNumItems(); i++)
                {
                    if (electrodeConfigurationComboBox->getItemText (i).equalsIgnoreCase (configurationName))
                    {
                        electrodeConfigurationComboBox->setSelectedItemIndex (i, dontSendNotification);
                        settings.electrodeConfigurationIndex = i - 1;

                        break;
                    }
                }

                settings.apFilterState = matchingNode->getIntAttribute ("filterCutIndex", 1) == 1;

                forEachXmlChildElement (*matchingNode, imroNode)
                {
                    if (imroNode->hasTagName ("IMRO_FILES"))
                    {
                        forEachXmlChildElement (*imroNode, fileNode)
                        {
                            imroFiles.add (fileNode->getStringAttribute ("PATH"));
                            imroLoadedFromFolder.add (false);
                            loadImroComboBox->addItem (File (imroFiles.getLast()).getFileName(),
                                                       imroFiles.size() + 1);
                        }
                    }
                }

                forEachXmlChildElement (*matchingNode, annotationNode)
                {
                    if (annotationNode->hasTagName ("ANNOTATIONS"))
                    {
                        Array<int> annotationChannels;
                        annotationChannels.add (annotationNode->getIntAttribute ("electrode"));
                        annotations.add (Annotation (annotationNode->getStringAttribute ("text"),
                                                     annotationChannels,
                                                     Colour (annotationNode->getIntAttribute ("R"),
                                                             annotationNode->getIntAttribute ("G"),
                                                             annotationNode->getIntAttribute ("B"))));
                    }
                }

                probe->isEnabled = matchingNode->getBoolAttribute ("isEnabled", true);
                probe->settings.isEnabled = probe->isEnabled;
                probeEnableButton->setToggleState (probe->isEnabled, dontSendNotification);
                if (probe->isEnabled)
                    probeEnableButton->setLabel ("ENABLED");
                else
                {
                    probeEnableButton->setLabel ("DISABLED");
                }
                stopAcquisition();
            }
        }

        probe->updateSettings (settings);

        applyProbeSettings (settings, false);
    }
}

// --------------------------------------

Annotation::Annotation (String t, Array<int> e, Colour c)
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

AnnotationColourSelector::AnnotationColourSelector (NeuropixInterface* np)
{
    npi = np;
    Path p;
    p.addRoundedRectangle (0, 0, 15, 15, 3);

    for (int i = 0; i < 6; i++)
    {
        standardColours.add (Colour (245, 245, 245 - 40 * i));
        hoverColours.add (Colour (215, 215, 215 - 40 * i));
    }

    for (int i = 0; i < 6; i++)
    {
        buttons.add (new ShapeButton (String (i), standardColours[i], hoverColours[i], hoverColours[i]));
        buttons[i]->setShape (p, true, true, false);
        buttons[i]->setBounds (18 * i, 0, 15, 15);
        buttons[i]->addListener (this);
        addAndMakeVisible (buttons[i]);

        strings.add ("Annotation " + String (i + 1));
    }

    npi->setAnnotationLabel (strings[0], standardColours[0]);

    activeButton = 0;
}

AnnotationColourSelector::~AnnotationColourSelector()
{
}

void AnnotationColourSelector::buttonClicked (Button* b)
{
    activeButton = buttons.indexOf ((ShapeButton*) b);

    npi->setAnnotationLabel (strings[activeButton], standardColours[activeButton]);
}

void AnnotationColourSelector::updateCurrentString (String s)
{
    strings.set (activeButton, s);
}

Colour AnnotationColourSelector::getCurrentColour()
{
    return standardColours[activeButton];
}