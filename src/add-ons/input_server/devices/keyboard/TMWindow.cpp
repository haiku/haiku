// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2004-2005, Haiku
//
//  This software is part of the Haiku distribution and is covered 
//  by the MIT license.
//
//
//  File:        TMWindow.cpp
//  Author:      Jérôme Duval
//  Description: Keyboard input server addon
//  Created :    October 13, 2004
// 
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~


#include "TMWindow.h"
#include "TMListItem.h"
#include "KeyboardInputDevice.h"

#include <Message.h>
#include <ScrollView.h>
#include <Screen.h>
#include <String.h>


const uint32 TM_CANCEL = 'TMca';
const uint32 TM_FORCE_REBOOT = 'TMfr';
const uint32 TM_KILL_APPLICATION = 'TMka';
const uint32 TM_SELECTED_TEAM = 'TMst';

#ifndef __HAIKU__
extern "C" void _kshutdown_(bool reboot);
#else
#	include <syscalls.h>
#	define _kshutdown_(x) _kern_shutdown(x)
#endif


TMWindow::TMWindow()
	: BWindow(BRect(0,0,350,300), "Team Monitor", 
		B_TITLED_WINDOW_LOOK, B_MODAL_ALL_WINDOW_FEEL, 
		B_NOT_MINIMIZABLE | B_NOT_ZOOMABLE | B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS,
		B_ALL_WORKSPACES),
	fQuitting(false)
{
	Lock();

	// ToDo: make this font sensitive

	fView = new TMView(Bounds(), "background", B_FOLLOW_LEFT | B_FOLLOW_TOP,
		B_WILL_DRAW, B_NO_BORDER);
	AddChild(fView);

	BRect screenFrame = BScreen(B_MAIN_SCREEN_ID).Frame();
	BPoint point;
	point.x = (screenFrame.Width() - Bounds().Width()) / 2;
	point.y = (screenFrame.Height() - Bounds().Height()) / 2;

	if (screenFrame.Contains(point))
		MoveTo(point);

	Unlock();
}


TMWindow::~TMWindow()
{
}


void
TMWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case SYSTEM_SHUTTING_DOWN:
			fQuitting = true;
			break;
		default:
			BWindow::MessageReceived(msg);
			break;
	}
}


bool
TMWindow::QuitRequested()
{
	Disable();
	return fQuitting;
}


void
TMWindow::Enable()
{
	SetPulseRate(1000000);

	if (IsHidden()) {
		fView->UpdateList();
		Show();
	}
}


void
TMWindow::Disable()
{
	fView->ListView()->DeselectAll();
	SetPulseRate(0);
	Hide();
}


//	#pragma mark -


TMView::TMView(BRect bounds, const char* name, uint32 resizeFlags,
	uint32 flags, border_style border)
	: BBox(bounds, name, resizeFlags, flags | B_PULSE_NEEDED, border)
{
	BRect rect = bounds;
	rect.InsetBy(12, 12);
	rect.right -= B_V_SCROLL_BAR_WIDTH;
	rect.bottom = rect.top + 146;

	BFont font = be_plain_font;

	fListView = new BListView(rect, "teams", B_SINGLE_SELECTION_LIST, 
		B_FOLLOW_ALL);
	fListView->SetSelectionMessage(new BMessage(TM_SELECTED_TEAM));

	BScrollView *scrollView = new BScrollView("scroll_teams", fListView, 
		B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP_BOTTOM, 0, false, true, B_FANCY_BORDER);
	AddChild(scrollView);

	rect = bounds;
	rect.right -= 10;
	rect.left = rect.right - font.StringWidth("Cancel") - 20;
	rect.bottom -= 14;
	rect.top = rect.bottom - 20;

	BButton *cancel = new BButton(rect, "cancel", "Cancel", 
		new BMessage(TM_CANCEL), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	AddChild(cancel);

	rect.left = 10;
	rect.right = rect.left + font.StringWidth("Force Reboot") + 20;

	BButton *forceReboot = new BButton(rect, "force", "Force Reboot", 
		new BMessage(TM_FORCE_REBOOT), B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	AddChild(forceReboot);

	rect.top -= 97;
	rect.bottom = rect.top + 20;
	rect.right = rect.left + font.StringWidth("Kill Application") + 20;

	fKillApp = new BButton(rect, "kill", "Kill Application", 
		new BMessage(TM_KILL_APPLICATION), B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	AddChild(fKillApp);
	fKillApp->SetEnabled(false);

	rect.top = rect.bottom + 10;
	rect.bottom = rect.top + 65;
	rect.right = bounds.right - 10;
	fDescView = new TMDescView(rect);
	AddChild(fDescView);
}


void
TMView::AttachedToWindow()
{
	if (BButton *cancel = dynamic_cast<BButton*>(FindView("cancel"))) {
		Window()->SetDefaultButton(cancel);
		cancel->SetTarget(this);
	}

	if (BButton *kill = dynamic_cast<BButton*>(FindView("kill")))
		kill->SetTarget(this);

	if (BButton *reboot = dynamic_cast<BButton*>(FindView("force")))
		reboot->SetTarget(this);

	fListView->SetTarget(this);
}


void
TMView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case TM_FORCE_REBOOT:
			_kshutdown_(true);
			break;
		case TM_KILL_APPLICATION: {
			TMListItem *item = (TMListItem*)ListView()->ItemAt(
				ListView()->CurrentSelection());
			kill_team(item->GetInfo()->team);
			fKillApp->SetEnabled(false);
			UpdateList();
			break;
		}
		case TM_SELECTED_TEAM: {
			fKillApp->SetEnabled(fListView->CurrentSelection() >= 0);
			TMListItem *item = (TMListItem*)ListView()->ItemAt(
				ListView()->CurrentSelection());
			fDescView->SetItem(item);
			break;
		}
		case TM_CANCEL:
			Window()->PostMessage(B_QUIT_REQUESTED);
			break;

		default:
			BBox::MessageReceived(msg);
			break;
	}
}


void
TMView::Pulse()
{
	UpdateList();
}


void
TMView::UpdateList()
{
	CALLED();
	bool changed = false;

	for (int32 i = 0; i < fListView->CountItems(); i++) {
		TMListItem *item = (TMListItem*)fListView->ItemAt(i);
		item->fFound = false;
	}

	int32 cookie = 0;
	team_info info;
	while (get_next_team_info(&cookie, &info) == B_OK) {
		if (info.team <=16)
			continue;

		bool found = false;
		for (int32 i = 0; i < fListView->CountItems(); i++) {
			TMListItem *item = (TMListItem*)fListView->ItemAt(i);
			if (item->GetInfo()->team == info.team) {
				item->fFound = true;
				found = true;
			}
		}

		if (!found) {
			TMListItem *item = new TMListItem(info);
			fListView->AddItem(item, 0);
			item->fFound = true;
			changed = true;
		}
	}	

	for (int32 i = fListView->CountItems() - 1; i >= 0; i--) {
		TMListItem *item = (TMListItem*)fListView->ItemAt(i);
		if (!item->fFound) {
			if (item == fDescView->Item())
				fDescView->SetItem(NULL);

			delete fListView->RemoveItem(i);
			changed = true;
		}
	}

	if (changed)
		fListView->Invalidate();
}


//	#pragma mark -


TMDescView::TMDescView(BRect rect)
	: BBox(rect, "descview", B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW, B_NO_BORDER),
	fItem(NULL)
{
}


void
TMDescView::Draw(BRect rect)
{
	rect = Bounds();

	if (fItem) {
		BRect frame(rect);
		frame.OffsetBy(2,3);
		frame.Set(frame.left, frame.top, frame.left+31, frame.top+31);
		SetDrawingMode(B_OP_OVER);
		DrawBitmap(fItem->LargeIcon(), frame);
		SetDrawingMode(B_OP_COPY);

		BFont font = be_plain_font;
		font_height	finfo;
		font.GetHeight(&finfo);
		SetFont(&font);
		MovePenTo(frame.right+9, frame.top - 2 + ((frame.Height() - (finfo.ascent + finfo.descent + finfo.leading)) / 4) +
			(finfo.ascent + finfo.descent) - 1);
		BString path = fItem->Path()->Path();
		TruncateString(&path, B_TRUNCATE_MIDDLE, rect.right - 9 - frame.right);
		DrawString(path.String());

		if (fItem->IsSystemServer()) {
			MovePenTo(frame.right+9, frame.top + 1 + ((frame.Height() - (finfo.ascent + finfo.descent + finfo.leading)) *3 / 4) +
				(finfo.ascent + finfo.descent) - 1);
			DrawString("(This team is a system component)");
		}
	} else {
		BFont font = be_plain_font;
		font_height	finfo;
		font.GetHeight(&finfo);
		SetFont(&font);
		BPoint point(rect.left+4, rect.top - 9 + ((rect.Height() - (finfo.ascent + finfo.descent + finfo.leading)) / 4) +
			(finfo.ascent + finfo.descent) - 1);
		MovePenTo(point);
		DrawString("Select an application from the list above and click the");

		point.y += 13;
		MovePenTo(point);
		DrawString("\"Kill Application\" button in order to close it.");

		point.y += 26;
		MovePenTo(point);
		DrawString("Hold CONTROL+ALT+DELETE for 4 seconds to reboot.");
	}
}


void
TMDescView::SetItem(TMListItem *item)
{
	fItem = item;
	Invalidate();
}
