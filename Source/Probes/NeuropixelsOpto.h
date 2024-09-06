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

#ifndef __NEUROPIXOPTO_H_2C4C2D67__
#define __NEUROPIXOPTO_H_2C4C2D67__

#include "../NeuropixComponents.h"

#define MAXPACKETS 64

/**

	Acquires data from a Neuropixels Opto probe,
	using IMEC's v3 API.

*/

class NeuropixelsOpto : public Probe
{
public:
    /** Constructor */
    NeuropixelsOpto (Basestation*,
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

    /** Selects an emission site by wavelength and index. */
    void setEmissionSite (Neuropixels::wavelength_t wavelength, int site);

    /** Runs a built-in self test. */
    bool runBist (BIST bistType) override;

    /** Uploads ADC, gain, and optical switch calibration files */
    void calibrate() override;

    /** Signals that this probe has an LFP data stream*/
    bool generatesLfpData() { return true; }

    /** Signals that this probe has an AP filter switch*/
    bool hasApFilterSwitch() { return true; }

    /** Prints the current settings for this probe*/
    void printSettings();

    /** Acquires data from the probe */
    void run() override; // acquire data

private:
    Neuropixels::electrodePacket packet[MAXPACKETS];

    int SKIP;

    float apSamples[385 * 12 * MAXPACKETS];
    float lfpSamples[385 * MAXPACKETS];
    int64 ap_timestamps[12 * MAXPACKETS];
    uint64 event_codes[12 * MAXPACKETS];
    int64 lfp_timestamps[MAXPACKETS];
    uint64 lfp_event_codes[MAXPACKETS];
};

#endif // _NEUROPIXOPTO_H_2C4C2D67__