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

#include "SettingsInterface.h"
#include <VisualizerEditorHeaders.h>

class NeuropixEditor;
class NeuropixCanvas;
class NeuropixThread;

// Background worker that executes the survey without blocking the UI.
struct SurveyTarget
{
    Probe* probe { nullptr };
    Array<Bank> banks;
    Array<int> shanks;
};

class SurveyRunner : public ThreadWithProgressWindow
{
public:
    SurveyRunner (NeuropixThread* t, NeuropixEditor* e, const Array<SurveyTarget>& targetsToSurvey, float secondsPerConfig);
    ~SurveyRunner() {}

    void run() override;

    bool wasCancelled() const { return threadShouldExit(); }

private:
    NeuropixThread* thread;
    NeuropixEditor* editor;
    Array<SurveyTarget> targets;
    float secondsPer;
};

// Simple UI to configure and launch a survey across probes
class SurveyInterface : public SettingsInterface,
                        public Button::Listener,
                        public ComboBox::Listener,
                        public TableListBoxModel
{
public:
    SurveyInterface (NeuropixThread* thread, NeuropixEditor* editor, NeuropixCanvas* canvas);
    ~SurveyInterface() {}

    // SettingsInterface overrides (no underlying DataSource for a top-level Survey panel)
    void startAcquisition() override;
    void stopAcquisition() override;
    bool applyProbeSettings (ProbeSettings, bool) override { return false; }
    void saveParameters (XmlElement*) override {}
    void loadParameters (XmlElement*) override {}
    void updateInfoString() override {}

    void paint (Graphics& g) override;
    void resized() override;

    // Listeners
    void buttonClicked (Button* b) override;
    void comboBoxChanged (ComboBox* cb) override;

    // TableListBoxModel
    int getNumRows() override;
    void paintRowBackground (Graphics& g, int rowNumber, int width, int height, bool rowIsSelected) override;
    void paintCell (Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override;
    Component* refreshComponentForCell (int rowNumber, int columnId, bool isRowSelected, Component* existingComponentToUpdate) override;
    void cellClicked (int rowNumber, int columnId, const MouseEvent&) override;

private:
    void refreshProbeList();
    String banksSummary (const Array<Bank>&) const;
    String shanksSummary (const Array<int>&, int shankCount) const;
    static String bankToString (Bank b);
    void showBanksSelector (int row, Component* anchor);
    void showShanksSelector (int row, Component* anchor);
    void launchSurvey();

    NeuropixThread* thread;
    NeuropixEditor* editor;
    NeuropixCanvas* canvas;

    // Controls
    std::unique_ptr<Label> titleLabel;
    std::unique_ptr<UtilityButton> runButton;
    std::unique_ptr<Slider> secondsPerBankSlider;

    // Table for probe selection and granular bank/shank selection
    std::unique_ptr<TableListBox> table;
    enum Columns
    {
        ColSelect = 1,
        ColName = 2,
        ColType = 3,
        ColBanks = 4,
        ColShanks = 5
    };

    struct RowState
    {
        Probe* probe { nullptr };
        bool selected { true };
        Array<Bank> availableBanks;
        int shankCount { 1 };
        Array<Bank> chosenBanks; // empty means all
        Array<int> chosenShanks; // empty means all
    };

    Array<RowState> rows;
};
