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

#include "ProbeBrowser.h"

// Convert Bank enum to a short String label
static String bankToString (Bank b)
{
    switch (b)
    {
        case Bank::A:
            return "A";
        case Bank::A1:
            return "A1";
        case Bank::A2:
            return "A2";
        case Bank::A3:
            return "A3";
        case Bank::A4:
            return "A4";
        case Bank::B:
            return "B";
        case Bank::B1:
            return "B1";
        case Bank::B2:
            return "B2";
        case Bank::B3:
            return "B3";
        case Bank::B4:
            return "B4";
        case Bank::C:
            return "C";
        case Bank::C1:
            return "C1";
        case Bank::C2:
            return "C2";
        case Bank::C3:
            return "C3";
        case Bank::C4:
            return "C4";
        case Bank::D:
            return "D";
        case Bank::D1:
            return "D1";
        case Bank::D2:
            return "D2";
        case Bank::D3:
            return "D3";
        case Bank::D4:
            return "D4";
        case Bank::E:
            return "E";
        case Bank::F:
            return "F";
        case Bank::G:
            return "G";
        case Bank::H:
            return "H";
        case Bank::I:
            return "I";
        case Bank::J:
            return "J";
        case Bank::K:
            return "K";
        case Bank::L:
            return "L";
        case Bank::M:
            return "M";
        case Bank::OFF:
            return "OFF";
        case Bank::NONE:
            return "NONE";
        default:
            return "NONE";
    }
}

ProbeBrowser::ProbeBrowser (NeuropixInterface* parent_) : parent (parent_)
{
    tooltipWindow = std::make_unique<TooltipWindow> (parent, 300);

    cursorType = MouseCursor::NormalCursor;

    activityToView = ActivityToView::APVIEW;

    ProbeType probeType = parent->probe->type;

    if (probeType == ProbeType::QUAD_BASE || probeType == ProbeType::NP2_1 || probeType == ProbeType::NP2_4)
    {
        maxPeakToPeakAmplitude = 1000.0f;
    }
    else
    {
        maxPeakToPeakAmplitude = 250.0f;
    }

    isOverZoomRegion = false;
    isOverUpperBorder = false;
    isOverLowerBorder = false;
    isSelectionActive = false;
    isOverElectrode = false;

    selectionBox = Rectangle<int> (0, 0, 0, 0);

    // PROBE SPECIFIC DRAWING SETTINGS

    // Set default values
    minZoomHeight = 40;
    maxZoomHeight = 120;
    pixelHeight = 1;
    zoomOffset = 50;
    int defaultZoomHeight = 100;

    shankPath = Path (parent->probe->shankOutline);

    // Adjust settings based on columns per shank
    const int columns = parent->probeMetadata.columns_per_shank;
    if (columns == 8)
    {
        maxZoomHeight = 450;
        minZoomHeight = 300;
        defaultZoomHeight = 400;
        pixelHeight = 10;
        zoomOffset = 0;
    }
    else if (columns > 8)
    {
        maxZoomHeight = 520;
        minZoomHeight = 520;
        defaultZoomHeight = 520;
        pixelHeight = 20;
        zoomOffset = 0;
    }

    // Adjust settings based on rows per shank
    const int rows = parent->probeMetadata.rows_per_shank;
    if (rows > 1400)
    {
        maxZoomHeight = 30;
        minZoomHeight = 5;
        defaultZoomHeight = 20;
    }
    else if (rows > 650)
    {
        maxZoomHeight = 60;
        minZoomHeight = 10;
        defaultZoomHeight = 30;
    }

    // Adjust default zoom height for 4-shank probes
    if (parent->probeMetadata.shank_count == 4)
    {
        defaultZoomHeight = 80;
    }

    // Configure channel label skip based on electrodes per shank
    const int electrodes = parent->probeMetadata.electrodes_per_shank;
    if (electrodes < 500)
        channelLabelSkip = 50;
    else if (electrodes < 1500)
        channelLabelSkip = 100;
    else if (electrodes < 3000)
        channelLabelSkip = 200;
    else
        channelLabelSkip = 500;

    //addMouseListener(this, false);

    zoomHeight = defaultZoomHeight; // number of rows
    lowerBound = 530; // bottom of interface

    // Initialize with default black for undefined/disabled banks
    disconnectedColours[Bank::NONE] = Colours::black;
    disconnectedColours[Bank::OFF] = Colours::black;

    // Group bank colours
    const Colour groupAColour (180, 180, 180); // lighter grey
    const Colour groupBColour (160, 160, 160); // medium-light grey
    const Colour groupCColour (140, 140, 140); // medium grey
    const Colour groupDColour (120, 120, 120); // darker grey

    auto setGroupColours = [this] (Colour c, std::initializer_list<Bank> banks)
    {
        for (auto b : banks)
            disconnectedColours[b] = c;
    };

    setGroupColours (groupAColour, { Bank::A, Bank::A1, Bank::A2, Bank::A3, Bank::A4, Bank::E, Bank::I, Bank::M });
    setGroupColours (groupBColour, { Bank::B, Bank::B1, Bank::B2, Bank::B3, Bank::B4, Bank::F, Bank::J });
    setGroupColours (groupCColour, { Bank::C, Bank::C1, Bank::C2, Bank::C3, Bank::C4, Bank::G, Bank::K });
    setGroupColours (groupDColour, { Bank::D, Bank::D1, Bank::D2, Bank::D3, Bank::D4, Bank::H, Bank::L });

    dragZoneWidth = 10;
}

ProbeBrowser::~ProbeBrowser()
{
}

void ProbeBrowser::mouseMove (const MouseEvent& event)
{
    const float y = event.y;
    const float x = event.x;

    const bool inZoomY = (y > lowerBound - zoomOffset - zoomHeight - dragZoneWidth / 2)
                         && (y < lowerBound - zoomOffset + dragZoneWidth / 2);
    const bool inZoomX = (x > 9) && (x < 54 + shankOffset);
    const bool isOverZoomRegionNew = inZoomY && inZoomX;

    const bool isOverUpperBorderNew = isOverZoomRegionNew
                                      && (y > lowerBound - zoomHeight - zoomOffset - dragZoneWidth / 2)
                                      && (y < lowerBound - zoomHeight - zoomOffset + dragZoneWidth / 2);

    const bool isOverLowerBorderNew = isOverZoomRegionNew
                                      && (y > lowerBound - zoomOffset - dragZoneWidth / 2)
                                      && (y < lowerBound - zoomOffset + dragZoneWidth / 2);

    // update cursor type
    if (isOverZoomRegionNew != isOverZoomRegion
        || isOverLowerBorderNew != isOverLowerBorder
        || isOverUpperBorderNew != isOverUpperBorder)
    {
        isOverZoomRegion = isOverZoomRegionNew;
        isOverUpperBorder = isOverUpperBorderNew;
        isOverLowerBorder = isOverLowerBorderNew;

        if (! isOverZoomRegion)
            cursorType = MouseCursor::NormalCursor;
        else if (isOverUpperBorder)
            cursorType = MouseCursor::TopEdgeResizeCursor;
        else if (isOverLowerBorder)
            cursorType = MouseCursor::BottomEdgeResizeCursor;
        else
            cursorType = MouseCursor::DraggingHandCursor;

        repaint();
    }

    // check for movement over electrode
    if ((x > leftEdge) && (x < rightEdge)
        && (y < lowerBound + 16) && (y > 16))
    {
        const int index = getNearestElectrode (event.x, event.y);

        if (index > -1)
        {
            isOverElectrode = true;
            electrodeInfoString = getElectrodeInfoString (index);
            tooltipWindow->displayTip (event.getScreenPosition(), electrodeInfoString);
        }
    }
    else if (isOverElectrode)
    {
        isOverElectrode = false;
        tooltipWindow->hideTip();
    }
}

int ProbeBrowser::getNearestElectrode (int x, int y)
{
    int row = static_cast<int> (std::floor (((lowerBound + 14 - electrodeHeight - y) / electrodeHeight) + zoomAreaMinRow + 1));

    float shankWidth = electrodeHeight * parent->probeMetadata.columns_per_shank;
    float totalWidth = shankWidth * parent->probeMetadata.shank_count + shankWidth * (parent->probeMetadata.shank_count - 1);

    int shank = 0;
    int column = -1;

    for (shank = 0; shank < parent->probeMetadata.shank_count; shank++)
    {
        const float shankLeftEdge = 220 + shankOffset - totalWidth / 2 + shankWidth * 2 * shank;
        const float shankRightEdge = shankLeftEdge + shankWidth;

        if (x >= shankLeftEdge && x <= shankRightEdge)
        {
            column = (x - shankLeftEdge) / electrodeHeight;
            break;
        }
    }

    //std::cout << "x: " << x << ", column: " << column << ", shank: " << shank << std::endl;
    //std::cout << "y: " << y <<  ", row: " << row << std::endl;

    for (int i = 0; i < parent->electrodeMetadata.size(); i++)
    {
        if ((parent->electrodeMetadata[i].row_index == row)
            && (parent->electrodeMetadata[i].column_index == column)
            && (parent->electrodeMetadata[i].shank == shank))
        {
            return i;
        }
    }

    return -1;
}

Array<int> ProbeBrowser::getElectrodesWithinBounds (int x, int y, int w, int h)
{
    int startrow = static_cast<int> (std::ceil ((lowerBound - y - h) / electrodeHeight) + zoomAreaMinRow + 1);
    int endrow = static_cast<int> (std::floor ((lowerBound - y) / electrodeHeight) + zoomAreaMinRow + 1);

    float shankWidth = electrodeHeight * parent->probeMetadata.columns_per_shank;

    Array<int> selectedColumns;

    for (int i = 0; i < parent->probeMetadata.shank_count * parent->probeMetadata.columns_per_shank; i++)
    {
        int shank = i / parent->probeMetadata.columns_per_shank;
        int column = i % parent->probeMetadata.columns_per_shank;

        int l = leftEdge + shankWidth * 2 * shank + electrodeHeight * column;
        int r = l + electrodeHeight / 2;

        if (x < l + electrodeHeight / 2 && x + w > r)
            selectedColumns.add (i);
    }

    //int startcolumn = (x - (220 + shankOffset - electrodeHeight * probeMetadata.columns_per_shank / 2)) / electrodeHeight;
    //int endcolumn = (x + w - (220 + shankOffset - electrodeHeight * probeMetadata.columns_per_shank / 2)) / electrodeHeight;

    Array<int> inds;

    //std::cout << startrow << " " << endrow << " " << startcolumn << " " << endcolumn << std::endl;

    for (int i = 0; i < parent->electrodeMetadata.size(); i++)
    {
        if ((parent->electrodeMetadata[i].row_index >= startrow) && (parent->electrodeMetadata[i].row_index <= endrow))

        {
            int column_id = parent->electrodeMetadata[i].shank * parent->probeMetadata.columns_per_shank + parent->electrodeMetadata[i].column_index;

            if (selectedColumns.indexOf (column_id) > -1)
            {
                inds.add (i);
            }
        }
    }

    return inds;
}

String ProbeBrowser::getElectrodeInfoString (int index)
{
    String a;
    a << "Electrode " << String (parent->electrodeMetadata[index].global_index + 1);
    a << "\nBank " << bankToString (parent->electrodeMetadata[index].bank);

    a << ", Channel " << String (parent->electrodeMetadata[index].channel + 1);

    a << "\nY Position: " << String (parent->electrodeMetadata[index].ypos);

    a << "\nType: ";

    if (parent->electrodeMetadata[index].type == ElectrodeType::REFERENCE)
    {
        a << "REFERENCE";
    }
    else
    {
        a << "SIGNAL";
        a << "\nEnabled: ";

        if (parent->electrodeMetadata[index].status == ElectrodeStatus::CONNECTED)
            a << "YES";
        else
            a << "NO";
    }

    if (parent->apGainComboBox != nullptr)
    {
        a << "\nAP Gain: " << String (parent->apGainComboBox->getText());
    }

    if (parent->lfpGainComboBox != nullptr)
    {
        a << "\nLFP Gain: " << String (parent->lfpGainComboBox->getText());
    }

    if (parent->referenceComboBox != nullptr)
    {
        a << "\nReference: " << String (parent->referenceComboBox->getText());
    }

    return a;
}

void ProbeBrowser::mouseUp (const MouseEvent& event)
{
    if (isSelectionActive)
    {
        isSelectionActive = false;
        repaint();
    }
}

void ProbeBrowser::mouseDown (const MouseEvent& event)
{
    initialOffset = zoomOffset;
    initialHeight = zoomHeight;

    if (! event.mods.isRightButtonDown())
    {
        handleLeftMouseDown (event);
    }
    else
    {
        handleRightMouseDown (event);
    }
}

void ProbeBrowser::handleLeftMouseDown (const MouseEvent& event)
{
    if (event.x > 150 && event.x < 400)
    {
        if (! event.mods.isShiftDown())
        {
            for (int i = 0; i < parent->electrodeMetadata.size(); i++)
                parent->electrodeMetadata.getReference (i).isSelected = false;
        }

        if (event.x > leftEdge && event.x < rightEdge)
        {
            int chan = getNearestElectrode (event.x, event.y);
            if (chan >= 0 && chan < parent->electrodeMetadata.size())
            {
                parent->electrodeMetadata.getReference (chan).isSelected = true;
            }
        }
        repaint();
    }
}

void ProbeBrowser::handleRightMouseDown (const MouseEvent& event)
{
    if (event.x > 225 + 10 && event.x < 225 + 150)
    {
        int currentAnnotationNum = -1;
        for (int i = 0; i < parent->annotations.size(); i++)
        {
            Annotation& a = parent->annotations.getReference (i);
            float yLoc = a.currentYLoc;
            if (float (event.y) < yLoc && float (event.y) > yLoc - 12)
            {
                currentAnnotationNum = i;
                break;
            }
        }

        if (currentAnnotationNum > -1)
        {
            PopupMenu annotationMenu;
            annotationMenu.addItem (1, "Delete annotation", true);
            const int result = annotationMenu.show();

            if (result == 1)
            {
                parent->annotations.removeRange (currentAnnotationNum, 1);
                repaint();
            }
        }
    }
    // Uncomment and implement if annotation adding is needed
    // if (event.x > 225 - channelHeight && event.x < 225 + channelHeight)
    // {
    //     PopupMenu annotationMenu;
    //     annotationMenu.addItem(1, "Add annotation", true);
    //     const int result = annotationMenu.show();
    //     if (result == 1)
    //     {
    //         // Handle annotation addition
    //     }
    // }
}

void ProbeBrowser::mouseDrag (const MouseEvent& event)
{
    if (isOverZoomRegion)
    {
        handleZoomDrag (event);
    }
    else if (event.x > 150 && event.x < 450)
    {
        handleSelectionDrag (event);
    }

    clampZoomValues();
    repaint();
}

void ProbeBrowser::handleZoomDrag (const MouseEvent& event)
{
    if (isOverUpperBorder)
    {
        zoomHeight = initialHeight - event.getDistanceFromDragStartY();
        if (zoomHeight > lowerBound - zoomOffset - 15)
            zoomHeight = lowerBound - zoomOffset - 15;
    }
    else if (isOverLowerBorder)
    {
        zoomOffset = initialOffset - event.getDistanceFromDragStartY();
        if (zoomOffset < 0)
        {
            zoomOffset = 0;
        }
        else
        {
            zoomHeight = initialHeight + event.getDistanceFromDragStartY();
        }
    }
    else
    {
        zoomOffset = initialOffset - event.getDistanceFromDragStartY();
        if (zoomOffset < 0)
        {
            zoomOffset = 0;
        }
    }
}

void ProbeBrowser::handleSelectionDrag (const MouseEvent& event)
{
    int w = event.getDistanceFromDragStartX();
    int h = event.getDistanceFromDragStartY();
    int x = event.getMouseDownX();
    int y = event.getMouseDownY();

    if (w < 0)
    {
        x = x + w;
        w = -w;
    }
    if (h < 0)
    {
        y = y + h;
        h = -h;
    }

    selectionBox = Rectangle<int> (x, y, w, h);
    isSelectionActive = true;

    Array<int> inBounds = getElectrodesWithinBounds (x, y, w, h);

    if (x < rightEdge)
    {
        for (int i = 0; i < parent->electrodeMetadata.size(); i++)
        {
            if (inBounds.indexOf (i) > -1)
            {
                parent->electrodeMetadata.getReference (i).isSelected = true;
            }
            else
            {
                if (! event.mods.isShiftDown())
                    parent->electrodeMetadata.getReference (i).isSelected = false;
            }
        }
    }
}

void ProbeBrowser::clampZoomValues()
{
    if (zoomHeight < minZoomHeight)
        zoomHeight = minZoomHeight;
    if (zoomHeight > maxZoomHeight)
        zoomHeight = maxZoomHeight;

    if (zoomOffset > lowerBound - zoomHeight - 15)
        zoomOffset = lowerBound - zoomHeight - 15;
    else if (zoomOffset < 0)
        zoomOffset = 0;
}

void ProbeBrowser::mouseWheelMove (const MouseEvent& event, const MouseWheelDetails& wheel)
{
    if (event.x > 100 && event.x < 450)
    {
        if (wheel.deltaY > 0)
            zoomOffset += 2;
        else
            zoomOffset -= 2;

        if (zoomOffset > lowerBound - zoomHeight - 16)
            zoomOffset = lowerBound - zoomHeight - 16;
        else if (zoomOffset < 0)
            zoomOffset = 0;

        repaint();
    }
}

MouseCursor ProbeBrowser::getMouseCursor()
{
    MouseCursor c = MouseCursor (cursorType);

    return c;
}

void ProbeBrowser::paint (Graphics& g)
{
    const int LEFT_BORDER = parent->probeMetadata.columns_per_shank >= 8 ? 23 : 30;
    const int TOP_BORDER = 33;
    const int SHANK_HEIGHT = 480;
    const int INTERSHANK_DISTANCE = 30;

    // Draw zoomed-out channels
    int channelSpan = SHANK_HEIGHT;
    int pixelGap = (parent->probeMetadata.columns_per_shank > 8) ? 1 : 2;

    // Draw all electrodes in zoomed-out view
    for (int i = 0; i < parent->electrodeMetadata.size(); ++i)
    {
        g.setColour (getElectrodeColour (i).withAlpha (0.5f));
        int col = parent->electrodeMetadata[i].column_index;
        int shank = parent->electrodeMetadata[i].shank;
        int row = parent->electrodeMetadata[i].row_index;

        for (int px = 0; px < pixelHeight; ++px)
        {
            int x = LEFT_BORDER + col * pixelGap + shank * INTERSHANK_DISTANCE;
            int y = TOP_BORDER + channelSpan
                    - int (float (row) * float (channelSpan) / float (parent->probeMetadata.rows_per_shank))
                    - px;
            g.fillRect (x, y, 1, 1);
        }
    }

    // Draw channel numbers and tick marks
    g.setColour (findColour (ThemeColours::defaultText));
    g.setFont (12);

    int ch = 0;
    int ch_interval = SHANK_HEIGHT * channelLabelSkip / parent->probeMetadata.electrodes_per_shank;
    shankOffset = INTERSHANK_DISTANCE * (parent->probeMetadata.shank_count - 1);

    for (int i = TOP_BORDER + channelSpan; i > TOP_BORDER; i -= ch_interval)
    {
        g.drawLine (6, i, 18, i);
        g.drawLine (44 + shankOffset, i, 54 + shankOffset, i);
        g.drawText (String (ch), 59 + shankOffset, i - 6, 100, 12, Justification::left, false);
        ch += channelLabelSkip;
    }

    // Draw top channel tick and label
    g.drawLine (6, TOP_BORDER, 18, TOP_BORDER);
    g.drawLine (44 + shankOffset, TOP_BORDER, 54 + shankOffset, TOP_BORDER);
    g.drawText (String (parent->probeMetadata.electrodes_per_shank),
                59 + shankOffset,
                TOP_BORDER - 6,
                100,
                12,
                Justification::left,
                false);

    // Draw shank outlines
    g.setColour (findColour (ThemeColours::outline).withAlpha (0.75f));
    for (int i = 0; i < parent->probeMetadata.shank_count; ++i)
    {
        Path outline = parent->probeMetadata.shankOutline;
        outline.applyTransform (AffineTransform::translation (INTERSHANK_DISTANCE * i, 0.0f));
        g.strokePath (outline, PathStrokeType (1.0));
    }

    // Calculate zoomed area parameters
    float miniRowHeight = float (channelSpan) / float (parent->probeMetadata.rows_per_shank);
    float lowestRow = (zoomOffset - 16) / miniRowHeight;
    float highestRow = lowestRow + (zoomHeight / miniRowHeight);
    zoomAreaMinRow = static_cast<int> (std::ceil (lowestRow));

    if (parent->probeMetadata.columns_per_shank > 8)
        electrodeHeight = jmin (lowerBound / (highestRow - lowestRow), 12.0f);
    else
        electrodeHeight = lowerBound / (highestRow - lowestRow);

    highestRow = zoomAreaMinRow + (zoomHeight / miniRowHeight);

    // Draw zoomed-in electrodes
    for (int i = 0; i < parent->electrodeMetadata.size(); ++i)
    {
        int row = parent->electrodeMetadata[i].row_index;
        if (row >= static_cast<int> (std::ceil (lowestRow)) && row < static_cast<int> (std::floor (highestRow)))
        {
            int col = parent->electrodeMetadata[i].column_index;
            int shank = parent->electrodeMetadata[i].shank;
            float xLoc = 220 + shankOffset - electrodeHeight * parent->probeMetadata.columns_per_shank / 2
                         + electrodeHeight * col + shank * electrodeHeight * 4
                         - (parent->probeMetadata.shank_count / 2 * electrodeHeight * 3);
            float yLoc = lowerBound - ((row - static_cast<int> (std::ceil (lowestRow))) * electrodeHeight)
                         + 15 - electrodeHeight;

            if (parent->electrodeMetadata[i].isSelected)
            {
                g.setColour (findColour (ThemeColours::componentBackground).contrasting());
                g.fillRect (xLoc, yLoc, electrodeHeight, electrodeHeight);
            }

            g.setColour (getElectrodeColour (i));
            g.fillRect (xLoc + 1, yLoc + 1, electrodeHeight - 2, electrodeHeight - 2);
        }
    }

    // Draw zoom area borders
    g.setColour (isOverZoomRegion
                     ? findColour (ThemeColours::outline)
                     : findColour (ThemeColours::outline).withAlpha (0.5f));

    Path upperBorder;
    upperBorder.startNewSubPath (5, lowerBound - zoomOffset - zoomHeight);
    upperBorder.lineTo (54 + shankOffset, lowerBound - zoomOffset - zoomHeight);
    upperBorder.lineTo (100 + shankOffset, 16);
    upperBorder.lineTo (330 + shankOffset, 16);

    Path lowerBorder;
    lowerBorder.startNewSubPath (5, lowerBound - zoomOffset);
    lowerBorder.lineTo (54 + shankOffset, lowerBound - zoomOffset);
    lowerBorder.lineTo (100 + shankOffset, lowerBound + 16);
    lowerBorder.lineTo (330 + shankOffset, lowerBound + 16);

    g.strokePath (upperBorder, PathStrokeType (2.0));
    g.strokePath (lowerBorder, PathStrokeType (2.0));

    // Calculate selection area edges
    float shankWidth = electrodeHeight * parent->probeMetadata.columns_per_shank;
    float totalWidth = shankWidth * parent->probeMetadata.shank_count
                       + shankWidth * (parent->probeMetadata.shank_count - 1);

    leftEdge = 220 + shankOffset - totalWidth / 2;
    rightEdge = 220 + shankOffset + totalWidth / 2;

    // Draw selection rectangle if active
    if (isSelectionActive)
    {
        g.setColour (findColour (ThemeColours::componentBackground).contrasting().withAlpha (0.5f));
        g.drawRect (selectionBox);
    }
}

void ProbeBrowser::drawAnnotations (Graphics& g)
{
    for (int i = 0; i < parent->annotations.size(); i++)
    {
        bool shouldAppear = false;

        Annotation& a = parent->annotations.getReference (i);

        for (int j = 0; j < a.electrodes.size(); j++)
        {
            if (j > lowestElectrode || j < highestElectrode)
            {
                shouldAppear = true;
                break;
            }
        }

        if (shouldAppear)
        {
            float xLoc = 225 + 30;
            int ch = a.electrodes[0];

            float midpoint = lowerBound / 2 + 8;

            float yLoc = lowerBound - ((ch - lowestElectrode - (ch % 2)) / 2 * electrodeHeight) + 10;

            //if (abs(yLoc - midpoint) < 200)
            yLoc = (midpoint + 3 * yLoc) / 4;
            a.currentYLoc = yLoc;

            float alpha;

            if (yLoc > lowerBound - 250)
                alpha = (lowerBound - yLoc) / (250.f);
            else if (yLoc < 250)
                alpha = 1.0f - (250.f - yLoc) / 200.f;
            else
                alpha = 1.0f;

            if (alpha < 0)
                alpha = -alpha;

            if (alpha < 0)
                alpha = 0;

            if (alpha > 1.0f)
                alpha = 1.0f;

            //float distFromMidpoint = yLoc - midpoint;
            //float ratioFromMidpoint = pow(distFromMidpoint / midpoint,1);

            //if (a.isMouseOver)
            //  g.setColour(Colours::yellow.withAlpha(alpha));
            //else
            g.setColour (a.colour.withAlpha (alpha));

            g.drawMultiLineText (a.text, xLoc + 2, yLoc, 150);

            float xLoc2 = 225 - electrodeHeight * (1 - (ch % 2)) + electrodeHeight / 2;
            float yLoc2 = lowerBound - ((ch - lowestElectrode - (ch % 2)) / 2 * electrodeHeight) + electrodeHeight / 2;

            g.drawLine (xLoc - 5, yLoc - 3, xLoc2, yLoc2);
            g.drawLine (xLoc - 5, yLoc - 3, xLoc, yLoc - 3);
        }
    }
}

Colour ProbeBrowser::getElectrodeColour (int i)
{
    if (parent->electrodeMetadata[i].status == ElectrodeStatus::DISCONNECTED) // not available
    {
        //std::cout << "Electrode " << i << " : " << (int) parent->electrodeMetadata[i].bank << " is disconnected" << std::endl;

        return disconnectedColours[parent->electrodeMetadata[i].bank];
        //return Colours::grey;
    }
    else if (parent->electrodeMetadata[i].type == ElectrodeType::REFERENCE)
        return Colours::black;
    else
    {
        if (parent->mode == VisualizationMode::ENABLE_VIEW) // ENABLED STATE
        {
            return Colours::yellow;
        }
        else if (parent->mode == VisualizationMode::AP_GAIN_VIEW) // AP GAIN
        {
            return Colour (25 * parent->probe->settings.apGainIndex, 25 * parent->probe->settings.apGainIndex, 50);
        }
        else if (parent->mode == VisualizationMode::LFP_GAIN_VIEW) // LFP GAIN
        {
            return Colour (66, 25 * parent->probe->settings.lfpGainIndex, 35 * parent->probe->settings.lfpGainIndex);
        }
        else if (parent->mode == VisualizationMode::REFERENCE_VIEW)
        {
            if (parent->referenceComboBox != nullptr)
            {
                String referenceDescription = parent->referenceComboBox->getText();

                if (referenceDescription.contains ("Ext"))
                    return Colours::darksalmon;
                else if (referenceDescription.contains ("Tip"))
                    return Colours::orange;
                else
                    return Colours::purple;
            }
            else
            {
                return Colours::black;
            }
        }
        else if (parent->mode == VisualizationMode::ACTIVITY_VIEW)
        {
            if (parent->electrodeMetadata[i].status == ElectrodeStatus::CONNECTED)
            {
                return parent->electrodeMetadata.getReference (i).colour;
            }
            else
            {
                return Colours::grey;
            }
        }

        // Fallback colour for unexpected mode values
        return Colours::grey;
    }
}

void ProbeBrowser::timerCallback()
{
    if (parent->mode != VisualizationMode::ACTIVITY_VIEW)
        return;

    const float* peakToPeakValues = parent->probe->getPeakToPeakValues (activityToView);

    for (int i = 0; i < parent->electrodeMetadata.size(); i++)
    {
        if (parent->electrodeMetadata[i].status == ElectrodeStatus::CONNECTED)
        {
            int channelNumber = parent->electrodeMetadata[i].channel;

            if (parent->probe->type == ProbeType::QUAD_BASE)
                channelNumber += parent->electrodeMetadata[i].shank * 384;

            parent->electrodeMetadata.getReference (i).colour =
                ColourScheme::getColourForNormalizedValue (peakToPeakValues[channelNumber] / maxPeakToPeakAmplitude);
        }
    }

    repaint();
}

int ProbeBrowser::getZoomHeight()
{
    return zoomHeight;
}

int ProbeBrowser::getZoomOffset()
{
    return zoomOffset;
}

void ProbeBrowser::setZoomHeightAndOffset (int newHeight, int newOffset)
{
    // Clamp height to allowed range
    if (newHeight < minZoomHeight)
        newHeight = minZoomHeight;
    if (newHeight > maxZoomHeight)
        newHeight = maxZoomHeight;

    zoomHeight = newHeight;

    // Clamp offset to valid range given the new height
    const int maxOffset = jmax (0, lowerBound - zoomHeight - 15);
    if (newOffset < 0)
        newOffset = 0;
    if (newOffset > maxOffset)
        newOffset = maxOffset;

    zoomOffset = newOffset;
}
