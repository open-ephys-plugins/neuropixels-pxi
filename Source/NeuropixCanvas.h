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

class NeuropixThread;
class NeuropixEditor;

class Probe;

/** 

	Holds the visualizer for additional probe settings

*/
class NeuropixCanvas : public Visualizer
{
public:

	/** Constructor */
	NeuropixCanvas(GenericProcessor*, NeuropixEditor*, NeuropixThread*);

	/** Destructor */
	~NeuropixCanvas();

	/** Fills background */
	void paint(Graphics& g);

	/** Renders the Visualizer on each animation callback cycle */
	void refresh();

	/** Starts animation (not needed for this component) */
	void beginAnimation() override { }

	/** Stops animation (not needed for this component) */
	void endAnimation() override { }

	/** Called when the Visualizer's tab becomes visible after being hidden */
	void refreshState();

	/** Called when the Visualizer is first created, and optionally when
		the parameters of the underlying processor are changed */
	void update();

	/** Sets which interface is active */
	void setSelectedInterface(DataSource* d);

	/** Set which basestation interface is active */
	void setSelectedBasestation(Basestation* b);

	/** Starts animation of sub-interfaces */
	void startAcquisition();

	/** Stops animation of sub-interfaces */
	void stopAcquisition();

	/** Stores probe settings (for copying) */
	void storeProbeSettings(ProbeSettings p);

	/** Gets the most recent probe settings (for copying) */
	ProbeSettings getProbeSettings();

	/** Applies settings to all probes */
	void applyParametersToAllProbes(ProbeSettings p);

	/** Saves custom UI settings */
	void saveCustomParametersToXml(XmlElement* xml) override;

	/** Loads custom UI settings*/
	void loadCustomParametersFromXml(XmlElement* xml) override;

	/** Sets bounds of sub-components*/
	void resized();

	ProbeSettings savedSettings;

	OwnedArray<SettingsInterface> settingsInterfaces;
	Array<DataSource*> dataSources;
	Array<Basestation*> basestations;

	NeuropixEditor* editor;
	NeuropixThread* thread;

	GenericProcessor* processor;

private:

	ScopedPointer<TabbedComponent> topLevelTabComponent;
	Array<TabbedComponent*> basestationTabs;

};
