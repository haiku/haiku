/*
 * Copyright 2012-2013 Tri-Edge AI <triedgeai@gmail.com>
 * Copyright 2014-2016 Haiku, Inc. All rights reserved.
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

#include "ColorItem.h"
#include "Gravity.h"
#include "RainbowItem.h"


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
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	fShadeList->SetSelectionMessage(new BMessage(kMsgShade));
	fShadeList->AddItem(new ColorItem("Red", (rgb_color){ 255, 65, 54 }));
	fShadeList->AddItem(new ColorItem("Green", (rgb_color){ 46, 204, 64 }));
	fShadeList->AddItem(new ColorItem("Blue", (rgb_color){ 0, 116, 217 }));
	fShadeList->AddItem(new ColorItem("Orange", (rgb_color){ 255, 133, 27 }));
	fShadeList->AddItem(new ColorItem("Purple", (rgb_color){ 177, 13, 201 }));
	fShadeList->AddItem(new ColorItem("White", (rgb_color){ 255, 255, 255 }));
	fShadeList->AddItem(new RainbowItem("Rainbow"));

	fShadeScroll = new BScrollView(B_EMPTY_STRING, fShadeList,
		B_WILL_DRAW | B_FRAME_EVENTS, false, true);

	fCountSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fCountSlider->SetHashMarkCount(5);
	fCountSlider->SetLimitLabels("128", "2048");
	fCountSlider->SetModificationMessage(new BMessage(kMsgSize));
	fCountSlider->SetValue(parent->Config.ParticleCount);

	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.AddGroup(B_VERTICAL, 0)
			.Add(fTitleString)
			.Add(fAuthorString)
			.End()
		.Add(fShadeString)
		.Add(fShadeScroll)
		.Add(fCountSlider)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.End();
}


void
ConfigView::AllAttached()
{
	// fixup scroll bar range
	fShadeScroll->ScrollBar(B_VERTICAL)->SetRange(0.0f,
		fShadeList->ItemFrame(fShadeList->CountItems()).bottom
			- fShadeList->ItemFrame((int32)0).top);

	// select the shade
	fShadeList->Select(fParent->Config.ShadeID);
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
