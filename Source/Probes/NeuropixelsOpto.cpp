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

#include "NeuropixelsOpto.h"
#include "Geometry.h"

#include "../NeuropixThread.h"
#include <thread>

#define MAXLEN 50

void NeuropixelsOpto::getInfo()
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

NeuropixelsOpto::NeuropixelsOpto (Basestation* bs, Headstage* hs, Flex* fl) : Probe (bs, hs, fl, 1)
{
    getInfo();

    setStatus (SourceStatus::DISCONNECTED);

    customName.probeSpecific = String (info.serial_number);

    if (Geometry::forPartNumber (info.part_number, electrodeMetadata, emissionSiteMetadata, probeMetadata))
    {
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

        settings.availableBanks = { Bank::A,
                                    Bank::B,
                                    Bank::C,
                                    Bank::D,
                                    Bank::E,
                                    Bank::F,
                                    Bank::G,
                                    Bank::H,
                                    Bank::I,
                                    Bank::J,
                                    Bank::K,
                                    Bank::L };

        settings.availableElectrodeConfigurations.add ("Bank A");
        settings.availableElectrodeConfigurations.add ("Bank B");
        settings.availableElectrodeConfigurations.add ("Bank C");
        settings.availableElectrodeConfigurations.add ("Single Column");
        settings.availableElectrodeConfigurations.add ("Tetrodes");

        open();
    }
    else
    {
        isValid = false;
    }
}

bool NeuropixelsOpto::open()
{
    errorCode = Neuropixels::openProbe (basestation->slot, headstage->port, dock);
    LOGD ("openProbe: slot: ", basestation->slot, " port: ", headstage->port, " dock: ", dock, " errorCode: ", errorCode);

    ap_timestamp = 0;
    lfp_timestamp = 0;
    eventCode = 0;

    apView = std::make_unique<ActivityView> (384, 3000, std::vector<std::vector<int>>(), probeMetadata.num_adcs, electrodeMetadata.size());
    lfpView = std::make_unique<ActivityView> (384, 250, std::vector<std::vector<int>>(), probeMetadata.num_adcs, electrodeMetadata.size());

    refreshActivityViewMapping();

    return errorCode == Neuropixels::SUCCESS;
}

bool NeuropixelsOpto::close()
{
    errorCode = Neuropixels::closeProbe (basestation->slot, headstage->port, dock);
    LOGD ("closeProbe: slot: ", basestation->slot, " port: ", headstage->port, " dock: ", dock, " errorCode: ", errorCode);

    return errorCode == Neuropixels::SUCCESS;
}

void NeuropixelsOpto::initialize (bool signalChainIsLoading)
{
    errorCode = Neuropixels::init (basestation->slot, headstage->port, dock);
    LOGD ("Neuropixels::init: errorCode: ", errorCode);

    checkError (Neuropixels::writeProbeConfiguration (basestation->slot, headstage->port, dock, false), "writeProbeConfiguration");
    errorCode = Neuropixels::bistSR (basestation->slot, headstage->port, dock);

    if (errorCode != Neuropixels::SUCCESS)
    {
        LOGC (" Shift register error detected -- possible broken shank");

        for (int i = 0; i < electrodeMetadata.size(); i++)
        {
            electrodeMetadata.getReference (i).shank_is_programmable = false;
        }
    }

    errorCode = Neuropixels::setOPMODE (basestation->slot, headstage->port, dock, Neuropixels::RECORDING);
    LOGD ("Neuropixels::setOPMODE: errorCode: ", errorCode);

    errorCode = Neuropixels::setHSLed (basestation->slot, headstage->port, false);
    LOGDD ("Neuropixels::setHSLed: errorCode: ", errorCode);
}

void NeuropixelsOpto::calibrate()
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
    String opticalFile = probeDirectory.getChildFile (String (info.serial_number) + "_optoCalibration.csv").getFullPathName();

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

    LOGDD ("Optical calibration file: ", adcFile);

    errorCode = Neuropixels::setOpticalCalibration (basestation->slot, headstage->port, dock, opticalFile.toRawUTF8());

    if (errorCode == 0)
    {
        LOGD ("Successful optical calibration.");
    }
    else
    {
        LOGD ("!!! Unsuccessful optical calibration, failed with error code: ", errorCode);
        return;
    }

    isCalibrated = true;
}

void NeuropixelsOpto::setEmissionSite (Neuropixels::wavelength_t wavelength, int site)
{
    errorCode = Neuropixels::setEmissionSite (basestation->slot, headstage->port, dock, wavelength, site);

    if (errorCode == 0)
    {
        LOGD ("Selected site ", site);
    }
    else
    {
        LOGD ("!!! Unsuccessful site selection, failed with error code: ", errorCode);
        return;
    }
}

void NeuropixelsOpto::printSettings()
{
    int apGainIndex;
    int lfpGainIndex;

    Neuropixels::getGain (basestation->slot, headstage->port, dock, 32, &apGainIndex, &lfpGainIndex);

    LOGD ("Current settings for probe on slot: ", basestation->slot, " port: ", headstage->port, " dock: ", dock, " AP=", settings.availableApGains[apGainIndex], " LFP=", settings.availableLfpGains[lfpGainIndex], " REF=", settings.availableReferences[settings.referenceIndex]);
}

void NeuropixelsOpto::selectElectrodes()
{
    Neuropixels::NP_ErrorCode ec;

    if (settings.selectedChannel.size() > 0)
    {
        for (int ch = 0; ch < settings.selectedChannel.size(); ch++)
        {
            //LOGD("Setting probe: ", headstage->port, " ch: ", settings.selectedChannel[ch], " to bank: ", settings.availableBanks.indexOf(settings.selectedBank[ch]));

            ec = Neuropixels::selectElectrode (basestation->slot,
                                               headstage->port,
                                               dock,
                                               settings.selectedChannel[ch],
                                               settings.selectedShank[ch],
                                               settings.availableBanks.indexOf (settings.selectedBank[ch]));
        }
    }
}

Array<int> NeuropixelsOpto::selectElectrodeConfiguration (String config)
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
        for (int i = 576; i < 960; i++)
        {
            selection.add (i);
        }
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

void NeuropixelsOpto::setApFilterState()
{
    for (int channel = 0; channel < 384; channel++)
        Neuropixels::setAPCornerFrequency (basestation->slot,
                                           headstage->port,
                                           dock,
                                           channel,
                                           ! settings.apFilterState); // true if disabled
}

void NeuropixelsOpto::setAllGains()
{
    LOGDD ("Setting gain AP=", settings.apGainIndex, " LFP=", settings.lfpGainIndex);

    for (int channel = 0; channel < 384; channel++)
    {
        Neuropixels::setGain (basestation->slot, headstage->port, dock, channel, settings.apGainIndex, settings.lfpGainIndex);
    }
}

void NeuropixelsOpto::setAllReferences()
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
        Neuropixels::setReference (basestation->slot, headstage->port, dock, channel, 0, refId, refElectrodeBank);
}

void NeuropixelsOpto::writeConfiguration()
{
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

void NeuropixelsOpto::startAcquisition()
{
    if (surveyModeActive && ! isEnabledForSurvey)
        return;

    ap_timestamp = 0;
    lfp_timestamp = 0;
    //std::cout << "... and clearing buffers" << std::endl;
    apBuffer->clear();
    lfpBuffer->clear();

    apView->reset();
    lfpView->reset();

    SKIP = sendSync ? 385 : 384;

    LOGD ("  NP Opto starting thread.");
    startThread();
}

void NeuropixelsOpto::stopAcquisition()
{
    LOGC ("Probe stopping thread.");
    signalThreadShouldExit();
}

void NeuropixelsOpto::run()
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

                        // apView->addSample (apSamples[j * (12 * count) + i + (packetNum * 12)], j);

                        if (i == 0)
                        {
                            lfpSamples[(j * count) + packetNum] =
                                float (packet[packetNum].lfpData[j]) * 1.2f / 1024.0f * 1000000.0f
                                    / settings.availableLfpGains[settings.lfpGainIndex]
                                - lfp_offsets[j][0]; // convert to microvolts

                            // lfpView->addSample (lfpSamples[(j * count) + packetNum], j);
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
            apView->addToBuffer (apSamples, 12 * count);
            lfpBuffer->addToBuffer (lfpSamples, lfp_timestamps, timestamp_s, lfp_event_codes, count);
            lfpView->addToBuffer (lfpSamples, count);

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

        int packetsAvailable;
        int headroom;

        if (! passedOneSecond)
        {
            if (ap_timestamp > 30000)
                passedOneSecond = true;
        }

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

bool NeuropixelsOpto::runBist (BIST bistType)
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
