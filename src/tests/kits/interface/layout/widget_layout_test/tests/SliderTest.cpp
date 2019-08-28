/*
 * Copyright 2008, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "SliderTest.h"

#include <stdio.h>

#include <Message.h>
#include <Slider.h>

#include "CheckBox.h"
#include "GroupView.h"
#include "RadioButton.h"
#include "TestView.h"


// messages
enum {
	MSG_ORIENTATION_CHANGED			= 'orch',
	MSG_THUMB_STYLE_CHANGED			= 'tstc',
	MSG_HASH_MARKS_CHANGED			= 'hmch',
	MSG_BAR_THICKNESS_CHANGED		= 'btch',
	MSG_LABEL_CHANGED				= 'lbch',
	MSG_LIMIT_LABELS_CHANGED		= 'lmch',
	MSG_UPDATE_TEXT_CHANGED			= 'utch'
};


// TestSlider
class SliderTest::TestSlider : public BSlider {
public:
	TestSlider()
		: BSlider("test slider", "Label", NULL, 1, 100, B_HORIZONTAL),
		fExportUpdateText(false)
	{
	}

	virtual char* UpdateText() const
	{
		if (!fExportUpdateText)
			return NULL;
		sprintf(fUpdateText, "%ld", Value());
		return fUpdateText;
	}

	mutable char fUpdateText[32];
	bool fExportUpdateText;
};

// OrientationRadioButton
class SliderTest::OrientationRadioButton : public LabeledRadioButton {
public:
	OrientationRadioButton(const char* label, enum orientation orientation)
		: LabeledRadioButton(label),
		  fOrientation(orientation)
	{
	}

	enum orientation fOrientation;
};

// ThumbStyleRadioButton
class SliderTest::ThumbStyleRadioButton : public LabeledRadioButton {
public:
	ThumbStyleRadioButton(const char* label, enum thumb_style style)
		: LabeledRadioButton(label),
		  fStyle(style)
	{
	}

	thumb_style fStyle;
};

// HashMarkLocationRadioButton
class SliderTest::HashMarkLocationRadioButton : public LabeledRadioButton {
public:
	HashMarkLocationRadioButton(const char* label, enum hash_mark_location
		location)
		: LabeledRadioButton(label),
		  fLocation(location)
	{
	}

	hash_mark_location fLocation;
};


// ThicknessRadioButton
class SliderTest::ThicknessRadioButton : public LabeledRadioButton {
public:
	ThicknessRadioButton(const char* label, float thickness)
		: LabeledRadioButton(label),
		  fThickness(thickness)
	{
	}

	float fThickness;
};


// LabelRadioButton
class SliderTest::LabelRadioButton : public LabeledRadioButton {
public:
	LabelRadioButton(const char* label, const char* sliderLabel)
		: LabeledRadioButton(label),
		  fLabel(sliderLabel)
	{
	}

	const char*	fLabel;
};


// LimitLabelsRadioButton
class SliderTest::LimitLabelsRadioButton : public LabeledRadioButton {
public:
	LimitLabelsRadioButton(const char* label, const char* minLabel,
		const char* maxLabel)
		: LabeledRadioButton(label),
		  fMinLabel(minLabel),
		  fMaxLabel(maxLabel)
	{
	}

	const char*	fMinLabel;
	const char*	fMaxLabel;
};


// constructor
SliderTest::SliderTest()
	: Test("Slider", NULL),
	  fSlider(new TestSlider()),
	  fOrientationRadioGroup(NULL),
	  fThumbStyleRadioGroup(NULL),
	  fHashMarkLocationRadioGroup(NULL),
	  fBarThicknessRadioGroup(NULL),
	  fLabelRadioGroup(NULL),
	  fLimitLabelsRadioGroup(NULL),
	  fUpdateTextCheckBox(NULL)
{
	SetView(fSlider);
}


// destructor
SliderTest::~SliderTest()
{
	delete fOrientationRadioGroup;
	delete fThumbStyleRadioGroup;
	delete fHashMarkLocationRadioGroup;
	delete fBarThicknessRadioGroup;
	delete fLabelRadioGroup;
	delete fLimitLabelsRadioGroup;
}


// CreateTest
Test*
SliderTest::CreateTest()
{
	return new SliderTest;
}


// ActivateTest
void
SliderTest::ActivateTest(View* controls)
{
	// BSlider sets its background color to that of its parent in
	// AttachedToWindow(). Override.
	rgb_color background = ui_color(B_PANEL_BACKGROUND_COLOR);
	fSlider->SetViewColor(background);
	fSlider->SetLowColor(background);
	fSlider->SetHashMarkCount(10);

	GroupView* group = new GroupView(B_VERTICAL);
	group->SetFrame(controls->Bounds());
	group->SetSpacing(0, 4);
	controls->AddChild(group);

	GroupView* hGroup = new GroupView(B_HORIZONTAL);
	group->AddChild(hGroup);

	// the radio button group for selecting the orientation

	GroupView* vGroup = new GroupView(B_VERTICAL);
	vGroup->SetSpacing(0, 4);
	hGroup->AddChild(vGroup);

	fOrientationRadioGroup = new RadioButtonGroup(
		new BMessage(MSG_ORIENTATION_CHANGED), this);

	// horizontal
	LabeledRadioButton* button = new OrientationRadioButton("Horizontal",
		B_HORIZONTAL);
	vGroup->AddChild(button);
	fOrientationRadioGroup->AddButton(button->GetRadioButton());

	// vertical
	button = new OrientationRadioButton("Vertical", B_VERTICAL);
	vGroup->AddChild(button);
	fOrientationRadioGroup->AddButton(button->GetRadioButton());

	// default to horizontal
	fOrientationRadioGroup->SelectButton((int32)0);

	// the radio button group for selecting the thumb style

	vGroup = new GroupView(B_VERTICAL);
	vGroup->SetSpacing(0, 4);
	hGroup->AddChild(vGroup);

	fThumbStyleRadioGroup = new RadioButtonGroup(
		new BMessage(MSG_THUMB_STYLE_CHANGED), this);

	// block thumb
	button = new ThumbStyleRadioButton("Block thumb", B_BLOCK_THUMB);
	vGroup->AddChild(button);
	fThumbStyleRadioGroup->AddButton(button->GetRadioButton());

	// triangle thumb
	button = new ThumbStyleRadioButton("Triangle thumb", B_TRIANGLE_THUMB);
	vGroup->AddChild(button);
	fThumbStyleRadioGroup->AddButton(button->GetRadioButton());

	// default to block thumb
	fThumbStyleRadioGroup->SelectButton((int32)0);

	// spacing
	group->AddChild(new VStrut(10));

	// the radio button group for selecting the thumb style

	fHashMarkLocationRadioGroup = new RadioButtonGroup(
		new BMessage(MSG_HASH_MARKS_CHANGED), this);

	// no hash marks
	button = new HashMarkLocationRadioButton("No hash marks",
		B_HASH_MARKS_NONE);
	group->AddChild(button);
	fHashMarkLocationRadioGroup->AddButton(button->GetRadioButton());

	// left/top hash marks
	button = new HashMarkLocationRadioButton("Left/top hash marks",
		B_HASH_MARKS_TOP);
	group->AddChild(button);
	fHashMarkLocationRadioGroup->AddButton(button->GetRadioButton());

	// right/bottom hash marks
	button = new HashMarkLocationRadioButton("Right/bottom hash marks",
		B_HASH_MARKS_BOTTOM);
	group->AddChild(button);
	fHashMarkLocationRadioGroup->AddButton(button->GetRadioButton());

	// both side hash marks
	button = new HashMarkLocationRadioButton("Both sides hash marks",
		B_HASH_MARKS_BOTH);
	group->AddChild(button);
	fHashMarkLocationRadioGroup->AddButton(button->GetRadioButton());

	// default to no hash marks
	fHashMarkLocationRadioGroup->SelectButton((int32)0);

	// spacing
	group->AddChild(new VStrut(10));

	// the radio button group for selecting the bar thickness

	fBarThicknessRadioGroup = new RadioButtonGroup(
		new BMessage(MSG_BAR_THICKNESS_CHANGED), this);

	// default bar thickness
	button = new ThicknessRadioButton("Thin bar", 1.0);
	group->AddChild(button);
	fBarThicknessRadioGroup->AddButton(button->GetRadioButton());

	// thicker bar
	button = new ThicknessRadioButton("Normal bar", fSlider->BarThickness());
	group->AddChild(button);
	fBarThicknessRadioGroup->AddButton(button->GetRadioButton());

	// very thick bar
	button = new ThicknessRadioButton("Thick bar", 25.0);
	group->AddChild(button);
	fBarThicknessRadioGroup->AddButton(button->GetRadioButton());

	// default to default thickness
	fBarThicknessRadioGroup->SelectButton((int32)1);

	// spacing
	group->AddChild(new VStrut(10));

	// the radio button group for selecting the label

	fLabelRadioGroup = new RadioButtonGroup(new BMessage(MSG_LABEL_CHANGED),
		this);

	// no label
	button = new LabelRadioButton("No label", NULL);
	group->AddChild(button);
	fLabelRadioGroup->AddButton(button->GetRadioButton());

	// label string
	button = new LabelRadioButton("Label", "Label");
	group->AddChild(button);
	fLabelRadioGroup->AddButton(button->GetRadioButton());

	// long label string
	button = new LabelRadioButton("Long label",
		"Quite Long Label for a BSlider");
	group->AddChild(button);
	fLabelRadioGroup->AddButton(button->GetRadioButton());

	// default to normal label
	fLabelRadioGroup->SelectButton((int32)1);

	// spacing
	group->AddChild(new VStrut(10));

	// the radio button group for selecting the limit labels

	fLimitLabelsRadioGroup = new RadioButtonGroup(
		new BMessage(MSG_LIMIT_LABELS_CHANGED), this);

	// no limit labels
	button = new LimitLabelsRadioButton("No limit labels", NULL, NULL);
	group->AddChild(button);
	fLimitLabelsRadioGroup->AddButton(button->GetRadioButton());

	// normal limit label strings
	button = new LimitLabelsRadioButton("Normal limit labels", "Min", "Max");
	group->AddChild(button);
	fLimitLabelsRadioGroup->AddButton(button->GetRadioButton());

	// long limit label strings
	button = new LimitLabelsRadioButton("Long limit labels",
		"Very long min label", "Very long max label");
	group->AddChild(button);
	fLimitLabelsRadioGroup->AddButton(button->GetRadioButton());

	// default to no limit labels
	fLimitLabelsRadioGroup->SelectButton((int32)0);

	// spacing
	group->AddChild(new VStrut(10));

	// update text
	fUpdateTextCheckBox = new LabeledCheckBox("Update text",
		new BMessage(MSG_UPDATE_TEXT_CHANGED), this);
	group->AddChild(fUpdateTextCheckBox);


	// glue
	group->AddChild(new Glue());
}


// DectivateTest
void
SliderTest::DectivateTest()
{
}


// MessageReceived
void
SliderTest::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_ORIENTATION_CHANGED:
			_UpdateOrientation();
			break;
		case MSG_THUMB_STYLE_CHANGED:
			_UpdateThumbStyle();
			break;
		case MSG_HASH_MARKS_CHANGED:
			_UpdateHashMarkLocation();
			break;
		case MSG_BAR_THICKNESS_CHANGED:
			_UpdateBarThickness();
			break;
		case MSG_LABEL_CHANGED:
			_UpdateLabel();
			break;
		case MSG_LIMIT_LABELS_CHANGED:
			_UpdateLimitLabels();
			break;
		case MSG_UPDATE_TEXT_CHANGED:
			_UpdateUpdateText();
			break;
		default:
			Test::MessageReceived(message);
			break;
	}
}


// _UpdateOrientation
void
SliderTest::_UpdateOrientation()
{
	if (fOrientationRadioGroup) {
		// We need to get the parent of the actually selected button, since
		// that is the labeled radio button we've derived our
		// BorderStyleRadioButton from.
		AbstractButton* selectedButton
			= fOrientationRadioGroup->SelectedButton();
		View* parent = (selectedButton ? selectedButton->Parent() : NULL);
		OrientationRadioButton* button = dynamic_cast<OrientationRadioButton*>(
			parent);
		if (button)
			fSlider->SetOrientation(button->fOrientation);
	}
}

// _UpdateThumbStyle
void
SliderTest::_UpdateThumbStyle()
{
	if (fThumbStyleRadioGroup) {
		// We need to get the parent of the actually selected button, since
		// that is the labeled radio button we've derived our
		// BorderStyleRadioButton from.
		AbstractButton* selectedButton
			= fThumbStyleRadioGroup->SelectedButton();
		View* parent = (selectedButton ? selectedButton->Parent() : NULL);
		ThumbStyleRadioButton* button = dynamic_cast<ThumbStyleRadioButton*>(
			parent);
		if (button)
			fSlider->SetStyle(button->fStyle);
	}
}

// _UpdateHashMarkLocation
void
SliderTest::_UpdateHashMarkLocation()
{
	if (fHashMarkLocationRadioGroup) {
		// We need to get the parent of the actually selected button, since
		// that is the labeled radio button we've derived our
		// BorderStyleRadioButton from.
		AbstractButton* selectedButton
			= fHashMarkLocationRadioGroup->SelectedButton();
		View* parent = (selectedButton ? selectedButton->Parent() : NULL);
		HashMarkLocationRadioButton* button
			= dynamic_cast<HashMarkLocationRadioButton*>(parent);
		if (button)
			fSlider->SetHashMarks(button->fLocation);
	}
}

// _UpdateBarThickness
void
SliderTest::_UpdateBarThickness()
{
	if (fBarThicknessRadioGroup) {
		// We need to get the parent of the actually selected button, since
		// that is the labeled radio button we've derived our
		// BorderStyleRadioButton from.
		AbstractButton* selectedButton
			= fBarThicknessRadioGroup->SelectedButton();
		View* parent = (selectedButton ? selectedButton->Parent() : NULL);
		ThicknessRadioButton* button
			= dynamic_cast<ThicknessRadioButton*>(parent);
		if (button)
			fSlider->SetBarThickness(button->fThickness);
	}
}

// _UpdateLabel
void
SliderTest::_UpdateLabel()
{
	if (fLabelRadioGroup) {
		// We need to get the parent of the actually selected button, since
		// that is the labeled radio button we've derived our
		// BorderStyleRadioButton from.
		AbstractButton* selectedButton = fLabelRadioGroup->SelectedButton();
		View* parent = (selectedButton ? selectedButton->Parent() : NULL);
		LabelRadioButton* button = dynamic_cast<LabelRadioButton*>(parent);
		if (button)
			fSlider->SetLabel(button->fLabel);
	}
}

// _UpdateLimitLabels
void
SliderTest::_UpdateLimitLabels()
{
	if (fLimitLabelsRadioGroup) {
		// We need to get the parent of the actually selected button, since
		// that is the labeled radio button we've derived our
		// BorderStyleRadioButton from.
		AbstractButton* selectedButton
			= fLimitLabelsRadioGroup->SelectedButton();
		View* parent = (selectedButton ? selectedButton->Parent() : NULL);
		LimitLabelsRadioButton* button = dynamic_cast<LimitLabelsRadioButton*>(
			parent);
		if (button)
			fSlider->SetLimitLabels(button->fMinLabel, button->fMaxLabel);
	}
}

// _UpdateUpdateText
void
SliderTest::_UpdateUpdateText()
{
	if (!fUpdateTextCheckBox
		|| fUpdateTextCheckBox->IsSelected() == fSlider->fExportUpdateText)
		return;

	fSlider->fExportUpdateText = fUpdateTextCheckBox->IsSelected();
	fSlider->UpdateTextChanged();
}

