/*
 * Copyright 2012-2013 Tri-Edge AI <triedgeai@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "ConfigView.h"

#include "Constants.h"
#include "Gravity.h"

#include <GroupLayoutBuilder.h>
#include <ListView.h>
#include <ScrollView.h>
#include <Slider.h>
#include <StringView.h>
#include <View.h>


ConfigView::ConfigView(Gravity* parent, BRect rect)
	:
	BView(rect, B_EMPTY_STRING, B_FOLLOW_ALL_SIDES, B_WILL_DRAW)
{
	fParent = parent;

	SetLayout(new BGroupLayout(B_HORIZONTAL));
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fTitleString = new BStringView(RECT_0, B_EMPTY_STRING,
		"OpenGL Gravity Effect", B_FOLLOW_LEFT);

	fAuthorString = new BStringView(RECT_0, B_EMPTY_STRING,
		"by Tri-Edge AI", B_FOLLOW_LEFT);

	fCountSlider = new BSlider(RECT_0, B_EMPTY_STRING, "Particle Count: ",
		new BMessage(MSG_COUNT), 0, 4, B_BLOCK_THUMB);

	fShadeString = new BStringView(RECT_0, B_EMPTY_STRING, "Shade: ",
		B_FOLLOW_LEFT);

	fShadeList = new BListView(RECT_0, B_EMPTY_STRING, B_SINGLE_SELECTION_LIST,
		B_FOLLOW_ALL);

	fShadeList->SetSelectionMessage(new BMessage(MSG_SHADE));

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

	fCountSlider->SetValue(parent->Config.ParticleCount);

	AddChild(BGroupLayoutBuilder(B_VERTICAL, B_USE_DEFAULT_SPACING)
			.Add(BGroupLayoutBuilder(B_VERTICAL, 0)
				.Add(fTitleString)
				.Add(fAuthorString)
			)
			.Add(fShadeString)
			.Add(fShadeScroll)
			.Add(fCountSlider)
			.SetInsets(B_USE_DEFAULT_SPACING,
				B_USE_DEFAULT_SPACING,
				B_USE_DEFAULT_SPACING,
				B_USE_DEFAULT_SPACING)
	);
}


void
ConfigView::AttachedToWindow()
{
	fShadeList->SetTarget(this);
	fCountSlider->SetTarget(this);
}


void
ConfigView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case MSG_COUNT:
			fParent->Config.ParticleCount = fCountSlider->Value();
			break;

		case MSG_SHADE:
			fParent->Config.ShadeID = fShadeList->CurrentSelection();
			break;

		default:
			BView::MessageReceived(msg);
	}
}
