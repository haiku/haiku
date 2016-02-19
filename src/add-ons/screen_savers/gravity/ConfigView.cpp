/*
 * Copyright 2012-2013 Tri-Edge AI <triedgeai@gmail.com>
 * Copyright 2014 Haiku, Inc. All rights reserved.
 *
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Tri-Edge AI
 *		John Scipione, jscipione@gmail.com
 */


#include "ConfigView.h"

#include <LayoutBuilder.h>
#include <ListView.h>
#include <ScrollView.h>
#include <Slider.h>
#include <StringView.h>
#include <View.h>

#include "Gravity.h"


static const int32 kMsgSize = 'size';
static const int32 kMsgShade = 'shad';


ConfigView::ConfigView(BRect frame, Gravity* parent)
	:
	BView(frame, B_EMPTY_STRING, B_FOLLOW_ALL_SIDES, B_WILL_DRAW),
	fParent(parent),
	fTitleString(new BStringView(B_EMPTY_STRING, "OpenGL Gravity Effect")),
	fAuthorString(new BStringView(B_EMPTY_STRING, "by Tri-Edge AI")),
	fCountSlider(new BSlider(B_EMPTY_STRING, "Particle Count: ",
		new BMessage(kMsgSize), 0, 4, B_HORIZONTAL, B_BLOCK_THUMB,
		B_NAVIGABLE | B_WILL_DRAW)),
	fShadeString(new BStringView(B_EMPTY_STRING, "Shade: ")),
	fShadeList(new BListView(B_EMPTY_STRING, B_SINGLE_SELECTION_LIST,
		B_WILL_DRAW | B_NAVIGABLE))
{
	SetLayout(new BGroupLayout(B_HORIZONTAL));
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	fShadeList->SetSelectionMessage(new BMessage(kMsgShade));

	fShadeList->AddItem(new BStringItem("Red"));
	fShadeList->AddItem(new BStringItem("Green"));
	fShadeList->AddItem(new BStringItem("Blue"));
	fShadeList->AddItem(new BStringItem("Orange"));
	fShadeList->AddItem(new BStringItem("Purple"));
	fShadeList->AddItem(new BStringItem("White"));
	fShadeList->AddItem(new BStringItem("Rainbow"));

	fShadeList->Select(parent->Config.ShadeID);

	fShadeScroll = new BScrollView(B_EMPTY_STRING, fShadeList,
		B_WILL_DRAW | B_FRAME_EVENTS, false, true);

	fCountSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fCountSlider->SetHashMarkCount(5);
	fCountSlider->SetLimitLabels("128", "2048");
	fCountSlider->SetModificationMessage(new BMessage(kMsgSize));
	fCountSlider->SetValue(parent->Config.ParticleCount);

	AddChild(BLayoutBuilder::Group<>(B_VERTICAL, B_USE_DEFAULT_SPACING)
		.AddGroup(B_VERTICAL, 0)
			.Add(fTitleString)
			.Add(fAuthorString)
			.End()
		.Add(fShadeString)
		.Add(fShadeScroll)
		.Add(fCountSlider)
		.SetInsets(B_USE_DEFAULT_SPACING));
}


void
ConfigView::AttachedToWindow()
{
	fShadeList->SetTarget(this);
	fCountSlider->SetTarget(this);
}


void
ConfigView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgSize:
			fParent->Config.ParticleCount = fCountSlider->Value();
			break;

		case kMsgShade:
			fParent->Config.ShadeID = fShadeList->CurrentSelection();
			break;

		default:
			BView::MessageReceived(message);
	}
}
