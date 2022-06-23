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
#ifndef __ONEBOXSETTINGSINTERFACE_H_2C4C2D67__
#define __ONEBOXSETTINGSINTERFACE_H_2C4C2D67__

#include <VisualizerEditorHeaders.h>

#include "SettingsInterface.h"
#include "../NeuropixComponents.h"
#include "../Probes/OneBoxADC.h"

class WavePlayer;
class DataPlayer;
class OneBoxDAC;

enum AdcChannelStatus {
    AVAILABLE = 0,
    IN_USE = 1
};


class AdcChannelButton : public ToggleButton
{
public:

    enum AdcInputRange {
        PLUS_MINUS_TWO_PT_5_V = 1,
        PLUS_MINUS_FIVE_V = 2,
        PLUS_MINUS_TEN_V = 3
    };

    enum AdcThreshold {
        ONE_V = 1,
        THREE_V = 2
    };

    enum TriggerWavePlayer {
        NO = 1,
        YES = 2
    };

    enum AvailableDacs {
        NO_DAC = 1,
        DAC0 = 2,
        DAC1 = 3,
        DAC2 = 4,
        DAC3 = 5,
        DAC4 = 6,
        DAC5 = 7,
        DAC6 = 8,
        DAC7 = 9,
        DAC8 = 10,
        DAC9 = 11,
        DAC10 = 12,
        DAC11 = 13
    };

    enum AvailableAdcs {
        NO_ADC = 1,
        ADC0 = 2,
        ADC1 = 3,
        ADC2 = 4,
        ADC3 = 5,
        ADC4 = 6,
        ADC5 = 7,
        ADC6 = 8,
        ADC7 = 9,
        ADC8 = 10,
        ADC9 = 11,
        ADC10 = 12,
        ADC11 = 13
    };

    AdcChannelButton(int channel);

    void setSelectedState(bool);

    void setStatus(AdcChannelStatus status, AvailableAdcs sharedChannel = NO_ADC);

    int inputRange;
    int threshold;
    int triggerWavePlayer;
    int mapToOutput;
    int inputSharedBy;
    int channel;

private:
    void paintButton(Graphics& g, bool isMouseOver, bool isButtonDown);

    AdcChannelStatus status;

    bool selected;

};

class OneBoxInterface : public SettingsInterface, 
                     public ComboBox::Listener, 
                     public Button::Listener
{
public:
    OneBoxInterface(DataSource* dataSource_, NeuropixThread* thread_, NeuropixEditor* editor_, NeuropixCanvas* canvas_);

    ~OneBoxInterface();

    void startAcquisition() override;
    void stopAcquisition() override;

    bool applyProbeSettings(ProbeSettings, bool shouldUpdateProbe = true) override { return false; }

    void saveParameters(XmlElement* xml) override;
    void loadParameters(XmlElement* xml) override;

    void paint(Graphics& g);

    void enableInput(int chan);
    void disableInput(int chan);

    void comboBoxChanged(ComboBox*);
    void buttonClicked(Button*);

    void updateInfoString() { }

private:

    void updateMappingSelector();

    OwnedArray<AdcChannelButton> channels;
    AdcChannelButton* selectedChannel;

    ScopedPointer<ComboBox> rangeSelector;
    ScopedPointer<ComboBox> thresholdSelector;
    ScopedPointer<ComboBox> triggerSelector;
    ScopedPointer<ComboBox> mappingSelector;

    ScopedPointer<WavePlayer> wavePlayer;
    ScopedPointer<DataPlayer> dataPlayer;

    OneBoxDAC* dac;
    OneBoxADC* adc;

};

#endif //__ONEBOXSETTINGSINTERFACE_H_2C4C2D67__