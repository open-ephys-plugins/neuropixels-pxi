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

#ifndef __SETTINGSINTERFACE_H_2C4C2D67__
#define __SETTINGSINTERFACE_H_2C4C2D67__

#include <VisualizerEditorHeaders.h>

#include "../NeuropixComponents.h"

class NeuropixThread;
class NeuropixEditor;
class NeuropixCanvas;
class SettingsInterface;

/** 

    A viewport with a pointer to the settings interface it holds

*/
class CustomViewport : public Component
{
public:
    CustomViewport(SettingsInterface* settingsInterface_) :
        settingsInterface(settingsInterface_) 
        { 
            viewport = std::make_unique<Viewport>();
            viewport->setViewedComponent((Component*)settingsInterface, false);
            viewport->setScrollBarsShown(true, true, true, true);
            viewport->setScrollBarThickness(12);

            auto& horzScrollBar = viewport->getVerticalScrollBar();
            horzScrollBar.setLookAndFeel(&lnf2);
            horzScrollBar.setColour(ScrollBar::ColourIds::thumbColourId, Colour(50, 50, 50));
            horzScrollBar.setColour(ScrollBar::ColourIds::trackColourId, Colours::darkgrey);

            auto& vertScrollbar = viewport->getHorizontalScrollBar();
            vertScrollbar.setLookAndFeel(&lnf2);
            vertScrollbar.setColour(ScrollBar::ColourIds::thumbColourId, Colour(50, 50, 50));
            vertScrollbar.setColour(ScrollBar::ColourIds::trackColourId, Colours::darkgrey);

            addAndMakeVisible(viewport.get());
        }
    
    ~CustomViewport() 
    {
        viewport->getVerticalScrollBar().setLookAndFeel(nullptr);
        viewport->getHorizontalScrollBar().setLookAndFeel(nullptr);
    }
    
    void resized() override
    {
        viewport->setBounds(getLocalBounds());
    }

    SettingsInterface* settingsInterface;

private:
    std::unique_ptr<Viewport> viewport;
    LookAndFeel_V2 lnf2;
};

/** 

	A base class for the graphical interface for updating data source settings

*/
class SettingsInterface : public Component
{
public:

    /** Settings interface type*/
    enum Type
    {
        PROBE_SETTINGS_INTERFACE,
        ONEBOX_SETTINGS_INTERFACE,
        BASESTATION_SETTINGS_INTERFACE,
        UNKNOWN_SETTINGS_INTERFACE
    };

    /** Constructor */
    SettingsInterface(DataSource* dataSource_, 
        NeuropixThread* thread_, 
        NeuropixEditor* editor_, 
        NeuropixCanvas* canvas_)
    {
        dataSource = dataSource_;
        editor = editor_;
        canvas = canvas_;
        thread = thread_;
        
        setBounds(0, 0, 1000, 820);

        viewport = new CustomViewport(this);
        
    }
    
    /** Destructor */
	~SettingsInterface() {}

    /** Called when acquisition begins */
	virtual void startAcquisition() = 0;

    /** Called when acquisition ends */
	virtual void stopAcquisition() = 0;

    /** Applies settings to the probe associated with this interface */
	virtual bool applyProbeSettings(ProbeSettings, bool shouldUpdateProbe = true) = 0;

    /** Saves settings */
	virtual void saveParameters(XmlElement* xml) = 0;

	/** Loads settings */
	virtual void loadParameters(XmlElement* xml) = 0;

    /** Updates the string with info about the underlying data source*/
    virtual void updateInfoString() = 0;

    /** Default type */
    Type type = UNKNOWN_SETTINGS_INTERFACE;

    /** Viewport for scrolling around this interface */
    ScopedPointer<CustomViewport> viewport;

    /** Pointer to the data source*/
    DataSource* dataSource;

protected:

	NeuropixThread* thread;
	NeuropixEditor* editor;
	NeuropixCanvas* canvas;

};

#endif //__SETTINGSINTERFACE_H_2C4C2D67__