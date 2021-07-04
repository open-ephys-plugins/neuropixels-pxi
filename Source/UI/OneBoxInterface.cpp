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

#include "OneBoxInterface.h"

#include "WavePlayer.h"
#include "DataPlayer.h"

AdcChannelButton::AdcChannelButton(int channel_) : channel(channel_), selected(false)
{
	status = AdcChannelStatus::AVAILABLE;

	inputRange = PLUS_MINUS_FIVE_V;
	threshold = ONE_V;
	triggerWavePlayer = NO;
	mapToOutput = NO_DAC;
	inputSharedBy = NO_ADC;

}


void AdcChannelButton::setSelectedState(bool state)
{
	selected = state;
	repaint();
}

void AdcChannelButton::setStatus(AdcChannelStatus status_, AvailableAdcs sharedChannel)
{
	status = status_;
	inputSharedBy = sharedChannel;
}

void AdcChannelButton::paintButton(Graphics& g, bool isMouseOver, bool isButtonDown)
{
	

	if (selected)
		g.setColour(Colours::white);
	else
		g.setColour(Colours::grey);

	g.fillEllipse(72, 0, 20, 20);

	Colour baseColour;
	String channelName = "ADC " + String(channel);
	String statusText;

	g.setFont(20);
	

	if (status == AdcChannelStatus::AVAILABLE)
	{
		baseColour = Colours::mediumspringgreen;
		statusText = "AVAILABLE";
	}
		
	else if (status == AdcChannelStatus::IN_USE)
	{
		baseColour = Colours::red;
		statusText = "DO NOT CONNECT";
	}
		
	if (isMouseOver || selected)
		baseColour = baseColour.brighter(1.9f);

	g.setColour(baseColour);

	g.drawText(channelName, 0, 0, 65, 20, Justification::right);

	g.fillEllipse(74, 2, 16, 16);

	g.drawText(statusText, 100, 0, 200, 20, Justification::left);

	
}

OneBoxInterface::OneBoxInterface(DataSource* dataSource_, NeuropixThread* thread_, NeuropixEditor* editor_, NeuropixCanvas* canvas_) :
    SettingsInterface(dataSource_, thread_, editor_, canvas_)
{
	adc = (OneBoxADC*) dataSource_;
	adc->ui = this;
	dac = adc->dac;

	for (int ch = 0; ch < 12; ch++)
	{
		AdcChannelButton* button = new AdcChannelButton(ch);
		button->setBounds(25, 100 + 40 * ch, 350, 20);
		addAndMakeVisible(button);
		channels.add(button);
		button->addListener(this);

		if (ch == 0)
		{
			selectedChannel = button;
			button->setSelectedState(true);
		}
			
	}

	rangeSelector = new ComboBox();
	rangeSelector->setBounds(300, 250, 120, 20);
	rangeSelector->addListener(this);
	rangeSelector->addItem("+/- 2.5 V", 1);
	rangeSelector->addItem("+/- 5 V", 2);
	rangeSelector->addItem("+/- 10 V", 3);
	rangeSelector->setSelectedId(2, dontSendNotification);
	addAndMakeVisible(rangeSelector);

	thresholdSelector = new ComboBox();
	thresholdSelector->setBounds(300, 300, 120, 20);
	thresholdSelector->addListener(this);
	thresholdSelector->addItem("1 V", 1);
	thresholdSelector->addItem("3 V", 2);
	thresholdSelector->setSelectedId(1, dontSendNotification);
	addAndMakeVisible(thresholdSelector);

	triggerSelector = new ComboBox();
	triggerSelector->setBounds(300, 350, 120, 20);
	triggerSelector->addListener(this);

	triggerSelector->addItem("FALSE", 1);
	triggerSelector->addItem("TRUE", 2);
	triggerSelector->setSelectedId(1, dontSendNotification);
	addAndMakeVisible(triggerSelector);

	mappingSelector = new ComboBox();
	mappingSelector->setBounds(300, 400, 120, 20);
	mappingSelector->addListener(this);
	addAndMakeVisible(mappingSelector);

	updateMappingSelector();

	wavePlayer = new WavePlayer(dac);
	wavePlayer->setBounds(500, 100, 320, 180);
	addAndMakeVisible(wavePlayer);

	dataPlayer = new DataPlayer(dac);
	dataPlayer->setBounds(500, 340, 320, 180);
	addAndMakeVisible(dataPlayer);
	
	
}

OneBoxInterface::~OneBoxInterface() {}

void OneBoxInterface::startAcquisition()
{

}

void OneBoxInterface::stopAcquisition()
{

}

void OneBoxInterface::enableInput(int chan)
{
	channels[chan]->setStatus(AdcChannelStatus::AVAILABLE);
	repaint();
}

void OneBoxInterface::disableInput(int chan)
{
	channels[chan]->setStatus(AdcChannelStatus::IN_USE);
	repaint();
}

void OneBoxInterface::comboBoxChanged(ComboBox* comboBox)
{
	if (comboBox == rangeSelector)
	{
		// change range
		// update bitVolts values
		selectedChannel->inputRange = comboBox->getSelectedId();
	}
	else if (comboBox == thresholdSelector)
	{
		// change threshold
		selectedChannel->threshold = comboBox->getSelectedId();

	}
	else if (comboBox == triggerSelector)
	{

		for (auto channel : channels)
		{
			channel->triggerWavePlayer = 1;
		}

		selectedChannel->triggerWavePlayer = comboBox->getSelectedId();

		// set trigger

	}
	else if (comboBox == mappingSelector)
	{

		if (selectedChannel->mapToOutput > 1)
		{
			enableInput(selectedChannel->mapToOutput - 2);
		}

		selectedChannel->mapToOutput = comboBox->getSelectedId();

		if (comboBox->getSelectedId() > 1)
			disableInput(comboBox->getSelectedId() - 2);
	}
}

void OneBoxInterface::buttonClicked(Button* button)
{
	for (auto channel : channels)
	{
		if (channel == button)
		{
			channel->setSelectedState(true);
			selectedChannel = channel;

			rangeSelector->setSelectedId(channel->inputRange);
			thresholdSelector->setSelectedId(channel->threshold);
			triggerSelector->setSelectedId(channel->triggerWavePlayer);
			updateMappingSelector();
			mappingSelector->setSelectedId(channel->mapToOutput);
		}
		else {
			channel->setSelectedState(false);
		}
	}
	
	repaint();
}

void OneBoxInterface::updateMappingSelector()
{
	mappingSelector->clear();
	mappingSelector->addItem("-", 1);

	Array<int> occupiedDacs;

	//std::cout << "OCCUPIED: " << std::endl;

	for (int i = 0; i < 12; i++)
	{
		//std::cout << "ADC " << i << " : " << channels[i]->mapToOutput << std::endl;

		if (channels[i]->mapToOutput > 1)
		{
			occupiedDacs.add(channels[i]->mapToOutput - 2);
			//std::cout << "   added." << std::endl;
		}
	}

	for (int i = 0; i < 12; i++)
	{
		//std::cout << "Checking ADC " << i << std::endl;

		if (selectedChannel->channel != i && !occupiedDacs.contains(i))
		{
			mappingSelector->addItem("DAC" + String(i), i + 2);
		}
		
	}
		
	mappingSelector->setSelectedId(selectedChannel->mapToOutput, dontSendNotification);
}


void OneBoxInterface::saveParameters(XmlElement* xml)
{

}

void OneBoxInterface::loadParameters(XmlElement* xml)
{

}

void OneBoxInterface::paint(Graphics& g)
{
	g.setColour(Colours::lightgrey);
	g.setFont(40);

	g.drawText("OneBox Settings",
		10, 5, 500, 45, Justification::left, false);

	g.setFont(15);
	g.drawText("PARAMETERS:", 300, 200, 300, 18, Justification::left, false);
	g.drawText("Input range:", 300, 250-20, 300, 18, Justification::left, false);
	g.drawText("Threshold level:", 300, 300-20, 300, 18, Justification::left, false);
	g.drawText("Trigger WavePlayer:", 300, 350-20, 300, 18, Justification::left, false);
	g.drawText("Map to output:", 300, 400-20, 300, 18, Justification::left, false);

	g.drawRect(290, 195, 140, 235);

	g.drawLine(selectedChannel->getX() + 82, selectedChannel->getBottom(),
		selectedChannel->getX() + 82, selectedChannel->getBottom() + 5, 1.0f);

	g.drawLine(selectedChannel->getX() + 82, selectedChannel->getBottom() +5,
		selectedChannel->getX() + 220, selectedChannel->getBottom() + 5, 1.0f);

	g.drawLine(selectedChannel->getX() + 220, selectedChannel->getBottom() +5,
		270, 312, 1.0f);

	g.drawLine(270, 312,
		290, 312, 1.0f);

}