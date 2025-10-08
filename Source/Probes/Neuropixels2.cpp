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

#include "Neuropixels2.h"
#include "Geometry.h"

#include "../NeuropixThread.h"

#define MAXLEN 50

void Neuropixels2::getInfo()
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

Neuropixels2::Neuropixels2 (Basestation* bs, Headstage* hs, Flex* fl, int dock) : Probe (bs, hs, fl, dock)
{
    getInfo();

    setStatus (SourceStatus::DISCONNECTED);

    customName.probeSpecific = String (info.serial_number);

    if (Geometry::forPartNumber (info.part_number, electrodeMetadata, probeMetadata))
    {
        name = probeMetadata.name;
        type = probeMetadata.type;

        settings.probeType = type;

        settings.probe = this;

        settings.availableBanks = probeMetadata.availableBanks;

        settings.apGainIndex = -1;
        settings.lfpGainIndex = -1;
        settings.referenceIndex = 0;
        settings.apFilterState = false;

        channel_count = 384;
        lfp_sample_rate = 2500.0f; // not used
        ap_sample_rate = 30000.0f;

        bitScaling = pow (2, probeMetadata.adc_bits);

        for (int i = 0; i < channel_count; i++)
        {
            settings.selectedBank.add (Bank::A);
            settings.selectedChannel.add (electrodeMetadata[i].channel);
            settings.selectedShank.add (0);
            settings.selectedElectrode.add (electrodeMetadata[i].global_index);
        }

        if (probeMetadata.shank_count == 1)
        {
            availableReferences.add ("Ext");
            availableReferences.add ("Tip");

            settings.availableElectrodeConfigurations.add ("Bank A");
            settings.availableElectrodeConfigurations.add ("Bank B");
            settings.availableElectrodeConfigurations.add ("Bank C");
            settings.availableElectrodeConfigurations.add ("Bank D");
        }
        else
        {
            availableReferences.add ("Ext");
            availableReferences.add ("1: Tip");
            availableReferences.add ("2: Tip");
            availableReferences.add ("3: Tip");
            availableReferences.add ("4: Tip");

            settings.availableElectrodeConfigurations.add ("Shank 1 Bank A");
            settings.availableElectrodeConfigurations.add ("Shank 1 Bank B");
            settings.availableElectrodeConfigurations.add ("Shank 1 Bank C");
            settings.availableElectrodeConfigurations.add ("Shank 1 Bank D");
            settings.availableElectrodeConfigurations.add ("Shank 2 Bank A");
            settings.availableElectrodeConfigurations.add ("Shank 2 Bank B");
            settings.availableElectrodeConfigurations.add ("Shank 2 Bank C");
            settings.availableElectrodeConfigurations.add ("Shank 2 Bank D");
            settings.availableElectrodeConfigurations.add ("Shank 3 Bank A");
            settings.availableElectrodeConfigurations.add ("Shank 3 Bank B");
            settings.availableElectrodeConfigurations.add ("Shank 3 Bank C");
            settings.availableElectrodeConfigurations.add ("Shank 3 Bank D");
            settings.availableElectrodeConfigurations.add ("Shank 4 Bank A");
            settings.availableElectrodeConfigurations.add ("Shank 4 Bank B");
            settings.availableElectrodeConfigurations.add ("Shank 4 Bank C");
            settings.availableElectrodeConfigurations.add ("Shank 4 Bank D");
            settings.availableElectrodeConfigurations.add ("All Shanks 1-96");
            settings.availableElectrodeConfigurations.add ("All Shanks 97-192");
            settings.availableElectrodeConfigurations.add ("All Shanks 193-288");
            settings.availableElectrodeConfigurations.add ("All Shanks 289-384");
            settings.availableElectrodeConfigurations.add ("All Shanks 385-480");
            settings.availableElectrodeConfigurations.add ("All Shanks 481-576");
            settings.availableElectrodeConfigurations.add ("All Shanks 577-672");
            settings.availableElectrodeConfigurations.add ("All Shanks 673-768");
            settings.availableElectrodeConfigurations.add ("All Shanks 769-864");
            settings.availableElectrodeConfigurations.add ("All Shanks 865-960");
            settings.availableElectrodeConfigurations.add ("All Shanks 961-1056");
            settings.availableElectrodeConfigurations.add ("All Shanks 1057-1152");
            settings.availableElectrodeConfigurations.add ("All Shanks 1153-1248");
        }

        if (info.part_number.equalsIgnoreCase ("NP2013") || 
            info.part_number.equalsIgnoreCase ("NP2014") || 
            info.part_number.equalsIgnoreCase ("NP2003") || 
            info.part_number.equalsIgnoreCase ("NP2004"))
        {
            availableReferences.add ("Ground");
        }

        settings.availableReferences = availableReferences;

        open();
    }
    else
    {
        isValid = false;
    }
}

bool Neuropixels2::open()
{
    checkError (Neuropixels::openProbe (basestation->slot, headstage->port, dock),
                "openProbe: slot: " + String (basestation->slot) + " port: " + String (headstage->port) + " dock: " + String (dock));

    ap_timestamp = 0;
    lfp_timestamp = 0;
    eventCode = 0;

    apView = std::make_unique<ActivityView> (384, 3000, std::vector<std::vector<int>>(), probeMetadata.num_adcs, electrodeMetadata.size());

    refreshActivityViewMapping();

    return errorCode == Neuropixels::SUCCESS;
}

bool Neuropixels2::close()
{
    checkError (Neuropixels::closeProbe (basestation->slot, headstage->port, dock),
                "closeProbe: slot: " + String (basestation->slot) + " port: " + String (headstage->port) + " dock: " + String (dock));

    return errorCode == Neuropixels::SUCCESS;
}

void Neuropixels2::initialize (bool signalChainIsLoading)
{
    checkError (Neuropixels::init (basestation->slot, headstage->port, dock),
                "init: slot: " + String (basestation->slot) + " port: " + String (headstage->port) + " dock: " + String (dock));
}

void Neuropixels2::calibrate()
{
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

    String gainFile = probeDirectory.getChildFile (String (info.serial_number) + "_gainCalValues.csv").getFullPathName();

    LOGD ("Gain file: ", gainFile);

    errorCode = checkError (Neuropixels::setGainCalibration (basestation->slot, headstage->port, dock, gainFile.toRawUTF8()), "setGainCalibration");

    if (errorCode == 0)
    {
        LOGD ("Successful gain calibration.");
    }
    else
    {
        LOGD ("Unsuccessful gain calibration, failed with error code: ", errorCode);
    }

    errorCode = Neuropixels::writeProbeConfiguration (basestation->slot, headstage->port, dock, false);

    if (! errorCode == Neuropixels::SUCCESS)
    {
        isCalibrated = false;
        LOGD ("Failed to write probe config w/ error code: ", errorCode);
    }
    else
    {
        isCalibrated = true;
        LOGD ("Successfully wrote probe config ");
    }

    checkError(Neuropixels::np_setHSLed (basestation->slot, headstage->port, false), "np_setHSLed");

  
}

void Neuropixels2::selectElectrodes()
{

    if (settings.selectedBank.size() == 0)
        return;

    for (int ch = 0; ch < settings.selectedChannel.size(); ch++)
    {
        checkError(Neuropixels::selectElectrode (basestation->slot,
                                           headstage->port,
                                           dock,
                                           settings.selectedChannel[ch],
                                           settings.selectedShank[ch],
                                           settings.availableBanks.indexOf (settings.selectedBank[ch])), "selectElectrode");
    }

    LOGD ("Updated electrode settings for slot: ", basestation->slot, " port: ", headstage->port, " dock: ", dock);
}

Array<int> Neuropixels2::selectElectrodeConfiguration (String config)
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
        for (int i = 768; i < 1152; i++)
            selection.add (i);
    }
    else if (config.equalsIgnoreCase ("Bank D"))
    {
        for (int i = 896; i < 1280; i++)
            selection.add (i);
    }
    else if (config.equalsIgnoreCase ("Shank 1 Bank A"))
    {
        for (int i = 0; i < 384; i++)
        {
            selection.add (i);
        }
    }
    else if (config.equalsIgnoreCase ("Shank 1 Bank B"))
    {
        for (int i = 384; i < 768; i++)
        {
            selection.add (i);
        }
    }
    else if (config.equalsIgnoreCase ("Shank 1 Bank C"))
    {
        for (int i = 768; i < 1152; i++)
        {
            selection.add (i);
        }
    }
    else if (config.equalsIgnoreCase ("Shank 1 Bank D"))
    {
        for (int i = 896; i < 1280; i++)
        {
            selection.add (i);
        }
    }
    else if (config.equalsIgnoreCase ("Shank 2 Bank A"))
    {
        int startElectrode = 1280;

        for (int i = startElectrode; i < startElectrode + 384; i++)
        {
            selection.add (i);
        }
    }
    else if (config.equalsIgnoreCase ("Shank 2 Bank B"))
    {
        int startElectrode = 1280 + 384;

        for (int i = startElectrode; i < startElectrode + 384; i++)
        {
            selection.add (i);
        }
    }
    else if (config.equalsIgnoreCase ("Shank 2 Bank C"))
    {
        int startElectrode = 1280 + 384 * 2;

        for (int i = startElectrode; i < startElectrode + 384; i++)
        {
            selection.add (i);
        }
    }
    else if (config.equalsIgnoreCase ("Shank 2 Bank D"))
    {
        int startElectrode = 1280 + 896;

        for (int i = startElectrode; i < startElectrode + 384; i++)
        {
            selection.add (i);
        }
    }
    else if (config.equalsIgnoreCase ("Shank 3 Bank A"))
    {
        int startElectrode = 1280 * 2;

        for (int i = startElectrode; i < startElectrode + 384; i++)
        {
            selection.add (i);
        }
    }
    else if (config.equalsIgnoreCase ("Shank 3 Bank B"))
    {
        int startElectrode = 1280 * 2 + 384;

        for (int i = startElectrode; i < startElectrode + 384; i++)
        {
            selection.add (i);
        }
    }
    else if (config.equalsIgnoreCase ("Shank 3 Bank C"))
    {
        int startElectrode = 1280 * 2 + 384 * 2;

        for (int i = startElectrode; i < startElectrode + 384; i++)
        {
            selection.add (i);
        }
    }
    else if (config.equalsIgnoreCase ("Shank 3 Bank D"))
    {
        int startElectrode = 1280 * 2 + 896;

        for (int i = startElectrode; i < startElectrode + 384; i++)
        {
            selection.add (i);
        }
    }
    else if (config.equalsIgnoreCase ("Shank 4 Bank A"))
    {
        int startElectrode = 1280 * 3;

        for (int i = startElectrode; i < startElectrode + 384; i++)
        {
            selection.add (i);
        }
    }
    else if (config.equalsIgnoreCase ("Shank 4 Bank B"))
    {
        int startElectrode = 1280 * 3 + 384;

        for (int i = startElectrode; i < startElectrode + 384; i++)
        {
            selection.add (i);
        }
    }
    else if (config.equalsIgnoreCase ("Shank 4 Bank C"))
    {
        int startElectrode = 1280 * 3 + 384 * 2;

        for (int i = startElectrode; i < startElectrode + 384; i++)
        {
            selection.add (i);
        }
    }
    else if (config.equalsIgnoreCase ("Shank 4 Bank D"))
    {
        int startElectrode = 1280 * 3 + 896;

        for (int i = startElectrode; i < startElectrode + 384; i++)
        {
            selection.add (i);
        }
    }
    else if (config.equalsIgnoreCase ("All Shanks 1-96"))
    {
        int startElectrode = 0;

        for (int shank = 0; shank < 4; shank++)
        {
            for (int i = startElectrode + 1280 * shank; i < startElectrode + 96 + 1280 * shank; i++)
            {
                selection.add (i);
            }
        }
    }
    else if (config.equalsIgnoreCase ("All Shanks 97-192"))
    {
        int startElectrode = 96;

        for (int shank = 0; shank < 4; shank++)
        {
            for (int i = startElectrode + 1280 * shank; i < startElectrode + 96 + 1280 * shank; i++)
            {
                selection.add (i);
            }
        }
    }
    else if (config.equalsIgnoreCase ("All Shanks 193-288"))
    {
        int startElectrode = 192;

        for (int shank = 0; shank < 4; shank++)
        {
            for (int i = startElectrode + 1280 * shank; i < startElectrode + 96 + 1280 * shank; i++)
            {
                selection.add (i);
            }
        }
    }
    else if (config.equalsIgnoreCase ("All Shanks 289-384"))
    {
        int startElectrode = 288;

        for (int shank = 0; shank < 4; shank++)
        {
            for (int i = startElectrode + 1280 * shank; i < startElectrode + 96 + 1280 * shank; i++)
            {
                selection.add (i);
            }
        }
    }
    else if (config.equalsIgnoreCase ("All Shanks 385-480"))
    {
        int startElectrode = 384;

        for (int shank = 0; shank < 4; shank++)
        {
            for (int i = startElectrode + 1280 * shank; i < startElectrode + 96 + 1280 * shank; i++)
            {
                selection.add (i);
            }
        }
    }
    else if (config.equalsIgnoreCase ("All Shanks 481-576"))
    {
        int startElectrode = 480;

        for (int shank = 0; shank < 4; shank++)
        {
            for (int i = startElectrode + 1280 * shank; i < startElectrode + 96 + 1280 * shank; i++)
            {
                selection.add (i);
            }
        }
    }
    else if (config.equalsIgnoreCase ("All Shanks 577-672"))
    {
        int startElectrode = 576;

        for (int shank = 0; shank < 4; shank++)
        {
            for (int i = startElectrode + 1280 * shank; i < startElectrode + 96 + 1280 * shank; i++)
            {
                selection.add (i);
            }
        }
    }
    else if (config.equalsIgnoreCase ("All Shanks 673-768"))
    {
        int startElectrode = 672;

        for (int shank = 0; shank < 4; shank++)
        {
            for (int i = startElectrode + 1280 * shank; i < startElectrode + 96 + 1280 * shank; i++)
            {
                selection.add (i);
            }
        }
    }
    else if (config.equalsIgnoreCase ("All Shanks 769-864"))
    {
        int startElectrode = 768;

        for (int shank = 0; shank < 4; shank++)
        {
            for (int i = startElectrode + 1280 * shank; i < startElectrode + 96 + 1280 * shank; i++)
            {
                selection.add (i);
            }
        }
    }
    else if (config.equalsIgnoreCase ("All Shanks 865-960"))
    {
        int startElectrode = 864;

        for (int shank = 0; shank < 4; shank++)
        {
            for (int i = startElectrode + 1280 * shank; i < startElectrode + 96 + 1280 * shank; i++)
            {
                selection.add (i);
            }
        }
    }
    else if (config.equalsIgnoreCase ("All Shanks 961-1056"))
    {
        int startElectrode = 960;

        for (int shank = 0; shank < 4; shank++)
        {
            for (int i = startElectrode + 1280 * shank; i < startElectrode + 96 + 1280 * shank; i++)
            {
                selection.add (i);
            }
        }
    }
    else if (config.equalsIgnoreCase ("All Shanks 1057-1152"))
    {
        int startElectrode = 1056;

        for (int shank = 0; shank < 4; shank++)
        {
            for (int i = startElectrode + 1280 * shank; i < startElectrode + 96 + 1280 * shank; i++)
            {
                selection.add (i);
            }
        }
    }
    else if (config.equalsIgnoreCase ("All Shanks 1153-1248"))
    {
        int startElectrode = 1152;

        for (int shank = 0; shank < 4; shank++)
        {
            for (int i = startElectrode + 1280 * shank; i < startElectrode + 96 + 1280 * shank; i++)
            {
                selection.add (i);
            }
        }
    }

    return selection;
}

void Neuropixels2::setApFilterState()
{
    // no filter cut available
}

void Neuropixels2::setAllGains()
{
    // no gain available
}

void Neuropixels2::setAllReferences()
{
    Neuropixels::channelreference_t refId = Neuropixels::EXT_REF;
    int refElectrodeBank = 0;
    int shank = 0;

    String selectedRef = availableReferences[settings.referenceIndex];

    if (selectedRef.equalsIgnoreCase ("Ext"))
        refId = Neuropixels::EXT_REF;
    else if (selectedRef.equalsIgnoreCase ("Tip"))
        refId = Neuropixels::TIP_REF;
    else if (selectedRef.equalsIgnoreCase ("Ground"))
        refId = Neuropixels::GND_REF;
    else if (selectedRef.equalsIgnoreCase ("1: Tip"))
    {
        refId = Neuropixels::TIP_REF;
        shank = 0;
    }
    else if (selectedRef.equalsIgnoreCase ("2: Tip"))
    {
        refId = Neuropixels::TIP_REF;
        shank = 1;
    }
    else if (selectedRef.equalsIgnoreCase ("3: Tip"))
    {
        refId = Neuropixels::TIP_REF;
        shank = 2;
    }
    else if (selectedRef.equalsIgnoreCase ("4: Tip"))
    {
        refId = Neuropixels::TIP_REF;
        shank = 3;
    }

    if (type == ProbeType::NP2_4)
    {
        // disconnect the four shank switches first
        for (int shank = 0; shank < 4; shank++)
        {
            Neuropixels::setReference (basestation->slot,
                                       headstage->port,
                                       dock,
                                       0,
                                       shank,
                                       Neuropixels::NONE_REF,
                                       0);
        }
    }

    LOGC ("Setting reference for slot: ", basestation->slot, " port: ", headstage->port, " dock: ", dock, " to ", selectedRef);

    for (int channel = 0; channel < channel_count; channel++)
    {
        if (checkError (Neuropixels::setReference (basestation->slot,
                                                   headstage->port,
                                                   dock,
                                                   channel,
                                                   shank,
                                                   refId,
                                                   refElectrodeBank),
                        "setReference")
            != Neuropixels::SUCCESS)
        {
            LOGD ("Failed to set reference for slot: ", basestation->slot, " port: ", headstage->port, " dock: ", dock, " to ", selectedRef);
        }
    }
        

}

void Neuropixels2::writeConfiguration()
{
    checkError(Neuropixels::writeProbeConfiguration (basestation->slot, headstage->port, dock, false) , "writeProbeConfiguration");
}

void Neuropixels2::startAcquisition()
{
    if (surveyModeActive && ! isEnabledForSurvey)
        return;

    ap_timestamp = 0;
    apBuffer->clear();

    apView->reset();

    last_npx_timestamp = 0;
    passedOneSecond = false;

    SKIP = sendSync ? 385 : 384;

    LOGD ("  Starting thread.");
    startThread();
}

void Neuropixels2::stopAcquisition()
{
    LOGC ("Probe stopping thread.");
    signalThreadShouldExit();
}

void Neuropixels2::run()
{
    while (! threadShouldExit())
    {
        int count = MAXPACKETS;

        errorCode = Neuropixels::readPackets (
            basestation->slot,
            headstage->port,
            dock,
            source,
            &packetInfo[0],
            &data[0],
            384,
            count,
            &count);

        if (errorCode == Neuropixels::SUCCESS && count > 0)
        {
            for (int packetNum = 0; packetNum < count; packetNum++)
            {
                eventCode = packetInfo[packetNum].Status >> 6;

                if (invertSyncLine)
                    eventCode = ~eventCode;

                uint32_t npx_timestamp = packetInfo[packetNum].Timestamp;

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
                    apSamples[j * count + packetNum] =
                        float (data[packetNum * 384 + j]) / bitScaling / amplifierGain * 1000000.0f; // convert to microvolts

                    // apView->addSample (apSamples[(j * count) + packetNum], j);
                }

                ap_timestamps[packetNum] = ap_timestamp++;
                event_codes[packetNum] = eventCode;

                if (sendSync)
                    apSamples[(384 * count) + packetNum] = (float) eventCode;
            }

            apBuffer->addToBuffer (apSamples, ap_timestamps, timestamp_s, event_codes, count);
            apView->addToBuffer (apSamples, count);
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

        Neuropixels::getPacketFifoStatus (
            basestation->slot,
            headstage->port,
            dock,
            source,
            &packetsAvailable,
            &headroom);

        fifoFillPercentage = float (packetsAvailable) / float (packetsAvailable + headroom);

        if (packetsAvailable < MAXPACKETS)
        {
            int uSecToWait = (MAXPACKETS - packetsAvailable) * 30;

            std::this_thread::sleep_for (std::chrono::microseconds (uSecToWait));
        }
    }
}

bool Neuropixels2::runBist (BIST bistType)
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

    return returnValue;
}
