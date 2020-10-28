/*
    ------------------------------------------------------------------

    This file is part of the Open Ephys GUI
    Copyright (C) 2020 Allen Institute for Brain Science and Open Ephys

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

#ifndef __NEUROPIXEDITOR_H_2AD3C591__
#define __NEUROPIXEDITOR_H_2AD3C591__

#include <VisualizerEditorHeaders.h>

#include "NeuropixComponents.h"

class UtilityButton;

class SourceNode;

class NeuropixEditor;
class NeuropixCanvas;
class NeuropixThread;

class EditorBackground : public Component
{
public:
	EditorBackground(int numBasestations, bool freqSelectEnabled);
	void setFreqSelectAvailable(bool available);

private:
	void paint(Graphics& g);

	int numBasestations;
	bool freqSelectEnabled;

};

class ProbeButton : public ToggleButton, public Timer
{
public:
	ProbeButton(int id, Probe* probe);

	void setSelectedState(bool);

	void setProbeStatus(ProbeStatus status);
	ProbeStatus getProbeStatus();

	void timerCallback();

	Probe* probe;
	bool connected;

	int id;

private:
	void paintButton(Graphics& g, bool isMouseOver, bool isButtonDown);

	ProbeStatus status;
	bool selected;
};

class FifoMonitor : public Component, public Timer
{
public:
	FifoMonitor(int id, Basestation* basestation);

	void setSlot(unsigned char);

	void setFillPercentage(float percentage);

	void timerCallback();

	unsigned char slot;
private:
	void paint(Graphics& g);

	float fillPercentage;
	Basestation* basestation;
	int id;
};

class BackgroundLoader : public Thread
{
public:
	BackgroundLoader(NeuropixThread* t, NeuropixEditor* e);
	~BackgroundLoader();
	void run();
private:
	NeuropixThread* thread;
	NeuropixEditor* editor;
};

/**

User interface for the Neuropixels source module.

@see SourceNode

*/
class NeuropixEditor : public VisualizerEditor, public ComboBox::Listener
{
public:
	NeuropixEditor(GenericProcessor* parentNode, NeuropixThread* thread, bool useDefaultParameterEditors);
	virtual ~NeuropixEditor();

	void collapsedStateChanged() override;

	void comboBoxChanged(ComboBox*);
	void buttonEvent(Button* button);

	void saveEditorParameters(XmlElement*);
	void loadEditorParameters(XmlElement*);

	Visualizer* createNewCanvas(void);

	OwnedArray<ProbeButton> probeButtons;

private:

	OwnedArray<UtilityButton> directoryButtons;
	OwnedArray<FifoMonitor> fifoMonitors;

	ScopedPointer<ComboBox> masterSelectBox;
	ScopedPointer<ComboBox> masterConfigBox;
	ScopedPointer<ComboBox> freqSelectBox;

	Array<File> savingDirectories;

	ScopedPointer<BackgroundLoader> uiLoader;
	ScopedPointer<EditorBackground> background;

	Viewport* viewport;
	NeuropixCanvas* canvas;
	NeuropixThread* thread;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NeuropixEditor);

};

#endif  // __NEUROPIXEDITOR_H_2AD3C591__
