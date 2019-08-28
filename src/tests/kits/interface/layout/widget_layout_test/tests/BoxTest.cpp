/*
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "BoxTest.h"

#include <stdio.h>

#include <Box.h>
#include <Button.h>
#include <Message.h>

#include "CheckBox.h"
#include "GroupView.h"
#include "RadioButton.h"
#include "TestView.h"


// messages
enum {
	MSG_BORDER_STYLE_CHANGED	= 'bstc',
	MSG_LABEL_CHANGED			= 'lbch',
	MSG_LONG_LABEL_CHANGED		= 'llch',
	MSG_CHILD_CHANGED			= 'chch'
};


// BorderStyleRadioButton
class BoxTest::BorderStyleRadioButton : public LabeledRadioButton {
public:
	BorderStyleRadioButton(const char* label, border_style style)
		: LabeledRadioButton(label),
		  fBorderStyle(style)
	{
	}

	border_style	fBorderStyle;
};


// LabelRadioButton
class BoxTest::LabelRadioButton : public LabeledRadioButton {
public:
	LabelRadioButton(const char* label, const char* boxLabel,
		bool labelView = false)
		: LabeledRadioButton(label),
		  fLabel(boxLabel),
		  fLabelView(labelView)
	{
	}

	const char*	fLabel;
	bool		fLabelView;
};


// constructor
BoxTest::BoxTest()
	: Test("Box", NULL),
	  fBox(new BBox("test box")),
	  fChild(NULL),
	  fBorderStyleRadioGroup(NULL),
	  fLabelRadioGroup(NULL),
	  fLongLabelCheckBox(NULL),
	  fChildCheckBox(NULL)
{
	SetView(fBox);
}


// destructor
BoxTest::~BoxTest()
{
	delete fBorderStyleRadioGroup;
	delete fLabelRadioGroup;
}


// CreateTest
Test*
BoxTest::CreateTest()
{
	return new BoxTest;
}


// ActivateTest
void
BoxTest::ActivateTest(View* controls)
{
	// BBox sets its background color to that of its parent in
	// AttachedToWindow(). Override.
	rgb_color background = ui_color(B_PANEL_BACKGROUND_COLOR);
	fBox->SetViewColor(background);
	fBox->SetLowColor(background);

	GroupView* group = new GroupView(B_VERTICAL);
	group->SetFrame(controls->Bounds());
	group->SetSpacing(0, 8);
	controls->AddChild(group);

	// the radio button group for selecting the border style

	fBorderStyleRadioGroup = new RadioButtonGroup(
		new BMessage(MSG_BORDER_STYLE_CHANGED), this);

	// no border
	LabeledRadioButton* button = new BorderStyleRadioButton("no border",
		B_NO_BORDER);
	group->AddChild(button);
	fBorderStyleRadioGroup->AddButton(button->GetRadioButton());

	// plain border
	button = new BorderStyleRadioButton("plain border", B_PLAIN_BORDER);
	group->AddChild(button);
	fBorderStyleRadioGroup->AddButton(button->GetRadioButton());

	// fancy border
	button = new BorderStyleRadioButton("fancy border", B_FANCY_BORDER);
	group->AddChild(button);
	fBorderStyleRadioGroup->AddButton(button->GetRadioButton());

	// default to no border
	fBorderStyleRadioGroup->SelectButton((int32)0);

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
	button = new LabelRadioButton("Label string", "");
	group->AddChild(button);
	fLabelRadioGroup->AddButton(button->GetRadioButton());

	// label view
	button = new LabelRadioButton("Label view", NULL, true);
	group->AddChild(button);
	fLabelRadioGroup->AddButton(button->GetRadioButton());

	// default to no border
	fLabelRadioGroup->SelectButton((int32)0);

	// spacing
	group->AddChild(new VStrut(10));

	// long label
	fLongLabelCheckBox = new LabeledCheckBox("Long label",
		new BMessage(MSG_LONG_LABEL_CHANGED), this);
	group->AddChild(fLongLabelCheckBox);

	// child
	fChildCheckBox = new LabeledCheckBox("Child",
		new BMessage(MSG_CHILD_CHANGED), this);
	group->AddChild(fChildCheckBox);


	// glue
	group->AddChild(new Glue());
}


// DectivateTest
void
BoxTest::DectivateTest()
{
}


// MessageReceived
void
BoxTest::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_BORDER_STYLE_CHANGED:
			_UpdateBorderStyle();
			break;
		case MSG_LABEL_CHANGED:
			_UpdateLabel();
			break;
		case MSG_LONG_LABEL_CHANGED:
			_UpdateLongLabel();
			break;
		case MSG_CHILD_CHANGED:
			_UpdateChild();
			break;
		default:
			Test::MessageReceived(message);
			break;
	}
}


// _UpdateBorderStyle
void
BoxTest::_UpdateBorderStyle()
{
	if (fBorderStyleRadioGroup) {
		// We need to get the parent of the actually selected button, since
		// that is the labeled radio button we've derived our
		// BorderStyleRadioButton from.
		AbstractButton* selectedButton
			= fBorderStyleRadioGroup->SelectedButton();
		View* parent = (selectedButton ? selectedButton->Parent() : NULL);
		BorderStyleRadioButton* button = dynamic_cast<BorderStyleRadioButton*>(
			parent);
		if (button)
			fBox->SetBorder(button->fBorderStyle);
	}
}


// _UpdateLabel
void
BoxTest::_UpdateLabel()
{
	if (fLabelRadioGroup) {
		// We need to get the parent of the actually selected button, since
		// that is the labeled radio button we've derived our
		// BorderStyleRadioButton from.
		AbstractButton* selectedButton = fLabelRadioGroup->SelectedButton();
		View* parent = (selectedButton ? selectedButton->Parent() : NULL);
		LabelRadioButton* button = dynamic_cast<LabelRadioButton*>(parent);
		if (button) {
			if (button->fLabelView)
				fBox->SetLabel(new BButton("", NULL));
			else
				fBox->SetLabel(button->fLabel);

			_UpdateLongLabel();
		}
	}
}


// _UpdateLongLabel
void
BoxTest::_UpdateLongLabel()
{
	if (!fLongLabelCheckBox)
		return;

	const char* label = (fLongLabelCheckBox->IsSelected()
		? "Quite Long Label for a BBox"
		: "Label");

	if (BView* labelView = fBox->LabelView()) {
		if (BButton* button = dynamic_cast<BButton*>(labelView))
			button->SetLabel(label);
	} else if (fBox->Label())
		fBox->SetLabel(label);
}


// _UpdateChild
void
BoxTest::_UpdateChild()
{
	if (!fChildCheckBox || fChildCheckBox->IsSelected() == (fChild != NULL))
		return;

	if (fChild) {
		fBox->RemoveChild(fChild);
		fChild = NULL;
	} else {
		fChild = new TestView(BSize(20, 10), BSize(350, 200), BSize(100, 70));
		fBox->AddChild(fChild);
	}
}
