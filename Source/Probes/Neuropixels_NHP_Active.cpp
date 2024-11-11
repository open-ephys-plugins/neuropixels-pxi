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

#include "Neuropixels_NHP_Active.h"
#include "Geometry.h"

#include "../NeuropixThread.h"

#define MAXLEN 50

void Neuropixels_NHP_Active::getInfo()
{
    checkError (Neuropixels::readProbeSN (basestation->slot,
                                          headstage->port,
                                          dock,
                                          &info.serial_number),
                "readProbeSN");

    char pn[MAXLEN];
    checkError (Neuropixels::readProbePN (basestation->slot,
                                          headstage->port,
                                          dock,
                                          pn,
                                          MAXLEN),
                "readProbePN");

    info.part_number = String (pn);
}

Neuropixels_NHP_Active::Neuropixels_NHP_Active (Basestation* bs, Headstage* hs, Flex* fl) : Probe (bs, hs, fl, 0)
{
    getInfo();

    setStatus (SourceStatus::DISCONNECTED);

    customName.probeSpecific = String (info.serial_number);

    Geometry::forPartNumber (info.part_number, electrodeMetadata, probeMetadata);

    name = probeMetadata.name;
    type = probeMetadata.type;

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

    settings.availableElectrodeConfigurations.add ("Bank A");
    settings.availableElectrodeConfigurations.add ("Bank B");
    settings.availableElectrodeConfigurations.add ("Bank C");

    if (type == ProbeType::NHP25 || type == ProbeType::NHP45)
    {
        settings.availableElectrodeConfigurations.add ("Bank D");
        settings.availableElectrodeConfigurations.add ("Bank E");
        settings.availableElectrodeConfigurations.add ("Bank F");
        settings.availableElectrodeConfigurations.add ("Bank G");

        if (type == ProbeType::NHP45)
        {
            settings.availableElectrodeConfigurations.add ("Bank H");
            settings.availableElectrodeConfigurations.add ("Bank I");
            settings.availableElectrodeConfigurations.add ("Bank J");
            settings.availableElectrodeConfigurations.add ("Bank K");
            settings.availableElectrodeConfigurations.add ("Bank L");
        }
    }

    settings.availableElectrodeConfigurations.add ("Single Column");
    settings.availableElectrodeConfigurations.add ("Tetrodes");

    open();
}

bool Neuropixels_NHP_Active::open()
{
    errorCode = Neuropixels::openProbe (basestation->slot, headstage->port, dock);
    LOGD ("openProbe: slot: ", basestation->slot, " port: ", headstage->port, " dock: ", dock, " errorCode: ", errorCode);

    ap_timestamp = 0;
    lfp_timestamp = 0;
    eventCode = 0;

    apView = std::make_unique<ActivityView> (384, 3000);
    lfpView = std::make_unique<ActivityView> (384, 250);

    return errorCode == Neuropixels::SUCCESS;
}

bool Neuropixels_NHP_Active::close()
{
    errorCode = Neuropixels::closeProbe (basestation->slot, headstage->port, dock);
    LOGD ("closeProbe: slot: ", basestation->slot, " port: ", headstage->port, " dock: ", dock, " errorCode: ", errorCode);

    return errorCode == Neuropixels::SUCCESS;
}

void Neuropixels_NHP_Active::initialize (bool signalChainIsLoading)
{
    errorCode = Neuropixels::init (basestation->slot, headstage->port, dock);
    LOGD ("init: slot: ", basestation->slot, " port: ", headstage->port, " dock: ", dock, " errorCode: ", errorCode);

    errorCode = Neuropixels::setOPMODE (basestation->slot, headstage->port, dock, Neuropixels::RECORDING);
    LOGD ("setOPMODE: slot: ", basestation->slot, " port: ", headstage->port, " dock: ", dock, " errorCode: ", errorCode);

    errorCode = Neuropixels::setHSLed (basestation->slot, headstage->port, false);
    LOGD ("setHSLed: slot: ", basestation->slot, " port: ", headstage->port, " dock: ", dock, " errorCode: ", errorCode);
}

void Neuropixels_NHP_Active::calibrate()
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

void Neuropixels_NHP_Active::printSettings()
{
    int apGainIndex;
    int lfpGainIndex;

    Neuropixels::getGain (basestation->slot, headstage->port, dock, 32, &apGainIndex, &lfpGainIndex);

    LOGD ("Current settings for probe on slot: ", basestation->slot, " port: ", headstage->port, " dock: ", dock, " AP=", settings.availableApGains[apGainIndex], " LFP=", settings.availableLfpGains[lfpGainIndex], " REF=", settings.availableReferences[settings.referenceIndex]);
}

void Neuropixels_NHP_Active::selectElectrodes()
{
    Neuropixels::NP_ErrorCode ec;

    if (settings.selectedChannel.size() > 0)
    {
        for (int ch = 0; ch < settings.selectedChannel.size(); ch++)
        {
            ec = Neuropixels::selectElectrode (basestation->slot,
                                               headstage->port,
                                               dock,
                                               settings.selectedChannel[ch],
                                               settings.selectedShank[ch],
                                               settings.availableBanks.indexOf (settings.selectedBank[ch]));
        }
    }
}

Array<int> Neuropixels_NHP_Active::selectElectrodeConfiguration (String config)
{
    Array<int> selection;

    if (config.equalsIgnoreCase ("Bank A"))
    {
        for (int i = 0; i < 384; i++)
            selection.add (i);
    }
    else if (config.equalsIgnoreCase ("Bank B"))
    {
        for (int i = 384; i < 768; i++)
            selection.add (i);
    }
    else if (config.equalsIgnoreCase ("Bank C"))
    {
        if (type == ProbeType::NHP25 || type == ProbeType::NHP45 || type == ProbeType::NP2_1)
        {
            for (int i = 768; i < 1152; i++)
                selection.add (i);
        }
        else
        {
            for (int i = 576; i < 960; i++)
                selection.add (i);
        }
    }
    else if (config.equalsIgnoreCase ("Bank D"))
    {
        if (type == ProbeType::NP2_1)
        {
            for (int i = 896; i < 1280; i++)
                selection.add (i);
        }
        else
        {
            for (int i = 1152; i < 1536; i++)
                selection.add (i);
        }
    }
    else if (config.equalsIgnoreCase ("Bank E"))
    {
        for (int i = 1536; i < 1920; i++)
            selection.add (i);
    }
    else if (config.equalsIgnoreCase ("Bank F"))
    {
        for (int i = 1920; i < 2304; i++)
            selection.add (i);
    }
    else if (config.equalsIgnoreCase ("Bank G"))
    {
        if (type == ProbeType::NHP45)
        {
            for (int i = 2304; i < 2688; i++)
                selection.add (i);
        }
        else
        {
            for (int i = 2112; i < 2496; i++)
                selection.add (i);
        }
    }
    else if (config.equalsIgnoreCase ("Bank H"))
    {
        for (int i = 2688; i < 3072; i++)
            selection.add (i);
    }
    else if (config.equalsIgnoreCase ("Bank I"))
    {
        for (int i = 3072; i < 3456; i++)
            selection.add (i);
    }
    else if (config.equalsIgnoreCase ("Bank J"))
    {
        for (int i = 3456; i < 3840; i++)
            selection.add (i);
    }
    else if (config.equalsIgnoreCase ("Bank K"))
    {
        for (int i = 3840; i < 4224; i++)
            selection.add (i);
    }
    else if (config.equalsIgnoreCase ("Bank L"))
    {
        for (int i = 4032; i < 4416; i++)
            selection.add (i);
    }
    else if (config.equalsIgnoreCase ("Single Column"))
    {
        for (int i = 0; i < 384; i += 2)
            selection.add (i);

        for (int i = 385; i < 768; i += 2)
            selection.add (i);
    }
    else if (config.equalsIgnoreCase ("Tetrodes"))
    {
        for (int i = 0; i < 384; i += 8)
        {
            for (int j = 0; j < 4; j++)
                selection.add (i + j);
        }

        for (int i = 388; i < 768; i += 8)
        {
            for (int j = 0; j < 4; j++)
                selection.add (i + j);
        }
    }

    return selection;
}

void Neuropixels_NHP_Active::setApFilterState()
{
    for (int channel = 0; channel < 384; channel++)
        Neuropixels::setAPCornerFrequency (basestation->slot,
                                           headstage->port,
                                           dock,
                                           channel,
                                           ! settings.apFilterState); // true if disabled
}

void Neuropixels_NHP_Active::setAllGains()
{
    for (int channel = 0; channel < 384; channel++)
    {
        Neuropixels::setGain (basestation->slot, headstage->port, dock, channel, settings.apGainIndex, settings.lfpGainIndex);
    }
}

void Neuropixels_NHP_Active::setAllReferences()
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
        default:
            refId = Neuropixels::EXT_REF;
    }

    for (int channel = 0; channel < 384; channel++)
        Neuropixels::setReference (basestation->slot, headstage->port, dock, channel, 0, refId, refElectrodeBank);
}

void Neuropixels_NHP_Active::writeConfiguration()
{
    if (basestation->isBusy())
        basestation->waitForThreadToExit();

    errorCode = Neuropixels::writeProbeConfiguration (basestation->slot, headstage->port, dock, false);

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

void Neuropixels_NHP_Active::startAcquisition()
{
    ap_timestamp = 0;
    lfp_timestamp = 0;

    apBuffer->clear();
    lfpBuffer->clear();

    apView->reset();
    lfpView->reset();

    last_npx_timestamp = 0;
    passedOneSecond = false;

    SKIP = sendSync ? 385 : 384;

    LOGD ("  Starting thread.");
    startThread();
}

void Neuropixels_NHP_Active::stopAcquisition()
{
    LOGC ("Probe stopping thread.");
    signalThreadShouldExit();
}

void Neuropixels_NHP_Active::run()
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
                        apSamples[j + i * SKIP + packetNum * 12 * SKIP] =
                            float (packet[packetNum].apData[i][j]) * 1.2f / 1024.0f * 1000000.0f
                                / settings.availableApGains[settings.apGainIndex]
                            - ap_offsets[j][0]; // convert to microvolts

                        apView->addSample (apSamples[j + i * SKIP + packetNum * 12 * SKIP], j);

                        if (i == 0)
                        {
                            lfpSamples[j + packetNum * SKIP] =
                                float (packet[packetNum].lfpData[j]) * 1.2f / 1024.0f * 1000000.0f
                                    / settings.availableLfpGains[settings.lfpGainIndex]
                                - lfp_offsets[j][0]; // convert to microvolts

                            lfpView->addSample (lfpSamples[j + packetNum * SKIP], j);
                        }
                    }

                    ap_timestamps[i + packetNum * 12] = ap_timestamp++;
                    event_codes[i + packetNum * 12] = eventCode;

                    if (sendSync)
                        apSamples[384 + i * SKIP + packetNum * 12 * SKIP] = (float) eventCode;
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

bool Neuropixels_NHP_Active::runBist (BIST bistType)
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
