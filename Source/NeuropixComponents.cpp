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

#include "NeuropixComponents.h"
#include "Basestations/PxiBasestation.h"

size_t FirmwareUpdater::totalFirmwareBytes = 0;
size_t FirmwareUpdater::completedFirmwareBytes = 0;
FirmwareUpdater* FirmwareUpdater::currentThread = nullptr;

Probe::Probe (Basestation* bs_, Headstage* hs_, Flex* fl_, int dock_)
    : DataSource (bs_),
      headstage (hs_),
      flex (fl_),
      dock (dock_),
      isValid (true),
      isCalibrated (false),
      calibrationWarningShown (false)
{
    for (int i = 0; i < 12 * MAXPACKETS; i++)
        timestamp_s[i] = -1.0;

    sourceType = DataSourceType::PROBE;

    for (int i = 0; i < 384; i++)
    {
        for (int j = 0; j < 100; j++)
        {
            ap_offsets[i][j] = 0;
            lfp_offsets[i][j] = 0;
        }
    }
}

bool Probe::canContinueAfterProbeConfiguration (Neuropixels::NP_ErrorCode result, const String& operation)
{
    if (result == Neuropixels::SUCCESS)
        return true;

    if (result == Neuropixels::PROBE_DEGRADATION_ERROR)
    {
        LOGC ("Probe degradation detected during ", operation, " on slot ", basestation->slot,
              " port ", headstage->port, " dock ", dock, "; continuing with usable shanks.");
        return true;
    }

    if (result == Neuropixels::PROBE_CONFIGURATION_FAILURE)
    {
        LOGE ("Probe configuration failed during ", operation, " on slot ", basestation->slot,
              " port ", headstage->port, " dock ", dock, ".");
    }
    else
    {
        LOGE ("Probe initialization terminated during ", operation, " on slot ", basestation->slot,
              " port ", headstage->port, " dock ", dock, ": ", Neuropixels::np_getErrorMessage (result));
    }

    isEnabled = false;
    setStatus (SourceStatus::DISABLED);
    return false;
}

Neuropixels::NP_ErrorCode Probe::runConfigurationBistAndRestore (uint8_t* shankOkMask)
{
    uint8_t returnedShankOkMask = 0;
    const auto bistResult = Neuropixels::np_bistConfig (basestation->slot,
                                                        headstage->port,
                                                        dock,
                                                        &returnedShankOkMask);

    if (bistResult == Neuropixels::SUCCESS)
    {
        LOGD ("Configuration BIST passed with shank mask: ", (int) returnedShankOkMask);
    }
    else if (bistResult == Neuropixels::PROBE_DEGRADATION_ERROR)
    {
        LOGC ("Configuration BIST detected probe degradation; shank mask: ", (int) returnedShankOkMask);
    }
    else if (bistResult == Neuropixels::PROBE_CONFIGURATION_FAILURE)
    {
        LOGE ("Configuration BIST detected base/configuration failure; shank mask: ", (int) returnedShankOkMask);
    }
    else
    {
        LOGE ("Configuration BIST terminated: ", Neuropixels::np_getErrorMessage (bistResult));
    }

    if (shankOkMask != nullptr
        && (bistResult == Neuropixels::SUCCESS
            || bistResult == Neuropixels::PROBE_DEGRADATION_ERROR
            || bistResult == Neuropixels::PROBE_CONFIGURATION_FAILURE))
    {
        *shankOkMask = returnedShankOkMask;
    }

    const auto restoreResult = checkError (Neuropixels::np_writeProbeConfiguration (basestation->slot,
                                                                                     headstage->port,
                                                                                     dock,
                                                                                     false),
                                           "restoreProbeConfigurationAfterBist");

    return restoreResult == Neuropixels::SUCCESS ? bistResult : restoreResult;
}

void Probe::logDegradedShanks (uint8_t shankOkMask)
{
    if (probeMetadata.shank_count > 1)
    {
        String degradedShanks;

        for (int shank = 0; shank < probeMetadata.shank_count; shank++)
        {
            if (((shankOkMask >> shank) & 1) == 0)
                degradedShanks += (degradedShanks.isEmpty() ? "" : ", ") + String (shank + 1);
        }

        LOGC ("Shank(s) appear to be broken: ", degradedShanks);
    }
    else
    {
        LOGC ("Probe degradation detected; shank appears to be broken.");
    }
}

void Probe::updateOffsets (float* samples, int64 timestamp, bool isApBand)
{
    if (isApBand && timestamp > 30000 * 5) // wait for amplifiers to settle
    {
        if (ap_offset_counter < 99)
        {
            for (int i = 0; i < 384; i++)
            {
                ap_offsets[i][ap_offset_counter + 1] = samples[i];
            }

            ap_offset_counter++;
        }
        else if (ap_offset_counter == 99)
        {
            for (int i = 0; i < 384; i++)
            {
                for (int j = 1; j < 100; j++)
                {
                    ap_offsets[i][0] += ap_offsets[i][j];
                }

                ap_offsets[i][0] /= 99;
            }

            ap_offset_counter++;
        }
    }
    else if (! isApBand && timestamp > 2500 * 5) // wait for amplifiers to settle
    {
        if (lfp_offset_counter < 99)
        {
            for (int i = 0; i < 384; i++)
            {
                lfp_offsets[i][lfp_offset_counter + 1] = samples[i];
            }

            lfp_offset_counter++;
        }
        else if (lfp_offset_counter == 99)
        {
            for (int i = 0; i < 384; i++)
            {
                for (int j = 1; j < 100; j++)
                {
                    lfp_offsets[i][0] += lfp_offsets[i][j];
                }

                lfp_offsets[i][0] /= 99;
            }

            lfp_offset_counter++;
        }
    }
}

void Probe::updateNamingScheme (ProbeNameConfig::NamingScheme scheme)
{
    namingScheme = scheme;

    switch (scheme)
    {
        case ProbeNameConfig::AUTO_NAMING:
            displayName = customName.automatic;
            break;
        case ProbeNameConfig::STREAM_INDICES:
            displayName = customName.streamSpecific;
            break;
        case ProbeNameConfig::PORT_SPECIFIC_NAMING:
            displayName = basestation->getCustomPortName (headstage->port, dock);
            break;
        case ProbeNameConfig::PROBE_SPECIFIC_NAMING:
            displayName = customName.probeSpecific;
            break;
    }
}

void Probe::refreshActivityViewMapping()
{

    if (settings.selectedChannel.size() != channel_count
        || settings.selectedElectrode.size() != channel_count
        || (type == ProbeType::QUAD_BASE && settings.selectedShank.size() != channel_count))
        return;

    std::vector<int> mapping;
    mapping.resize ((size_t) channel_count);

    for (int i = 0; i < channel_count; ++i)
    {
        int channelIndex = settings.selectedChannel[i];

        if (type == ProbeType::QUAD_BASE)
            channelIndex += settings.selectedShank[i] * 384;

        int selectedElectrode = settings.selectedElectrode[i];
        //LOGD ("  Channel ", channelIndex, " mapped to electrode ", selectedElectrode);
        mapping[channelIndex] = selectedElectrode;
    }

    if (apView)
        apView->setChannelToElectrodeMapping (mapping);

    if (lfpView)
        lfpView->setChannelToElectrodeMapping (mapping);
}

Array<int> Probe::getHalfBankOverlapSelection (const String& config, int electrodeOffset)
{
    Array<int> selection;
    const int bankIndex = config[config.indexOf ("Bank ") + 5] - 'A';
    const int firstElectrode = electrodeOffset + bankIndex * 384 + 192;

    for (int electrode = firstElectrode; electrode < firstElectrode + 384; ++electrode)
        selection.add (electrode);

    return selection;
}

FirmwareUpdater::FirmwareFileSelectionComponent::FirmwareFileSelectionComponent (const String& expectedBsFilename_,
                                                                                  const String& expectedBscFilename_)
    : expectedBsFilename (expectedBsFilename_),
      expectedBscFilename (expectedBscFilename_)
{
    configureFileLabel (bsFileLabel, "No BS firmware file selected");
    configureFileLabel (bscFileLabel, "No BSC firmware file selected");

    bsSelectButton.setButtonText ("Select BS File...");
    bsSelectButton.onClick = [this] { selectFirmwareFile ("BS", expectedBsFilename, bsFirmwareFile, bsFileLabel); };
    addAndMakeVisible (bsSelectButton);

    bscSelectButton.setButtonText ("Select BSC File...");
    bscSelectButton.onClick = [this] { selectFirmwareFile ("BSC", expectedBscFilename, bscFirmwareFile, bscFileLabel); };
    addAndMakeVisible (bscSelectButton);

    continueButton.setButtonText ("Continue");
    continueButton.setEnabled (false);
    continueButton.onClick = [this] { closeDialog (1); };
    addAndMakeVisible (continueButton);

    cancelButton.setButtonText ("Cancel");
    cancelButton.onClick = [this] { closeDialog (0); };
    addAndMakeVisible (cancelButton);
}

void FirmwareUpdater::FirmwareFileSelectionComponent::paint (Graphics& g)
{
    g.fillAll (findColour (ThemeColours::componentBackground));
    g.setColour (findColour (ThemeColours::defaultText));
    g.setFont (FontOptions ("Inter", "Medium", 16.0f));
    g.drawText ("BS firmware (" + expectedBsFilename + ")", 20, 16, getWidth() - 220, 28, Justification::centredLeft);
    g.drawText ("BSC firmware (" + expectedBscFilename + ")", 20, 96, getWidth() - 220, 28, Justification::centredLeft);
}

void FirmwareUpdater::FirmwareFileSelectionComponent::resized()
{
    bsSelectButton.setBounds (getWidth() - 190, 16, 170, 24);
    bsFileLabel.setBounds (20, 50, getWidth() - 40, 28);
    bscSelectButton.setBounds (getWidth() - 190, 96, 170, 24);
    bscFileLabel.setBounds (20, 130, getWidth() - 40, 28);
    continueButton.setBounds (getWidth() / 2 - 120, getHeight() - 48, 110, 30);
    cancelButton.setBounds (getWidth() / 2 + 10, getHeight() - 48, 100, 30);
}

File FirmwareUpdater::FirmwareFileSelectionComponent::getBsFirmwareFile() const
{
    return bsFirmwareFile;
}

File FirmwareUpdater::FirmwareFileSelectionComponent::getBscFirmwareFile() const
{
    return bscFirmwareFile;
}

void FirmwareUpdater::FirmwareFileSelectionComponent::configureFileLabel (Label& label, const String& text)
{
    label.setText (text, dontSendNotification);
    label.setFont (FontOptions ("Inter", "Regular", 14.0f));
    label.setJustificationType (Justification::centredLeft);
    label.setMinimumHorizontalScale (0.7f);
    addAndMakeVisible (label);
}

void FirmwareUpdater::FirmwareFileSelectionComponent::selectFirmwareFile (const String& componentName,
                                                                           const String& expectedFilename,
                                                                           File& selectedFile,
                                                                           Label& fileLabel)
{
    FileChooser fileChooser ("Select the " + componentName + " firmware file (" + expectedFilename + ").",
                             File(),
                             expectedFilename);

    if (! fileChooser.browseForFileToOpen())
        return;

    const File result = fileChooser.getResult();
    if (result.getFileName() != expectedFilename)
    {
        AlertWindow::showMessageBox (AlertWindow::WarningIcon,
                                     "Incorrect firmware file",
                                     "The selected " + componentName + " firmware file must be named " + expectedFilename + ".",
                                     "OK");
        return;
    }

    selectedFile = result;
    fileLabel.setText (result.getFullPathName(), dontSendNotification);
    fileLabel.setTooltip (result.getFullPathName());
    continueButton.setEnabled (bsFirmwareFile.existsAsFile() && bscFirmwareFile.existsAsFile());
}

void FirmwareUpdater::FirmwareFileSelectionComponent::closeDialog (int result)
{
    if (DialogWindow* dialogWindow = findParentComponentOfClass<DialogWindow>())
        dialogWindow->exitModalState (result);
}

FirmwareUpdater::FirmwareUpdater (Basestation* basestation_)
    : ThreadWithProgressWindow ("Firmware Update...", true, false),
      basestation (basestation_)
{
    FirmwareUpdater::currentThread = this;
    FirmwareUpdater::totalFirmwareBytes = 0;
    FirmwareUpdater::completedFirmwareBytes = 0;
    setStatusMessage ("Preparing firmware update...");

    runThread();
    FirmwareUpdater::currentThread = nullptr;

    if (updateResult != Neuropixels::SUCCESS
        && (basestation->type == BasestationType::PXI || basestation->type == BasestationType::OPTO))
    {
        const String automaticFailure = failedOperation + " failed: " + String (Neuropixels::np_getErrorMessage (updateResult));
        const bool shouldTryManualUpdate = AlertWindow::showOkCancelBox (
            AlertWindow::WarningIcon,
            "Automatic firmware update failed",
            automaticFailure + "\n\nWould you like to select BS and BSC firmware package files manually?",
            "Select Firmware Files",
            "Cancel");

        if (shouldTryManualUpdate)
        {
            const String expectedBsFilename = basestation->type == BasestationType::OPTO
                                                ? OPTO_BS_FIRMWARE_FILENAME
                                                : BS_FIRMWARE_FILENAME;
            const String expectedBscFilename = basestation->type == BasestationType::OPTO
                                                 ? OPTO_BSC_FIRMWARE_FILENAME
                                                 : BSC_FIRMWARE_FILENAME;

            if (selectManualFirmwareFiles (expectedBsFilename, expectedBscFilename))
            {
                manualUpdate = true;
                updateResult = Neuropixels::SUCCESS;
                failedOperation.clear();
                totalFirmwareBytes = 0;
                completedFirmwareBytes = 0;
                setStatusMessage ("Preparing manual firmware update...");
                FirmwareUpdater::currentThread = this;
                runThread();
                FirmwareUpdater::currentThread = nullptr;
            }
        }
    }

    if (updateResult == Neuropixels::SUCCESS)
    {
        AlertWindow::showMessageBoxAsync (AlertWindow::InfoIcon,
                                          "Successful firmware update",
                                          "The basestation and basestation connect board firmware were updated successfully. "
                                          "Please restart your computer and power cycle the PXI chassis for the changes to take effect."
                                          "\n\nIf there are multiple basestations in the PXI chassis, please update the firmware on each of them before restarting your computer.");
    }
    else
    {
        const String message = failedOperation + " failed: " + String (Neuropixels::np_getErrorMessage (updateResult));
        LOGE (message);
        AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon, "Firmware update failed", message);
    }
}

bool FirmwareUpdater::selectManualFirmwareFiles (const String& expectedBsFilename, const String& expectedBscFilename)
{
    auto selectionComponent = std::make_unique<FirmwareFileSelectionComponent> (expectedBsFilename, expectedBscFilename);
    DialogWindow::LaunchOptions options;
    options.content.setNonOwned (selectionComponent.get());
    options.content->setSize (680, 230);
    options.dialogTitle = "Select Firmware Files";
    options.dialogBackgroundColour = selectionComponent->findColour (ThemeColours::componentBackground);
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = false;
    options.resizable = false;

    if (options.runModal() != 1)
        return false;

    bsFirmwareFile = selectionComponent->getBsFirmwareFile();
    bscFirmwareFile = selectionComponent->getBscFirmwareFile();
    return true;
}

void FirmwareUpdater::run()
{
    if (basestation->type == BasestationType::SIMULATED)
    {
        setStatusMessage ("Updating BS firmware (1 of 2)...");
        for (int step = 0; step < 20; step++)
        {
            setProgress (0.025 * step);
            std::this_thread::sleep_for (std::chrono::milliseconds (100));
        }

        setStatusMessage ("Updating BSC firmware (2 of 2)...");
        for (int step = 0; step < 20; step++)
        {
            setProgress (0.5 + 0.025 * step);
            std::this_thread::sleep_for (std::chrono::milliseconds (100));
        }

        setProgress (1.0);
        return;
    }

    size_t bsFirmwareBytes = 0;
    size_t bscFirmwareBytes = 0;

    if (manualUpdate)
        updateResult = Neuropixels::np_getFirmwarePackageSize (bsFirmwareFile.getFullPathName().toRawUTF8(), &bsFirmwareBytes);
    else
        updateResult = Neuropixels::np_bs_getFirmwareSize (basestation->slot, &bsFirmwareBytes);

    if (updateResult != Neuropixels::SUCCESS)
    {
        failedOperation = manualUpdate ? "Reading the selected BS firmware package size"
                                       : "Reading the built-in BS firmware size";
        return;
    }

    if (manualUpdate)
        updateResult = Neuropixels::np_getFirmwarePackageSize (bscFirmwareFile.getFullPathName().toRawUTF8(), &bscFirmwareBytes);
    else
        updateResult = Neuropixels::np_bsc_getFirmwareSize (basestation->slot, &bscFirmwareBytes);

    if (updateResult != Neuropixels::SUCCESS)
    {
        failedOperation = manualUpdate ? "Reading the selected BSC firmware package size"
                                       : "Reading the built-in BSC firmware size";
        return;
    }

    totalFirmwareBytes = bsFirmwareBytes + bscFirmwareBytes;
    if (totalFirmwareBytes == 0)
    {
        updateResult = Neuropixels::FAILED;
        failedOperation = "Reading the built-in firmware sizes";
        return;
    }

    setStatusMessage ("Updating BS firmware (1 of 2)...");
    completedFirmwareBytes = 0;
    if (manualUpdate)
        updateResult = Neuropixels::np_bs_updateFirmware (basestation->slot,
                                                          bsFirmwareFile.getFullPathName().toRawUTF8(),
                                                          firmwareUpdateCallback,
                                                          false);
    else
        updateResult = Neuropixels::np_bs_resetFirmware (basestation->slot, firmwareUpdateCallback);

    if (updateResult != Neuropixels::SUCCESS)
    {
        failedOperation = "BS firmware update";
        return;
    }

    setProgress (double (bsFirmwareBytes) / double (totalFirmwareBytes));
    std::this_thread::sleep_for (std::chrono::milliseconds (1000)); // wait for the BS to reset before updating the BSC firmware
    setStatusMessage ("Updating BSC firmware (2 of 2)...");
    completedFirmwareBytes = bsFirmwareBytes;
    if (manualUpdate)
        updateResult = Neuropixels::np_bsc_updateFirmware (basestation->slot,
                                                            bscFirmwareFile.getFullPathName().toRawUTF8(),
                                                            firmwareUpdateCallback,
                                                            false);
    else
        updateResult = Neuropixels::np_bsc_resetFirmware (basestation->slot, firmwareUpdateCallback);

    if (updateResult != Neuropixels::SUCCESS)
    {
        failedOperation = "BSC firmware update";
        return;
    }

    setProgress (1.0);
}

void Basestation::updateFirmware()
{
    std::unique_ptr<FirmwareUpdater> firmwareUpdater = std::make_unique<FirmwareUpdater> ((Basestation*) this);
}
