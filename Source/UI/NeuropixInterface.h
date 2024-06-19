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

    Probe* probe;

    Basestation* basestation;

private:
    Array<ElectrodeMetadata> electrodeMetadata;
    ProbeMetadata probeMetadata;

    XmlElement neuropix_info;

    bool acquisitionIsActive = false;

    // Combo box - probe-specific settings
    ScopedPointer<ComboBox> electrodeConfigurationComboBox;
    ScopedPointer<ComboBox> lfpGainComboBox;
    ScopedPointer<ComboBox> apGainComboBox;
    ScopedPointer<ComboBox> referenceComboBox;
    ScopedPointer<ComboBox> filterComboBox;
    ScopedPointer<ComboBox> activityViewComboBox;
    ScopedPointer<ComboBox> redEmissionSiteComboBox;
    ScopedPointer<ComboBox> blueEmissionSiteComboBox;

    // Combo box - basestation settings
    ScopedPointer<ComboBox> bistComboBox;
    ScopedPointer<ComboBox> bscFirmwareComboBox;
    ScopedPointer<ComboBox> bsFirmwareComboBox;

    // Combo box - probe settings
    ScopedPointer<ComboBox> loadImroComboBox;

    // LABELS
    ScopedPointer<Viewport> infoLabelView;
    ScopedPointer<Label> nameLabel;
    ScopedPointer<Label> infoLabel;
    ScopedPointer<Label> lfpGainLabel;
    ScopedPointer<Label> apGainLabel;
    ScopedPointer<Label> electrodesLabel;
    ScopedPointer<Label> electrodePresetLabel;
    ScopedPointer<Label> referenceLabel;
    ScopedPointer<Label> filterLabel;
    ScopedPointer<Label> bankViewLabel;
    ScopedPointer<Label> activityViewLabel;
    ScopedPointer<Label> redEmissionSiteLabel;
    ScopedPointer<Label> blueEmissionSiteLabel;

    ScopedPointer<Label> bistLabel;
    ScopedPointer<Label> bscFirmwareLabel;
    ScopedPointer<Label> bsFirmwareLabel;
    ScopedPointer<Label> firmwareInstructionsLabel;

    ScopedPointer<Label> probeSettingsLabel;

    ScopedPointer<Label> annotationLabelLabel;
    ScopedPointer<Label> annotationLabel;

    // BUTTONS
    ScopedPointer<UtilityButton> enableButton;

    ScopedPointer<UtilityButton> enableViewButton;
    ScopedPointer<UtilityButton> lfpGainViewButton;
    ScopedPointer<UtilityButton> apGainViewButton;
    ScopedPointer<UtilityButton> referenceViewButton;
    ScopedPointer<UtilityButton> bankViewButton;
    ScopedPointer<UtilityButton> activityViewButton;

    ScopedPointer<UtilityButton> annotationButton;
    ScopedPointer<UtilityButton> bistButton;
    ScopedPointer<UtilityButton> bsFirmwareButton;
    ScopedPointer<UtilityButton> bscFirmwareButton;
    ScopedPointer<UtilityButton> firmwareToggleButton;

    ScopedPointer<UtilityButton> copyButton;
    ScopedPointer<UtilityButton> pasteButton;
    ScopedPointer<UtilityButton> applyToAllButton;
    ScopedPointer<UtilityButton> loadImroButton;
    ScopedPointer<UtilityButton> saveImroButton;
    ScopedPointer<UtilityButton> loadJsonButton;
    ScopedPointer<UtilityButton> saveJsonButton;

    ScopedPointer<AnnotationColourSelector> annotationColourSelector;

    ScopedPointer<ProbeBrowser> probeBrowser;

    VisualizationMode mode;

    void drawLegend (Graphics& g);
    void drawAnnotations (Graphics& g);

    Array<Annotation> annotations;

    Array<int> getSelectedElectrodes();

    Array<BIST> availableBists;
    Array<String> imroFiles;
    Array<bool> imroLoadedFromFolder;
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