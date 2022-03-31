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

#include <stdio.h>
#include "ProbeNameConfig.h"

ProbeNameEditor::ProbeNameEditor(ProbeNameConfig* p, int slot_, int port_, int dock_)
{
    slot = slot_;
    port = port_;
    dock = dock_;

    autoName    = "<>";
    autoNumber  = "<>";
    customPort  = "<>";
    customProbe = "<>";

    hasProbe = false;

}

void SelectionButton::paintButton(Graphics& g, bool isMouseOver, bool isButtonDown)
{
    int x = getScreenX();
    int y = getScreenY();
    int width = getWidth();
    int height = getHeight();

    int padding = 0.3 * height;
    g.setColour(Colour(255, 255, 255));
    Path triangle;

    /* Draw triangle in the correct direction */
    if (isPrev)
        triangle.addTriangle(padding, height / 2, width/2, padding, width/2, height - padding);
    else
        triangle.addTriangle(width / 2, padding, width / 2, height - padding, width - padding, height / 2);
    g.fillPath(triangle);
}

void SelectionButton::mouseUp(const MouseEvent& event)
{
    if (isPrev)
        p->showPrevScheme();
    else
        p->showNextScheme();
}

ProbeNameConfig::ProbeNameConfig(NeuropixThread* t_, int slot, int schemeIdx_)
{

    t = t_;
    schemeIdx = schemeIdx_;

    int width   = 240;
    int height  = 1.5*width;

    setSize(width, height);

    titleLabel = new Label("Probe Naming Scheme", "Probe Naming Scheme");
    titleLabel->setJustificationType(Justification::centred);
    titleLabel->setBounds(0, 0, width, 40);
    titleLabel->setFont(Font("Large Text", 20.0f, Font::plain));
    titleLabel->setColour(juce::Label::textColourId, juce::Colour(255,255,255));
    addAndMakeVisible(titleLabel);

    prevButton = new SelectionButton(this, true);
    prevButton->setBounds(0, 42, 40, 40);
    addAndMakeVisible(prevButton);

    nextButton = new SelectionButton(this, false);
    nextButton->setBounds(width - 40, 42, 40, 40);
    addAndMakeVisible(nextButton);

    schemeLabel = new Label("Active Scheme", schemes[schemeIdx]);
    schemeLabel->setJustificationType(Justification::centred);
    schemeLabel->setBounds(40, 42, width - 80, 40);
    schemeLabel->setFont(Font("Large Text", 20.0f, Font::plain));
    schemeLabel->setColour(juce::Label::textColourId, juce::Colour(255, 255, 255));
    addAndMakeVisible(schemeLabel);

    description = new Label("Scheme description", descriptions[schemeIdx]);
    description->setJustificationType(Justification::topLeft);
    description->setBounds(0, 82, width+2, 150);
    description->setFont(Font("Small Text", 12.0f, Font::plain));
    description->setColour(juce::Label::textColourId, juce::Colour(255, 255, 255));
    addAndMakeVisible(description);

    int padding = 9;
    width = (width - 3 * padding) / 2;
    height = height / 8 - int(5.0f * padding / 4);

    int x, y;
    for (int port = 4; port > 0; port--)
    {
        for (int dock = 1; dock <= 2; dock++)
        {
            x = padding + (dock - 1) * (padding + width);
            y = getHeight() - (port) * (padding + height);

            probeNames.push_back(std::move(std::make_unique<ProbeNameEditor>(this, slot, port, dock)));
            probeNames.back()->setBounds(x, y, width, height);
            probeNames.back()->setJustification(Justification::centred);
            probeNames.back()->setText("<EMPTY>");
            addAndMakeVisible(probeNames.back().get());
        }
    }

    dock1Label = new Label("dock1Label", "Dock 1");
    dock1Label->setJustificationType(Justification::centred);
    dock1Label->setBounds(x - (padding + width), getHeight() - 5*(padding + height), width, 1.5*height);
    dock1Label->setFont(Font("Small Text", 14.0f, Font::plain));
    dock1Label->setColour(juce::Label::textColourId, juce::Colour(255, 255, 255));
    addAndMakeVisible(dock1Label);

    dock2Label = new Label("dock2Label", "Dock 2");
    dock2Label->setJustificationType(Justification::centred);
    dock2Label->setBounds(x, getHeight() - 5 * (padding + height), width, 1.5*height);
    dock2Label->setFont(Font("Small Text", 14.0f, Font::plain));
    dock2Label->setColour(juce::Label::textColourId, juce::Colour(255, 255, 255));
    addAndMakeVisible(dock2Label);

    int probeIdx = 0;
    for (auto& probe : t->getProbes()) {
        for (auto&& label : probeNames)
        {

            if (label->slot == probe->basestation->slot
                && label->port == probe->headstage->port
                && label->dock == probe->dock)
            {
                label->autoName = probe->autoName;
                label->autoNumber = probe->autoNumber;
                label->customPort = probe->customPort.isEmpty() ? "Port" + String(probe->basestation->slot) + "-" + String(probe->headstage->port) + "-" + String(probe->dock) : probe->customPort;
                label->customProbe = probe->customProbe.isEmpty() ? String(probe->info.serial_number) : probe->customProbe;

                label->hasProbe = true;

                probeIdx++;
            }
        }
    }
    update();

}

void ProbeNameConfig::update()
{

    schemeLabel->setText(schemes[schemeIdx], juce::NotificationType::sendNotificationAsync);
    description->setText(descriptions[schemeIdx], juce::NotificationType::sendNotificationAsync);
    
    for (auto&& label : probeNames)
    {
        label->setEnabled(false);
        label->setColour(juce::TextEditor::ColourIds::backgroundColourId, juce::Colour(150, 150, 150));

        if (label->hasProbe)
        {
            if (schemeIdx == 0)
            {
                label->setText(label->autoName);
                label->setColour(juce::TextEditor::ColourIds::backgroundColourId, juce::Colour(210, 210, 210));
            }
            else if (schemeIdx == 1)
            {
                label->setText(label->autoNumber);
                label->setColour(juce::TextEditor::ColourIds::backgroundColourId, juce::Colour(210, 210, 210));
            }
            else if (schemeIdx == 2)
            {
                label->setText(label->customPort);
                label->setEnabled(true);
                label->setColour(juce::TextEditor::ColourIds::backgroundColourId, juce::Colour(255, 255, 255));
            }
            else
            {
                label->setText(label->customProbe);
                label->setEnabled(true);
                label->setColour(juce::TextEditor::ColourIds::backgroundColourId, juce::Colour(255, 255, 255));
            }
        }
    }

}


void ProbeNameConfig::showPrevScheme()
{
    schemeIdx--;
    if (schemeIdx < 0) schemeIdx = 3;
    update();
}

void ProbeNameConfig::showNextScheme()
{
    schemeIdx++;
    if (schemeIdx > 3) schemeIdx = 0;
    update();
}

void ProbeNameConfig::saveStateToXml(XmlElement* xml)
{

    XmlElement* state = xml->createNewChildElement("PROBENAME");
    /*
    for (auto field : fields)
    {
        XmlElement* currentField = state->createNewChildElement(String(field->types[field->type]).toUpperCase());
        currentField->setAttribute("state", field->state);
        currentField->setAttribute("value", field->value);
    }
    */

}

/** Load settings. */
void ProbeNameConfig::loadStateFromXml(XmlElement* xml)
{

    for (auto* xmlNode : xml->getChildIterator())
    {
        if (xmlNode->hasTagName("FILENAMECONFIG"))
        {


        }

    }

}
