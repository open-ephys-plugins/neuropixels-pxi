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

    if (updateResult == Neuropixels::SUCCESS)
    {
        AlertWindow::showMessageBoxAsync (AlertWindow::InfoIcon,
                                          "Successful firmware update",
                                          "The basestation and basestation connect board firmware were updated successfully. "
                                          "Please restart your computer and power cycle the PXI chassis for the changes to take effect.");
    }
    else
    {
        const String message = failedOperation + " failed: " + String (Neuropixels::np_getErrorMessage (updateResult));
        LOGE (message);
        AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon, "Firmware update failed", message);
    }
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

    updateResult = Neuropixels::np_bs_getFirmwareSize (basestation->slot, &bsFirmwareBytes);
    if (updateResult != Neuropixels::SUCCESS)
    {
        failedOperation = "Reading the built-in BS firmware size";
        return;
    }

    updateResult = Neuropixels::np_bsc_getFirmwareSize (basestation->slot, &bscFirmwareBytes);
    if (updateResult != Neuropixels::SUCCESS)
    {
        failedOperation = "Reading the built-in BSC firmware size";
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
