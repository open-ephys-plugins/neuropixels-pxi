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

#ifndef __ONEBOX_DAC_H_2C4C2D67__
#define __ONEBOX_DAC_H_2C4C2D67__

#include "../NeuropixComponents.h"

class OneBoxADC;

/**

	Interface for OneBox DAC channels

	Each DAC line is shared with an ADC, and must be 
	enabled in order to be used.

*/
class OneBoxDAC : public DataSource
{
public:
    /** Constructor */
    OneBoxDAC (Basestation*);

    /** Returns name of data source */
    String getName() { return "DAC"; }

    /** Return info about part numbers, etc. -- not used*/
    void getInfo() override {}

    /** Open connection to the DACs -- not used */
    bool open() override { return true; }

    /** Close connection to the DACs -- not used */
    bool close() override { return true; }

    /** Initialize DAC settings */
    void initialize (bool signalChainIsLoading) override;

    /** Called when acquisition starts -- not used */
    void startAcquisition() override {}

    /** Called when acquisition stops -- not used */
    void stopAcquisition() override {}

    /** Adds data to buffer (not used) */
    void run() override {}

    /** Sets WavePlayer waveform */
    void setWaveform (Array<float> samples);

    /** Plays cued waveform */
    void playWaveform();

    /** Stops WavePlayer */
    void stopWaveform();

    /** Maps DataPlayer to a headstage channel */
    void configureDataPlayer (int DACChannel,
                              int portID,
                              int dockID,
                              int channel,
                              int sourceType);

    /** Enables DAC output channel */
    void disableOutput (int chan);

    /** Disables DAC output channel */
    void enableOutput (int chan);

};

#endif // __ONEBOX_DAC_H_2C4C2D67__