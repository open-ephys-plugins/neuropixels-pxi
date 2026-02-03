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

#include "XDAQADCInterface.h"

#include "../NeuropixThread.h"

static constexpr int NUM_CHANNELS = 8;

class XDAQAdcChannelButton : public ToggleButton
{
public:
    XDAQAdcChannelButton (int channel_) : channel (channel_) {}
    void setSelectedState (bool state)
    {
        selected = state;
        repaint();
    }
    int getChannelIndex() { return channel; }

private:
    void paintButton (Graphics& g, bool isMouseOver, bool isButtonDown) override
    {
        if (selected)
            g.setColour (Colours::white);
        else
            g.setColour (Colours::grey);

        g.fillEllipse (72, 0, 20, 20);

        Colour baseColour;
        String channelName = "ADC " + String (channel);
        String statusText = "ENABLED";

        g.setFont (20);

        if (findColour (ThemeColours::componentBackground) == Colour (225, 225, 225))
            baseColour = Colours::darkgreen;
        else
            baseColour = Colours::mediumspringgreen;

        if (isMouseOver || selected)
            baseColour = baseColour.brighter (1.0f);

        g.setColour (baseColour);
        g.drawText (channelName, 0, 0, 65, 20, Justification::right);
        g.fillEllipse (74, 2, 16, 16);
        g.drawText (statusText, 100, 0, 200, 20, Justification::left);
    }

    int channel;
    bool selected = false;
};

XDAQADCInterface::XDAQADCInterface (DataSource* dataSource_,
                                    NeuropixThread* thread_,
                                    NeuropixEditor* editor_,
                                    NeuropixCanvas* canvas_)
    : SettingsInterface (dataSource_, thread_, editor_, canvas_)
{
    type = SettingsInterface::XDAQ_SETTINGS_INTERFACE;

    for (int ch = 0; ch < NUM_CHANNELS; ch++)
    {
        auto button = new XDAQAdcChannelButton (ch);
        button->setBounds (25, 100 + 40 * ch, 350, 20);
        addAndMakeVisible (button);
        channels.add (button);
        button->addListener (this);

        if (ch == 0)
        {
            selectedChannel = button;
            button->setSelectedState (true);
        }
    }
}

XDAQADCInterface::~XDAQADCInterface() {}

void XDAQADCInterface::startAcquisition() {}

void XDAQADCInterface::stopAcquisition() {}

void XDAQADCInterface::saveParameters (XmlElement* xml) {}

void XDAQADCInterface::loadParameters (XmlElement* xml) {}

void XDAQADCInterface::buttonClicked (Button* button)
{
    for (auto channel : channels)
    {
        if (channel == button)
        {
            channel->setSelectedState (true);
            selectedChannel = channel;
        }
        else
        {
            channel->setSelectedState (false);
        }
        channel->repaint();
    }
    repaint();
}

void XDAQADCInterface::paint (Graphics& g)
{
    g.setColour (findColour (ThemeColours::defaultText));
    g.setFont (40);

    g.drawText ("XDAQ ADC Settings",
                20,
                10,
                500,
                45,
                Justification::left,
                false);

    g.setFont (15);
    g.drawText ("CHANNEL PARAMETERS:", 300, 250, 300, 18, Justification::left, false);
    g.drawText ("ADC input range:", 300, 190 - 20, 300, 18, Justification::left, false);
    g.drawText ("+/- 10 V", 300, 190, 120, 20, Justification::left, false);

    int infoY = 450;
    g.drawText ("API version: " + thread->getApiVersion(), 25, infoY, 400, 18, Justification::left, false);
    g.drawText ("Basestation", 25, infoY + 25, 400, 18, Justification::left, false);
    g.drawText ("  Firmware version: " + dataSource->basestation->info.boot_version, 25, infoY + 45, 400, 18, Justification::left, false);
    g.drawText ("  Part number: " + dataSource->basestation->info.part_number, 25, infoY + 65, 400, 18, Justification::left, false);

    g.drawRect (290, 240, 180, 50);

    g.drawLine (selectedChannel->getX() + 82, selectedChannel->getBottom(), selectedChannel->getX() + 82, selectedChannel->getBottom() + 5, 1.0f);
    g.drawLine (selectedChannel->getX() + 82, selectedChannel->getBottom() + 5, selectedChannel->getX() + 220, selectedChannel->getBottom() + 5, 1.0f);
    g.drawLine (selectedChannel->getX() + 220, selectedChannel->getBottom() + 5, 270, 265, 1.0f);
    g.drawLine (270, 265, 290, 265, 1.0f);
}
