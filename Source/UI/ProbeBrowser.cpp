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

#include <cstdlib>
#include <limits>

namespace
{
constexpr int TOP_BORDER = 33;
constexpr int SHANK_HEIGHT = 480;
constexpr int INTERSHANK_DISTANCE = 30;
} // namespace

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
    cursorType = MouseCursor::NormalCursor;

    activityToView = ActivityToView::APVIEW;

    ProbeType probeType = parent->probe->type;

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
        maxZoomHeight = columns >= 8 ? 100 : 60;
        minZoomHeight = columns >= 8 ? 20 : 10;
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

    shankOffset = INTERSHANK_DISTANCE * (parent->probeMetadata.shank_count - 1);

    // Initialize with default black for undefined/disabled banks
    disconnectedColours[Bank::NONE] = Colours::black;
    disconnectedColours[Bank::OFF] = Colours::black;

    // Group bank colours
    const Colour groupAColour (180, 180, 180); // lighter grey
    const Colour groupBColour (155, 155, 155); // medium-light grey
    // const Colour groupCColour (140, 140, 140); // medium grey
    // const Colour groupDColour (120, 120, 120); // darker grey

    auto setGroupColours = [this] (Colour c, std::initializer_list<Bank> banks)
    {
        for (auto b : banks)
            disconnectedColours[b] = c;
    };

    setGroupColours (groupAColour, { Bank::A, Bank::A1, Bank::A2, Bank::A3, Bank::A4, Bank::E, Bank::I, Bank::M });
    setGroupColours (groupBColour, { Bank::B, Bank::B1, Bank::B2, Bank::B3, Bank::B4, Bank::F, Bank::J });
    setGroupColours (groupAColour, { Bank::C, Bank::C1, Bank::C2, Bank::C3, Bank::C4, Bank::G, Bank::K });
    setGroupColours (groupBColour, { Bank::D, Bank::D1, Bank::D2, Bank::D3, Bank::D4, Bank::H, Bank::L });

    dragZoneWidth = 10;

    overviewElectrodeColours.insertMultiple (0, Colour (160, 160, 160), parent->electrodeMetadata.size());
}

void ProbeBrowser::setDisplayMode (DisplayMode mode)
{
    if (displayMode == mode)
        return;

    displayMode = mode;

    const bool interactive = (displayMode == DisplayMode::Interactive);
    setInterceptsMouseClicks (interactive, interactive);
    setWantsKeyboardFocus (interactive);

    if (! interactive)
        cursorType = MouseCursor::NormalCursor;

    repaint();
}

ProbeBrowser::~ProbeBrowser()
{
}

String ProbeBrowser::getTooltip()
{
    if (hoveredElectrodeIndex >= 0 && hoveredElectrodeIndex < parent->electrodeMetadata.size())
        return getElectrodeInfoString (hoveredElectrodeIndex);

    return {};
}

void ProbeBrowser::mouseMove (const MouseEvent& event)
{
    if (displayMode == DisplayMode::OverviewOnly)
    {
        cursorType = MouseCursor::NormalCursor;
        return;
    }

    const float y = event.y;
    const float x = event.x;

    const bool inZoomY = (y > lowerBound - zoomOffset - zoomHeight - dragZoneWidth / 2)
                         && (y < lowerBound - zoomOffset + dragZoneWidth / 2);
    const bool inZoomX = (x > 49) && (x < 94 + shankOffset);
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

        if (index > -1 && index != hoveredElectrodeIndex)
        {
            hoveredElectrodeIndex = index;
            isOverElectrode = true;
        }
    }
    else if (isOverElectrode)
    {
        hoveredElectrodeIndex = -1;
        isOverElectrode = false;
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
        const float shankLeftEdge = 260 + shankOffset - totalWidth / 2 + shankWidth * 2 * shank;
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
    int startrow = static_cast<int> (std::ceil (((lowerBound + 15 - electrodeHeight - (y + h)) / electrodeHeight) + zoomAreaMinRow + 1));
    int endrow = static_cast<int> (std::floor (((lowerBound + 15 - electrodeHeight - y) / electrodeHeight) + zoomAreaMinRow));

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

    //int startcolumn = (x - (260 + shankOffset - electrodeHeight * probeMetadata.columns_per_shank / 2)) / electrodeHeight;
    //int endcolumn = (x + w - (260 + shankOffset - electrodeHeight * probeMetadata.columns_per_shank / 2)) / electrodeHeight;

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

int ProbeBrowser::findElectrodeIndexForRow (int row) const
{
    if (parent == nullptr || parent->electrodeMetadata.isEmpty())
        return -1;

    int bestIndex = -1;
    int bestDiff = std::numeric_limits<int>::max();

    for (int i = 0; i < parent->electrodeMetadata.size(); ++i)
    {
        const int diff = std::abs (parent->electrodeMetadata[i].row_index - row);
        if (diff < bestDiff)
        {
            bestDiff = diff;
            bestIndex = i;
            if (diff == 0)
                break;
        }
    }

    return bestIndex;
}

String ProbeBrowser::getElectrodeInfoString (int index)
{
    String a;
    a << "Electrode " << String (parent->electrodeMetadata[index].global_index);
    a << "\nBank " << bankToString (parent->electrodeMetadata[index].bank);

    a << ", Channel " << String (parent->electrodeMetadata[index].channel);

    a << "\nY Position: " << String (parent->electrodeMetadata[index].ypos);

    // a << "\nType: ";

    // if (parent->electrodeMetadata[index].type == ElectrodeType::REFERENCE)
    // {
    //     a << "REFERENCE";
    // }
    // else
    // {
    //     a << "SIGNAL";
    //     a << "\nEnabled: ";

    //     if (parent->electrodeMetadata[index].status == ElectrodeStatus::CONNECTED)
    //         a << "YES";
    //     else
    //         a << "NO";
    // }

    // if (parent->apGainComboBox != nullptr)
    // {
    //     a << "\nAP Gain: " << String (parent->apGainComboBox->getText());
    // }

    // if (parent->lfpGainComboBox != nullptr)
    // {
    //     a << "\nLFP Gain: " << String (parent->lfpGainComboBox->getText());
    // }

    // if (parent->referenceComboBox != nullptr)
    // {
    //     a << "\nReference: " << String (parent->referenceComboBox->getText());
    // }

    return a;
}

void ProbeBrowser::mouseUp (const MouseEvent& event)
{
    if (displayMode == DisplayMode::OverviewOnly)
        return;

    if (isSelectionActive)
    {
        isSelectionActive = false;
        repaint();
    }
}

void ProbeBrowser::mouseDown (const MouseEvent& event)
{
    if (displayMode == DisplayMode::OverviewOnly)
        return;

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
    if (displayMode == DisplayMode::OverviewOnly)
        return;

    if (event.x > 190 && event.x < 440)
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
    if (displayMode == DisplayMode::OverviewOnly)
        return;

    if (event.x > 265 + 10 && event.x < 265 + 150)
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
    if (displayMode == DisplayMode::OverviewOnly)
        return;

    if (isOverZoomRegion)
    {
        handleZoomDrag (event);
    }
    else if (event.x > 190 && event.x < 490)
    {
        handleSelectionDrag (event);
    }

    clampZoomValues();
    repaint();
}

void ProbeBrowser::handleZoomDrag (const MouseEvent& event)
{
    if (displayMode == DisplayMode::OverviewOnly)
        return;

    int yDist = event.getDistanceFromDragStartY();

    if (isOverUpperBorder)
    {
        zoomHeight = initialHeight - yDist;
        if (zoomHeight > lowerBound - zoomOffset - 16)
            zoomHeight = lowerBound - zoomOffset - 16;
    }
    else if (isOverLowerBorder)
    {
        zoomOffset = initialOffset - yDist;
        if (zoomOffset < 0)
        {
            zoomOffset = 0;
        }
        else
        {
            zoomHeight = initialHeight + yDist;
        }
    }
    else
    {
        zoomOffset = initialOffset - yDist;
        if (zoomOffset < 0)
        {
            zoomOffset = 0;
        }
    }
}

void ProbeBrowser::handleSelectionDrag (const MouseEvent& event)
{
    if (displayMode == DisplayMode::OverviewOnly)
        return;

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

    if (zoomOffset > lowerBound - zoomHeight - 16)
        zoomOffset = lowerBound - zoomHeight - 16;
    else if (zoomOffset < 0)
        zoomOffset = 0;
}

void ProbeBrowser::mouseWheelMove (const MouseEvent& event, const MouseWheelDetails& wheel)
{
    if (displayMode == DisplayMode::OverviewOnly)
        return;

    if (event.x > 140 && event.x < 490)
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
    if (displayMode == DisplayMode::OverviewOnly)
        return MouseCursor (MouseCursor::NormalCursor);

    return MouseCursor (cursorType);
}

void ProbeBrowser::paintOverview (Graphics& g)
{
    if (parent == nullptr || parent->probe == nullptr || parent->electrodeMetadata.isEmpty())
        return;

    const Rectangle<float> outerBounds = getLocalBounds().toFloat();
    auto panelColour = findColour (ThemeColours::componentBackground);
    g.setColour (panelColour);
    g.fillRoundedRectangle (outerBounds, 6.0f);

    const float padding = 16.0f;
    Rectangle<float> content = outerBounds.reduced (padding);
    if (content.getWidth() <= 0.0f || content.getHeight() <= 0.0f)
        return;

    const float verticalPadding = 20.0f;
    constexpr float shankGap = 20.0f;
    constexpr float electrodePixelWidth = 20.0f;

    const float axisTrimWidth = 70.0f;
    const float bankTrimWidth = 70.0f;

    Rectangle<float> electrodeArea = content.withTrimmedLeft (axisTrimWidth)
                                         .withTrimmedRight (bankTrimWidth)
                                         .withTrimmedTop (verticalPadding)
                                         .withTrimmedBottom (verticalPadding);

    if (electrodeArea.getWidth() <= 0.0f || electrodeArea.getHeight() <= 0.0f)
        return;

    const int shankCount = jmax (1, parent->probeMetadata.shank_count);
    const int columns = jmax (1, parent->probeMetadata.columns_per_shank);
    const int rows = jmax (1, parent->probeMetadata.rows_per_shank);

    const float layoutShankWidth = columns * electrodePixelWidth;
    const float layoutWidth = shankCount * layoutShankWidth + jmax (0, shankCount - 1) * shankGap;

    float layoutLeft = electrodeArea.getX();
    if (layoutWidth < electrodeArea.getWidth())
        layoutLeft = electrodeArea.getX() + (electrodeArea.getWidth() - layoutWidth) * 0.5f;

    const float maxLayoutLeft = electrodeArea.getRight() - layoutWidth;
    if (layoutLeft > maxLayoutLeft)
        layoutLeft = maxLayoutLeft;
    layoutLeft = jmax (layoutLeft, electrodeArea.getX());

    Rectangle<float> layoutArea (layoutLeft, electrodeArea.getY(), layoutWidth, electrodeArea.getHeight());

    // // Probe background area
    // Rectangle<float> backgroundArea = layoutArea;
    // backgroundArea.expand (8.0f, 8.0f);
    // backgroundArea.setLeft (jmax (backgroundArea.getX(), electrodeArea.getX()));
    // backgroundArea.setRight (jmin (backgroundArea.getRight(), electrodeArea.getRight()));

    // // Probe background
    // g.setColour (findColour (ThemeColours::componentBackground));
    // g.fillRoundedRectangle (backgroundArea, 4.0f);

    const float electrodeHeight = layoutArea.getHeight() / rows;

    const float axisLabelWidth = 75.0f;
    const float axisLabelPadding = 8.0f;
    const float tickLength = 6.0f;
    const float leftTickStartX = layoutArea.getX() - tickLength;
    const float leftLabelX = leftTickStartX - (axisLabelWidth + axisLabelPadding);
    const float axisHeadingTop = layoutArea.getY() - 20.0f;

    const float rightLabelX = layoutArea.getRight() + tickLength + axisLabelPadding;
    const float bankLabelWidth = axisLabelWidth;
    const float bankMarkerX = rightLabelX + 30.0f;
    const float bankHeadingTop = axisHeadingTop;

    // Draw electrodes
    for (int i = 0; i < parent->electrodeMetadata.size(); ++i)
    {
        const auto& meta = parent->electrodeMetadata.getReference (i);
        const float x = layoutArea.getX()
                        + meta.shank * (layoutShankWidth + shankGap)
                        + meta.column_index * electrodePixelWidth;
        const float y = layoutArea.getBottom() - (meta.row_index + 1) * electrodeHeight;

        Rectangle<float> electrodeRect (x, y, electrodePixelWidth, electrodeHeight);
        if (electrodeRect.getWidth() > 1.3f && electrodeRect.getHeight() > 1.3f)
            electrodeRect = electrodeRect.reduced (0.2f, 0.2f);

        g.setColour (overviewElectrodeColours.getUnchecked (i));
        g.fillRect (electrodeRect);
    }

    // Shank outlines
    g.setColour (findColour (ThemeColours::outline));
    for (int shank = 0; shank < shankCount; ++shank)
    {
        const float x = layoutArea.getX() + shank * (layoutShankWidth + shankGap);
        Rectangle<float> shankRect (x, layoutArea.getY(), layoutShankWidth, layoutArea.getHeight());
        g.drawRoundedRectangle (shankRect, 3.0f, 1.0f);
    }

    // Axis labels
    g.setColour (findColour (ThemeColours::defaultText));
    g.setFont (FontOptions ("Inter", "Medium", 13.0f));
    g.drawText ("Y Pos (um)", leftLabelX, axisHeadingTop, axisLabelWidth, 16, Justification::centredRight);
    g.drawText ("Electrode", rightLabelX, bankHeadingTop, bankLabelWidth, 16, Justification::centredLeft);

    // Electrode labels
    g.setColour (findColour (ThemeColours::defaultText).withAlpha (0.75f));
    g.setFont (FontOptions (12.0f));
    const float labelYOffset = 6.0f;

    // Use same logic as paint() method - iterate through electrodes directly
    int ch = 0;
    for (int i = 0; i < parent->probeMetadata.electrodes_per_shank; i += channelLabelSkip)
    {
        if (i >= parent->electrodeMetadata.size())
            break;

        const auto& meta = parent->electrodeMetadata.getReference (i);
        const float y = layoutArea.getBottom() - meta.row_index * electrodeHeight;

        if (y < layoutArea.getY() || y > layoutArea.getBottom())
            continue;

        g.drawLine (leftTickStartX, y, leftTickStartX + tickLength, y, 1.0f);
        g.drawLine (layoutArea.getRight(), y, layoutArea.getRight() + tickLength, y, 1.0f);

        const float depth = meta.ypos;
        const int electrodeNumber = ch;
        g.drawText (String (depth), leftLabelX, (int) (y - labelYOffset), axisLabelWidth, 12, Justification::right);
        g.drawText (String (electrodeNumber), (int) rightLabelX, (int) (y - labelYOffset), (int) axisLabelWidth, 12, Justification::left);

        ch += channelLabelSkip;
    }

    if (parent->probeMetadata.availableBanks.size() < 2)
        return;

    // Bank labels
    g.setColour (findColour (ThemeColours::defaultText).withAlpha (0.5f));
    g.setFont (FontOptions (16.0f));
    int bankIndex = 0;
    for (auto bank : parent->probeMetadata.availableBanks)
    {
        if (bank < Bank::A || bank > Bank::M)
            continue;

        int minRow = std::numeric_limits<int>::max();
        int maxRow = std::numeric_limits<int>::min();

        for (int ei = 0; ei < parent->electrodeMetadata.size(); ++ei)
        {
            const auto& meta = parent->electrodeMetadata.getReference (ei);
            if (meta.bank != bank)
                continue;

            minRow = jmin (minRow, meta.row_index);
            maxRow = jmax (maxRow, meta.row_index);
        }

        if (minRow > maxRow)
            continue;

        const float topY = layoutArea.getBottom() - (maxRow + 1) * electrodeHeight;
        const float bottomY = layoutArea.getBottom() - minRow * electrodeHeight;

        // Draw bank marker line(s)
        if (bankIndex == 0)
            g.drawLine (leftTickStartX - 30.0f, bottomY, bankMarkerX, bottomY, 1.0f);

        g.drawLine (leftTickStartX - 30.0f, topY, bankMarkerX, topY, 1.0f);

        const float labelY = (topY + bottomY) * 0.5f - 8.0f;
        g.drawText (bankToString (bank), (int) bankMarkerX, (int) labelY, (int) bankLabelWidth, 16, Justification::left);

        bankIndex++;
    }
}

void ProbeBrowser::paint (Graphics& g)
{
    if (displayMode == DisplayMode::OverviewOnly)
    {
        paintOverview (g);
        return;
    }

    const int LEFT_BORDER = parent->probeMetadata.columns_per_shank >= 8 ? 63 : 70;

    // Draw zoomed-out channels
    int channelSpan = SHANK_HEIGHT;
    int pixelGap = (parent->probeMetadata.columns_per_shank > 8) ? 1 : 2;
    float miniRowHeight = float (channelSpan) / float (parent->probeMetadata.rows_per_shank);

    // Draw all electrodes in zoomed-out view
    for (int i = 0; i < parent->electrodeMetadata.size(); ++i)
    {
        g.setColour (getElectrodeColour (i));
        int col = parent->electrodeMetadata[i].column_index;
        int shank = parent->electrodeMetadata[i].shank;
        int row = parent->electrodeMetadata[i].row_index;

        for (int px = 0; px < pixelHeight; ++px)
        {
            int x = LEFT_BORDER + col * pixelGap + shank * INTERSHANK_DISTANCE;
            int y = TOP_BORDER + channelSpan
                    - int (float (row) * miniRowHeight)
                    - px;
            g.fillRect (x, y, 1, 1);
        }
    }

    // Draw channel numbers and tick marks
    g.setFont (FontOptions (12.0f));

    g.setColour (findColour (ThemeColours::defaultText).withAlpha (0.5f));
    g.drawText ("Y Pos (um)", 5.0f, 10.0f, 60.0f, 12.0f, Justification::right, false);
    g.drawText ("Electrode", 84.0f + shankOffset, 10.0f, 100.0f, 12.0f, Justification::left, false);

    g.setColour (findColour (ThemeColours::defaultText));

    int ch = 0;
    float chInterval = (float) SHANK_HEIGHT * channelLabelSkip / parent->probeMetadata.electrodes_per_shank;

    for (float i = TOP_BORDER + channelSpan; i > TOP_BORDER; i -= chInterval)
    {
        int eid = ch == 0 ? ch : ch + 1;
        float depth = parent->electrodeMetadata[eid].ypos;
        g.drawText (String (depth), 6.0f, i - 6.0f, 35.0f, 12.0f, Justification::right, false);
        g.drawLine (46.0f, i, 58.0f, i);
        g.drawLine (84.0f + shankOffset, i, 94.0f + shankOffset, i);
        g.drawText (String (ch), 99.0f + shankOffset, i - 6.0f, 100.0f, 12.0f, Justification::left, false);
        ch += channelLabelSkip;
    }

    // Draw top channel tick and label
    g.drawText (String (parent->electrodeMetadata.getLast().ypos), 6.0f, TOP_BORDER - 6.0f, 35.0f, 12.0f, Justification::right, false);
    g.drawLine (46.0f, TOP_BORDER, 58.0f, TOP_BORDER);
    g.drawLine (84.0f + shankOffset, TOP_BORDER, 94.0f + shankOffset, TOP_BORDER);
    g.drawText (String (parent->probeMetadata.electrodes_per_shank), 99.0f + shankOffset, TOP_BORDER - 6.0f, 100.0f, 12.0f, Justification::left, false);

    // Draw shank outlines
    g.setColour (findColour (ThemeColours::outline).withAlpha (0.75f));
    for (int i = 0; i < parent->probeMetadata.shank_count; ++i)
    {
        Path outline = parent->probeMetadata.shankOutline;
        outline.applyTransform (AffineTransform::translation (INTERSHANK_DISTANCE * i, 0.0f));
        g.strokePath (outline, PathStrokeType (1.0));
    }

    // Calculate zoomed area parameters
    float lowestRow = (zoomOffset - 16) / miniRowHeight;
    float highestRow = lowestRow + (zoomHeight / miniRowHeight);
    zoomAreaMinRow = static_cast<int> (std::ceil (lowestRow));
    float numVisibleRows = highestRow - lowestRow;

    if (parent->probeMetadata.columns_per_shank > 8)
        electrodeHeight = jmin (lowerBound / numVisibleRows, 12.0f);
    else
        electrodeHeight = lowerBound / numVisibleRows;

    highestRow = zoomAreaMinRow + (zoomHeight / miniRowHeight);

    // Draw zoomed-in electrodes
    for (int i = 0; i < parent->electrodeMetadata.size(); ++i)
    {
        int row = parent->electrodeMetadata[i].row_index;
        if (row >= static_cast<int> (std::ceil (lowestRow)) && row < static_cast<int> (std::floor (highestRow)))
        {
            int col = parent->electrodeMetadata[i].column_index;
            int shank = parent->electrodeMetadata[i].shank;
            float xLoc = 260 + shankOffset - electrodeHeight * parent->probeMetadata.columns_per_shank / 2
                         + electrodeHeight * col + shank * electrodeHeight * 4
                         - (parent->probeMetadata.shank_count / 2 * electrodeHeight * 3);
            float yLoc = lowerBound - ((row - static_cast<int> (std::ceil (lowestRow))) * electrodeHeight)
                         + 15 - electrodeHeight;

            if (parent->electrodeMetadata[i].isSelected)
            {
                g.setColour (findColour (ThemeColours::componentBackground).contrasting());
                g.drawRect (xLoc, yLoc, electrodeHeight, electrodeHeight);
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
    upperBorder.startNewSubPath (45, lowerBound - zoomOffset - zoomHeight - 1);
    upperBorder.lineTo (94 + shankOffset, lowerBound - zoomOffset - zoomHeight - 1);
    upperBorder.lineTo (140 + shankOffset, 16);
    upperBorder.lineTo (370 + shankOffset, 16);

    Path lowerBorder;
    lowerBorder.startNewSubPath (45, lowerBound - zoomOffset - 1);
    lowerBorder.lineTo (94 + shankOffset, lowerBound - zoomOffset - 1);
    lowerBorder.lineTo (140 + shankOffset, lowerBound + 16);
    lowerBorder.lineTo (370 + shankOffset, lowerBound + 16);

    g.strokePath (upperBorder, PathStrokeType (2.0));
    g.strokePath (lowerBorder, PathStrokeType (2.0));

    // Calculate selection area edges
    float shankWidth = electrodeHeight * parent->probeMetadata.columns_per_shank;
    float totalWidth = shankWidth * parent->probeMetadata.shank_count
                       + shankWidth * (parent->probeMetadata.shank_count - 1);

    leftEdge = 260 + shankOffset - totalWidth / 2;
    rightEdge = 260 + shankOffset + totalWidth / 2;

    // Draw bank ticks/labels in the zoom area: compute each bank's bottom visible row
    const int bankCount = parent->probeMetadata.availableBanks.size();
    if (bankCount > 1)
    {
        g.setColour (findColour (ThemeColours::defaultText));
        g.setFont (FontOptions (15.0f));

        for (int bi = 0; bi < bankCount; ++bi)
        {
            Bank b = parent->probeMetadata.availableBanks[bi];
            if (b < Bank::A || b > Bank::M)
                continue;

            int minRow = std::numeric_limits<int>::max();
            int maxRow = std::numeric_limits<int>::min();

            for (int ei = 0; ei < parent->electrodeMetadata.size(); ++ei)
            {
                if (parent->electrodeMetadata[ei].bank == b)
                {
                    minRow = jmin (minRow, parent->electrodeMetadata[ei].row_index);
                    maxRow = jmax (maxRow, parent->electrodeMetadata[ei].row_index);
                }
            }

            if (minRow > maxRow)
                continue; // no electrodes for this bank

            // If the bank's rows fall within the current zoom window, draw a tick at the bank's lowest visible row
            int visibleLowestRow = jmax (minRow, zoomAreaMinRow);

            // Map the visibleLowestRow to zoomed-in Y coordinate
            float y = lowerBound - ((visibleLowestRow - zoomAreaMinRow) * electrodeHeight) + 15;

            if (visibleLowestRow > maxRow || y < 16)
                continue; // bank not visible in this zoom

            // Draw small horizontal tick and label near the zoomed-in electrodes
            g.drawLine (leftEdge - electrodeHeight, y, rightEdge + electrodeHeight, y);
            g.drawText (bankToString (b), leftEdge - electrodeHeight - 25, (int) y - 8, 15, 16, Justification::left, false);
        }
    }

    // Draw depth ticks/labels on a dynamic grid size (µm) depending on zoom level.
    // Only draw ticks whose mapped Y falls inside the current zoom area.
    // Compute global min/max ypos across the entire probe (all electrodes)
    float globalMinYpos = parent->electrodeMetadata.getFirst().ypos;
    float globalMaxYpos = parent->electrodeMetadata.getLast().ypos;

    if (globalMinYpos <= globalMaxYpos)
    {
        // Total number of rows on shank (used to convert normalized depth -> row index)
        const float totalRows = static_cast<float> (parent->probeMetadata.rows_per_shank);

        // Determine grid spacing (start at 100 µm, increase if pixels per tick are too small)
        const float probeDepth = globalMaxYpos - globalMinYpos;

        // pixels covered by a 100 µm step
        float pixelsPer100 = 0.0f;
        if (probeDepth > 0.0f)
            pixelsPer100 = (100.0f / probeDepth) * (totalRows - 1.0f) * electrodeHeight;

        // choose grid size (µm) to keep at least minPixelSpacing between ticks
        const float minPixelSpacing = 40.0f; // minimum pixels between ticks to avoid clutter
        int grid = 100;
        const int maxGrid = 500;
        while (pixelsPer100 * (grid / 100.0f) < minPixelSpacing && grid < maxGrid)
            grid *= 2;

        // Range of ticks (round to 'grid' µm)
        int tickStart = static_cast<int> (std::floor (globalMinYpos / (float) grid) * grid);
        int tickEnd = static_cast<int> (std::ceil (globalMaxYpos / (float) grid) * grid);

        // use lower alpha so ticks are less visually dominant
        g.setColour (findColour (ThemeColours::defaultText).withAlpha (0.40f));
        g.setFont (FontOptions (12.0f));

        const float tickLength = jmin (12.0f, electrodeHeight);
        const float tickXStart = leftEdge - tickLength - 2.0f; // starting X for ticks just left of the electrode area

        for (int depth = tickStart; depth <= tickEnd; depth += grid)
        {
            if (depth == 0)
                continue;

            // normalized position along probe (0..1)
            float t = (globalMaxYpos == globalMinYpos)
                          ? 0.0f
                          : (float) (depth - globalMinYpos) / (globalMaxYpos - globalMinYpos);

            // convert to a fractional row (0..totalRows)
            float rowFloat = t * (totalRows - 1.0f);

            // Map fractional row into the zoomed pixel coordinate system used for electrodes
            float y = lowerBound - ((rowFloat - zoomAreaMinRow) * electrodeHeight) + 15;

            // Only draw if inside visible region
            if (y < 16 || y > lowerBound)
                continue;

            // draw a short tick to the left of the electrode area
            g.drawLine (tickXStart, y, tickXStart + tickLength, y);

            // draw the label to the left of the tick, right-justified
            String label = String (depth) + " µm";
            int labelWidth = 64;
            int labelX = static_cast<int> (tickXStart - labelWidth - 2.0f);
            g.drawText (label, labelX, (int) y - 8, labelWidth, 16, Justification::right, false);
        }
    }

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
    if (parent->mode == VisualizationMode::ACTIVITY_VIEW)
    {
        if (parent->electrodeMetadata[i].status == ElectrodeStatus::CONNECTED)
        {
            return parent->electrodeMetadata.getReference (i).colour;
        }
        else
        {
            if (CoreServices::getAcquisitionStatus())
                return Colour (160, 160, 160);
            else
                return parent->electrodeMetadata.getReference (i).colour.withAlpha (0.4f);
        }
    }
    else if (parent->electrodeMetadata[i].status == ElectrodeStatus::DISCONNECTED) // not available
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
            if (parent->electrodeMetadata[i].shank_is_programmable)
            {
                return Colours::yellow;
            }
            else
            {
                return Colours::salmon;
            }
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

        // Fallback colour for unexpected mode values
        return Colour (160, 160, 160);
    }
}

void ProbeBrowser::calculateElectrodeColours()
{
    const float* peakToPeakValues = parent->probe->getPeakToPeakValues (activityToView);

    if (peakToPeakValues == nullptr)
        return;

    const int electrodeCount = parent->electrodeMetadata.size();

    for (int i = 0; i < electrodeCount; i++)
    {
        const int electrodeIdx = parent->electrodeMetadata[i].global_index;
        const float value = peakToPeakValues[electrodeIdx];

        if (value < 0.0f)
        {
            overviewElectrodeColours.setUnchecked (i, Colour (160, 160, 160));
            parent->electrodeMetadata.getReference (i).colour = Colour (160, 160, 160);
        }
        else
        {
            overviewElectrodeColours.setUnchecked (i, ColourScheme::getColourForNormalizedValue (value / overviewMaxPeakToPeakAmplitude));
            parent->electrodeMetadata.getReference (i).colour = ColourScheme::getColourForNormalizedValue (value / parent->getMaxPeakToPeakValue());
        }
    }

    repaint();
}

void ProbeBrowser::timerCallback()
{
    // Skip for individual probe browser in activity view if not visible
    if (displayMode != DisplayMode::OverviewOnly && (parent->mode != VisualizationMode::ACTIVITY_VIEW || ! isShowing()))
        return;

    // Skip for survey interface probe browser if not visible
    if (displayMode == DisplayMode::OverviewOnly && ! isShowing())
        return;

    calculateElectrodeColours();
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

void ProbeBrowser::setMaxPeakToPeakAmplitude (float amp)
{
    if (displayMode == DisplayMode::OverviewOnly)
        overviewMaxPeakToPeakAmplitude = jmax (amp, 1.0f);

    if (! isTimerRunning())
        calculateElectrodeColours();
}
