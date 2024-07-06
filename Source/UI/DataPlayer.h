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

#ifndef __DATAPLAYER_H__
#define __DATAPLAYER_H__

#include <VisualizerEditorHeaders.h>

#include "../NeuropixComponents.h"
#include "AnalogPatternGenerator.h"

class Probe;

class OneBoxInterface;

class UtilityButton;

/**

User interface for defining custom OneBox DAC waveforms

*/

class OneBoxDAC;
class OneBoxADC;

enum StreamType
{
    AP = 1,
    LFP = 2
};

class DataPlayerBackground : public Component
{
public:
    DataPlayerBackground();

private:
    void paint (Graphics& g);
    Path spikePath;
    AffineTransform pathTransform;
};

class DataPlayer : public Component,
                   public ComboBox::Listener
{
public:
    DataPlayer (OneBoxDAC*, OneBoxADC*, OneBoxInterface*);
    virtual ~DataPlayer();

    void comboBoxChanged (ComboBox*);

    void saveCustomParameters (XmlElement*);
    void loadCustomParameters (XmlElement*);

    void setAvailableChans (Array<DataSourceType> channelTypes);

    void resized();

private:
    std::unique_ptr<ComboBox> probeSelector;
    std::unique_ptr<ComboBox> streamSelector;
    std::unique_ptr<ComboBox> channelSelector;
    std::unique_ptr<ComboBox> outputSelector;

    std::unique_ptr<ComboBox> playerIndex;

    std::unique_ptr<DataPlayerBackground> background;

    OneBoxADC* adc;
    OneBoxDAC* dac;
    OneBoxInterface* onebox;

    Probe* selectedProbe;
    int inputChan;
    int outputChan;
    StreamType streamType;

    Array<Probe*> availableProbes;
};

#endif // __DATAPLAYER_H__
