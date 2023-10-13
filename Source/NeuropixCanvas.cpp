/*
------------------------------------------------------------------

This file is part of the Open Ephys GUI
Copyright(C) 2020 Allen Institute for Brain Science and Open Ephys

------------------------------------------------------------------

This program is free software : you can redistribute itand /or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.If not, see < http://www.gnu.org/licenses/>.

*/

#include "NeuropixCanvas.h"

#include "UI/NeuropixInterface.h"
#include "UI/OneBoxInterface.h"
#include "NeuropixEditor.h"
#include "NeuropixThread.h"

CustomTabComponent::CustomTabComponent(NeuropixEditor* editor_, bool isTopLevel_) :
    TabbedComponent(TabbedButtonBar::TabsAtTop),
    editor(editor_),
    isTopLevel(isTopLevel_)
{
    setTabBarDepth(26);
    setOutline(0);
    setIndent(0);
}



void CustomTabComponent::currentTabChanged(int newCurrentTabIndex, const String& newCurrentTabName)
{
    if (isTopLevel)
    {
        TabbedComponent* currentTab = (TabbedComponent*)getCurrentContentComponent();

        if (currentTab != nullptr)
        {
            CustomViewport* viewport = (CustomViewport*)currentTab->getCurrentContentComponent();

            if (viewport != nullptr)
                editor->selectSource(viewport->settingsInterface->dataSource);
        }
        
    }
    else {
        CustomViewport* viewport = (CustomViewport*)getCurrentContentComponent();
        editor->selectSource(viewport->settingsInterface->dataSource);
    }
    
    std::cout << newCurrentTabIndex << ", " << newCurrentTabName << std::endl;
}


NeuropixCanvas::NeuropixCanvas(GenericProcessor* processor_, NeuropixEditor* editor_, NeuropixThread* thread_) : 
    processor(processor_),
    editor(editor_),
    thread(thread_)

{

    topLevelTabComponent = new CustomTabComponent(editor, true);

    topLevelTabComponent->setColour(TabbedComponent::outlineColourId,
        Colours::darkgrey);
    topLevelTabComponent->setColour(TabbedComponent::backgroundColourId,
        Colours::black.withAlpha(0.0f));
	addAndMakeVisible(topLevelTabComponent);

    Array<Basestation*> availableBasestations = thread->getBasestations();

    int topLevelTabNumber = 0;

    for (auto basestation : availableBasestations)
    {

        CustomTabComponent* basestationTab = new CustomTabComponent(editor, false);
        topLevelTabComponent->addTab(String("Slot " + String(basestation->slot)), 
            Colour(70,70,70),
            basestationTab, 
            true);
        
        basestationTab->setTabBarDepth(26);
        basestationTab->setIndent(0); // gap to leave around the edge
        basestationTab->setOutline(0);
        basestationTabs.add(basestationTab);
        
		int probeCount = basestation->getProbes().size();
        basestations.add(basestation);

        Array<DataSource*> availableDataSources = thread->getDataSources();

		int basestationTabNumber = 0;

        for (auto source : availableDataSources)
        {

            if (source->sourceType == DataSourceType::PROBE && source->basestation == basestation)
            {
                NeuropixInterface* neuropixInterface = new NeuropixInterface(source, thread, editor, this);
                settingsInterfaces.add((SettingsInterface*)neuropixInterface);
                
				basestationTab->addTab(source->getName(), 
                    Colours::darkgrey,
                    neuropixInterface->viewport.get(), 
                    false);

				topLevelTabIndex.add(topLevelTabNumber);
                basestationTabIndex.add(basestationTabNumber);

            }
            else if (source->sourceType == DataSourceType::ADC && source->basestation == basestation)
            {
                OneBoxInterface* oneBoxInterface = new OneBoxInterface(source, thread, editor, this);
                settingsInterfaces.add(oneBoxInterface);
                
                //addChildComponent(oneBoxInterface->viewport.get());
                basestationTab->addTab(source->getName(), 
                    Colours::darkgrey, 
                    oneBoxInterface->viewport.get(), 
                    false);
                
                topLevelTabIndex.add(topLevelTabNumber);
                basestationTabIndex.add(basestationTabNumber);
            }
            // else if (source->sourceType == DataSourceType::DAC)
            // {
            //     DacInterface* dacInterface = new DacInterface(source, thread, editor, this);
             //    settingsInterfaces.add(dacInterface);
             //}

            dataSources.add(source);

            basestationTabNumber += 1;

        }

        if (basestationTabNumber == 0)
        {
            BasestationInterface* basestationInterface = new BasestationInterface(basestation, thread, editor, this);
            settingsInterfaces.add(basestationInterface);
            basestationTab->addTab("Firmware Update",
                Colours::darkgrey,
                basestationInterface->viewport.get(),
                false);
            topLevelTabIndex.add(topLevelTabNumber);
            basestationTabIndex.add(basestationTabNumber);

            dataSources.add(nullptr);
        }

        topLevelTabNumber += 1;
    }


    //neuropixViewport->setViewedComponent(settingsInterfaces.getFirst(), false);
    //addAndMakeVisible(neuropixViewport);

    //settingsInterfaces[0]->viewport->setVisible(true);

    //resized();

    savedSettings.probeType = ProbeType::NONE;
}

NeuropixCanvas::~NeuropixCanvas()
{

}

void NeuropixCanvas::paint(Graphics& g)
{

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
    for (int i = 0; i < settingsInterfaces.size(); i++)
        settingsInterfaces[i]->updateInfoString();
}


void NeuropixCanvas::resized()
{

    topLevelTabComponent->setBounds(0, -3, getWidth(), getHeight()+3);

    //for (int i = 0; i < basestationTabs.size(); i++)
	//	basestationTabs[i]->setBounds(0, 20, getWidth(), getHeight() - 20);
   
    //for (int i = 0; i < settingsInterfaces.size(); i++)
    //{
    ///    settingsInterfaces[i]->viewport->setBounds(10, 0, getWidth() -10, getHeight() - 20);
   // }        
    
}


void NeuropixCanvas::startAcquisition()
{
    for (auto settingsInterface : settingsInterfaces)
        settingsInterface->startAcquisition();
}

void NeuropixCanvas::stopAcquisition()
{
    for (auto settingsInterface : settingsInterfaces)
        settingsInterface->stopAcquisition();
}

void NeuropixCanvas::setSelectedInterface(DataSource* dataSource)
{
    if (dataSource != nullptr)
    {

        int index = dataSources.indexOf(dataSource);
        std::cout << "Index: " << index << std::endl;
		std::cout << "Top Level Tab Index: " << topLevelTabIndex[index] << std::endl;
		std::cout << "Basestation Tab Index: " << basestationTabIndex[index] << std::endl;

        topLevelTabComponent->setCurrentTabIndex(topLevelTabIndex[index], false);
        basestationTabs[topLevelTabIndex[index]]->setCurrentTabIndex(basestationTabIndex[index], false);
    }

}

void NeuropixCanvas::setSelectedBasestation(Basestation* basestation)
{

    if (basestation != nullptr)
    {

        int index = basestations.indexOf(basestation) + dataSources.size();

        for (int i = 0; i < settingsInterfaces.size(); i++)
        {
            if (i == index)
                settingsInterfaces[i]->viewport->setVisible(true);
            else
                settingsInterfaces[i]->viewport->setVisible(false);
        }
    }

}

void NeuropixCanvas::storeProbeSettings(ProbeSettings p)
{
    savedSettings = p;
}

ProbeSettings NeuropixCanvas::getProbeSettings()
{
    return savedSettings;
}

void NeuropixCanvas::applyParametersToAllProbes(ProbeSettings p)
{
    for (auto settingsInterface : settingsInterfaces)
    {
        if (settingsInterface->type == SettingsInterface::PROBE_SETTINGS_INTERFACE)
        {
            if (settingsInterface->applyProbeSettings(p, false))
            {
                NeuropixInterface* ni = (NeuropixInterface*)settingsInterface;
                p.probe = ni->probe;
                p.probe->updateSettings(p);
                thread->updateProbeSettingsQueue(p);
            }
        }

    }

    editor->uiLoader->startThread();

    CoreServices::updateSignalChain(editor);
}

void NeuropixCanvas::saveCustomParametersToXml(XmlElement* xml)
{

    for (int i = 0; i < settingsInterfaces.size(); i++)
        settingsInterfaces[i]->saveParameters(xml);
}

void NeuropixCanvas::loadCustomParametersFromXml(XmlElement* xml)
{

    for (int i = 0; i < settingsInterfaces.size(); i++)
        settingsInterfaces[i]->loadParameters(xml);
}
