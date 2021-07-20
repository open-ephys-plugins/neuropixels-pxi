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

#include <VisualizerEditorHeaders.h>

#include "NeuropixComponents.h"
#include "UI/SettingsInterface.h"
#include "UI/NeuropixInterface.h"

class NeuropixThread;
class NeuropixEditor;

class Probe;

class NeuropixCanvas : public Visualizer, public Button::Listener
{
public:
	NeuropixCanvas(GenericProcessor*, NeuropixEditor*, NeuropixThread*);
	~NeuropixCanvas();

	void paint(Graphics& g);

	// needed for Visualizer class
	void refresh();

	void beginAnimation();
	void endAnimation();

	void refreshState();
	void update();

	void setParameter(int, float);
	void setParameter(int, int, int, float);
	void buttonClicked(Button* button);
	// end Visualizer class methods

	void setSelectedProbe(Probe* p);

	void startAcquisition();
	void stopAcquisition();

	void storeProbeSettings(ProbeSettings p);
	ProbeSettings getProbeSettings();
	void applyParametersToAllProbes(ProbeSettings p);

	ProbeSettings savedSettings;

	void saveVisualizerParameters(XmlElement* xml);
	void loadVisualizerParameters(XmlElement* xml);

	void resized();

	ScopedPointer<Viewport> neuropixViewport;
	OwnedArray<NeuropixInterface> neuropixInterfaces;
	Array<Probe*> probes;

	NeuropixEditor* editor;
	NeuropixThread* thread;

	GenericProcessor* processor;

};