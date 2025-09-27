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
            auto* btn = new UtilityButton (String (i));
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

        refreshButtonStates();
        notifySelectionChanged();
    }

    int shankCount;
    Array<int> selection;
    std::unique_ptr<TextButton> allButton;
    OwnedArray<UtilityButton> shankButtons;
    std::function<void (const Array<int>&)> onSelectionChanged;
};

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

    // For progress
    int totalSteps = 0;
    for (const auto& tgt : targets)
    {
        auto* p = tgt.probe;
        int shanks = tgt.shanks.size() > 0 ? tgt.shanks.size() : jmax (1, p->probeMetadata.shank_count);
        int banks = tgt.banks.size() > 0 ? tgt.banks.size() : jmax (1, p->settings.availableBanks.size());
        totalSteps += shanks * banks;
    }

    int completed = 0;

    // Ensure settings queue is idle
    if (editor->uiLoader->isThreadRunning())
        editor->uiLoader->waitForThreadToExit (20000);

    for (const auto& target : targets)
    {
        Probe* probe = target.probe;
        // Make a working copy of settings
        ProbeSettings base = probe->settings;
        base.clearElectrodeSelection();

        // select initial population as currently connected electrodes, but we'll change bank/shank selection per combo
        for (int i = 0; i < probe->channel_count; ++i)
        {
            base.selectedChannel.add (i);
            // fill placeholders; we overwrite bank/shank per iteration
            base.selectedBank.add (Bank::A);
            base.selectedShank.add (0);
            // map channel -> an electrode index with same channel to keep metadata lookup consistent
            int fallbackElectrode = i;
            for (int eIdx = 0; eIdx < probe->electrodeMetadata.size(); ++eIdx)
            {
                if (probe->electrodeMetadata[eIdx].channel == i)
                {
                    fallbackElectrode = eIdx;
                    break;
                }
            }
            base.selectedElectrode.add (fallbackElectrode);
        }

        Array<Bank> banks = (target.banks.size() > 0 ? target.banks : probe->settings.availableBanks);
        if (banks.size() == 0)
            banks.add (Bank::A);
        Array<int> shanksList;
        if (target.shanks.size() > 0)
            shanksList = target.shanks;
        else
            for (int i = 0; i < jmax (1, probe->probeMetadata.shank_count); ++i)
                shanksList.add (i);

        for (int shIndex = 0; shIndex < shanksList.size() && ! threadShouldExit(); ++shIndex)
        {
            int sh = shanksList[shIndex];
            for (int bi = 0; bi < banks.size() && ! threadShouldExit(); ++bi)
            {
                Bank bank = banks[bi];

                // Build settings for this combo
                ProbeSettings pset = base;
                pset.selectedBank.clear();
                pset.selectedShank.clear();
                pset.selectedElectrode.clear();

                // Assign per-channel bank/shank and resolve electrodes to this bank/shank
                for (int ch = 0; ch < probe->channel_count; ++ch)
                {
                    pset.selectedBank.add (bank);
                    pset.selectedShank.add (sh);

                    // Find an electrode that maps to this ch, bank, shank; if not found, keep fallback mapping
                    int elecIndex = -1;
                    for (int eIdx = 0; eIdx < probe->electrodeMetadata.size(); ++eIdx)
                    {
                        const auto& em = probe->electrodeMetadata[eIdx];
                        if (em.channel == ch && em.shank == sh && em.bank == bank)
                        {
                            elecIndex = eIdx;
                            break;
                        }
                    }
                    if (elecIndex < 0)
                        elecIndex = pset.selectedElectrode[jmin (jmax (0, ch), pset.selectedElectrode.size() - 1)];

                    pset.selectedElectrode.add (elecIndex);
                }

                // Ensure acquisition is stopped for reconfiguration
                thread->stopAcquisition();
                Time::waitForMillisecondCounter (Time::getMillisecondCounter() + 150);

                // Queue settings and apply in background (UI thread orchestrates the write) while stopped
                pset.probe = probe;
                probe->updateSettings (pset);
                thread->updateProbeSettingsQueue (pset);

                // Apply queued settings now; we wait until uiLoader finishes after run() returns
                if (! editor->uiLoader->isThreadRunning())
                    editor->uiLoader->startThread();

                // Wait for settings to apply before measuring
                while (editor->uiLoader->isThreadRunning() && ! threadShouldExit())
                    Time::waitForMillisecondCounter (Time::getMillisecondCounter() + 10);

                // Prepare activity views for survey averaging and start acquisition
                probe->setActivityViewSurveyMode (true, ActivityToView::APVIEW);
                if (probe->generatesLfpData())
                    probe->setActivityViewSurveyMode (true, ActivityToView::LFPVIEW);

                // Start acquisition for this window
                thread->startAcquisition();

                // Reset activity views and allow a short settle to avoid mixing prior config data
                Time::waitForMillisecondCounter (Time::getMillisecondCounter() + 300);

                // Collect P2P over the requested window; sample at ~10 Hz (for both AP and LFP if available)
                const double startMs = Time::getMillisecondCounterHiRes();
                const double durationMs = secondsPer * 1000.0;
                while ((Time::getMillisecondCounterHiRes() - startMs) < durationMs && ! threadShouldExit())
                {
                    probe->getPeakToPeakValues (ActivityToView::APVIEW);

                    if (probe->generatesLfpData())
                        probe->getPeakToPeakValues (ActivityToView::LFPVIEW);

                    Time::waitForMillisecondCounter (Time::getMillisecondCounter() + 100); // ~10 Hz
                }

                // Stop acquisition for this window before proceeding to next config
                thread->stopAcquisition();
                Time::waitForMillisecondCounter (Time::getMillisecondCounter() + 100);

                const int electrodeCount = probe->electrodeMetadata.size();

                HeapBlock<float> averagedAP (electrodeCount);
                const float* apAverages = probe->getPeakToPeakValues (ActivityToView::APVIEW);
                if (apAverages != nullptr)
                {
                    for (int i = 0; i < electrodeCount; ++i)
                        averagedAP[i] = apAverages[i];
                }
                else
                {
                    for (int i = 0; i < electrodeCount; ++i)
                        averagedAP[i] = 0.0f;
                }

                HeapBlock<float> averagedLFP;
                if (probe->generatesLfpData())
                {
                    averagedLFP.malloc ((size_t) electrodeCount);
                    const float* lfpAverages = probe->getPeakToPeakValues (ActivityToView::LFPVIEW);
                    if (lfpAverages != nullptr)
                    {
                        for (int i = 0; i < electrodeCount; ++i)
                            averagedLFP[i] = lfpAverages[i];
                    }
                    else
                    {
                        for (int i = 0; i < electrodeCount; ++i)
                            averagedLFP[i] = 0.0f;
                    }
                }

                probe->setActivityViewSurveyMode (false, ActivityToView::APVIEW, false);
                if (probe->generatesLfpData())
                    probe->setActivityViewSurveyMode (false, ActivityToView::LFPVIEW, false);

                completed++;
                setStatusMessage ("Surveying " + probe->getName() + ": bank " + String ((int) bank) + ", shank " + String (sh + 1));
                setProgress ((double) completed / (double) totalSteps);
            }
        }
    }

    // Ensure acquisition is stopped at the end
    thread->stopAcquisition();
}

// --------------------- SurveyInterface -------------------------

SurveyInterface::SurveyInterface (NeuropixThread* t, NeuropixEditor* e, NeuropixCanvas* c)
    : SettingsInterface (nullptr, t, e, c), thread (t), editor (e), canvas (c)
{
    type = SettingsInterface::UNKNOWN_SETTINGS_INTERFACE;

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
    addAndMakeVisible (*table);

    refreshProbeList();
}

void SurveyInterface::paint (Graphics& g)
{
    g.fillAll (findColour (ThemeColours::componentParentBackground));

    g.setColour (findColour (ThemeColours::componentBackground));
    g.fillRoundedRectangle (10.0f, 10.0f, 510.0f, getHeight() - 20.0f, 8.0f);

    g.setColour (findColour (ThemeColours::defaultText));
    g.setFont (FontOptions ("Inter", "Medium", 16.0f));
    g.drawText ("Seconds per bank/shank:", 30, 70, 200, 25, Justification::centredLeft);
}

void SurveyInterface::resized()
{
    runButton->setBounds (205, 30, 120, 25);

    secondsPerBankSlider->setBounds (190, 70, 250, 25);

    table->setBounds (30, 120, 470, (getNumRows() + 1) * table->getRowHeight() + 10);
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
    // Disable all controls during acquisition
    runButton->setEnabled (false);
    secondsPerBankSlider->setEnabled (false);
    table->setEnabled (false);
}

void SurveyInterface::stopAcquisition()
{
    // Re-enable controls
    runButton->setEnabled (true);
    secondsPerBankSlider->setEnabled (true);
    table->setEnabled (true);
}

void SurveyInterface::refreshProbeList()
{
    rows.clear();
    for (auto* p : thread->getProbes())
    {
        RowState r;
        r.probe = p;
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
}

void SurveyInterface::launchSurvey()
{
    Array<SurveyTarget> targets;
    for (const auto& r : rows)
    {
        if (! r.selected)
            continue;
        SurveyTarget t;
        t.probe = r.probe;
        t.banks = r.chosenBanks; // empty => all
        t.shanks = r.chosenShanks; // empty => all
        targets.add (t);
    }

    if (targets.size() == 0)
    {
        CoreServices::sendStatusMessage ("No probes selected for survey.");
        return;
    }

    std::unique_ptr<SurveyRunner> runner = std::make_unique<SurveyRunner> (thread, editor, targets, (float) secondsPerBankSlider->getValue());
    runner->runThread();
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
        g.fillAll (bg.darker (0.15f));
    else
        g.fillAll (bg.darker (0.25f));
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
        parts.add (String (s));
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
