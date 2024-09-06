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

#include "OneBoxDAC.h"

#define MAXLEN 50

OneBoxDAC::OneBoxDAC (Basestation* bs_) : DataSource (bs_)
{
    sourceType = DataSourceType::DAC;

    channel_count = 0;
    sample_rate = 30000.0f;
}

void OneBoxDAC::initialize (bool signalChainIsLoading)
{
    errorCode = Neuropixels::waveplayer_setSampleFrequency (basestation->slot,
                                                            30000.0f);

    LOGD ("waveplayer_setSampleFrequency error code: ", errorCode);
}

void OneBoxDAC::setWaveform (Array<float> samples)
{
    Array<int16_t> samples_t;

    for (auto sample : samples)
        samples_t.add (int (sample / 5.0f * 65535));

    errorCode = Neuropixels::waveplayer_writeBuffer (basestation->slot, samples_t.getRawDataPointer(), samples_t.size());

    LOGC ("waveplayer_writeBuffer error code: ", errorCode);

    errorCode = Neuropixels::waveplayer_arm (basestation->slot, true);

    LOGC ("waveplayer_arm error code: ", errorCode);
}

void OneBoxDAC::playWaveform()
{
    errorCode = Neuropixels::setSWTriggerEx (basestation->slot, Neuropixels::swtrigger2);

    LOGD ("setSWTriggerEx error code: ", errorCode);
}

void OneBoxDAC::stopWaveform()
{
    LOGD ("Stop waveform not implemented.");
}

void OneBoxDAC::configureDataPlayer (int DACChannel, int portID, int dockID, int channelnr, int sourceType)
{
    Neuropixels::streamsource_t sourcetype = Neuropixels::SourceAP;

    if (sourceType == 1)
        sourceType = Neuropixels::SourceAP;
    else
        sourceType = Neuropixels::SourceLFP;

    errorCode = Neuropixels::DAC_setProbeSniffer (basestation->slot, DACChannel, portID, dockID, channelnr, sourcetype);

    LOGC ("DAC_setProbeSniffer error code: ", errorCode);
}

void OneBoxDAC::disableOutput (int chan)
{
    errorCode = Neuropixels::DAC_enableOutput (basestation->slot, chan, false);

    LOGC ("Disabling DAC ", chan);
}

void OneBoxDAC::enableOutput (int chan)
{
    errorCode = Neuropixels::DAC_enableOutput (basestation->slot, chan, true);

    LOGC ("Enabling DAC ", chan);
}
