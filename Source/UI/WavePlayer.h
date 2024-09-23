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

#ifndef __WAVEPLAYER_H__
#define __WAVEPLAYER_H__

#include <VisualizerEditorHeaders.h>

#include "AnalogPatternGenerator.h"

class UtilityButton;

/**

User interface for defining custom OneBox DAC waveforms

*/

class OneBoxDAC;
class OneBoxADC;
class OneBoxInterface;
class PulsePatternGenerator;
class SinePatternGenerator;
class CustomPatternGenerator;
class AnalogPatternInfo;

/** 

	Draws WavePlayer background

*/
class WavePlayerBackground : public Component
{
public:
    /** Constructor */
    WavePlayerBackground();

    /** Sets current waveform pattern */
    void updateCurrentWaveform (Pattern* pattern);

private:
    /** Paint interface */
    void paint (Graphics& g);

    Path currentWaveform;
    AffineTransform pathTransform;
};

/**

	Defines a waveform for OneBox DAC

*/
class WavePlayer : public Component,
                   public ComboBox::Listener,
                   public Button::Listener,
        		   public Timer    
{
public:
    /** Constructor */
    WavePlayer (OneBoxDAC*, OneBoxADC*, OneBoxInterface*);

    /** Destructor */
    virtual ~WavePlayer();

    /** Button callback */
    void buttonClicked (Button* button);

    /** ComboBox callback */
    void comboBoxChanged (ComboBox*);

    /** Saves parameters to XML*/
    void saveCustomParameters (XmlElement*);

    /** Loads parameters from XML*/
    void loadCustomParameters (XmlElement*);

    /** Returns waveform sample rate*/
    float getSampleRate();

    /** Updates the waveform to be triggered */
    void updateWaveform();

    /** Resizes interface */
    void resized();

    /** Reverts toggle state of "Run" button */
    void timerCallback();

    OwnedArray<Pattern> availablePatterns;
    Pattern* currentPattern;

private:
    std::unique_ptr<ComboBox> patternSelector;
    std::unique_ptr<ComboBox> triggerSelector;

    std::unique_ptr<UtilityButton> enableButton;
    std::unique_ptr<UtilityButton> pulsePatternButton;
    std::unique_ptr<UtilityButton> sinePatternButton;
    std::unique_ptr<UtilityButton> customPatternButton;

    std::unique_ptr<UtilityButton> startStopButton;

    std::unique_ptr<WavePlayerBackground> background;

    void selectPatternType (PatternType t);
    void updatePatternSelector();
    void initializePattern (Pattern* pattern);

    OneBoxDAC* dac;
    OneBoxADC* adc;
    OneBoxInterface* ui;
    int nextPatternId;
};

#endif // __WAVEPLAYER_H__
