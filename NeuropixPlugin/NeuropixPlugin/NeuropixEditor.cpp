/*
    ------------------------------------------------------------------

    This file is part of the Open Ephys GUI
    Copyright (C) 2016 Allen Institute for Brain Science and Open Ephys

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

#include "NeuropixThread.h"
#include "NeuropixEditor.h"

NeuropixEditor::NeuropixEditor(GenericProcessor* parentNode, NeuropixThread* t, bool useDefaultParameterEditors)
 : VisualizerEditor(parentNode, useDefaultParameterEditors)
{

    thread = t;
    canvas = nullptr;

    desiredWidth = 200;
    tabText = "Neuropix PXI";

    triggerTypeButton = new UtilityButton("INTERNAL", Font("Small Text", 13, Font::plain));
    triggerTypeButton->setRadius(3.0f);
    triggerTypeButton->setBounds(20,70,85,22);
    triggerTypeButton->addListener(this);
    triggerTypeButton->setTooltip("Switch between external and internal triggering");
    triggerTypeButton->setToggleState(true, dontSendNotification);
    addAndMakeVisible(triggerTypeButton);

    internalTrigger = true;

    triggerTypeLabel = new Label("Trigger", "Trigger");
    triggerTypeLabel->setFont(Font("Small Text", 13, Font::plain));
    triggerTypeLabel->setBounds(105,71,100,20);
    triggerTypeLabel->setColour(Label::textColourId, Colours::darkgrey);
    addAndMakeVisible(triggerTypeLabel);

    restartButton = new UtilityButton("YES", Font("Small Text", 13, Font::plain));
	restartButton->setRadius(3.0f);
	restartButton->setBounds(20, 100, 34, 22);
	restartButton->addListener(this);
	restartButton->setTooltip("Auto-restart if probe stops sending data");
	restartButton->setToggleState(true, dontSendNotification);
	addAndMakeVisible(restartButton);

	autoRestart = true;

    lfpButton = new UtilityButton("LFP", Font("Small Text", 13, Font::plain));
    lfpButton->setRadius(3.0f);
    lfpButton->setBounds(20, 35, 34, 22);
    lfpButton->addListener(this);
    lfpButton->setTooltip("Toggle LFP data output");
    lfpButton->setToggleState(true, dontSendNotification);
    //   addAndMakeVisible(lfpButton);

    sendLfp = true;

    apButton = new UtilityButton("AP", Font("Small Text", 13, Font::plain));
    apButton->setRadius(3.0f);
    apButton->setBounds(65, 35, 34, 22);
    apButton->addListener(this);
    apButton->setTooltip("Toggle AP data output");
    apButton->setToggleState(true, dontSendNotification);
    //addAndMakeVisible(apButton);

    sendAp = true;

    restartLabel = new Label("Auto restart", "Auto restart");
	restartLabel->setFont(Font("Small Text", 13, Font::plain));
	restartLabel->setBounds(55, 101, 200, 20);
	restartLabel->setColour(Label::textColourId, Colours::darkgrey);
	addAndMakeVisible(restartLabel);

    
}

NeuropixEditor::~NeuropixEditor()
{

}

void NeuropixEditor::comboBoxChanged(ComboBox* comboBox)
{
   
}

void NeuropixEditor::buttonCallback(Button* button)
{
    if (!acquisitionIsActive)
    {
    
        if (button == triggerTypeButton)
        {
            internalTrigger = !internalTrigger;

            if (internalTrigger)
            {
                triggerTypeButton->setLabel("INTERNAL");
                triggerTypeButton->setToggleState(true, dontSendNotification);
            } else {
                triggerTypeButton->setLabel("EXTERNAL");
                triggerTypeButton->setToggleState(false, dontSendNotification);
            }

            thread->setTriggerMode(internalTrigger);
        
        }
		else if (button == restartButton)
        {
			autoRestart = !autoRestart;

			if (autoRestart)
            {
				restartButton->setLabel("YES");
                restartButton->setToggleState(true, dontSendNotification);
            }
            else {
                restartButton->setLabel("NO");
				restartButton->setToggleState(false, dontSendNotification);
            }

			thread->setAutoRestart(autoRestart);
        } 
        else if (button == apButton)
        {
            sendAp = !sendAp;
            apButton->setToggleState(sendAp, dontSendNotification);
           // thread->toggleApData(sendAp);
        }
        else if (button == lfpButton)
        {
            sendLfp = !sendLfp;
            lfpButton->setToggleState(sendLfp, dontSendNotification);
           // thread->toggleLfpData(sendLfp);
        }
    }
    else {
        CoreServices::sendStatusMessage("Cannot update parameters while acquisition is active.");
    }
}


void NeuropixEditor::saveEditorParameters(XmlElement* xml)
{

}

void NeuropixEditor::loadEditorParameters(XmlElement* xml)
{

}

Visualizer* NeuropixEditor::createNewCanvas(void)
{
    std::cout << "Button clicked..." << std::endl;
    GenericProcessor* processor = (GenericProcessor*) getProcessor();
    std::cout << "Got processor." << std::endl;
    canvas = new NeuropixCanvas(processor, thread);
    std::cout << "Created canvas." << std::endl;
    return canvas;
}

/********************************************/

NeuropixCanvas::NeuropixCanvas(GenericProcessor* p, NeuropixThread* thread)
{

    processor = (SourceNode*) p;

    neuropixViewport = new Viewport();
    neuropixInterface = new NeuropixInterface(thread, (NeuropixEditor*) p->getEditor());
    neuropixViewport->setViewedComponent(neuropixInterface, false);
    addAndMakeVisible(neuropixViewport);

    resized();
    update();
}

NeuropixCanvas::~NeuropixCanvas()
{

}

void NeuropixCanvas::paint(Graphics& g)
{
    g.fillAll(Colours::darkgrey);
}

void NeuropixCanvas::refresh()
{
    repaint();
}

void NeuropixCanvas::refreshState()
{
    resized();
}

void NeuropixCanvas::update()
{

}

void NeuropixCanvas::beginAnimation()
{
}

void NeuropixCanvas::endAnimation()
{
}

void NeuropixCanvas::resized()
{

    neuropixViewport->setBounds(0,0,getWidth(),getHeight());
    neuropixInterface->setBounds(0,0,getWidth()-neuropixViewport->getScrollBarThickness(), 600);
}

void NeuropixCanvas::setParameter(int x, float f)
{

}

void NeuropixCanvas::setParameter(int a, int b, int c, float d)
{
}

void NeuropixCanvas::buttonClicked(Button* button)
{

}

void NeuropixCanvas::saveVisualizerParameters(XmlElement* xml)
{
    neuropixInterface->saveParameters(xml);
}

void NeuropixCanvas::loadVisualizerParameters(XmlElement* xml)
{
    neuropixInterface->loadParameters(xml);
}

/*****************************************************/
NeuropixInterface::NeuropixInterface(NeuropixThread* t, NeuropixEditor* e) : thread(t), editor(e)
{
    cursorType = MouseCursor::NormalCursor;

    

    isOverZoomRegion = false;
    isOverUpperBorder = false;
    isOverLowerBorder = false;
    isSelectionActive = false;
    isOverChannel = false;
    
    for (int i = 0; i < 966; i++)
    {
        channelStatus.add(-1);
        channelReference.add(0);
        channelLfpGain.add(0);
        channelApGain.add(0);
        channelSelectionState.add(0);
        channelOutput.add(1);
        channelColours.add(Colour(20,20,20));
    }

    visualizationMode = 0;

    addMouseListener(this, true);

    zoomHeight = 50;
    lowerBound = 530;
    zoomOffset = 0;
    dragZoneWidth = 10;

    apGainComboBox = new ComboBox("apGainComboBox");
    apGainComboBox->setBounds(400, 150, 65, 22);
    apGainComboBox->addListener(this);

    lfpGainComboBox = new ComboBox("lfpGainComboBox");
    lfpGainComboBox->setBounds(400, 200, 65, 22);
    lfpGainComboBox->addListener(this);

    Array<int> gains;
    gains.add(50);
    gains.add(125);
    gains.add(250);
    gains.add(500);
    gains.add(1000);
    gains.add(1500);
    gains.add(2000);
    gains.add(3000);

    for (int i = 0; i < 8; i++)
    {
        lfpGainComboBox->addItem(String(gains[i]) + String("x"), i + 1);
        apGainComboBox->addItem(String(gains[i]) + String("x"), i + 1);
    }

    lfpGainComboBox->setSelectedId(3, dontSendNotification);
    apGainComboBox->setSelectedId(4, dontSendNotification);

    referenceComboBox = new ComboBox("ReferenceComboBox");
    referenceComboBox->setBounds(400, 250, 65, 22);
    referenceComboBox->addListener(this);
    referenceComboBox->addItem("Ext", 1);
	referenceComboBox->addItem("Tip", 2);
	referenceComboBox->addItem("192", 3);
	referenceComboBox->addItem("576", 4);
	referenceComboBox->addItem("959", 5);
    referenceComboBox->setSelectedId(1, dontSendNotification);

    filterComboBox = new ComboBox("FilterComboBox");
    filterComboBox->setBounds(400, 300, 75, 22);
    filterComboBox->addListener(this);
    filterComboBox->addItem("ON", 1);
    filterComboBox->addItem("OFF", 2);
    filterComboBox->setSelectedId(1, dontSendNotification);

    enableButton = new UtilityButton("ENABLE", Font("Small Text", 13, Font::plain));
    enableButton->setRadius(3.0f);
    enableButton->setBounds(400,95,65,22);
    enableButton->addListener(this);
    enableButton->setTooltip("Enable selected channel(s)");

    selectAllButton = new UtilityButton("SELECT ALL", Font("Small Text", 13, Font::plain));
    selectAllButton->setRadius(3.0f);
    selectAllButton->setBounds(400,50,95,22);
    selectAllButton->addListener(this);
    selectAllButton->setTooltip("Select all channels");

    outputOnButton = new UtilityButton("ON", Font("Small Text", 13, Font::plain));
    outputOnButton->setRadius(3.0f);
    outputOnButton->setBounds(400,350,40,22);
    outputOnButton->addListener(this);
    outputOnButton->setTooltip("Turn output on for selected channels");

    outputOffButton = new UtilityButton("OFF", Font("Small Text", 13, Font::plain));
    outputOffButton->setRadius(3.0f);
    outputOffButton->setBounds(450,350,40,22);
    outputOffButton->addListener(this);
    outputOffButton->setTooltip("Turn output off for selected channels");

    enableViewButton = new UtilityButton("VIEW", Font("Small Text", 12, Font::plain));
    enableViewButton->setRadius(3.0f);
    enableViewButton->setBounds(480,97,45,18);
    enableViewButton->addListener(this);
    enableViewButton->setTooltip("View channel enabled state");

    lfpGainViewButton = new UtilityButton("VIEW", Font("Small Text", 12, Font::plain));
    lfpGainViewButton->setRadius(3.0f);
    lfpGainViewButton->setBounds(480,202,45,18);
    lfpGainViewButton->addListener(this);
    lfpGainViewButton->setTooltip("View LFP gain of each channel");

    apGainViewButton = new UtilityButton("VIEW", Font("Small Text", 12, Font::plain));
    apGainViewButton->setRadius(3.0f);
    apGainViewButton->setBounds(480,152,45,18);
    apGainViewButton->addListener(this);
    apGainViewButton->setTooltip("View AP gain of each channel");

    referenceViewButton = new UtilityButton("VIEW", Font("Small Text", 12, Font::plain));
    referenceViewButton->setRadius(3.0f);
    referenceViewButton->setBounds(480,252,45,18);
    referenceViewButton->addListener(this);
    referenceViewButton->setTooltip("View reference of each channel");

    annotationButton = new UtilityButton("ADD", Font("Small Text", 12, Font::plain));
    annotationButton->setRadius(3.0f);
    annotationButton->setBounds(400,480,40,18);
    annotationButton->addListener(this);
    annotationButton->setTooltip("Add annotation to selected channels");

    calibrationButton = new UtilityButton("ADC CALIBRATION", Font("Small Text", 12, Font::plain));
    calibrationButton->setRadius(3.0f);
    calibrationButton->setBounds(400, 520, 150, 24);
    calibrationButton->addListener(this);
    calibrationButton->setTooltip("Load adc calibration settings from EEPROM");

    calibrationButton2 = new UtilityButton("GAIN CALIBRATION", Font("Small Text", 12, Font::plain));
    calibrationButton2->setRadius(3.0f);
    calibrationButton2->setBounds(570, 520, 150, 24);
    calibrationButton2->addListener(this);
    calibrationButton2->setTooltip("Load gain calibration settings from EEPROM");

	calibrationButton3 = new UtilityButton("CALIBRATE FROM FILE", Font("Small Text", 12, Font::plain));
	calibrationButton3->setRadius(3.0f);
	calibrationButton3->setBounds(400, 560, 200, 24);
	calibrationButton3->addListener(this);
	calibrationButton3->setTooltip("Load calibration settings from file");

    addAndMakeVisible(lfpGainComboBox);
    addAndMakeVisible(apGainComboBox);
    addAndMakeVisible(referenceComboBox);
    addAndMakeVisible(filterComboBox);
    //addAndMakeVisible(activityViewComboBox);

    addAndMakeVisible(enableButton);
    //addAndMakeVisible(selectAllButton);
    //addAndMakeVisible(outputOnButton);
    //addAndMakeVisible(outputOffButton);
    addAndMakeVisible(enableViewButton);
    addAndMakeVisible(lfpGainViewButton);
    addAndMakeVisible(apGainViewButton);
    addAndMakeVisible(referenceViewButton);
    addAndMakeVisible(annotationButton);
    //addAndMakeVisible(calibrationButton);
    //addAndMakeVisible(calibrationButton2);
	//addAndMakeVisible(calibrationButton3);

    
    infoLabel = new Label("INFO", "INFO");
    infoLabel->setFont(Font("Small Text", 13, Font::plain));
    infoLabel->setBounds(550, 10, 300, 250);
    infoLabel->setColour(Label::textColourId, Colours::grey);
    addAndMakeVisible(infoLabel);

    lfpGainLabel = new Label("LFP GAIN","LFP GAIN");
    lfpGainLabel->setFont(Font("Small Text", 13, Font::plain));
    lfpGainLabel->setBounds(396,180,100,20);
    lfpGainLabel->setColour(Label::textColourId, Colours::grey);
    addAndMakeVisible(lfpGainLabel);

    apGainLabel = new Label("AP GAIN","AP GAIN");
    apGainLabel->setFont(Font("Small Text", 13, Font::plain));
    apGainLabel->setBounds(396,130,100,20);
    apGainLabel->setColour(Label::textColourId, Colours::grey);
    addAndMakeVisible(apGainLabel);

    referenceLabel = new Label("REFERENCE", "REFERENCE");
    referenceLabel->setFont(Font("Small Text", 13, Font::plain));
    referenceLabel->setBounds(396,230,100,20);
    referenceLabel->setColour(Label::textColourId, Colours::grey);
    addAndMakeVisible(referenceLabel);

    filterLabel = new Label("FILTER", "AP FILTER CUT");
    filterLabel->setFont(Font("Small Text", 13, Font::plain));
    filterLabel->setBounds(396,280,200,20);
    filterLabel->setColour(Label::textColourId, Colours::grey);
    addAndMakeVisible(filterLabel);

    outputLabel = new Label("OUTPUT", "OUTPUT");
    outputLabel->setFont(Font("Small Text", 13, Font::plain));
    outputLabel->setBounds(396,330,200,20);
    outputLabel->setColour(Label::textColourId, Colours::grey);
    //addAndMakeVisible(outputLabel);

    annotationLabel = new Label("ANNOTATION", "Custom annotation");
    annotationLabel->setBounds(396,420,200,20);
    annotationLabel->setColour(Label::textColourId, Colours::white);
    annotationLabel->setEditable(true);
    annotationLabel->addListener(this);
    addAndMakeVisible(annotationLabel);

    annotationLabelLabel = new Label("ANNOTATION_LABEL", "ANNOTATION");
    annotationLabelLabel->setFont(Font("Small Text", 13, Font::plain));
    annotationLabelLabel->setBounds(396,400,200,20);
    annotationLabelLabel->setColour(Label::textColourId, Colours::grey);
    addAndMakeVisible(annotationLabelLabel);

    outputLabel = new Label("OUTPUT", "OUTPUT");
    outputLabel->setFont(Font("Small Text", 13, Font::plain));
    outputLabel->setBounds(396,330,200,20);
    outputLabel->setColour(Label::textColourId, Colours::grey);
    addAndMakeVisible(outputLabel);

    shankPath.startNewSubPath(27, 28);
    shankPath.lineTo(27, 514);
    shankPath.lineTo(27+5, 522);
    shankPath.lineTo(27+10, 514);
    shankPath.lineTo(27+10, 28);
    shankPath.closeSubPath();

    colorSelector = new ColorSelector(this);
    colorSelector->setBounds(400, 450, 250, 20);
    addAndMakeVisible(colorSelector);

    std::cout << "Created Neuropix Interface" << std::endl;

	updateInfoString();

    displayBuffer.setSize(768, 10000);

	for (int i = 0; i < 384; i++)
	{
		if (i == 191)
			channelStatus.set(i, -2);
		else
			channelStatus.set(i, 1);
	}

	for (int i = 384; i < 966; i++)
	{
		if (i == 575 || i == 959)
			channelStatus.set(i, -2);
		else
			channelStatus.set(i, 0);
	}

	// default settings
	for (int i = 0; i < 384; i++)
	{
		channelApGain.set(i, 3);
		channelLfpGain.set(i, 2);
		channelReference.set(i, 0);
	}

}

NeuropixInterface::~NeuropixInterface()
{

}

void NeuropixInterface::updateInfoString()
{
	String probeInfo;
	String hsInfo;
	String bscInfo;
	String bsInfo;
	String apiInfo;

	thread->getInfo(probeInfo, hsInfo, bscInfo, bsInfo, apiInfo);
	String labelString = "Probe: ";
	labelString += probeInfo;
	labelString += "\n\nHeadstage: ";
	labelString += hsInfo;
	labelString += "\n\nBasestation: ";
	labelString += bscInfo;
	labelString += "\n\nFPGA: ";
	labelString += bsInfo; // option
	labelString += "\n\API: ";
	labelString += apiInfo;

    infoLabel->setText(labelString, dontSendNotification);

}

void NeuropixInterface::labelTextChanged(Label* label)
{
    if (label == annotationLabel)
    {
        colorSelector->updateCurrentString(label->getText());
    }
}

void NeuropixInterface::comboBoxChanged(ComboBox* comboBox)
{

    if (!editor->acquisitionIsActive)
    {
        if (comboBox == apGainComboBox | comboBox == lfpGainComboBox)
        {
			int gainSettingAp = apGainComboBox->getSelectedId() - 1;
			int gainSettingLfp = lfpGainComboBox->getSelectedId() - 1;

			thread->setAllGains(gainSettingAp, gainSettingLfp);

			for (int i = 0; i < 966; i++)
			{
				channelApGain.set(i, gainSettingAp);
				channelLfpGain.set(i, gainSettingLfp);
			}
        }
        else if (comboBox == referenceComboBox)
        {

			int refSetting = comboBox->getSelectedId() - 1;
            
			thread->setAllReferences(refSetting);

			for (int i = 0; i < 966; i++)
			{
				channelReference.set(i, refSetting);
			}

        }
        else if (comboBox == filterComboBox)
        {
            // inform the thread of the new settings
            int filterSetting = comboBox->getSelectedId() - 1;

            // 0 = ON
            // 1 = OFF

			if (filterSetting == 0)
				thread->setFilter(true);
			else
				thread->setFilter(false);
        }
        

        repaint();
    } 
     else {
         CoreServices::sendStatusMessage("Cannot update parameters while acquisition is active");// no parameter change while acquisition is active
    }
    
}

void NeuropixInterface::setAnnotationLabel(String s, Colour c)
{
    annotationLabel->setText(s, NotificationType::dontSendNotification);
    annotationLabel->setColour(Label::textColourId, c);
}

void NeuropixInterface::buttonClicked(Button* button)
{
    if (button == selectAllButton)
    {
        for (int i = 0; i < 966; i++)
        {
            channelSelectionState.set(i, 1);
        }

        repaint();

    } else if (button == enableViewButton)
    {
        visualizationMode = 0;
        stopTimer();
        repaint();
    } 
     else if (button == apGainViewButton)
    {
        visualizationMode = 1;
        stopTimer();
        repaint();
    } else if (button == lfpGainViewButton)
    {
        visualizationMode = 2;
        stopTimer();
        repaint();
    }
    else if (button == referenceViewButton)
    {
        visualizationMode = 3;
        stopTimer();
        repaint();
    } else if (button == enableButton)
    {
        if (!editor->acquisitionIsActive)
        {
            int maxChan = 0;

            for (int i = 0; i < 966; i++)
            {
                if (channelSelectionState[i] == 1) // channel is currently selected
                {

                    if (channelStatus[i] != -1) // channel can be turned on
                    {
                        if (channelStatus[i] > -1) // not a reference
                            channelStatus.set(i, 1); // turn channel on
                        else
                            channelStatus.set(i, -2); // turn channel on

                        // 4. enable electrode
                        thread->selectElectrode(getChannelForElectrode(i), getConnectionForChannel(i), false);
                        maxChan = i;

                        int startPoint = -768;
                        int jump = 384;

                        for (int j = startPoint; j <= -startPoint; j += jump)
                        {
                            //std::cout << "Checking channel " << j + i << std::endl;

                            int newChan = j + i;

                            if (newChan >= 0 && newChan < 966 && newChan != i)
                            {
                                //std::cout << "  In range" << std::endl;

                                if (channelStatus[newChan] != -1)
                                {
                                    //std::cout << "    Turning off." << std::endl;
                                    if (channelStatus[i] > -1) // not a reference
                                        channelStatus.set(newChan, 0); // turn connected channel off
                                    else
                                        channelStatus.set(newChan, -3); // turn connected channel off
                                }
                            }
                        }
                    }
                }
            }

            thread->selectElectrode(getChannelForElectrode(maxChan), getConnectionForChannel(maxChan), true);
            repaint();
        }

    } else if (button == outputOnButton)
    {

        if (!editor->acquisitionIsActive)
        {


            for (int i = 0; i < 966; i++)
            {
                if (channelSelectionState[i] == 1)
                {
                    channelOutput.set(i, 1);
                    // 5. turn output on
                }

            }

            repaint();
        }
    } else if (button == outputOffButton)
    {
        if (!editor->acquisitionIsActive)
        {
            for (int i = 0; i < 966; i++)
            {
                if (channelSelectionState[i] == 1)
                {
                    channelOutput.set(i, 0);
                    // 6. turn output off
                }
                
            }
            repaint();
        }

    } else if (button == annotationButton)
    {
        //Array<int> a = getSelectedChannels();

        //if (a.size() > 0)
        String s = annotationLabel->getText();
        Array<int> a = getSelectedChannels();
        //Annotation a = Annotation(, getSelectedChannels());

        if (a.size() > 0)
            annotations.add(Annotation(s, a, colorSelector->getCurrentColour()));

        repaint();
    }
    else if (button == calibrationButton)
    {
        if (!editor->acquisitionIsActive)
        {
            std::cout << "Calibrating ADCs..." << std::endl;
            thread->calibrateADCs();
            std::cout << "Calibration settings loaded." << std::endl;
			calibrationButton->setToggleState(true, dontSendNotification);
        }
    }
    else if (button == calibrationButton2)
    {
        if (!editor->acquisitionIsActive)
        {

			std::cout << "Calibrating gain..." << std::endl;
			thread->calibrateGains();
			std::cout << "Calibration settings loaded." << std::endl;
			calibrationButton2->setToggleState(true, dontSendNotification);
        }
	}
	else if (button == calibrationButton3)
	{
		if (!editor->acquisitionIsActive)
		{

			thread->calibrateFromCsv();
			calibrationButton3->setToggleState(true, dontSendNotification);
			std::cout << "Done." << std::endl;
		}
	}
    
}

Array<int> NeuropixInterface::getSelectedChannels()
{
    Array<int> a;

    for (int i = 0; i < 966; i++)
    {
        if (channelSelectionState[i] == 1)
        {
            a.add(i);
        }
    }

    return a;
}

void NeuropixInterface::mouseMove(const MouseEvent& event)
{
    float y = event.y;
    float x = event.x;

    //std::cout << x << " " << y << std::endl;

    bool isOverZoomRegionNew = false;
    bool isOverUpperBorderNew = false;
    bool isOverLowerBorderNew = false;

    if (y > lowerBound - zoomOffset - zoomHeight - dragZoneWidth/2 
     && y < lowerBound - zoomOffset + dragZoneWidth/2 &&  x > 9 && x < 54)
    {
        isOverZoomRegionNew = true;
    } else {
        isOverZoomRegionNew = false;
    }

    if (isOverZoomRegionNew)
    {
        if (y > lowerBound - zoomHeight - zoomOffset - dragZoneWidth/2
            && y <  lowerBound - zoomHeight - zoomOffset + dragZoneWidth/2 )
        {
            isOverUpperBorderNew = true;

        } else if (y > lowerBound  - zoomOffset - dragZoneWidth/2
            && y <  lowerBound  - zoomOffset + dragZoneWidth/2)
        {
            isOverLowerBorderNew = true;

        } else {
            isOverUpperBorderNew = false;
            isOverLowerBorderNew = false;
        }
    }

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
        } else {

            if (isOverUpperBorder)
                cursorType = MouseCursor::TopEdgeResizeCursor;
            else if (isOverLowerBorder)
                cursorType = MouseCursor::BottomEdgeResizeCursor;
            else
                cursorType = MouseCursor::NormalCursor;
        }

        repaint();
    }

    if (x > 225 - channelHeight && x < 225 + channelHeight && y < lowerBound && y > 18)
    {
        int chan = getNearestChannel(x, y);
        isOverChannel = true;
        channelInfoString = getChannelInfoString(chan);

        //std::cout << channelInfoString << std::endl;

        repaint();
    } else {
        bool isOverChannelNew = false;

        if (isOverChannelNew != isOverChannel)
        {
            isOverChannel = isOverChannelNew;
            repaint();
        }
    }

}

int NeuropixInterface::getNearestChannel(int x, int y)
{
    int chan = ((lowerBound - y) * 2 / channelHeight) + lowestChan + 2;

    if (chan % 2 == 1)
        chan += 1;

    if (x > 225)
        chan += 1;

    return chan;
}

String NeuropixInterface::getChannelInfoString(int chan)
{
    String a;
    a += "Channel ";
    a += String(chan + 1);
    a += "\n\nType: ";
    
    if (channelStatus[chan] < -1)
    {
        a += "REF";
        if (channelStatus[chan] == -2)
            a += "\nEnabled";
        else
            a += "\nDisabled";
        return a;
    }
    else
    {
        a += "SIGNAL";
    }

    a += "\nEnabled: ";

    if (channelStatus[chan] == 1)
        a += "YES";
    else
        a += "NO";

    a += "\nAP Gain: ";
    a += String(apGainComboBox->getItemText(channelApGain[chan]));

    a += "\nLFP Gain: ";
    a += String(lfpGainComboBox->getItemText(channelLfpGain[chan]));

    a += "\nReference: ";
    a += String(channelReference[chan]);

    return a;
}

void NeuropixInterface::mouseUp(const MouseEvent& event)
{
    if (isSelectionActive)
    {

        isSelectionActive = false;
        repaint();
    }
    
}

void NeuropixInterface::mouseDown(const MouseEvent& event)
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
                for (int i = 0; i < 966; i++)
                    channelSelectionState.set(i, 0);
            }

            if (event.x > 225 - channelHeight && event.x < 225 + channelHeight)
            {
                int chan = getNearestChannel(event.x, event.y);

                //std::cout << chan << std::endl;

                if (chan >= 0 && chan < 966)
                {
                    channelSelectionState.set(chan, 1);
                }

            }
            repaint();
        }
    } else {

        if (event.x > 225 + 10 && event.x < 225 + 150)
        {
            int currentAnnotationNum;

            for (int i = 0; i < annotations.size(); i++)
            {
                Annotation& a = annotations.getReference(i);
                float yLoc = a.currentYLoc;

                if (float(event.y) < yLoc && float(event.y) > yLoc - 12)
                {
                    currentAnnotationNum = i;
                    break;
                } else {
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
                        annotations.removeRange(currentAnnotationNum,1);
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

void NeuropixInterface::mouseDrag(const MouseEvent& event)
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
            } else {
                zoomHeight = initialHeight + event.getDistanceFromDragStartY();
            }

        }
        else {
            zoomOffset = initialOffset - event.getDistanceFromDragStartY();
        }
        //std::cout << zoomOffset << std::endl;
    } else if (event.x > 150 && event.x < 450)
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
        int chanStart = getNearestChannel(224, y + h);
        int chanEnd = getNearestChannel(224, y) + 1;

        //std::cout << chanStart << " " << chanEnd << std::endl;

        if (x < 225 + channelHeight)
        {
            for (int i = 0; i < 966; i++)
            {
                if (i >= chanStart && i <= chanEnd)
                {
                    if (i % 2 == 1)
                    {
                        if ((x + w > 225) || (x > 225 && x < 225 + channelHeight))
                            channelSelectionState.set(i, 1);
                        else
                            channelSelectionState.set(i, 0);
                    } else {
                        if ((x < 225) && (x + w > (225 - channelHeight)))
                            channelSelectionState.set(i, 1);
                        else
                            channelSelectionState.set(i, 0);
                    }
                } else {
                    if (!event.mods.isShiftDown())
                        channelSelectionState.set(i, 0);
                }
            }
        } else {
            for (int i = 0; i < 966; i++)
            {
                if (!event.mods.isShiftDown())
                    channelSelectionState.set(i, 0);
            }
        }

        repaint();
    }

    if (zoomOffset > lowerBound - zoomHeight - 18)
        zoomOffset = lowerBound - zoomHeight - 18;
    else if (zoomOffset < 0)
        zoomOffset = 0;

    if (zoomHeight < 10)
        zoomHeight = 10;
    if (zoomHeight > 100)
        zoomHeight = 100;

    repaint();
}

void NeuropixInterface::mouseWheelMove(const MouseEvent&  event, const MouseWheelDetails&   wheel)
{

    if (event.x > 100 && event.x < 450)
    {

        if (wheel.deltaY > 0)
            zoomOffset += 2;
        else
            zoomOffset -= 2;

        //std::cout << wheel.deltaY << " " << zoomOffset << std::endl;

        if (zoomOffset < 0)
        {
            zoomOffset = 0;
        } else if (zoomOffset + 18 + zoomHeight > lowerBound)
        {
            zoomOffset = lowerBound - zoomHeight - 18;
        }

        repaint();
    }

}

MouseCursor NeuropixInterface::getMouseCursor()
{
    MouseCursor c = MouseCursor(cursorType);

    return c;
}

void NeuropixInterface::paint(Graphics& g)
{

    int xOffset = 27;

    // draw zoomed-out channels channels

    for (int i = 0; i < channelStatus.size(); i++)
    {
        g.setColour(getChannelColour(i));

        g.setPixel(xOffset + 3 + ((i % 2)) * 2, 513 - (i / 2));
        g.setPixel(xOffset + 3 + ((i % 2)) * 2 + 1, 513 - (i / 2));
    }

    // channel 1 = pixel 513
    // channel 966 = pixel 30
    // 483 pixels for 966 channels

    // draw channel numbers

    g.setColour(Colours::grey);
    g.setFont(12);

    int ch = 0;

    for (int i = 513; i > 30; i -= 50)
    {
        g.drawLine(6, i, 18, i);
        g.drawLine(44, i, 54, i);
        g.drawText(String(ch), 59, int(i) - 6, 100, 12, Justification::left, false);
        ch += 100;
    }

    // draw shank outline
    g.setColour(Colours::lightgrey);
    g.strokePath(shankPath, PathStrokeType(1.0));

    // draw zoomed channels

    lowestChan = (513 - (lowerBound - zoomOffset)) * 2 - 1;
    highestChan = (513 - (lowerBound - zoomOffset - zoomHeight)) * 2 + 10;

    float totalHeight = float(lowerBound + 100);
    channelHeight = totalHeight / ((highestChan - lowestChan) / 2);

    for (int i = lowestChan; i <= highestChan; i++)
    {
        if (i >= 0 && i < 966)
        {

            float xLoc = 225 - channelHeight * (1 - (i % 2));
            float yLoc = lowerBound - ((i - lowestChan - (i % 2)) / 2 * channelHeight);

            if (channelSelectionState[i])
            {
                g.setColour(Colours::white);
                g.fillRect(xLoc, yLoc, channelHeight, channelHeight);
            }

            g.setColour(getChannelColour(i));

            g.fillRect(xLoc+1, yLoc+1, channelHeight-2, channelHeight-2);

        }
        
    }

    // draw annotations
    drawAnnotations(g);

    // draw borders around zoom area

    g.setColour(Colours::darkgrey.withAlpha(0.7f));
    g.fillRect(25, 0, 15, lowerBound - zoomOffset - zoomHeight);
    g.fillRect(25, lowerBound-zoomOffset, 15, zoomOffset+10);

    g.setColour(Colours::darkgrey);
    g.fillRect(100, 0, 250, 22);
    g.fillRect(100, lowerBound + 10, 250, 100);

    if (isOverZoomRegion)
        g.setColour(Colour(25,25,25));
    else
        g.setColour(Colour(55,55,55));

    Path upperBorder;
    upperBorder.startNewSubPath(5, lowerBound - zoomOffset - zoomHeight);
    upperBorder.lineTo(54, lowerBound - zoomOffset - zoomHeight);
    upperBorder.lineTo(100, 16);
    upperBorder.lineTo(350, 16);

    Path lowerBorder;
    lowerBorder.startNewSubPath(5, lowerBound - zoomOffset);
    lowerBorder.lineTo(54, lowerBound - zoomOffset);
    lowerBorder.lineTo(100, lowerBound + 16);
    lowerBorder.lineTo(350, lowerBound + 16);

    g.strokePath(upperBorder, PathStrokeType(2.0));
    g.strokePath(lowerBorder, PathStrokeType(2.0));

    // draw selection zone

    if (isSelectionActive)
    {
        g.setColour(Colours::white.withAlpha(0.5f));
//        g.drawRect(selectionBox);
    }

    if (isOverChannel)
    {
        //std::cout << "YES" << std::endl;
        g.setColour(Colour(55, 55, 55));
        g.setFont(15);
        g.drawMultiLineText(channelInfoString, 280, 310, 250);
    }

    drawLegend(g);

}

void NeuropixInterface::drawAnnotations(Graphics& g)
{
    for (int i = 0; i < annotations.size(); i++)
    {
        bool shouldAppear = false;

        Annotation& a = annotations.getReference(i);

        for (int j = 0; j < a.channels.size(); j++)
        {
            if (j > lowestChan || j < highestChan)
            {
                shouldAppear = true;
                break;
            }
        }   
    
        if (shouldAppear)
        {
            float xLoc = 225 + 30;
            int ch = a.channels[0];

            float midpoint = lowerBound / 2 + 8;

            float yLoc = lowerBound - ((ch - lowestChan - (ch % 2)) / 2 * channelHeight) + 10;

            //if (abs(yLoc - midpoint) < 200)
            yLoc = (midpoint + 3*yLoc)/4;
            a.currentYLoc = yLoc;

            float alpha;

            if (yLoc > lowerBound - 250)
                alpha = (lowerBound - yLoc)/(250.f);
            else if (yLoc < 250)
                alpha = 1.0f - (250.f - yLoc)/200.f;
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

            float xLoc2 = 225 - channelHeight * (1 - (ch % 2)) + channelHeight / 2;
            float yLoc2 = lowerBound - ((ch - lowestChan - (ch % 2)) / 2 * channelHeight) + channelHeight / 2;

            g.drawLine(xLoc - 5, yLoc - 3, xLoc2, yLoc2);
            g.drawLine(xLoc - 5, yLoc - 3, xLoc, yLoc - 3);
        }
    }
}

void NeuropixInterface::drawLegend(Graphics& g)
{
    g.setColour(Colour(55, 55, 55));
    g.setFont(15);

    int xOffset = 100;
    int yOffset = 310;

    switch (visualizationMode)
    {
        case 0: // ENABLED STATE
            g.drawMultiLineText("ENABLED?", xOffset, yOffset, 200);
            g.drawMultiLineText("YES", xOffset+30, yOffset+22, 200);
            g.drawMultiLineText("X OUT", xOffset+30, yOffset+42, 200);
            g.drawMultiLineText("X IN", xOffset+30, yOffset+62, 200);
            g.drawMultiLineText("N/A", xOffset+30, yOffset+82, 200);
            g.drawMultiLineText("AVAIL REF", xOffset+30, yOffset+102, 200);
            g.drawMultiLineText("X REF", xOffset+30, yOffset+122, 200);

            g.setColour(Colours::yellow);
            g.fillRect(xOffset+10, yOffset + 10, 15, 15);

            g.setColour(Colours::goldenrod);
            g.fillRect(xOffset+10, yOffset + 30, 15, 15);

            g.setColour(Colours::maroon);
            g.fillRect(xOffset+10, yOffset + 50, 15, 15);

            g.setColour(Colours::grey);
            g.fillRect(xOffset+10, yOffset + 70, 15, 15);

            g.setColour(Colours::black);
            g.fillRect(xOffset+10, yOffset + 90, 15, 15);

            g.setColour(Colours::brown);
            g.fillRect(xOffset+10, yOffset + 110, 15, 15);

            break;

        case 1: // AP GAIN
            g.drawMultiLineText("AP GAIN", xOffset, yOffset, 200);

            for (int i = 0; i < 8; i++)
            {
                g.drawMultiLineText(String(i), xOffset+30, yOffset+22 + 20*i, 200);
            }

            for (int i = 0; i < 8; i++)
            {
                g.setColour(Colour(25*i,25*i,50));
                g.fillRect(xOffset+10, yOffset + 10 + 20*i, 15, 15);
            }



            break;

        case 2: // LFP GAIN
            g.drawMultiLineText("LFP GAIN", xOffset, yOffset, 200);

            for (int i = 0; i < 8; i++)
            {
                g.drawMultiLineText(String(i), xOffset+30, yOffset+22 + 20*i, 200);
            }

            for (int i = 0; i < 8; i++)
            {
                g.setColour(Colour(66,25*i,35*i));
                g.fillRect(xOffset+10, yOffset + 10 + 20*i, 15, 15);
            }

            break;

        case 3: // REFERENCE
            g.drawMultiLineText("REFERENCE", xOffset, yOffset, 200);

            for (int i = 0; i < referenceComboBox->getNumItems(); i++)
            {
                g.drawMultiLineText(referenceComboBox->getItemText(i), xOffset+30, yOffset+22 + 20*i, 200);
            }


            for (int i = 0; i < referenceComboBox->getNumItems(); i++)
            {
                g.setColour(Colour(200-10*i, 110-10*i, 20*i));
                g.fillRect(xOffset+10, yOffset + 10 + 20*i, 15, 15);
            }

            break;
    }
}

Colour NeuropixInterface::getChannelColour(int i)
{
    if (visualizationMode == 0) // ENABLED STATE
    {
        if (channelStatus[i] == -1) // not available
        {
            return Colours::grey;
        } 
        else if (channelStatus[i] == 0) // disabled
        {
            return Colours::maroon;
        } 
        else if (channelStatus[i] == 1) // enabled
        {
            if (channelOutput[i] == 1)
                return Colours::yellow;
            else
                return Colours::goldenrod;
        } 
        else if (channelStatus[i] == -2) // reference
        {
            return Colours::black;
        } else 
        {
            return Colours::brown; // non-selectable reference
        }
    } else if (visualizationMode == 1) // AP GAIN
    {
        if (channelStatus[i] == -1) // not available
        {
            return Colours::grey;
        } 
        else if (channelStatus[i] < -1) // reference
        {
            return Colours::black;
        }
        else
        {
            return Colour(25*channelApGain[i],25*channelApGain[i],50);
        } 
    } else if (visualizationMode == 2) // LFP GAIN
    {
        if (channelStatus[i] == -1) // not available
        {
            return Colours::grey;
        } 
        else if (channelStatus[i] < -1) // reference
        {
            return Colours::black;
        }
        else
        {
            return Colour(66,25*channelLfpGain[i],35*channelLfpGain[i]);
        } 
    } else if (visualizationMode == 3) // REFERENCE
    {
        if (channelStatus[i] == -1) // not available
        {
            return Colours::grey;
        } 
        else if (channelStatus[i] < -1) // reference
        {
            return Colours::black;
        }
        else
        {
            return Colour(200-10*channelReference[i], 110-10*channelReference[i], 20*channelReference[i]);
        } 
    }
    else if (visualizationMode == 4) // SPIKES
    {
        if (channelStatus[i] == -1) // not available
        {
            return Colours::grey;
        }
        else {
            return channelColours[i];
        }
        
    }
    else if (visualizationMode == 5) // LFP
    {
        if (channelStatus[i] == -1)
        {
            return Colours::grey;
        }
        else {
            return channelColours[i];
        }   
    }
}

void NeuropixInterface::timerCallback()
{
    Random random;
    uint64 timestamp;
    uint64 eventCode;

    int numSamples;

    if (editor->acquisitionIsActive)
        numSamples = 10;
    else
        numSamples = 0;
    
    //

    if (numSamples > 0)
    {
        for (int i = 0; i < 966; i++)
        {
            if (visualizationMode == 4)
                channelColours.set(i, Colour(random.nextInt(256), random.nextInt(256), 0));
            else
                channelColours.set(i, Colour(0, random.nextInt(256), random.nextInt(256)));
        }
    }
    else {
        for (int i = 0; i < 966; i++)
        {
            channelColours.set(i, Colour(20, 20, 20));
        }
    }

    // NOT WORKING:
    //{
    //  ScopedLock(*thread->getMutex());
    //  int numSamples2 = inputBuffer->readAllFromBuffer(displayBuffer, &timestamp, &eventCode, 10000);
    //}
    //

    repaint();
}



int NeuropixInterface::getChannelForElectrode(int ch)
{
    // returns actual mapped channel for individual electrode
    if (ch < 384)
        return ch;
    else if (ch >= 384 && ch < 768)
        return ch - 384;
    else
        return ch - 384 * 2;
    
}

int NeuropixInterface::getConnectionForChannel(int ch)
{

    if (ch < 384)
        return 0;
    else if (ch >= 384 && ch < 768)
        return 1;
    else
        return 2;
  
}

void NeuropixInterface::saveParameters(XmlElement* xml)
{

	std::cout << "Saving Neuropix display." << std::endl;

	XmlElement* xmlNode = xml->createNewChildElement("NEUROPIXELS");

	xmlNode->setAttribute("ZoomHeight", zoomHeight);
	xmlNode->setAttribute("ZoomOffset", zoomOffset);

	xmlNode->setAttribute("apGainValue", apGainComboBox->getText());
	xmlNode->setAttribute("apGainIndex", apGainComboBox->getSelectedId());

	xmlNode->setAttribute("lfpGainValue", lfpGainComboBox->getText());
	xmlNode->setAttribute("lfpGainIndex", lfpGainComboBox->getSelectedId());

	xmlNode->setAttribute("referenceChannel", referenceComboBox->getText());
	xmlNode->setAttribute("referenceChannelIndex", referenceComboBox->getSelectedId());

	xmlNode->setAttribute("filterCut", filterComboBox->getText());
	xmlNode->setAttribute("filterCutIndex", filterComboBox->getSelectedId());

	xmlNode->setAttribute("visualizationMode", visualizationMode);

	xmlNode->setAttribute("info", infoLabel->getText());

	// annotations
	for (int i = 0; i < annotations.size(); i++)
	{
		Annotation& a = annotations.getReference(i);
		XmlElement* annotationNode = xmlNode->createNewChildElement("ANNOTATION");
		annotationNode->setAttribute("text", a.text);
		annotationNode->setAttribute("channel", a.channels[0]);
		annotationNode->setAttribute("R", a.colour.getRed());
		annotationNode->setAttribute("G", a.colour.getGreen());
		annotationNode->setAttribute("B", a.colour.getBlue());
	}
}

void NeuropixInterface::loadParameters(XmlElement* xml)
{

	forEachXmlChildElement(*xml, xmlNode)
	{
		if (xmlNode->hasTagName("NEUROPIXELS"))
		{
			zoomHeight = xmlNode->getIntAttribute("ZoomHeight");
			zoomOffset = xmlNode->getIntAttribute("ZoomOffset");

			int apGainIndex = xmlNode->getIntAttribute("apGainIndex");
			if (apGainIndex != apGainComboBox->getSelectedId())
				apGainComboBox->setSelectedId(apGainIndex, sendNotification);

			int lfpGainIndex = xmlNode->getIntAttribute("lfpGainIndex");
			if (lfpGainIndex != lfpGainComboBox->getSelectedId())
				lfpGainComboBox->setSelectedId(lfpGainIndex, sendNotification);

			int referenceChannelIndex = xmlNode->getIntAttribute("referenceChannelIndex");
			if (referenceChannelIndex != referenceComboBox->getSelectedId())
				referenceComboBox->setSelectedId(referenceChannelIndex, sendNotification);

			int filterCutIndex = xmlNode->getIntAttribute("filterCutIndex");
			if (filterCutIndex != filterComboBox->getSelectedId())
				filterComboBox->setSelectedId(filterCutIndex, sendNotification);

			forEachXmlChildElement(*xmlNode, annotationNode)
			{
				if (annotationNode->hasTagName("ANNOTATION"))
				{
					Array<int> annotationChannels;
					annotationChannels.add(annotationNode->getIntAttribute("channel"));
					annotations.add(Annotation(annotationNode->getStringAttribute("text"),
						annotationChannels,
						Colour(annotationNode->getIntAttribute("R"),
						annotationNode->getIntAttribute("G"),
						annotationNode->getIntAttribute("B"))));
				}
			}

		}
	}

}

// --------------------------------------

Annotation::Annotation(String t, Array<int> chans, Colour c)
{
    text = t;
    channels = chans;

    currentYLoc = -100.f;

    isMouseOver = false;
    isSelected = false;

    colour = c;
}

Annotation::~Annotation()
{

}

// ---------------------------------------

ColorSelector::ColorSelector(NeuropixInterface* np)
{
    npi = np;
    Path p;
    p.addRoundedRectangle(0,0,15,15,3);

    for (int i = 0; i < 6; i++)
    {
        standardColors.add(Colour(245, 245, 245 - 40*i));
        hoverColors.add(   Colour(215, 215, 215 - 40*i));
    }
        

    for (int i = 0; i < 6; i++)
    {
        buttons.add(new ShapeButton(String(i), standardColors[i], hoverColors[i], hoverColors[i]));
        buttons[i]->setShape(p, true, true, false);
        buttons[i]->setBounds(18*i,0,15,15);
        buttons[i]->addListener(this);
        addAndMakeVisible(buttons[i]);
    }

    strings.add("Annotation 1");
    strings.add("Annotation 2");
    strings.add("Annotation 3");
    strings.add("Annotation 4");
    strings.add("Annotation 5");
    strings.add("Annotation 6");

    npi->setAnnotationLabel(strings[0], standardColors[0]);

    activeButton = 0;

}

ColorSelector::~ColorSelector()
{


}

void ColorSelector::buttonClicked(Button* b)
{
    activeButton = buttons.indexOf((ShapeButton*) b);

    npi->setAnnotationLabel(strings[activeButton], standardColors[activeButton]);
}

void ColorSelector::updateCurrentString(String s)
{
    strings.set(activeButton, s);
}

Colour ColorSelector::getCurrentColour()
{
    return standardColors[activeButton];
}