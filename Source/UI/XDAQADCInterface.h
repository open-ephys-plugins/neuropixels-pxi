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

#pragma once

#include "../NeuropixComponents.h"
#include "SettingsInterface.h"
#include <VisualizerEditorHeaders.h>

class XDAQAdcChannelButton;

class XDAQADCInterface : public SettingsInterface,
                         public Button::Listener
{
public:
    XDAQADCInterface (DataSource* dataSource_,
                      NeuropixThread* thread_,
                      NeuropixEditor* editor_,
                      NeuropixCanvas* canvas_);

    ~XDAQADCInterface();

    void startAcquisition() override;
    void stopAcquisition() override;

    bool applyProbeSettings (ProbeSettings, bool shouldUpdateProbe = true) override { return false; }

    void saveParameters (XmlElement* xml) override;
    void loadParameters (XmlElement* xml) override;

    void paint (Graphics& g) override;
    void updateInfoString() override {}

    void buttonClicked (Button* button) override;

private:
    OwnedArray<XDAQAdcChannelButton> channels;
    XDAQAdcChannelButton* selectedChannel = nullptr;
};
