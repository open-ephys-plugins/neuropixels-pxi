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
#include <array>

class NeuropixEditor;
class NeuropixCanvas;
class NeuropixThread;
class ProbeBrowser;

class SurveyProbePanel : public Component
{
public:
    explicit SurveyProbePanel (Probe* p);
    ~SurveyProbePanel() override;

    void refresh();
    void paint (Graphics& g) override;
    void resized() override;

    ProbeBrowser* getProbeBrowser() const { return probeBrowser.get(); }
    Probe* getProbe() const { return probe; }
    void setMaxPeakToPeakAmplitude (float amplitude);

    int getOptimalWidth () const;

    static constexpr int width = 460;
    static constexpr int minHeight = 300;

private:
    Probe* probe { nullptr };
    std::unique_ptr<Label> title;
    std::unique_ptr<Label> subTitle;
    std::unique_ptr<ProbeBrowser> probeBrowser;
    std::unique_ptr<Label> placeholder;
};

/**

    Expands/collapses the settings panel of the SurveyInterface

*/
class PanelToggleButton : public Button
{
public:
    /** Constructor */
    PanelToggleButton();

    /** Destructor */
    ~PanelToggleButton() {}

    void paintButton (Graphics& g, bool isMouseOver, bool isButtonDown) override;

private:
    Path collapsePath;
    Path expandPath;
};

// Background worker that executes the survey without blocking the UI.
struct SurveyTarget
{
    Probe* probe { nullptr };
    Array<String> electrodeConfigs;
    Array<int> electrodesToRestore;
    Array<Bank> banks;
    Array<int> shanks;
    int shankCount { 1 };
    bool surveyComplete { false };
    bool originalFilterState { true };
    bool originalCARState { true };
};

class SurveyRunner : public ThreadWithProgressWindow
{
public:
    SurveyRunner (NeuropixThread* t, NeuropixEditor* e, const Array<SurveyTarget>& targetsToSurvey, float secondsPerConfig, bool recordDuringSurvey);
    ~SurveyRunner() {}

    void run() override;

private:
    NeuropixThread* thread;
    NeuropixEditor* editor;
    Array<SurveyTarget> targets;
    float secondsPer;
    int threadSleepMs { 50 };
    bool recordDuringSurvey;
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
    void saveParameters (XmlElement*) override;
    void loadParameters (XmlElement*) override;
    void updateInfoString() override;

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

    static String bankToString (Bank b);

private:
    void refreshProbeList();
    void rebuildProbePanels();
    void layoutProbePanels();
    String banksSummary (const Array<Bank>&) const;
    String shanksSummary (const Array<int>&, int shankCount) const;
    void showBanksSelector (int row, Component* anchor);
    void showShanksSelector (int row, Component* anchor);
    void launchSurvey();
    void saveSurveyResultsToJson (const Array<SurveyTarget>& targets, float secondsPerConfig);
    void applyMaxAmplitudeToPanels();

    NeuropixThread* thread;
    NeuropixEditor* editor;
    NeuropixCanvas* canvas;

    // Controls
    std::unique_ptr<Label> titleLabel;
    std::unique_ptr<PanelToggleButton> panelToggleButton;
    std::unique_ptr<TextButton> runButton;
    std::unique_ptr<ToggleButton> recordingToggleButton;
    std::unique_ptr<UtilityButton> activityViewFilterToggle;
    std::unique_ptr<UtilityButton> activityViewCARToggle;
    std::unique_ptr<ComboBox> secondsPerBankComboBox;
    std::unique_ptr<ComboBox> amplitudeRangeComboBox;
    std::unique_ptr<UtilityButton> saveButton;

    bool leftPanelCollapsed { false };

    // Table for probe selection and granular bank/shank selection
    std::unique_ptr<TableListBox> table;
    std::unique_ptr<Viewport> probeViewport;
    std::unique_ptr<Component> probeViewportContent;
    OwnedArray<SurveyProbePanel> probePanels;
    int probePanelsWidth { 0 };

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
        Array<String> electrodeConfigs;
        bool selected { true };
        Array<Bank> availableBanks;
        int shankCount { 1 };
        Array<Bank> chosenBanks; // empty means all
        Array<int> chosenShanks; // empty means all
    };

    Array<RowState> rows;
    bool isSurveyRunning { false };
    Array<SurveyTarget> lastSurveyTargets;
    float lastSurveySecondsPerConfig { 2.0f };
    float currentMaxPeakToPeak { 500.0f };
    Array<float> amplitudeOptions { 250.0f, 500.0f, 750.0f, 1000.0f };
    Array<float> timeOptions { 2.0f, 5.0f, 10.0f, 30.0f, 60.0f, 300.0f, 600.0f }; // in seconds
};
