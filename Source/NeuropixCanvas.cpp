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

#include "NeuropixCanvas.h"

#include "NeuropixEditor.h"
#include "NeuropixThread.h"
#include "UI/NeuropixInterface.h"
#include "UI/OneBoxInterface.h"

SettingsUpdater* SettingsUpdater::currentThread = nullptr;

CustomTabButton::CustomTabButton (const String& name, TabbedComponent* parent, bool isTopLevel_) : TabBarButton (name, parent->getTabbedButtonBar()),
                                                                                                   isTopLevel (isTopLevel_)
{
}

void CustomTabButton::paintButton (Graphics& g,
                                   bool isMouseOver,
                                   bool isMouseDown)
{
    Colour tabColour = findColour (ThemeColours::componentBackground).darker (isTopLevel ? 0.2f : 0.0f);
    getTabbedButtonBar().setTabBackgroundColour (getIndex(), tabColour);

    getLookAndFeel().drawTabButton (*this, g, isMouseOver, isMouseDown);
}

CustomTabComponent::CustomTabComponent (NeuropixEditor* editor_, bool isTopLevel_) : TabbedComponent (TabbedButtonBar::TabsAtTop),
                                                                                     editor (editor_),
                                                                                     isTopLevel (isTopLevel_)
{
    setTabBarDepth (26);
    setOutline (0);
    setIndent (0);
}

void CustomTabComponent::currentTabChanged (int newCurrentTabIndex, const String& newCurrentTabName)
{
    if (isTopLevel)
    {
        TabbedComponent* currentTab = (TabbedComponent*) getCurrentContentComponent();

        if (currentTab != nullptr)
        {
            CustomViewport* viewport = (CustomViewport*) currentTab->getCurrentContentComponent();

            if (viewport != nullptr)
                editor->selectSource (viewport->settingsInterface->dataSource);
        }
    }
    else
    {
        CustomViewport* viewport = (CustomViewport*) getCurrentContentComponent();

        if (viewport != nullptr)
            editor->selectSource (viewport->settingsInterface->dataSource);
    }

    // std::cout << newCurrentTabIndex << ", " << newCurrentTabName << std::endl;
}

NeuropixCanvas::NeuropixCanvas (GenericProcessor* processor_, NeuropixEditor* editor_, NeuropixThread* thread_) : Visualizer (processor_),
                                                                                                                  editor (editor_),
                                                                                                                  thread (thread_)

{
    topLevelTabComponent = new CustomTabComponent (editor, true);
    addAndMakeVisible (topLevelTabComponent);

    Array<Basestation*> availableBasestations = thread->getBasestations();

    int topLevelTabNumber = 0;

    for (auto basestation : availableBasestations)
    {
        CustomTabComponent* basestationTab = new CustomTabComponent (editor, false);
        topLevelTabComponent->addTab (String (" Slot " + String (basestation->slot) + " "),
                                      findColour (ThemeColours::componentBackground).darker (0.2f),
                                      basestationTab,
                                      true);

        basestationTab->setTabBarDepth (26);
        basestationTab->setIndent (0); // gap to leave around the edge
        basestationTab->setOutline (0);
        basestationTabs.add (basestationTab);

        int probeCount = basestation->getProbes().size();
        basestations.add (basestation);

        Array<DataSource*> availableDataSources = thread->getDataSources();

        int basestationTabNumber = 0;

        for (auto source : availableDataSources)
        {
            if (source->sourceType == DataSourceType::PROBE && source->basestation == basestation)
            {
                NeuropixInterface* neuropixInterface = new NeuropixInterface (source, thread, editor, this);
                settingsInterfaces.add ((SettingsInterface*) neuropixInterface);

                basestationTab->addTab (" " + source->getName() + " ",
                                        findColour (ThemeColours::componentBackground),
                                        neuropixInterface->viewport.get(),
                                        false);

                topLevelTabIndex.add (topLevelTabNumber);
                basestationTabIndex.add (basestationTabNumber++);
            }
            else if (source->sourceType == DataSourceType::ADC && source->basestation == basestation)
            {
                OneBoxInterface* oneBoxInterface = new OneBoxInterface (source, thread, editor, this);
                settingsInterfaces.add (oneBoxInterface);

                //addChildComponent(oneBoxInterface->viewport.get());
                basestationTab->addTab (" " + source->getName() + " ",
                                        findColour (ThemeColours::componentBackground),
                                        oneBoxInterface->viewport.get(),
                                        false);

                topLevelTabIndex.add (topLevelTabNumber);
                basestationTabIndex.add (basestationTabNumber++);
            }
            // else if (source->sourceType == DataSourceType::DAC)
            // {
            //     DacInterface* dacInterface = new DacInterface(source, thread, editor, this);
            //    settingsInterfaces.add(dacInterface);
            //}

            dataSources.add (source);
        }

        if (basestationTabNumber == 0)
        {
            BasestationInterface* basestationInterface = new BasestationInterface (basestation, thread, editor, this);
            settingsInterfaces.add (basestationInterface);
            basestationTab->addTab (" Firmware Update ",
                                    findColour (ThemeColours::componentBackground),
                                    basestationInterface->viewport.get(),
                                    false);
            topLevelTabIndex.add (topLevelTabNumber);
            basestationTabIndex.add (basestationTabNumber++);

            dataSources.add (nullptr);
        }

        topLevelTabNumber += 1;
    }

    topLevelTabComponent->setCurrentTabIndex (topLevelTabNumber - 1);

    //neuropixViewport->setViewedComponent(settingsInterfaces.getFirst(), false);
    //addAndMakeVisible(neuropixViewport);

    //settingsInterfaces[0]->viewport->setVisible(true);

    //resized();

    savedSettings.probeType = ProbeType::NONE;
}

NeuropixCanvas::~NeuropixCanvas()
{
}

void NeuropixCanvas::paint (Graphics& g)
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

void NeuropixCanvas::updateSettings()
{
    for (int i = 0; i < settingsInterfaces.size(); i++)
        settingsInterfaces[i]->updateInfoString();

    for (int i = 0; i < topLevelTabComponent->getNumTabs(); i++)
    {
        CustomTabComponent* t = (CustomTabComponent*) topLevelTabComponent->getTabContentComponent (i);

        for (int j = 0; j < t->getNumTabs(); j++)
        {
            if (t->getTabContentComponent (j) != nullptr)
            {
                CustomViewport* v = (CustomViewport*) t->getTabContentComponent (j);

                if (v != nullptr)
                {
                    DataSource* dataSource = v->settingsInterface->dataSource;

                    if (dataSource != nullptr)
                        t->setTabName (j, " " + dataSource->getName() + " ");
                    else
                        t->setTabName (j, "Firmware update");
                }
            }
        }
    }
}

void NeuropixCanvas::resized()
{
    topLevelTabComponent->setBounds (0, -3, getWidth(), getHeight() + 3);
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

void NeuropixCanvas::setSelectedInterface (DataSource* dataSource)
{
    if (dataSource != nullptr)
    {
        int index = dataSources.indexOf (dataSource);
        // std::cout << "Index: " << index << std::endl;
        //std::cout << "Top Level Tab Index: " << topLevelTabIndex[index] << std::endl;
        //std::cout << "Basestation Tab Index: " << basestationTabIndex[index] << std::endl;

        topLevelTabComponent->setCurrentTabIndex (topLevelTabIndex[index], false);
        basestationTabs[topLevelTabIndex[index]]->setCurrentTabIndex (basestationTabIndex[index], false);
    }
}

void NeuropixCanvas::setSelectedBasestation (Basestation* basestation)
{
    if (basestation != nullptr)
    {
        int index = basestations.indexOf (basestation);

        topLevelTabComponent->setCurrentTabIndex (index, false);
    }
}

void NeuropixCanvas::storeProbeSettings (ProbeSettings p)
{
    savedSettings = p;
}

ProbeSettings NeuropixCanvas::getProbeSettings()
{
    return savedSettings;
}

void NeuropixCanvas::applyParametersToAllProbes (ProbeSettings p)
{
    std::unique_ptr<SettingsUpdater> settingsUpdater = std::make_unique<SettingsUpdater> (this, p);
    /*
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
    */

    CoreServices::updateSignalChain (editor);
}

void NeuropixCanvas::saveCustomParametersToXml (XmlElement* xml)
{
    for (int i = 0; i < settingsInterfaces.size(); i++)
        settingsInterfaces[i]->saveParameters (xml);
}

void NeuropixCanvas::loadCustomParametersFromXml (XmlElement* xml)
{
    for (int i = 0; i < settingsInterfaces.size(); i++)
        settingsInterfaces[i]->loadParameters (xml);
}

SettingsUpdater::SettingsUpdater (NeuropixCanvas* canvas_, ProbeSettings p) : ThreadWithProgressWindow ("Updating settings", true, true),
                                                                              canvas (canvas_),
                                                                              settings (p),
                                                                              numProbesToUpdate (0)
{
    SettingsUpdater::currentThread = this;

    // Only update probes of the same type and with different names
    for (auto settingsInterface : canvas->settingsInterfaces)
    {
        if (settingsInterface->type == SettingsInterface::PROBE_SETTINGS_INTERFACE)
        {
            NeuropixInterface* ni = (NeuropixInterface*) settingsInterface;
            if (ni->probe->type == settings.probe->type && ni->probe->getName() != settings.probe->getName())
            {
                ni->applyProbeSettings (settings, false);
                numProbesToUpdate++;
            }
        }
    }

    if (numProbesToUpdate > 0)
    {
        String message = "Found " + String (numProbesToUpdate) + (numProbesToUpdate > 1 ? " probes " : " probe ") + "to update";
        this->setStatusMessage (message);
        runThread();
    }
    else
    {
        CoreServices::sendStatusMessage ("No probes of same type found, not applying settings.");
    }
}

void SettingsUpdater::run()
{
    // Pause to show how many probes were detected and are being updated
    //Time::waitForMillisecondCounter(Time::getMillisecondCounter() + 1000);
    int count = 0;
    for (auto settingsInterface : canvas->settingsInterfaces)
    {
        if (settingsInterface->type == SettingsInterface::PROBE_SETTINGS_INTERFACE)
        {
            NeuropixInterface* ni = (NeuropixInterface*) settingsInterface;
            if (ni->probe->type == settings.probe->type && settings.probe->getName() != ni->probe->getName())
            {
                count++;
                this->setStatusMessage ("Updating settings for " + ni->probe->getName() + " (" + String (count) + " of " + String (numProbesToUpdate) + ")");
                ni->updateProbeSettingsInBackground();
                float updateTime = 1000.0; // milliseconds
                for (float fraction = 0.0; fraction < 1.0; fraction += 0.01)
                {
                    currentThread->setProgress (float (count + fraction - 1) / float (numProbesToUpdate));
                    Time::waitForMillisecondCounter (Time::getMillisecondCounter() + updateTime / 100.0);
                }
                while (canvas->editor->uiLoader->isThreadRunning())
                    Time::waitForMillisecondCounter (10);
            }
        }
    }
    CoreServices::sendStatusMessage ("Applied saved settings to all probes of same type.");
}
