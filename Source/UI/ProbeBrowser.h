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

#ifndef __PROBEBROWSER_H__
#define __PROBEBROWSER_H__

#include <VisualizerEditorHeaders.h>

#include "NeuropixInterface.h"

/** 

    Interactive graphical interface for viewing and selecting probe electrodes

*/
class ProbeBrowser : public Component,
                     public Timer
{
public:
    enum class DisplayMode
    {
        Interactive,
        OverviewOnly
    };

    /** Constructor */
    ProbeBrowser (NeuropixInterface*);

    /** Destructor */
    virtual ~ProbeBrowser();

    // Mouse interaction methods
    void mouseMove (const MouseEvent& event);
    void mouseDown (const MouseEvent& event);
    void mouseDrag (const MouseEvent& event);
    void mouseUp (const MouseEvent& event);
    void mouseWheelMove (const MouseEvent& event, const MouseWheelDetails& wheel);

    MouseCursor getMouseCursor();

    /** Timer callback for updating activity visualization */
    void timerCallback();

    /** Main paint method */
    void paint (Graphics& g);

    /** Switch between interactive and overview-only render modes */
    void setDisplayMode (DisplayMode mode);
    DisplayMode getDisplayMode() const { return displayMode; }

    /** Draw annotation overlays */
    void drawAnnotations (Graphics& g);

    // Zoom control methods
    int getZoomHeight();
    int getZoomOffset();
    void setZoomHeightAndOffset (int, int);

    /** Set max peak-to-peak amplitude */
    void setMaxPeakToPeakAmplitude (float);

    ActivityToView activityToView;
    float maxPeakToPeakAmplitude;

private:
    std::map<Bank, Colour> disconnectedColours;

    // display variables
    int zoomHeight;
    int zoomOffset;

    bool isOverZoomRegion;
    bool isOverUpperBorder;
    bool isOverLowerBorder;
    bool isOverElectrode;
    bool isSelectionActive;

    Rectangle<int> selectionBox;

    int initialOffset;
    int initialHeight;
    int lowerBound;
    int dragZoneWidth;
    int zoomAreaMinRow;
    int minZoomHeight;
    int maxZoomHeight;
    int shankOffset;
    int channelLabelSkip;
    int pixelHeight;

    float leftEdge;
    float rightEdge;

    int lowestElectrode;
    int highestElectrode;

    float electrodeHeight;

    Path shankPath;

    MouseCursor::StandardCursorType cursorType;

    String electrodeInfoString;

    // Helper methods
    void paintOverview (Graphics& g);
    Colour getElectrodeColour (int index);
    void calculateElectrodeColours();
    int getNearestElectrode (int x, int y);
    Array<int> getElectrodesWithinBounds (int x, int y, int w, int h);
    String getElectrodeInfoString (int index);
    int findElectrodeIndexForRow (int row) const;

    void handleLeftMouseDown (const MouseEvent& event);
    void handleRightMouseDown (const MouseEvent& event);
    void handleZoomDrag (const MouseEvent& event);
    void handleSelectionDrag (const MouseEvent& event);
    void clampZoomValues();

    std::unique_ptr<TooltipWindow> tooltipWindow;

    NeuropixInterface* parent;

    DisplayMode displayMode { DisplayMode::Interactive };
};

#endif // __PROBEBROWSER_H__
