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

#ifndef __NEUROPIXINTERFACE_H_2C4C2D67__
#define __NEUROPIXINTERFACE_H_2C4C2D67__

#include <VisualizerEditorHeaders.h>

#include "../NeuropixComponents.h"
#include "ColourScheme.h"
#include "SettingsInterface.h"

class AnnotationColourSelector;
class ProbeBrowser;

enum VisualizationMode
{
    ENABLE_VIEW,
    AP_GAIN_VIEW,
    LFP_GAIN_VIEW,
    REFERENCE_VIEW,
    ACTIVITY_VIEW
};

class Annotation
{
public:
    Annotation (String text, Array<int> channels, Colour c);
    ~Annotation();

    Array<int> electrodes;
    String text;

    float currentYLoc;

    bool isMouseOver;
    bool isSelected;

    Colour colour;
};

/** 

	Extended graphical interface for updating probe settings

*/
class NeuropixInterface : public SettingsInterface,
                          public Button::Listener,
                          public ComboBox::Listener,
                          public Label::Listener
{
public:
    friend class ProbeBrowser;

    /** Constructor */
    NeuropixInterface (DataSource* probe, NeuropixThread* thread, NeuropixEditor* editor, NeuropixCanvas* canvas, Basestation* basestation = nullptr);

    /** Destructor */
    ~NeuropixInterface();

    /** Draws the legend */
    void paint (Graphics& g);

    /** Listener methods*/
    void buttonClicked (Button*);
    void comboBoxChanged (ComboBox*);
    void labelTextChanged (Label* l);

    /** Disables buttons and starts animation if necessary */
    void startAcquisition();

    /** Enables buttons and start animation if necessary */
    void stopAcquisition();

    /** Settings-related functions*/
    bool applyProbeSettings (ProbeSettings, bool shouldUpdateProbe = true);
    void applyProbeSettingsFromImro (File imroFile);
    ProbeSettings getProbeSettings();
    void updateProbeSettingsInBackground();

    /** Save parameters to XML */
    void saveParameters (XmlElement* xml);

    /** Load parameters from XML */
    void loadParameters (XmlElement* xml);

    /** Updates the annotation label */
    void setAnnotationLabel (String, Colour);

    /** Updates the info string on the right-hand side of the component */
    void updateInfoString();

    /** Set parameters */
    void setApGain (int index);
    void setLfpGain (int index);
    void setReference (int index);
    void setApFilterState (bool state);
    void setEmissionSite (String wavelength, int site);
    void selectElectrodes (Array<int> electrodes);

    float getMaxPeakToPeakValue() const { return currentMaxPeakToPeak; }

    Probe* probe;

    Basestation* basestation;

private:
    Array<ElectrodeMetadata> electrodeMetadata;
    ProbeMetadata probeMetadata;

    XmlElement neuropix_info;

    bool acquisitionIsActive = false;

    // Combo box - probe-specific settings
    std::unique_ptr<ComboBox> electrodeConfigurationComboBox;
    std::unique_ptr<ComboBox> lfpGainComboBox;
    std::unique_ptr<ComboBox> apGainComboBox;
    std::unique_ptr<ComboBox> referenceComboBox;
    std::unique_ptr<ComboBox> filterComboBox;
    std::unique_ptr<ComboBox> activityViewComboBox;
    std::unique_ptr<ComboBox> activityViewAmplitudeComboBox;
    std::unique_ptr<ComboBox> redEmissionSiteComboBox;
    std::unique_ptr<ComboBox> blueEmissionSiteComboBox;

    // Combo box - basestation settings
    std::unique_ptr<ComboBox> bistComboBox;
    std::unique_ptr<ComboBox> bscFirmwareComboBox;
    std::unique_ptr<ComboBox> bsFirmwareComboBox;

    // Combo box - probe settings
    std::unique_ptr<ComboBox> loadImroComboBox;

    // LABELS
    std::unique_ptr<Viewport> infoLabelView;
    std::unique_ptr<Label> nameLabel;
    std::unique_ptr<Label> infoLabel;
    std::unique_ptr<Label> lfpGainLabel;
    std::unique_ptr<Label> apGainLabel;
    std::unique_ptr<Label> electrodesLabel;
    std::unique_ptr<Label> electrodePresetLabel;
    std::unique_ptr<Label> referenceLabel;
    std::unique_ptr<Label> filterLabel;
    std::unique_ptr<Label> bankViewLabel;
    std::unique_ptr<Label> activityViewLabel;
    std::unique_ptr<Label> redEmissionSiteLabel;
    std::unique_ptr<Label> blueEmissionSiteLabel;

    std::unique_ptr<Label> bistLabel;
    std::unique_ptr<Label> bscFirmwareLabel;
    std::unique_ptr<Label> bsFirmwareLabel;
    std::unique_ptr<Label> firmwareInstructionsLabel;

    std::unique_ptr<Label> probeSettingsLabel;

    std::unique_ptr<Label> annotationLabelLabel;
    std::unique_ptr<Label> annotationLabel;

    // BUTTONS
    std::unique_ptr<UtilityButton> probeEnableButton;
    std::unique_ptr<UtilityButton> enableButton;

    std::unique_ptr<UtilityButton> enableViewButton;
    std::unique_ptr<UtilityButton> lfpGainViewButton;
    std::unique_ptr<UtilityButton> apGainViewButton;
    std::unique_ptr<UtilityButton> referenceViewButton;
    std::unique_ptr<UtilityButton> bankViewButton;
    std::unique_ptr<UtilityButton> activityViewButton;
    std::unique_ptr<UtilityButton> activityViewFilterButton;
    std::unique_ptr<UtilityButton> activityViewCARButton;

    std::unique_ptr<UtilityButton> annotationButton;
    std::unique_ptr<UtilityButton> bistButton;
    std::unique_ptr<UtilityButton> bsFirmwareButton;
    std::unique_ptr<UtilityButton> bscFirmwareButton;
    std::unique_ptr<UtilityButton> firmwareToggleButton;

    std::unique_ptr<UtilityButton> copyButton;
    std::unique_ptr<UtilityButton> pasteButton;
    std::unique_ptr<UtilityButton> applyToAllButton;
    std::unique_ptr<UtilityButton> loadImroButton;
    std::unique_ptr<UtilityButton> saveImroButton;
    std::unique_ptr<UtilityButton> loadJsonButton;
    std::unique_ptr<UtilityButton> saveJsonButton;

    std::unique_ptr<AnnotationColourSelector> annotationColourSelector;

    std::unique_ptr<ProbeBrowser> probeBrowser;

    VisualizationMode mode;

    void drawLegend (Graphics& g);
    void drawAnnotations (Graphics& g);

    Array<Annotation> annotations;

    Array<int> getSelectedElectrodes();

    Array<BIST> availableBists;
    Array<String> imroFiles;
    Array<bool> imroLoadedFromFolder;

    Array<float> amplitudeOptions { 250.0f, 500.0f, 750.0f, 1000.0f };
    float currentMaxPeakToPeak { 500.0f };
};

/**

	Extended graphical interface for updating basestation settings

*/
class BasestationInterface : public NeuropixInterface
{
public:
    /** Constructor */
    BasestationInterface (Basestation* basestation_, NeuropixThread* thread, NeuropixEditor* editor, NeuropixCanvas* canvas)
        : NeuropixInterface (nullptr, thread, editor, canvas, basestation_)
    {
    }
};

class AnnotationColourSelector : public Component, public Button::Listener
{
public:
    AnnotationColourSelector (NeuropixInterface* np);
    ~AnnotationColourSelector();

    Array<Colour> standardColours;
    Array<Colour> hoverColours;
    StringArray strings;

    OwnedArray<ShapeButton> buttons;

    void buttonClicked (Button* button);

    void updateCurrentString (String s);

    Colour getCurrentColour();

    NeuropixInterface* npi;

    int activeButton;
};

#endif //__NEUROPIXINTERFACE_H_2C4C2D67__