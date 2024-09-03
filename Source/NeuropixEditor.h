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

#ifndef __NEUROPIXEDITOR_H_2AD3C591__
#define __NEUROPIXEDITOR_H_2AD3C591__

#include "UI\ProbeNameConfig.h"
#include <VisualizerEditorHeaders.h>

#include "NeuropixComponents.h"

class UtilityButton;

class SourceNode;

class NeuropixEditor;
class NeuropixCanvas;
class NeuropixThread;

/**
    Refreshes the basestation to check for any hardware changes

*/

class RefreshButton : public Button
{

public:
    /** Constructor */
    RefreshButton ();

    /** Destructor */
    ~RefreshButton() {}

    void paintButton (Graphics& g, bool isMouseOver, bool isButtonDown) override;

private:

    std::unique_ptr<Drawable> refreshIcon;
};
/** 

	Displays the slot number, and opens a pop-up name
	configuration window when clicked.

*/
class SlotButton : public Button, public ComponentListener
{
public:
    /** Constructor */
    SlotButton (Basestation* bs, NeuropixThread* thread);

    /** Destructor */
    ~SlotButton() {}

    bool isEnabled;

private:
    Basestation* basestation;
    NeuropixThread* thread;
    int slot;

    /** Draws the slot number */
    void paintButton (Graphics& g, bool isMouseOver, bool isButtonDown);

    /** Opens the configuration window */
    void mouseUp (const MouseEvent& event);

    /** Called when the configuration window is closed */
    void componentBeingDeleted (Component& component);
};

/** 
* 
	Draws the background for the Neuropixels-PXI Editor

	*/
class EditorBackground : public Component, public ComponentListener
{
public:
    /** Constructor */
    EditorBackground (NeuropixThread* t, bool freqSelectEnabled);

    /** Toggles the frequency select option */
    void setFreqSelectAvailable (bool available);

    /* A map from slot number label to its bounds in the editor */
    std::vector<std::unique_ptr<SlotButton>> slotButtons;

    /** Pointer to the probe naming popup */
    std::unique_ptr<ProbeNameConfig> probeNamingPopup;

    /** Disables/enables slot buttons during/after acquisition*/
    void setEnabled (bool isEnabled)
    {
        for (int i = 0; i < slotButtons.size(); i++)
            slotButtons[i]->isEnabled = isEnabled;
    }

private:
    /** Draws the background */
    void paint (Graphics& g);

    int numBasestations;
    bool freqSelectEnabled;

    /* An array of Basestation objections, one for each basestation detected */
    Array<Basestation*> basestations;

    DeviceType type;
};

/** 

	Button representing one data source (usually a probe)

*/
class SourceButton : public ToggleButton, public Timer
{
public:
    /** Constructor */
    SourceButton (int id, DataSource* probe, Basestation* basestation = nullptr);

    /** Toggles the button selected state */
    void setSelectedState (bool);

    /** Sets the status (CONNECTED, CONNECTING, etc.) */
    void setSourceStatus (SourceStatus status);

    /** Returns the status of the associated source */
    SourceStatus getSourceStatus();

    /** Checks whether the status has changed */
    void timerCallback();

    DataSource* dataSource;
    Basestation* basestation;
    bool connected;

    int id;

private:
    /** Draws the button */
    void paintButton (Graphics& g, bool isMouseOver, bool isButtonDown);

    SourceStatus status;
    DataSourceType sourceType;
    bool selected;
};

/** 

	Displays the FIFO filling state for each basestation

*/
class FifoMonitor : public Component, public Timer
{
public:
    /** Constructor */
    FifoMonitor (int id, Basestation* basestation);

    /** Sets the slot ID for this monitor */
    void setSlot (unsigned char);

    /** Sets the fill percentage to display */
    void setFillPercentage (float percentage);

    /** Calls repaint() */
    void timerCallback();

    unsigned char slot;

private:
    /** Renders the monitor */
    void paint (Graphics& g);

    float fillPercentage;
    Basestation* basestation;
    int id;
};

/** 

	A thread that loads probe settings in the background
	(to prevent blocking the UI)

*/
class BackgroundLoader : public Thread
{
public:
    /** Constructor */
    BackgroundLoader (NeuropixThread* t, NeuropixEditor* e);

    /** Destructor */
    ~BackgroundLoader();

    /** Runs the thread */
    void run();

    bool signalChainIsLoading;
    bool isRefreshing;

private:
    NeuropixThread* thread;
    NeuropixEditor* editor;

    bool isInitialized;
};

/**

User interface for the Neuropixels source module.

@see SourceNode

*/
class NeuropixEditor : public VisualizerEditor,
                       public ComboBox::Listener,
                       public Button::Listener
{
public:
    /** Constructor */
    NeuropixEditor (GenericProcessor* parentNode, NeuropixThread* thread);

    /** Destructor */
    virtual ~NeuropixEditor() {}

    /** Called when editor is collapsed */
    void collapsedStateChanged() override;

    /** Respond to combo box changes*/
    void comboBoxChanged (ComboBox*);

    /** Respond to button presses */
    void buttonClicked (Button* button) override;

    /** Save editor parameters (e.g. sync settings)*/
    void saveVisualizerEditorParameters (XmlElement*) override;

    /** Load editor parameters (e.g. sync settings)*/
    void loadVisualizerEditorParameters (XmlElement*) override;

    /** Called just prior to the start of acquisition, to allow custom commands. */
    void startAcquisition() override;

    /** Called after the end of acquisition, to allow custom commands .*/
    void stopAcquisition() override;

    /** Creates the Neuropixels settings interface*/
    Visualizer* createNewCanvas (void);

    /** Initializes the probes in a background thread */
    void initialize (bool signalChainIsLoading);

    /** Update settings */
    void update();

    /** Draw basestations UI */
    void drawBasestations(Array<Basestation*> basestations);

    /** Select a data source button */
    void selectSource (DataSource* source);

    void checkCanvas() { checkForCanvas(); };

    void resetCanvas();

    std::vector<std::unique_ptr<SourceButton>> sourceButtons;

    std::unique_ptr<BackgroundLoader> uiLoader;

    NeuropixCanvas* canvas;

private:
    OwnedArray<UtilityButton> directoryButtons;
    OwnedArray<FifoMonitor> fifoMonitors;

    std::unique_ptr<ComboBox> mainSyncSelector;
    std::unique_ptr<ComboBox> inputOutputSyncSelector;
    // std::unique_ptr<ComboBox> syncFrequencySelector;
    std::unique_ptr<Label> syncFrequencyLabel;

    Array<File> savingDirectories;
    Array<int> slotNamingSchemes;

    std::unique_ptr<EditorBackground> background;

    std::unique_ptr<UtilityButton> addSyncChannelButton;
    std::unique_ptr<RefreshButton> refreshButton;

    Viewport* viewport;
    NeuropixThread* thread;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NeuropixEditor);
};

#endif // __NEUROPIXEDITOR_H_2AD3C591__
