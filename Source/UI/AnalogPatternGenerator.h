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

#ifndef __ANALOGPATTERNGENERATOR_H__
#define __ANALOGPATTERNGENERATOR_H__

#include <VisualizerEditorHeaders.h>

class WavePlayer;

enum class PatternType
{
    pulse = 0,
    sine = 1,
    custom = 2
};

struct Pattern
{
    Array<float> samples;
    PatternType patternType;
    float sampleRate;
    float maxVoltage;

    int id;
    String name = "PATTERN 1";

    int triggerChannel = 0;
    int gateChannel = -1;
    int analogOutputChannel = 0;

    struct Pulse
    {
        int onDuration = 100;
        int offDuration = 100;
        int delayDuration = 0;
        int repeatNumber = 1;
        int rampOnDuration = 0;
        int rampOffDuration = 0;
        float maxVoltage = 5.0f;
    };

    Pulse pulse;

    struct Sine
    {
        int frequency = 5;
        int cycles = 1;
        int delayDuration = 0;
        float maxVoltage = 5.0f;
    };

    Sine sine;

    struct Custom
    {
        Array<float> samples = { 0, 0, 0 };
        String string = "0,0,0";
    };

    Custom custom;
};

class AnalogPatternGenerator : public Component
{
public:
    AnalogPatternGenerator (WavePlayer* wp, Pattern* pattern);
    ~AnalogPatternGenerator();

    bool editable;

    virtual void updatePattern() = 0;
    virtual void buildWaveform() = 0;
    virtual void setState (Pattern* p) = 0;

protected:
    WavePlayer* wv;
    Pattern* pattern;
};

template <typename T>
class EditableTextInput : public Component, public Label::Listener
{
public:
    EditableTextInput<T> (String mainlabelText,
                          String unitsLabelText,
                          T minValue,
                          T maxValue,
                          T defaultValue,
                          AnalogPatternGenerator* apg);

    void labelTextChanged (Label* label);

    ~EditableTextInput();

    T getValue();
    void setValue (T);

    AnalogPatternGenerator* apg;

private:
    std::unique_ptr<Label> mainLabel;
    std::unique_ptr<Label> unitsLabel;
    std::unique_ptr<Label> inputBox;

    T minValue;
    T maxValue;
    T defaultValue;
    T lastValue;
};

class PulsePatternGenerator : public AnalogPatternGenerator
{
public:
    PulsePatternGenerator (WavePlayer* wp, Pattern* pattern);
    ~PulsePatternGenerator();

    void updatePattern();
    void setState (Pattern* p);

    void buildWaveform();

private:
    std::unique_ptr<EditableTextInput<int>> onDuration;
    std::unique_ptr<EditableTextInput<int>> offDuration;
    std::unique_ptr<EditableTextInput<int>> delayDuration;
    std::unique_ptr<EditableTextInput<int>> repeatNumber;
    std::unique_ptr<EditableTextInput<int>> rampOnDuration;
    std::unique_ptr<EditableTextInput<int>> rampOffDuration;
    std::unique_ptr<EditableTextInput<float>> maxVoltage;
};

class SinePatternGenerator : public AnalogPatternGenerator
{
public:
    SinePatternGenerator (WavePlayer* wp, Pattern* pattern);
    ~SinePatternGenerator();

    void updatePattern();
    void setState (Pattern* p);

    void buildWaveform();

private:
    std::unique_ptr<EditableTextInput<int>> frequency;
    std::unique_ptr<EditableTextInput<int>> cycles;
    std::unique_ptr<EditableTextInput<int>> delay;
    std::unique_ptr<EditableTextInput<float>> maxVoltage;
};

class CustomPatternGenerator : public AnalogPatternGenerator,
                               public TextEditor::Listener
{
public:
    CustomPatternGenerator (WavePlayer* wp, Pattern* pattern);
    ~CustomPatternGenerator();

    void updatePattern();
    void setState (Pattern* p);

    void buildWaveform();

    void textEditorReturnKeyPressed (TextEditor&);

    void keyPressed (KeyPress& key);

private:
    std::unique_ptr<TextEditor> textEditor;
    std::unique_ptr<Label> mainLabel;

    float constrainVoltage (float input, float min = 0, float max = 5);

    float maxVoltage;
};

#endif