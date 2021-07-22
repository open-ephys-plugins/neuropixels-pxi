/*
------------------------------------------------------------------

This file is part of the Open Ephys GUI
Copyright (C) 2019 Allen Institute for Brain Science and Open Ephys

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

#include "../Probes/OneBoxDAC.h"


WavePlayerBackground::WavePlayerBackground() {

	currentWaveform = Path();
}

void WavePlayerBackground::updateCurrentWaveform(Pattern* pattern)
{
	float waveformWidth = 80;

	float numSamples = (float) pattern->samples.size();

	float blockSize = numSamples / waveformWidth;

	currentWaveform.clear();

	currentWaveform.startNewSubPath(0, 1);

	float lastSample = pattern->samples[0];
	float lastIndex = 0;

	currentWaveform.lineTo(0, 1 - lastSample / pattern->maxVoltage);

	for (int i = 0; i < pattern->samples.size(); i++)
	{

		if (pattern->samples[i] != lastSample)
		{
			currentWaveform.lineTo(float(i) / numSamples, 1 - lastSample / pattern->maxVoltage);
			currentWaveform.lineTo(float(i) / numSamples, 1 - pattern->samples[i] / pattern->maxVoltage);
			lastSample = pattern->samples[i];
			lastIndex = i;
		}
	}

	pathTransform = currentWaveform.getTransformToScaleToFit(140, 80, 160, 98, false);

	repaint();

}

void WavePlayerBackground::paint(Graphics& g)
{
	g.setColour(Colours::lightgrey);
	g.drawRoundedRectangle(0, 0, getWidth(), getHeight(), 3.0, 2.0);

	g.setFont(20);
	g.drawText("WavePlayer", 7, 5, 150, 20, Justification::left);

	g.setColour(Colours::orange);
	g.strokePath(currentWaveform, PathStrokeType(1.0), pathTransform);

}

WavePlayer::WavePlayer(OneBoxDAC* dac_)
	: dac(dac_), nextPatternId(1)
{

	background = new WavePlayerBackground();
	background->setBounds(0, 0, 350, 200);
	addAndMakeVisible(background);

	patternSelector = new ComboBox();
	patternSelector->setBounds(12, 40, 120, 20);
	patternSelector->addListener(this);
	patternSelector->setEditableText(true);
	addAndMakeVisible(patternSelector);

	startStopButton = new UtilityButton("RUN", Font("Small Text", 15, Font::plain));
	startStopButton->setBounds(30, 85, 55, 30);
	startStopButton->addListener(this);
	addAndMakeVisible(startStopButton);

	pulsePatternButton = new UtilityButton("Pulse", Font("Small Text", 10, Font::plain));
	pulsePatternButton->setCorners(true, false, true, false);
	pulsePatternButton->setBounds(140, 40, 50, 20);
	pulsePatternButton->addListener(this);
	pulsePatternButton->setToggleState(true, false);
	addAndMakeVisible(pulsePatternButton);

	sinePatternButton = new UtilityButton("Sine", Font("Small Text", 10, Font::plain));
	sinePatternButton->setCorners(false, false, false, false);
	sinePatternButton->setBounds(190, 40, 50, 20);
	sinePatternButton->addListener(this);
	addAndMakeVisible(sinePatternButton);

	customPatternButton = new UtilityButton("Custom", Font("Small Text", 10, Font::plain));
	customPatternButton->setCorners(false, true, false, true);
	customPatternButton->setBounds(240, 40, 60, 20);
	customPatternButton->addListener(this);
	addAndMakeVisible(customPatternButton);

	currentPattern = new Pattern();
	currentPattern->id = nextPatternId++;

	availablePatterns.add(currentPattern);

	updatePatternSelector();

	PulsePatternGenerator* ppg = new PulsePatternGenerator(this, currentPattern);
	ppg->buildWaveform();
	
	background->updateCurrentWaveform(currentPattern);

	delete ppg;
}

WavePlayer::~WavePlayer()
{

		

}

void WavePlayer::resized()
{
	background->setBounds(0, 0, getWidth(), getHeight());
}

void WavePlayer::timerCallback()
{

}

float WavePlayer::getSampleRate()
{
	return 30000.0f;
}

void WavePlayer::updateWaveform()
{
	background->updateCurrentWaveform(currentPattern);

	dac->setWaveform(currentPattern->samples);
}

void WavePlayer::updatePatternSelector()
{
	patternSelector->clear();

	for (auto pattern : availablePatterns)
	{
		patternSelector->addItem(pattern->name, pattern->id);
	}

	patternSelector->addItem("Add new pattern...", 9999);
	patternSelector->setSelectedId(currentPattern->id, dontSendNotification);

}


void WavePlayer::comboBoxChanged(ComboBox* comboBox)
{
	if (comboBox == patternSelector)
	{
		std::cout << "COMBO BOX CHANGED" << std::endl;

		std::cout << "Selected ID: " << comboBox->getSelectedId() << std::endl;

		if (comboBox->getSelectedId() == 0)
		{
			currentPattern->name = patternSelector->getText();

			updatePatternSelector();
		}

		else if (comboBox->getSelectedId() == 9999) // add new pattern
		{
			currentPattern = new Pattern();
			currentPattern->id = nextPatternId++;
			currentPattern->name = "PATTERN " + String(currentPattern->id);
			availablePatterns.add(currentPattern);
			PulsePatternGenerator* ppg = new PulsePatternGenerator(this, currentPattern);
			ppg->buildWaveform();
			delete ppg;

			updatePatternSelector();
			selectPatternType(currentPattern->patternType);
			background->updateCurrentWaveform(currentPattern);

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

				if (!triggerTaken)
				{
					currentPattern->triggerChannel = i;
					break;
				}
			}

		}
		else {

			for (auto pattern : availablePatterns)
			{
				if (pattern->id == comboBox->getSelectedId())
					currentPattern = pattern;
			}

			initializePattern(currentPattern);

		}
	}

}

void WavePlayer::selectPatternType(PatternType t)
{
	if (t == PatternType::pulse)
	{
		pulsePatternButton->setToggleState(true, false);
		sinePatternButton->setToggleState(false, false);
		customPatternButton->setToggleState(false, false);
	}
	else if (t == PatternType::sine)
	{
		pulsePatternButton->setToggleState(false, false);
		sinePatternButton->setToggleState(true, false);
		customPatternButton->setToggleState(false, false);
	}
	else if (t == PatternType::custom)
	{
		pulsePatternButton->setToggleState(false, false);
		sinePatternButton->setToggleState(false, false);
		customPatternButton->setToggleState(true, false);
	}
}

void WavePlayer::initializePattern(Pattern* pattern)
{

	currentPattern = pattern;

	if (pattern->patternType == PatternType::pulse)
	{
		auto* patternGenerator = new PulsePatternGenerator(this, pattern);

		patternGenerator->buildWaveform();

		delete patternGenerator;

	}
	else if (pattern->patternType == PatternType::sine)
	{
		auto* patternGenerator = new SinePatternGenerator(this, pattern);

		patternGenerator->buildWaveform();

		delete patternGenerator;

	}
	else if (pattern->patternType == PatternType::sine)
	{
		auto* patternGenerator = new CustomPatternGenerator(this, pattern);

		patternGenerator->buildWaveform();

		delete patternGenerator;
	}

	updatePatternSelector();
	selectPatternType(pattern->patternType);

	updateWaveform();

}


void WavePlayer::buttonClicked(Button* button)
{

	if (button == pulsePatternButton)
	{
		auto* patternGenerator = new PulsePatternGenerator(this, currentPattern);

		patternGenerator->buildWaveform();

		background->updateCurrentWaveform(currentPattern);

		selectPatternType(currentPattern->patternType);

		CallOutBox& myBox
			= CallOutBox::launchAsynchronously(std::unique_ptr<Component>(patternGenerator),
				button->getScreenBounds(), 
				nullptr);

	}
	else if (button == sinePatternButton)
	{
		auto* patternGenerator = new SinePatternGenerator(this, currentPattern);

		patternGenerator->buildWaveform();

		background->updateCurrentWaveform(currentPattern);

		selectPatternType(currentPattern->patternType);

		CallOutBox& myBox
			= CallOutBox::launchAsynchronously(std::unique_ptr<Component>(patternGenerator),
				button->getScreenBounds(), 
				nullptr);

	}
	else if (button == customPatternButton)
	{
		auto* patternGenerator = new CustomPatternGenerator(this, currentPattern);

		patternGenerator->buildWaveform();

		selectPatternType(currentPattern->patternType);

		CallOutBox& myBox
			= CallOutBox::launchAsynchronously(std::unique_ptr<Component>(patternGenerator), 
				button->getScreenBounds(), 
				nullptr);

	}
	else if (button == startStopButton)
	{
		dac->playWaveform();
		//startTimer(currentPattern->samples.size() / getSampleRate() * 1000);
	}

}


void WavePlayer::saveCustomParameters(XmlElement* xml)
{
	//xml->setAttribute("productName", processor->getProductName());

	for (auto pattern : availablePatterns)
	{
		XmlElement* xmlNode = xml->createNewChildElement("PATTERN");

		xmlNode->setAttribute("id", pattern->id);
		xmlNode->setAttribute("name", pattern->name);
		xmlNode->setAttribute("analog_output_channel", pattern->analogOutputChannel);
		xmlNode->setAttribute("trigger_channel", pattern->triggerChannel);
		xmlNode->setAttribute("gate_channel", pattern->gateChannel);
		xmlNode->setAttribute("pulse_on_duration", pattern->pulse.onDuration);
		xmlNode->setAttribute("pulse_off_duration", pattern->pulse.offDuration);
		xmlNode->setAttribute("pulse_delay_duration", pattern->pulse.delayDuration);
		xmlNode->setAttribute("pulse_repeat_number", pattern->pulse.repeatNumber);

		xmlNode->setAttribute("pulse_ramp_on_duration", pattern->pulse.rampOnDuration);
		xmlNode->setAttribute("pulse_ramp_off_duration", pattern->pulse.rampOffDuration);
		xmlNode->setAttribute("pulse_max_voltage", pattern->pulse.maxVoltage);

		xmlNode->setAttribute("sine_frequency", pattern->sine.frequency);
		xmlNode->setAttribute("sine_cycles", pattern->sine.cycles);
		xmlNode->setAttribute("sine_delay_duration", pattern->sine.delayDuration);
		xmlNode->setAttribute("sine_max_voltage", pattern->sine.maxVoltage);

		xmlNode->setAttribute("custom_value_string", pattern->custom.string);

		switch (pattern->patternType)
		{
		case PatternType::pulse:
			xmlNode->setAttribute("pattern_type", 0);
			break;
		case PatternType::sine:
			xmlNode->setAttribute("pattern_type", 1);
			break;
		case PatternType::custom:
			xmlNode->setAttribute("pattern_type", 2);
			break;
		default:
			xmlNode->setAttribute("pattern_type", 0);
		}

		xmlNode->setAttribute("is_current_pattern", pattern == currentPattern);

	}
}


void WavePlayer::loadCustomParameters(XmlElement* xml)
{
	//String productName = xml->getStringAttribute("productName", "NIDAQmx");
	//if (!thread->swapConnection(productName));
	//draw();

	availablePatterns.clear();

	bool foundPattern = false;

	forEachXmlChildElement(*xml, xmlNode)
	{
		if (xmlNode->hasTagName("PATTERN"))
		{
			foundPattern = true;

			Pattern* newPattern = new Pattern();

			newPattern->id = xmlNode->getIntAttribute("id");
			newPattern->name = xmlNode->getStringAttribute("name");
			newPattern->analogOutputChannel = xmlNode->getIntAttribute("analog_output_channel", 0);
			newPattern->triggerChannel = xmlNode->getIntAttribute("trigger_channel", 0);
			newPattern->gateChannel = xmlNode->getIntAttribute("gate_channel", -1);

			newPattern->pulse.onDuration = xmlNode->getIntAttribute("pulse_on_duration", 100);
			newPattern->pulse.offDuration = xmlNode->getIntAttribute("pulse_off_duration", 100);
			newPattern->pulse.delayDuration = xmlNode->getIntAttribute("pulse_delay_duration", 0);
			newPattern->pulse.repeatNumber = xmlNode->getIntAttribute("pulse_repeat_number", 1);
			newPattern->pulse.rampOnDuration = xmlNode->getIntAttribute("pulse_ramp_on_duration", 0);
			newPattern->pulse.rampOffDuration = xmlNode->getIntAttribute("pulse_ramp_off_duration", 0);
			newPattern->pulse.maxVoltage = xmlNode->getDoubleAttribute("pulse_max_voltage", 5.0);

			newPattern->sine.frequency = xmlNode->getIntAttribute("sine_frequency", 5);
			newPattern->sine.cycles = xmlNode->getIntAttribute("sine_cycles", 100);
			newPattern->sine.delayDuration = xmlNode->getIntAttribute("sine_delay_duration", 0);
			newPattern->sine.maxVoltage = xmlNode->getDoubleAttribute("sine_max_voltage", 5.0);

			newPattern->custom.string = xmlNode->getStringAttribute("custom_value_string", "0,0,0");

			int patternTypeId = xmlNode->getIntAttribute("pattern_type", 0);

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

			if (xmlNode->getBoolAttribute("is_current_pattern"))
			{
				currentPattern = newPattern;
			}
				
			availablePatterns.add(newPattern);

			if (newPattern->id > nextPatternId)
			{
				nextPatternId = newPattern->id + 1;
			}
		}
	}

	if (foundPattern)
		initializePattern(currentPattern);
}
