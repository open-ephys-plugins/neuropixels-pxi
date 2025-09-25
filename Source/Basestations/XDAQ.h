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
#pragma once

#include "../NeuropixComponents.h"

/** 
	Communicates with a XDAQ
*/
class XDAQ_BS : public Basestation

{
public:
    /** Constructor */
    XDAQ_BS (NeuropixThread*, int serial_number);

    /** Destructor */
    ~XDAQ_BS();

    /** Opens connection to XDAQ */
    bool open() override;

    /** Closes connection to XDAQ */
    void close() override;

    /** Initializes in a separate thread*/
    void initialize (bool signalChainIsLoading) override;

    /** Search for probes connected to OneBox */
    void searchForProbes() override;

    /** Returns the total number of connected probes */
    int getProbeCount() override;

    /** Gets info about this device */
    void getInfo() override;

    /** Sets the SMA port to input mode */
    void setSyncAsInput() override;

    /** Sets the SMA port to output mode */
    void setSyncAsOutput (int freqIndex) override;

    /** Sets the OneBox as passive input (does nothing) */
    void setSyncAsPassive() override;

    /** Returns the available sync frequencies*/
    Array<int> getSyncFrequencies() override;

    /** Starts acquisition on all probes */
    void startAcquisition() override;

    /** Stops acquisition on all probes */
    void stopAcquisition() override;

    /** Gets fill percentage of XDAQ FIFO buffer */
    float getFillPercentage() override;

    /** Returns any non-probe data sources (e.g. ADCs)*/
    Array<DataSource*> getAdditionalDataSources() override;

    /** Triggers the Waveplayer output */
    // void triggerWaveplayer (bool shouldStart);

    // static Array<int> existing_oneboxes;
    // int serial_number = -1;

    // const int first_available_slot = 16;

    // std::unique_ptr<OneBoxADC> adcSource;
    // std::unique_ptr<OneBoxDAC> dacSource;
};
