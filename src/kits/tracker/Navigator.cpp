/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2000, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/


#include "Bitmaps.h"
#include "Commands.h"
#include "ContainerWindow.h"
#include "FSUtils.h"
#include "Model.h"
#include "Navigator.h"
#include "Tracker.h"

#include <Picture.h>
#include <TextControl.h>
#include <Window.h>


namespace BPrivate {

static const int32 kMaxHistory = 32;

}

// BPictureButton() will crash when giving zero pointers,
// although we really want and have to set up the
// pictures when we can, e.g. on a AttachedToWindow.
static BPicture sPicture;


BNavigatorButton::BNavigatorButton(BRect rect, const char* name, BMessage* message,
	int32 resIDon, int32 resIDoff, int32 resIDdisabled)
	:	BPictureButton(rect, name, &sPicture, &sPicture, message),
		fResIDOn(resIDon),
		fResIDOff(resIDoff),
		fResIDDisabled(resIDdisabled)
{
	// Clear to background color to avoid ugly border on click
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


BNavigatorButton::~BNavigatorButton()
{
}


void
BNavigatorButton::AttachedToWindow()
{
	BBitmap* bmpOn = 0;
	GetTrackerResources()->GetBitmapResource(B_MESSAGE_TYPE, fResIDOn, &bmpOn);
	SetPicture(bmpOn, true, true);
	delete bmpOn;
	
	BBitmap* bmpOff = 0;
	GetTrackerResources()->GetBitmapResource(B_MESSAGE_TYPE, fResIDOff, &bmpOff);
	SetPicture(bmpOff, true, false);
	delete bmpOff;

	BBitmap* bmpDisabled = 0;
	GetTrackerResources()->GetBitmapResource(B_MESSAGE_TYPE, fResIDDisabled, &bmpDisabled);
	SetPicture(bmpDisabled, false, false);
	SetPicture(bmpDisabled, false, true);
	delete bmpDisabled;
}


void
BNavigatorButton::SetPicture(BBitmap* bitmap, bool enabled, bool on)
{
	if (bitmap) {
		BPicture picture;
		BView view(bitmap->Bounds(), "", 0, 0);
		AddChild(&view);
		view.BeginPicture(&picture);
		view.SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		view.FillRect(view.Bounds());
		view.SetDrawingMode(B_OP_OVER);
		view.DrawBitmap(bitmap, BPoint(0, 0));
		view.EndPicture();
		RemoveChild(&view);
		if (enabled)
			if (on)
				SetEnabledOn(&picture);
			else
				SetEnabledOff(&picture);
		else
			if (on)
				SetDisabledOn(&picture);
			else
				SetDisabledOff(&picture);
	}
}


BNavigator::BNavigator(const Model* model, BRect rect, uint32 resizeMask)
	:	BView(rect, "Navigator", resizeMask, B_WILL_DRAW),
	fBack(0),
	fForw(0),
	fUp(0),
	fBackHistory(8, true),
	fForwHistory(8, true)
{
	// Get initial path
	model->GetPath(&fPath);
	
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	float top = 2 + (be_plain_font->Size() - 8) / 2;

	// Set up widgets
	fBack = new BNavigatorButton(BRect(3, top, 21, top + 17), "Back",
		new BMessage(kNavigatorCommandBackward), R_ResBackNavActiveSel,
		R_ResBackNavActive, R_ResBackNavInactive);
	fBack->SetEnabled(false);
	AddChild(fBack);

	fForw = new BNavigatorButton(BRect(35, top, 53, top + 17), "Forw",
		new BMessage(kNavigatorCommandForward), R_ResForwNavActiveSel,
		R_ResForwNavActive, R_ResForwNavInactive);
	fForw->SetEnabled(false);
	AddChild(fForw);

	fUp = new BNavigatorButton(BRect(67, top, 84, top + 17), "Up",
		new BMessage(kNavigatorCommandUp), R_ResUpNavActiveSel,
		R_ResUpNavActive, R_ResUpNavInactive);
	fUp->SetEnabled(false);
	AddChild(fUp);

	fLocation = new BTextControl(BRect(97, 2, rect.Width() - 2, 21),
		"Location", "", "", new BMessage(kNavigatorCommandLocation),
		B_FOLLOW_LEFT_RIGHT);
	fLocation->SetDivider(0);
	AddChild(fLocation);
}


BNavigator::~BNavigator()
{
}


void
BNavigator::AttachedToWindow()
{
	// Inital setup of widget states
	UpdateLocation(0, kActionSet);

	// All messages should arrive here
	fBack->SetTarget(this);
	fForw->SetTarget(this);
	fUp->SetTarget(this);
	fLocation->SetTarget(this);
}


void
BNavigator::Draw(BRect)
{
	rgb_color bgColor = ui_color(B_PANEL_BACKGROUND_COLOR);
	rgb_color shineColor = ui_color(B_SHINE_COLOR);
	rgb_color halfDarkColor = tint_color(bgColor, B_DARKEN_1_TINT);
	rgb_color darkColor = tint_color(bgColor, B_DARKEN_2_TINT);
	// Draws a beveled smooth border
	BeginLineArray(4);
	AddLine(Bounds().LeftTop(), Bounds().RightTop(), shineColor);
	AddLine(Bounds().LeftTop(), Bounds().LeftBottom() - BPoint(0, 1), shineColor);
	AddLine(Bounds().LeftBottom() - BPoint(-1, 1), Bounds().RightBottom() - BPoint(0, 1), halfDarkColor);
	AddLine(Bounds().LeftBottom(), Bounds().RightBottom(), darkColor);
	EndLineArray();
}


void
BNavigator::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kNavigatorCommandBackward:
			GoBackward((modifiers() & B_OPTION_KEY) == B_OPTION_KEY);
			break;

		case kNavigatorCommandForward:
			GoForward((modifiers() & B_OPTION_KEY) == B_OPTION_KEY);
			break;

		case kNavigatorCommandUp:
			GoUp((modifiers() & B_OPTION_KEY) == B_OPTION_KEY);
			break;

		case kNavigatorCommandLocation:
			GoTo();
			break;

		default:
		{
			// Catch any dropped refs and try to switch to this new directory
			entry_ref ref;
			if (message->FindRef("refs", &ref) == B_OK) {
				BMessage message(kSwitchDirectory);
				BEntry entry(&ref, true);
				if (!entry.IsDirectory()) {
					entry.GetRef(&ref);
					BPath path(&ref);
					path.GetParent(&path);
					get_ref_for_path(path.Path(), &ref);
				}
				message.AddRef("refs", &ref);
				message.AddInt32("action", kActionSet);
				Window()->PostMessage(&message);
			}
		}
	}
}


void
BNavigator::GoBackward(bool option)
{
	int32 itemCount = fBackHistory.CountItems();
	if (itemCount >= 2 && fBackHistory.ItemAt(itemCount - 2)) {
		BEntry entry;
		if (entry.SetTo(fBackHistory.ItemAt(itemCount - 2)->Path()) == B_OK)
			SendNavigationMessage(kActionBackward, &entry, option);
	}
}


void
BNavigator::GoForward(bool option)
{
	if (fForwHistory.CountItems() >= 1) {
		BEntry entry;
		if (entry.SetTo(fForwHistory.LastItem()->Path()) == B_OK)
			SendNavigationMessage(kActionForward, &entry, option);
	}
}


void
BNavigator::GoUp(bool option)
{
	BEntry entry;
	if (entry.SetTo(fPath.Path()) == B_OK) {
		BEntry parentEntry;
		if (entry.GetParent(&parentEntry) == B_OK && !FSIsDeskDir(&parentEntry))
			SendNavigationMessage(kActionUp, &parentEntry, option);
	}
}


void
BNavigator::SendNavigationMessage(NavigationAction action, BEntry* entry, bool option)
{
	entry_ref ref;

	if (entry->GetRef(&ref) == B_OK) {
		BMessage message;
		message.AddRef("refs", &ref);
		message.AddInt32("action", action);
		
		// get the node of this folder for selecting it in the new location
		const node_ref* nodeRef;
		if (Window() && Window()->TargetModel())
			nodeRef = Window()->TargetModel()->NodeRef();
		else
			nodeRef = NULL;
		
		// if the option key was held down, open in new window (send message to be_app)
		// otherwise send message to this window. TTracker (be_app) understands nodeRefToSlection,
		// BContainerWindow doesn't, so we have to select the item manually
		if (option) {
			message.what = B_REFS_RECEIVED;
			if (nodeRef)
				message.AddData("nodeRefToSelect", B_RAW_TYPE, nodeRef, sizeof(node_ref));
			be_app->PostMessage(&message);
		} else {
			message.what = kSwitchDirectory;
			Window()->PostMessage(&message);
			UnlockLooper();
				// This is to prevent a dead-lock situation. SelectChildInParentSoon()
				// eventually locks the TaskLoop::fLock. Later, when StandAloneTaskLoop::Run()
				// runs, it also locks TaskLoop::fLock and subsequently locks this window's looper.
				// Therefore we can't call SelectChildInParentSoon with our Looper locked,
				// because we would get different orders of locking (thus the risk of dead-locking).
				//
				// Todo: Change the locking behaviour of StandAloneTaskLoop::Run() and sub-
				// sequently called functions.
			if (nodeRef)
				dynamic_cast<TTracker*>(be_app)->SelectChildInParentSoon(&ref, nodeRef);
			LockLooper();
		}
	}
}


void
BNavigator::GoTo()
{
	BString pathname = fLocation->Text();

	if (pathname.Compare("") == 0)
		pathname = "/";

	BEntry entry;
	entry_ref ref;

	if (entry.SetTo(pathname.String()) == B_OK
		&& !FSIsDeskDir(&entry)
		&& entry.GetRef(&ref) == B_OK) {
		BMessage message(kSwitchDirectory);
		message.AddRef("refs", &ref);
		message.AddInt32("action", kActionLocation);
		Window()->PostMessage(&message);
	} else {
		BPath path;
		
		if (Window() && Window()->TargetModel()) {
			Window()->TargetModel()->GetPath(&path);
			fLocation->SetText(path.Path());
		}
	}
}


void
BNavigator::UpdateLocation(const Model* newmodel, int32 action)
{
	if (newmodel)
		newmodel->GetPath(&fPath);

	// Modify history according to commands
	switch (action) {
		case kActionBackward:
			fForwHistory.AddItem(fBackHistory.RemoveItemAt(fBackHistory.CountItems()-1));
			break;

		case kActionForward:
			fBackHistory.AddItem(fForwHistory.RemoveItemAt(fForwHistory.CountItems()-1));
			break;

		case kActionUpdatePath:
			break;

		default:
			fForwHistory.MakeEmpty();
			fBackHistory.AddItem(new BPath(fPath));

			while (fBackHistory.CountItems() > kMaxHistory)
				fBackHistory.RemoveItem(fBackHistory.FirstItem(), true);
			break;
	}

	// Enable Up button when there is any parent
	BEntry entry;
	if (entry.SetTo(fPath.Path()) == B_OK) {
		BEntry parentEntry;
		fUp->SetEnabled(entry.GetParent(&parentEntry) == B_OK && !FSIsDeskDir(&parentEntry));
	}

	// Enable history buttons if history contains something
	fForw->SetEnabled(fForwHistory.CountItems() > 0);
	fBack->SetEnabled(fBackHistory.CountItems() > 1);

	// Avoid loss of selection and cursor position
	if (action != kActionLocation)
		fLocation->SetText(fPath.Path());
}


float
BNavigator::CalcNavigatorHeight(void)
{
	// Empiric formula from how much space the textview
	// will take once it is attached (using be_plain_font):
	return  ceilf(11.0f + be_plain_font->Size()*(1.0f + 7.0f / 30.0f));
}
