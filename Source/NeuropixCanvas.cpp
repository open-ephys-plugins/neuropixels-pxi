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


NeuropixCanvas::NeuropixCanvas(GenericProcessor* processor_, NeuropixEditor* editor_, NeuropixThread* thread_) : 
    processor(processor_),
    editor(editor_),
    thread(thread_)

{

    Array<DataSource*> availableDataSources = thread->getDataSources();

    for (auto source : availableDataSources)
    {

        if (source->sourceType == DataSourceType::PROBE)
        {
            NeuropixInterface* neuropixInterface = new NeuropixInterface(source, thread, editor, this);
            settingsInterfaces.add((SettingsInterface*) neuropixInterface);
            addChildComponent(neuropixInterface->viewport.get());
        
            
        }
        else if (source->sourceType == DataSourceType::ADC)
        {
            OneBoxInterface* oneBoxInterface = new OneBoxInterface(source, thread, editor, this);
            settingsInterfaces.add(oneBoxInterface);
            addChildComponent(oneBoxInterface->viewport.get());
        }
       // else if (source->sourceType == DataSourceType::DAC)
       // {
       //     DacInterface* dacInterface = new DacInterface(source, thread, editor, this);
        //    settingsInterfaces.add(dacInterface);
        //}

        dataSources.add(source);
        
    }

    Array<Basestation*> availableBasestations = thread->getBasestations();

    for (auto basestation : availableBasestations)
    {
        BasestationInterface* basestationInterface = new BasestationInterface(basestation, thread, editor, this);
        settingsInterfaces.add(basestationInterface);
        basestations.add(basestation);
        addChildComponent(basestationInterface->viewport.get());
    }

    //neuropixViewport->setViewedComponent(settingsInterfaces.getFirst(), false);
    //addAndMakeVisible(neuropixViewport);

    settingsInterfaces[0]->viewport->setVisible(true);

    resized();

    savedSettings.probeType = ProbeType::NONE;
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
    for (int i = 0; i < settingsInterfaces.size(); i++)
        settingsInterfaces[i]->updateInfoString();
}


void NeuropixCanvas::resized()
{
   
    for (int i = 0; i < settingsInterfaces.size(); i++)
    {
        settingsInterfaces[i]->viewport->setBounds(10, 10, getWidth() - 10, getHeight() - 10);
    }        
    
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

        for (int i = 0; i < settingsInterfaces.size(); i++)
        {
            if (i == index)
                settingsInterfaces[i]->viewport->setVisible(true);
            else
                settingsInterfaces[i]->viewport->setVisible(false);
        }
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
