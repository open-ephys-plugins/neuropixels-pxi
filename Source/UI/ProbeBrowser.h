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

class ProbeBrowser : public Component,
                     public Timer
{
public:
    ProbeBrowser (NeuropixInterface*);
    virtual ~ProbeBrowser();

    void mouseMove (const MouseEvent& event);
    void mouseDown (const MouseEvent& event);
    void mouseDrag (const MouseEvent& event);
    void mouseUp (const MouseEvent& event);
    void mouseWheelMove (const MouseEvent& event, const MouseWheelDetails& wheel);

    MouseCursor getMouseCursor();

    void timerCallback();

    void paint (Graphics& g);

    void drawAnnotations (Graphics& g);

    int getZoomHeight();
    int getZoomOffset();

    void setZoomHeightAndOffset (int, int);

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

    int initialOffset;
    int initialHeight;
    int lowerBound;
    int dragZoneWidth;
    int zoomAreaMinRow;
    int minZoomHeight;
    int maxZoomHeight;
    int shankOffset;
    int leftEdge;
    int rightEdge;
    int channelLabelSkip;
    int pixelHeight;

    int lowestElectrode;
    int highestElectrode;

    float electrodeHeight;

    Path shankPath;

    MouseCursor::StandardCursorType cursorType;

    String electrodeInfoString;

    Colour getElectrodeColour (int index);
    int getNearestElectrode (int x, int y);
    Array<int> getElectrodesWithinBounds (int x, int y, int w, int h);
    String getElectrodeInfoString (int index);

    NeuropixInterface* parent;
};

#endif // __PROBEBROWSER_H__
