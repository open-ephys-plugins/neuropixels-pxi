
/*
------------------------------------------------------------------

This file is part of the Open Ephys GUI
Copyright (C) 2021 Allen Institute for Brain Science and Open Ephys

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

#include <type_traits>

#include "AnalogPatternGenerator.h"
#include "WavePlayer.h"

#define MAX_SAMPLES 10000
#define PI 3.14159265

AnalogPatternGenerator::AnalogPatternGenerator(WavePlayer* wv_, Pattern* pattern_) : wv(wv_), pattern(pattern_)
{
	editable = true;


}

AnalogPatternGenerator::~AnalogPatternGenerator()
{

}

template<typename T>
EditableTextInput<T>::EditableTextInput(String mainlabelText,
	String unitsLabelText,
	T minValue_,
	T maxValue_,
	T defaultValue,
	AnalogPatternGenerator* apg_
) : minValue(minValue_), maxValue(maxValue_), apg(apg_)
{
	mainLabel = new Label("Main Label", mainlabelText);
	mainLabel->setFont(Font("Small Text", 12, Font::plain));
	mainLabel->setBounds(0, 0, 100, 20);
	mainLabel->setColour(Label::textColourId, Colours::white);
	mainLabel->setJustificationType(Justification::centredRight);
	addAndMakeVisible(mainLabel);

	unitsLabel = new Label("Units Label", unitsLabelText);
	unitsLabel->setFont(Font("Small Text", 12, Font::plain));
	unitsLabel->setBounds(150, 0, 30, 20);
	unitsLabel->setColour(Label::textColourId, Colours::white);
	addAndMakeVisible(unitsLabel);

	inputBox = new Label("Input Box", String(defaultValue));
	inputBox->setFont(Font("Small Text", 12, Font::plain));
	inputBox->setBounds(100, 0, 50, 20);
	inputBox->setColour(Label::backgroundColourId, Colours::lightgrey);
	inputBox->setColour(Label::textColourId, Colours::darkgrey);
	inputBox->addListener(this);
	inputBox->setEditable(true);
	addAndMakeVisible(inputBox);

	lastValue = defaultValue;
}

template<typename T>
EditableTextInput<T>::~EditableTextInput()
{

}

void EditableTextInput<int>::labelTextChanged(Label* label)
{
	String labelString = label->getText();

	if (!labelString.containsOnly("0123456789.- "))
	{
		inputBox->setText(String(lastValue), dontSendNotification);
		return;
	}

	int value = labelString.getIntValue();

	if (value < minValue)
	{
		inputBox->setText(String(minValue), dontSendNotification);
		return;
	}
	else if (value > maxValue)
	{
		inputBox->setText(String(maxValue), dontSendNotification);
		return;
	}
	else {

		inputBox->setText(String(value), dontSendNotification);
		lastValue = value;
	}

	apg->updatePattern();
}

void EditableTextInput<float>::labelTextChanged(Label* label)
{
	String labelString = label->getText();

	if (!labelString.containsOnly("0123456789.- "))
	{
		inputBox->setText(String(lastValue), dontSendNotification);
		return;
	}

	
	float value = labelString.getFloatValue();

	if (value < minValue)
	{
		inputBox->setText(String(minValue), dontSendNotification);
		return;
	}
	else if (value > maxValue)
	{
		inputBox->setText(String(maxValue), dontSendNotification);
		return;
	}
	else {

		lastValue = value;
	}


	apg->updatePattern();
}

float EditableTextInput<float>::getValue()
{
	return inputBox->getText().getFloatValue();
}

int EditableTextInput<int>::getValue()
{
	return inputBox->getText().getIntValue();

}

template<typename T>
void EditableTextInput<T>::setValue(T value)
{
	inputBox->setText(String(value), dontSendNotification);
}


PulsePatternGenerator::PulsePatternGenerator(WavePlayer* wv_, Pattern* pattern_) : AnalogPatternGenerator(wv_, pattern_)
{
	setSize(190, 190);

	onDuration = new EditableTextInput<int>("On duration:",
		"ms",
		0,
		10000,
		100,
		this);

	onDuration->setBounds(10, 10, 180, 20);
	addAndMakeVisible(onDuration);

	offDuration = new EditableTextInput<int>("Off duration:",
		"ms",
		0,
		10000,
		100,
		this);

	offDuration->setBounds(10, 35, 180, 20);
	addAndMakeVisible(offDuration);

	delayDuration = new EditableTextInput<int>("Delay:",
		"ms",
		0,
		10000,
		100,
		this);

	delayDuration->setBounds(10, 60, 180, 20);
	addAndMakeVisible(delayDuration);

	repeatNumber = new EditableTextInput<int>("Num repeats:",
		"x",
		0,
		100,
		1,
		this);

	repeatNumber->setBounds(10, 85, 180, 20);
	addAndMakeVisible(repeatNumber);

	rampOnDuration = new EditableTextInput<int>("Ramp on:",
		"ms",
		0,
		100,
		0,
		this);

	rampOnDuration->setBounds(10, 110, 180, 20);
	addAndMakeVisible(rampOnDuration);

	rampOffDuration = new EditableTextInput<int>("Ramp off:",
		"ms",
		0,
		100,
		0,
		this);

	rampOffDuration->setBounds(10, 135, 180, 20);
	addAndMakeVisible(rampOffDuration);

	maxVoltage = new EditableTextInput<float>("Max voltage:",
		"V",
		0.0f,
		5.0f,
		5.0f,
		this);

	maxVoltage->setBounds(10, 160, 180, 20);
	addAndMakeVisible(maxVoltage);

	setState(pattern);

}

PulsePatternGenerator::~PulsePatternGenerator()
{

}

void PulsePatternGenerator::updatePattern()
{
	pattern->pulse.onDuration = onDuration->getValue();
	pattern->pulse.offDuration = offDuration->getValue();

	pattern->pulse.rampOnDuration = rampOnDuration->getValue();
	pattern->pulse.rampOffDuration = rampOffDuration->getValue();

	pattern->pulse.repeatNumber = repeatNumber->getValue();

	pattern->pulse.delayDuration = delayDuration->getValue();
	pattern->pulse.maxVoltage = maxVoltage->getValue();

	pattern->patternType = PatternType::pulse;

	buildWaveform();

	wv->updateWaveform();
}

void PulsePatternGenerator::buildWaveform()
{

	pattern->patternType = PatternType::pulse;
	pattern->maxVoltage = pattern->pulse.maxVoltage;
	pattern->samples.clear();

	float sampleRate = wv->getSampleRate();
	pattern->sampleRate = sampleRate;

	float numDelaySamples = sampleRate * float(pattern->pulse.delayDuration) / 1000;
	float numOnRampSamples = sampleRate * float(pattern->pulse.rampOnDuration) / 1000;
	float numOffRampSamples = sampleRate * float(pattern->pulse.rampOffDuration) / 1000;
	float numOnSamples = sampleRate * float(pattern->pulse.onDuration) / 1000;
	float numOffSamples = sampleRate * float(pattern->pulse.offDuration) / 1000;

	Array<float> samples;

	float rampOffStart = numOnSamples - numOffRampSamples;
	float maxVoltage = pattern->pulse.maxVoltage;

	for (float i = 0; i < numOnSamples; i++)
	{
		if (i < numOnRampSamples)
		{
			samples.add(i / numOnRampSamples * maxVoltage);
		}
		else if (i > rampOffStart)
		{
			samples.add((1 - (i - rampOffStart) / numOffRampSamples) * maxVoltage);
		}
		else {
			samples.add(maxVoltage);
		}
	}

	for (float i = 0; i < numOffSamples; i++)
	{
		samples.add(0);
	}

	samples.add(0);

	for (float i = 0; i < numDelaySamples; i++)
		pattern->samples.add(0);

	for (int r = 0; r < pattern->pulse.repeatNumber; r++)
	{
		for (int i = 0; i < samples.size(); i++)
		{
			pattern->samples.add(samples[i]);
		}
	}
				
}

void PulsePatternGenerator::setState(Pattern* pattern)
{
	onDuration->setValue(pattern->pulse.onDuration);
	offDuration->setValue(pattern->pulse.offDuration);
	rampOnDuration->setValue(pattern->pulse.rampOnDuration);
	rampOffDuration->setValue(pattern->pulse.rampOffDuration);
	repeatNumber->setValue(pattern->pulse.repeatNumber);

	delayDuration->setValue(pattern->pulse.delayDuration);
	maxVoltage->setValue(pattern->pulse.maxVoltage);
}

SinePatternGenerator::SinePatternGenerator(WavePlayer* wv_, Pattern* pattern_) : AnalogPatternGenerator(wv_, pattern_)
{
	setSize(190, 120);

	frequency = new EditableTextInput<int>("Frequency:",
		"Hz",
		1,
		1000,
		5,
		this);

	frequency->setBounds(10, 10, 180, 20);
	addAndMakeVisible(frequency);

	cycles = new EditableTextInput<int>("Num cycles:",
		"",
		1,
		10000,
		5,
		this);

	cycles->setBounds(10, 35, 180, 20);
	addAndMakeVisible(cycles);

	delay = new EditableTextInput<int>("Delay:",
		"ms",
		0,
		10000,
		100,
		this);

	delay->setBounds(10, 60, 180, 20);
	addAndMakeVisible(delay);

	maxVoltage = new EditableTextInput<float>("Max voltage:",
		"V",
		0.0f,
		5.0f,
		5.0f,
		this);

	maxVoltage->setBounds(10, 85, 180, 20);
	addAndMakeVisible(maxVoltage);

	setState(pattern);

}

SinePatternGenerator::~SinePatternGenerator()
{

}

void SinePatternGenerator::updatePattern()
{
	pattern->sine.frequency = frequency->getValue();
	pattern->sine.cycles = cycles->getValue();
	pattern->sine.delayDuration = delay->getValue();
	pattern->sine.maxVoltage = maxVoltage->getValue();

	pattern->patternType = PatternType::sine;

	buildWaveform();

	wv->updateWaveform();
		
}

void SinePatternGenerator::setState(Pattern* pattern)
{

	frequency->setValue(pattern->sine.frequency);
	delay->setValue(pattern->sine.delayDuration);
	maxVoltage->setValue(pattern->sine.maxVoltage);
	cycles->setValue(pattern->sine.cycles);
}


void SinePatternGenerator::buildWaveform()
{

	pattern->maxVoltage = pattern->sine.maxVoltage;
	pattern->samples.clear();

	float sampleRate = wv->getSampleRate();
	pattern->sampleRate = sampleRate;

	float maxVoltage = pattern->maxVoltage;

	float numDelaySamples = sampleRate * float(pattern->sine.delayDuration) / 1000;

	float totalTime = float(pattern->sine.cycles) / float(pattern->sine.frequency);

	Array<float> times;

	for (int i = 0; i < int(sampleRate * totalTime); i++)
		times.add(float(i) / sampleRate);

	for (float i = 0; i < numDelaySamples; i++)
		pattern->samples.add(0);

	for (int i = 0; i < times.size(); i++) // range(len(samples)) :
		pattern->samples.add((2 - (cos(2 * PI * float(pattern->sine.frequency) * times[i]) + 1)) / 2 * maxVoltage);

	pattern->samples.add(0);

	pattern->patternType = PatternType::sine;

}

CustomPatternGenerator::CustomPatternGenerator(WavePlayer* wv_, Pattern* pattern_) : AnalogPatternGenerator(wv_, pattern_)
{
	setSize(280, 250);

	TextEditor::LengthAndCharacterRestriction* inputFilter = 
		new TextEditor::LengthAndCharacterRestriction(-1, "-0123456789,. ");

	mainLabel = new Label("Main Label", "Enter voltage values separated by commas:");
	mainLabel->setBounds(10, 2, 260, 20);
	mainLabel->setColour(Label::textColourId, Colours::grey);
	mainLabel->setJustificationType(Justification::centredLeft);
	addAndMakeVisible(mainLabel);

	textEditor = new TextEditor();
	textEditor->setBounds(10, 25, 260, 215);
	textEditor->addListener(this);
	textEditor->setMultiLine(true);
	textEditor->setColour(TextEditor::ColourIds::textColourId, Colours::white);
	textEditor->setColour(TextEditor::ColourIds::backgroundColourId, Colours::darkgrey);
	textEditor->setInputFilter(inputFilter, true);
	addAndMakeVisible(textEditor);

	setState(pattern);
}

/*void CustomPatternGenerator::keyPressed(KeyPress& key)
{
	/*if (key.getTextCharacter() == 'c' && key.getModifiers().ctrlModifier)
	{
		textEditor->copyToClipboard();
	}
	else if (key.getTextCharacter() == 'p' && key.getModifiers().ctrlModifier)
	{
		textEditor->pasteFromClipboard();
*/


CustomPatternGenerator::~CustomPatternGenerator()
{

}

void CustomPatternGenerator::textEditorReturnKeyPressed(TextEditor& editor)
{
	updatePattern();
}

void CustomPatternGenerator::updatePattern()
{
	String text = textEditor->getText();

	int startLoc = 0;
	int commaLoc = text.indexOf(",");
	int endLoc = commaLoc;

	pattern->custom.samples.clear();

	for (int i = 0; i < MAX_SAMPLES; i++)
	{
		//std::cout << "Start: " << startLoc << ", end: " << endLoc << ", comma: " << commaLoc << std::endl;

		String substring = text.substring(startLoc, endLoc);

		if (substring.length() > 0)
			pattern->custom.samples.add(constrainVoltage(substring.getFloatValue()));

		//std::cout << "Value: " << substring.getFloatValue() << std::endl;

		startLoc = endLoc + 1;
		commaLoc = text.substring(startLoc).indexOf(",");
		endLoc = startLoc + commaLoc;

		if (commaLoc < 0)
		{
			endLoc = text.length();
			//std::cout << "Start: " << startLoc << ", end: " << endLoc << ", comma: " << commaLoc << std::endl;
			String substring = text.substring(startLoc, endLoc);

			if (substring.length() > 0)
				pattern->custom.samples.add(constrainVoltage(substring.getFloatValue()));

			//std::cout << "Value: " << substring.getFloatValue() << std::endl;
			break;
		}
				

		//std::cout << "Start: " << startLoc << ", end: " << endLoc << ", comma: " << commaLoc << std::endl;
	}

	maxVoltage = 0;

	for (int i = 0; i < pattern->custom.samples.size(); i++)
	{
		if (pattern->custom.samples[i] > maxVoltage)
			maxVoltage = pattern->custom.samples[i];
	}

	setState(pattern);
}

float CustomPatternGenerator::constrainVoltage(float input, float min, float max)
{
	if (input < min)
		return min;
	else if (input > max)
		return max;
	else
		return input;
}

void CustomPatternGenerator::setState(Pattern* pattern)
{
	String text = "";

	for (int i = 0; i < pattern->custom.samples.size(); i++)
	{
		text += String(pattern->custom.samples[i]);
		text += ",";
	}

	text = text.upToLastOccurrenceOf(",", false, true);

	textEditor->setText(text);
}

void CustomPatternGenerator::buildWaveform()
{

	pattern->maxVoltage = maxVoltage;
	pattern->samples = pattern->custom.samples;
	pattern->patternType = PatternType::custom;
}

