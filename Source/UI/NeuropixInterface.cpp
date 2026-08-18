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

namespace
{
// Dashboard layout metrics
constexpr int OUTER_MARGIN = 20;
constexpr int PANEL_GAP = 16;
constexpr int PANEL_PADDING = 12;
constexpr int PANEL_TITLE_HEIGHT = 22;
constexpr int PANEL_TITLE_GAP = 8;
constexpr int LABEL_HEIGHT = 20;
constexpr int ROW_HEIGHT = 22;
constexpr int SECTION_GAP = 12;
constexpr int LEGEND_HEIGHT = 44;
constexpr int PROBE_CONTROL_WIDTH = 280;
constexpr int DEVICE_COLUMN_WIDTH = 360;
constexpr int PROBE_SETTINGS_HEIGHT = 90;
constexpr int SELF_TEST_HEIGHT = 120;
constexpr int MIN_CONTENT_WIDTH = 1300;
} // namespace

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
        addAndMakeVisible (probeBrowser.get());

        probeStatusLabel = std::make_unique<Label> ("PROBE STATUS", "PROBE STATUS");
        probeStatusLabel->setFont (FontOptions ("Inter", "Regular", 13.0f));
        addAndMakeVisible (probeStatusLabel.get());

        probeEnableButton = std::make_unique<UtilityButton> ("ENABLED");
        probeEnableButton->setRadius (3.0f);
        probeEnableButton->setClickingTogglesState (true);
        probeEnableButton->setToggleState (probe->settings.isEnabled, dontSendNotification);
        probeEnableButton->setTooltip ("If disabled, probe will not stream data during acquisition");
        probeEnableButton->addListener (this);
        addAndMakeVisible (probeEnableButton.get());

        calibrationStatusValue = std::make_unique<Label> ("CALIBRATION STATUS", "UNCALIBRATED");
        calibrationStatusValue->setFont (FontOptions ("Inter", "Regular", 12.0f));
        calibrationStatusValue->setJustificationType (Justification::centred);
        calibrationStatusValue->setColour (Label::textColourId, Colours::white);
        calibrationStatusValue->setInterceptsMouseClicks (false, false);
        addAndMakeVisible (calibrationStatusValue.get());

        updateCalibrationStatusIndicator();

        electrodesLabel = std::make_unique<Label> ("ELECTRODES", "ELECTRODES");
        electrodesLabel->setFont (FontOptions ("Inter", "Regular", 13.0f));
        addAndMakeVisible (electrodesLabel.get());

        enableViewButton = std::make_unique<UtilityButton> ("VIEW");
        enableViewButton->setRadius (3.0f);
        enableViewButton->addListener (this);
        enableViewButton->setTooltip ("View electrode enabled state");
        addAndMakeVisible (enableViewButton.get());

        enableButton = std::make_unique<UtilityButton> ("ENABLE");
        enableButton->setRadius (3.0f);
        enableButton->addListener (this);
        enableButton->setTooltip ("Enable selected electrodes");
        // Make button invisible for UHD2 probes (as they use presets)
        if (probe->type == ProbeType::UHD2)
            addChildComponent (enableButton.get());
        else
            addAndMakeVisible (enableButton.get());

        electrodePresetLabel = std::make_unique<Label> ("ELECTRODE PRESET", "ELECTRODE PRESET");
        electrodePresetLabel->setFont (FontOptions ("Inter", "Regular", 13.0f));
        addAndMakeVisible (electrodePresetLabel.get());

        electrodeConfigurationComboBox = std::make_unique<ComboBox> ("electrodeConfigurationComboBox");
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

        if (probe->type == ProbeType::UHD2)
            electrodeConfigurationComboBox->setSelectedId (probe->settings.electrodeConfigurationIndex + 2, dontSendNotification);

        addAndMakeVisible (electrodeConfigurationComboBox.get());

        if (probe->settings.availableApGains.size() > 0)
        {
            apGainComboBox = std::make_unique<ComboBox> ("apGainComboBox");
            apGainComboBox->addListener (this);

            for (int i = 0; i < probe->settings.availableApGains.size(); i++)
                apGainComboBox->addItem (String (probe->settings.availableApGains[i]) + "x", i + 1);

            apGainComboBox->setSelectedId (probe->settings.apGainIndex + 1, dontSendNotification);
            addAndMakeVisible (apGainComboBox.get());

            apGainViewButton = std::make_unique<UtilityButton> ("VIEW");
            apGainViewButton->setRadius (3.0f);
            apGainViewButton->addListener (this);
            apGainViewButton->setTooltip ("View AP gain of each channel");
            addAndMakeVisible (apGainViewButton.get());

            apGainLabel = std::make_unique<Label> ("AP GAIN", "AP GAIN");
            apGainLabel->setFont (FontOptions ("Inter", "Regular", 13.0f));
            addAndMakeVisible (apGainLabel.get());
        }

        if (probe->settings.availableLfpGains.size() > 0)
        {
            lfpGainComboBox = std::make_unique<ComboBox> ("lfpGainComboBox");
            lfpGainComboBox->addListener (this);

            for (int i = 0; i < probe->settings.availableLfpGains.size(); i++)
                lfpGainComboBox->addItem (String (probe->settings.availableLfpGains[i]) + "x", i + 1);

            lfpGainComboBox->setSelectedId (probe->settings.lfpGainIndex + 1, dontSendNotification);
            addAndMakeVisible (lfpGainComboBox.get());

            lfpGainViewButton = std::make_unique<UtilityButton> ("VIEW");
            lfpGainViewButton->setRadius (3.0f);
            lfpGainViewButton->addListener (this);
            lfpGainViewButton->setTooltip ("View LFP gain of each channel");
            addAndMakeVisible (lfpGainViewButton.get());

            lfpGainLabel = std::make_unique<Label> ("LFP GAIN", "LFP GAIN");
            lfpGainLabel->setFont (FontOptions ("Inter", "Regular", 13.0f));
            addAndMakeVisible (lfpGainLabel.get());
        }

        if (probe->settings.availableReferences.size() > 0)
        {
            referenceComboBox = std::make_unique<ComboBox> ("ReferenceComboBox");
            referenceComboBox->addListener (this);

            for (int i = 0; i < probe->settings.availableReferences.size(); i++)
            {
                referenceComboBox->addItem (probe->settings.availableReferences[i], i + 1);
            }

            // if NP2_4 probe then find any broken shanks and disable tip reference option for those shanks
            if (probe->type == ProbeType::NP2_4 && referenceComboBox != nullptr)
            {
                Array<bool> shankProgrammable;
                shankProgrammable.insertMultiple (0, true, probeMetadata.shank_count);

                for (const auto& em : probe->electrodeMetadata)
                {
                    // If any electrode reports its shank as non-programmable, mark the whole shank as non-programmable
                    if (! em.shank_is_programmable)
                        shankProgrammable.set (em.shank, false);
                }

                // Disable "Tip" reference items corresponding to broken/non-programmable shanks
                for (int i = 0; i < probeMetadata.shank_count; ++i)
                {
                    if (! shankProgrammable[i])
                    {
                        // Find and disable tip reference option for this shank
                        for (int index = 0; index <= referenceComboBox->getNumItems(); ++index)
                        {
                            if (referenceComboBox->getItemText (index).equalsIgnoreCase (String (i + 1) + ": Tip"))
                            {
                                referenceComboBox->setItemEnabled (index + 1, false);
                                break;
                            }
                        }
                    }
                }
            }

            referenceComboBox->setSelectedId (probe->settings.referenceIndex + 1, dontSendNotification);
            addAndMakeVisible (referenceComboBox.get());

            referenceViewButton = std::make_unique<UtilityButton> ("VIEW");
            referenceViewButton->setRadius (3.0f);
            referenceViewButton->addListener (this);
            referenceViewButton->setTooltip ("View reference of each channel");
            addAndMakeVisible (referenceViewButton.get());

            referenceLabel = std::make_unique<Label> ("REFERENCE", "REFERENCE");
            referenceLabel->setFont (FontOptions ("Inter", "Regular", 13.0f));
            addAndMakeVisible (referenceLabel.get());
        }

        if (probe->hasApFilterSwitch())
        {
            filterComboBox = std::make_unique<ComboBox> ("FilterComboBox");
            filterComboBox->addListener (this);
            filterComboBox->addItem ("ON", 1);
            filterComboBox->addItem ("OFF", 2);
            filterComboBox->setSelectedId (1, dontSendNotification);
            addAndMakeVisible (filterComboBox.get());

            filterLabel = std::make_unique<Label> ("FILTER", "AP FILTER CUT");
            filterLabel->setFont (FontOptions ("Inter", "Regular", 13.0f));
            addAndMakeVisible (filterLabel.get());
        }

        activityViewButton = std::make_unique<UtilityButton> ("VIEW");
        activityViewButton->setRadius (3.0f);

        activityViewButton->addListener (this);
        activityViewButton->setTooltip ("View peak-to-peak amplitudes for each channel");
        addAndMakeVisible (activityViewButton.get());

        activityViewComboBox = std::make_unique<ComboBox> ("ActivityView Combo Box");

        if (probe->settings.availableLfpGains.size() > 0)
        {
            activityViewComboBox->addListener (this);
            activityViewComboBox->addItem ("AP", 1);
            activityViewComboBox->addItem ("LFP", 2);
            activityViewComboBox->setSelectedId (1, dontSendNotification);
            addAndMakeVisible (activityViewComboBox.get());
        }

        activityViewLabel = std::make_unique<Label> ("PROBE SIGNAL", "PROBE SIGNAL");
        activityViewLabel->setFont (FontOptions ("Inter", "Regular", 13.0f));
        addAndMakeVisible (activityViewLabel.get());

        activityViewFilterButton = std::make_unique<UtilityButton> ("BP FILTER");
        activityViewFilterButton->addListener (this);
        activityViewFilterButton->setTooltip ("View bandpass filtered signal");
        activityViewFilterButton->setClickingTogglesState (true);
        activityViewFilterButton->setToggleState (true, dontSendNotification);
        addAndMakeVisible (activityViewFilterButton.get());

        activityViewCARButton = std::make_unique<UtilityButton> ("CAR");
        activityViewCARButton->addListener (this);
        activityViewCARButton->setTooltip ("View common average referenced signal");
        activityViewCARButton->setClickingTogglesState (true);
        activityViewCARButton->setToggleState (true, dontSendNotification);
        addAndMakeVisible (activityViewCARButton.get());

        activityViewAmplitudeComboBox = std::make_unique<ComboBox> ("Activity View Amplitude Range");
        const std::array<const char*, 4> amplitudeLabels { "0 - 250 \xC2\xB5V", "0 - 500 \xC2\xB5V", "0 - 750 \xC2\xB5V", "0 - 1000 \xC2\xB5V" };
        for (int i = 0; i < amplitudeOptions.size(); ++i)
            activityViewAmplitudeComboBox->addItem (String::fromUTF8 (amplitudeLabels[i]), i + 1);
        activityViewAmplitudeComboBox->setSelectedId (2, dontSendNotification); // Default to 500 µV
        activityViewAmplitudeComboBox->addListener (this);
        activityViewAmplitudeComboBox->setTooltip ("Set amplitude scale for activity view");
        addAndMakeVisible (activityViewAmplitudeComboBox.get());

        if (probe->info.part_number == "NP1300" || probe->info.part_number == "NP1400") // Neuropixels Opto
        {
            redEmissionSiteLabel = std::make_unique<Label> ("RED EMISSION SITE", "RED EMISSION SITE");
            redEmissionSiteLabel->setFont (FontOptions ("Inter", "Regular", 13.0f));
            addAndMakeVisible (redEmissionSiteLabel.get());

            redEmissionSiteComboBox = std::make_unique<ComboBox> ("Red Emission Site Combo Box");
            redEmissionSiteComboBox->addListener (this);
            redEmissionSiteComboBox->addItem ("OFF", 1);

            for (int i = 0; i < 14; i++)
                redEmissionSiteComboBox->addItem (String (i + 1), i + 2);

            redEmissionSiteComboBox->setSelectedId (1, dontSendNotification);
            addAndMakeVisible (redEmissionSiteComboBox.get());

            blueEmissionSiteLabel = std::make_unique<Label> ("BLUE EMISSION SITE", "BLUE EMISSION SITE");
            blueEmissionSiteLabel->setFont (FontOptions ("Inter", "Regular", 13.0f));
            addAndMakeVisible (blueEmissionSiteLabel.get());

            blueEmissionSiteComboBox = std::make_unique<ComboBox> ("Blue Emission Site Combo Box");
            blueEmissionSiteComboBox->addListener (this);
            blueEmissionSiteComboBox->addItem ("OFF", 1);

            for (int i = 0; i < 14; i++)
                blueEmissionSiteComboBox->addItem (String (i + 1), i + 2);

            blueEmissionSiteComboBox->setSelectedId (1, dontSendNotification);
            addAndMakeVisible (blueEmissionSiteComboBox.get());
        }

        // BIST
        bistComboBox = std::make_unique<ComboBox> ("BistComboBox");
        bistComboBox->addListener (this);

        bistComboBox->addItem ("Select a test...", 1);
        bistComboBox->setItemEnabled (1, false);
        bistComboBox->addSeparator();

        availableBists.add (BIST::EMPTY);
        availableBists.add (BIST::SIGNAL); // 2 -- disabled
        availableBists.add (BIST::NOISE); // 3 -- disabled
        availableBists.add (BIST::PSB);
        bistComboBox->addItem ("Test PSB bus", 4);

        availableBists.add (BIST::CONFIG);
        bistComboBox->addItem ("Test probe configuration", 5);

        availableBists.add (BIST::EEPROM);
        bistComboBox->addItem ("Test EEPROM", 6);

        availableBists.add (BIST::SERDES);
        bistComboBox->addItem ("Test Serdes", 7);

        availableBists.add (BIST::HB);
        bistComboBox->addItem ("Test Heartbeat", 8);

        bistComboBox->setSelectedId (1, dontSendNotification);
        addAndMakeVisible (bistComboBox.get());

        bistButton = std::make_unique<UtilityButton> ("RUN");
        bistButton->setRadius (3.0f);
        bistButton->addListener (this);
        bistButton->setTooltip ("Run selected test");
        addAndMakeVisible (bistButton.get());

        // "BUILT-IN SELF TESTS" title is painted as part of the panel
        bistLabel = std::make_unique<Label> ("BIST", "Built-in self tests:");
        bistLabel->setFont (FontOptions ("Inter", "Regular", 15.0f));

        // COPY / PASTE / UPLOAD
        copyButton = std::make_unique<UtilityButton> ("COPY");
        copyButton->setRadius (3.0f);
        copyButton->addListener (this);
        copyButton->setTooltip ("Copy probe settings");
        addAndMakeVisible (copyButton.get());

        pasteButton = std::make_unique<UtilityButton> ("PASTE");
        pasteButton->setRadius (3.0f);
        pasteButton->addListener (this);
        pasteButton->setTooltip ("Paste probe settings");
        pasteButton->setEnabled (false);
        addAndMakeVisible (pasteButton.get());

        applyToAllButton = std::make_unique<UtilityButton> ("APPLY TO ALL");
        applyToAllButton->setRadius (3.0f);
        applyToAllButton->addListener (this);
        applyToAllButton->setTooltip ("Apply this probe's settings to all others");
        addAndMakeVisible (applyToAllButton.get());

        saveImroButton = std::make_unique<UtilityButton> ("SAVE TO IMRO");
        saveImroButton->setRadius (3.0f);
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
        saveJsonButton->addListener (this);
        saveJsonButton->setTooltip ("Save channel map to probeinterface .json file");
        addAndMakeVisible (saveJsonButton.get());

        loadJsonButton = std::make_unique<UtilityButton> ("LOAD FROM JSON");
        loadJsonButton->setRadius (3.0f);
        loadJsonButton->addListener (this);
        loadJsonButton->setTooltip ("Load channel map from probeinterface .json file");
        // addAndMakeVisible(loadJsonButton);

        loadImroComboBox = std::make_unique<ComboBox> ("Quick-load IMRO");
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

        // "PROBE SETTINGS" title is painted as part of the panel
        probeSettingsLabel = std::make_unique<Label> ("Settings", "Probe settings:");
        probeSettingsLabel->setFont (FontOptions ("Inter", "Regular", 13.0f));
    }
    else
    {
        type = SettingsInterface::BASESTATION_SETTINGS_INTERFACE;
    }

    // FIRMWARE
    firmwareUpdateButton = std::make_unique<TextButton> ("UPDATE FIRMWARE ...");
    firmwareUpdateButton->addListener (this);

    const bool firmwareUpdateRequired = basestation->isFirmwareUpdateRequired();
    firmwareUpdateButton->setEnabled (firmwareUpdateRequired);
    firmwareUpdateButton->setTooltip (firmwareUpdateRequired
                                          ? "Install API-compatible built-in firmware (BS first, then BSC)"
                                          : "Basestation firmware is already up to date");

    if (thread->type == PXI)
        addAndMakeVisible (firmwareUpdateButton.get());

    // PROBE INFO
    nameLabel = std::make_unique<Label> ("MAIN", "NAME");
    nameLabel->setFont (FontOptions ("Fira Code", "Medium", 30.0f));
    nameLabel->setJustificationType (Justification::centredLeft);
    addAndMakeVisible (nameLabel.get());

    infoLabel = std::make_unique<TextEditor> ("INFO");
    infoLabel->setMultiLine (true, false);
    infoLabel->setReadOnly (true);
    infoLabel->setScrollbarsShown (true);
    infoLabel->setCaretVisible (false);
    infoLabel->setPopupMenuEnabled (true);
    infoLabel->setFont (FontOptions (15.0f));
    infoLabel->setColour (TextEditor::backgroundColourId, Colours::transparentBlack);
    infoLabel->setColour (TextEditor::outlineColourId, Colours::transparentBlack);
    infoLabel->setColour (TextEditor::focusedOutlineColourId, Colours::transparentBlack);
    addAndMakeVisible (infoLabel.get());

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

    // The three-column dashboard needs more horizontal space than the default
    if (probe != nullptr)
    {
        viewport->setMinimumContentWidth (MIN_CONTENT_WIDTH);
    }
    else
    {
        int basestationInterfaceHeight = 45 + PANEL_TITLE_GAP + infoTextHeight + 20 + 24 + 2 * PANEL_PADDING;
        basestationInterfaceBounds = Rectangle<int> (OUTER_MARGIN, OUTER_MARGIN, DEVICE_COLUMN_WIDTH, basestationInterfaceHeight);
        viewport->setMinimumContentWidth (DEVICE_COLUMN_WIDTH + 2 * OUTER_MARGIN);
        viewport->setMinimumContentHeight (basestationInterfaceHeight + 2 * OUTER_MARGIN);
    }

    // Ensure initial layout is performed
    resized();

    // Check for damaged shanks on Quad Base probes and show warning if any found
    if (probe != nullptr && probe->type == ProbeType::QUAD_BASE)
    {
        for (int i = 0; i < probe->electrodeMetadata.size(); ++i)
        {
            if (! probe->electrodeMetadata[i].shank_is_programmable)
            {
                showDamagedShankWarning();
                break;
            }
        }
    }
}

NeuropixInterface::~NeuropixInterface()
{
}

void NeuropixInterface::updateCalibrationStatusIndicator()
{
    if (calibrationStatusValue == nullptr || probe == nullptr)
        return;

    const bool calibrated = probe->isCalibrated;
    calibrationStatusValue->setText (calibrated ? "CALIBRATED" : "UNCALIBRATED", dontSendNotification);
    calibrationStatusValue->setColour (Label::backgroundColourId, calibrated ? Colour (32, 118, 62) : Colour (166, 44, 44));
    calibrationStatusValue->setColour (Label::textColourId, Colours::white);
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

        if (probe->type == ProbeType::NP2_1 || probe->type == ProbeType::NP2_4 || probe->type == ProbeType::QUAD_BASE)
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
    }

    auto infoTextLines = StringArray::fromLines (infoString).size();
    infoTextHeight = infoTextLines * infoLabel->getFont().getHeight() + 2 * infoLabel->getTopIndent();

    infoLabel->setText (infoString, false);
    nameLabel->setText (nameString, dontSendNotification);
}

void NeuropixInterface::labelTextChanged (Label* label)
{
    if (label == annotationLabel.get())
    {
        annotationColourSelector->updateCurrentString (label->getText());
    }
}

void NeuropixInterface::lookAndFeelChanged()
{
    infoLabel->applyColourToAllText (findColour (ThemeColours::defaultText));
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
            }
            else
            {
                probeBrowser->activityToView = ActivityToView::LFPVIEW;
                ColourScheme::setColourScheme (ColourSchemeId::VIRIDIS);
            }
        }
        else if (comboBox == activityViewAmplitudeComboBox.get())
        {
            const int optionIndex = activityViewAmplitudeComboBox->getSelectedId() - 1;
            currentMaxPeakToPeak = amplitudeOptions[static_cast<size_t> (optionIndex)];
            probeBrowser->setMaxPeakToPeakAmplitude (currentMaxPeakToPeak);
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
            }
            else
            {
                probeBrowser->activityToView = ActivityToView::LFPVIEW;
                ColourScheme::setColourScheme (ColourSchemeId::VIRIDIS);
            }

            repaint();
        }
        else if (comboBox == activityViewAmplitudeComboBox.get())
        {
            const int optionIndex = activityViewAmplitudeComboBox->getSelectedId() - 1;
            currentMaxPeakToPeak = amplitudeOptions[static_cast<size_t> (optionIndex)];
            probeBrowser->setMaxPeakToPeakAmplitude (currentMaxPeakToPeak);
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
        if (applyProbeSettings (canvas->getProbeSettings()))
            CoreServices::updateSignalChain (editor);
    }
    else if (button == applyToAllButton.get())
    {
        canvas->applyParametersToAllProbes (getProbeSettings());
    }
    else if (button == firmwareUpdateButton.get())
    {
        const bool shouldProceed = AlertWindow::showOkCancelBox (AlertWindow::QuestionIcon,
                                                                 "Update firmware",
                                                                 "This will install API-compatible built-in firmware (BS first, then BSC) "
                                                                 "to the basestation on slot " + String (basestation->slot) + ". Do you want to continue?",
                                                                 "Continue",
                                                                 "Cancel");

        if (shouldProceed)
        {
            firmwareUpdateButton->setEnabled (false);
            basestation->updateFirmware();
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
    if (referenceComboBox->isItemEnabled (index + 1))
        referenceComboBox->setSelectedId (index + 1, true);
    else
        CoreServices::sendStatusMessage ("Unable to set reference to " + probe->settings.availableReferences[index]);
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

        if (electrodeConfigurationComboBox != nullptr
            && isPositiveAndBelow (probe->settings.electrodeConfigurationIndex, probe->settings.availableElectrodeConfigurations.size()))
        {
            electrodeConfigurationComboBox->setSelectedItemIndex (probe->settings.electrodeConfigurationIndex + 1, dontSendNotification);
        }

        for (int i = 0; i < electrodeMetadata.size(); i++)
        {
            electrodeMetadata.getReference (i).shank_is_programmable = probe->electrodeMetadata.getReference (i).shank_is_programmable;
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
        bool electrodeFromBrokenShankSelected = false;
        for (int i = 0; i < electrodes.size(); i++)
        {
            Bank bank = electrodeMetadata[electrodes[i]].bank;
            int channel = electrodeMetadata[electrodes[i]].channel;
            int shank = electrodeMetadata[electrodes[i]].shank;
            int global_index = electrodeMetadata[electrodes[i]].global_index;

            for (int j = 0; j < electrodeMetadata.size(); j++)
            {
                electrodeMetadata.getReference (i).shank_is_programmable = probe->electrodeMetadata.getReference (i).shank_is_programmable;

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
                            if (electrodeMetadata[j].shank_is_programmable == false)
                            {
                                electrodeFromBrokenShankSelected = true;
                            }
                        }

                        else
                        {
                            electrodeMetadata.getReference (j).status = ElectrodeStatus::DISCONNECTED;
                        }
                    }
                }
            }
        }

        if (electrodeFromBrokenShankSelected)
        {
            showDamagedShankWarning();
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

    if (firmwareUpdateButton != nullptr)
        firmwareUpdateButton->setEnabled (false);

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
        pasteButton->setEnabled (canvas->getProbeSettings().probeType == probe->type);

    if (applyToAllButton != nullptr)
        applyToAllButton->setEnabled (enabledState);

    if (loadImroButton != nullptr)
        loadImroButton->setEnabled (enabledState);

    if (loadJsonButton != nullptr)
        loadJsonButton->setEnabled (enabledState);

    if (loadImroComboBox != nullptr)
        loadImroComboBox->setEnabled (enabledState);

    if (firmwareUpdateButton != nullptr)
        firmwareUpdateButton->setEnabled (basestation->isFirmwareUpdateRequired());
}

void NeuropixInterface::resized()
{
    if (probe == nullptr)
    {
        layoutBasestationInterface();
        return;
    }

    auto bounds = getLocalBounds().reduced (OUTER_MARGIN);

    // Full-width settings row at the bottom
    probeSettingsBounds = bounds.removeFromBottom (PROBE_SETTINGS_HEIGHT);
    bounds.removeFromBottom (PANEL_GAP);

    // Three main columns: overview takes the space left over by the fixed-width columns
    auto deviceColumn = bounds.removeFromRight (DEVICE_COLUMN_WIDTH);
    bounds.removeFromRight (PANEL_GAP);
    probeControlBounds = bounds.removeFromRight (PROBE_CONTROL_WIDTH);
    bounds.removeFromRight (PANEL_GAP);
    probeOverviewBounds = bounds;

    // Right column: device info on top, self tests below
    selfTestBounds = deviceColumn.removeFromBottom (SELF_TEST_HEIGHT);
    deviceColumn.removeFromBottom (PANEL_GAP);
    deviceInfoBounds = deviceColumn;

    // Probe overview: browser fills the panel, legend strip at the bottom
    auto overview = probeOverviewBounds.reduced (PANEL_PADDING);
    overview.removeFromTop (PANEL_TITLE_HEIGHT + PANEL_TITLE_GAP);
    electrodeLegendBounds = overview.removeFromBottom (LEGEND_HEIGHT);
    probeBrowser->setBounds (overview);

    layoutProbeControls (probeControlBounds.reduced (PANEL_PADDING));
    layoutDeviceInfo (deviceInfoBounds.reduced (PANEL_PADDING));
    layoutSelfTests (selfTestBounds.reduced (PANEL_PADDING));
    layoutProbeSettings (probeSettingsBounds.reduced (PANEL_PADDING));
}

void NeuropixInterface::layoutProbeControls (Rectangle<int> area)
{
    area.removeFromTop (PANEL_TITLE_HEIGHT + PANEL_TITLE_GAP); // painted panel title

    const int x = area.getX();
    const int w = area.getWidth();
    int y = area.getY();

    // Labels have internal horizontal padding; nudge left so text aligns with controls
    auto sectionLabel = [&] (Label* label)
    {
        if (label != nullptr)
        {
            label->setBounds (x - 4, y, w, LABEL_HEIGHT);
            y += LABEL_HEIGHT;
        }
    };

    // PROBE STATUS
    sectionLabel (probeStatusLabel.get());
    probeEnableButton->setBounds (x, y, 100, ROW_HEIGHT);
    calibrationStatusValue->setBounds (x + 108, y, 120, ROW_HEIGHT);
    y += ROW_HEIGHT + SECTION_GAP;

    // ELECTRODES
    sectionLabel (electrodesLabel.get());
    if (enableButton->isVisible())
    {
        enableButton->setBounds (x, y, 65, ROW_HEIGHT);
        enableViewButton->setBounds (x + 73, y + 2, 45, 18);
    }
    else
    {
        enableViewButton->setBounds (x, y + 2, 45, 18);
    }
    y += ROW_HEIGHT + SECTION_GAP;

    // ELECTRODE PRESET
    sectionLabel (electrodePresetLabel.get());
    electrodeConfigurationComboBox->setBounds (x, y, 160, ROW_HEIGHT);
    y += ROW_HEIGHT + SECTION_GAP;

    // AP GAIN
    if (apGainComboBox != nullptr)
    {
        sectionLabel (apGainLabel.get());
        apGainComboBox->setBounds (x, y, 65, ROW_HEIGHT);
        apGainViewButton->setBounds (x + 73, y + 2, 45, 18);
        y += ROW_HEIGHT + SECTION_GAP;
    }

    // LFP GAIN
    if (lfpGainComboBox != nullptr)
    {
        sectionLabel (lfpGainLabel.get());
        lfpGainComboBox->setBounds (x, y, 65, ROW_HEIGHT);
        lfpGainViewButton->setBounds (x + 73, y + 2, 45, 18);
        y += ROW_HEIGHT + SECTION_GAP;
    }

    // REFERENCE
    if (referenceComboBox != nullptr)
    {
        sectionLabel (referenceLabel.get());
        referenceComboBox->setBounds (x, y, 65, ROW_HEIGHT);
        referenceViewButton->setBounds (x + 73, y + 2, 45, 18);
        y += ROW_HEIGHT + SECTION_GAP;
    }

    // AP FILTER
    if (filterComboBox != nullptr)
    {
        sectionLabel (filterLabel.get());
        filterComboBox->setBounds (x, y, 75, ROW_HEIGHT);
        y += ROW_HEIGHT + SECTION_GAP;
    }

    // PROBE SIGNAL
    sectionLabel (activityViewLabel.get());
    if (activityViewComboBox->isVisible())
    {
        activityViewComboBox->setBounds (x, y, 65, ROW_HEIGHT);
        activityViewButton->setBounds (x + 73, y + 2, 45, 18);
    }
    else
    {
        activityViewButton->setBounds (x, y + 2, 45, 18);
    }
    y += ROW_HEIGHT + 6;
    activityViewFilterButton->setBounds (x, y, 70, 18);
    activityViewCARButton->setBounds (x + 78, y, 50, 18);
    y += 18 + 6;
    activityViewAmplitudeComboBox->setBounds (x, y, 100, ROW_HEIGHT);
    y += ROW_HEIGHT + SECTION_GAP;

    // OPTO EMISSION SITES (NP1300 only)
    if (redEmissionSiteComboBox != nullptr)
    {
        sectionLabel (redEmissionSiteLabel.get());
        redEmissionSiteComboBox->setBounds (x, y, 65, ROW_HEIGHT);
        y += ROW_HEIGHT + SECTION_GAP;

        sectionLabel (blueEmissionSiteLabel.get());
        blueEmissionSiteComboBox->setBounds (x, y, 65, ROW_HEIGHT);
        y += ROW_HEIGHT + SECTION_GAP;
    }
}

void NeuropixInterface::layoutDeviceInfo (Rectangle<int> area)
{
    area.removeFromTop (PANEL_TITLE_HEIGHT + PANEL_TITLE_GAP); // painted panel title

    nameLabel->setBounds (area.removeFromTop (40));
    area.removeFromTop (PANEL_TITLE_GAP);

    infoLabel->setBounds (area.withHeight (jmin (int(infoTextHeight), area.getHeight())));
}

void NeuropixInterface::layoutSelfTests (Rectangle<int> area)
{
    area.removeFromTop (PANEL_TITLE_HEIGHT + PANEL_TITLE_GAP); // painted panel title

    if (bistComboBox != nullptr)
    {
        auto row = area.removeFromTop (ROW_HEIGHT);
        bistButton->setBounds (row.removeFromRight (50));
        row.removeFromRight (8);
        bistComboBox->setBounds (row);
        area.removeFromTop (20);
    }

    firmwareUpdateButton->setBounds (area.removeFromTop (24).withWidth (160));
}

void NeuropixInterface::layoutProbeSettings (Rectangle<int> area)
{
    area.removeFromTop (PANEL_TITLE_HEIGHT + PANEL_TITLE_GAP); // painted panel title

    auto row = area.removeFromTop (ROW_HEIGHT);

    // One horizontal row; invisible controls (e.g. IMRO buttons on UHD probes) are skipped
    const std::pair<Component*, int> settingsControls[] = {
        { copyButton.get(), 75 },
        { pasteButton.get(), 75 },
        { applyToAllButton.get(), 120 },
        { saveImroButton.get(), 125 },
        { loadImroButton.get(), 135 },
        { saveJsonButton.get(), 125 },
        { loadImroComboBox.get(), 190 }
    };

    int x = row.getX();

    for (const auto& [component, width] : settingsControls)
    {
        if (component == nullptr || ! component->isVisible())
            continue;

        component->setBounds (x, row.getY(), width, row.getHeight());
        x += width + 10;
    }
}

void NeuropixInterface::layoutBasestationInterface()
{
    auto bounds = basestationInterfaceBounds.reduced (PANEL_PADDING);
    nameLabel->setBounds (bounds.getX(), bounds.getY(), bounds.getWidth(), 45);

    infoLabel->setBounds (bounds.getX(), nameLabel->getBottom() + PANEL_TITLE_GAP, bounds.getWidth(), infoTextHeight);

    const int x = bounds.getX();
    const int y = infoLabel->getBottom() + 20;
    firmwareUpdateButton->setBounds (x, y, 160, 24);
}

void NeuropixInterface::paint (Graphics& g)
{
    if (probe == nullptr)
    {
        drawPanel (g, basestationInterfaceBounds, "");

        return;
    }

    drawPanel (g, probeOverviewBounds, "PROBE OVERVIEW");
    drawPanel (g, probeControlBounds, "PROBE CONTROL");
    drawPanel (g, deviceInfoBounds, "DEVICE INFO");
    drawPanel (g, selfTestBounds, "BUILT-IN SELF TESTS");
    drawPanel (g, probeSettingsBounds, "PROBE SETTINGS");

    if (probe->info.part_number != "NP1300" && probe->info.part_number != "NP1400") // Neuropixels Opto
        drawLegend (g);
}

void NeuropixInterface::drawPanel (Graphics& g, Rectangle<int> area, const String& title)
{
    if (area.isEmpty())
        return;

    auto panelBounds = area.toFloat();

    g.setColour (findColour (ThemeColours::componentParentBackground).withAlpha (0.25f));
    g.fillRoundedRectangle (panelBounds, 8.0f);

    g.setColour (findColour (ThemeColours::outline).withAlpha (0.75f));
    g.drawRoundedRectangle (panelBounds, 8.0f, 1.0f);

    if (title.isEmpty())
        return;

    g.setColour (findColour (ThemeColours::defaultText));
    g.setFont (FontOptions ("Inter", "Semi Bold", 16.0f));
    g.drawText (title,
                area.getX() + PANEL_PADDING,
                area.getY() + PANEL_PADDING,
                area.getWidth() - 2 * PANEL_PADDING,
                PANEL_TITLE_HEIGHT,
                Justification::centredLeft);
}

void NeuropixInterface::drawLegend (Graphics& g)
{
    if (thread->isRefreshing)
        return;

    // Build the list of swatch/text entries for the current visualization mode
    String heading;
    Array<std::pair<Colour, String>> entries;

    switch (mode)
    {
        case ENABLE_VIEW:
            heading = "ELECTRODE STATUS";
            entries.add ({ Colours::yellow, "ENABLED" });
            entries.add ({ Colour (180, 180, 180), "DISABLED" });
            entries.add ({ Colours::salmon, "SHANK ERROR" });

            if (probe->type == ProbeType::NP2_1 || probe->type == ProbeType::NP2_4)
                entries.add ({ Colours::purple, "SELECTABLE REFERENCE" });
            else
                entries.add ({ Colours::black, "REFERENCE" });

            break;

        case AP_GAIN_VIEW:
            heading = "AP GAIN";

            for (int i = 0; i < 8; i++)
                entries.add ({ Colour (25 * i, 25 * i, 50), apGainComboBox->getItemText (i) });

            break;

        case LFP_GAIN_VIEW:
            heading = "LFP GAIN";

            for (int i = 0; i < 8; i++)
                entries.add ({ Colour (66, 25 * i, 35 * i), lfpGainComboBox->getItemText (i) });

            break;

        case REFERENCE_VIEW:
            heading = "REFERENCE";

            for (int i = 0; i < referenceComboBox->getNumItems(); i++)
            {
                String referenceDescription = referenceComboBox->getItemText (i);

                Colour swatchColour;

                if (referenceDescription.contains ("Ext"))
                    swatchColour = Colours::pink;
                else if (referenceDescription.contains ("Tip"))
                    swatchColour = Colours::orange;
                else
                    swatchColour = Colours::purple;

                entries.add ({ swatchColour, referenceDescription });
            }

            break;

        case ACTIVITY_VIEW:
            heading = "AMPLITUDE";

            for (int i = 0; i < 6; i++)
                entries.add ({ ColourScheme::getColourForNormalizedValue (float (i) / 5.0f),
                               String (float (currentMaxPeakToPeak) / 5.0f * float (i)) + " uV" });

            break;
    }

    // Flow the entries horizontally beneath the probe browser, wrapping if needed
    const Font legendFont (FontOptions ("Inter", "Regular", 13.0f));
    const Colour textColour = findColour (ThemeColours::defaultText).withAlpha (0.75f);

    const int swatchSize = 12;
    const int rowHeight = 18;
    const int itemGap = 14;

    g.setFont (legendFont);

    int x = electrodeLegendBounds.getX() + PANEL_PADDING;
    int y = electrodeLegendBounds.getY() + 4;

    if (heading.isNotEmpty())
    {
        const String headingText = heading + ":";
        const int headingWidth = GlyphArrangement::getStringWidthInt (legendFont, headingText);

        g.setColour (textColour);
        g.drawText (headingText, x, y, headingWidth, rowHeight, Justification::centredLeft);
        x += headingWidth + itemGap;
    }

    for (const auto& [swatchColour, text] : entries)
    {
        const int textWidth = GlyphArrangement::getStringWidthInt (legendFont, text);
        const int itemWidth = swatchSize + 5 + textWidth;

        if (x + itemWidth > electrodeLegendBounds.getRight() && x > electrodeLegendBounds.getX())
        {
            x = electrodeLegendBounds.getX() + PANEL_PADDING;
            y += rowHeight;
        }

        g.setColour (swatchColour);
        g.fillRect (x, y + (rowHeight - swatchSize) / 2, swatchSize, swatchSize);

        g.setColour (textColour);
        g.drawText (text, x + swatchSize + 5, y, textWidth, rowHeight, Justification::centredLeft);

        x += itemWidth + itemGap;
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
    if (p.probeType != probe->type)
    {
        CoreServices::sendStatusMessage ("Probe types do not match.");
        return false;
    }

    if (p.probe == nullptr)
    {
        CoreServices::sendStatusMessage ("Probe settings invalid.");
        return false;
    }

    LOGD ("NeuropixInterface applying probe settings for ", p.probe->name, " shouldUpdate: ", shouldUpdateProbe);

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
    {
        if (probe->type == ProbeType::NP2_4)
        {
            int itemId = referenceComboBox->getItemId (p.referenceIndex);
            if (referenceComboBox->isItemEnabled (itemId))
                referenceComboBox->setSelectedId (p.referenceIndex + 1, dontSendNotification);
        }
        else
        {
            referenceComboBox->setSelectedId (p.referenceIndex + 1, dontSendNotification);
        }
    }

    for (int i = 0; i < electrodeMetadata.size(); i++)
    {
        if (electrodeMetadata[i].status == ElectrodeStatus::CONNECTED)
            electrodeMetadata.getReference (i).status = ElectrodeStatus::DISCONNECTED;
    }

    if (probe->type == ProbeType::UHD2)
    {
        if (p.electrodeConfigurationIndex >= 0 && p.electrodeConfigurationIndex < electrodeConfigurationComboBox->getNumItems() - 1)
        {
            electrodeConfigurationComboBox->setSelectedItemIndex (p.electrodeConfigurationIndex + 1, dontSendNotification);
            probe->settings.electrodeConfigurationIndex = p.electrodeConfigurationIndex;
        }

        String configName = electrodeConfigurationComboBox->getText();

        Array<int> selection = probe->selectElectrodeConfiguration (configName);

        selectElectrodes (selection);
    }
    else
    {
        // update selection state
        bool electrodeFromBrokenShankSelected = false;
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

                    if (electrodeMetadata[j].shank_is_programmable == false)
                    {
                        electrodeFromBrokenShankSelected = true;
                    }
                }
            }
        }

        if (electrodeFromBrokenShankSelected && probe->type != ProbeType::QUAD_BASE)
        {
            showDamagedShankWarning();
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

void NeuropixInterface::setPasteButtonEnabled (bool enabled)
{
    if (pasteButton != nullptr)
        pasteButton->setEnabled (enabled);
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
            XmlElement* electrodeNode = xmlNode->createNewChildElement ("ELECTRODE_INDEX");

            ProbeSettings p = getProbeSettings();

            // Create sorted indices based on channel number
            Array<int> sortedIndices;
            for (int i = 0; i < p.selectedChannel.size(); i++)
                sortedIndices.add (i);

            // Sort indices by channel number
            for (int i = 0; i < sortedIndices.size() - 1; i++)
            {
                for (int j = i + 1; j < sortedIndices.size(); j++)
                {
                    if (p.selectedChannel[sortedIndices[j]] < p.selectedChannel[sortedIndices[i]])
                    {
                        int temp = sortedIndices[i];
                        sortedIndices.set (i, sortedIndices[j]);
                        sortedIndices.set (j, temp);
                    }
                }
            }

            for (int idx = 0; idx < sortedIndices.size(); idx++)
            {
                int i = sortedIndices[idx];

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
                        electrodeNode->setAttribute (chId, elec);
                        channelNode->setAttribute (chId, chString);
                        xposNode->setAttribute (chId, String (probe->electrodeMetadata[elec].xpos + 250 * shank));
                        yposNode->setAttribute (chId, String (probe->electrodeMetadata[elec].ypos));
                    }
                }
                else
                {
                    electrodeNode->setAttribute (chId, elec);
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
            xmlNode->setAttribute ("isCalibrated", bool (probe->isCalibrated));
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

void NeuropixInterface::showDamagedShankWarning()
{
    if (probe == nullptr)
        return;

    if (probe->isSurveyModeActive())
        return;

    String message = "One or more selected electrodes for " + probe->getName() + " are located on a shank that may be damaged. "
                     "Although data acquisition can proceed, there is no guarantee these electrodes will be selected as intended.";

    if (probe->type == ProbeType::NP2_4)
    {
        message += "\n\nIf possible, please select electrodes on shanks that appear yellow in the probe display.";
    }

    MessageManager::callAsync ([message]()
                               { AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon,
                                                                   "Warning: damaged shank detected.",
                                                                   message); });
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