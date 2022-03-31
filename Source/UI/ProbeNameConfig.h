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
#include "../NeuropixThread.h"

class ProbeNameConfig;
class NeuropixThread;

class ProbeNameEditor : public TextEditor
{
public:
    ProbeNameEditor(ProbeNameConfig* p, int slot, int port, int dock);
    ~ProbeNameEditor() {};

    int slot;
    int port;
    int dock;

    bool hasProbe;
    
    String autoName;
    String autoNumber;
    String customPort;
    String customProbe;


};

class SelectionButton : public Button
{
public:
    SelectionButton(ProbeNameConfig* p_, bool isPrev_) : Button(String(int(isPrev_))) {
        isPrev = isPrev_;
        p = p_; 
    };
    ~SelectionButton() {};

private:
    ProbeNameConfig* p;

    bool isPrev;
    void paintButton(Graphics& g, bool isMouseOver, bool isButtonDown);
    void mouseUp(const MouseEvent& event);
};

class ProbeNameConfig : public Component
{

public:

    /** Constructor */
    ProbeNameConfig(NeuropixThread* t_, int slot, int schemeIdx);

    /** Destructor */
    ~ProbeNameConfig() {}

    int getSchemeIdx() { return schemeIdx; };
    void update();
    void showNextScheme();
    void showPrevScheme();

    /** Save settings. */
    void saveStateToXml(XmlElement*);

    /** Load settings. */
    void loadStateFromXml(XmlElement*);

    std::vector<std::unique_ptr<ProbeNameEditor>> probeNames;

private: 

    NeuropixThread* t;
    int schemeIdx = 0;

    std::string schemes[4] = {
        "Automatic naming",
        "Automatic numbering",
        "Custom port names",
        "Custom probe names"
    };

    std::string descriptions[4] = {
    "Probes are given names in the order they appear (\"ProbeA\", \"ProbeB\", \"ProbeC\", etc.); \" - AP\" and \" - LFP\" are appended to the streams of 1.0 probes\n                       ",
    "Data streams are named in order \"0\", \"1\", \"2\", etc.; 1.0 probes have two streams each, 2.0 probes have 1                                                                         ",
    "Each port has a name associated with it (default, e.g. = \"Port2 - 1 - AP\" for AP band of a 1.0 probe in slot 2, port 1, \"Port2 - 2 - 1\" for a 2.0 probe in slot 2, port 2, dock 1).",
    "Each probe has a name associated with it (default = probe serial number). There should be one text box for each probe that is currently connected                                      ",
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
