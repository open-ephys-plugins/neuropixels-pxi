/*
    ------------------------------------------------------------------

    This file is part of the Open Ephys GUI
    Copyright (C) 2014 Open Ephys

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

#ifndef __PROBENAMECONFIG_H_F0BD2DD9__
#define __PROBENAMECONFIG_H_F0BD2DD9__

#include "../../JuceLibraryCode/JuceHeader.h" 

class ProbeNameConfig;
class NeuropixThread;
class Probe;
class Basestation;

/** 
    
    Custom text box for modifying the probe name

*/
class ProbeNameEditor : public Label,
    public Label::Listener
{
public:

    /** Constructor */
    ProbeNameEditor(ProbeNameConfig* p, int port, int dock);

    /** Destructor */
    ~ProbeNameEditor() {};

    /** Called when text is updated */
    void labelTextChanged(Label* label);

    int port;
    int dock;

    Probe* probe;
    
    String autoName;
    String autoNumber;
    String customPort;
    String customProbe;

    ProbeNameConfig* config;

};

/** 
    
    Button for selecting the naming scheme for a given slot

*/
class SelectionButton : public Button
{
public:

    /** Constructor */
    SelectionButton(ProbeNameConfig* p_, bool isPrev_) : Button(String(int(isPrev_))) {
        isPrev = isPrev_;
        p = p_; 
    };

    /** Destructor */
    ~SelectionButton() {};

private:
    ProbeNameConfig* p;

    bool isPrev;
    void paintButton(Graphics& g, bool isMouseOver, bool isButtonDown);
    void mouseUp(const MouseEvent& event);
};

/** 

    Popup component for defining custom names for probes
    
*/
class ProbeNameConfig : public Component
{

public:

    enum NamingScheme {
        AUTO_NAMING = 0,
        STREAM_INDICES,
        PORT_SPECIFIC_NAMING,
        PROBE_SPECIFIC_NAMING
    };

    /** Constructor */
    ProbeNameConfig(Basestation* basestation, NeuropixThread* thread);

    /** Destructor */
    ~ProbeNameConfig() {}

    /** Returns the active naming scheme */
    NamingScheme getNamingScheme() { return namingScheme; };

    /** Called when the */
    void update();
    void showNextScheme();
    void showPrevScheme();

    void paint(Graphics& g) override;

    std::vector<std::unique_ptr<ProbeNameEditor>> probeNames;

    /** Checks whether a requested name is unique, and if not appends a string */
    String checkUnique(String input, ProbeNameEditor* originalLabel);

    Basestation* basestation;
    NeuropixThread* thread;

private: 

    
    NamingScheme namingScheme = AUTO_NAMING;

    std::string schemes[4] = {
        "Automatic naming",
        "Automatic numbering",
        "Custom port names",
        "Custom probe names"
    };

    std::string descriptions[4] = {
    "Probes are given names in the order they appear (\"ProbeA\", \"ProbeB\", \"ProbeC\", etc.); \" - AP\" and \" - LFP\" are appended to the streams of 1.0 probes.",
    "Data streams are named in order \"0\", \"1\", \"2\", etc.; 1.0 probes have two streams each, 2.0 probes have one.",
    "Each port has a name associated with it (default, e.g. = \"slot2-port1-1\" for AP band of a 1.0 probe in slot 2, port 1, \"slot2-port2-2\" for a 2.0 probe in slot 2, port 2, dock 2).",
    "Each probe has a name associated with it (default = probe serial number). There should be one text box for each probe that is currently connected.",
    };

    ScopedPointer<Label> titleLabel;
    ScopedPointer<SelectionButton> prevButton;
    ScopedPointer<SelectionButton> nextButton;
    ScopedPointer<Label> schemeLabel;
    ScopedPointer<Label> description;
    ScopedPointer<Label> dock1Label;
    ScopedPointer<Label> dock2Label;

};


#endif  // __PROBENAMECONFIG_H_F0BD2DD9__
