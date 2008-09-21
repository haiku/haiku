/*
 * Copyright 2008, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "TextViewTest.h"

#include <TextView.h>

#include "CheckBox.h"
#include "GroupView.h"
#include "TestView.h"


// messages
enum {
	MSG_UPDATE_INSETS			= 'upin',
	MSG_UPDATE_TEXT				= 'uptx',
	MSG_UPDATE_FONT				= 'upfn',
};


static const char* kNormalText = "Some text to get something on the screen.\n"
	"There is even a line break to see that as well.";
static const char* kAlternativeText = "This is some different Text to "
	"check out what happens when the text changes.";


TextViewTest::TextViewTest()
	: Test("TextView", NULL),

	  fTextView(new BTextView("text view")),

//	  fTextView(new BTextView(BRect(0, 0, 49, 49), "name",
//	  	BRect(0, 0, 49, 49), B_FOLLOW_NONE, B_WILL_DRAW | B_PULSE_NEEDED
//	  		| B_SUPPORTS_LAYOUT)),


	  fUseInsetsCheckBox(NULL),
	  fTextCheckBox(NULL),
	  fFontCheckBox(NULL)
{
	SetView(fTextView);
	fTextView->SetText(kNormalText);
}


Test*
TextViewTest::CreateTest()
{
	return new TextViewTest;
}


void
TextViewTest::ActivateTest(View* controls)
{
	GroupView* group = new GroupView(B_VERTICAL);
	group->SetFrame(controls->Bounds());
	group->SetSpacing(0, 4);
	controls->AddChild(group);

	// insets
	fUseInsetsCheckBox = new LabeledCheckBox("Use text rect insets",
		new BMessage(MSG_UPDATE_INSETS), this);
	group->AddChild(fUseInsetsCheckBox);

	// text
	fTextCheckBox = new LabeledCheckBox("Use alternative text",
		new BMessage(MSG_UPDATE_TEXT), this);
	group->AddChild(fTextCheckBox);

	// font
	fFontCheckBox = new LabeledCheckBox("Use large font",
		new BMessage(MSG_UPDATE_FONT), this);
	group->AddChild(fFontCheckBox);

	// glue
	group->AddChild(new Glue());
}


void
TextViewTest::DectivateTest()
{
}


void
TextViewTest::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_UPDATE_INSETS:
			_UpdateInsets();
			break;
		case MSG_UPDATE_TEXT:
			_UpdateText();
			break;
		case MSG_UPDATE_FONT:
			_UpdateFont();
			break;
		default:
			Test::MessageReceived(message);
			break;
	}
}


// #pragma mark - private


void
TextViewTest::_UpdateInsets()
{
	if (fUseInsetsCheckBox == NULL)
		return;

	if (fUseInsetsCheckBox->IsSelected())
		fTextView->SetInsets(10, 10, 10, 10);
	else
		fTextView->SetInsets(0, 0, 0, 0);
}


void
TextViewTest::_UpdateText()
{
	if (fTextCheckBox == NULL)
		return;

	if (fTextCheckBox->IsSelected())
		fTextView->SetText(kAlternativeText);
	else
		fTextView->SetText(kNormalText);
}


void
TextViewTest::_UpdateFont()
{
	if (fFontCheckBox == NULL)
		return;

	BFont font(be_plain_font);
	if (fFontCheckBox->IsSelected()) {
		font.SetSize(ceilf(font.Size() * 1.5));
		fTextView->SetFontAndColor(&font);
	} else {
		fTextView->SetFontAndColor(&font);
	}
}



