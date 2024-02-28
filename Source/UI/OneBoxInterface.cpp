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

AdcChannelButton::AdcChannelButton(int channel_) : 
	channel(channel_)
{
	status = AdcChannelStatus::AVAILABLE;
}


void AdcChannelButton::setSelectedState(bool state)
{
	selected = state;
	repaint();
}

void AdcChannelButton::setStatus(AdcChannelStatus status_, 
	int sharedChannel)
{
	status = status_;
	mapToOutput = sharedChannel;
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
		statusText = "ENABLED";
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

OneBoxInterface::OneBoxInterface(DataSource* dataSource_, 
	NeuropixThread* thread_, 
	NeuropixEditor* editor_, 
	NeuropixCanvas* canvas_) :
    SettingsInterface(dataSource_, thread_, editor_, canvas_)
{
	adc = (OneBoxADC*) dataSource_;
	adc->ui = this;

	type = SettingsInterface::ONEBOX_SETTINGS_INTERFACE;

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
	rangeSelector->addItem("+/- 2.5 V", (int) AdcInputRange::PLUSMINUS2PT5V);
	rangeSelector->addItem("+/- 5 V", (int)AdcInputRange::PLUSMINUS5V);
	rangeSelector->addItem("+/- 10 V", (int)AdcInputRange::PLUSMINUS10V);
	rangeSelector->setSelectedId((int)AdcInputRange::PLUSMINUS5V, dontSendNotification);
	addAndMakeVisible(rangeSelector);

	thresholdSelector = new ComboBox();
	thresholdSelector->setBounds(300, 300, 120, 20);
	thresholdSelector->addListener(this);
	thresholdSelector->addItem("1 V", (int)AdcThresholdLevel::ONE_VOLT);
	thresholdSelector->addItem("3 V", (int)AdcThresholdLevel::THREE_VOLTS);
	thresholdSelector->setSelectedId((int)AdcThresholdLevel::ONE_VOLT, dontSendNotification);
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

	wavePlayer = new WavePlayer(dac);
	wavePlayer->setBounds(500, 100, 320, 180);
	addAndMakeVisible(wavePlayer);

	dataPlayer = new DataPlayer(dac, adc, this);
	dataPlayer->setBounds(500, 340, 320, 180);
	addAndMakeVisible(dataPlayer);

	updateAvailableChannels();
	
}

OneBoxInterface::~OneBoxInterface() {}

void OneBoxInterface::startAcquisition()
{
	rangeSelector->setEnabled(false);
}

void OneBoxInterface::stopAcquisition()
{
	rangeSelector->setEnabled(true);
}

void OneBoxInterface::setChannelType(int chan, DataSourceType type)
{
	adc->setChannelType(chan, type);

	repaint();
}

void OneBoxInterface::comboBoxChanged(ComboBox* comboBox)
{
	if (comboBox == rangeSelector)
	{
		// change range
		// update bitVolts values
		adc->setAdcInputRange((AdcInputRange) comboBox->getSelectedId(),
			selectedChannel->getChannelIndex());

		CoreServices::updateSignalChain((GenericEditor*) editor);
	}
	else if (comboBox == thresholdSelector)
	{
		// change threshold
		adc->setAdcThresholdLevel((AdcThresholdLevel)comboBox->getSelectedId(),
			selectedChannel->getChannelIndex());

	}
	else if (comboBox == triggerSelector)
	{
		// set trigger
		for (auto channel : channels)
		{
			adc->setTriggersWaveplayer(false,
				channel->getChannelIndex());
		}

		adc->setTriggersWaveplayer((bool) comboBox->getSelectedId() - 1,
			selectedChannel->getChannelIndex());

	}
	else if (comboBox == mappingSelector)
	{

		/*if (selectedChannel->mapToOutput > 1)
		{
			adc->setChannelType(selectedChannel->mapToOutput,
				DataSourceType::ADC);
		}

		selectedChannel->mapToOutput = comboBox->getSelectedId() - 2;

		if (selectedChannel->mapToOutput > -1)
			adc->setChannelType(selectedChannel->mapToOutput,
				DataSourceType::DAC);*/

		//updateAvailableChannels();
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

			rangeSelector->setSelectedId((int) adc->getAdcInputRange(selectedChannel->getChannelIndex()));
			thresholdSelector->setSelectedId((int)adc->getAdcThresholdLevel(selectedChannel->getChannelIndex()));
			triggerSelector->setSelectedId((int)adc->getTriggersWaveplayer(selectedChannel->getChannelIndex()) + 1);
		}
		else {
			channel->setSelectedState(false);
		}

		channel->repaint();
	}
	
	//updateAvailableChannels();

	repaint();

}

void OneBoxInterface::updateAvailableChannels()
{
	//mappingSelector->clear();
	//mappingSelector->addItem("-", 1);

	//Array<DataSourceType> channelTypes = adc->getChannelTypes();

	//std::cout << "OCCUPIED: " << std::endl;

	//for (int i = 0; i < channelTypes.size(); i++)
	//{
		//std::cout << "ADC " << i << " : " << channels[i]->mapToOutput << std::endl;

		//if (selectedChannel->channel != i &&
		//	channelTypes[i] == DataSourceType::ADC)
		//{
		//	mappingSelector->addItem("DAC" + String(i), i + 2);
			//std::cout << "   added." << std::endl;
		//}
	//}

	//mappingSelector->setSelectedId(selectedChannel->mapToOutput, 
	//	dontSendNotification);

	//dataPlayer->setAvailableChans(channelTypes);

}


void OneBoxInterface::saveParameters(XmlElement* xml)
{

	for (auto channel : channels)
	{
		XmlElement* xmlNode = xml->createNewChildElement("ADC_CHANNEL");

		xmlNode->setAttribute("index", channel->getChannelIndex());
		xmlNode->setAttribute("input_range", (int) adc->getAdcInputRange(channel->getChannelIndex()));
		xmlNode->setAttribute("threshold_level", (int) adc->getAdcThresholdLevel(channel->getChannelIndex()));
		xmlNode->setAttribute("triggers_waveplayer", adc->getTriggersWaveplayer(channel->getChannelIndex()));
		//xmlNode->setAttribute("map_to_output", channel->mapToOutput);
	
	}

}

void OneBoxInterface::loadParameters(XmlElement* xml)
{
	forEachXmlChildElement(*xml, xmlNode)
	{
		if (xmlNode->hasTagName("ADC_CHANNEL"))
		{
			int index = xmlNode->getIntAttribute("index", 0);
			int input_range = xmlNode->getIntAttribute("input_range", (int)AdcInputRange::PLUSMINUS5V);
			int threshold_level = xmlNode->getIntAttribute("threshold_level", (int)AdcThresholdLevel::ONE_VOLT);
			bool triggers_waveplayer = xmlNode->getBoolAttribute("triggers_waveplayer", false); \

				adc->setAdcInputRange((AdcInputRange)input_range, index);
			adc->setAdcThresholdLevel((AdcThresholdLevel)threshold_level, index);
			adc->setTriggersWaveplayer(triggers_waveplayer, index);

		}
	}
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