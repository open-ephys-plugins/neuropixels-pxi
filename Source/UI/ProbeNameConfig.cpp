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

#include "../NeuropixThread.h"


ProbeNameEditor::ProbeNameEditor(ProbeNameConfig* p, int port_, int dock_)
{
    port = port_;
    dock = dock_;

    config = p;

    probe = nullptr;

    autoName    = "<>";
    autoNumber  = "<>";
    customPort  = "<>";
    customProbe = "<>";

    setJustificationType(Justification::centred);
    addListener(this);
    setEditable(false);

}

void ProbeNameEditor::labelTextChanged(Label* label)
{

    String desiredText = label->getText();

    String charactersRemoved = desiredText.removeCharacters(" ./_");

    String uniqueName = config->checkUnique(charactersRemoved, this);

    setText(uniqueName, dontSendNotification);

    switch (config->basestation->getNamingScheme())
    {
    case ProbeNameConfig::PROBE_SPECIFIC_NAMING:
        probe->customName.probeSpecific = uniqueName;
        config->thread->setCustomProbeName(String(probe->info.serial_number), uniqueName);
        probe->displayName = uniqueName;
        customProbe = uniqueName;
        break;
    case ProbeNameConfig::PORT_SPECIFIC_NAMING:
        config->basestation->setCustomPortName(uniqueName, port, dock);
        if (probe != nullptr)
        {
            probe->displayName = uniqueName;
        }
        customPort = uniqueName;
        break;
    }
    
}

String ProbeNameConfig::checkUnique(String input, ProbeNameEditor* originalLabel)
{

    String output = String(input);

    bool foundMatch = true;
    int index = 1;

    while (foundMatch)
    {
        for (auto&& label : probeNames)
        {

            if (label.get() != originalLabel)
            {
                if (label->getText().equalsIgnoreCase(output))
                {
                    foundMatch = true;
                    output = String(input) + "-" + String(index++);
                    break;
                }
                else {
                    foundMatch = false;
                }
            }
            else {
                foundMatch = false;
            }
        }
    }

    return output;
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

ProbeNameConfig::ProbeNameConfig(Basestation* bs_, NeuropixThread* thread_)
{

    basestation = bs_;
    thread = thread_;

    namingScheme = basestation->getNamingScheme();

    int width   = 340;
    int height  = 300;

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

    schemeLabel = new Label("Active Scheme", schemes[(int)namingScheme]);
    schemeLabel->setJustificationType(Justification::centred);
    schemeLabel->setBounds(40, 42, width - 80, 40);
    schemeLabel->setFont(Font("Large Text", 20.0f, Font::plain));
    schemeLabel->setColour(juce::Label::textColourId, juce::Colour(255, 255, 255));
    addAndMakeVisible(schemeLabel);

    description = new Label("Scheme description", descriptions[(int)namingScheme]);
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

            probeNames.push_back(std::move(std::make_unique<ProbeNameEditor>(this, port, dock)));
            probeNames.back()->setBounds(x, y, width, height);
            probeNames.back()->setText("<EMPTY>", dontSendNotification);
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

    for (auto& probe : basestation->getProbes()) {
        
        for (auto&& label : probeNames)
        {

            if (label->port == probe->headstage->port
                && label->dock == probe->dock)
            {

                label->autoName = probe->customName.automatic;
                label->autoNumber = probe->customName.streamSpecific;
                label->customPort = probe->basestation->getCustomPortName(label->port, label->dock);
                label->customProbe = probe->customName.probeSpecific;

                label->probe = probe;
            }
        }
    }
    
    update();

}

void ProbeNameConfig::update()
{

    std::cout << "Naming scheme: " << namingScheme << std::endl;

    basestation->setNamingScheme(namingScheme);

    schemeLabel->setText(schemes[(int)namingScheme], juce::NotificationType::sendNotificationAsync);
    description->setText(descriptions[(int)namingScheme], juce::NotificationType::sendNotificationAsync);
    
    for (auto&& label : probeNames)
    {

        if (namingScheme == PORT_SPECIFIC_NAMING)
        {
            label->setEditable(true);
            label->setColour(juce::Label::ColourIds::backgroundColourId, juce::Colour(255, 255, 255));
            label->setText(basestation->getCustomPortName(label->port, label->dock), dontSendNotification);

            if (label->probe != nullptr)
                label->probe->displayName = basestation->getCustomPortName(label->port, label->dock);
        }
        else {

            label->setEditable(false);
            label->setColour(juce::Label::ColourIds::backgroundColourId, juce::Colour(150, 150, 150));
            label->setText("<>", dontSendNotification);

            if (label->probe != nullptr)
            {
                if (namingScheme == AUTO_NAMING)
                {
                    label->setText(label->autoName, dontSendNotification);
                    label->setEditable(false);
                    label->setColour(juce::Label::ColourIds::backgroundColourId, juce::Colour(210, 210, 210));
                    label->probe->displayName = label->autoName;
                }
                else if (namingScheme == STREAM_INDICES)
                {
                    label->setText(label->autoNumber, dontSendNotification);
                    label->setEditable(false);
                    label->setColour(juce::Label::ColourIds::backgroundColourId, juce::Colour(210, 210, 210));
                    label->probe->displayName = label->autoNumber;
                }
                else if (namingScheme == PROBE_SPECIFIC_NAMING)
                {
                    label->setText(label->customProbe, dontSendNotification);
                    label->setEditable(true);
                    label->setColour(juce::Label::ColourIds::backgroundColourId, juce::Colour(255, 255, 255));
                    label->probe->displayName = label->customProbe;
                }
            }
        }

       
    }

}


void ProbeNameConfig::showPrevScheme()
{
    int currentIndex = (int)namingScheme; 
    currentIndex--;
    if (currentIndex < 0) currentIndex = 3;

    namingScheme = (NamingScheme)currentIndex;
    update();
}

void ProbeNameConfig::showNextScheme()
{
    int currentIndex = (int)namingScheme;
    currentIndex++;
    if (currentIndex > 3) currentIndex = 0;

    namingScheme = (NamingScheme)currentIndex;
    update();
}
