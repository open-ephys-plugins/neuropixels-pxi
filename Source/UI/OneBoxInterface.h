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

#ifndef __ONEBOXSETTINGSINTERFACE_H_2C4C2D67__
#define __ONEBOXSETTINGSINTERFACE_H_2C4C2D67__

#include <VisualizerEditorHeaders.h>

#include "../NeuropixComponents.h"
#include "../Probes/OneBoxADC.h"
#include "SettingsInterface.h"

class WavePlayer;
class DataPlayer;
class OneBoxDAC;

enum AdcChannelStatus
{
    AVAILABLE = 0,
    IN_USE = 1
};

class AdcChannelButton : public ToggleButton
{
public:
    /** Constructor */
    AdcChannelButton (int channel);

    /** Called when channel is selected */
    void setSelectedState (bool);

    /** Sets whether the ADC is active*/
    void setStatus (AdcChannelStatus status, int sharedChannel);

    /** Returns channel index*/
    int getChannelIndex() { return channel; }

    /** Whether ADC channel is using the comparator*/
    bool useAsDigitalInput = false;

private:
    void paintButton (Graphics& g, bool isMouseOver, bool isButtonDown);

    AdcChannelStatus status;
    
    int channel;
    int mapToOutput = -1;

    bool selected = false;
};

/** 

    User interface for the OneBox ADC/DAC channels

*/
class OneBoxInterface : public SettingsInterface,
                        public ComboBox::Listener,
                        public Button::Listener
{
public:
    /** Constructor */
    OneBoxInterface (DataSource* dataSource_,
                     NeuropixThread* thread_,
                     NeuropixEditor* editor_,
                     NeuropixCanvas* canvas_);

    /** Destructor */
    ~OneBoxInterface();

    /** Disable UI elements that can't be changed during acquisition */
    void startAcquisition() override;

    /** Re-enable UI elements once acquisition stops */
    void stopAcquisition() override;

    /** Not used */
    bool applyProbeSettings (ProbeSettings, bool shouldUpdateProbe = true) override
    {
        return false;
    }

    /** Save parameters */
    void saveParameters (XmlElement* xml) override;

    /** Load parameters */
    void loadParameters (XmlElement* xml) override;

    /** Draw the interface */
    void paint (Graphics& g);

    /** Set channel as ADC or DAC */
    void setChannelType (int chan, DataSourceType type);

    /** ComboBox callback */
    void comboBoxChanged (ComboBox*);

    /** Button callback */
    void buttonClicked (Button*);

    /** DataSource method */
    void updateInfoString() override {}

    /** Update combo boxes to reflect available channels */
    void updateAvailableChannels();

private:
    OwnedArray<AdcChannelButton> channels;
    AdcChannelButton* selectedChannel;

    std::unique_ptr<ComboBox> rangeSelector;
    std::unique_ptr<ComboBox> digitalInputSelector;
    std::unique_ptr<ComboBox> thresholdSelector;
    std::unique_ptr<ComboBox> triggerSelector;
    std::unique_ptr<ComboBox> mappingSelector;

    std::unique_ptr<WavePlayer> wavePlayer;
    std::unique_ptr<DataPlayer> dataPlayer;

    OneBoxDAC* dac;
    OneBoxADC* adc;
};

#endif //__ONEBOXSETTINGSINTERFACE_H_2C4C2D67__