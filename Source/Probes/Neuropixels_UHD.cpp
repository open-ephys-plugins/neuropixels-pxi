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

#include "Neuropixels_UHD.h"
#include "Geometry.h"

#include "../NeuropixThread.h"

#define MAXLEN 50

void Neuropixels_UHD::getInfo()
{
    errorCode = checkError (Neuropixels::getProbeHardwareID (headstage->basestation->slot,
                                                             headstage->port,
                                                             dock,
                                                             &info.hardwareID),
                            "getProbeHardwareID");

    info.version = String (info.hardwareID.version_Major)
                   + "." + String (info.hardwareID.version_Minor);
    info.part_number = String (info.hardwareID.ProductNumber);
    info.serial_number = info.hardwareID.SerialNumber;
}

Neuropixels_UHD::Neuropixels_UHD (Basestation* bs, Headstage* hs, Flex* fl) : Probe (bs, hs, fl, 1)
{
    getInfo();

    createElectrodeConfigurations();

    setStatus (SourceStatus::DISCONNECTED);

    customName.probeSpecific = String (info.serial_number);

    if (Geometry::forPartNumber (info.part_number, electrodeMetadata, probeMetadata))
    {
        name = probeMetadata.name;
        type = probeMetadata.type;
        switchable = probeMetadata.switchable;

        settings.probeType = type;

        settings.probe = this;
        settings.availableBanks = probeMetadata.availableBanks;

        settings.apGainIndex = 3;
        settings.lfpGainIndex = 2;
        settings.referenceIndex = 0;
        settings.apFilterState = true;

        channel_count = 384;
        lfp_sample_rate = 2500.0f;
        ap_sample_rate = 30000.0f;

        for (int i = 0; i < 16; i++)
            availableElectrodeConfigurations.add ("8 x 48: Bank " + String (i));
        availableElectrodeConfigurations.add ("1 x 384: Tip Half");
        availableElectrodeConfigurations.add ("1 x 384: Base Half");
        availableElectrodeConfigurations.add ("2 x 192");
        availableElectrodeConfigurations.add ("4 x 96");
        availableElectrodeConfigurations.add ("2 x 2 x 96");

        settings.availableElectrodeConfigurations = availableElectrodeConfigurations;

        settings.electrodeConfigurationIndex = 1; // 8 x 48: Bank 0

        for (int i = 0; i < channel_count; i++)
        {
            settings.selectedBank.add (Bank::A);
            settings.selectedChannel.add (i);
            settings.selectedShank.add (0);
            settings.selectedElectrode.add (i);
        }

        settings.availableApGains.add (50.0f);
        settings.availableApGains.add (125.0f);
        settings.availableApGains.add (250.0f);
        settings.availableApGains.add (500.0f);
        settings.availableApGains.add (1000.0f);
        settings.availableApGains.add (1500.0f);
        settings.availableApGains.add (2000.0f);
        settings.availableApGains.add (3000.0f);

        settings.availableLfpGains.add (50.0f);
        settings.availableLfpGains.add (125.0f);
        settings.availableLfpGains.add (250.0f);
        settings.availableLfpGains.add (500.0f);
        settings.availableLfpGains.add (1000.0f);
        settings.availableLfpGains.add (1500.0f);
        settings.availableLfpGains.add (2000.0f);
        settings.availableLfpGains.add (3000.0f);

        settings.availableReferences.add ("Ext");
        settings.availableReferences.add ("Tip");

        open();
    }
    else
    {
        isValid = false;
    }
}

bool Neuropixels_UHD::open()
{
    LOGC ("Opening probe...");
    errorCode = Neuropixels::openProbe (basestation->slot, headstage->port, dock);

    LOGC ("openProbe: slot: ", basestation->slot, " port: ", headstage->port, " dock: ", dock, " errorCode: ", errorCode);

    ap_timestamp = 0;
    lfp_timestamp = 0;
    eventCode = 0;

    apView = std::make_unique<ActivityView> (384, 3000);
    lfpView = std::make_unique<ActivityView> (384, 250);

    return errorCode == Neuropixels::SUCCESS;
}

bool Neuropixels_UHD::close()
{
    errorCode = Neuropixels::closeProbe (basestation->slot, headstage->port, dock);
    LOGD ("closeProbe: slot: ", basestation->slot, " port: ", headstage->port, " dock: ", dock, " errorCode: ", errorCode);

    return errorCode == Neuropixels::SUCCESS;
}

void Neuropixels_UHD::initialize (bool signalChainIsLoading)
{
    errorCode = Neuropixels::init (basestation->slot, headstage->port, dock);
    LOGD ("Neuropixels::init: errorCode: ", errorCode);

    errorCode = Neuropixels::setOPMODE (basestation->slot, headstage->port, dock, Neuropixels::RECORDING);
    LOGD ("Neuropixels::setOPMODE: errorCode: ", errorCode);

    errorCode = Neuropixels::setHSLed (basestation->slot, headstage->port, false);
    LOGDD ("Neuropixels::setHSLed: errorCode: ", errorCode);
}

void Neuropixels_UHD::calibrate()
{
    LOGD ("Calibrating probe...");

    File baseDirectory = File::getSpecialLocation (File::currentExecutableFile).getParentDirectory();
    File calibrationDirectory = baseDirectory.getChildFile ("CalibrationInfo");
    File probeDirectory = calibrationDirectory.getChildFile (String (info.serial_number));

    if (! probeDirectory.exists())
    {
        // check alternate location
        baseDirectory = CoreServices::getSavedStateDirectory();
        calibrationDirectory = baseDirectory.getChildFile ("CalibrationInfo");
        probeDirectory = calibrationDirectory.getChildFile (String (info.serial_number));
    }

    if (! probeDirectory.exists())
    {
        LOGD ("!!! Calibration files not found for probe serial number: ", info.serial_number);
        return;
    }

    String adcFile = probeDirectory.getChildFile (String (info.serial_number) + "_ADCCalibration.csv").getFullPathName();
    String gainFile = probeDirectory.getChildFile (String (info.serial_number) + "_gainCalValues.csv").getFullPathName();
    LOGDD ("ADC file: ", adcFile);

    errorCode = Neuropixels::setADCCalibration (basestation->slot, headstage->port, adcFile.toRawUTF8());

    if (errorCode == 0)
    {
        LOGD ("Successful ADC calibration.");
    }
    else
    {
        LOGD ("!!! Unsuccessful ADC calibration, failed with error code: ", errorCode);
        return;
    }

    LOGDD ("Gain file: ", gainFile);

    errorCode = Neuropixels::setGainCalibration (basestation->slot, headstage->port, dock, gainFile.toRawUTF8());

    if (errorCode == 0)
    {
        LOGD ("Successful gain calibration.");
    }
    else
    {
        LOGD ("!!! Unsuccessful gain calibration, failed with error code: ", errorCode);
        return;
    }

    isCalibrated = true;
}

void Neuropixels_UHD::printSettings()
{
    int apGainIndex;
    int lfpGainIndex;

    Neuropixels::getGain (basestation->slot, headstage->port, dock, 32, &apGainIndex, &lfpGainIndex);

    LOGD ("Current settings for probe on slot: ", basestation->slot, " port: ", headstage->port, " dock: ", dock, " AP=", settings.availableApGains[apGainIndex], " LFP=", settings.availableLfpGains[lfpGainIndex], " REF=", settings.availableReferences[settings.referenceIndex]);
}

void Neuropixels_UHD::selectElectrodes()
{
    if (settings.electrodeConfigurationIndex >= 0 && switchable)
    {
        selectElectrodeConfiguration (availableElectrodeConfigurations[settings.electrodeConfigurationIndex]);
    }
}

Array<int> Neuropixels_UHD::selectElectrodeConfiguration (String electrodeConfiguration)
{
    Array<int> defaultReturnValue;

    for (int i = 0; i < 384; i++)
        defaultReturnValue.add (i);

    /*
	*           ---------BANK 0--------|---------BANK 1--------
	*         /| 0 | 4 | 8 | 12| 16| 20| 2 | 6 | 10| 14| 18| 22|         |
	* PROBE  / | 2 | 6 | 10| 14| 18| 22| 0 | 4 | 8 | 12| 16| 20| ... x 8 | PROBE
	*  TIP   \ | 3 | 7 | 11| 15| 19| 23| 1 | 5 | 9 | 13| 17| 21|         | BASE
	*         \| 1 | 5 | 9 | 13| 17| 21| 3 | 7 | 11| 15| 19| 23|         |
	*           --------24 GROUPS------|-------24 GROUPS-------
	*/

    int banksPerProbe = 16;
    int groupsPerBank = 24;
    int groupsPerBankColumn = 6;
    int electrodesPerGroup = 16;

    Neuropixels::NP_ErrorCode ec;

    int index = availableElectrodeConfigurations.indexOf (electrodeConfiguration);

    if (index == -1)
        return defaultReturnValue;

    settings.selectedElectrode.clear();

    for (int i = 0; i < 384; i++)
        settings.selectedElectrode.add (electrodeConfigurations[index]->getUnchecked (i));

    if (index < banksPerProbe || index > 17)
    {
        LOGC ("Selecting column pattern: ALL");
        // select columnar configuration
        checkError (Neuropixels::selectColumnPattern (
                        basestation->slot,
                        headstage->port,
                        dock,
                        Neuropixels::ALL
                        ),
                    "selectColumnPattern - ALL");
    }
    else
    {
        LOGC ("Selecting column pattern: OUTER");
        // select columnar configuration
        checkError (Neuropixels::selectColumnPattern (
                        basestation->slot,
                        headstage->port,
                        dock,
                        Neuropixels::OUTER

                        ),
                    "selectColumnPattern - OUTER");
    }

    // Select all groups in a particular bank
    if (index < banksPerProbe)
    {
        LOGC ("Selecting bank: ", index);

        // Select all groups at this bank index
        for (int group = 0; group < groupsPerBank; group++)
            checkError (Neuropixels::selectElectrodeGroup (
                            basestation->slot, // slot
                            headstage->port, // port
                            dock, // dock
                            group, // group number
                            index), // bank index
                        "selectElectrodeGroup");

        return *electrodeConfigurations[index];
    }
    else if (index == 17) // 2 x 192
    {
        LOGC ("Selecting 2 x 192 configuration");

        // Select G2, G6, G10, G14, G18, G22 from bank 0
        for (int group = 2; group < groupsPerBank; group += 4)
            checkError (Neuropixels::selectElectrodeGroup (
                            basestation->slot, // slot
                            headstage->port, // port
                            dock, // dock
                            group, // group number
                            0), // bank index
                        "selectElectrodeGroup");

        // Select G0, G4, G8, G12, G16, G20 from bank 1
        for (int group = 0; group < groupsPerBank; group += 4)
            checkError (Neuropixels::selectElectrodeGroup (
                            basestation->slot, // slot
                            headstage->port, // port
                            dock, // dock
                            group, // group number
                            1), // bank index
                        "selectElectrodeGroup");

        // Select G3, G7, G11, G15, G19, G23 from bank 2
        for (int group = 3; group < groupsPerBank; group += 4)
            checkError (Neuropixels::selectElectrodeGroup (
                            basestation->slot, // slot
                            headstage->port, // port
                            dock, // dock
                            group, // group number
                            2), // bank index
                        "selectElectrodeGroup");

        // Select G1, G5, G9, G13, G17, G21 from bank 3
        for (int group = 1; group < groupsPerBank; group += 4)
            checkError (Neuropixels::selectElectrodeGroup (
                            basestation->slot, // slot
                            headstage->port, // port
                            dock, // dock
                            group, // group number
                            3), // bank index
                        "selectElectrodeGroup");

        return *electrodeConfigurations[index];
    }
    else if (index == 18) // 4 x 96
    {
        // Select G2, G6, G10, G14, G18, G22 from bank 0
        for (int group = 2; group < groupsPerBank; group += 4)
            checkError (Neuropixels::selectElectrodeGroup (
                            basestation->slot, // slot
                            headstage->port, // port
                            dock, // dock
                            group, // group number
                            0), // bank index
                        "selectElectrodeGroup");

        // Select G3, G7, G11, G15, G19, G23 from bank 0
        for (int group = 3; group < groupsPerBank; group += 4)
            checkError (Neuropixels::selectElectrodeGroup (
                            basestation->slot, // slot
                            headstage->port, // port
                            dock, // dock
                            group, // group number
                            0), // bank index
                        "selectElectrodeGroup");

        // Select G0, G4, G8, G12, G16, G20 from bank 1
        for (int group = 0; group < groupsPerBank; group += 4)
            checkError (Neuropixels::selectElectrodeGroup (
                            basestation->slot, // slot
                            headstage->port, // port
                            dock, // dock
                            group, // group number
                            1), // bank index
                        "selectElectrodeGroup");

        // Select G1, G5, G9, G13, G17, G21 from bank 1
        for (int group = 1; group < groupsPerBank; group += 4)
            checkError (Neuropixels::selectElectrodeGroup (
                            basestation->slot, // slot
                            headstage->port, // port
                            dock, // dock
                            group, // group number
                            1), // bank index
                        "selectElectrodeGroup");

        return *electrodeConfigurations[index];
    }
    else if (index == 19) // 2 x 2 x 96
    {
        // Select G2, G6, G10, G14, G18, G22 from bank 1
        for (int group = 2; group < groupsPerBank; group += 4)
            checkError (Neuropixels::selectElectrodeGroup (
                            basestation->slot, // slot
                            headstage->port, // port
                            dock, // dock
                            group, // group number
                            1), // bank index
                        "selectElectrodeGroup");

        // Select G3, G7, G11, G15, G19, G23 from bank 1
        for (int group = 3; group < groupsPerBank; group += 4)
            checkError (Neuropixels::selectElectrodeGroup (
                            basestation->slot, // slot
                            headstage->port, // port
                            dock, // dock
                            group, // group number
                            1), // bank index
                        "selectElectrodeGroup");

        // Select G0, G4, G8, G12, G16, G20 from bank 0
        for (int group = 0; group < groupsPerBank; group += 4)
            checkError (Neuropixels::selectElectrodeGroup (
                            basestation->slot, // slot
                            headstage->port, // port
                            dock, // dock
                            group, // group number
                            0), // bank index
                        "selectElectrodeGroup");

        // Select G1, G5, G9, G13, G17, G21 from bank 0
        for (int group = 1; group < groupsPerBank; group += 4)
            checkError (Neuropixels::selectElectrodeGroup (
                            basestation->slot, // slot
                            headstage->port, // port
                            dock, // dock
                            group, // group number
                            0), // bank index
                        "selectElectrodeGroup");

        return *electrodeConfigurations[index];
    }

    // This code is only reached for 1 x 384 configurations!!

    // Select a subset of groups in multiple banks to span half of the probe
    //   0 = Select groups from middle to tip of probe (Banks 0-7)
    //   1 = Select groups from middle to base of probe (Banks 8-15)
    int verticalConfiguration = index - banksPerProbe;

    int offset; //controls offset of bank index

    if (verticalConfiguration == 0)
    {
        offset = 0;
    }
    else
    {
        offset = 8;
    }
    // Vertical line in banks 0-7
    for (int bank = offset; bank < offset + 4; bank++)
    {
        // Direct vs. Group Cross numbering based on bank index
        int start_group = bank % 4 < 2 ? 0 : 1;
        start_group = bank % 2 == 0 ? start_group + 2 : start_group;

        LOGC ("Selecting bank: ", bank, ", start group: ", start_group);

        Neuropixels::electrodebanks_t bank1 = (Neuropixels::electrodebanks_t) (1 << bank);
        Neuropixels::electrodebanks_t bank2 = (Neuropixels::electrodebanks_t) (1 << (bank + 4));

        for (int group = 0; group < groupsPerBankColumn; group++)
        {
            int G = start_group + group * 4;

            checkError (Neuropixels::selectElectrodeGroupMask (
                            basestation->slot, // slot
                            headstage->port, // port
                            dock, // dock
                            G, // group number
                            (Neuropixels::electrodebanks_t) (bank1 + bank2)),
                        "selectElectrodeGroupMask"); // bank mask
        }
    }

    return *electrodeConfigurations[index];
}

void Neuropixels_UHD::setApFilterState()
{
    for (int channel = 0; channel < 384; channel++)
        checkError(Neuropixels::setAPCornerFrequency (basestation->slot,
                                           headstage->port,
                                           dock,
                                           channel,
                                           ! settings.apFilterState), // true if disabled
            "setAPCornerFrequency");
}

void Neuropixels_UHD::setAllGains()
{
    LOGDD ("Setting gain AP=", settings.apGainIndex, " LFP=", settings.lfpGainIndex);

    for (int channel = 0; channel < 384; channel++)
    {
        checkError(Neuropixels::setGain (basestation->slot, 
            headstage->port, 
            dock, 
            channel, 
            settings.apGainIndex, 
            settings.lfpGainIndex), "setGain");
    }
}

void Neuropixels_UHD::setAllReferences()
{
    Neuropixels::channelreference_t refId;
    int refElectrodeBank = 0;

    switch (settings.referenceIndex)
    {
        case 0:
            refId = Neuropixels::EXT_REF;
            break;
        case 1:
            refId = Neuropixels::TIP_REF;
            break;
        case 2:
            refId = Neuropixels::INT_REF;
            break;
        case 3:
            refId = Neuropixels::INT_REF;
            refElectrodeBank = 1;
            break;
        case 4:
            refId = Neuropixels::INT_REF;
            refElectrodeBank = 2;
            break;
        default:
            refId = Neuropixels::EXT_REF;
    }

    for (int channel = 0; channel < 384; channel++)
        checkError(Neuropixels::setReference (basestation->slot, headstage->port, dock, channel, 0, refId, refElectrodeBank), "setReference");
}

void Neuropixels_UHD::writeConfiguration()
{
    if (basestation->isBusy())
        basestation->waitForThreadToExit();

    errorCode = checkError(Neuropixels::writeProbeConfiguration (basestation->slot, headstage->port, dock, false), "writeProbeConfiguration");

    if (errorCode == Neuropixels::SUCCESS)
    {
        LOGD ("Succesfully wrote probe configuration");
        printSettings();
    }
    else
    {
        LOGD ("!!! FAILED TO WRITE PROBE CONFIGURATION !!! Slot: ", basestation->slot, " port: ", headstage->port, " error code: ", errorCode);
    }
}

void Neuropixels_UHD::startAcquisition()
{
    ap_timestamp = 0;
    lfp_timestamp = 0;

    apBuffer->clear();
    lfpBuffer->clear();

    apView->reset();
    lfpView->reset();

    SKIP = sendSync ? 385 : 384;

    LOGD ("  Starting thread.");
    startThread();
}

void Neuropixels_UHD::stopAcquisition()
{
    LOGC ("Probe stopping thread.");
    signalThreadShouldExit();
}

void Neuropixels_UHD::run()
{
    while (! threadShouldExit())
    {
        int count = MAXPACKETS;

        errorCode = Neuropixels::readElectrodeData (
            basestation->slot,
            headstage->port,
            dock,
            &packet[0],
            &count,
            count);

        if (errorCode == Neuropixels::SUCCESS && count > 0)
        {
            for (int packetNum = 0; packetNum < count; packetNum++)
            {
                for (int i = 0; i < 12; i++)
                {
                    eventCode = packet[packetNum].Status[i] >> 6; // AUX_IO<0:13>

                    if (invertSyncLine)
                        eventCode = ~eventCode;

                    uint32_t npx_timestamp = packet[packetNum].timestamp[i];

                    uint32_t timestamp_jump = npx_timestamp - last_npx_timestamp;

                    if (timestamp_jump > MAX_ALLOWABLE_TIMESTAMP_JUMP)
                    {
                        if (passedOneSecond && timestamp_jump < MAX_HEADSTAGE_CLK_SAMPLE)
                        {
                            String msg = "NPX TIMESTAMP JUMP: " + String (timestamp_jump) + ", expected 3 or 4...Possible data loss on slot " + String (basestation->slot_c) + ", probe " + String (headstage->port_c) + " at sample number " + String (ap_timestamp);

                            LOGC (msg);

                            basestation->neuropixThread->sendBroadcastMessage (msg);
                        }
                    }

                    last_npx_timestamp = npx_timestamp;

                    for (int j = 0; j < 384; j++)
                    {
                        apSamples[j * (12 * count) + i + (packetNum * 12)] =
                            float (packet[packetNum].apData[i][j]) * 1.2f / 1024.0f * 1000000.0f
                                / settings.availableApGains[settings.apGainIndex]
                            - ap_offsets[j][0]; // convert to microvolts

                        apView->addSample (apSamples[j * (12 * count) + i + (packetNum * 12)], j);

                        if (i == 0)
                        {
                            lfpSamples[(j * count) + packetNum] =
                                float (packet[packetNum].lfpData[j]) * 1.2f / 1024.0f * 1000000.0f
                                    / settings.availableLfpGains[settings.lfpGainIndex]
                                - lfp_offsets[j][0]; // convert to microvolts

                            lfpView->addSample (lfpSamples[(j * count) + packetNum], j);
                        }
                    }

                    ap_timestamps[i + packetNum * 12] = ap_timestamp++;
                    event_codes[i + packetNum * 12] = eventCode;

                    if (sendSync)
                        apSamples[384 * (12 * count) + i + (packetNum * 12)] = (float) eventCode;
                }

                lfp_timestamps[packetNum] = lfp_timestamp++;
                lfp_event_codes[packetNum] = eventCode;

                if (sendSync)
                    lfpSamples[(384 * count) + packetNum] = (float) eventCode;
            }

            apBuffer->addToBuffer (apSamples, ap_timestamps, timestamp_s, event_codes, 12 * count);
            lfpBuffer->addToBuffer (lfpSamples, lfp_timestamps, timestamp_s, lfp_event_codes, count);

            if (ap_offsets[0][0] == 0)
            {
                updateOffsets (apSamples, ap_timestamp, true);
                updateOffsets (lfpSamples, lfp_timestamp, false);
            }
        }
        else if (errorCode != Neuropixels::SUCCESS)
        {
            LOGD ("readPackets error code: ", errorCode, " for Basestation ", int (basestation->slot), ", probe ", int (headstage->port));
        }

        if (! passedOneSecond)
        {
            if (ap_timestamp > 30000)
                passedOneSecond = true;
        }

        int packetsAvailable;
        int headroom;

        Neuropixels::getElectrodeDataFifoState (
            basestation->slot,
            headstage->port,
            dock,
            &packetsAvailable,
            &headroom);

        fifoFillPercentage = float (packetsAvailable) / float (packetsAvailable + headroom);

        if (packetsAvailable < MAXPACKETS)
        {
            int uSecToWait = (MAXPACKETS - packetsAvailable) * 400;

            std::this_thread::sleep_for (std::chrono::microseconds (uSecToWait));
        }
    }
}

bool Neuropixels_UHD::runBist (BIST bistType)
{
    close();
    open();

    int slot = basestation->slot;
    int port = headstage->port;

    bool returnValue = false;

    switch (bistType)
    {
        case BIST::SIGNAL:
        {
            if (Neuropixels::bistSignal (slot, port, dock) == Neuropixels::SUCCESS)
                returnValue = true;
            break;
        }
        case BIST::NOISE:
        {
            if (Neuropixels::bistNoise (slot, port, dock) == Neuropixels::SUCCESS)
                returnValue = true;
            break;
        }
        case BIST::PSB:
        {
            if (Neuropixels::bistPSB (slot, port, dock) == Neuropixels::SUCCESS)
                returnValue = true;
            break;
        }
        case BIST::SR:
        {
            if (Neuropixels::bistSR (slot, port, dock) == Neuropixels::SUCCESS)
                returnValue = true;
            break;
        }
        case BIST::EEPROM:
        {
            if (Neuropixels::bistEEPROM (slot, port) == Neuropixels::SUCCESS)
                returnValue = true;
            break;
        }
        case BIST::I2C:
        {
            if (Neuropixels::bistI2CMM (slot, port, dock) == Neuropixels::SUCCESS)
                returnValue = true;
            break;
        }
        case BIST::SERDES:
        {
            int errors;
            Neuropixels::bistStartPRBS (slot, port);
            std::this_thread::sleep_for (std::chrono::milliseconds (200));
            Neuropixels::bistStopPRBS (slot, port, &errors);

            if (errors == 0)
                returnValue = true;
            break;
        }
        case BIST::HB:
        {
            if (Neuropixels::bistHB (slot, port, dock) == Neuropixels::SUCCESS)
                returnValue = true;
            break;
        }
        case BIST::BS:
        {
            if (Neuropixels::bistBS (slot) == Neuropixels::SUCCESS)
                returnValue = true;
            break;
        }
        default:
            CoreServices::sendStatusMessage ("Test not found.");
    }

    close();
    open();
    initialize (false);

    errorCode = Neuropixels::setSWTrigger (slot);
    errorCode = Neuropixels::arm (slot);

    return returnValue;
}

void Neuropixels_UHD::createElectrodeConfigurations()
{
    const int bank0[] = { 0, 57, 2, 59, 8, 49, 10, 51, 16, 41, 18, 43, 24, 33, 26, 35, 32, 25, 34, 27, 40, 17, 42, 19, 48, 9, 50, 11, 56, 1, 58, 3, 4, 61, 6, 63, 12, 53, 14, 55, 20, 45, 22, 47, 28, 37, 30, 39, 36, 29, 38, 31, 44, 21, 46, 23, 52, 13, 54, 15, 60, 5, 62, 7, 64, 121, 66, 123, 72, 113, 74, 115, 80, 105, 82, 107, 88, 97, 90, 99, 96, 89, 98, 91, 104, 81, 106, 83, 112, 73, 114, 75, 120, 65, 122, 67, 68, 125, 70, 127, 76, 117, 78, 119, 84, 109, 86, 111, 92, 101, 94, 103, 100, 93, 102, 95, 108, 85, 110, 87, 116, 77, 118, 79, 124, 69, 126, 71, 128, 185, 130, 187, 136, 177, 138, 179, 144, 169, 146, 171, 152, 161, 154, 163, 160, 153, 162, 155, 168, 145, 170, 147, 176, 137, 178, 139, 184, 129, 186, 131, 132, 189, 134, 191, 140, 181, 142, 183, 148, 173, 150, 175, 156, 165, 158, 167, 164, 157, 166, 159, 172, 149, 174, 151, 180, 141, 182, 143, 188, 133, 190, 135, 192, 249, 194, 251, 200, 241, 202, 243, 208, 233, 210, 235, 216, 225, 218, 227, 224, 217, 226, 219, 232, 209, 234, 211, 240, 201, 242, 203, 248, 193, 250, 195, 196, 253, 198, 255, 204, 245, 206, 247, 212, 237, 214, 239, 220, 229, 222, 231, 228, 221, 230, 223, 236, 213, 238, 215, 244, 205, 246, 207, 252, 197, 254, 199, 256, 313, 258, 315, 264, 305, 266, 307, 272, 297, 274, 299, 280, 289, 282, 291, 288, 281, 290, 283, 296, 273, 298, 275, 304, 265, 306, 267, 312, 257, 314, 259, 260, 317, 262, 319, 268, 309, 270, 311, 276, 301, 278, 303, 284, 293, 286, 295, 292, 285, 294, 287, 300, 277, 302, 279, 308, 269, 310, 271, 316, 261, 318, 263, 320, 377, 322, 379, 328, 369, 330, 371, 336, 361, 338, 363, 344, 353, 346, 355, 352, 345, 354, 347, 360, 337, 362, 339, 368, 329, 370, 331, 376, 321, 378, 323, 324, 381, 326, 383, 332, 373, 334, 375, 340, 365, 342, 367, 348, 357, 350, 359, 356, 349, 358, 351, 364, 341, 366, 343, 372, 333, 374, 335, 380, 325, 382, 327 };

    const int bank1[] = {
        388, 445, 390, 447, 396, 437, 398, 439, 404, 429, 406, 431, 412, 421, 414, 423, 420, 413, 422, 415, 428, 405, 430, 407, 436, 397, 438, 399, 444, 389, 446, 391, 384, 441, 386, 443, 392, 433, 394, 435, 400, 425, 402, 427, 408, 417, 410, 419, 416, 409, 418, 411, 424, 401, 426, 403, 432, 393, 434, 395, 440, 385, 442, 387, 452, 509, 454, 511, 460, 501, 462, 503, 468, 493, 470, 495, 476, 485, 478, 487, 484, 477, 486, 479, 492, 469, 494, 471, 500, 461, 502, 463, 508, 453, 510, 455, 448, 505, 450, 507, 456, 497, 458, 499, 464, 489, 466, 491, 472, 481, 474, 483, 480, 473, 482, 475, 488, 465, 490, 467, 496, 457, 498, 459, 504, 449, 506, 451, 516, 573, 518, 575, 524, 565, 526, 567, 532, 557, 534, 559, 540, 549, 542, 551, 548, 541, 550, 543, 556, 533, 558, 535, 564, 525, 566, 527, 572, 517, 574, 519, 512, 569, 514, 571, 520, 561, 522, 563, 528, 553, 530, 555, 536, 545, 538, 547, 544, 537, 546, 539, 552, 529, 554, 531, 560, 521, 562, 523, 568, 513, 570, 515, 580, 637, 582, 639, 588, 629, 590, 631, 596, 621, 598, 623, 604, 613, 606, 615, 612, 605, 614, 607, 620, 597, 622, 599, 628, 589, 630, 591, 636, 581, 638, 583, 576, 633, 578, 635, 584, 625, 586, 627, 592, 617, 594, 619, 600, 609, 602, 611, 608, 601, 610, 603, 616, 593, 618, 595, 624, 585, 626, 587, 632, 577, 634, 579, 644, 701, 646, 703, 652, 693, 654, 695, 660, 685, 662, 687, 668, 677, 670, 679, 676, 669, 678, 671, 684, 661, 686, 663, 692, 653, 694, 655, 700, 645, 702, 647, 640, 697, 642, 699, 648, 689, 650, 691, 656, 681, 658, 683, 664, 673, 666, 675, 672, 665, 674, 667, 680, 657, 682, 659, 688, 649, 690, 651, 696, 641, 698, 643, 708, 765, 710, 767, 716, 757, 718, 759, 724, 749, 726, 751, 732, 741, 734, 743, 740, 733, 742, 735, 748, 725, 750, 727, 756, 717, 758, 719, 764, 709, 766, 711, 704, 761, 706, 763, 712, 753, 714, 755, 720, 745, 722, 747, 728, 737, 730, 739, 736, 729, 738, 731, 744, 721, 746, 723, 752, 713, 754, 715, 760, 705, 762, 707
    };

    const int bank2[] = { 768, 825, 770, 827, 776, 817, 778, 819, 784, 809, 786, 811, 792, 801, 794, 803, 800, 793, 802, 795, 808, 785, 810, 787, 816, 777, 818, 779, 824, 769, 826, 771, 772, 829, 774, 831, 780, 821, 782, 823, 788, 813, 790, 815, 796, 805, 798, 807, 804, 797, 806, 799, 812, 789, 814, 791, 820, 781, 822, 783, 828, 773, 830, 775, 832, 889, 834, 891, 840, 881, 842, 883, 848, 873, 850, 875, 856, 865, 858, 867, 864, 857, 866, 859, 872, 849, 874, 851, 880, 841, 882, 843, 888, 833, 890, 835, 836, 893, 838, 895, 844, 885, 846, 887, 852, 877, 854, 879, 860, 869, 862, 871, 868, 861, 870, 863, 876, 853, 878, 855, 884, 845, 886, 847, 892, 837, 894, 839, 896, 953, 898, 955, 904, 945, 906, 947, 912, 937, 914, 939, 920, 929, 922, 931, 928, 921, 930, 923, 936, 913, 938, 915, 944, 905, 946, 907, 952, 897, 954, 899, 900, 957, 902, 959, 908, 949, 910, 951, 916, 941, 918, 943, 924, 933, 926, 935, 932, 925, 934, 927, 940, 917, 942, 919, 948, 909, 950, 911, 956, 901, 958, 903, 960, 1017, 962, 1019, 968, 1009, 970, 1011, 976, 1001, 978, 1003, 984, 993, 986, 995, 992, 985, 994, 987, 1000, 977, 1002, 979, 1008, 969, 1010, 971, 1016, 961, 1018, 963, 964, 1021, 966, 1023, 972, 1013, 974, 1015, 980, 1005, 982, 1007, 988, 997, 990, 999, 996, 989, 998, 991, 1004, 981, 1006, 983, 1012, 973, 1014, 975, 1020, 965, 1022, 967, 1024, 1081, 1026, 1083, 1032, 1073, 1034, 1075, 1040, 1065, 1042, 1067, 1048, 1057, 1050, 1059, 1056, 1049, 1058, 1051, 1064, 1041, 1066, 1043, 1072, 1033, 1074, 1035, 1080, 1025, 1082, 1027, 1028, 1085, 1030, 1087, 1036, 1077, 1038, 1079, 1044, 1069, 1046, 1071, 1052, 1061, 1054, 1063, 1060, 1053, 1062, 1055, 1068, 1045, 1070, 1047, 1076, 1037, 1078, 1039, 1084, 1029, 1086, 1031, 1088, 1145, 1090, 1147, 1096, 1137, 1098, 1139, 1104, 1129, 1106, 1131, 1112, 1121, 1114, 1123, 1120, 1113, 1122, 1115, 1128, 1105, 1130, 1107, 1136, 1097, 1138, 1099, 1144, 1089, 1146, 1091, 1092, 1149, 1094, 1151, 1100, 1141, 1102, 1143, 1108, 1133, 1110, 1135, 1116, 1125, 1118, 1127, 1124, 1117, 1126, 1119, 1132, 1109, 1134, 1111, 1140, 1101, 1142, 1103, 1148, 1093, 1150, 1095 };

    const int bank3[] = { 1156, 1213, 1158, 1215, 1164, 1205, 1166, 1207, 1172, 1197, 1174, 1199, 1180, 1189, 1182, 1191, 1188, 1181, 1190, 1183, 1196, 1173, 1198, 1175, 1204, 1165, 1206, 1167, 1212, 1157, 1214, 1159, 1152, 1209, 1154, 1211, 1160, 1201, 1162, 1203, 1168, 1193, 1170, 1195, 1176, 1185, 1178, 1187, 1184, 1177, 1186, 1179, 1192, 1169, 1194, 1171, 1200, 1161, 1202, 1163, 1208, 1153, 1210, 1155, 1220, 1277, 1222, 1279, 1228, 1269, 1230, 1271, 1236, 1261, 1238, 1263, 1244, 1253, 1246, 1255, 1252, 1245, 1254, 1247, 1260, 1237, 1262, 1239, 1268, 1229, 1270, 1231, 1276, 1221, 1278, 1223, 1216, 1273, 1218, 1275, 1224, 1265, 1226, 1267, 1232, 1257, 1234, 1259, 1240, 1249, 1242, 1251, 1248, 1241, 1250, 1243, 1256, 1233, 1258, 1235, 1264, 1225, 1266, 1227, 1272, 1217, 1274, 1219, 1284, 1341, 1286, 1343, 1292, 1333, 1294, 1335, 1300, 1325, 1302, 1327, 1308, 1317, 1310, 1319, 1316, 1309, 1318, 1311, 1324, 1301, 1326, 1303, 1332, 1293, 1334, 1295, 1340, 1285, 1342, 1287, 1280, 1337, 1282, 1339, 1288, 1329, 1290, 1331, 1296, 1321, 1298, 1323, 1304, 1313, 1306, 1315, 1312, 1305, 1314, 1307, 1320, 1297, 1322, 1299, 1328, 1289, 1330, 1291, 1336, 1281, 1338, 1283, 1348, 1405, 1350, 1407, 1356, 1397, 1358, 1399, 1364, 1389, 1366, 1391, 1372, 1381, 1374, 1383, 1380, 1373, 1382, 1375, 1388, 1365, 1390, 1367, 1396, 1357, 1398, 1359, 1404, 1349, 1406, 1351, 1344, 1401, 1346, 1403, 1352, 1393, 1354, 1395, 1360, 1385, 1362, 1387, 1368, 1377, 1370, 1379, 1376, 1369, 1378, 1371, 1384, 1361, 1386, 1363, 1392, 1353, 1394, 1355, 1400, 1345, 1402, 1347, 1412, 1469, 1414, 1471, 1420, 1461, 1422, 1463, 1428, 1453, 1430, 1455, 1436, 1445, 1438, 1447, 1444, 1437, 1446, 1439, 1452, 1429, 1454, 1431, 1460, 1421, 1462, 1423, 1468, 1413, 1470, 1415, 1408, 1465, 1410, 1467, 1416, 1457, 1418, 1459, 1424, 1449, 1426, 1451, 1432, 1441, 1434, 1443, 1440, 1433, 1442, 1435, 1448, 1425, 1450, 1427, 1456, 1417, 1458, 1419, 1464, 1409, 1466, 1411, 1476, 1533, 1478, 1535, 1484, 1525, 1486, 1527, 1492, 1517, 1494, 1519, 1500, 1509, 1502, 1511, 1508, 1501, 1510, 1503, 1516, 1493, 1518, 1495, 1524, 1485, 1526, 1487, 1532, 1477, 1534, 1479, 1472, 1529, 1474, 1531, 1480, 1521, 1482, 1523, 1488, 1513, 1490, 1515, 1496, 1505, 1498, 1507, 1504, 1497, 1506, 1499, 1512, 1489, 1514, 1491, 1520, 1481, 1522, 1483, 1528, 1473, 1530, 1475 };

    const int bank4[] = { 1538, 1595, 1536, 1593, 1546, 1587, 1544, 1585, 1554, 1579, 1552, 1577, 1562, 1571, 1560, 1569, 1570, 1563, 1568, 1561, 1578, 1555, 1576, 1553, 1586, 1547, 1584, 1545, 1594, 1539, 1592, 1537, 1542, 1599, 1540, 1597, 1550, 1591, 1548, 1589, 1558, 1583, 1556, 1581, 1566, 1575, 1564, 1573, 1574, 1567, 1572, 1565, 1582, 1559, 1580, 1557, 1590, 1551, 1588, 1549, 1598, 1543, 1596, 1541, 1602, 1659, 1600, 1657, 1610, 1651, 1608, 1649, 1618, 1643, 1616, 1641, 1626, 1635, 1624, 1633, 1634, 1627, 1632, 1625, 1642, 1619, 1640, 1617, 1650, 1611, 1648, 1609, 1658, 1603, 1656, 1601, 1606, 1663, 1604, 1661, 1614, 1655, 1612, 1653, 1622, 1647, 1620, 1645, 1630, 1639, 1628, 1637, 1638, 1631, 1636, 1629, 1646, 1623, 1644, 1621, 1654, 1615, 1652, 1613, 1662, 1607, 1660, 1605, 1666, 1723, 1664, 1721, 1674, 1715, 1672, 1713, 1682, 1707, 1680, 1705, 1690, 1699, 1688, 1697, 1698, 1691, 1696, 1689, 1706, 1683, 1704, 1681, 1714, 1675, 1712, 1673, 1722, 1667, 1720, 1665, 1670, 1727, 1668, 1725, 1678, 1719, 1676, 1717, 1686, 1711, 1684, 1709, 1694, 1703, 1692, 1701, 1702, 1695, 1700, 1693, 1710, 1687, 1708, 1685, 1718, 1679, 1716, 1677, 1726, 1671, 1724, 1669, 1730, 1787, 1728, 1785, 1738, 1779, 1736, 1777, 1746, 1771, 1744, 1769, 1754, 1763, 1752, 1761, 1762, 1755, 1760, 1753, 1770, 1747, 1768, 1745, 1778, 1739, 1776, 1737, 1786, 1731, 1784, 1729, 1734, 1791, 1732, 1789, 1742, 1783, 1740, 1781, 1750, 1775, 1748, 1773, 1758, 1767, 1756, 1765, 1766, 1759, 1764, 1757, 1774, 1751, 1772, 1749, 1782, 1743, 1780, 1741, 1790, 1735, 1788, 1733, 1794, 1851, 1792, 1849, 1802, 1843, 1800, 1841, 1810, 1835, 1808, 1833, 1818, 1827, 1816, 1825, 1826, 1819, 1824, 1817, 1834, 1811, 1832, 1809, 1842, 1803, 1840, 1801, 1850, 1795, 1848, 1793, 1798, 1855, 1796, 1853, 1806, 1847, 1804, 1845, 1814, 1839, 1812, 1837, 1822, 1831, 1820, 1829, 1830, 1823, 1828, 1821, 1838, 1815, 1836, 1813, 1846, 1807, 1844, 1805, 1854, 1799, 1852, 1797, 1858, 1915, 1856, 1913, 1866, 1907, 1864, 1905, 1874, 1899, 1872, 1897, 1882, 1891, 1880, 1889, 1890, 1883, 1888, 1881, 1898, 1875, 1896, 1873, 1906, 1867, 1904, 1865, 1914, 1859, 1912, 1857, 1862, 1919, 1860, 1917, 1870, 1911, 1868, 1909, 1878, 1903, 1876, 1901, 1886, 1895, 1884, 1893, 1894, 1887, 1892, 1885, 1902, 1879, 1900, 1877, 1910, 1871, 1908, 1869, 1918, 1863, 1916, 1861 };

    const int bank5[] = { 1926, 1983, 1924, 1981, 1934, 1975, 1932, 1973, 1942, 1967, 1940, 1965, 1950, 1959, 1948, 1957, 1958, 1951, 1956, 1949, 1966, 1943, 1964, 1941, 1974, 1935, 1972, 1933, 1982, 1927, 1980, 1925, 1922, 1979, 1920, 1977, 1930, 1971, 1928, 1969, 1938, 1963, 1936, 1961, 1946, 1955, 1944, 1953, 1954, 1947, 1952, 1945, 1962, 1939, 1960, 1937, 1970, 1931, 1968, 1929, 1978, 1923, 1976, 1921, 1990, 2047, 1988, 2045, 1998, 2039, 1996, 2037, 2006, 2031, 2004, 2029, 2014, 2023, 2012, 2021, 2022, 2015, 2020, 2013, 2030, 2007, 2028, 2005, 2038, 1999, 2036, 1997, 2046, 1991, 2044, 1989, 1986, 2043, 1984, 2041, 1994, 2035, 1992, 2033, 2002, 2027, 2000, 2025, 2010, 2019, 2008, 2017, 2018, 2011, 2016, 2009, 2026, 2003, 2024, 2001, 2034, 1995, 2032, 1993, 2042, 1987, 2040, 1985, 2054, 2111, 2052, 2109, 2062, 2103, 2060, 2101, 2070, 2095, 2068, 2093, 2078, 2087, 2076, 2085, 2086, 2079, 2084, 2077, 2094, 2071, 2092, 2069, 2102, 2063, 2100, 2061, 2110, 2055, 2108, 2053, 2050, 2107, 2048, 2105, 2058, 2099, 2056, 2097, 2066, 2091, 2064, 2089, 2074, 2083, 2072, 2081, 2082, 2075, 2080, 2073, 2090, 2067, 2088, 2065, 2098, 2059, 2096, 2057, 2106, 2051, 2104, 2049, 2118, 2175, 2116, 2173, 2126, 2167, 2124, 2165, 2134, 2159, 2132, 2157, 2142, 2151, 2140, 2149, 2150, 2143, 2148, 2141, 2158, 2135, 2156, 2133, 2166, 2127, 2164, 2125, 2174, 2119, 2172, 2117, 2114, 2171, 2112, 2169, 2122, 2163, 2120, 2161, 2130, 2155, 2128, 2153, 2138, 2147, 2136, 2145, 2146, 2139, 2144, 2137, 2154, 2131, 2152, 2129, 2162, 2123, 2160, 2121, 2170, 2115, 2168, 2113, 2182, 2239, 2180, 2237, 2190, 2231, 2188, 2229, 2198, 2223, 2196, 2221, 2206, 2215, 2204, 2213, 2214, 2207, 2212, 2205, 2222, 2199, 2220, 2197, 2230, 2191, 2228, 2189, 2238, 2183, 2236, 2181, 2178, 2235, 2176, 2233, 2186, 2227, 2184, 2225, 2194, 2219, 2192, 2217, 2202, 2211, 2200, 2209, 2210, 2203, 2208, 2201, 2218, 2195, 2216, 2193, 2226, 2187, 2224, 2185, 2234, 2179, 2232, 2177, 2246, 2303, 2244, 2301, 2254, 2295, 2252, 2293, 2262, 2287, 2260, 2285, 2270, 2279, 2268, 2277, 2278, 2271, 2276, 2269, 2286, 2263, 2284, 2261, 2294, 2255, 2292, 2253, 2302, 2247, 2300, 2245, 2242, 2299, 2240, 2297, 2250, 2291, 2248, 2289, 2258, 2283, 2256, 2281, 2266, 2275, 2264, 2273, 2274, 2267, 2272, 2265, 2282, 2259, 2280, 2257, 2290, 2251, 2288, 2249, 2298, 2243, 2296, 2241 };

    const int bank6[] = { 2306, 2363, 2304, 2361, 2314, 2355, 2312, 2353, 2322, 2347, 2320, 2345, 2330, 2339, 2328, 2337, 2338, 2331, 2336, 2329, 2346, 2323, 2344, 2321, 2354, 2315, 2352, 2313, 2362, 2307, 2360, 2305, 2310, 2367, 2308, 2365, 2318, 2359, 2316, 2357, 2326, 2351, 2324, 2349, 2334, 2343, 2332, 2341, 2342, 2335, 2340, 2333, 2350, 2327, 2348, 2325, 2358, 2319, 2356, 2317, 2366, 2311, 2364, 2309, 2370, 2427, 2368, 2425, 2378, 2419, 2376, 2417, 2386, 2411, 2384, 2409, 2394, 2403, 2392, 2401, 2402, 2395, 2400, 2393, 2410, 2387, 2408, 2385, 2418, 2379, 2416, 2377, 2426, 2371, 2424, 2369, 2374, 2431, 2372, 2429, 2382, 2423, 2380, 2421, 2390, 2415, 2388, 2413, 2398, 2407, 2396, 2405, 2406, 2399, 2404, 2397, 2414, 2391, 2412, 2389, 2422, 2383, 2420, 2381, 2430, 2375, 2428, 2373, 2434, 2491, 2432, 2489, 2442, 2483, 2440, 2481, 2450, 2475, 2448, 2473, 2458, 2467, 2456, 2465, 2466, 2459, 2464, 2457, 2474, 2451, 2472, 2449, 2482, 2443, 2480, 2441, 2490, 2435, 2488, 2433, 2438, 2495, 2436, 2493, 2446, 2487, 2444, 2485, 2454, 2479, 2452, 2477, 2462, 2471, 2460, 2469, 2470, 2463, 2468, 2461, 2478, 2455, 2476, 2453, 2486, 2447, 2484, 2445, 2494, 2439, 2492, 2437, 2498, 2555, 2496, 2553, 2506, 2547, 2504, 2545, 2514, 2539, 2512, 2537, 2522, 2531, 2520, 2529, 2530, 2523, 2528, 2521, 2538, 2515, 2536, 2513, 2546, 2507, 2544, 2505, 2554, 2499, 2552, 2497, 2502, 2559, 2500, 2557, 2510, 2551, 2508, 2549, 2518, 2543, 2516, 2541, 2526, 2535, 2524, 2533, 2534, 2527, 2532, 2525, 2542, 2519, 2540, 2517, 2550, 2511, 2548, 2509, 2558, 2503, 2556, 2501, 2562, 2619, 2560, 2617, 2570, 2611, 2568, 2609, 2578, 2603, 2576, 2601, 2586, 2595, 2584, 2593, 2594, 2587, 2592, 2585, 2602, 2579, 2600, 2577, 2610, 2571, 2608, 2569, 2618, 2563, 2616, 2561, 2566, 2623, 2564, 2621, 2574, 2615, 2572, 2613, 2582, 2607, 2580, 2605, 2590, 2599, 2588, 2597, 2598, 2591, 2596, 2589, 2606, 2583, 2604, 2581, 2614, 2575, 2612, 2573, 2622, 2567, 2620, 2565, 2626, 2683, 2624, 2681, 2634, 2675, 2632, 2673, 2642, 2667, 2640, 2665, 2650, 2659, 2648, 2657, 2658, 2651, 2656, 2649, 2666, 2643, 2664, 2641, 2674, 2635, 2672, 2633, 2682, 2627, 2680, 2625, 2630, 2687, 2628, 2685, 2638, 2679, 2636, 2677, 2646, 2671, 2644, 2669, 2654, 2663, 2652, 2661, 2662, 2655, 2660, 2653, 2670, 2647, 2668, 2645, 2678, 2639, 2676, 2637, 2686, 2631, 2684, 2629 };

    const int bank7[] = { 2694, 2751, 2692, 2749, 2702, 2743, 2700, 2741, 2710, 2735, 2708, 2733, 2718, 2727, 2716, 2725, 2726, 2719, 2724, 2717, 2734, 2711, 2732, 2709, 2742, 2703, 2740, 2701, 2750, 2695, 2748, 2693, 2690, 2747, 2688, 2745, 2698, 2739, 2696, 2737, 2706, 2731, 2704, 2729, 2714, 2723, 2712, 2721, 2722, 2715, 2720, 2713, 2730, 2707, 2728, 2705, 2738, 2699, 2736, 2697, 2746, 2691, 2744, 2689, 2758, 2815, 2756, 2813, 2766, 2807, 2764, 2805, 2774, 2799, 2772, 2797, 2782, 2791, 2780, 2789, 2790, 2783, 2788, 2781, 2798, 2775, 2796, 2773, 2806, 2767, 2804, 2765, 2814, 2759, 2812, 2757, 2754, 2811, 2752, 2809, 2762, 2803, 2760, 2801, 2770, 2795, 2768, 2793, 2778, 2787, 2776, 2785, 2786, 2779, 2784, 2777, 2794, 2771, 2792, 2769, 2802, 2763, 2800, 2761, 2810, 2755, 2808, 2753, 2822, 2879, 2820, 2877, 2830, 2871, 2828, 2869, 2838, 2863, 2836, 2861, 2846, 2855, 2844, 2853, 2854, 2847, 2852, 2845, 2862, 2839, 2860, 2837, 2870, 2831, 2868, 2829, 2878, 2823, 2876, 2821, 2818, 2875, 2816, 2873, 2826, 2867, 2824, 2865, 2834, 2859, 2832, 2857, 2842, 2851, 2840, 2849, 2850, 2843, 2848, 2841, 2858, 2835, 2856, 2833, 2866, 2827, 2864, 2825, 2874, 2819, 2872, 2817, 2886, 2943, 2884, 2941, 2894, 2935, 2892, 2933, 2902, 2927, 2900, 2925, 2910, 2919, 2908, 2917, 2918, 2911, 2916, 2909, 2926, 2903, 2924, 2901, 2934, 2895, 2932, 2893, 2942, 2887, 2940, 2885, 2882, 2939, 2880, 2937, 2890, 2931, 2888, 2929, 2898, 2923, 2896, 2921, 2906, 2915, 2904, 2913, 2914, 2907, 2912, 2905, 2922, 2899, 2920, 2897, 2930, 2891, 2928, 2889, 2938, 2883, 2936, 2881, 2950, 3007, 2948, 3005, 2958, 2999, 2956, 2997, 2966, 2991, 2964, 2989, 2974, 2983, 2972, 2981, 2982, 2975, 2980, 2973, 2990, 2967, 2988, 2965, 2998, 2959, 2996, 2957, 3006, 2951, 3004, 2949, 2946, 3003, 2944, 3001, 2954, 2995, 2952, 2993, 2962, 2987, 2960, 2985, 2970, 2979, 2968, 2977, 2978, 2971, 2976, 2969, 2986, 2963, 2984, 2961, 2994, 2955, 2992, 2953, 3002, 2947, 3000, 2945, 3014, 3071, 3012, 3069, 3022, 3063, 3020, 3061, 3030, 3055, 3028, 3053, 3038, 3047, 3036, 3045, 3046, 3039, 3044, 3037, 3054, 3031, 3052, 3029, 3062, 3023, 3060, 3021, 3070, 3015, 3068, 3013, 3010, 3067, 3008, 3065, 3018, 3059, 3016, 3057, 3026, 3051, 3024, 3049, 3034, 3043, 3032, 3041, 3042, 3035, 3040, 3033, 3050, 3027, 3048, 3025, 3058, 3019, 3056, 3017, 3066, 3011, 3064, 3009 };

    const int bank8[] = { 3072, 3129, 3074, 3131, 3080, 3121, 3082, 3123, 3088, 3113, 3090, 3115, 3096, 3105, 3098, 3107, 3104, 3097, 3106, 3099, 3112, 3089, 3114, 3091, 3120, 3081, 3122, 3083, 3128, 3073, 3130, 3075, 3076, 3133, 3078, 3135, 3084, 3125, 3086, 3127, 3092, 3117, 3094, 3119, 3100, 3109, 3102, 3111, 3108, 3101, 3110, 3103, 3116, 3093, 3118, 3095, 3124, 3085, 3126, 3087, 3132, 3077, 3134, 3079, 3136, 3193, 3138, 3195, 3144, 3185, 3146, 3187, 3152, 3177, 3154, 3179, 3160, 3169, 3162, 3171, 3168, 3161, 3170, 3163, 3176, 3153, 3178, 3155, 3184, 3145, 3186, 3147, 3192, 3137, 3194, 3139, 3140, 3197, 3142, 3199, 3148, 3189, 3150, 3191, 3156, 3181, 3158, 3183, 3164, 3173, 3166, 3175, 3172, 3165, 3174, 3167, 3180, 3157, 3182, 3159, 3188, 3149, 3190, 3151, 3196, 3141, 3198, 3143, 3200, 3257, 3202, 3259, 3208, 3249, 3210, 3251, 3216, 3241, 3218, 3243, 3224, 3233, 3226, 3235, 3232, 3225, 3234, 3227, 3240, 3217, 3242, 3219, 3248, 3209, 3250, 3211, 3256, 3201, 3258, 3203, 3204, 3261, 3206, 3263, 3212, 3253, 3214, 3255, 3220, 3245, 3222, 3247, 3228, 3237, 3230, 3239, 3236, 3229, 3238, 3231, 3244, 3221, 3246, 3223, 3252, 3213, 3254, 3215, 3260, 3205, 3262, 3207, 3264, 3321, 3266, 3323, 3272, 3313, 3274, 3315, 3280, 3305, 3282, 3307, 3288, 3297, 3290, 3299, 3296, 3289, 3298, 3291, 3304, 3281, 3306, 3283, 3312, 3273, 3314, 3275, 3320, 3265, 3322, 3267, 3268, 3325, 3270, 3327, 3276, 3317, 3278, 3319, 3284, 3309, 3286, 3311, 3292, 3301, 3294, 3303, 3300, 3293, 3302, 3295, 3308, 3285, 3310, 3287, 3316, 3277, 3318, 3279, 3324, 3269, 3326, 3271, 3328, 3385, 3330, 3387, 3336, 3377, 3338, 3379, 3344, 3369, 3346, 3371, 3352, 3361, 3354, 3363, 3360, 3353, 3362, 3355, 3368, 3345, 3370, 3347, 3376, 3337, 3378, 3339, 3384, 3329, 3386, 3331, 3332, 3389, 3334, 3391, 3340, 3381, 3342, 3383, 3348, 3373, 3350, 3375, 3356, 3365, 3358, 3367, 3364, 3357, 3366, 3359, 3372, 3349, 3374, 3351, 3380, 3341, 3382, 3343, 3388, 3333, 3390, 3335, 3392, 3449, 3394, 3451, 3400, 3441, 3402, 3443, 3408, 3433, 3410, 3435, 3416, 3425, 3418, 3427, 3424, 3417, 3426, 3419, 3432, 3409, 3434, 3411, 3440, 3401, 3442, 3403, 3448, 3393, 3450, 3395, 3396, 3453, 3398, 3455, 3404, 3445, 3406, 3447, 3412, 3437, 3414, 3439, 3420, 3429, 3422, 3431, 3428, 3421, 3430, 3423, 3436, 3413, 3438, 3415, 3444, 3405, 3446, 3407, 3452, 3397, 3454, 3399 };

    const int bank9[] = { 3460, 3517, 3462, 3519, 3468, 3509, 3470, 3511, 3476, 3501, 3478, 3503, 3484, 3493, 3486, 3495, 3492, 3485, 3494, 3487, 3500, 3477, 3502, 3479, 3508, 3469, 3510, 3471, 3516, 3461, 3518, 3463, 3456, 3513, 3458, 3515, 3464, 3505, 3466, 3507, 3472, 3497, 3474, 3499, 3480, 3489, 3482, 3491, 3488, 3481, 3490, 3483, 3496, 3473, 3498, 3475, 3504, 3465, 3506, 3467, 3512, 3457, 3514, 3459, 3524, 3581, 3526, 3583, 3532, 3573, 3534, 3575, 3540, 3565, 3542, 3567, 3548, 3557, 3550, 3559, 3556, 3549, 3558, 3551, 3564, 3541, 3566, 3543, 3572, 3533, 3574, 3535, 3580, 3525, 3582, 3527, 3520, 3577, 3522, 3579, 3528, 3569, 3530, 3571, 3536, 3561, 3538, 3563, 3544, 3553, 3546, 3555, 3552, 3545, 3554, 3547, 3560, 3537, 3562, 3539, 3568, 3529, 3570, 3531, 3576, 3521, 3578, 3523, 3588, 3645, 3590, 3647, 3596, 3637, 3598, 3639, 3604, 3629, 3606, 3631, 3612, 3621, 3614, 3623, 3620, 3613, 3622, 3615, 3628, 3605, 3630, 3607, 3636, 3597, 3638, 3599, 3644, 3589, 3646, 3591, 3584, 3641, 3586, 3643, 3592, 3633, 3594, 3635, 3600, 3625, 3602, 3627, 3608, 3617, 3610, 3619, 3616, 3609, 3618, 3611, 3624, 3601, 3626, 3603, 3632, 3593, 3634, 3595, 3640, 3585, 3642, 3587, 3652, 3709, 3654, 3711, 3660, 3701, 3662, 3703, 3668, 3693, 3670, 3695, 3676, 3685, 3678, 3687, 3684, 3677, 3686, 3679, 3692, 3669, 3694, 3671, 3700, 3661, 3702, 3663, 3708, 3653, 3710, 3655, 3648, 3705, 3650, 3707, 3656, 3697, 3658, 3699, 3664, 3689, 3666, 3691, 3672, 3681, 3674, 3683, 3680, 3673, 3682, 3675, 3688, 3665, 3690, 3667, 3696, 3657, 3698, 3659, 3704, 3649, 3706, 3651, 3716, 3773, 3718, 3775, 3724, 3765, 3726, 3767, 3732, 3757, 3734, 3759, 3740, 3749, 3742, 3751, 3748, 3741, 3750, 3743, 3756, 3733, 3758, 3735, 3764, 3725, 3766, 3727, 3772, 3717, 3774, 3719, 3712, 3769, 3714, 3771, 3720, 3761, 3722, 3763, 3728, 3753, 3730, 3755, 3736, 3745, 3738, 3747, 3744, 3737, 3746, 3739, 3752, 3729, 3754, 3731, 3760, 3721, 3762, 3723, 3768, 3713, 3770, 3715, 3780, 3837, 3782, 3839, 3788, 3829, 3790, 3831, 3796, 3821, 3798, 3823, 3804, 3813, 3806, 3815, 3812, 3805, 3814, 3807, 3820, 3797, 3822, 3799, 3828, 3789, 3830, 3791, 3836, 3781, 3838, 3783, 3776, 3833, 3778, 3835, 3784, 3825, 3786, 3827, 3792, 3817, 3794, 3819, 3800, 3809, 3802, 3811, 3808, 3801, 3810, 3803, 3816, 3793, 3818, 3795, 3824, 3785, 3826, 3787, 3832, 3777, 3834, 3779 };

    const int bank10[] = { 3840, 3897, 3842, 3899, 3848, 3889, 3850, 3891, 3856, 3881, 3858, 3883, 3864, 3873, 3866, 3875, 3872, 3865, 3874, 3867, 3880, 3857, 3882, 3859, 3888, 3849, 3890, 3851, 3896, 3841, 3898, 3843, 3844, 3901, 3846, 3903, 3852, 3893, 3854, 3895, 3860, 3885, 3862, 3887, 3868, 3877, 3870, 3879, 3876, 3869, 3878, 3871, 3884, 3861, 3886, 3863, 3892, 3853, 3894, 3855, 3900, 3845, 3902, 3847, 3904, 3961, 3906, 3963, 3912, 3953, 3914, 3955, 3920, 3945, 3922, 3947, 3928, 3937, 3930, 3939, 3936, 3929, 3938, 3931, 3944, 3921, 3946, 3923, 3952, 3913, 3954, 3915, 3960, 3905, 3962, 3907, 3908, 3965, 3910, 3967, 3916, 3957, 3918, 3959, 3924, 3949, 3926, 3951, 3932, 3941, 3934, 3943, 3940, 3933, 3942, 3935, 3948, 3925, 3950, 3927, 3956, 3917, 3958, 3919, 3964, 3909, 3966, 3911, 3968, 4025, 3970, 4027, 3976, 4017, 3978, 4019, 3984, 4009, 3986, 4011, 3992, 4001, 3994, 4003, 4000, 3993, 4002, 3995, 4008, 3985, 4010, 3987, 4016, 3977, 4018, 3979, 4024, 3969, 4026, 3971, 3972, 4029, 3974, 4031, 3980, 4021, 3982, 4023, 3988, 4013, 3990, 4015, 3996, 4005, 3998, 4007, 4004, 3997, 4006, 3999, 4012, 3989, 4014, 3991, 4020, 3981, 4022, 3983, 4028, 3973, 4030, 3975, 4032, 4089, 4034, 4091, 4040, 4081, 4042, 4083, 4048, 4073, 4050, 4075, 4056, 4065, 4058, 4067, 4064, 4057, 4066, 4059, 4072, 4049, 4074, 4051, 4080, 4041, 4082, 4043, 4088, 4033, 4090, 4035, 4036, 4093, 4038, 4095, 4044, 4085, 4046, 4087, 4052, 4077, 4054, 4079, 4060, 4069, 4062, 4071, 4068, 4061, 4070, 4063, 4076, 4053, 4078, 4055, 4084, 4045, 4086, 4047, 4092, 4037, 4094, 4039, 4096, 4153, 4098, 4155, 4104, 4145, 4106, 4147, 4112, 4137, 4114, 4139, 4120, 4129, 4122, 4131, 4128, 4121, 4130, 4123, 4136, 4113, 4138, 4115, 4144, 4105, 4146, 4107, 4152, 4097, 4154, 4099, 4100, 4157, 4102, 4159, 4108, 4149, 4110, 4151, 4116, 4141, 4118, 4143, 4124, 4133, 4126, 4135, 4132, 4125, 4134, 4127, 4140, 4117, 4142, 4119, 4148, 4109, 4150, 4111, 4156, 4101, 4158, 4103, 4160, 4217, 4162, 4219, 4168, 4209, 4170, 4211, 4176, 4201, 4178, 4203, 4184, 4193, 4186, 4195, 4192, 4185, 4194, 4187, 4200, 4177, 4202, 4179, 4208, 4169, 4210, 4171, 4216, 4161, 4218, 4163, 4164, 4221, 4166, 4223, 4172, 4213, 4174, 4215, 4180, 4205, 4182, 4207, 4188, 4197, 4190, 4199, 4196, 4189, 4198, 4191, 4204, 4181, 4206, 4183, 4212, 4173, 4214, 4175, 4220, 4165, 4222, 4167 };

    const int bank11[] = { 4228, 4285, 4230, 4287, 4236, 4277, 4238, 4279, 4244, 4269, 4246, 4271, 4252, 4261, 4254, 4263, 4260, 4253, 4262, 4255, 4268, 4245, 4270, 4247, 4276, 4237, 4278, 4239, 4284, 4229, 4286, 4231, 4224, 4281, 4226, 4283, 4232, 4273, 4234, 4275, 4240, 4265, 4242, 4267, 4248, 4257, 4250, 4259, 4256, 4249, 4258, 4251, 4264, 4241, 4266, 4243, 4272, 4233, 4274, 4235, 4280, 4225, 4282, 4227, 4292, 4349, 4294, 4351, 4300, 4341, 4302, 4343, 4308, 4333, 4310, 4335, 4316, 4325, 4318, 4327, 4324, 4317, 4326, 4319, 4332, 4309, 4334, 4311, 4340, 4301, 4342, 4303, 4348, 4293, 4350, 4295, 4288, 4345, 4290, 4347, 4296, 4337, 4298, 4339, 4304, 4329, 4306, 4331, 4312, 4321, 4314, 4323, 4320, 4313, 4322, 4315, 4328, 4305, 4330, 4307, 4336, 4297, 4338, 4299, 4344, 4289, 4346, 4291, 4356, 4413, 4358, 4415, 4364, 4405, 4366, 4407, 4372, 4397, 4374, 4399, 4380, 4389, 4382, 4391, 4388, 4381, 4390, 4383, 4396, 4373, 4398, 4375, 4404, 4365, 4406, 4367, 4412, 4357, 4414, 4359, 4352, 4409, 4354, 4411, 4360, 4401, 4362, 4403, 4368, 4393, 4370, 4395, 4376, 4385, 4378, 4387, 4384, 4377, 4386, 4379, 4392, 4369, 4394, 4371, 4400, 4361, 4402, 4363, 4408, 4353, 4410, 4355, 4420, 4477, 4422, 4479, 4428, 4469, 4430, 4471, 4436, 4461, 4438, 4463, 4444, 4453, 4446, 4455, 4452, 4445, 4454, 4447, 4460, 4437, 4462, 4439, 4468, 4429, 4470, 4431, 4476, 4421, 4478, 4423, 4416, 4473, 4418, 4475, 4424, 4465, 4426, 4467, 4432, 4457, 4434, 4459, 4440, 4449, 4442, 4451, 4448, 4441, 4450, 4443, 4456, 4433, 4458, 4435, 4464, 4425, 4466, 4427, 4472, 4417, 4474, 4419, 4484, 4541, 4486, 4543, 4492, 4533, 4494, 4535, 4500, 4525, 4502, 4527, 4508, 4517, 4510, 4519, 4516, 4509, 4518, 4511, 4524, 4501, 4526, 4503, 4532, 4493, 4534, 4495, 4540, 4485, 4542, 4487, 4480, 4537, 4482, 4539, 4488, 4529, 4490, 4531, 4496, 4521, 4498, 4523, 4504, 4513, 4506, 4515, 4512, 4505, 4514, 4507, 4520, 4497, 4522, 4499, 4528, 4489, 4530, 4491, 4536, 4481, 4538, 4483, 4548, 4605, 4550, 4607, 4556, 4597, 4558, 4599, 4564, 4589, 4566, 4591, 4572, 4581, 4574, 4583, 4580, 4573, 4582, 4575, 4588, 4565, 4590, 4567, 4596, 4557, 4598, 4559, 4604, 4549, 4606, 4551, 4544, 4601, 4546, 4603, 4552, 4593, 4554, 4595, 4560, 4585, 4562, 4587, 4568, 4577, 4570, 4579, 4576, 4569, 4578, 4571, 4584, 4561, 4586, 4563, 4592, 4553, 4594, 4555, 4600, 4545, 4602, 4547 };

    const int bank12[] = { 4610, 4667, 4608, 4665, 4618, 4659, 4616, 4657, 4626, 4651, 4624, 4649, 4634, 4643, 4632, 4641, 4642, 4635, 4640, 4633, 4650, 4627, 4648, 4625, 4658, 4619, 4656, 4617, 4666, 4611, 4664, 4609, 4614, 4671, 4612, 4669, 4622, 4663, 4620, 4661, 4630, 4655, 4628, 4653, 4638, 4647, 4636, 4645, 4646, 4639, 4644, 4637, 4654, 4631, 4652, 4629, 4662, 4623, 4660, 4621, 4670, 4615, 4668, 4613, 4674, 4731, 4672, 4729, 4682, 4723, 4680, 4721, 4690, 4715, 4688, 4713, 4698, 4707, 4696, 4705, 4706, 4699, 4704, 4697, 4714, 4691, 4712, 4689, 4722, 4683, 4720, 4681, 4730, 4675, 4728, 4673, 4678, 4735, 4676, 4733, 4686, 4727, 4684, 4725, 4694, 4719, 4692, 4717, 4702, 4711, 4700, 4709, 4710, 4703, 4708, 4701, 4718, 4695, 4716, 4693, 4726, 4687, 4724, 4685, 4734, 4679, 4732, 4677, 4738, 4795, 4736, 4793, 4746, 4787, 4744, 4785, 4754, 4779, 4752, 4777, 4762, 4771, 4760, 4769, 4770, 4763, 4768, 4761, 4778, 4755, 4776, 4753, 4786, 4747, 4784, 4745, 4794, 4739, 4792, 4737, 4742, 4799, 4740, 4797, 4750, 4791, 4748, 4789, 4758, 4783, 4756, 4781, 4766, 4775, 4764, 4773, 4774, 4767, 4772, 4765, 4782, 4759, 4780, 4757, 4790, 4751, 4788, 4749, 4798, 4743, 4796, 4741, 4802, 4859, 4800, 4857, 4810, 4851, 4808, 4849, 4818, 4843, 4816, 4841, 4826, 4835, 4824, 4833, 4834, 4827, 4832, 4825, 4842, 4819, 4840, 4817, 4850, 4811, 4848, 4809, 4858, 4803, 4856, 4801, 4806, 4863, 4804, 4861, 4814, 4855, 4812, 4853, 4822, 4847, 4820, 4845, 4830, 4839, 4828, 4837, 4838, 4831, 4836, 4829, 4846, 4823, 4844, 4821, 4854, 4815, 4852, 4813, 4862, 4807, 4860, 4805, 4866, 4923, 4864, 4921, 4874, 4915, 4872, 4913, 4882, 4907, 4880, 4905, 4890, 4899, 4888, 4897, 4898, 4891, 4896, 4889, 4906, 4883, 4904, 4881, 4914, 4875, 4912, 4873, 4922, 4867, 4920, 4865, 4870, 4927, 4868, 4925, 4878, 4919, 4876, 4917, 4886, 4911, 4884, 4909, 4894, 4903, 4892, 4901, 4902, 4895, 4900, 4893, 4910, 4887, 4908, 4885, 4918, 4879, 4916, 4877, 4926, 4871, 4924, 4869, 4930, 4987, 4928, 4985, 4938, 4979, 4936, 4977, 4946, 4971, 4944, 4969, 4954, 4963, 4952, 4961, 4962, 4955, 4960, 4953, 4970, 4947, 4968, 4945, 4978, 4939, 4976, 4937, 4986, 4931, 4984, 4929, 4934, 4991, 4932, 4989, 4942, 4983, 4940, 4981, 4950, 4975, 4948, 4973, 4958, 4967, 4956, 4965, 4966, 4959, 4964, 4957, 4974, 4951, 4972, 4949, 4982, 4943, 4980, 4941, 4990, 4935, 4988, 4933 };

    const int bank13[] = { 4998, 5055, 4996, 5053, 5006, 5047, 5004, 5045, 5014, 5039, 5012, 5037, 5022, 5031, 5020, 5029, 5030, 5023, 5028, 5021, 5038, 5015, 5036, 5013, 5046, 5007, 5044, 5005, 5054, 4999, 5052, 4997, 4994, 5051, 4992, 5049, 5002, 5043, 5000, 5041, 5010, 5035, 5008, 5033, 5018, 5027, 5016, 5025, 5026, 5019, 5024, 5017, 5034, 5011, 5032, 5009, 5042, 5003, 5040, 5001, 5050, 4995, 5048, 4993, 5062, 5119, 5060, 5117, 5070, 5111, 5068, 5109, 5078, 5103, 5076, 5101, 5086, 5095, 5084, 5093, 5094, 5087, 5092, 5085, 5102, 5079, 5100, 5077, 5110, 5071, 5108, 5069, 5118, 5063, 5116, 5061, 5058, 5115, 5056, 5113, 5066, 5107, 5064, 5105, 5074, 5099, 5072, 5097, 5082, 5091, 5080, 5089, 5090, 5083, 5088, 5081, 5098, 5075, 5096, 5073, 5106, 5067, 5104, 5065, 5114, 5059, 5112, 5057, 5126, 5183, 5124, 5181, 5134, 5175, 5132, 5173, 5142, 5167, 5140, 5165, 5150, 5159, 5148, 5157, 5158, 5151, 5156, 5149, 5166, 5143, 5164, 5141, 5174, 5135, 5172, 5133, 5182, 5127, 5180, 5125, 5122, 5179, 5120, 5177, 5130, 5171, 5128, 5169, 5138, 5163, 5136, 5161, 5146, 5155, 5144, 5153, 5154, 5147, 5152, 5145, 5162, 5139, 5160, 5137, 5170, 5131, 5168, 5129, 5178, 5123, 5176, 5121, 5190, 5247, 5188, 5245, 5198, 5239, 5196, 5237, 5206, 5231, 5204, 5229, 5214, 5223, 5212, 5221, 5222, 5215, 5220, 5213, 5230, 5207, 5228, 5205, 5238, 5199, 5236, 5197, 5246, 5191, 5244, 5189, 5186, 5243, 5184, 5241, 5194, 5235, 5192, 5233, 5202, 5227, 5200, 5225, 5210, 5219, 5208, 5217, 5218, 5211, 5216, 5209, 5226, 5203, 5224, 5201, 5234, 5195, 5232, 5193, 5242, 5187, 5240, 5185, 5254, 5311, 5252, 5309, 5262, 5303, 5260, 5301, 5270, 5295, 5268, 5293, 5278, 5287, 5276, 5285, 5286, 5279, 5284, 5277, 5294, 5271, 5292, 5269, 5302, 5263, 5300, 5261, 5310, 5255, 5308, 5253, 5250, 5307, 5248, 5305, 5258, 5299, 5256, 5297, 5266, 5291, 5264, 5289, 5274, 5283, 5272, 5281, 5282, 5275, 5280, 5273, 5290, 5267, 5288, 5265, 5298, 5259, 5296, 5257, 5306, 5251, 5304, 5249, 5318, 5375, 5316, 5373, 5326, 5367, 5324, 5365, 5334, 5359, 5332, 5357, 5342, 5351, 5340, 5349, 5350, 5343, 5348, 5341, 5358, 5335, 5356, 5333, 5366, 5327, 5364, 5325, 5374, 5319, 5372, 5317, 5314, 5371, 5312, 5369, 5322, 5363, 5320, 5361, 5330, 5355, 5328, 5353, 5338, 5347, 5336, 5345, 5346, 5339, 5344, 5337, 5354, 5331, 5352, 5329, 5362, 5323, 5360, 5321, 5370, 5315, 5368, 5313 };

    const int bank14[] = { 5378, 5435, 5376, 5433, 5386, 5427, 5384, 5425, 5394, 5419, 5392, 5417, 5402, 5411, 5400, 5409, 5410, 5403, 5408, 5401, 5418, 5395, 5416, 5393, 5426, 5387, 5424, 5385, 5434, 5379, 5432, 5377, 5382, 5439, 5380, 5437, 5390, 5431, 5388, 5429, 5398, 5423, 5396, 5421, 5406, 5415, 5404, 5413, 5414, 5407, 5412, 5405, 5422, 5399, 5420, 5397, 5430, 5391, 5428, 5389, 5438, 5383, 5436, 5381, 5442, 5499, 5440, 5497, 5450, 5491, 5448, 5489, 5458, 5483, 5456, 5481, 5466, 5475, 5464, 5473, 5474, 5467, 5472, 5465, 5482, 5459, 5480, 5457, 5490, 5451, 5488, 5449, 5498, 5443, 5496, 5441, 5446, 5503, 5444, 5501, 5454, 5495, 5452, 5493, 5462, 5487, 5460, 5485, 5470, 5479, 5468, 5477, 5478, 5471, 5476, 5469, 5486, 5463, 5484, 5461, 5494, 5455, 5492, 5453, 5502, 5447, 5500, 5445, 5506, 5563, 5504, 5561, 5514, 5555, 5512, 5553, 5522, 5547, 5520, 5545, 5530, 5539, 5528, 5537, 5538, 5531, 5536, 5529, 5546, 5523, 5544, 5521, 5554, 5515, 5552, 5513, 5562, 5507, 5560, 5505, 5510, 5567, 5508, 5565, 5518, 5559, 5516, 5557, 5526, 5551, 5524, 5549, 5534, 5543, 5532, 5541, 5542, 5535, 5540, 5533, 5550, 5527, 5548, 5525, 5558, 5519, 5556, 5517, 5566, 5511, 5564, 5509, 5570, 5627, 5568, 5625, 5578, 5619, 5576, 5617, 5586, 5611, 5584, 5609, 5594, 5603, 5592, 5601, 5602, 5595, 5600, 5593, 5610, 5587, 5608, 5585, 5618, 5579, 5616, 5577, 5626, 5571, 5624, 5569, 5574, 5631, 5572, 5629, 5582, 5623, 5580, 5621, 5590, 5615, 5588, 5613, 5598, 5607, 5596, 5605, 5606, 5599, 5604, 5597, 5614, 5591, 5612, 5589, 5622, 5583, 5620, 5581, 5630, 5575, 5628, 5573, 5634, 5691, 5632, 5689, 5642, 5683, 5640, 5681, 5650, 5675, 5648, 5673, 5658, 5667, 5656, 5665, 5666, 5659, 5664, 5657, 5674, 5651, 5672, 5649, 5682, 5643, 5680, 5641, 5690, 5635, 5688, 5633, 5638, 5695, 5636, 5693, 5646, 5687, 5644, 5685, 5654, 5679, 5652, 5677, 5662, 5671, 5660, 5669, 5670, 5663, 5668, 5661, 5678, 5655, 5676, 5653, 5686, 5647, 5684, 5645, 5694, 5639, 5692, 5637, 5698, 5755, 5696, 5753, 5706, 5747, 5704, 5745, 5714, 5739, 5712, 5737, 5722, 5731, 5720, 5729, 5730, 5723, 5728, 5721, 5738, 5715, 5736, 5713, 5746, 5707, 5744, 5705, 5754, 5699, 5752, 5697, 5702, 5759, 5700, 5757, 5710, 5751, 5708, 5749, 5718, 5743, 5716, 5741, 5726, 5735, 5724, 5733, 5734, 5727, 5732, 5725, 5742, 5719, 5740, 5717, 5750, 5711, 5748, 5709, 5758, 5703, 5756, 5701 };

    const int bank15[] = { 5766, 5823, 5764, 5821, 5774, 5815, 5772, 5813, 5782, 5807, 5780, 5805, 5790, 5799, 5788, 5797, 5798, 5791, 5796, 5789, 5806, 5783, 5804, 5781, 5814, 5775, 5812, 5773, 5822, 5767, 5820, 5765, 5762, 5819, 5760, 5817, 5770, 5811, 5768, 5809, 5778, 5803, 5776, 5801, 5786, 5795, 5784, 5793, 5794, 5787, 5792, 5785, 5802, 5779, 5800, 5777, 5810, 5771, 5808, 5769, 5818, 5763, 5816, 5761, 5830, 5887, 5828, 5885, 5838, 5879, 5836, 5877, 5846, 5871, 5844, 5869, 5854, 5863, 5852, 5861, 5862, 5855, 5860, 5853, 5870, 5847, 5868, 5845, 5878, 5839, 5876, 5837, 5886, 5831, 5884, 5829, 5826, 5883, 5824, 5881, 5834, 5875, 5832, 5873, 5842, 5867, 5840, 5865, 5850, 5859, 5848, 5857, 5858, 5851, 5856, 5849, 5866, 5843, 5864, 5841, 5874, 5835, 5872, 5833, 5882, 5827, 5880, 5825, 5894, 5951, 5892, 5949, 5902, 5943, 5900, 5941, 5910, 5935, 5908, 5933, 5918, 5927, 5916, 5925, 5926, 5919, 5924, 5917, 5934, 5911, 5932, 5909, 5942, 5903, 5940, 5901, 5950, 5895, 5948, 5893, 5890, 5947, 5888, 5945, 5898, 5939, 5896, 5937, 5906, 5931, 5904, 5929, 5914, 5923, 5912, 5921, 5922, 5915, 5920, 5913, 5930, 5907, 5928, 5905, 5938, 5899, 5936, 5897, 5946, 5891, 5944, 5889, 5958, 6015, 5956, 6013, 5966, 6007, 5964, 6005, 5974, 5999, 5972, 5997, 5982, 5991, 5980, 5989, 5990, 5983, 5988, 5981, 5998, 5975, 5996, 5973, 6006, 5967, 6004, 5965, 6014, 5959, 6012, 5957, 5954, 6011, 5952, 6009, 5962, 6003, 5960, 6001, 5970, 5995, 5968, 5993, 5978, 5987, 5976, 5985, 5986, 5979, 5984, 5977, 5994, 5971, 5992, 5969, 6002, 5963, 6000, 5961, 6010, 5955, 6008, 5953, 6022, 6079, 6020, 6077, 6030, 6071, 6028, 6069, 6038, 6063, 6036, 6061, 6046, 6055, 6044, 6053, 6054, 6047, 6052, 6045, 6062, 6039, 6060, 6037, 6070, 6031, 6068, 6029, 6078, 6023, 6076, 6021, 6018, 6075, 6016, 6073, 6026, 6067, 6024, 6065, 6034, 6059, 6032, 6057, 6042, 6051, 6040, 6049, 6050, 6043, 6048, 6041, 6058, 6035, 6056, 6033, 6066, 6027, 6064, 6025, 6074, 6019, 6072, 6017, 6086, 6143, 6084, 6141, 6094, 6135, 6092, 6133, 6102, 6127, 6100, 6125, 6110, 6119, 6108, 6117, 6118, 6111, 6116, 6109, 6126, 6103, 6124, 6101, 6134, 6095, 6132, 6093, 6142, 6087, 6140, 6085, 6082, 6139, 6080, 6137, 6090, 6131, 6088, 6129, 6098, 6123, 6096, 6121, 6106, 6115, 6104, 6113, 6114, 6107, 6112, 6105, 6122, 6099, 6120, 6097, 6130, 6091, 6128, 6089, 6138, 6083, 6136, 6081 };

    const int tiphalf[] = { 388, 1213, 1924, 2749, 396, 1205, 1932, 2741, 404, 1197, 1940, 2733, 412, 1189, 1948, 2725, 420, 1181, 1956, 2717, 428, 1173, 1964, 2709, 436, 1165, 1972, 2701, 444, 1157, 1980, 2693, 4, 829, 1540, 2365, 12, 821, 1548, 2357, 20, 813, 1556, 2349, 28, 805, 1564, 2341, 36, 797, 1572, 2333, 44, 789, 1580, 2325, 52, 781, 1588, 2317, 60, 773, 1596, 2309, 452, 1277, 1988, 2813, 460, 1269, 1996, 2805, 468, 1261, 2004, 2797, 476, 1253, 2012, 2789, 484, 1245, 2020, 2781, 492, 1237, 2028, 2773, 500, 1229, 2036, 2765, 508, 1221, 2044, 2757, 68, 893, 1604, 2429, 76, 885, 1612, 2421, 84, 877, 1620, 2413, 92, 869, 1628, 2405, 100, 861, 1636, 2397, 108, 853, 1644, 2389, 116, 845, 1652, 2381, 124, 837, 1660, 2373, 516, 1341, 2052, 2877, 524, 1333, 2060, 2869, 532, 1325, 2068, 2861, 540, 1317, 2076, 2853, 548, 1309, 2084, 2845, 556, 1301, 2092, 2837, 564, 1293, 2100, 2829, 572, 1285, 2108, 2821, 132, 957, 1668, 2493, 140, 949, 1676, 2485, 148, 941, 1684, 2477, 156, 933, 1692, 2469, 164, 925, 1700, 2461, 172, 917, 1708, 2453, 180, 909, 1716, 2445, 188, 901, 1724, 2437, 580, 1405, 2116, 2941, 588, 1397, 2124, 2933, 596, 1389, 2132, 2925, 604, 1381, 2140, 2917, 612, 1373, 2148, 2909, 620, 1365, 2156, 2901, 628, 1357, 2164, 2893, 636, 1349, 2172, 2885, 196, 1021, 1732, 2557, 204, 1013, 1740, 2549, 212, 1005, 1748, 2541, 220, 997, 1756, 2533, 228, 989, 1764, 2525, 236, 981, 1772, 2517, 244, 973, 1780, 2509, 252, 965, 1788, 2501, 644, 1469, 2180, 3005, 652, 1461, 2188, 2997, 660, 1453, 2196, 2989, 668, 1445, 2204, 2981, 676, 1437, 2212, 2973, 684, 1429, 2220, 2965, 692, 1421, 2228, 2957, 700, 1413, 2236, 2949, 260, 1085, 1796, 2621, 268, 1077, 1804, 2613, 276, 1069, 1812, 2605, 284, 1061, 1820, 2597, 292, 1053, 1828, 2589, 300, 1045, 1836, 2581, 308, 1037, 1844, 2573, 316, 1029, 1852, 2565, 708, 1533, 2244, 3069, 716, 1525, 2252, 3061, 724, 1517, 2260, 3053, 732, 1509, 2268, 3045, 740, 1501, 2276, 3037, 748, 1493, 2284, 3029, 756, 1485, 2292, 3021, 764, 1477, 2300, 3013, 324, 1149, 1860, 2685, 332, 1141, 1868, 2677, 340, 1133, 1876, 2669, 348, 1125, 1884, 2661, 356, 1117, 1892, 2653, 364, 1109, 1900, 2645, 372, 1101, 1908, 2637, 380, 1093, 1916, 2629 };

    const int basehalf[] = { 3460, 4285, 4996, 5821, 3468, 4277, 5004, 5813, 3476, 4269, 5012, 5805, 3484, 4261, 5020, 5797, 3492, 4253, 5028, 5789, 3500, 4245, 5036, 5781, 3508, 4237, 5044, 5773, 3516, 4229, 5052, 5765, 3076, 3901, 4612, 5437, 3084, 3893, 4620, 5429, 3092, 3885, 4628, 5421, 3100, 3877, 4636, 5413, 3108, 3869, 4644, 5405, 3116, 3861, 4652, 5397, 3124, 3853, 4660, 5389, 3132, 3845, 4668, 5381, 3524, 4349, 5060, 5885, 3532, 4341, 5068, 5877, 3540, 4333, 5076, 5869, 3548, 4325, 5084, 5861, 3556, 4317, 5092, 5853, 3564, 4309, 5100, 5845, 3572, 4301, 5108, 5837, 3580, 4293, 5116, 5829, 3140, 3965, 4676, 5501, 3148, 3957, 4684, 5493, 3156, 3949, 4692, 5485, 3164, 3941, 4700, 5477, 3172, 3933, 4708, 5469, 3180, 3925, 4716, 5461, 3188, 3917, 4724, 5453, 3196, 3909, 4732, 5445, 3588, 4413, 5124, 5949, 3596, 4405, 5132, 5941, 3604, 4397, 5140, 5933, 3612, 4389, 5148, 5925, 3620, 4381, 5156, 5917, 3628, 4373, 5164, 5909, 3636, 4365, 5172, 5901, 3644, 4357, 5180, 5893, 3204, 4029, 4740, 5565, 3212, 4021, 4748, 5557, 3220, 4013, 4756, 5549, 3228, 4005, 4764, 5541, 3236, 3997, 4772, 5533, 3244, 3989, 4780, 5525, 3252, 3981, 4788, 5517, 3260, 3973, 4796, 5509, 3652, 4477, 5188, 6013, 3660, 4469, 5196, 6005, 3668, 4461, 5204, 5997, 3676, 4453, 5212, 5989, 3684, 4445, 5220, 5981, 3692, 4437, 5228, 5973, 3700, 4429, 5236, 5965, 3708, 4421, 5244, 5957, 3268, 4093, 4804, 5629, 3276, 4085, 4812, 5621, 3284, 4077, 4820, 5613, 3292, 4069, 4828, 5605, 3300, 4061, 4836, 5597, 3308, 4053, 4844, 5589, 3316, 4045, 4852, 5581, 3324, 4037, 4860, 5573, 3716, 4541, 5252, 6077, 3724, 4533, 5260, 6069, 3732, 4525, 5268, 6061, 3740, 4517, 5276, 6053, 3748, 4509, 5284, 6045, 3756, 4501, 5292, 6037, 3764, 4493, 5300, 6029, 3772, 4485, 5308, 6021, 3332, 4157, 4868, 5693, 3340, 4149, 4876, 5685, 3348, 4141, 4884, 5677, 3356, 4133, 4892, 5669, 3364, 4125, 4900, 5661, 3372, 4117, 4908, 5653, 3380, 4109, 4916, 5645, 3388, 4101, 4924, 5637, 3780, 4605, 5316, 6141, 3788, 4597, 5324, 6133, 3796, 4589, 5332, 6125, 3804, 4581, 5340, 6117, 3812, 4573, 5348, 6109, 3820, 4565, 5356, 6101, 3828, 4557, 5364, 6093, 3836, 4549, 5372, 6085, 3396, 4221, 4932, 5757, 3404, 4213, 4940, 5749, 3412, 4205, 4948, 5741, 3420, 4197, 4956, 5733, 3428, 4189, 4964, 5725, 3436, 4181, 4972, 5717, 3444, 4173, 4980, 5709, 3452, 4165, 4988, 5701 };

    const int _2x192[] = { 388, 1213, 390, 1215, 396, 1205, 398, 1207, 404, 1197, 406, 1199, 412, 1189, 414, 1191, 420, 1181, 422, 1183, 428, 1173, 430, 1175, 436, 1165, 438, 1167, 444, 1157, 446, 1159, 4, 829, 6, 831, 12, 821, 14, 823, 20, 813, 22, 815, 28, 805, 30, 807, 36, 797, 38, 799, 44, 789, 46, 791, 52, 781, 54, 783, 60, 773, 62, 775, 452, 1277, 454, 1279, 460, 1269, 462, 1271, 468, 1261, 470, 1263, 476, 1253, 478, 1255, 484, 1245, 486, 1247, 492, 1237, 494, 1239, 500, 1229, 502, 1231, 508, 1221, 510, 1223, 68, 893, 70, 895, 76, 885, 78, 887, 84, 877, 86, 879, 92, 869, 94, 871, 100, 861, 102, 863, 108, 853, 110, 855, 116, 845, 118, 847, 124, 837, 126, 839, 516, 1341, 518, 1343, 524, 1333, 526, 1335, 532, 1325, 534, 1327, 540, 1317, 542, 1319, 548, 1309, 550, 1311, 556, 1301, 558, 1303, 564, 1293, 566, 1295, 572, 1285, 574, 1287, 132, 957, 134, 959, 140, 949, 142, 951, 148, 941, 150, 943, 156, 933, 158, 935, 164, 925, 166, 927, 172, 917, 174, 919, 180, 909, 182, 911, 188, 901, 190, 903, 580, 1405, 582, 1407, 588, 1397, 590, 1399, 596, 1389, 598, 1391, 604, 1381, 606, 1383, 612, 1373, 614, 1375, 620, 1365, 622, 1367, 628, 1357, 630, 1359, 636, 1349, 638, 1351, 196, 1021, 198, 1023, 204, 1013, 206, 1015, 212, 1005, 214, 1007, 220, 997, 222, 999, 228, 989, 230, 991, 236, 981, 238, 983, 244, 973, 246, 975, 252, 965, 254, 967, 644, 1469, 646, 1471, 652, 1461, 654, 1463, 660, 1453, 662, 1455, 668, 1445, 670, 1447, 676, 1437, 678, 1439, 684, 1429, 686, 1431, 692, 1421, 694, 1423, 700, 1413, 702, 1415, 260, 1085, 262, 1087, 268, 1077, 270, 1079, 276, 1069, 278, 1071, 284, 1061, 286, 1063, 292, 1053, 294, 1055, 300, 1045, 302, 1047, 308, 1037, 310, 1039, 316, 1029, 318, 1031, 708, 1533, 710, 1535, 716, 1525, 718, 1527, 724, 1517, 726, 1519, 732, 1509, 734, 1511, 740, 1501, 742, 1503, 748, 1493, 750, 1495, 756, 1485, 758, 1487, 764, 1477, 766, 1479, 324, 1149, 326, 1151, 332, 1141, 334, 1143, 340, 1133, 342, 1135, 348, 1125, 350, 1127, 356, 1117, 358, 1119, 364, 1109, 366, 1111, 372, 1101, 374, 1103, 380, 1093, 382, 1095 };

    const int _4x96[] = { 388, 445, 390, 447, 396, 437, 398, 439, 404, 429, 406, 431, 412, 421, 414, 423, 420, 413, 422, 415, 428, 405, 430, 407, 436, 397, 438, 399, 444, 389, 446, 391, 4, 61, 6, 63, 12, 53, 14, 55, 20, 45, 22, 47, 28, 37, 30, 39, 36, 29, 38, 31, 44, 21, 46, 23, 52, 13, 54, 15, 60, 5, 62, 7, 452, 509, 454, 511, 460, 501, 462, 503, 468, 493, 470, 495, 476, 485, 478, 487, 484, 477, 486, 479, 492, 469, 494, 471, 500, 461, 502, 463, 508, 453, 510, 455, 68, 125, 70, 127, 76, 117, 78, 119, 84, 109, 86, 111, 92, 101, 94, 103, 100, 93, 102, 95, 108, 85, 110, 87, 116, 77, 118, 79, 124, 69, 126, 71, 516, 573, 518, 575, 524, 565, 526, 567, 532, 557, 534, 559, 540, 549, 542, 551, 548, 541, 550, 543, 556, 533, 558, 535, 564, 525, 566, 527, 572, 517, 574, 519, 132, 189, 134, 191, 140, 181, 142, 183, 148, 173, 150, 175, 156, 165, 158, 167, 164, 157, 166, 159, 172, 149, 174, 151, 180, 141, 182, 143, 188, 133, 190, 135, 580, 637, 582, 639, 588, 629, 590, 631, 596, 621, 598, 623, 604, 613, 606, 615, 612, 605, 614, 607, 620, 597, 622, 599, 628, 589, 630, 591, 636, 581, 638, 583, 196, 253, 198, 255, 204, 245, 206, 247, 212, 237, 214, 239, 220, 229, 222, 231, 228, 221, 230, 223, 236, 213, 238, 215, 244, 205, 246, 207, 252, 197, 254, 199, 644, 701, 646, 703, 652, 693, 654, 695, 660, 685, 662, 687, 668, 677, 670, 679, 676, 669, 678, 671, 684, 661, 686, 663, 692, 653, 694, 655, 700, 645, 702, 647, 260, 317, 262, 319, 268, 309, 270, 311, 276, 301, 278, 303, 284, 293, 286, 295, 292, 285, 294, 287, 300, 277, 302, 279, 308, 269, 310, 271, 316, 261, 318, 263, 708, 765, 710, 767, 716, 757, 718, 759, 724, 749, 726, 751, 732, 741, 734, 743, 740, 733, 742, 735, 748, 725, 750, 727, 756, 717, 758, 719, 764, 709, 766, 711, 324, 381, 326, 383, 332, 373, 334, 375, 340, 365, 342, 367, 348, 357, 350, 359, 356, 349, 358, 351, 364, 341, 366, 343, 372, 333, 374, 335, 380, 325, 382, 327 };

    const int _2x2x96[] = { 0, 57, 2, 59, 8, 49, 10, 51, 16, 41, 18, 43, 24, 33, 26, 35, 32, 25, 34, 27, 40, 17, 42, 19, 48, 9, 50, 11, 56, 1, 58, 3, 384, 441, 386, 443, 392, 433, 394, 435, 400, 425, 402, 427, 408, 417, 410, 419, 416, 409, 418, 411, 424, 401, 426, 403, 432, 393, 434, 395, 440, 385, 442, 387, 64, 121, 66, 123, 72, 113, 74, 115, 80, 105, 82, 107, 88, 97, 90, 99, 96, 89, 98, 91, 104, 81, 106, 83, 112, 73, 114, 75, 120, 65, 122, 67, 448, 505, 450, 507, 456, 497, 458, 499, 464, 489, 466, 491, 472, 481, 474, 483, 480, 473, 482, 475, 488, 465, 490, 467, 496, 457, 498, 459, 504, 449, 506, 451, 128, 185, 130, 187, 136, 177, 138, 179, 144, 169, 146, 171, 152, 161, 154, 163, 160, 153, 162, 155, 168, 145, 170, 147, 176, 137, 178, 139, 184, 129, 186, 131, 512, 569, 514, 571, 520, 561, 522, 563, 528, 553, 530, 555, 536, 545, 538, 547, 544, 537, 546, 539, 552, 529, 554, 531, 560, 521, 562, 523, 568, 513, 570, 515, 192, 249, 194, 251, 200, 241, 202, 243, 208, 233, 210, 235, 216, 225, 218, 227, 224, 217, 226, 219, 232, 209, 234, 211, 240, 201, 242, 203, 248, 193, 250, 195, 576, 633, 578, 635, 584, 625, 586, 627, 592, 617, 594, 619, 600, 609, 602, 611, 608, 601, 610, 603, 616, 593, 618, 595, 624, 585, 626, 587, 632, 577, 634, 579, 256, 313, 258, 315, 264, 305, 266, 307, 272, 297, 274, 299, 280, 289, 282, 291, 288, 281, 290, 283, 296, 273, 298, 275, 304, 265, 306, 267, 312, 257, 314, 259, 640, 697, 642, 699, 648, 689, 650, 691, 656, 681, 658, 683, 664, 673, 666, 675, 672, 665, 674, 667, 680, 657, 682, 659, 688, 649, 690, 651, 696, 641, 698, 643, 320, 377, 322, 379, 328, 369, 330, 371, 336, 361, 338, 363, 344, 353, 346, 355, 352, 345, 354, 347, 360, 337, 362, 339, 368, 329, 370, 331, 376, 321, 378, 323, 704, 761, 706, 763, 712, 753, 714, 755, 720, 745, 722, 747, 728, 737, 730, 739, 736, 729, 738, 731, 744, 721, 746, 723, 752, 713, 754, 715, 760, 705, 762, 707 };

    electrodeConfigurations.add (new Array<int> (bank0, 384));
    electrodeConfigurations.add (new Array<int> (bank1, 384));
    electrodeConfigurations.add (new Array<int> (bank2, 384));
    electrodeConfigurations.add (new Array<int> (bank3, 384));
    electrodeConfigurations.add (new Array<int> (bank4, 384));
    electrodeConfigurations.add (new Array<int> (bank5, 384));
    electrodeConfigurations.add (new Array<int> (bank6, 384));
    electrodeConfigurations.add (new Array<int> (bank7, 384));
    electrodeConfigurations.add (new Array<int> (bank8, 384));
    electrodeConfigurations.add (new Array<int> (bank9, 384));
    electrodeConfigurations.add (new Array<int> (bank10, 384));
    electrodeConfigurations.add (new Array<int> (bank11, 384));
    electrodeConfigurations.add (new Array<int> (bank12, 384));
    electrodeConfigurations.add (new Array<int> (bank13, 384));
    electrodeConfigurations.add (new Array<int> (bank14, 384));
    electrodeConfigurations.add (new Array<int> (bank15, 384));
    electrodeConfigurations.add (new Array<int> (tiphalf, 384));
    electrodeConfigurations.add (new Array<int> (basehalf, 384));
    electrodeConfigurations.add (new Array<int> (_2x192, 384));
    electrodeConfigurations.add (new Array<int> (_4x96, 384));
    electrodeConfigurations.add (new Array<int> (_2x2x96, 384));
}