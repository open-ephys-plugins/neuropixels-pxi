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
#include "NeuropixEditor.h"
#include "NeuropixThread.h"
#include "Utils.h"

NeuropixCanvas::NeuropixCanvas(GenericProcessor* processor_, NeuropixEditor* editor_, NeuropixThread* thread_) : 
    processor(processor_),
    editor(editor_),
    thread(thread_)

{
    neuropixViewport = new Viewport();
    neuropixViewport->setScrollBarsShown(true, true, true, true);

    Array<Probe*> available_probes = thread->getProbes();

    for (auto probe : available_probes)
    {
        NeuropixInterface* neuropixInterface = new NeuropixInterface(probe, thread, editor, this);
        neuropixInterfaces.add(neuropixInterface);
        probes.add(probe);
    }

    neuropixViewport->setViewedComponent(neuropixInterfaces.getFirst(), false);
    addAndMakeVisible(neuropixViewport);

    resized();
    //update();

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

}

void NeuropixCanvas::beginAnimation()
{
}

void NeuropixCanvas::endAnimation()
{
}

void NeuropixCanvas::resized()
{
   
    for (int i = 0; i < neuropixInterfaces.size(); i++)
        neuropixInterfaces[i]->setBounds(0, 0, 1000, 820);

    neuropixViewport->setBounds(10, 10, getWidth(), getHeight());

    // why is this not working?
    neuropixViewport->setScrollBarsShown(true, true, true, true);
    neuropixViewport->setScrollBarThickness(20);
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

void NeuropixCanvas::startAcquisition()
{
    for (auto npInterface : neuropixInterfaces)
        npInterface->startAcquisition();
}

void NeuropixCanvas::stopAcquisition()
{
    for (auto npInterface : neuropixInterfaces)
        npInterface->stopAcquisition();
}

void NeuropixCanvas::setSelectedProbe(Probe* probe)
{
    if (probe != nullptr)
    {

        int index = probes.indexOf(probe);

        if (index > -1)
            neuropixViewport->setViewedComponent(neuropixInterfaces[index], false);
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
    for (auto npInterface : neuropixInterfaces)
    {
        npInterface->applyProbeSettings(p);
    }
}

void NeuropixCanvas::saveVisualizerParameters(XmlElement* xml)
{
    editor->saveEditorParameters(xml);

    LOGD("Saved ", neuropixInterfaces.size(), " interfaces.");

    for (int i = 0; i < neuropixInterfaces.size(); i++)
        neuropixInterfaces[i]->saveParameters(xml);
}

void NeuropixCanvas::loadVisualizerParameters(XmlElement* xml)
{

    editor->loadEditorParameters(xml);

    LOGD("Loaded ", neuropixInterfaces.size(), " interfaces.");

    for (int i = 0; i < neuropixInterfaces.size(); i++)
        neuropixInterfaces[i]->loadParameters(xml);
}