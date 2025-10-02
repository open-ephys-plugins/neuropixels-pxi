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
#include "NeuropixInterface.h"
#include "ProbeBrowser.h"

#include <cmath>
#include <functional>
#include <utility>

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
            bankButtons[i]->setToggleState (selection.contains (bank), dontSendNotification);
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
        if (! isPositiveAndBelow (idx, availableBanks.size()))
            return;

        const Bank bank = availableBanks[idx];
        if (selection.contains (bank))
            selection.removeFirstMatchingValue (bank);
        else
            selection.add (bank);

        selection.sort();

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
            shankButtons[i]->setToggleState (selection.contains (shank), dontSendNotification);
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
            selection.removeFirstMatchingValue (idx);
        else
            selection.add (idx);

        selection.sort();

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
constexpr int surveyProbePanelSpacing = 20;
} // namespace

SurveyProbePanel::SurveyProbePanel (Probe* p) : probe (p)
{
    title = std::make_unique<Label>();
    title->setJustificationType (Justification::centred);
    title->setFont (FontOptions ("Inter", "Semi Bold", 20.0f));
    title->setInterceptsMouseClicks (false, false);
    addAndMakeVisible (*title);

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

    if (probeBrowser != nullptr)
        probeBrowser->repaint();
}

void SurveyProbePanel::paint (Graphics& g)
{
    auto panelBounds = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (findColour (ThemeColours::componentParentBackground).withAlpha (0.5f));
    g.fillRoundedRectangle (panelBounds, 8.0f);

    g.setColour (findColour (ThemeColours::outline).withAlpha (0.75f));
    g.drawRoundedRectangle (panelBounds, 8.0f, 1.0f);
}

void SurveyProbePanel::resized()
{
    auto bounds = getLocalBounds().reduced (14);
    auto header = bounds.removeFromTop (34);
    if (title != nullptr)
        title->setBounds (header);

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

// --------------------- SurveyRunner -------------------------

SurveyRunner::SurveyRunner (NeuropixThread* t, NeuropixEditor* e, const Array<SurveyTarget>& targetsToSurvey, float secondsPerConfig)
    : ThreadWithProgressWindow ("Running survey", true, true),
      thread (t),
      editor (e),
      targets (targetsToSurvey),
      secondsPer (secondsPerConfig)
{
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

    for (int i = 0; i < maxSteps && ! threadShouldExit(); ++i)
    {
        setProgress (i / (float) maxSteps);
        setStatusMessage ("Surveying probes... Step " + String (i + 1) + "/" + String (maxSteps));
        LOGD ("SurveyRunner: Step ", i + 1, "/", maxSteps);

        int targetIdx = 0;
        for (auto& target : targets)
        {
            Probe* probe = target.probe;

            if (shanksIndices[targetIdx] < target.shanks.size()
                && bankIndices[targetIdx] < target.banks.size())
            {
                int sh = target.shanks[shanksIndices[targetIdx]];
                Bank bank = target.banks[bankIndices[targetIdx]];

                LOGD ("SurveyRunner: Applying settings to probe ", probe->getName().toRawUTF8(), " - Bank=", SurveyInterface::bankToString (bank).toRawUTF8(), " Shank=", sh);

                // Build settings for this combo
                for (const auto& config : target.electrodeConfigs)
                {
                    if (config.containsIgnoreCase ("Bank " + SurveyInterface::bankToString (bank)))
                    {
                        if (target.shankCount > 1 && config.containsIgnoreCase ("Shank " + String (sh + 1)))
                        {
                            auto selected = probe->selectElectrodeConfiguration (config);
                            probe->ui->selectElectrodes (selected);
                            LOGD ("SurveyRunner: Selected configuration ", config.toRawUTF8(), " for probe ", probe->getName().toRawUTF8());
                            break;
                        }
                        else if (target.shankCount == 1)
                        {
                            auto selected = probe->selectElectrodeConfiguration (config);
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

        // Start acquisition for this window
        CoreServices::setAcquisitionStatus (true);
        LOGD ("SurveyRunner: Acquisition started for step ", i + 1);

        Time::waitForMillisecondCounter (Time::getMillisecondCounter() + (secondsPer * 1000) + 100);

        // Stop acquisition for this window before proceeding to next config
        CoreServices::setAcquisitionStatus (false);
        LOGD ("SurveyRunner: Acquisition stopped for step ", i + 1);

        Time::waitForMillisecondCounter (Time::getMillisecondCounter() + 100);
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

// --------------------- SurveyInterface -------------------------

SurveyInterface::SurveyInterface (NeuropixThread* t, NeuropixEditor* e, NeuropixCanvas* c)
    : SettingsInterface (nullptr, t, e, c), thread (t), editor (e), canvas (c)
{
    type = SettingsInterface::SURVEY_SETTINGS_INTERFACE;

    secondsPerBankSlider = std::make_unique<Slider> (Slider::LinearHorizontal, Slider::TextBoxRight);
    secondsPerBankSlider->setRange (1, 30.0, 1.0);
    secondsPerBankSlider->setValue (2.0);
    addAndMakeVisible (*secondsPerBankSlider);

    runButton = std::make_unique<UtilityButton> ("RUN SURVEY...");
    runButton->setToggleState (true, dontSendNotification);
    runButton->addListener (this);
    addAndMakeVisible (*runButton);

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
    const float leftPanelWidth = 510.0f;
    const float panelHeight = (float) getHeight() - 20.0f;

    g.setColour (findColour (ThemeColours::componentBackground));
    g.fillRoundedRectangle (leftPanelX, 10.0f, leftPanelWidth, panelHeight, 8.0f);

    Rectangle<int> rightArea = getLocalBounds();
    rightArea.removeFromLeft ((int) (leftPanelX + leftPanelWidth) + 20);
    if (! rightArea.isEmpty())
    {
        g.fillRoundedRectangle (rightArea.reduced (10).toFloat(), 8.0f);
    }

    g.setColour (findColour (ThemeColours::defaultText));
    g.setFont (FontOptions ("Inter", "Medium", 16.0f));
    g.drawText ("Seconds per bank/shank:", 30, 80, 200, 25, Justification::centredLeft);
}

void SurveyInterface::resized()
{
    const int leftPanelX = 10;
    const int leftPanelWidth = 510;
    const int topMargin = 10;

    if (runButton != nullptr)
        runButton->setBounds (leftPanelX + (leftPanelWidth - 140) / 2, topMargin + 20, 140, 30);

    if (secondsPerBankSlider != nullptr)
        secondsPerBankSlider->setBounds (200, topMargin + 70, 220, 25);

    if (table != nullptr)
    {
        const int tableTop = topMargin + 120;
        const int desiredHeight = (getNumRows() + 1) * table->getRowHeight() + 8;
        const int availableHeight = getHeight() - tableTop - 40;
        table->setBounds (leftPanelX + 20, tableTop, leftPanelWidth - 38, jmin (desiredHeight, availableHeight));
    }

    if (probeViewport != nullptr)
    {
        Rectangle<int> rightArea = getLocalBounds();
        rightArea.removeFromLeft (leftPanelX + leftPanelWidth + 20);
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
    if (b == runButton.get())
        launchSurvey();
}

void SurveyInterface::comboBoxChanged (ComboBox* cb)
{
}

void SurveyInterface::startAcquisition()
{
    for (auto& panel : probePanels)
    {
        if (panel->getProbe()->getEnabledForSurvey())
            panel->getProbeBrowser()->startTimer (100);
    }
}

void SurveyInterface::stopAcquisition()
{
    for (auto& panel : probePanels)
    {
        panel->getProbeBrowser()->stopTimer();
    }
}

void SurveyInterface::updateInfoString()
{
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
            panel->setBounds (x, 0, SurveyProbePanel::width, SurveyProbePanel::minHeight);
            panel->refresh();
            probeViewportContent->addAndMakeVisible (panel.get());
            probePanels.add (panel.release());

            x += SurveyProbePanel::width + surveyProbePanelSpacing;
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

        panel->setBounds (x, 20, SurveyProbePanel::width, contentHeight - 40);
        x += SurveyProbePanel::width + surveyProbePanelSpacing;
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

            if (r.probe->type == ProbeType::NP2_4 && b == Bank::D)
                continue; // Bank D not available on NP2 4-shank

            r.availableBanks.add (b);
        }

        r.shankCount = p->type == ProbeType::QUAD_BASE ? 1 : jmax (1, p->probeMetadata.shank_count);
        rows.add (r);
    }
    if (table)
        table->updateContent();

    rebuildProbePanels();
}

void SurveyInterface::launchSurvey()
{
    // Disable all controls during acquisition
    runButton->setEnabled (false);
    secondsPerBankSlider->setEnabled (false);
    table->setEnabled (false);

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
        return;
    }

    std::unique_ptr<SurveyRunner> runner = std::make_unique<SurveyRunner> (thread, editor, targets, (float) secondsPerBankSlider->getValue());
    runner->runThread();

    // Restore activity views to normal mode
    for (const auto& r : rows)
    {
        r.probe->setSurveyMode (false, false);
    }

    // Re-enable controls
    runButton->setEnabled (true);
    secondsPerBankSlider->setEnabled (true);
    table->setEnabled (true);
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
