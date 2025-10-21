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

#include "SurveyInterface.h"

#include "../NeuropixCanvas.h"
#include "../NeuropixEditor.h"
#include "../NeuropixThread.h"
#include "ColourScheme.h"
#include "NeuropixInterface.h"
#include "ProbeBrowser.h"

#include <cmath>
#include <functional>
#include <utility>

static Bank stringToBank (const String& text)
{
    // if text is from "A" to "M", map to Bank::A to Bank::M
    if (text.length() == 1)
    {
        const juce_wchar c = text.toUpperCase()[0];
        if (c >= 'A' && c <= 'M')
            return static_cast<Bank> (c - 'A');
    }

    return Bank::NONE;
}
class BankSelectorComponent : public Component,
                              private Button::Listener
{
public:
    BankSelectorComponent (const Array<Bank>& available, const StringArray& labels, const Array<Bank>& initiallySelected, std::function<void (const Array<Bank>&)> onChange)
        : availableBanks (available),
          selection (initiallySelected),
          onSelectionChanged (std::move (onChange))
    {
        allButton = std::make_unique<TextButton> ("ALL");
        allButton->setClickingTogglesState (false);
        allButton->addListener (this);
        addAndMakeVisible (*allButton);

        for (int i = 0; i < availableBanks.size(); ++i)
        {
            const auto label = (isPositiveAndBelow (i, labels.size()) ? labels[i] : String (i));
            auto* btn = new UtilityButton (label);
            btn->setClickingTogglesState (true);
            btn->setRadius (2.0f);
            btn->addListener (this);
            btn->setComponentID (String (i));
            bankButtons.add (btn);
            addAndMakeVisible (btn);
        }

        refreshButtonStates();
        updatePreferredSize();
    }

    ~BankSelectorComponent() override = default;

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (5);
        auto footer = bounds.removeFromBottom (24);

        const int numButtons = bankButtons.size();
        const int gap = 1;
        const int buttonWidth = 20;
        const int buttonHeight = 20;

        // Lay out buttons in a single horizontal row
        for (int i = 0; i < numButtons; ++i)
        {
            if (auto* btn = bankButtons[i])
            {
                const int x = bounds.getX() + i * (buttonWidth + gap);
                const int y = bounds.getY();
                btn->setBounds (x, y, buttonWidth, buttonHeight);
            }
        }

        allButton->setBounds (footer.withSizeKeepingCentre (60, 20));
    }

private:
    void updatePreferredSize()
    {
        const int numButtons = bankButtons.size();
        const int gap = 1;
        const int buttonWidth = 20;
        const int buttonHeight = 20;

        const int width = jmax (10, numButtons * buttonWidth + jmax (0, numButtons - 1) * gap + 10);
        const int height = buttonHeight + 10 + 24;
        setSize (width, height);
    }

    void refreshButtonStates()
    {
        for (int i = 0; i < bankButtons.size(); ++i)
        {
            const Bank bank = availableBanks[i];
            const bool isSelected = selection.contains (bank) || (selection.isEmpty());
            bankButtons[i]->setToggleState (isSelected, dontSendNotification);
        }
    }

    void notifySelectionChanged()
    {
        if (onSelectionChanged != nullptr)
            onSelectionChanged (selection);
    }

    void buttonClicked (Button* button) override
    {
        if (button == allButton.get())
        {
            selection.clear();
            refreshButtonStates();
            notifySelectionChanged();
            return;
        }

        if (! bankButtons.contains ((UtilityButton*) button))
            return;

        const int idx = bankButtons.indexOf ((UtilityButton*) button);

        const Bank bank = availableBanks[idx];
        if (selection.contains (bank))
        {
            selection.removeFirstMatchingValue (bank);
        }
        else if (selection.isEmpty())
        {
            for (const auto b : availableBanks)
                if (b != bank)
                    selection.add (b);
        }
        else
        {
            selection.add (bank);
        }

        selection.sort();

        // if selection contains all available banks, treat as "all" (empty selection)
        if (selection.size() == availableBanks.size())
            selection.clear();

        refreshButtonStates();
        notifySelectionChanged();
    }

    Array<Bank> availableBanks;
    Array<Bank> selection;
    std::unique_ptr<TextButton> allButton;
    OwnedArray<UtilityButton> bankButtons;
    std::function<void (const Array<Bank>&)> onSelectionChanged;
};

class ShankSelectorComponent : public Component,
                               private Button::Listener
{
public:
    ShankSelectorComponent (int totalShanks, const Array<int>& initiallySelected, std::function<void (const Array<int>&)> onChange)
        : shankCount (totalShanks),
          selection (initiallySelected),
          onSelectionChanged (std::move (onChange))
    {
        allButton = std::make_unique<TextButton> ("ALL");
        allButton->addListener (this);
        addAndMakeVisible (*allButton);

        for (int i = 0; i < shankCount; ++i)
        {
            auto* btn = new UtilityButton (String (i + 1));
            btn->setClickingTogglesState (true);
            btn->setRadius (2.0f);
            btn->addListener (this);
            btn->setComponentID (String (i));
            shankButtons.add (btn);
            addAndMakeVisible (btn);
        }

        refreshButtonStates();
        updatePreferredSize();
    }

    ~ShankSelectorComponent() override = default;

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (5);
        auto footer = bounds.removeFromBottom (24);

        const int numButtons = shankButtons.size();
        const int gap = 1;
        const int buttonWidth = 20;
        const int buttonHeight = 20;

        // Lay out buttons in a single horizontal row.
        for (int i = 0; i < numButtons; ++i)
        {
            if (auto* btn = shankButtons[i])
            {
                const int x = bounds.getX() + i * (buttonWidth + gap);
                const int y = bounds.getY();
                btn->setBounds (x, y, buttonWidth, buttonHeight);
            }
        }

        allButton->setBounds (footer.withSizeKeepingCentre (60, 20));
    }

private:
    void updatePreferredSize()
    {
        const int numButtons = shankButtons.size();
        const int gap = 1;
        const int buttonWidth = 20;
        const int buttonHeight = 20;

        const int width = jmax (10, numButtons * buttonWidth + jmax (0, numButtons - 1) * gap + 10);
        const int height = buttonHeight + 10 + 24;
        setSize (width, height);
    }

    void refreshButtonStates()
    {
        for (int i = 0; i < shankButtons.size(); ++i)
        {
            const int shank = i;
            const bool isSelected = selection.contains (shank) || (selection.isEmpty());
            shankButtons[i]->setToggleState (isSelected, dontSendNotification);
        }
    }

    void notifySelectionChanged()
    {
        if (onSelectionChanged != nullptr)
            onSelectionChanged (selection);
    }

    void buttonClicked (Button* button) override
    {
        if (button == allButton.get())
        {
            selection.clear();
            refreshButtonStates();
            notifySelectionChanged();
            return;
        }

        if (! shankButtons.contains ((UtilityButton*) button))
            return;

        const int idx = shankButtons.indexOf ((UtilityButton*) button);
        if (! isPositiveAndBelow (idx, shankCount))
            return;

        if (selection.contains (idx))
        {
            selection.removeFirstMatchingValue (idx);
        }
        else if (selection.isEmpty())
        {
            for (int i = 0; i < shankCount; ++i)
                if (i != idx)
                    selection.add (i);
        }
        else
        {
            selection.add (idx);
        }

        selection.sort();

        // if selection contains all available shanks, treat as "all" (empty selection)
        if (selection.size() == shankCount)
            selection.clear();

        refreshButtonStates();
        notifySelectionChanged();
    }

    int shankCount;
    Array<int> selection;
    std::unique_ptr<TextButton> allButton;
    OwnedArray<UtilityButton> shankButtons;
    std::function<void (const Array<int>&)> onSelectionChanged;
};

namespace
{
static constexpr int surveyProbePanelSpacing = 0;
static constexpr int leftPanelExpandedWidth = 510;
static constexpr int leftPanelToggleWidth = 25;
} // namespace

SurveyProbePanel::SurveyProbePanel (Probe* p) : probe (p)
{
    title = std::make_unique<Label>();
    title->setJustificationType (Justification::centred);
    title->setInterceptsMouseClicks (false, false);
    title->setFont (FontOptions ("Inter", "Bold", 24.0f));
    addAndMakeVisible (*title);

    subTitle = std::make_unique<Label>();
    subTitle->setJustificationType (Justification::centred);
    subTitle->setInterceptsMouseClicks (false, false);
    addAndMakeVisible (*subTitle);

    if (probe != nullptr && probe->ui != nullptr)
    {
        auto browser = std::make_unique<ProbeBrowser> (probe->ui);
        browser->setDisplayMode (ProbeBrowser::DisplayMode::OverviewOnly);
        browser->setInterceptsMouseClicks (false, false);
        browser->setOpaque (false);
        probeBrowser = std::move (browser);
        addAndMakeVisible (*probeBrowser);
    }
    else
    {
        placeholder = std::make_unique<Label>();
        placeholder->setJustificationType (Justification::centred);
        placeholder->setFont (FontOptions ("Inter", "Regular", 14.0f));
        placeholder->setColour (Label::textColourId, findColour (ThemeColours::defaultText).withAlpha (0.6f));
        placeholder->setText ("Probe view unavailable", dontSendNotification);
        placeholder->setInterceptsMouseClicks (false, false);
        addAndMakeVisible (*placeholder);
    }

    refresh();
}

SurveyProbePanel::~SurveyProbePanel() = default;

void SurveyProbePanel::refresh()
{
    if (title != nullptr && probe != nullptr)
        title->setText (probe->getName(), dontSendNotification);

    if (subTitle != nullptr && probe != nullptr)
        subTitle->setText ("Slot " + String (probe->basestation->slot) + ", Port " + String (probe->headstage->port), dontSendNotification);

    if (probeBrowser != nullptr)
        probeBrowser->repaint();
}

void SurveyProbePanel::paint (Graphics& g)
{
    /* auto panelBounds = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (findColour (ThemeColours::componentParentBackground).withAlpha (0.5f));
    g.fillRoundedRectangle (panelBounds, 8.0f);

    g.setColour (findColour (ThemeColours::outline).withAlpha (0.75f));
    g.drawRoundedRectangle (panelBounds, 8.0f, 1.0f); */
}

void SurveyProbePanel::resized()
{
    auto bounds = getLocalBounds().reduced (14);
    auto header = bounds.removeFromTop (22);
    auto subHeader = bounds.removeFromTop (40);
    if (title != nullptr)
        title->setBounds (header);

    if (subTitle != nullptr)
        subTitle->setBounds (subHeader);

    if (probeBrowser != nullptr)
    {
        bounds.removeFromTop (6);
        probeBrowser->setBounds (bounds);
    }
    else if (placeholder != nullptr)
    {
        placeholder->setBounds (bounds);
    }
}

void SurveyProbePanel::setMaxPeakToPeakAmplitude (float amplitude)
{
    if (probeBrowser != nullptr)
    {
        probeBrowser->setMaxPeakToPeakAmplitude (amplitude);
        probeBrowser->repaint();
    }
}

int SurveyProbePanel::getOptimalWidth() const
{
    const int shankCount = probe->probeMetadata.shank_count;
    // Keep full width for 4+ shanks, reduce for fewer shanks
    if (shankCount >= 4)
        return 460;
    else if (shankCount == 3)
        return 400;
    else if (shankCount == 2)
        return 340;
    else // 1 shank
        return 280;
}

// --------------------- SurveyRunner -------------------------

SurveyRunner::SurveyRunner (NeuropixThread* t, NeuropixEditor* e, const Array<SurveyTarget>& targetsToSurvey, float secondsPerConfig, bool recordDuringSurvey_)
    : ThreadWithProgressWindow ("Running survey", true, true),
      thread (t),
      editor (e),
      targets (targetsToSurvey),
      secondsPer (secondsPerConfig),
      recordDuringSurvey (recordDuringSurvey_)
{
    if (secondsPer < 10.0f)
        threadSleepMs = 50;
    else if (secondsPer < 60.0f)
        threadSleepMs = 100;
    else
        threadSleepMs = 200;
}

void SurveyRunner::run()
{
    if (targets.size() == 0)
        return;

    LOGC ("SurveyRunner: Starting survey with ", targets.size(), " targets");

    // For progress
    int maxSteps = 0;
    for (const auto& tgt : targets)
    {
        auto* p = tgt.probe;
        maxSteps = jmax (maxSteps, tgt.banks.size() * tgt.shanks.size());
    }

    const double invMaxSteps = maxSteps > 0 ? 1.0 / static_cast<double> (maxSteps) : 0.0;

    int completed = 0;

    // Ensure settings queue is idle
    if (editor->uiLoader->isThreadRunning())
    {
        LOGD ("SurveyRunner: uiLoader thread running, waiting for it to exit");
        editor->uiLoader->waitForThreadToExit (20000);
    }

    setStatusMessage ("Surveying probes...");

    Array<int> bankIndices;
    bankIndices.insertMultiple (0, 0, targets.size());
    Array<int> shanksIndices;
    shanksIndices.insertMultiple (0, 0, targets.size());

    for (int i = 0; i < maxSteps; ++i)
    {
        if (! threadShouldExit())
        {
            setProgress (static_cast<double> (i) * invMaxSteps);
            setStatusMessage ("Surveying probes... Step " + String (i + 1) + "/" + String (maxSteps));
            LOGD ("SurveyRunner: Step ", i + 1, "/", maxSteps);

            int targetIdx = 0;
            for (auto& target : targets)
            {
                if (threadShouldExit())
                {
                    LOGC ("Cancel button pressed, stopping survey early");
                    return;
                }

                Probe* probe = target.probe;

                if (shanksIndices[targetIdx] < target.shanks.size()
                    && bankIndices[targetIdx] < target.banks.size())
                {
                    int sh = target.shanks[shanksIndices[targetIdx]];
                    Bank bank = target.banks[bankIndices[targetIdx]];

                    LOGD ("SurveyRunner: Applying settings to probe ", probe->getName().toRawUTF8(), " - Bank=", SurveyInterface::bankToString (bank).toRawUTF8(), " Shank=", sh + 1);

                    // Build settings for this combo
                    for (const auto& config : target.electrodeConfigs)
                    {
                        if (config.containsIgnoreCase ("Bank " + SurveyInterface::bankToString (bank)))
                        {
                            if (target.shankCount > 1 && config.containsIgnoreCase ("Shank " + String (sh + 1)))
                            {
                                auto selected = probe->selectElectrodeConfiguration (config);
                                const MessageManagerLock mmLock;
                                probe->ui->selectElectrodes (selected);
                                LOGD ("SurveyRunner: Selected configuration ", config.toRawUTF8(), " for probe ", probe->getName().toRawUTF8());
                                break;
                            }
                            else if (target.shankCount == 1)
                            {
                                auto selected = probe->selectElectrodeConfiguration (config);
                                const MessageManagerLock mmLock;
                                probe->ui->selectElectrodes (selected);
                                LOGD ("SurveyRunner: Selected configuration ", config.toRawUTF8(), " for probe ", probe->getName().toRawUTF8());
                                break;
                            }
                        }
                    }

                    bankIndices.set (targetIdx, bankIndices[targetIdx] + 1);
                    if (bankIndices[targetIdx] >= target.banks.size())
                    {
                        bankIndices.set (targetIdx, 0);
                        shanksIndices.set (targetIdx, shanksIndices[targetIdx] + 1);
                    }
                }
                else
                {
                    target.surveyComplete = true;
                    probe->setEnabledForSurvey (false);
                    LOGD ("SurveyRunner: Survey complete for probe ", probe->getName().toRawUTF8());
                }

                targetIdx++;
            }

            // Wait for settings to apply before measuring
            if (editor->uiLoader->isThreadRunning())
                LOGD ("SurveyRunner: Waiting for uiLoader to finish applying settings");

            while (editor->uiLoader->isThreadRunning() && ! threadShouldExit())
                Time::waitForMillisecondCounter (Time::getMillisecondCounter() + 10);

            if (threadShouldExit())
            {
                LOGC ("Cancel button pressed, stopping survey early");
                return;
            }

            // Start acquisition/recording for this window
            if (recordDuringSurvey)
                CoreServices::setRecordingStatus (true);
            else
                CoreServices::setAcquisitionStatus (true);

            LOGD ("SurveyRunner: Acquisition started for step ", i + 1);

            const double mainWaitMs = jmax (0.0, static_cast<double> (secondsPer) * 1000.0);
            const double waitStartMs = Time::getMillisecondCounterHiRes();

            if (mainWaitMs > 0.0)
            {
                while (! threadShouldExit())
                {
                    const double elapsedMs = Time::getMillisecondCounterHiRes() - waitStartMs;
                    const double clampedFraction = jlimit (0.0, 1.0, elapsedMs / mainWaitMs);

                    if (maxSteps > 0)
                        setProgress ((static_cast<double> (i) + clampedFraction) * invMaxSteps);

                    const double remainingSecondsRaw = jmax (0.0, (mainWaitMs - elapsedMs) / 1000.0);
                    const int remainingSecondsInt = static_cast<int> (std::round (remainingSecondsRaw));

                    String remainingText;
                    if (remainingSecondsInt >= 60)
                    {
                        const int minutes = remainingSecondsInt / 60;
                        const int seconds = remainingSecondsInt % 60;
                        remainingText = String (minutes) + " m " + String (seconds) + " s";
                    }
                    else
                    {
                        remainingText = String (remainingSecondsInt) + " s";
                    }

                    setStatusMessage ("Surveying probes... Step " + String (i + 1) + "/" + String (maxSteps)
                                      + " (" + remainingText + " remaining)");

                    if (elapsedMs >= mainWaitMs)
                        break;

                    Thread::sleep (threadSleepMs);
                }

                if (threadShouldExit())
                {
                    LOGC ("Cancel button pressed, stopping survey early");
                    return;
                }
            }

            const int postAcquisitionLeadMs = 100;
            if (postAcquisitionLeadMs > 0)
                Time::waitForMillisecondCounter (Time::getMillisecondCounter() + postAcquisitionLeadMs);

            if (maxSteps > 0)
                setProgress ((static_cast<double> (i) + 1.0) * invMaxSteps);

            if (threadShouldExit())
            {
                LOGC ("Cancel button pressed, stopping survey early");
                return;
            }

            // Stop acquisition for this window before proceeding to next config
            CoreServices::setAcquisitionStatus (false);
            LOGD ("SurveyRunner: Acquisition stopped for step ", i + 1);

            Time::waitForMillisecondCounter (Time::getMillisecondCounter() + 100);
        }
        else
        {
            LOGC ("Cancel button pressed, stopping survey early");
            return;
        }
    }

    setProgress (1.0f);

    setStatusMessage ("Restoring pre-survey probe settings...");
    LOGC ("Restoring pre-survey probe settings...");

    int targetIdx = 0;
    for (auto& target : targets)
    {
        target.probe->ui->selectElectrodes (target.electrodesToRestore);
    }

    LOGC ("SurveyRunner: Survey run finished");
}

// --------------------- PanelToggleButton -------------------------
PanelToggleButton::PanelToggleButton() : Button ("Panel Toggle")
{
    setClickingTogglesState (true);
    setTooltip ("Show/hide settings panel");
    // String hamburgerPath = "M20 7L4 7 M20 12L4 12 M20 17L4 17";
    String collapse = "M4 4m0 2a2 2 0 0 1 2 -2h12a2 2 0 0 1 2 2v12a2 2 0 0 1 -2 2h-12a2 2 0 0 1 -2 -2z M9 4v16 M15 10l-2 2l2 2";
    collapsePath = Drawable::parseSVGPath (collapse);

    String expand = "M4 4m0 2a2 2 0 0 1 2 -2h12a2 2 0 0 1 2 2v12a2 2 0 0 1 -2 2h-12a2 2 0 0 1 -2 -2z M9 4v16 M14 10l2 2l-2 2";
    expandPath = Drawable::parseSVGPath (expand);
}

void PanelToggleButton::paintButton (Graphics& g, bool isMouseOver, bool isButtonDown)
{
    auto bounds = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (findColour (ThemeColours::widgetBackground));
    g.fillRoundedRectangle (bounds, 4.0f);

    auto iconArea = bounds;
    Path toggleIcon = getToggleState() ? collapsePath : expandPath;
    toggleIcon.scaleToFit (iconArea.getX(), iconArea.getY(), iconArea.getWidth(), iconArea.getHeight(), true);
    g.setColour (findColour (ThemeColours::defaultText).withAlpha (isMouseOver ? 1.0f : 0.6f));
    g.strokePath (toggleIcon, PathStrokeType (1.5f));
}

// --------------------- SurveyInterface -------------------------

SurveyInterface::SurveyInterface (NeuropixThread* t, NeuropixEditor* e, NeuropixCanvas* c)
    : SettingsInterface (nullptr, t, e, c), thread (t), editor (e), canvas (c)
{
    type = SettingsInterface::SURVEY_SETTINGS_INTERFACE;

    panelToggleButton = std::make_unique<PanelToggleButton>();
    panelToggleButton->setToggleState (true, dontSendNotification);
    panelToggleButton->addListener (this);
    addAndMakeVisible (*panelToggleButton);

    secondsPerBankComboBox = std::make_unique<ComboBox> ("Seconds Per Bank");
    const std::array<const char*, 7> timeLabels { "2 s", "5 s", "10 s", "30 s", "1 min", "5 mins", "10 mins" };
    for (int i = 0; i < timeOptions.size(); ++i)
        secondsPerBankComboBox->addItem (timeLabels[i], i + 1);
    secondsPerBankComboBox->setSelectedId (1, dontSendNotification); // Default to 2s
    secondsPerBankComboBox->addListener (this);
    addAndMakeVisible (*secondsPerBankComboBox);

    amplitudeRangeComboBox = std::make_unique<ComboBox> ("Amplitude Range");
    const std::array<const char*, 4> amplitudeLabels { "0 - 250 \xC2\xB5V", "0 - 500 \xC2\xB5V", "0 - 750 \xC2\xB5V", "0 - 1000 \xC2\xB5V" };
    for (int i = 0; i < amplitudeOptions.size(); ++i)
        amplitudeRangeComboBox->addItem (String::fromUTF8 (amplitudeLabels[i]), i + 1);
    amplitudeRangeComboBox->setSelectedId (2, dontSendNotification);
    amplitudeRangeComboBox->addListener (this);
    addAndMakeVisible (*amplitudeRangeComboBox);

    runButton = std::make_unique<TextButton> (String::fromUTF8 ("RUN SURVEY \t\t \xE2\x96\xB6")); // Play symbol
    runButton->setColour (TextButton::buttonColourId, Colour (80, 180, 70));
    runButton->setColour (TextButton::textColourOffId, Colours::black);
    runButton->setColour (TextButton::textColourOnId, Colours::black);
    runButton->addListener (this);
    addAndMakeVisible (*runButton);

    recordingToggleButton = std::make_unique<ToggleButton> ("Record survey to disk");
    recordingToggleButton->setToggleState (false, dontSendNotification);
    recordingToggleButton->addListener (this);
    recordingToggleButton->setTooltip ("If enabled, each record node will record to disk during the survey. "
                                       "Otherwise, data will be acquired but not saved. You can still save the survey results to a JSON file afterwards.");
    addAndMakeVisible (*recordingToggleButton);

    activityViewFilterToggle = std::make_unique<UtilityButton> ("BP FILTER");
    activityViewFilterToggle->setToggleState (true, dontSendNotification);
    activityViewFilterToggle->setClickingTogglesState (true);
    activityViewFilterToggle->addListener (this);
    activityViewFilterToggle->setTooltip ("Apply bandpass filtering (300 Hz - 6 kHz) to activity view during survey");
    addAndMakeVisible (*activityViewFilterToggle);

    activityViewCARToggle = std::make_unique<UtilityButton> ("CAR");
    activityViewCARToggle->setToggleState (true, dontSendNotification);
    activityViewCARToggle->setClickingTogglesState (true);
    activityViewCARToggle->addListener (this);
    activityViewCARToggle->setTooltip ("Apply common average referencing to activity view during survey");
    addAndMakeVisible (*activityViewCARToggle);

    table = std::make_unique<TableListBox> ("Survey Table", this);
    table->getHeader().addColumn ("Use", Columns::ColSelect, 30);
    table->getHeader().addColumn ("Probe", Columns::ColName, 100);
    table->getHeader().addColumn ("Type", Columns::ColType, 120);
    table->getHeader().addColumn ("Banks", Columns::ColBanks, 120);
    table->getHeader().addColumn ("Shanks", Columns::ColShanks, 100);
    table->setAutoSizeMenuOptionShown (false);
    table->getHeader().setInterceptsMouseClicks (false, false);
    table->setOutlineThickness (1);
    addAndMakeVisible (*table);

    saveButton = std::make_unique<UtilityButton> ("SAVE RESULTS");
    saveButton->setClickingTogglesState (false);
    saveButton->addListener (this);
    saveButton->setEnabled (false);
    saveButton->setTooltip ("Save survey results (peak-to-peak amplitude) to a JSON file");
    addAndMakeVisible (*saveButton);

    probeViewportContent = std::make_unique<Component>();
    probeViewport = std::make_unique<Viewport> ("SurveyProbeViewport");
    probeViewport->setScrollBarsShown (false, true);
    probeViewport->setScrollBarThickness (12);
    probeViewport->setViewedComponent (probeViewportContent.get(), false);
    probeViewport->setInterceptsMouseClicks (true, false);
    addAndMakeVisible (*probeViewport);

    refreshProbeList();
}

void SurveyInterface::paint (Graphics& g)
{
    g.fillAll (findColour (ThemeColours::componentParentBackground));

    const float leftPanelX = 10.0f;
    const float panelHeight = (float) getHeight() - 20.0f;
    const bool showSettings = ! leftPanelCollapsed;

    g.setColour (findColour (ThemeColours::componentBackground));
    if (showSettings)
    {
        g.fillRoundedRectangle (leftPanelX, 10.0f, (float) leftPanelExpandedWidth, panelHeight, 8.0f);
        g.setColour (findColour (ThemeColours::outline).withAlpha (0.75f));
        g.drawRoundedRectangle (leftPanelX, 10.0f, (float) leftPanelExpandedWidth, panelHeight, 8.0f, 1.0f);

        g.setFont (FontOptions ("Inter", "Semi Bold", 25.0f));
        g.setColour (findColour (ThemeColours::defaultText));
        g.drawText ("SURVEY SETTINGS", leftPanelX + 20.0f, 35.0f, leftPanelExpandedWidth, 25.0f, Justification::centredLeft);
    }
    else
    {
        g.fillRoundedRectangle (leftPanelX, 10.0f, (float) leftPanelToggleWidth, panelHeight, 8.0f);
        g.setColour (findColour (ThemeColours::outline).withAlpha (0.75f));
        g.drawRoundedRectangle (leftPanelX, 10.0f, (float) leftPanelToggleWidth, panelHeight, 8.0f, 1.0f);

        g.addTransform (AffineTransform::rotation (-MathConstants<double>::halfPi));
        g.setFont (FontOptions ("Inter", "Semi Bold", 18.0f));
        g.setColour (findColour (ThemeColours::defaultText).withAlpha (0.5f));
        g.drawText ("SURVEY SETTINGS", -(int) (panelHeight + 20), 10, (int) panelHeight, leftPanelToggleWidth, Justification::centred);
        g.addTransform (AffineTransform::rotation (MathConstants<double>::halfPi));
    }

    Rectangle<int> rightArea = getLocalBounds();
    const int leftReservedWidth = (int) leftPanelX + (showSettings ? leftPanelExpandedWidth : leftPanelToggleWidth) + 20;
    rightArea.removeFromLeft (leftReservedWidth);
    if (! rightArea.isEmpty())
    {
        auto rightPanel = rightArea.reduced (10);
        if (! rightPanel.isEmpty())
        {
            g.setColour (findColour (ThemeColours::componentBackground));
            g.fillRoundedRectangle (rightPanel.toFloat(), 8.0f);
        }
    }

    if (showSettings)
    {
        g.setColour (findColour (ThemeColours::defaultText));
        g.setFont (FontOptions ("Inter", "Medium", 18.0f));
        const int secondsLabelY = secondsPerBankComboBox != nullptr ? secondsPerBankComboBox->getY() : 120;
        g.drawText ("Time per bank/shank:", 30, secondsLabelY, 170, 25, Justification::centredLeft);

        const int filterY = secondsLabelY + 45;
        g.drawText ("Activity view options:", 30, filterY, 170, 25, Justification::centredLeft);

        const int amplitudeY = amplitudeRangeComboBox != nullptr ? amplitudeRangeComboBox->getY() : secondsLabelY + 60;
        g.drawText ("Amplitude scale:", 30, amplitudeY, 200, 25, Justification::centredLeft);

        const int legendX = 50;
        const int legendY = amplitudeY + 40;
        const int legendEntryHeight = 20;
        const int legendRectSize = 15;
        const int legendSteps = 5;

        g.setFont (FontOptions ("Inter", "Regular", 15.0f));
        for (int i = 0; i <= legendSteps; ++i)
        {
            const float normalized = legendSteps == 0 ? 0.0f : static_cast<float> (i) / static_cast<float> (legendSteps);
            g.setColour (ColourScheme::getColourForNormalizedValue (normalized));
            g.fillRect (legendX, legendY + legendEntryHeight * i, legendRectSize, legendRectSize);

            const int amplitudeValue = juce::roundToInt ((legendSteps == 0 ? 0.0f : currentMaxPeakToPeak / static_cast<float> (legendSteps)) * static_cast<float> (i));
            g.setColour (findColour (ThemeColours::defaultText));
            g.drawText (String (amplitudeValue) + String::fromUTF8 (" \xC2\xB5V"), legendX + legendRectSize + 8, legendY + legendEntryHeight * i - 2, 150, legendRectSize + 4, Justification::centredLeft);
        }
    }
}

void SurveyInterface::resized()
{
    const int leftPanelX = 10;
    const int topMargin = 50;
    const bool showSettings = ! leftPanelCollapsed;

    const int toggleWidth = 24;
    const int toggleX = leftPanelX + (showSettings ? leftPanelExpandedWidth - 12 : toggleWidth - 12);
    const int toggleY = 25;
    panelToggleButton->setBounds (toggleX, toggleY, toggleWidth, toggleWidth);

    runButton->setVisible (showSettings);
    if (showSettings)
        runButton->setBounds (leftPanelExpandedWidth - 200, topMargin - 20, 180, 30);

    secondsPerBankComboBox->setVisible (showSettings);
    if (showSettings)
    {
        const int comboBoxY = runButton->getBottom() + 20;
        secondsPerBankComboBox->setBounds (leftPanelX + 190, comboBoxY, 90, 25);
    }

    activityViewFilterToggle->setVisible (showSettings);
    if (showSettings)
    {
        const int toggleWidth = 100;
        const int toggleX = leftPanelX + 190;
        const int toggleY = secondsPerBankComboBox->getBottom() + 20;
        activityViewFilterToggle->setBounds (toggleX, toggleY, toggleWidth, 22);
    }

    activityViewCARToggle->setVisible (showSettings);
    if (showSettings)
    {
        const int toggleWidth = 100;
        const int toggleX = activityViewFilterToggle->getRight() + 10;
        const int toggleY = activityViewFilterToggle->getY();
        activityViewCARToggle->setBounds (toggleX, toggleY, toggleWidth, 22);
    }

    recordingToggleButton->setVisible (showSettings);
    if (showSettings)
    {
        const int toggleWidth = 200;
        const int toggleX = secondsPerBankComboBox->getRight() + 10;
        const int toggleY = runButton->getBottom() + 20;
        recordingToggleButton->setBounds (toggleX, toggleY, toggleWidth, 24);
    }

    table->setVisible (showSettings);
    if (showSettings)
    {
        const int tableTop = activityViewCARToggle->getBottom() + 20;
        const int desiredHeight = (getNumRows() + 1) * table->getRowHeight() + 8;
        const int availableHeight = getHeight() - tableTop - 40;
        table->setBounds (leftPanelX + 20, tableTop, leftPanelExpandedWidth - 38, jmin (desiredHeight, availableHeight));
    }

    saveButton->setVisible (showSettings);
    if (showSettings && table != nullptr)
        saveButton->setBounds (leftPanelX + (leftPanelExpandedWidth - 110) / 2, table->getBottom() + 30, 110, 24);

    amplitudeRangeComboBox->setVisible (showSettings);
    if (showSettings)
        amplitudeRangeComboBox->setBounds (leftPanelX + 150, saveButton->getBottom() + 30, 110, 22);

    if (probeViewport != nullptr)
    {
        Rectangle<int> rightArea = getLocalBounds();
        const int leftReservedWidth = leftPanelX + (showSettings ? leftPanelExpandedWidth : leftPanelToggleWidth) + 20;
        rightArea.removeFromLeft (leftReservedWidth);
        rightArea = rightArea.reduced (10, 10);
        if (rightArea.getWidth() < 0)
            rightArea.setWidth (0);
        if (rightArea.getHeight() < 0)
            rightArea.setHeight (0);

        probeViewport->setBounds (rightArea);
        layoutProbePanels();
    }
}

void SurveyInterface::buttonClicked (Button* b)
{
    if (b == panelToggleButton.get())
    {
        leftPanelCollapsed = ! panelToggleButton->getToggleState();
        resized();
        repaint();
    }
    else if (b == runButton.get() && ! CoreServices::getAcquisitionStatus())
        launchSurvey();
    else if (b == saveButton.get() && lastSurveyTargets.size() > 0)
        saveSurveyResultsToJson (lastSurveyTargets, lastSurveySecondsPerConfig);
    // Note: activityViewFilterToggle and activityViewCARToggle are only applied when survey is launched
}

void SurveyInterface::comboBoxChanged (ComboBox* cb)
{
    if (cb == amplitudeRangeComboBox.get())
    {
        const int optionIndex = amplitudeRangeComboBox->getSelectedId() - 1;
        if (isPositiveAndBelow (optionIndex, static_cast<int> (amplitudeOptions.size())))
        {
            const float newAmplitude = amplitudeOptions[static_cast<size_t> (optionIndex)];
            currentMaxPeakToPeak = newAmplitude;
            applyMaxAmplitudeToPanels();
            repaint();
        }
    }
}

void SurveyInterface::saveParameters (XmlElement* xml)
{
    if (xml == nullptr)
        return;

    auto* surveyNode = xml->createNewChildElement ("SURVEY_SETTINGS");

    int secondsIndex = secondsPerBankComboBox != nullptr ? secondsPerBankComboBox->getSelectedId() - 1 : 0;

    const float secondsPerConfig = timeOptions[secondsIndex];
    surveyNode->setAttribute ("seconds_per_config", secondsPerConfig);

    const bool recordSurvey = recordingToggleButton->getToggleState();
    const bool filterEnabled = activityViewFilterToggle->getToggleState();
    const bool carEnabled = activityViewCARToggle->getToggleState();

    surveyNode->setAttribute ("record_during_survey", recordSurvey);
    surveyNode->setAttribute ("bandpass_filter_enabled", filterEnabled);
    surveyNode->setAttribute ("car_enabled", carEnabled);

    int amplitudeIndex = amplitudeRangeComboBox->getSelectedId() - 1;

    const float amplitudeValue = amplitudeOptions[amplitudeIndex];
    surveyNode->setAttribute ("max_amplitude", amplitudeValue);

    for (const auto& row : rows)
    {
        if (row.probe == nullptr)
            continue;

        auto* probeNode = surveyNode->createNewChildElement ("SURVEY_PROBE");
        probeNode->setAttribute ("name", row.probe->getName());
        probeNode->setAttribute ("type", String (probeTypeToString (row.probe->type)));
        probeNode->setAttribute ("type_id", static_cast<int> (row.probe->type));
        probeNode->setAttribute ("selected", row.selected);

        const bool useAllBanks = row.chosenBanks.isEmpty();
        if (useAllBanks)
        {
            probeNode->setAttribute ("banks", "All");
        }
        else
        {
            StringArray bankTokens;
            for (auto bank : row.chosenBanks)
                bankTokens.add (bankToString (bank));
            probeNode->setAttribute ("banks", bankTokens.joinIntoString (","));
        }

        const bool useAllShanks = row.chosenShanks.isEmpty();

        if (useAllShanks)
        {
            probeNode->setAttribute ("shanks", "All");
        }
        else
        {
            StringArray shankTokens;
            for (auto shank : row.chosenShanks)
                shankTokens.add (String (shank));
            probeNode->setAttribute ("shanks", shankTokens.joinIntoString (","));
        }
    }
}

void SurveyInterface::loadParameters (XmlElement* xml)
{
    if (xml == nullptr)
        return;

    if (rows.isEmpty())
        refreshProbeList();

    XmlElement* surveyNode = nullptr;
    forEachXmlChildElement (*xml, child)
    {
        if (child->hasTagName ("SURVEY_SETTINGS"))
        {
            surveyNode = child;
            break;
        }
    }

    if (surveyNode == nullptr)
        return;

    const float defaultSeconds = timeOptions.isEmpty() ? 2.0f : timeOptions[0];
    const float secondsValue = static_cast<float> (surveyNode->getDoubleAttribute ("seconds_per_config", defaultSeconds));

    int secondsIndex = 0;
    if (timeOptions.contains (secondsValue))
        secondsIndex = timeOptions.indexOf (secondsValue);

    if (secondsPerBankComboBox != nullptr)
        secondsPerBankComboBox->setSelectedId (secondsIndex + 1, dontSendNotification);

    const bool recordSurvey = surveyNode->getBoolAttribute ("record_during_survey", false);
    const bool filterEnabled = surveyNode->getBoolAttribute ("bandpass_filter_enabled", true);
    const bool carEnabled = surveyNode->getBoolAttribute ("car_enabled", true);

    recordingToggleButton->setToggleState (recordSurvey, dontSendNotification);
    activityViewFilterToggle->setToggleState (filterEnabled, dontSendNotification);
    activityViewCARToggle->setToggleState (carEnabled, dontSendNotification);

    const float amplitudeValue = static_cast<float> (surveyNode->getDoubleAttribute ("max_amplitude", currentMaxPeakToPeak));

    int amplitudeIndex = 1; // Default to 500 uV
    if (amplitudeOptions.contains (amplitudeValue))
        amplitudeIndex = amplitudeOptions.indexOf (amplitudeValue);

    if (amplitudeRangeComboBox != nullptr)
        amplitudeRangeComboBox->setSelectedId (amplitudeIndex + 1, dontSendNotification);

    currentMaxPeakToPeak = amplitudeOptions[amplitudeIndex];
    applyMaxAmplitudeToPanels();

    forEachXmlChildElement (*surveyNode, probeNode)
    {
        if (! probeNode->hasTagName ("SURVEY_PROBE"))
            continue;

        const auto probeName = probeNode->getStringAttribute ("name");
        const int typeId = probeNode->getIntAttribute ("type_id", -1);
        const String typeString = probeNode->getStringAttribute ("type");

        int matchedRowIndex = -1;
        for (int rowIdx = 0; rowIdx < rows.size(); ++rowIdx)
        {
            auto& row = rows.getReference (rowIdx);
            if (row.probe == nullptr)
                continue;

            if (row.probe->getName() != probeName)
                continue;

            const int rowTypeId = static_cast<int> (row.probe->type);
            if (typeId != -1 && rowTypeId != typeId)
                continue;

            if (typeId == -1 && typeString.isNotEmpty())
            {
                if (String (probeTypeToString (row.probe->type)) != typeString)
                    continue;
            }

            matchedRowIndex = rowIdx;
            break;
        }

        if (matchedRowIndex < 0)
            continue;

        auto& row = rows.getReference (matchedRowIndex);

        row.selected = probeNode->getBoolAttribute ("selected", row.selected);

        row.chosenBanks.clear();
        String banksString = probeNode->getStringAttribute ("banks", "All");
        if (banksString.equalsIgnoreCase ("All"))
        {
            // leave chosenBanks empty to indicate all banks
        }
        else
        {
            StringArray bankTokens;
            if (banksString.isNotEmpty())
            {
                bankTokens.addTokens (banksString, ",", "");
                bankTokens.trim();
                bankTokens.removeEmptyStrings();
            }

            for (const auto& token : bankTokens)
            {
                Bank bankValue = stringToBank (token);
                if (bankValue != Bank::NONE && row.availableBanks.contains (bankValue))
                    row.chosenBanks.addIfNotAlreadyThere (bankValue);
            }
            row.chosenBanks.sort();
        }

        row.chosenShanks.clear();
        String shanksString = probeNode->getStringAttribute ("shanks", "All");
        if (shanksString.equalsIgnoreCase ("All") || row.shankCount == 1)
        {
            // leave chosenShanks empty to indicate all shanks
        }
        else
        {
            StringArray shankTokens;
            if (shanksString.isNotEmpty())
            {
                shankTokens.addTokens (shanksString, ",", "");
                shankTokens.trim();
                shankTokens.removeEmptyStrings();
            }

            for (const auto& token : shankTokens)
            {
                const auto trimmed = token.trim();
                if (trimmed.isEmpty())
                    continue;

                int shankIndex = trimmed.getIntValue();

                if (isPositiveAndBelow (shankIndex, row.shankCount))
                    row.chosenShanks.addIfNotAlreadyThere (shankIndex);
            }
            row.chosenShanks.sort();
        }

        if (table != nullptr)
            table->repaintRow (matchedRowIndex);
    }

    if (table != nullptr)
    {
        table->updateContent();
        table->repaint();
    }

    repaint();
}

void SurveyInterface::startAcquisition()
{
    for (auto& panel : probePanels)
    {
        if (panel->getProbe()->getEnabledForSurvey() || ! isSurveyRunning)
            panel->getProbeBrowser()->startTimer (100);
    }

    if (! isSurveyRunning)
    {
        runButton->setEnabled (false);
        activityViewFilterToggle->setEnabled (false);
        activityViewCARToggle->setEnabled (false);
    }
}

void SurveyInterface::stopAcquisition()
{
    for (auto& panel : probePanels)
    {
        panel->getProbeBrowser()->stopTimer();
    }

    if (! isSurveyRunning)
    {
        runButton->setEnabled (true);
        activityViewFilterToggle->setEnabled (true);
        activityViewCARToggle->setEnabled (true);
    }
}

void SurveyInterface::updateInfoString()
{
    if (isSurveyRunning)
        return;

    for (auto& panel : probePanels)
    {
        panel->refresh();
    }

    if (table)
    {
        table->repaint();
    }
}

void SurveyInterface::rebuildProbePanels()
{
    if (probeViewportContent == nullptr)
        return;

    probeViewportContent->removeAllChildren();
    probePanels.clear();
    probePanelsWidth = 0;

    int x = 0;
    if (thread != nullptr)
    {
        for (auto* probe : thread->getProbes())
        {
            if (probe == nullptr)
                continue;

            auto panel = std::make_unique<SurveyProbePanel> (probe);
            const int panelWidth = panel->getOptimalWidth();
            panel->setBounds (x, 0, panelWidth, SurveyProbePanel::minHeight);
            panel->setMaxPeakToPeakAmplitude (currentMaxPeakToPeak);
            panel->refresh();
            probeViewportContent->addAndMakeVisible (panel.get());
            probePanels.add (panel.release());

            x += panelWidth + surveyProbePanelSpacing;
        }
    }

    probePanelsWidth = x > 0 ? x + surveyProbePanelSpacing : 0;

    layoutProbePanels();

    if (probeViewport != nullptr)
        probeViewport->setViewPosition (0, 0);
}

void SurveyInterface::layoutProbePanels()
{
    if (probeViewport == nullptr || probeViewportContent == nullptr)
        return;

    const int viewportWidth = probeViewport->getWidth();
    const int viewportHeight = probeViewport->getHeight();

    const int contentWidth = probePanelsWidth;
    const int contentHeight = jmax (SurveyProbePanel::minHeight, viewportHeight);
    probeViewportContent->setSize (contentWidth, contentHeight);

    int x = surveyProbePanelSpacing;
    for (auto* panel : probePanels)
    {
        if (panel == nullptr)
            continue;

        const int panelWidth = panel->getWidth();
        panel->setBounds (x, 20, panelWidth, contentHeight - 40);
        x += panelWidth + surveyProbePanelSpacing;
    }
}

void SurveyInterface::refreshProbeList()
{
    rows.clear();
    for (auto* p : thread->getProbes())
    {
        RowState r;
        r.probe = p;
        r.electrodeConfigs = p->settings.availableElectrodeConfigurations;
        r.selected = true;

        for (const auto& b : p->settings.availableBanks)
        {
            if (b < Bank::A || b > Bank::M)
                continue;

            r.availableBanks.add (b);
        }

        r.shankCount = p->type == ProbeType::QUAD_BASE ? 1 : jmax (1, p->probeMetadata.shank_count);
        rows.add (r);
    }
    if (table)
        table->updateContent();

    rebuildProbePanels();
    applyMaxAmplitudeToPanels();
}

void SurveyInterface::launchSurvey()
{
    bool shouldRecordSurvey = recordingToggleButton->getToggleState();

    if (shouldRecordSurvey && CoreServices::getAvailableRecordNodeIds().size() == 0)
    {
        bool shouldProceed = AlertWindow::showOkCancelBox (AlertWindow::WarningIcon,
                                                           "No Record Node Found",
                                                           "You have chosen to record the survey to disk, but no Record Node is available. Would you like to proceed with acquisition only?",
                                                           "Yes",
                                                           "No",
                                                           this);

        if (shouldProceed)
            shouldRecordSurvey = false;
        else
            return;
    }

    // Disable all controls during acquisition
    runButton->setEnabled (false);
    secondsPerBankComboBox->setEnabled (false);
    activityViewCARToggle->setEnabled (false);
    activityViewFilterToggle->setEnabled (false);
    table->setEnabled (false);
    saveButton->setEnabled (false);
    recordingToggleButton->setEnabled (false);

    lastSurveyTargets.clear();

    // Get the desired filter and CAR states from the toggle buttons
    bool desiredFilterState = activityViewFilterToggle->getToggleState();
    bool desiredCARState = activityViewCARToggle->getToggleState();

    // Get the selected time duration
    const int timeOptionIndex = secondsPerBankComboBox->getSelectedId() - 1;
    float secondsPerConfig = timeOptions[timeOptionIndex];

    Array<SurveyTarget> targets;
    for (const auto& r : rows)
    {
        // Prepare activity views for survey averaging
        r.probe->setSurveyMode (true);

        if (! r.selected)
        {
            r.probe->setEnabledForSurvey (false);
            continue;
        }

        SurveyTarget t;
        t.probe = r.probe;
        t.electrodeConfigs = r.electrodeConfigs;
        t.electrodesToRestore = r.probe->settings.selectedElectrode;
        t.probe->setEnabledForSurvey (true);

        // Store original filter and CAR states
        t.originalFilterState = r.probe->getActivityViewFilterState();
        t.originalCARState = r.probe->getActivityViewCARState();

        // Apply the desired states from toggle buttons
        r.probe->setActivityViewFilterState (desiredFilterState);
        r.probe->setActivityViewCARState (desiredCARState);

        if (r.chosenBanks.size() > 0)
            t.banks = r.chosenBanks;
        else
            t.banks = r.availableBanks; // empty => all available

        if (r.chosenShanks.size() > 0)
            t.shanks = r.chosenShanks;
        else
        {
            for (int i = 0; i < r.shankCount; ++i)
                t.shanks.add (i); // empty => all shanks
        }
        t.shankCount = r.shankCount;

        targets.add (t);
    }

    if (targets.size() == 0)
    {
        CoreServices::sendStatusMessage ("No probes selected for survey.");
        runButton->setEnabled (true);
        secondsPerBankComboBox->setEnabled (true);
        activityViewCARToggle->setEnabled (true);
        activityViewFilterToggle->setEnabled (true);
        table->setEnabled (true);
        recordingToggleButton->setEnabled (true);
        return;
    }

    isSurveyRunning = true;

    std::unique_ptr<SurveyRunner> runner = std::make_unique<SurveyRunner> (thread, editor, targets, secondsPerConfig, shouldRecordSurvey);

    if (runner->runThread())
    {
        lastSurveyTargets = targets;
        lastSurveySecondsPerConfig = secondsPerConfig;
        saveButton->setEnabled (true);
    }
    else
    {
        // Survey was cancelled
        CoreServices::setAcquisitionStatus (false);

        int targetIdx = 0;
        for (auto& target : targets)
        {
            target.probe->ui->selectElectrodes (target.electrodesToRestore);
        }

        CoreServices::sendStatusMessage ("Survey cancelled.");
    }

    // Restore activity views to normal mode and restore original filter/CAR states
    for (const auto& t : targets)
    {
        t.probe->setSurveyMode (false, false);
        t.probe->setEnabledForSurvey (false);

        // Restore original filter and CAR states
        t.probe->setActivityViewFilterState (t.originalFilterState);
        t.probe->setActivityViewCARState (t.originalCARState);
    }

    // Also restore for non-selected probes
    for (const auto& r : rows)
    {
        r.probe->setSurveyMode (false, false);
        r.probe->setEnabledForSurvey (false);
    }

    isSurveyRunning = false;

    // Re-enable controls
    runButton->setEnabled (true);
    secondsPerBankComboBox->setEnabled (true);
    activityViewCARToggle->setEnabled (true);
    activityViewFilterToggle->setEnabled (true);
    table->setEnabled (true);
    recordingToggleButton->setEnabled (true);
}

void SurveyInterface::saveSurveyResultsToJson (const Array<SurveyTarget>& targets, float secondsPerConfig)
{
    if (targets.isEmpty())
        return;

    const auto timestamp = Time::getCurrentTime();

    DynamicObject root;
    root.setProperty (Identifier ("generated_at"), timestamp.toISO8601 (true));
    root.setProperty (Identifier ("seconds_per_configuration"), static_cast<double> (secondsPerConfig));
    root.setProperty (Identifier ("probe_count"), targets.size());

    Array<var> probesVar;

    for (const auto& target : targets)
    {
        Probe* probe = target.probe;
        if (probe == nullptr)
            continue;

        ActivityView::SurveyStatistics apStats = probe->getSurveyStatistics (ActivityToView::APVIEW);

        DynamicObject::Ptr probeObj = new DynamicObject();
        probeObj->setProperty (Identifier ("name"), probe->getName());
        probeObj->setProperty (Identifier ("type"), String (probeTypeToString (probe->type)));
        probeObj->setProperty (Identifier ("shank_count"), probe->probeMetadata.shank_count);
        probeObj->setProperty (Identifier ("sample_rate"), static_cast<double> (probe->ap_sample_rate));

        if (probe->info.serial_number != 0)
            probeObj->setProperty (Identifier ("serial_number"), String (probe->info.serial_number));

        Array<var> bankStrings;
        for (auto bank : target.banks)
            bankStrings.add (bankToString (bank));
        probeObj->setProperty (Identifier ("banks_surveyed"), bankStrings);

        Array<var> shankIndices;
        for (auto shank : target.shanks)
            shankIndices.add (shank);
        probeObj->setProperty (Identifier ("shanks_surveyed"), shankIndices);

        Array<var> electrodesVar;
        const int electrodeCount = probe->electrodeMetadata.size();

        for (int idx = 0; idx < electrodeCount; ++idx)
        {
            const auto& meta = probe->electrodeMetadata.getReference (idx);
            const size_t index = static_cast<size_t> (idx);
            bool wasSurveyed = (target.banks.isEmpty() || target.banks.contains (meta.bank)) && (target.shanks.isEmpty() || target.shanks.contains (meta.shank));

            DynamicObject::Ptr electrodeObj = new DynamicObject();
            electrodeObj->setProperty (Identifier ("global_index"), meta.global_index);
            electrodeObj->setProperty (Identifier ("shank"), meta.shank);
            electrodeObj->setProperty (Identifier ("column"), meta.column_index);
            electrodeObj->setProperty (Identifier ("row"), meta.row_index);
            electrodeObj->setProperty (Identifier ("bank"), bankToString (meta.bank));
            electrodeObj->setProperty (Identifier ("is_reference"), meta.type == ElectrodeType::REFERENCE);
            electrodeObj->setProperty (Identifier ("position_x_um"), meta.xpos);
            electrodeObj->setProperty (Identifier ("position_y_um"), meta.ypos);
            electrodeObj->setProperty (Identifier ("was_surveyed"), wasSurveyed);

            const float apPeak = index < apStats.averages.size() ? apStats.averages[index] : 0.0f;
            electrodeObj->setProperty (Identifier ("peak_to_peak"), apPeak);

            electrodesVar.add (electrodeObj.get());
        }

        probeObj->setProperty (Identifier ("electrodes"), electrodesVar);

        probesVar.add (probeObj.get());
    }

    if (probesVar.isEmpty())
    {
        CoreServices::sendStatusMessage ("No survey data collected to export.");
        return;
    }

    root.setProperty (Identifier ("probes"), probesVar);

    String defaultName = "neuropixels_survey_" + timestamp.formatted ("%Y-%m-%d_%H-%M-%S") + ".json";
    File defaultLocation = CoreServices::getDefaultUserSaveDirectory().getChildFile (defaultName);

    FileChooser fileChooser ("Save survey results as JSON", defaultLocation, "*.json");

    if (! fileChooser.browseForFileToSave (true))
    {
        CoreServices::sendStatusMessage ("Survey results export cancelled.");
        return;
    }

    File outputFile = fileChooser.getResult();
    if (! outputFile.hasFileExtension (".json"))
        outputFile = outputFile.withFileExtension (".json");

    FileOutputStream outputStream (outputFile);
    if (! outputStream.openedOk())
    {
        CoreServices::sendStatusMessage ("Unable to write survey results to " + outputFile.getFullPathName());
        return;
    }

    root.writeAsJSON (outputStream, JSON::FormatOptions {}.withIndentLevel (4).withSpacing (JSON::Spacing::multiLine).withMaxDecimalPlaces (6));
    outputStream.flush();

    CoreServices::sendStatusMessage ("Survey results saved to " + outputFile.getFullPathName());
}

void SurveyInterface::applyMaxAmplitudeToPanels()
{
    for (auto* panel : probePanels)
    {
        if (panel == nullptr)
            continue;

        panel->setMaxPeakToPeakAmplitude (currentMaxPeakToPeak);
    }

    if (probeViewportContent != nullptr)
        probeViewportContent->repaint();
}

// ---------------------- TableListBoxModel ----------------------

int SurveyInterface::getNumRows()
{
    return rows.size();
}

void SurveyInterface::paintRowBackground (Graphics& g, int rowNumber, int width, int height, bool rowIsSelected)
{
    auto bg = findColour (ThemeColours::widgetBackground);

    if (rowNumber % 2 == 0)
        g.fillAll (bg);
    else
        g.fillAll (bg.darker (0.1f));
}

void SurveyInterface::paintCell (Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected)
{
    if (rowNumber < 0 || rowNumber >= rows.size())
        return;
    const auto& r = rows.getReference (rowNumber);
    g.setColour (findColour (ThemeColours::defaultText));
    g.setFont (FontOptions ("Inter", "Medium", 14.0f));

    if (columnId == Columns::ColName)
        g.drawText (r.probe ? r.probe->getName() : String ("<null>"), 4, 0, width - 8, height, Justification::centredLeft);
    else if (columnId == Columns::ColType)
        g.drawText (probeTypeToString (r.probe->type).substr (12), 4, 0, width - 8, height, Justification::centredLeft);
    else if (columnId == Columns::ColBanks)
    {
        if (table == nullptr || table->getCellComponent (rowNumber, columnId) == nullptr)
            g.drawText (banksSummary (r.chosenBanks), 4, 0, width - 8, height, Justification::centredLeft);
    }
    else if (columnId == Columns::ColShanks)
    {
        if (table == nullptr || table->getCellComponent (rowNumber, columnId) == nullptr)
            g.drawText (shanksSummary (r.chosenShanks, r.shankCount), 4, 0, width - 8, height, Justification::centredLeft);
    }
}

Component* SurveyInterface::refreshComponentForCell (int rowNumber, int columnId, bool, Component* existing)
{
    if (rowNumber < 0 || rowNumber >= rows.size())
        return nullptr;
    auto& r = rows.getReference (rowNumber);

    if (columnId == Columns::ColSelect)
    {
        ToggleButton* tb = dynamic_cast<ToggleButton*> (existing);
        if (tb == nullptr)
        {
            tb = new ToggleButton (" ");
            tb->onClick = [this, rowNumber, tb]()
            {
                rows.getReference (rowNumber).selected = tb->getToggleState();
            };
        }
        tb->setToggleState (r.selected, dontSendNotification);
        return tb;
    }

    if (columnId == Columns::ColBanks)
    {
        TextButton* btn = dynamic_cast<TextButton*> (existing);
        if (btn == nullptr)
        {
            btn = new TextButton();
            btn->setButtonText (banksSummary (r.chosenBanks));
            btn->setTooltip ("Select banks");
        }
        btn->setEnabled (r.availableBanks.size() > 0);
        btn->setButtonText (banksSummary (r.chosenBanks));
        btn->onClick = [this, rowNumber, btn]()
        {
            showBanksSelector (rowNumber, btn);
        };
        return btn;
    }

    if (columnId == Columns::ColShanks)
    {
        TextButton* btn = dynamic_cast<TextButton*> (existing);
        if (btn == nullptr)
        {
            btn = new TextButton();
            btn->setTooltip ("Select shanks");
        }

        btn->setEnabled (r.shankCount > 1);
        btn->setButtonText (shanksSummary (r.chosenShanks, r.shankCount));
        btn->onClick = [this, rowNumber, btn]()
        {
            showShanksSelector (rowNumber, btn);
        };
        return btn;
    }

    return nullptr;
}

void SurveyInterface::cellClicked (int rowNumber, int columnId, const MouseEvent&)
{
    if (rowNumber < 0 || rowNumber >= rows.size())
        return;
    if (columnId == Columns::ColBanks)
        showBanksSelector (rowNumber, table != nullptr ? table->getCellComponent (rowNumber, columnId) : nullptr);
    else if (columnId == Columns::ColShanks)
        showShanksSelector (rowNumber, table != nullptr ? table->getCellComponent (rowNumber, columnId) : nullptr);
}

String SurveyInterface::bankToString (Bank b)
{
    // Display letters for standard banks; fallback to numeric
    int bi = static_cast<int> (b);
    if (bi >= 0 && bi <= 12)
        return String::charToString ((juce_wchar) ('A' + bi));
    return String (bi);
}

String SurveyInterface::banksSummary (const Array<Bank>& banks) const
{
    if (banks.size() == 0)
        return "All";
    StringArray parts;
    for (auto b : banks)
        parts.add (bankToString (b));
    return parts.joinIntoString (", ");
}

String SurveyInterface::shanksSummary (const Array<int>& shanks, int shankCount) const
{
    if (shanks.size() == 0)
        return shankCount > 1 ? String ("All") : String ("--");
    StringArray parts;
    for (auto s : shanks)
        parts.add (String (s + 1));
    return parts.joinIntoString (", ");
}

void SurveyInterface::showBanksSelector (int row, Component* anchor)
{
    if (row < 0 || row >= rows.size())
        return;

    if (anchor == nullptr && table != nullptr)
        anchor = table->getCellComponent (row, Columns::ColBanks);

    auto* anchorButton = dynamic_cast<TextButton*> (anchor);
    Component::SafePointer<TextButton> safeButton (anchorButton);

    auto& r = rows.getReference (row);
    StringArray labels;
    for (auto bank : r.availableBanks)
        labels.add (bankToString (bank));

    auto selector = std::make_unique<BankSelectorComponent> (r.availableBanks, labels, r.chosenBanks, [this, row, safeButton] (const Array<Bank>& selection)
                                                             {
                                                                 auto& rowState = rows.getReference (row);
                                                                 rowState.chosenBanks = selection;
                                                                 if (auto* btn = safeButton.getComponent())
                                                                     btn->setButtonText (banksSummary (rowState.chosenBanks));
                                                                 if (table != nullptr)
                                                                     table->repaintRow (row); });

    if (anchor != nullptr)
    {
        CallOutBox::launchAsynchronously (std::move (selector), anchor->getScreenBounds(), nullptr);
    }
    else
    {
        auto bounds = getScreenBounds().withSizeKeepingCentre (200, 150);
        CallOutBox::launchAsynchronously (std::move (selector), bounds, this);
    }
}

void SurveyInterface::showShanksSelector (int row, Component* anchor)
{
    if (row < 0 || row >= rows.size())
        return;

    auto& r = rows.getReference (row);
    if (r.shankCount <= 1)
        return;

    if (anchor == nullptr && table != nullptr)
        anchor = table->getCellComponent (row, Columns::ColShanks);

    auto* anchorButton = dynamic_cast<TextButton*> (anchor);
    Component::SafePointer<TextButton> safeButton (anchorButton);

    auto selector = std::make_unique<ShankSelectorComponent> (r.shankCount, r.chosenShanks, [this, row, safeButton] (const Array<int>& selection)
                                                              {
                                                                   auto& rowState = rows.getReference (row);
                                                                   rowState.chosenShanks = selection;
                                                                   if (auto* btn = safeButton.getComponent())
                                                                       btn->setButtonText (shanksSummary (rowState.chosenShanks, rowState.shankCount));
                                                                   if (table != nullptr)
                                                                       table->repaintRow (row); });

    if (anchor != nullptr)
    {
        CallOutBox::launchAsynchronously (std::move (selector), anchor->getScreenBounds(), nullptr);
    }
    else
    {
        auto bounds = getScreenBounds().withSizeKeepingCentre (200, 150);
        CallOutBox::launchAsynchronously (std::move (selector), bounds, this);
    }
}
