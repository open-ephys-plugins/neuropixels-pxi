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

#include "WavePlayer.h"
#include "AnalogPatternGenerator.h"
#include "OneBoxInterface.h"

#include "../Probes/OneBoxADC.h"
#include "../Probes/OneBoxDAC.h"

WavePlayerBackground::WavePlayerBackground()
{
    currentWaveform = Path();
}

void WavePlayerBackground::updateCurrentWaveform (Pattern* pattern)
{
    float waveformWidth = 80;

    float numSamples = (float) pattern->samples.size();

    float blockSize = numSamples / waveformWidth;

    currentWaveform.clear();

    currentWaveform.startNewSubPath (0, 1);

    float lastSample = pattern->samples[0];
    float lastIndex = 0;

    float maxVoltage = pattern->maxVoltage;

    if (maxVoltage == 0.0f)
    {
        maxVoltage = 5.0f;
    }

    currentWaveform.lineTo (0, 1 - lastSample / maxVoltage);

    for (int i = 0; i < pattern->samples.size(); i++)
    {
        if (pattern->samples[i] != lastSample)
        {
            currentWaveform.lineTo (float (i) / numSamples, 1 - lastSample / maxVoltage);
            currentWaveform.lineTo (float (i) / numSamples, 1 - pattern->samples[i] / maxVoltage);
            lastSample = pattern->samples[i];
            lastIndex = i;
        }
    }

    pathTransform = currentWaveform.getTransformToScaleToFit (140, 80, 160, 98, false);

    repaint();
}

void WavePlayerBackground::paint (Graphics& g)
{
    g.setColour (findColour (ThemeColours::defaultText));
    g.drawRect (0, 0, getWidth(), getHeight(), 1.0);

    g.setFont (20);
    g.drawText ("WavePlayer", 7, 5, 150, 20, Justification::left);

    //g.setFont (13);
    //g.drawText ("Trigger channel:", 12, 77, 150, 20, Justification::left);

    g.setColour (Colours::orange);
    g.strokePath (currentWaveform, PathStrokeType (1.0), pathTransform);
}

WavePlayer::WavePlayer (OneBoxDAC* dac_, OneBoxADC* adc_, OneBoxInterface* ui_)
    : dac (dac_),
      adc (adc_),
      ui (ui_),
      nextPatternId (1)
{
    background = std::make_unique<WavePlayerBackground>();
    background->setBounds (0, 0, 350, 200);
    addAndMakeVisible (background.get());

    patternSelector = std::make_unique<ComboBox>();
    patternSelector->setBounds (12, 40, 120, 20);
    patternSelector->addListener (this);
    patternSelector->setEditableText (true);
    addAndMakeVisible (patternSelector.get());

    triggerSelector = std::make_unique<ComboBox>();
    triggerSelector->setBounds (12, 100, 120, 20);
    triggerSelector->addListener (this);
    triggerSelector->setEnabled (false);
    triggerSelector->addItem ("NONE", 1);
    triggerSelector->setSelectedId (1);
    //addAndMakeVisible (triggerSelector.get());

    enableButton = std::make_unique<UtilityButton> ("DISABLED");
    enableButton->setBounds (120, 5, 70, 20);
    enableButton->addListener (this);
    addAndMakeVisible (enableButton.get());

    startStopButton = std::make_unique<UtilityButton> ("RUN");
    startStopButton->setBounds (42, 105, 60, 30);
    startStopButton->addListener (this);
    startStopButton->setEnabled (false);
    addAndMakeVisible (startStopButton.get());

    pulsePatternButton = std::make_unique<UtilityButton> ("Pulse");
    pulsePatternButton->setCorners (true, false, true, false);
    pulsePatternButton->setBounds (143, 40, 80, 20);
    pulsePatternButton->addListener (this);
    pulsePatternButton->setToggleState (true, false);
    addAndMakeVisible (pulsePatternButton.get());

    sinePatternButton = std::make_unique<UtilityButton> ("Sine");
    sinePatternButton->setCorners (false, true, false, true);
    sinePatternButton->setBounds (221, 40, 78, 20);
    sinePatternButton->addListener (this);
    addAndMakeVisible (sinePatternButton.get());

    customPatternButton = std::make_unique<UtilityButton> ("Custom");
    customPatternButton->setCorners (false, true, false, true);
    customPatternButton->setBounds (240, 40, 60, 20);
    customPatternButton->addListener (this);
    //addAndMakeVisible (customPatternButton.get());

    currentPattern = new Pattern();
    currentPattern->id = nextPatternId++;

    availablePatterns.add (currentPattern);

    updatePatternSelector();

    PulsePatternGenerator* ppg = new PulsePatternGenerator (this, currentPattern);
    ppg->buildWaveform();

    updateWaveform();

    delete ppg;
}

WavePlayer::~WavePlayer()
{
}

void WavePlayer::resized()
{
    background->setBounds (0, 0, getWidth(), getHeight());
}

void WavePlayer::updateAvailableTriggerChannels(Array <AdcChannelButton*> channels)
{
	triggerSelector->clear();
    triggerSelector->addItem ("NONE", 1);

    for (int i = 0; i < channels.size(); i++)
    {
        triggerSelector->addItem (channels[i]->getName(), channels[i]->getChannelIndex()+2);
	}
}

void WavePlayer::setTriggerChannel(int triggerChannel)
{
	currentPattern->triggerChannel = triggerChannel;

    triggerSelector->setSelectedId (triggerChannel + 2);
}

float WavePlayer::getSampleRate()
{
    // sample rate fixed at 30 kHz
    return 30000.0f;
}

void WavePlayer::updateWaveform()
{
    LOGD("Updating waveform for ", currentPattern->name);

    background->updateCurrentWaveform (currentPattern);

    //dac->setWaveform (currentPattern->samples);
}

void WavePlayer::updatePatternSelector()
{
    patternSelector->clear();

    LOGD ("Updating pattern selector.")

    for (auto pattern : availablePatterns)
    {
        LOGD("  Adding pattern: ", pattern->name);
        patternSelector->addItem (pattern->name, pattern->id);
    }

    patternSelector->addItem ("Add new pattern...", 9999);
    patternSelector->setSelectedId (currentPattern->id, dontSendNotification);
}

void WavePlayer::comboBoxChanged (ComboBox* comboBox)
{
    if (comboBox == patternSelector.get())
    {
        if (comboBox->getSelectedId() == 0)
        {
            currentPattern->name = patternSelector->getText();

            updatePatternSelector();
        }

        else if (comboBox->getSelectedId() == 9999) // add new pattern
        {
            currentPattern = new Pattern();
            currentPattern->id = nextPatternId++;
            currentPattern->name = "PATTERN " + String (currentPattern->id);
            availablePatterns.add (currentPattern);
            PulsePatternGenerator* ppg = new PulsePatternGenerator (this, currentPattern);
            ppg->buildWaveform();
            delete ppg;

            updatePatternSelector();
            selectPatternType (currentPattern->patternType);
            updateWaveform();

            for (int i = 0; i < 16; i++)
            {
                bool triggerTaken = false;

                for (auto pattern : availablePatterns)
                {
                    if (pattern->triggerChannel == i)
                    {
                        triggerTaken = true;
                    }
                }

                if (! triggerTaken)
                {
                    currentPattern->triggerChannel = i;
                    break;
                }
            }
        }
        else
        {
            for (auto pattern : availablePatterns)
            {
                if (pattern->id == comboBox->getSelectedId())
                    currentPattern = pattern;
            }

            initializePattern (currentPattern);
        }
    }
    else if (comboBox == triggerSelector.get())
    {
        currentPattern->triggerChannel = triggerSelector->getSelectedId() - 2;
        ui->setTriggerChannel (currentPattern->triggerChannel);
    }
}

void WavePlayer::selectPatternType (PatternType t)
{
    LOGD ("Selecting pattern type: ", (int) t);

    if (t == PatternType::pulse)
    {
        pulsePatternButton->setToggleState (true, false);
        sinePatternButton->setToggleState (false, false);
        customPatternButton->setToggleState (false, false);
    }
    else if (t == PatternType::sine)
    {
        pulsePatternButton->setToggleState (false, false);
        sinePatternButton->setToggleState (true, false);
        customPatternButton->setToggleState (false, false);
    }
    else if (t == PatternType::custom)
    {
        pulsePatternButton->setToggleState (false, false);
        sinePatternButton->setToggleState (false, false);
        customPatternButton->setToggleState (true, false);
    }
}

void WavePlayer::initializePattern (Pattern* pattern)
{
    currentPattern = pattern;

    LOGD ("Initializing pattern.");

    if (pattern->patternType == PatternType::pulse)
    {
        auto* patternGenerator = new PulsePatternGenerator (this, pattern);

        patternGenerator->buildWaveform();

        LOGD ("Creating pulse wave.");

        delete patternGenerator;
    }
    else if (pattern->patternType == PatternType::sine)
    {
        auto* patternGenerator = new SinePatternGenerator (this, pattern);

        patternGenerator->buildWaveform();

        LOGD ("Creating sine wave.");

        delete patternGenerator;
    }
    else if (pattern->patternType == PatternType::sine)
    {
        auto* patternGenerator = new CustomPatternGenerator (this, pattern);

        patternGenerator->buildWaveform();

        LOGD ("Creating custom wave.");

        delete patternGenerator;
    }

    updatePatternSelector();
    selectPatternType (pattern->patternType);

    updateWaveform();
}

void WavePlayer::buttonClicked (Button* button)
{
    if (button == enableButton.get())
    {
        if (enableButton->getToggleState())
        {
            enableButton->setToggleState (false, false);
            enableButton->setLabel ("DISABLED");
            ui->setAsAdc(0);
            startStopButton->setEnabled (false);
            triggerSelector->setEnabled (false);
        }
        else
        {
            enableButton->setToggleState (true, false);
            enableButton->setLabel ("ENABLED");
            startStopButton->setEnabled (true);
            triggerSelector->setEnabled (true);

            ui->setAsDac (0);
        }
    }
    else if (button == pulsePatternButton.get())
    {
        auto* patternGenerator = new PulsePatternGenerator (this, currentPattern);

        patternGenerator->buildWaveform();

        updateWaveform();

        selectPatternType (currentPattern->patternType);

        CallOutBox& myBox = CallOutBox::launchAsynchronously (std::unique_ptr<Component> (patternGenerator),
                                                              button->getScreenBounds(),
                                                              nullptr);
    }
    else if (button == sinePatternButton.get())
    {
        auto* patternGenerator = new SinePatternGenerator (this, currentPattern);

        patternGenerator->buildWaveform();

        updateWaveform();

        selectPatternType (currentPattern->patternType);

        CallOutBox& myBox = CallOutBox::launchAsynchronously (std::unique_ptr<Component> (patternGenerator),
                                                              button->getScreenBounds(),
                                                              nullptr);
    }
    else if (button == customPatternButton.get())
    {
        auto* patternGenerator = new CustomPatternGenerator (this, currentPattern);

        patternGenerator->buildWaveform();

        selectPatternType (currentPattern->patternType);

        updateWaveform();

        CallOutBox& myBox = CallOutBox::launchAsynchronously (std::unique_ptr<Component> (patternGenerator),
                                                              button->getScreenBounds(),
                                                              nullptr);
    }
    else if (button == startStopButton.get())
    {
        dac->playWaveform();
        startStopButton->setToggleState (true, false);
        startTimer(currentPattern->samples.size() / getSampleRate() * 1000);
    }
}

void WavePlayer::timerCallback()
{
    startStopButton->setToggleState (false, false);
}

void WavePlayer::saveCustomParameters (XmlElement* xml)
{

    XmlElement* xmlNode = xml->createNewChildElement ("WAVEPLAYER");
    xmlNode->setAttribute ("enabled", enableButton->getToggleState());

    for (auto pattern : availablePatterns)
    {
        XmlElement* xmlNode = xml->createNewChildElement ("PATTERN");

        xmlNode->setAttribute ("id", pattern->id);
        xmlNode->setAttribute ("name", pattern->name);
        xmlNode->setAttribute ("analog_output_channel", pattern->analogOutputChannel);
        xmlNode->setAttribute ("trigger_channel", pattern->triggerChannel);
        xmlNode->setAttribute ("gate_channel", pattern->gateChannel);
        xmlNode->setAttribute ("pulse_on_duration", pattern->pulse.onDuration);
        xmlNode->setAttribute ("pulse_off_duration", pattern->pulse.offDuration);
        xmlNode->setAttribute ("pulse_delay_duration", pattern->pulse.delayDuration);
        xmlNode->setAttribute ("pulse_repeat_number", pattern->pulse.repeatNumber);

        xmlNode->setAttribute ("pulse_ramp_on_duration", pattern->pulse.rampOnDuration);
        xmlNode->setAttribute ("pulse_ramp_off_duration", pattern->pulse.rampOffDuration);
        xmlNode->setAttribute ("pulse_max_voltage", pattern->pulse.maxVoltage);

        xmlNode->setAttribute ("sine_frequency", pattern->sine.frequency);
        xmlNode->setAttribute ("sine_cycles", pattern->sine.cycles);
        xmlNode->setAttribute ("sine_delay_duration", pattern->sine.delayDuration);
        xmlNode->setAttribute ("sine_max_voltage", pattern->sine.maxVoltage);

        xmlNode->setAttribute ("custom_value_string", pattern->custom.string);

        switch (pattern->patternType)
        {
            case PatternType::pulse:
                xmlNode->setAttribute ("pattern_type", 0);
                break;
            case PatternType::sine:
                xmlNode->setAttribute ("pattern_type", 1);
                break;
            case PatternType::custom:
                xmlNode->setAttribute ("pattern_type", 2);
                break;
            default:
                xmlNode->setAttribute ("pattern_type", 0);
        }

        xmlNode->setAttribute ("is_current_pattern", pattern == currentPattern);
    }
}

void WavePlayer::loadCustomParameters (XmlElement* xml)
{
    availablePatterns.clear();

    bool foundPattern = false;

    forEachXmlChildElement (*xml, xmlNode)
    {
        if (xmlNode->hasTagName("WAVEPLAYER"))
        {
            if (xmlNode->getBoolAttribute ("enabled", false))
                buttonClicked (enableButton.get());
        }
            

        if (xmlNode->hasTagName ("PATTERN"))
        {
            foundPattern = true;
            Pattern* newPattern = new Pattern();

            newPattern->id = xmlNode->getIntAttribute ("id");
            newPattern->name = xmlNode->getStringAttribute ("name");

            LOGD ("Loading pattern ", newPattern->name);
            newPattern->analogOutputChannel = xmlNode->getIntAttribute ("analog_output_channel", 0);
            newPattern->triggerChannel = xmlNode->getIntAttribute ("trigger_channel", 0);
            newPattern->gateChannel = xmlNode->getIntAttribute ("gate_channel", -1);

            newPattern->pulse.onDuration = xmlNode->getIntAttribute ("pulse_on_duration", 100);
            newPattern->pulse.offDuration = xmlNode->getIntAttribute ("pulse_off_duration", 100);
            newPattern->pulse.delayDuration = xmlNode->getIntAttribute ("pulse_delay_duration", 0);
            newPattern->pulse.repeatNumber = xmlNode->getIntAttribute ("pulse_repeat_number", 1);
            newPattern->pulse.rampOnDuration = xmlNode->getIntAttribute ("pulse_ramp_on_duration", 0);
            newPattern->pulse.rampOffDuration = xmlNode->getIntAttribute ("pulse_ramp_off_duration", 0);
            newPattern->pulse.maxVoltage = xmlNode->getDoubleAttribute ("pulse_max_voltage", 5.0);

            newPattern->sine.frequency = xmlNode->getIntAttribute ("sine_frequency", 5);
            newPattern->sine.cycles = xmlNode->getIntAttribute ("sine_cycles", 1);
            newPattern->sine.delayDuration = xmlNode->getIntAttribute ("sine_delay_duration", 0);
            newPattern->sine.maxVoltage = xmlNode->getDoubleAttribute ("sine_max_voltage", 5.0);

            newPattern->custom.string = xmlNode->getStringAttribute ("custom_value_string", "0,0,0");

            int patternTypeId = xmlNode->getIntAttribute ("pattern_type", 0);

            LOGD ("Pattern type: ", patternTypeId);

            switch (patternTypeId)
            {
                case 0:
                    newPattern->patternType = PatternType::pulse;
                    break;
                case 1:
                    newPattern->patternType = PatternType::sine;
                    break;
                case 2:
                    newPattern->patternType = PatternType::custom;
                    break;
                default:
                    newPattern->patternType = PatternType::pulse;
            }

            if (xmlNode->getBoolAttribute ("is_current_pattern"))
            {
                currentPattern = newPattern;
                LOGD ("Setting as current pattern");
            }

            availablePatterns.add (newPattern);

            if (newPattern->id > nextPatternId)
            {
                nextPatternId = newPattern->id + 1;
            }
        }
    }

    if (foundPattern)
        initializePattern (currentPattern);
}
