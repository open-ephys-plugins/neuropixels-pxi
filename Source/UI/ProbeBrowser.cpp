/*
------------------------------------------------------------------

This file is part of the Open Ephys GUI
Copyright (C) 2021 Allen Institute for Brain Science and Open Ephys

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

ProbeBrowser::ProbeBrowser(NeuropixInterface* parent_) : parent(parent_)
{

	cursorType = MouseCursor::NormalCursor;

    activityToView = ActivityToView::APVIEW;
    maxPeakToPeakAmplitude = 100.0f;

    isOverZoomRegion = false;
    isOverUpperBorder = false;
    isOverLowerBorder = false;
    isSelectionActive = false;
    isOverElectrode = false;

    // PROBE SPECIFIC DRAWING SETTINGS
    minZoomHeight = 40;
    maxZoomHeight = 120;
    pixelHeight = 1;
    int defaultZoomHeight = 100;

    shankPath = Path(parent->probe->shankOutline);

    if (parent->probeMetadata.columns_per_shank == 8)
    {
        maxZoomHeight = 450;
        minZoomHeight = 300;
        defaultZoomHeight = 400;
        pixelHeight = 10;
    }

    if (parent->probeMetadata.shank_count == 4)
    {
        maxZoomHeight = 250;
        minZoomHeight = 40;
        defaultZoomHeight = 100;
        pixelHeight = 1;
    }

    // ALSO CONFIGURE CHANNEL JUMP
    if (parent->probeMetadata.electrodes_per_shank < 500)
        channelLabelSkip = 50;
    else if (parent->probeMetadata.electrodes_per_shank >= 500
        && parent->probeMetadata.electrodes_per_shank < 1500)
        channelLabelSkip = 100;
    else if (parent->probeMetadata.electrodes_per_shank >= 1500
        && parent->probeMetadata.electrodes_per_shank < 3000)
        channelLabelSkip = 200;
    else
        channelLabelSkip = 500;


    addMouseListener(this, true);

    zoomHeight = defaultZoomHeight; // number of rows
    lowerBound = 530; // bottom of interface

    zoomOffset = 50;
    dragZoneWidth = 10;
}

ProbeBrowser::~ProbeBrowser()
{

		

}


void ProbeBrowser::mouseMove(const MouseEvent& event)
{
    float y = event.y;
    float x = event.x;

    //std::cout << x << " " << y << std::endl;

    bool isOverZoomRegionNew = false;
    bool isOverUpperBorderNew = false;
    bool isOverLowerBorderNew = false;

    // check for move into zoom region
    if ((y > lowerBound - zoomOffset - zoomHeight - dragZoneWidth / 2)
        && (y < lowerBound - zoomOffset + dragZoneWidth / 2)
        && (x > 9)
        && (x < 54 + shankOffset))
    {
        isOverZoomRegionNew = true;
    }
    else {
        isOverZoomRegionNew = false;
    }

    // check for move over upper border or lower border
    if (isOverZoomRegionNew)
    {
        if (y > lowerBound - zoomHeight - zoomOffset - dragZoneWidth / 2
            && y < lowerBound - zoomHeight - zoomOffset + dragZoneWidth / 2)
        {
            isOverUpperBorderNew = true;

        }
        else if (y > lowerBound - zoomOffset - dragZoneWidth / 2
            && y < lowerBound - zoomOffset + dragZoneWidth / 2)
        {
            isOverLowerBorderNew = true;

        }
        else {
            isOverUpperBorderNew = false;
            isOverLowerBorderNew = false;
        }
    }

    // update cursor type
    if (isOverZoomRegionNew != isOverZoomRegion ||
        isOverLowerBorderNew != isOverLowerBorder ||
        isOverUpperBorderNew != isOverUpperBorder)
    {
        isOverZoomRegion = isOverZoomRegionNew;
        isOverUpperBorder = isOverUpperBorderNew;
        isOverLowerBorder = isOverLowerBorderNew;

        if (!isOverZoomRegion)
        {
            cursorType = MouseCursor::NormalCursor;
        }
        else {

            if (isOverUpperBorder)
                cursorType = MouseCursor::TopEdgeResizeCursor;
            else if (isOverLowerBorder)
                cursorType = MouseCursor::BottomEdgeResizeCursor;
            else
                cursorType = MouseCursor::NormalCursor;
        }

        repaint();
    }

    // check for movement over electrode
    if ((x > leftEdge)  // in electrode selection region
        && (x < rightEdge)
        && (y < lowerBound)
        && (y > 18))
    {
        int index = getNearestElectrode(x, y);

        if (index > -1)
        {
            isOverElectrode = true;
            electrodeInfoString = getElectrodeInfoString(index);
        }

        repaint();
    }
    else {
        bool isOverChannelNew = false;

        if (isOverChannelNew != isOverElectrode)
        {
            isOverElectrode = isOverChannelNew;
            repaint();
        }
    }

}

int ProbeBrowser::getNearestElectrode(int x, int y)
{

    int row = (lowerBound - y) / electrodeHeight + zoomAreaMinRow + 1;

    int shankWidth = electrodeHeight * parent->probeMetadata.columns_per_shank;
    int totalWidth = shankWidth * parent->probeMetadata.shank_count + shankWidth * (parent->probeMetadata.shank_count - 1);

    int shank = 0;
    int column = -1;

    for (shank = 0; shank < parent->probeMetadata.shank_count; shank++)
    {
        int leftEdge = 220 + shankOffset - totalWidth / 2 + shankWidth * 2 * shank;
        int rightEdge = leftEdge + shankWidth;

        if (x >= leftEdge && x <= rightEdge)
        {
            column = (x - leftEdge) / electrodeHeight;
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

Array<int> ProbeBrowser::getElectrodesWithinBounds(int x, int y, int w, int h)
{
    int startrow = (lowerBound - y - h) / electrodeHeight + zoomAreaMinRow + 1;
    int endrow = (lowerBound - y) / electrodeHeight + zoomAreaMinRow + 1;

    int shankWidth = electrodeHeight * parent->probeMetadata.columns_per_shank;
    int totalWidth = shankWidth * parent->probeMetadata.shank_count + shankWidth * (parent->probeMetadata.shank_count - 1);

    Array<int> selectedColumns;

    for (int i = 0; i < parent->probeMetadata.shank_count * parent->probeMetadata.columns_per_shank; i++)
    {
        int shank = i / parent->probeMetadata.columns_per_shank;
        int column = i % parent->probeMetadata.columns_per_shank;

        int l = leftEdge + shankWidth * 2 * shank + electrodeHeight * column;
        int r = l + electrodeHeight / 2;

        if (x < l + electrodeHeight / 2 && x + w > r)
            selectedColumns.add(i);
    }

    //int startcolumn = (x - (220 + shankOffset - electrodeHeight * probeMetadata.columns_per_shank / 2)) / electrodeHeight;
    //int endcolumn = (x + w - (220 + shankOffset - electrodeHeight * probeMetadata.columns_per_shank / 2)) / electrodeHeight;

    Array<int> inds;

    //std::cout << startrow << " " << endrow << " " << startcolumn << " " << endcolumn << std::endl;

    for (int i = 0; i < parent->electrodeMetadata.size(); i++)
    {
        if ((parent->electrodeMetadata[i].row_index >= startrow) &&
            (parent->electrodeMetadata[i].row_index <= endrow))

        {
            int column_id = parent->electrodeMetadata[i].shank * parent->probeMetadata.columns_per_shank + parent->electrodeMetadata[i].column_index;

            if (selectedColumns.indexOf(column_id) > -1)
            {
                inds.add(i);
            }

        }
    }

    return inds;

}

String ProbeBrowser::getElectrodeInfoString(int index)
{
    String a;
    a += "Electrode ";
    a += String(parent->electrodeMetadata[index].global_index + 1);

    a += "\n\Bank ";

    switch (parent->electrodeMetadata[index].bank)
    {
    case Bank::A:
        a += "A";
        break;
    case Bank::B:
        a += "B";
        break;
    case Bank::C:
        a += "C";
        break;
    case Bank::D:
        a += "D";
        break;
    case Bank::E:
        a += "E";
        break;
    case Bank::F:
        a += "F";
        break;
    case Bank::G:
        a += "G";
        break;
    case Bank::H:
        a += "H";
        break;
    case Bank::I:
        a += "I";
        break;
    case Bank::J:
        a += "J";
        break;
    case Bank::K:
        a += "K";
        break;
    case Bank::L:
        a += "L";
        break;
    case Bank::M:
        a += "M";
        break;
    default:
        a += " NONE";
    }

    a += ", Channel ";
    a += String(parent->electrodeMetadata[index].channel + 1);

    a += "\n\nType: ";

    if (parent->electrodeMetadata[index].status == ElectrodeStatus::REFERENCE ||
        parent->electrodeMetadata[index].status == ElectrodeStatus::OPTIONAL_REFERENCE ||
        parent->electrodeMetadata[index].status == ElectrodeStatus::CONNECTED_OPTIONAL_REFERENCE)
    {
        a += "REFERENCE";
    }
    else
    {
        a += "SIGNAL";

        a += "\nEnabled: ";

        if (parent->electrodeMetadata[index].status == ElectrodeStatus::CONNECTED ||
            parent->electrodeMetadata[index].status == ElectrodeStatus::CONNECTED_OPTIONAL_REFERENCE)
            a += "YES";
        else
            a += "NO";
    }

    if (parent->apGainComboBox != nullptr)
    {
        a += "\nAP Gain: ";
        a += String(parent->apGainComboBox->getText());
    }

    if (parent->lfpGainComboBox != nullptr)
    {
        a += "\nLFP Gain: ";
        a += String(parent->lfpGainComboBox->getText());
    }

    if (parent->referenceComboBox != nullptr)
    {
        a += "\nReference: ";
        a += String(parent->referenceComboBox->getText());
    }

    return a;
}


void ProbeBrowser::mouseUp(const MouseEvent& event)
{
    if (isSelectionActive)
    {

        isSelectionActive = false;
        repaint();
    }

}

void ProbeBrowser::mouseDown(const MouseEvent& event)
{
    initialOffset = zoomOffset;
    initialHeight = zoomHeight;

    //std::cout << event.x << std::endl;

    if (!event.mods.isRightButtonDown())
    {
        if (event.x > 150 && event.x < 400)
        {

            if (!event.mods.isShiftDown())
            {
                for (int i = 0; i < parent->electrodeMetadata.size(); i++)
                    parent->electrodeMetadata.getReference(i).isSelected = false;
            }

            if (event.x > leftEdge && event.x < rightEdge)
            {
                int chan = getNearestElectrode(event.x, event.y);

                //std::cout << chan << std::endl;

                if (chan >= 0 && chan < parent->electrodeMetadata.size())
                {
                    parent->electrodeMetadata.getReference(chan).isSelected = true;
                }

            }
            repaint();
        }
    }
    else {

        if (event.x > 225 + 10 && event.x < 225 + 150)
        {
            int currentAnnotationNum;

            for (int i = 0; i < parent->annotations.size(); i++)
            {
                Annotation& a = parent->annotations.getReference(i);
                float yLoc = a.currentYLoc;

                if (float(event.y) < yLoc && float(event.y) > yLoc - 12)
                {
                    currentAnnotationNum = i;
                    break;
                }
                else {
                    currentAnnotationNum = -1;
                }
            }

            if (currentAnnotationNum > -1)
            {
                PopupMenu annotationMenu;

                annotationMenu.addItem(1, "Delete annotation", true);

                const int result = annotationMenu.show();

                switch (result)
                {
                case 0:
                    break;
                case 1:
                    parent->annotations.removeRange(currentAnnotationNum, 1);
                    repaint();
                    break;
                default:

                    break;
                }
            }

        }



        // if (event.x > 225 - channelHeight && event.x < 225 + channelHeight)
        // {
        //  PopupMenu annotationMenu;

     //        annotationMenu.addItem(1, "Add annotation", true);
        //  const int result = annotationMenu.show();

     //        switch (result)
     //        {
     //            case 1:
     //                std::cout << "Annotate!" << std::endl;
     //                break;
     //            default:
     //             break;
        //  }
        // }

    }


}

void ProbeBrowser::mouseDrag(const MouseEvent& event)
{
    if (isOverZoomRegion)
    {
        if (isOverUpperBorder)
        {
            zoomHeight = initialHeight - event.getDistanceFromDragStartY();

            if (zoomHeight > lowerBound - zoomOffset - 18)
                zoomHeight = lowerBound - zoomOffset - 18;
        }
        else if (isOverLowerBorder)
        {

            zoomOffset = initialOffset - event.getDistanceFromDragStartY();

            if (zoomOffset < 0)
            {
                zoomOffset = 0;
            }
            else {
                zoomHeight = initialHeight + event.getDistanceFromDragStartY();
            }

        }
        else {
            zoomOffset = initialOffset - event.getDistanceFromDragStartY();
        }
        //std::cout << zoomOffset << std::endl;
    }
    else if (event.x > 150 && event.x < 450)
    {
        int w = event.getDistanceFromDragStartX();
        int h = event.getDistanceFromDragStartY();
        int x = event.getMouseDownX();
        int y = event.getMouseDownY();

        if (w < 0)
        {
            x = x + w; w = -w;
        }

        if (h < 0)
        {
            y = y + h; h = -h;
        }

        //        selectionBox = Rectangle<int>(x, y, w, h);
        isSelectionActive = true;

        //if (x < 225)
        //{
        //int chanStart = getNearestRow(224, y + h) - 1;
       // int chanEnd = getNearestRow(224, y);

        Array<int> inBounds = getElectrodesWithinBounds(x, y, w, h);

        //std::cout << chanStart << " " << chanEnd << std::endl;

        if (x < rightEdge)
        {
            for (int i = 0; i < parent->electrodeMetadata.size(); i++)
            {
                if (inBounds.indexOf(i) > -1)
                {
                    parent->electrodeMetadata.getReference(i).isSelected = true;
                }
                else
                {
                    if (!event.mods.isShiftDown())
                        parent->electrodeMetadata.getReference(i).isSelected = false;
                }
            }
        }

        repaint();
    }

    if (zoomHeight < minZoomHeight)
        zoomHeight = minZoomHeight;
    if (zoomHeight > maxZoomHeight)
        zoomHeight = maxZoomHeight;

    if (zoomOffset > lowerBound - zoomHeight - 15)
        zoomOffset = lowerBound - zoomHeight - 15;
    else if (zoomOffset < 0)
        zoomOffset = 0;

    repaint();
}


MouseCursor ProbeBrowser::getMouseCursor()
{
    MouseCursor c = MouseCursor(cursorType);

    return c;
}

void ProbeBrowser::paint(Graphics& g)
{
    int LEFT_BORDER = 30;
    int TOP_BORDER = 33;
    int SHANK_HEIGHT = 480;
    int INTERSHANK_DISTANCE = 30;

    // draw zoomed-out channels channels

    for (int i = 0; i < parent->electrodeMetadata.size(); i++)
    {
        g.setColour(getElectrodeColour(i).withAlpha(0.5f));

        for (int px = 0; px < pixelHeight; px++)
            g.setPixel(LEFT_BORDER + parent->electrodeMetadata[i].column_index * 2 + parent->electrodeMetadata[i].shank * INTERSHANK_DISTANCE,
                TOP_BORDER + SHANK_HEIGHT - float(parent->electrodeMetadata[i].row_index) * float(SHANK_HEIGHT) / float(parent->probeMetadata.rows_per_shank) - px);
    }

    // channel 1 = pixel 513
    // channel 960 = pixel 33
    // 480 pixels for 960 channels

    // draw channel numbers

    g.setColour(Colours::grey);
    g.setFont(12);

    int ch = 0;

    int ch_interval = SHANK_HEIGHT * channelLabelSkip / parent->probeMetadata.electrodes_per_shank;

    // draw mark for every N channels

    //int WWW = 10;

    shankOffset = INTERSHANK_DISTANCE * (parent->probeMetadata.shank_count - 1); // +WWW;
    for (int i = TOP_BORDER + SHANK_HEIGHT; i > TOP_BORDER; i -= ch_interval)
    {
        g.drawLine(6, i, 18, i);
        g.drawLine(44 + shankOffset, i, 54 + shankOffset, i);
        g.drawText(String(ch), 59 + shankOffset, int(i) - 6, 100, 12, Justification::left, false);
        ch += channelLabelSkip;
    }

    // g.drawLine(220 + shankOffset, 0, 220 + shankOffset, getHeight());

     // draw top channel
    g.drawLine(6, TOP_BORDER, 18, TOP_BORDER);
    g.drawLine(44 + shankOffset, TOP_BORDER, 54 + shankOffset, TOP_BORDER);
    g.drawText(String(parent->probeMetadata.electrodes_per_shank),
        59 + shankOffset, TOP_BORDER - 6, 100, 12, Justification::left, false);

    // draw shank outline
    g.setColour(Colours::lightgrey);


    for (int i = 0; i < parent->probeMetadata.shank_count; i++)
    {
        Path shankPath = parent->probeMetadata.shankOutline;
        shankPath.applyTransform(AffineTransform::translation(INTERSHANK_DISTANCE * i, 0.0f));
        g.strokePath(shankPath, PathStrokeType(1.0));
    }

    // draw zoomed channels
    float miniRowHeight = float(SHANK_HEIGHT) / float(parent->probeMetadata.rows_per_shank); // pixels per row

    float lowestRow = (zoomOffset - 17) / miniRowHeight;
    float highestRow = lowestRow + (zoomHeight / miniRowHeight);

    zoomAreaMinRow = int(lowestRow);

    float totalHeight = float(lowerBound + 100);
    electrodeHeight = totalHeight / (highestRow - lowestRow);

    //std::cout << "Lowest row: " << lowestRow << ", highest row: " << highestRow << std::endl;
    //std::cout << "Zoom offset: " << zoomOffset << ", Zoom height: " << zoomHeight << std::endl;

    for (int i = 0; i < parent->electrodeMetadata.size(); i++)
    {
        if (parent->electrodeMetadata[i].row_index >= int(lowestRow)
            &&
            parent->electrodeMetadata[i].row_index < int(highestRow))
        {

            float xLoc = 220 + shankOffset - electrodeHeight * parent->probeMetadata.columns_per_shank / 2
                + electrodeHeight * parent->electrodeMetadata[i].column_index + parent->electrodeMetadata[i].shank * electrodeHeight * 4
                - (parent->probeMetadata.shank_count / 2 * electrodeHeight * 3);
            float yLoc = lowerBound - ((parent->electrodeMetadata[i].row_index - int(lowestRow)) * electrodeHeight);

            //std::cout << "Drawing electrode " << i << ", X: " << xLoc << ", Y:" << yLoc << std::endl;

            if (parent->electrodeMetadata[i].isSelected)
            {
                g.setColour(Colours::white);
                g.fillRect(xLoc, yLoc, electrodeHeight, electrodeHeight);
            }

            g.setColour(getElectrodeColour(i));

            g.fillRect(xLoc + 1, yLoc + 1, electrodeHeight - 2, electrodeHeight - 2);

        }

    }

    // draw annotations
    //drawAnnotations(g);

    // draw borders around zoom area

    g.setColour(Colours::darkgrey.withAlpha(0.7f));
    g.fillRect(25, 0, 25 + shankOffset, lowerBound - zoomOffset - zoomHeight);
    g.fillRect(25, lowerBound - zoomOffset, 25 + shankOffset, zoomOffset + 10);

    g.setColour(Colours::darkgrey);
    g.fillRect(100, 0, 250 + shankOffset, 20);
    g.fillRect(100, lowerBound + 14, 250 + shankOffset, 100);

    if (isOverZoomRegion)
        g.setColour(Colour(25, 25, 25));
    else
        g.setColour(Colour(55, 55, 55));

    Path upperBorder;
    upperBorder.startNewSubPath(5, lowerBound - zoomOffset - zoomHeight);
    upperBorder.lineTo(54 + shankOffset, lowerBound - zoomOffset - zoomHeight);
    upperBorder.lineTo(100 + shankOffset, 16);
    upperBorder.lineTo(330 + shankOffset, 16);

    Path lowerBorder;
    lowerBorder.startNewSubPath(5, lowerBound - zoomOffset);
    lowerBorder.lineTo(54 + shankOffset, lowerBound - zoomOffset);
    lowerBorder.lineTo(100 + shankOffset, lowerBound + 16);
    lowerBorder.lineTo(330 + shankOffset, lowerBound + 16);

    g.strokePath(upperBorder, PathStrokeType(2.0));
    g.strokePath(lowerBorder, PathStrokeType(2.0));

    // draw selection zone

    int shankWidth = electrodeHeight * parent->probeMetadata.columns_per_shank;
    int totalWidth = shankWidth * parent->probeMetadata.shank_count + shankWidth * (parent->probeMetadata.shank_count - 1);

    leftEdge = 220 + shankOffset - totalWidth / 2;
    rightEdge = 220 + shankOffset + totalWidth / 2;

    if (isSelectionActive)
    {
        g.setColour(Colours::white.withAlpha(0.5f));
        //g.drawRect(selectionBox);
    }

    if (isOverElectrode)
    {
        g.setColour(Colour(55, 55, 55));
        g.setFont(15);
        g.drawMultiLineText(electrodeInfoString,
            250 + shankOffset + 80, 350, 250);
    }
}



void ProbeBrowser::drawAnnotations(Graphics& g)
{
    for (int i = 0; i < parent->annotations.size(); i++)
    {
        bool shouldAppear = false;

        Annotation& a = parent->annotations.getReference(i);

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
            g.setColour(a.colour.withAlpha(alpha));

            g.drawMultiLineText(a.text, xLoc + 2, yLoc, 150);

            float xLoc2 = 225 - electrodeHeight * (1 - (ch % 2)) + electrodeHeight / 2;
            float yLoc2 = lowerBound - ((ch - lowestElectrode - (ch % 2)) / 2 * electrodeHeight) + electrodeHeight / 2;

            g.drawLine(xLoc - 5, yLoc - 3, xLoc2, yLoc2);
            g.drawLine(xLoc - 5, yLoc - 3, xLoc, yLoc - 3);
        }
    }
}


Colour ProbeBrowser::getElectrodeColour(int i)
{
    if (parent->electrodeMetadata[i].status == ElectrodeStatus::DISCONNECTED) // not available
    {
        return Colours::grey;
    }
    else if (parent->electrodeMetadata[i].status == ElectrodeStatus::REFERENCE)
        return Colours::black;
    else if (parent->electrodeMetadata[i].status == ElectrodeStatus::OPTIONAL_REFERENCE)
        return Colours::purple;
    else {
        if (parent->mode == VisualizationMode::ENABLE_VIEW) // ENABLED STATE
        {
            return Colours::yellow;
        }
        else if (parent->mode == VisualizationMode::AP_GAIN_VIEW) // AP GAIN
        {
            return  Colour(25 * parent->probe->settings.apGainIndex, 25 * parent->probe->settings.apGainIndex, 50);
        }
        else if (parent->mode == VisualizationMode::LFP_GAIN_VIEW) // LFP GAIN
        {
            return Colour(66, 25 * parent->probe->settings.lfpGainIndex, 35 * parent->probe->settings.lfpGainIndex);

        }
        else if (parent->mode == VisualizationMode::REFERENCE_VIEW)
        {
            if (parent->referenceComboBox != nullptr)
            {
                String referenceDescription = parent->referenceComboBox->getText();

                if (referenceDescription.contains("Ext"))
                    return Colours::pink;
                else if (referenceDescription.contains("Tip"))
                    return Colours::orange;
                else
                    return Colours::purple;
            }
            else {
                return Colours::black;
            }


        }
        else if (parent->mode == VisualizationMode::ACTIVITY_VIEW)
        {
            if (parent->electrodeMetadata[i].status == ElectrodeStatus::CONNECTED)
            {
                return parent->electrodeMetadata.getReference(i).colour;
            }
            else
            {
                return Colours::grey;
            }
        }
    }


}

void ProbeBrowser::timerCallback()
{

    if (parent->mode != VisualizationMode::ACTIVITY_VIEW)
        return;

    const float* peakToPeakValues = parent->probe->getPeakToPeakValues(activityToView);

    for (int i = 0; i < parent->electrodeMetadata.size(); i++)
    {

        if (parent->electrodeMetadata[i].status == ElectrodeStatus::CONNECTED)
        {
            int channelNumber = parent->electrodeMetadata[i].channel;

            parent->electrodeMetadata.getReference(i).colour =
                ColourScheme::getColourForNormalizedValue(peakToPeakValues[channelNumber] / maxPeakToPeakAmplitude);
        }
    }

    repaint();
}
