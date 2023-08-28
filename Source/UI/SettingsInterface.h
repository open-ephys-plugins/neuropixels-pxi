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

#ifndef __SETTINGSINTERFACE_H_2C4C2D67__
#define __SETTINGSINTERFACE_H_2C4C2D67__

#include <VisualizerEditorHeaders.h>

#include "../NeuropixComponents.h"

class NeuropixThread;
class NeuropixEditor;
class NeuropixCanvas;

class SettingsInterface : public Component
{
public:

    enum Type
    {
        PROBE_SETTINGS_INTERFACE,
        ONEBOX_SETTINGS_INTERFACE,
        BASESTATION_SETTINGS_INTERFACE,
        UNKNOWN_SETTINGS_INTERFACE
    };

    SettingsInterface(DataSource* dataSource_, NeuropixThread* thread_, NeuropixEditor* editor_, NeuropixCanvas* canvas_)
    {
        dataSource = dataSource_;
        editor = editor_;
        canvas = canvas_;
        thread = thread_;
    }
	~SettingsInterface() {}

	virtual void startAcquisition() = 0;
	virtual void stopAcquisition() = 0;

	virtual bool applyProbeSettings(ProbeSettings, bool shouldUpdateProbe = true) = 0;

	virtual void saveParameters(XmlElement* xml) = 0;
	virtual void loadParameters(XmlElement* xml) = 0;

    virtual void updateInfoString() = 0;

    Type type = UNKNOWN_SETTINGS_INTERFACE;

protected:

	NeuropixThread* thread;
	NeuropixEditor* editor;
	NeuropixCanvas* canvas;
	DataSource* dataSource;
};

#endif //__SETTINGSINTERFACE_H_2C4C2D67__