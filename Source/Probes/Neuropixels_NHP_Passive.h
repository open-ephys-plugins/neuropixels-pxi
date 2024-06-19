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

#ifndef __NEUROPIXNHP_PASSIVE_H_2C4C2D67__
#define __NEUROPIXNHP_PASSIVE_H_2C4C2D67__

#include "../NeuropixComponents.h"

#define MAXPACKETS 64

/**

	Acquires data from a 128-channel Neuropixels NHP Passive
	probe connected to a digitizing headstage, using IMEC's v3 API.

*/

class Neuropixels_NHP_Passive : public Probe
{
public:
    /** Constructor */
    Neuropixels_NHP_Passive (Basestation*,
                             Headstage*,
                             Flex*);

    /** Reads probe part number and serial number */
    void getInfo() override;

    /** Opens the connection to the probe (fast) */
    bool open() override;

    /** Closes the connection to the probe (fast) */
    bool close() override;

    /** Call init, setOPMODE, and setHSLED (slow) */
    void initialize (bool signalChainIsLoading) override;

    /** Selects active electrodes based on settings.selectedChannel */
    void selectElectrodes() override;

    /** Select a preset electrode configuration */
    Array<int> selectElectrodeConfiguration (String config);

    /** Sets reference for all channels based on settings.referenceIndex */
    void setAllReferences() override;

    /** Sets gains for all channels based on settings.apGainIndex and settings.lfpGainIndex */
    void setAllGains() override;

    /** Sets AP filter cut based on settings.apFilterState */
    void setApFilterState() override;

    /** Writes latest settings to the probe (slow) */
    void writeConfiguration() override;

    /** Resets timestamps, clears buffers, and starts the thread*/
    void startAcquisition() override;

    /** Stops the thread */
    void stopAcquisition() override;

    /** Runs a built-in self test. */
    bool runBist (BIST bistType) override;

    /** Uploads ADC and gain calibration files */
    void calibrate() override;

    /** Signals that this probe has an LFP data stream*/
    bool generatesLfpData() { return true; }

    /** Signals that this probe has an AP filter switch*/
    bool hasApFilterSwitch() { return true; }

    /** Acquires data from the probe */
    void run() override; // acquire data

private:
    Neuropixels::electrodePacket packet[SAMPLECOUNT];
    Neuropixels::NP_ErrorCode errorCode;

    Array<int> channel_map;

    int SKIP;

    float apSamples[385 * 12 * MAXPACKETS];
    float lfpSamples[385 * MAXPACKETS];
    int64 ap_timestamps[12 * MAXPACKETS];
    uint64 event_codes[12 * MAXPACKETS];
    int64 lfp_timestamps[MAXPACKETS];
    uint64 lfp_event_codes[MAXPACKETS];
};

#endif // _NEUROPIXNHP_PASSIVE_2C4C2D67__
