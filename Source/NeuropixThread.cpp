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

#include "NeuropixThread.h"
#include "NeuropixEditor.h"

#include "Basestations/OneBox.h"
#include "Basestations/PxiBasestation.h"
#include "Basestations/SimulatedBasestation.h"
#include "Probes/OneBoxADC.h"

#include "UI/NeuropixInterface.h"

#include <vector>

//Helpful for debugging when PXI system is connected but don't want to connect to real probes
#define FORCE_SIMULATION_MODE false

DataThread* NeuropixThread::createDataThread (SourceNode* sn, DeviceType type)
{
    return new NeuropixThread (sn, type);
}

std::unique_ptr<GenericEditor> NeuropixThread::createEditor (SourceNode* sn)
{
    std::unique_ptr<NeuropixEditor> ed = std::make_unique<NeuropixEditor> (sn, this);

    editor = ed.get();

    return ed;
}

void Initializer::run()
{
    setProgress (-1); // endless moving progress bar

    Neuropixels::scanBS();
    Neuropixels::basestationID list[16];
    int count = getDeviceList (&list[0], 16);

    LOGC ("  Found ", count, " device", count == 1 ? "." : "s.");

    Array<int> slotIDs;

    if (! FORCE_SIMULATION_MODE)
    {
        int countForType = 0;
        for (int i = 0; i < count; i++)
        {
            Neuropixels::NP_ErrorCode ec = Neuropixels::getDeviceInfo (list[i].ID, &list[i]);

            LOGD ("Device ID: ", list[i].ID, ", Platform ID: ", list[i].platformid);

            if (list[i].platformid == Neuropixels::NPPlatform_PXI && type == PXI)
                countForType++;
            else if (list[i].platformid == Neuropixels::NPPlatform_USB && type == ONEBOX)
                countForType++;
        }

        setStatusMessage ("Found " + String (countForType) + " device" + (countForType == 1 ? "." : "s."));

        int deviceNum = 0;

        for (int i = 0; i < count; i++)
        {
            int slotID;

            Neuropixels::NP_ErrorCode ec = Neuropixels::getDeviceInfo (list[i].ID, &list[i]);

            LOGD ("Device ID: ", list[i].ID, ", Platform ID: ", list[i].platformid);

            bool foundSlot = Neuropixels::tryGetSlotID (&list[i], &slotID);

            if (foundSlot)
            {
                LOGD ("  Slot ID: ", slotID);
            }
            else
            {
                LOGD ("  Could not find slot ID");
            }

            if (foundSlot && list[i].platformid == Neuropixels::NPPlatform_PXI && type == PXI)
            {
                deviceNum++;
                LOGC ("  Opening device on slot ", slotID);
                setStatusMessage ("Opening basestation on PXI slot " + String (slotID) + " (" + String (deviceNum) + "/" + String (countForType) + ")");

                Basestation* bs = new PxiBasestation (neuropixThread, slotID);

                if (bs->open()) //returns true if Basestation firmware >= 2.0
                {
                    int insertionIndex = 0;

                    if (slotIDs.size() > 0)
                    {
                        insertionIndex = slotIDs.size();

                        LOGC ("  Checking ", insertionIndex, ": ", slotIDs[insertionIndex - 1]);

                        while (insertionIndex > 0 && slotIDs[insertionIndex - 1] > slotID)
                        {
                            LOGC ("Moving backward...");
                            insertionIndex--;
                            LOGC ("  Checking ", insertionIndex, ": ", slotIDs[insertionIndex - 1]);
                        }
                    }

                    LOGC ("Insertion index:", insertionIndex);

                    basestations.insert (insertionIndex, bs);
                    slotIDs.insert (insertionIndex, slotID);

                    LOGC ("  Adding basestation");
                    setStatusMessage ("Adding basestation found on PXI slot " + String (slotID));
                }
                else
                {
                    LOGC ("  Could not open basestation");
                    setStatusMessage ("Could not open basestation");
                    delete bs;
                }
            }
            else if (list[i].platformid == Neuropixels::NPPlatform_USB && type == ONEBOX)
            {
                deviceNum++;
                setStatusMessage ("Opening OneBox with serial number " + String (list[i].ID));

                Basestation* bs = new OneBox (neuropixThread, list[i].ID);

                if (bs->open())
                {
                    basestations.add (bs);

                    if (! bs->getProbeCount())
                        CoreServices::sendStatusMessage ("OneBox found, no probes connected.");

                    break; // prevent multiple OneBoxes from being opened
                }
                else
                {
                    LOGC ("  Could not open OneBox");
                    setStatusMessage ("Could not open OneBox");
                    delete bs;
                }
            }
            else
            {
                LOGC ("   Slot ", slotID, " did not match desired platform.");
            }
        }
    }
}

NeuropixThread::NeuropixThread (SourceNode* sn, DeviceType type_) : DataThread (sn),
                                                                    type (type_),
                                                                    baseStationAvailable (false),
                                                                    probesInitialized (false),
                                                                    calibrationWarningShown (false),
                                                                    initializationComplete (false),
                                                                    configurationComplete (false)
{
    defaultSyncFrequencies.add (1);

    api_v3.isActive = true;

    LOGC ("Scanning for devices...");

    LOGD ("Setting debug level to 0");
    Neuropixels::np_dbg_setlevel (0);

    LOGD ("### NeuropixThread()");
    LOGD ("### basestation.size() = ", basestations.size());
    LOGD ("### Running initializer thread");

    initializer = std::make_unique<Initializer> (this, basestations, type, api_v3);
    initializer->setStatusMessage ("Scanning for devices...");
    initializer->runThread();

    LOGD ("### basestation.size() = ", basestations.size());

    // If no basestations were found, ask user if they want to run in simulation mode
    if (basestations.size() == 0)
    {
        bool response = true;

        if (! FORCE_SIMULATION_MODE)
        {
            if (type == PXI)
            {
                response = AlertWindow::showOkCancelBox (AlertWindow::NoIcon,
                                                         "No basestations detected",
                                                         "No Neuropixels PXI basestations were detected. Do you want to run this plugin in simulation mode?",
                                                         "Yes",
                                                         "No",
                                                         0,
                                                         0);
            }
            else if (type == ONEBOX)
            {
                response = AlertWindow::showOkCancelBox (AlertWindow::NoIcon,
                                                         "No OneBox detected",
                                                         "No OneBox was detected. Do you want to run this plugin in simulation mode?",
                                                         "Yes",
                                                         "No",
                                                         0,
                                                         0);
            }
        }

        if (response)
        {
            if (type == PXI)
            {
                basestations.add (new SimulatedBasestation (this, type, 2));
                basestations.getLast()->open(); // detects # of probes
            }
            else if (type == ONEBOX)
            {
                basestations.add (new SimulatedBasestation (this, type, 16));
                basestations.getLast()->open(); // detects # of probes
            }
        }
    }

    for (auto basestation : basestations)
    {
        basestation->checkFirmwareVersion();
    }

    initializeProbes();

    if (type == PXI)
    {
        setMainSync (0);
    }

    updateStreamInfo();
}

void NeuropixThread::initializeProbes()
{
    sourceStreams.clear();

    bool foundSync = false;

    int probeIndex = 0;
    int streamIndex = 0;

    if (type == ONEBOX && basestations.size() > 0)
    {
        if (basestations[0]->type != BasestationType::SIMULATED)
        {
            baseStationAvailable = true;
        }
    }

    for (auto probe : getProbes())
    {
        baseStationAvailable = true;

        if (! foundSync)
        {
            foundSync = true;
        }

        /* Generate names for probes based on order of appearance in chassis */
        probe->customName.automatic = generateProbeName (probeIndex, ProbeNameConfig::AUTO_NAMING);
        probe->displayName = probe->customName.automatic;
        probe->streamIndex = streamIndex;
        probe->customName.streamSpecific = generateProbeName (probeIndex, ProbeNameConfig::STREAM_INDICES);

        if (probe->generatesLfpData())
            streamIndex += 2;
        else if (probe->type == ProbeType::QUAD_BASE)
            streamIndex += 4;
        else
            streamIndex += 1;

        probeIndex++;
    }

    if (baseStationAvailable)
    {
        for (auto bs : basestations)
        {
            bs->initialize (false);
        }

        probesInitialized = true;
    }
}

String NeuropixThread::generateProbeName (int probeIndex, ProbeNameConfig::NamingScheme namingScheme)
{
    StringArray probeNames = { "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z" };

    Array<Probe*> probes = getProbes();

    Probe* probe = probes[probeIndex];

    String name;

    switch (namingScheme)
    {
        case ProbeNameConfig::AUTO_NAMING:
            name = "Probe" + probeNames[probeIndex];
            break;
        case ProbeNameConfig::PROBE_SPECIFIC_NAMING:
            name = probe->customName.probeSpecific;
            break;
        case ProbeNameConfig::PORT_SPECIFIC_NAMING:
            name = probe->basestation->getCustomPortName (probe->headstage->port, probe->dock);
            break;
        case ProbeNameConfig::STREAM_INDICES:
            name = String (probe->streamIndex);
            if (probe->generatesLfpData())
                name += "," + String (probe->streamIndex + 1);
            else if (probe->type == ProbeType::QUAD_BASE)
                name += "," + String (probe->streamIndex + 1) + "," + String (probe->streamIndex + 2) + "," + String (probe->streamIndex + 3);
            break;
        default:
            name = "Probe" + probeNames[probeIndex];
    }

    return name;
}

void NeuropixThread::updateStreamInfo (bool enabledStateChanged)
{
    if (enabledStateChanged)
        sourceStreams.clear();

    streamInfo.clear();
    sourceBuffers.clear();

    int probe_index = 0;

    for (auto source : getDataSources())
    {
        if (source->sourceType == DataSourceType::PROBE)
        {
            Probe* probe = (Probe*) source;

            if (! probe->isEnabled)
            {
                probe_index++;
                continue;
            }

            if (probe->type != ProbeType::QUAD_BASE)
            {
                StreamInfo apInfo;

                apInfo.num_channels = probe->sendSync ? probe->channel_count + 1 : probe->channel_count;
                apInfo.sample_rate = probe->ap_sample_rate;
                apInfo.probe = probe;
                apInfo.probe_index = probe_index++;

                if (probe->generatesLfpData())
                    apInfo.type = AP_BAND;
                else
                    apInfo.type = BROAD_BAND;

                apInfo.sendSyncAsContinuousChannel = probe->sendSync;

                streamInfo.add (apInfo);

                sourceBuffers.add (new DataBuffer (apInfo.num_channels, 460800)); // AP band buffer
                probe->apBuffer = sourceBuffers.getLast();

                if (probe->generatesLfpData())
                {
                    StreamInfo lfpInfo;
                    lfpInfo.num_channels = probe->sendSync ? probe->channel_count + 1 : probe->channel_count;
                    lfpInfo.sample_rate = probe->lfp_sample_rate;
                    lfpInfo.type = LFP_BAND;
                    lfpInfo.probe = probe;
                    lfpInfo.sendSyncAsContinuousChannel = probe->sendSync;

                    sourceBuffers.add (new DataBuffer (lfpInfo.num_channels, 38400)); // LFP band buffer
                    probe->lfpBuffer = sourceBuffers.getLast();

                    streamInfo.add (lfpInfo);
                }

                LOGD ("Probe (slot=", probe->basestation->slot, ", port=", probe->headstage->port, ") CH=", apInfo.num_channels, " SR=", apInfo.sample_rate, " Hz");
            }
            else
            {
                probe->quadBaseBuffers.clear();

                for (int shank = 0; shank < 4; shank++)
                {
                    StreamInfo apInfo;

                    apInfo.num_channels = probe->sendSync ? 385 : 384;
                    apInfo.sample_rate = probe->ap_sample_rate;
                    apInfo.probe = probe;
                    apInfo.probe_index = probe_index;
                    apInfo.type = QUAD_BASE;
                    apInfo.shank = shank;

                    apInfo.sendSyncAsContinuousChannel = probe->sendSync;

                    streamInfo.add (apInfo);

                    sourceBuffers.add (new DataBuffer (apInfo.num_channels, 460800)); // AP band buffer

                    probe->quadBaseBuffers.add (sourceBuffers.getLast());

                    LOGD ("Probe (slot=", probe->basestation->slot, ", port=", probe->headstage->port, ") SHANK=", shank + 1, " CH = ", 384, " SR = ", apInfo.sample_rate, " Hz");
                }

                probe_index++;
            }
        }
        else
        {
            if (true)
            {
                StreamInfo adcInfo;

                adcInfo.num_channels = source->channel_count;
                adcInfo.sample_rate = source->sample_rate;
                adcInfo.type = ADC;
                adcInfo.sendSyncAsContinuousChannel = false;
                adcInfo.probe = nullptr;
                adcInfo.adc = (OneBoxADC*) source;

                sourceBuffers.add (new DataBuffer (adcInfo.num_channels, 10000)); // data buffer
                source->apBuffer = sourceBuffers.getLast();

                streamInfo.add (adcInfo);
            }
        }
    }

    LOGD ("Finished updating stream info");
}

NeuropixThread::~NeuropixThread()
{

     LOGD ("NeuropixThread destructor.");

     editor->uiLoader->waitForThreadToExit (-1);

    closeConnection();
}

void NeuropixThread::updateProbeSettingsQueue (ProbeSettings settings)
{
    probeSettingsUpdateQueue.add (settings);
}

void NeuropixThread::applyProbeSettingsQueue()
{
    configurationComplete = false;

    for (auto settings : probeSettingsUpdateQueue)
    {
        settings.probe->setStatus (SourceStatus::UPDATING);
    }

    Array<Probe*> uncalibratedProbes;

    // Apply settings to probes
    for (auto settings : probeSettingsUpdateQueue)
    {
        if (settings.probe->basestation->isBusy())
            settings.probe->basestation->waitForThreadToExit();

        LOGC ("Applying probe settings for ", settings.probe->name);

        if (settings.probe != nullptr)
        {
            settings.probe->selectElectrodes();
            settings.probe->setAllGains();
            settings.probe->setAllReferences();
            settings.probe->setApFilterState();

            settings.probe->calibrate();

            settings.probe->writeConfiguration();

            if (settings.probe->isEnabled)
                settings.probe->setStatus (SourceStatus::CONNECTED);
            else
                settings.probe->setStatus (SourceStatus::DISABLED);

            if (! settings.probe->isCalibrated)
                uncalibratedProbes.add (settings.probe);

            //Update probe map
            int slot = settings.probe->basestation->slot;
            int port = settings.probe->headstage->port;
            int dock = settings.probe->dock;

            std::tuple<int, int, int> key = std::make_tuple (slot, port, dock);

            probeMap[key] = std::make_pair (settings.probe->info.serial_number, settings);

            LOGC ("Wrote configuration");
        }
    }

    probeSettingsUpdateQueue.clear();

    // Show warning if any probes are uncalibrated
    if (uncalibratedProbes.size() > 0 && ! calibrationWarningShown)
    {
        String message = "Missing calibration files for the following probe(s):\n\n";

        for (auto probe : uncalibratedProbes)
        {
            message += String::fromUTF8 (" \xe2\x80\xa2 ") + probe->name + " (serial number: " + String (probe->info.serial_number) + ")\n";
        }

        message += "\nADC and Gain calibration files must be located in \"CalibrationInfo\\<serial_number>\" folder in the directory where the Open Ephys GUI was launched. ";
        message += "The GUI will proceed without calibration. The plugin must be deleted and re-inserted once calibration files have been added";

        // Show alert window on the message thread asynchronously
        MessageManager::callAsync ([this, message]
                                   { auto* alertWindow = new AlertWindow ("Calibration files not found", message, MessageBoxIconType::WarningIcon, nullptr);
                                   alertWindow->addButton ("OK", 1, KeyPress (KeyPress::returnKey), KeyPress (KeyPress::escapeKey));
                                   alertWindow->enterModalState (true, nullptr, true); });

        uncalibratedProbes.clear();
        calibrationWarningShown = true;
    }

    configurationComplete = true;
}

void NeuropixThread::initialize (bool signalChainIsLoading)
{
    editor->initialize (signalChainIsLoading);
}

bool NeuropixThread::isReady()
{
    return initializationComplete && configurationComplete;
}

void NeuropixThread::initializeBasestations (bool signalChainIsLoading)
{
    // slower task, run in background thread

    LOGD ("NeuropixThread::initializeBasestations");

    for (auto basestation : basestations)
    {
        basestation->initialize (signalChainIsLoading); // prepares probes for acquisition; may be slow
    }

    //Neuropixels::setParameter (Neuropixels::NP_PARAM_BUFFERSIZE, MAXSTREAMBUFFERSIZE);
    //Neuropixels::setParameter (Neuropixels::NP_PARAM_BUFFERCOUNT, MAXSTREAMBUFFERCOUNT);

    initializationComplete = true;
}

Array<Basestation*> NeuropixThread::getBasestations()
{
    Array<Basestation*> bs;

    for (auto bs_ : basestations)
    {
        bs.add (bs_);
    }

    return bs;
}

Array<OneBox*> NeuropixThread::getOneBoxes()
{
    Array<OneBox*> bs;

    for (auto bs_ : basestations)
    {
        if (bs_->type == BasestationType::ONEBOX)
            bs.add ((OneBox*) bs_);
    }

    return bs;
}

Array<Basestation*> NeuropixThread::getOptoBasestations()
{
    Array<Basestation*> bs;

    for (auto bs_ : basestations)
    {
        if (bs_->type == BasestationType::OPTO)
            bs.add ((Basestation*) bs_);
    }

    return bs;
}

Array<Probe*> NeuropixThread::getProbes()
{
    Array<Probe*> probes;

    for (auto bs : basestations)
    {
        for (auto probe : bs->getProbes())
        {
            probes.add (probe);
        }
    }

    return probes;
}

String NeuropixThread::getProbeInfoString()
{
    DynamicObject output;

    output.setProperty (Identifier ("plugin"),
                        var ("Neuropix-PXI"));
    output.setProperty (Identifier ("version"),
                        var (PLUGIN_VERSION));

    Array<var> probes;

    for (auto probe : getProbes())
    {
        DynamicObject::Ptr p = new DynamicObject();

        p->setProperty (Identifier ("name"), probe->displayName);
        p->setProperty (Identifier ("type"), probe->probeMetadata.name);
        p->setProperty (Identifier ("slot"), probe->basestation->slot);
        p->setProperty (Identifier ("port"), probe->headstage->port);
        p->setProperty (Identifier ("dock"), probe->dock);

        p->setProperty (Identifier ("part_number"), probe->info.part_number);
        p->setProperty (Identifier ("serial_number"), String (probe->info.serial_number));

        p->setProperty (Identifier ("is_calibrated"), probe->isCalibrated);

        probes.add (p.get());
    }

    output.setProperty (Identifier ("probes"), probes);

    MemoryOutputStream f;
    output.writeAsJSON (f, JSON::FormatOptions {}.withIndentLevel (0).withSpacing (JSON::Spacing::singleLine).withMaxDecimalPlaces (4));

    return f.toString();
}

Array<DataSource*> NeuropixThread::getDataSources()
{
    Array<DataSource*> sources;

    for (auto bs : basestations)
    {
        for (auto probe : bs->getProbes())
        {
            sources.add (probe);
        }

        for (auto additionalSource : bs->getAdditionalDataSources())
        {
            sources.add (additionalSource);
        }
    }

    return sources;
}

String NeuropixThread::getApiVersion()
{
    return api_v3.info.version;
}

void NeuropixThread::setMainSync (int slotIndex)
{

    LOGC ("NeuropixThread::setMainSync");

    if (foundInputSource() && slotIndex > -1)
    {
        for (int i = 0; i < basestations.size(); i++)
        {
            if (basestations[i]->type == BasestationType::PXI)
            {
                if (i == slotIndex)
                    basestations[i]->setSyncAsInput();
                else
                    basestations[i]->setSyncAsPassive();
            }
            else
            {
                basestations[i]->setSyncAsInput();
            }
		}
    }
}

void NeuropixThread::setSyncOutput (int slotIndex)
{

    LOGC ("NeuropixThread::setSyncOutput");

    if (foundInputSource() && slotIndex > -1)
    {
        for (int i = 0; i < basestations.size(); i++)
        {
            if (i == slotIndex)
                basestations[i]->setSyncAsOutput(0);
            else
                basestations[i]->setSyncAsPassive();
        }
    }
}

Array<int> NeuropixThread::getSyncFrequencies()
{
    if (foundInputSource())
        return basestations[0]->getSyncFrequencies();
    else
        return defaultSyncFrequencies;
}

void NeuropixThread::setSyncFrequency (int slotIndex, int freqIndex)
{
    if (foundInputSource() && slotIndex > -1)
        basestations[slotIndex]->setSyncAsOutput (freqIndex);
}

void NeuropixThread::closeConnection()
{
}

/** Returns true if the data source is connected, false otherwise.*/
bool NeuropixThread::foundInputSource()
{
    return baseStationAvailable;
}

XmlElement NeuropixThread::getInfoXml()
{
    XmlElement neuropix_info ("NEUROPIX-PXI");

    XmlElement* api_info = new XmlElement ("API");

    api_info->setAttribute ("version", api_v3.info.version);

    neuropix_info.addChildElement (api_info);

    for (int i = 0; i < basestations.size(); i++)
    {
        XmlElement* basestation_info = new XmlElement ("BASESTATION");
        basestation_info->setAttribute ("index", i + 1);
        basestation_info->setAttribute ("slot", int (basestations[i]->slot));
        basestation_info->setAttribute ("firmware_version", basestations[i]->info.boot_version);
        basestation_info->setAttribute ("bsc_firmware_version", basestations[i]->basestationConnectBoard->info.boot_version);
        basestation_info->setAttribute ("bsc_part_number", basestations[i]->basestationConnectBoard->info.part_number);
        basestation_info->setAttribute ("bsc_serial_number", String (basestations[i]->basestationConnectBoard->info.serial_number));

        for (auto probe : basestations[i]->getProbes())
        {
            XmlElement* probe_info = new XmlElement ("PROBE");
            probe_info->setAttribute ("port", probe->headstage->port);
            probe_info->setAttribute ("port", probe->dock);
            probe_info->setAttribute ("probe_serial_number", String (probe->info.serial_number));
            probe_info->setAttribute ("hs_serial_number", String (probe->headstage->info.serial_number));
            probe_info->setAttribute ("hs_part_number", probe->info.part_number);
            probe_info->setAttribute ("hs_version", probe->headstage->info.version);
            probe_info->setAttribute ("flex_part_number", probe->flex->info.part_number);
            probe_info->setAttribute ("flex_version", probe->flex->info.version);

            basestation_info->addChildElement (probe_info);
        }

        neuropix_info.addChildElement (basestation_info);
    }

    return neuropix_info;
}

String NeuropixThread::getInfoString()
{
    String infoString;

    infoString += "API Version: ";
    infoString += api_v3.info.version;
    infoString += "\n";
    infoString += "\n";
    infoString += "\n";

    for (int i = 0; i < basestations.size(); i++)
    {
        infoString += "Basestation ";
        infoString += String (i + 1);
        infoString += "\n";
        infoString += "  Firmware version: ";
        infoString += basestations[i]->info.boot_version;
        infoString += "\n";
        infoString += "  BSC firmware version: ";
        infoString += basestations[i]->basestationConnectBoard->info.boot_version;
        infoString += "\n";
        infoString += "  BSC part number: ";
        infoString += basestations[i]->basestationConnectBoard->info.part_number;
        infoString += "\n";
        infoString += "  BSC serial number: ";
        infoString += String (basestations[i]->basestationConnectBoard->info.serial_number);
        infoString += "\n";
        infoString += "\n";

        for (auto probe : basestations[i]->getProbes())
        {
            infoString += "    Port ";
            infoString += String (probe->headstage->port);
            infoString += "\n";
            infoString += "\n";
            infoString += "    Probe serial number: ";
            infoString += String (probe->info.serial_number);
            infoString += "\n";
            infoString += "\n";
            infoString += "    Headstage serial number: ";
            infoString += String (probe->headstage->info.serial_number);
            infoString += "\n";
            infoString += "    Headstage part number: ";
            infoString += probe->headstage->info.part_number;
            infoString += "\n";
            infoString += "    Headstage version: ";
            infoString += probe->headstage->info.version;
            infoString += "\n";
            infoString += "\n";
            infoString += "    Flex part number: ";
            infoString += probe->flex->info.part_number;
            infoString += "\n";
            infoString += "    Flex version: ";
            infoString += probe->flex->info.version;
            infoString += "\n";
            infoString += "\n";
            infoString += "\n";
        }
        infoString += "\n";
        infoString += "\n";
    }

    return infoString;
}

/** Initializes data transfer.*/
bool NeuropixThread::startAcquisition()
{
    if (editor->uiLoader->isThreadRunning())
    {
        LOGD ("Waiting for Neuropixels settings thread to exit.");
        editor->uiLoader->waitForThreadToExit (20000);
        LOGD ("Neuropixels settings thread finished.");
    }

    for (int i = 0; i < basestations.size(); i++)
    {
        basestations[i]->startAcquisition();
    }

    return true;
}

void NeuropixThread::setDirectoryForSlot (int slotIndex, File directory)
{
    LOGD ("Thread setting directory for slot ", slotIndex, " to ", directory.getFileName());

    if (slotIndex < basestations.size())
    {
        basestations[slotIndex]->setSavingDirectory (directory);
    }
}

File NeuropixThread::getDirectoryForSlot (int slotIndex)
{
    if (slotIndex < basestations.size())
    {
        return basestations[slotIndex]->getSavingDirectory();
    }
    else
    {
        return File::getCurrentWorkingDirectory();
    }
}

void NeuropixThread::setNamingSchemeForSlot (int slot, ProbeNameConfig::NamingScheme namingScheme)
{
    for (auto bs : getBasestations())
        if (bs->slot == slot)
            bs->setNamingScheme (namingScheme);
}

ProbeNameConfig::NamingScheme NeuropixThread::getNamingSchemeForSlot (int slot)
{
    for (auto bs : getBasestations())
        if (bs->slot == slot)
            return bs->getNamingScheme();
}

/** Stops data transfer.*/
bool NeuropixThread::stopAcquisition()
{
    if (isThreadRunning())
    {
        signalThreadShouldExit();
    }

    for (int i = 0; i < basestations.size(); i++)
    {
        basestations[i]->stopAcquisition();
    }

    return true;
}

void NeuropixThread::updateSettings (OwnedArray<ContinuousChannel>* continuousChannels,
                                     OwnedArray<EventChannel>* eventChannels,
                                     OwnedArray<SpikeChannel>* spikeChannels,
                                     OwnedArray<DataStream>* dataStreams,
                                     OwnedArray<DeviceInfo>* devices,
                                     OwnedArray<ConfigurationObject>* configurationObjects)
{
    bool checkStreamNames = true;

    if (sourceStreams.size() == 0) // initialize data streams
    {
        String lastName;

        for (auto info : streamInfo)
        {
            String streamName, description, identifier;

            if (info.type == stream_type::ADC)
            {
                streamName = "OneBox-ADC";
                description = "OneBox ADC data stream";
                identifier = "onebox.adc";
            }

            else if (info.type == stream_type::AP_BAND)
            {
                lastName = generateProbeName (info.probe_index, info.probe->namingScheme);

                if (info.probe->namingScheme != ProbeNameConfig::STREAM_INDICES)
                    streamName = lastName + "-AP";
                else
                    streamName = String (info.probe->streamIndex);

                description = "Neuropixels AP band data stream";
                identifier = "neuropixels.data.ap";
            }

            else if (info.type == stream_type::BROAD_BAND)
            {
                streamName = generateProbeName (info.probe_index, info.probe->namingScheme);
                description = "Neuropixels data stream";
                identifier = "neuropixels.data";
            }

            else if (info.type == stream_type::LFP_BAND)
            {
                if (info.probe->namingScheme != ProbeNameConfig::STREAM_INDICES)
                    streamName = lastName + "-LFP";
                else
                    streamName = String (info.probe->streamIndex + 1);

                description = "Neuropixels LFP band data stream";
                identifier = "neuropixels.data.lfp";
            }

            else if (info.type == stream_type::QUAD_BASE)
            {
                lastName = generateProbeName (info.probe_index, info.probe->namingScheme);

                if (info.probe->namingScheme != ProbeNameConfig::STREAM_INDICES)
                    streamName = lastName + "-" + String (info.shank + 1);
                else
                    streamName = String (info.probe->streamIndex);

                description = "Neuropixels Quad Base data stream";
                identifier = "neuropixels.data.quad_base";
            }

            DataStream::Settings settings {
                streamName,
                "description",
                "identifier",

                info.sample_rate

            };

            sourceStreams.add (new DataStream (settings));

            checkStreamNames = false;
        }
    }

    dataStreams->clear();
    eventChannels->clear();
    continuousChannels->clear();
    spikeChannels->clear();
    devices->clear();
    configurationObjects->clear();

    int probeIdx = 0;

    for (int i = 0; i < sourceStreams.size(); i++)
    {
        DataStream* currentStream = sourceStreams[i];

        StreamInfo info = streamInfo[i];

        if (checkStreamNames)
        {
            String streamName;

            if (info.type == stream_type::AP_BAND)
            {
                Probe* p = getProbes()[probeIdx];
                p->updateNamingScheme (p->namingScheme); // update displayName

                streamName = generateProbeName (probeIdx, p->namingScheme);

                if (p->namingScheme != ProbeNameConfig::STREAM_INDICES)
                    streamName += "-AP";
                else
                    streamName = String (p->streamIndex);
            }
            else if (info.type == stream_type::LFP_BAND)
            {
                Probe* p = getProbes()[probeIdx];
                p->updateNamingScheme (p->namingScheme); // update displayName

                streamName = generateProbeName (probeIdx, p->namingScheme);

                if (p->namingScheme != ProbeNameConfig::STREAM_INDICES)
                    streamName += "-LFP";
                else
                    streamName = String (p->streamIndex + 1);

                probeIdx++;
            }
            else if (info.type == stream_type::BROAD_BAND)
            {
                Probe* p = getProbes()[probeIdx];
                p->updateNamingScheme (p->namingScheme); // update displayName

                streamName = generateProbeName (probeIdx, p->namingScheme);

                probeIdx++;
            }
            else if (info.type == stream_type::QUAD_BASE)
            {
                Probe* p = getProbes()[probeIdx];
                p->updateNamingScheme (p->namingScheme); // update displayName

                streamName = generateProbeName (probeIdx, p->namingScheme);

                if (p->namingScheme != ProbeNameConfig::STREAM_INDICES)
                    streamName += "-" + String (info.shank + 1);
                else
                    streamName = String (p->streamIndex + info.shank);

                if (info.shank == 3)
                    probeIdx++;
            }
            else
            {
                streamName = currentStream->getName();
            }

            currentStream->setName (streamName);
        }

        ContinuousChannel::Type type;

        String description, identifier;

        if (info.type == stream_type::ADC)
        {
            type = ContinuousChannel::Type::ADC;
            description = "OneBox ADC channel";
            identifier = "neuropixels.adc";
        }
        else
        {
            type = ContinuousChannel::Type::ELECTRODE;
            description = "Neuropixels electrode";
            identifier = "neuropixels.electrode";
        }

        currentStream->clearChannels();

        for (int ch = 0; ch < info.num_channels; ch++)
        {
            float bitVolts;

            if (info.type == stream_type::ADC)
            {
                bitVolts = info.adc->getChannelGain (ch);
            }
            else
            {
                bitVolts = 0.1950000f;
            }

            String name;

            if (info.type == stream_type::ADC)
            {
                name = "ADC";
            }
            else if (info.type == stream_type::AP_BAND)
            {
                name = "AP";
            }
            else if (info.type == stream_type::LFP_BAND)
            {
                name = "LFP";
            }
            else
            {
                name = "CH";
            }

            float depth = -1.0f;

            if (info.sendSyncAsContinuousChannel && (ch == info.num_channels - 1))
            {
                type = ContinuousChannel::Type::ADC;
                name += "_SYNC";
                bitVolts = 1.0;
                description = "Neuropixels sync line (continuously sampled)";
                identifier = "neuropixels.sync";
            }
            else
            {
                name += String (ch);
            }

            ContinuousChannel::Settings settings {
                type,
                name,
                description,
                identifier,

                bitVolts,

                currentStream
            };

            continuousChannels->add (new ContinuousChannel (settings));

            if (type == ContinuousChannel::Type::ELECTRODE)
            {
                float depth = 0.0f;
                int chIndex = 0;

                if (info.probe->type != ProbeType::UHD2)
                {
                    chIndex = info.probe->settings.selectedChannel.indexOf (ch);

                    Array<Bank> availableBanks = info.probe->settings.availableBanks;

                    int selectedBank = availableBanks.indexOf (info.probe->settings.selectedBank[chIndex]);

                    int selectedElectrode = info.probe->settings.selectedElectrode[chIndex];
                    int shank = info.probe->settings.selectedShank[chIndex];

                    depth = float (info.probe->electrodeMetadata[selectedElectrode].ypos)
                            + shank * 10000.0f
                            + info.probe->electrodeMetadata[selectedElectrode].xpos * 0.001f;

                    if (false)
                        std::cout << "Channel: " << ch << " chIndex: " << chIndex << " ypos: " << info.probe->electrodeMetadata[selectedElectrode].ypos << " xpos: " << info.probe->electrodeMetadata[selectedElectrode].xpos << " depth: " << depth << std::endl;
                }
                else
                {
                    int electrodeIndex = 0;

                    for (int i = 0; i < info.probe->settings.selectedElectrode.size(); i++)
                    {
                        if (info.probe->settings.selectedChannel[i] == ch)
                        {
                            electrodeIndex = info.probe->settings.selectedElectrode[i];
                            break;
                        }
                    }

                    depth = float (info.probe->electrodeMetadata[electrodeIndex].ypos)
                            + info.probe->electrodeMetadata[electrodeIndex].xpos * 0.001f;

                    if (false)
                        std::cout << "Channel: " << ch << " electrodeIndex: " << electrodeIndex << " ypos: " << info.probe->electrodeMetadata[electrodeIndex].ypos << " xpos: " << info.probe->electrodeMetadata[electrodeIndex].xpos << " depth: " << depth << std::endl;
                }

                continuousChannels->getLast()->position.y = depth;

                MetadataDescriptor descriptor (MetadataDescriptor::MetadataType::UINT16,
                                               1,
                                               "electrode_index",
                                               "Electrode index for this channel",
                                               "neuropixels.electrode_index");

                MetadataValue value (MetadataDescriptor::MetadataType::UINT16, 1);
                value.setValue ((uint16) info.probe->settings.selectedElectrode[chIndex]);

                //LOGD("Setting channel ", ch, " electrode_index metadata value to ", (uint16)info.probe->settings.selectedElectrode[chIndex]);

                continuousChannels->getLast()->addMetadata (descriptor, value);
            }

        } // end channel loop

        if (info.type != stream_type::ADC)
        {
            EventChannel::Settings settings {
                EventChannel::Type::TTL,
                "Neuropixels PXI Sync",
                "Status of SMA sync line on PXI card",
                "neuropixels.sync",
                currentStream,
                1
            };

            eventChannels->add (new EventChannel (settings));
        }
        else
        {
            EventChannel::Settings settings {
                EventChannel::Type::TTL,
                "OneBox ADC Digital Lines",
                "Status of digital inputs on OneBox ADC",
                "onebox.sync",
                currentStream,
                13
            };

            eventChannels->add (new EventChannel (settings));
        }

        dataStreams->add (new DataStream (*currentStream)); // copy existing stream

        if (info.probe != nullptr)
        {
            DeviceInfo::Settings deviceSettings {
                info.probe->name,
                "Neuropixels probe",
                info.probe->info.part_number,

                String (info.probe->info.serial_number),
                "imec"
            };

            DeviceInfo* device = new DeviceInfo (deviceSettings);

            MetadataDescriptor descriptor (MetadataDescriptor::MetadataType::UINT16,
                                           1,
                                           "num_adcs",
                                           "Number of analog-to-digital converter for this probe",
                                           "neuropixels.adcs");

            MetadataValue value (MetadataDescriptor::MetadataType::UINT16, 1);
            value.setValue ((uint16) info.probe->probeMetadata.num_adcs);

            device->addMetadata (descriptor, value);

            devices->add (device); // unique device object owned by SourceNode

            dataStreams->getLast()->device = device; // DataStream object just gets a pointer
        }

    } // end source stream loop

    editor->update();
}

void NeuropixThread::sendSyncAsContinuousChannel (bool shouldSend)
{
    for (auto probe : getProbes())
    {
        LOGD ("Setting sendSyncAsContinuousChannel to: ", shouldSend);
        probe->sendSyncAsContinuousChannel (shouldSend);
    }

    updateStreamInfo();
}

void NeuropixThread::setTriggerMode (bool trigger)
{
    internalTrigger = trigger;
}

void NeuropixThread::setAutoRestart (bool restart)
{
    autoRestart = restart;
}

void NeuropixThread::handleBroadcastMessage (const String& msg, const int64 messageTimeMillis)
{
    // Available commands:
    //
    // NP OPTO <bs> <port> <probe> <wavelength> <site>
    // NP WAVEPLAYER <bs> <"start"/"stop">
    //

    LOGD ("Neuropix-PXI received ", msg);

    StringArray parts = StringArray::fromTokens (msg, " ", "");

    if (parts[0].equalsIgnoreCase ("NP"))
    {
        LOGD ("Found NP command: ", msg);

        if (parts.size() > 0)
        {
            String command = parts[1];

            if (command.equalsIgnoreCase ("OPTO"))
            {
                if (parts.size() == 7)
                {
                    int slot = parts[2].getIntValue();
                    int port = parts[3].getIntValue();
                    int dock = parts[4].getIntValue();
                    String wavelength = parts[5];
                    int emitter = parts[6].getIntValue();

                    if (emitter < 0 || emitter > 14)
                    {
                        LOGD ("Invalid site number, must be between 0 and 14, got ", emitter);
                        return;
                    }

                    for (auto bs : getOptoBasestations())
                    {
                        if (bs->slot == slot)
                        {
                            for (auto probe : getProbes())
                            {
                                if (probe->basestation->slot == slot && probe->headstage->port == port && probe->dock == dock)
                                {
                                    probe->ui->setEmissionSite (wavelength, emitter);
                                }
                            }
                        }
                    }
                }
                else
                {
                    LOGD ("Incorrect number of argument for OPTO command. Found ", parts.size(), ", requires 7.");
                }
            }
            else if (command.equalsIgnoreCase ("OPTO"))

            {
                if (parts.size() == 4)
                {
                    int slot = parts[2].getIntValue();
                    bool shouldStart = parts[3].equalsIgnoreCase ("start");

                    for (auto bs : getOneBoxes())
                    {
                        if (bs->slot == slot)
                            bs->triggerWaveplayer (shouldStart);
                    }
                }
                else
                {
                    LOGD ("Incorrect number of argument for WAVEPLAYER message. Found ", parts.size(), ", requires 4.");
                }
            }
        }
    }
}

String NeuropixThread::handleConfigMessage (const String& msg)
{
    // Available commands:
    // NP SELECT <bs> <port> <dock> <electrode> <electrode> <electrode> ...
    // NP SELECT "<preset>"
    // NP GAIN <bs> <port> <dock> <AP/LFP> <gainval>
    // NP REFERENCE <bs> <port> <dock> <EXT/TIP>
    // NP FILTER <bs> <port> <dock> <ON/OFF>
    // NP INFO

    LOGD ("Neuropix-PXI received ", msg);

    StringArray parts = StringArray::fromTokens (msg, " ", "");

    if (parts[0].equalsIgnoreCase ("NP"))
    {
        LOGD ("Found NP command.");

        if (parts.size() > 0)
        {
            String command = parts[1];

            LOGD ("Command: ", command);

            if (command.equalsIgnoreCase ("INFO"))
            {
                return getProbeInfoString();
            }
            else
            {
                if (CoreServices::getAcquisitionStatus())
                {
                    return "Neuropixels plugin cannot update settings while acquisition is active.";
                }

                if (command.equalsIgnoreCase ("SELECT") || command.equalsIgnoreCase ("GAIN") || command.equalsIgnoreCase ("REFERENCE") || command.equalsIgnoreCase ("FILTER"))
                {
                    if (parts.size() > 5)
                    {
                        int slot = parts[2].getIntValue();
                        int port = parts[3].getIntValue();
                        int dock = parts[4].getIntValue();

                        LOGD ("Slot: ", slot, ", Port: ", port, ", Dock: ", dock);

                        for (auto probe : getProbes())
                        {
                            if (probe->basestation->slot == slot && probe->headstage->port == port && probe->dock == dock)
                            {
                                if (command.equalsIgnoreCase ("GAIN"))
                                {
                                    bool isApBand = parts[5].equalsIgnoreCase ("AP");
                                    float gain = parts[6].getFloatValue();

                                    if (isApBand)
                                    {
                                        if (probe->settings.availableApGains.size() > 0)
                                        {
                                            int gainIndex = probe->settings.availableApGains.indexOf (gain);

                                            if (gainIndex > -1)
                                            {
                                                probe->ui->setApGain (gainIndex);
                                            }
                                        }
                                    }
                                    else
                                    {
                                        if (probe->settings.availableLfpGains.size() > 0)
                                        {
                                            int gainIndex = probe->settings.availableLfpGains.indexOf (gain);

                                            if (gainIndex > -1)
                                            {
                                                probe->ui->setLfpGain (gainIndex);
                                            }
                                        }
                                    }
                                }
                                else if (command.equalsIgnoreCase ("REFERENCE"))
                                {
                                    int referenceIndex = 0;

                                    if (parts[5].equalsIgnoreCase ("EXT"))
                                    {
                                        referenceIndex = 0;
                                    }
                                    else if (parts[5].equalsIgnoreCase ("TIP"))
                                    {
                                        referenceIndex = 1;
                                    }

                                    probe->ui->setReference (referenceIndex);
                                }
                                else if (command.equalsIgnoreCase ("FILTER"))
                                {
                                    if (probe->hasApFilterSwitch())
                                    {
                                        probe->ui->setApFilterState (parts[5].equalsIgnoreCase ("ON"));
                                    }
                                }
                                else if (command.equalsIgnoreCase ("SELECT"))
                                {
                                    Array<int> electrodes;

                                    if (parts[5].substring (0, 1) == "\"")
                                    {
                                        String presetName = msg.fromFirstOccurrenceOf ("\"", false, false).upToFirstOccurrenceOf ("\"", false, false);

                                        LOGD ("Selecting preset: ", presetName);

                                        electrodes = probe->selectElectrodeConfiguration (presetName);

                                        probe->ui->selectElectrodes (electrodes);
                                    }
                                    else
                                    {
                                        LOGD ("Selecting electrodes: ")

                                        for (int i = 5; i < parts.size(); i++)
                                        {
                                            int electrode = parts[i].getIntValue();

                                            //std::cout << electrode << std::endl;

                                            if (electrode > 0 && electrode < probe->electrodeMetadata.size() + 1)
                                                electrodes.add (electrode - 1);
                                        }

                                        probe->ui->selectElectrodes (electrodes);
                                    }
                                }
                            }
                        }

                        return "SUCCESS";
                    }
                    else
                    {
                        return "Incorrect number of argument for " + command + ". Found " + String (parts.size()) + ", requires 6.";
                    }
                }
                else
                {
                    return "NP command " + command + " not recognized.";
                }
            }
        }
    }

    return "Command not recognized.";
}

String NeuropixThread::getCustomProbeName (String serialNumber)
{
    if (customProbeNames.count (serialNumber) > 0)
    {
        return customProbeNames[serialNumber];
    }
    else
    {
        return "";
    }
}

void NeuropixThread::setCustomProbeName (String serialNumber, String customName)
{
    customProbeNames[serialNumber] = customName;
}
