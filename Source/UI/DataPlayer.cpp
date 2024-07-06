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

#include "DataPlayer.h"

#include "../Probes/OneBoxDAC.h"
#include "OneBoxInterface.h"

#include "../NeuropixComponents.h"

DataPlayerBackground::DataPlayerBackground()
{
    spikePath = Path();

    pathTransform = spikePath.getTransformToScaleToFit (100, 65, 80, 20, false);
}

void DataPlayerBackground::paint (Graphics& g)
{
    g.setColour (Colours::lightgrey);
    g.drawRoundedRectangle (0, 0, getWidth(), getHeight(), 3.0, 2.0);

    g.setFont (20);
    g.drawText ("DataPlayer", 7, 5, 150, 20, Justification::left);

    g.setColour (Colours::red);
    g.strokePath (spikePath, PathStrokeType (1.0), pathTransform);

    g.setColour (Colours::orange);
    g.setFont (15);
    //g.drawText("PROBE", 5, 38, 100, 15, Justification::centredRight);
    g.drawText ("STREAM", 5, 73, 128, 15, Justification::centredRight);
    g.drawText ("CHANNEL", 5, 103, 128, 15, Justification::centredRight);
    g.drawText ("OUTPUT", 5, 133, 128, 15, Justification::centredRight);
}

DataPlayer::DataPlayer (OneBoxDAC* dac_, OneBoxADC* adc_, OneBoxInterface* onebox_)
    : dac (dac_),
      adc (adc_),
      onebox (onebox_)
{
    inputChan = 0;
    outputChan = -1;

    background = std::make_unique<DataPlayerBackground>();

    addAndMakeVisible (background.get());

    int leftMargin = 140;

    playerIndex = std::make_unique<ComboBox>();
    playerIndex->setBounds (12, 40, 120, 20);
    ;
    playerIndex->addListener (this);
    for (int i = 1; i < 9; i++)
        playerIndex->addItem ("DataPlayer " + String (i), i);
    playerIndex->setSelectedId (1, dontSendNotification);
    addAndMakeVisible (playerIndex.get());

    availableProbes = adc_->basestation->getProbes();

    probeSelector = std::make_unique<ComboBox>();
    probeSelector->setBounds (leftMargin, 40, 110, 20);
    probeSelector->addListener (this);

    for (int i = 0; i < availableProbes.size(); i++)
    {
        Probe* currentProbe = availableProbes[i];

        if (i == 0)
            selectedProbe = currentProbe;

        probeSelector->addItem (currentProbe->getName(), i + 1);
    }

    probeSelector->setSelectedId (1, dontSendNotification);
    addAndMakeVisible (probeSelector.get());

    streamSelector = std::make_unique<ComboBox>();
    streamSelector->setBounds (leftMargin, 70, 110, 20);
    streamSelector->addListener (this);
    streamSelector->addItem ("AP", StreamType::AP);
    streamSelector->addItem ("LFP", StreamType::LFP);
    streamSelector->setSelectedId (1, dontSendNotification);
    addAndMakeVisible (streamSelector.get());

    channelSelector = std::make_unique<ComboBox>();
    channelSelector->setBounds (leftMargin, 100, 110, 20);
    channelSelector->addListener (this);

    for (int i = 0; i < 384; i++)
        channelSelector->addItem (String (i + 1), i + 1);

    channelSelector->setSelectedId (1, dontSendNotification);
    addAndMakeVisible (channelSelector.get());

    outputSelector = std::make_unique<ComboBox>();
    outputSelector->setBounds (leftMargin, 130, 110, 20);
    outputSelector->addListener (this);

    addAndMakeVisible (outputSelector.get());
}

DataPlayer::~DataPlayer()
{
}

void DataPlayer::resized()
{
    background->setBounds (0, 0, getWidth(), getHeight());
}

void DataPlayer::comboBoxChanged (ComboBox* comboBox)
{
    if (comboBox == probeSelector.get())
    {
        selectedProbe = availableProbes[comboBox->getSelectedId()];
    }
    else if (comboBox == streamSelector.get())
    {
        streamType = (StreamType) comboBox->getSelectedId();
    }
    else if (comboBox == channelSelector.get())
    {
        inputChan = comboBox->getSelectedId() - 1;
    }
    else if (comboBox == outputSelector.get())
    {
        if (comboBox->getSelectedId() == 1) // deselect output
        {
            //std::cout << "Selected: " << comboBox->getSelectedId() << std::endl;
            //std::cout << "Current output: " << outputChan << std::endl;

            if (outputChan > -1)
            {
                //adc->setChannelType(outputChan, DataSourceType::ADC);
                //onebox->enableInput(outputChan);
            }

            outputChan = -1;

            //std::cout << "New output: " << outputChan << std::endl;
        }
        else if (comboBox->getSelectedId() > 1)
        {
            //std::cout << "Selected: " << comboBox->getSelectedId() << std::endl;
            //std::cout << "Current output: " << outputChan << std::endl;

            if (outputChan > -1)
            {
                //adc->setChannelType(outputChan, DataSourceType::ADC);
                //onebox->enableInput(outputChan);
            }

            outputChan = comboBox->getSelectedId() - 2;
            //adc->setChannelType(outputChan, DataSourceType::DAC);
            //onebox->disableInput(outputChan);

            //std::cout << "New output: " << outputChan << std::endl;
        }
    }

    //onebox->updateAvailableChannels();

    //dac->configureDataPlayer(selectedProbe->basestation->slot,
    //	outputChan,
    //	selectedProbe->port,
    //	selectedProbe->dock,
    //	inputChan,
    //	(int) streamType);
}

void DataPlayer::setAvailableChans (Array<DataSourceType> channelTypes)
{
    outputSelector->clear();
    outputSelector->addItem ("-", 1);

    for (int i = 0; i < channelTypes.size(); i++)
    {
        outputSelector->addItem ("DAC" + String (i), i + 2);
    }

    outputSelector->setSelectedId (outputChan + 2, dontSendNotification);
}

void DataPlayer::saveCustomParameters (XmlElement* xml)
{
}

void DataPlayer::loadCustomParameters (XmlElement* xml)
{
}
