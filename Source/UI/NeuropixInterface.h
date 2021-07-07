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

#ifndef __NEUROPIXINTERFACE_H_2C4C2D67__
#define __NEUROPIXINTERFACE_H_2C4C2D67__

#include <VisualizerEditorHeaders.h>

#include "SettingsInterface.h"
#include "../NeuropixComponents.h"
#include "ColourScheme.h"

class ColorSelector;
class Annotation;

class NeuropixInterface : public SettingsInterface, 
	public Button::Listener, 
	public ComboBox::Listener, 
	public Label::Listener, 
	public Timer
{
public:
	NeuropixInterface(DataSource* probe, NeuropixThread* thread, NeuropixEditor* editor, NeuropixCanvas* canvas);
	~NeuropixInterface();

	void paint(Graphics& g);

	void mouseMove(const MouseEvent& event);
	void mouseDown(const MouseEvent& event);
	void mouseDrag(const MouseEvent& event);
	void mouseUp(const MouseEvent& event);
	//void mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel);
	MouseCursor getMouseCursor();

	void buttonClicked(Button*);
	void comboBoxChanged(ComboBox*);
	void labelTextChanged(Label* l);

	void startAcquisition();
	void stopAcquisition();

	void applyProbeSettings(ProbeSettings, bool shouldUpdateProbe = true);
	ProbeSettings getProbeSettings();
	void updateProbeSettingsInBackground();

	void saveParameters(XmlElement* xml);
	void loadParameters(XmlElement* xml);

	void setAnnotationLabel(String, Colour);
	void updateInfoString();

	void timerCallback();

private:

	Probe* probe;

	XmlElement neuropix_info;

	// Combo box - probe-specific settings
	ScopedPointer<ComboBox> lfpGainComboBox;
	ScopedPointer<ComboBox> apGainComboBox;
	ScopedPointer<ComboBox> referenceComboBox;
	ScopedPointer<ComboBox> filterComboBox;
	ScopedPointer<ComboBox> activityViewComboBox;

	// Combo box - basestation settings
	ScopedPointer<ComboBox> bistComboBox;
	ScopedPointer<ComboBox> bscFirmwareComboBox;
	ScopedPointer<ComboBox> bsFirmwareComboBox;

	// LABELS
	ScopedPointer<Viewport> infoLabelView;
	ScopedPointer<Label> mainLabel;
	ScopedPointer<Label> nameLabel;
	ScopedPointer<Label> infoLabel;
	ScopedPointer<Label> lfpGainLabel;
	ScopedPointer<Label> apGainLabel;
	ScopedPointer<Label> referenceLabel;
	ScopedPointer<Label> filterLabel;
	ScopedPointer<Label> bankViewLabel;
	ScopedPointer<Label> activityViewLabel;
	
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

	ScopedPointer<ColorSelector> colorSelector;

	Array<ElectrodeMetadata> electrodeMetadata;
	ProbeMetadata probeMetadata;

	// display booleans
	bool isOverZoomRegion;
	bool isOverUpperBorder;
	bool isOverLowerBorder;
	bool isOverElectrode;
	bool isSelectionActive;

	// display variables
	int zoomHeight;
	int zoomOffset;
	int initialOffset;
	int initialHeight;
	int lowerBound;
	int dragZoneWidth;
	int zoomAreaMinRow;
	int minZoomHeight;
	int maxZoomHeight;
	int shankOffset;
	int leftEdge;
	int rightEdge;
	int channelLabelSkip;
	int pixelHeight;

	int lowestElectrode;
	int highestElectrode;

	float electrodeHeight;

	enum VisualizationMode {
		ENABLE_VIEW,
		AP_GAIN_VIEW,
		LFP_GAIN_VIEW,
		REFERENCE_VIEW,
		ACTIVITY_VIEW
	};

	VisualizationMode mode;

	MouseCursor::StandardCursorType cursorType;

	Path shankPath;

	String electrodeInfoString;

	Colour getElectrodeColour(int index);
	int getNearestElectrode(int x, int y);
	Array<int> getElectrodesWithinBounds(int x, int y, int w, int h);
	String getElectrodeInfoString(int index);

	void drawLegend(Graphics& g);
	void drawAnnotations(Graphics& g);

	Array<Annotation> annotations;

	Array<int> getSelectedElectrodes();

	Array<BIST> availableBists;

	// For activity view, may use later
	//DataBuffer* inputBuffer;
	//AudioSampleBuffer displayBuffer;

};

class Annotation
{
public:
	Annotation(String text, Array<int> channels, Colour c);
	~Annotation();

	Array<int> electrodes;
	String text;

	float currentYLoc;

	bool isMouseOver;
	bool isSelected;

	Colour colour;

};

class ColorSelector : public Component, public ButtonListener
{
public:
	ColorSelector(NeuropixInterface* np);
	~ColorSelector();

	Array<Colour> standardColors;
	Array<Colour> hoverColors;
	StringArray strings;

	OwnedArray<ShapeButton> buttons;

	void buttonClicked(Button* button);

	void updateCurrentString(String s);

	Colour getCurrentColour();

	NeuropixInterface* npi;

	int activeButton;

};

#endif //__NEUROPIXINTERFACE_H_2C4C2D67__