/*
 * Copyright 2008, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "ScrollBarTest.h"

#include <Message.h>
#include <ScrollBar.h>

#include "RadioButton.h"
#include "TestView.h"


// messages
enum {
	MSG_ORIENTATION_CHANGED			= 'orch'
};


// OrientationRadioButton
class ScrollBarTest::OrientationRadioButton : public LabeledRadioButton {
public:
	OrientationRadioButton(const char* label, enum orientation orientation)
		: LabeledRadioButton(label),
		  fOrientation(orientation)
	{
	}

	enum orientation fOrientation;
};


ScrollBarTest::ScrollBarTest()
	: Test("ScrollBar", NULL),
	  fScrollBar(new BScrollBar("scroll bar", NULL, 0, 100, B_HORIZONTAL)),
	  fOrientationRadioGroup(NULL)
{
	SetView(fScrollBar);
}


ScrollBarTest::~ScrollBarTest()
{
	delete fOrientationRadioGroup;
}


Test*
ScrollBarTest::CreateTest()
{
	return new ScrollBarTest;
}


void
ScrollBarTest::ActivateTest(View* controls)
{
	// the radio button group for selecting the orientation

	GroupView* vGroup = new GroupView(B_VERTICAL);
	vGroup->SetFrame(controls->Bounds());
	vGroup->SetSpacing(0, 4);
	controls->AddChild(vGroup);

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

	// glue
	vGroup->AddChild(new Glue());
}


void
ScrollBarTest::DectivateTest()
{
}


void
ScrollBarTest::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_ORIENTATION_CHANGED:
			_UpdateOrientation();
			break;
		default:
			Test::MessageReceived(message);
			break;
	}
}


void
ScrollBarTest::_UpdateOrientation()
{
	if (fOrientationRadioGroup == NULL)
		return;

	// We need to get the parent of the actually selected button, since
	// that is the labeled radio button we've derived our
	// BorderStyleRadioButton from.
	AbstractButton* selectedButton
		= fOrientationRadioGroup->SelectedButton();
	View* parent = (selectedButton ? selectedButton->Parent() : NULL);
	OrientationRadioButton* button = dynamic_cast<OrientationRadioButton*>(
		parent);
	if (button)
		fScrollBar->SetOrientation(button->fOrientation);
}





