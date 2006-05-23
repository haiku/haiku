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

#include "tracker_private.h"

#include <Message.h>
#include <Roster.h>
#include <ScrollView.h>
#include <Screen.h>
#include <String.h>


const uint32 TM_CANCEL = 'TMca';
const uint32 TM_FORCE_REBOOT = 'TMfr';
const uint32 TM_KILL_APPLICATION = 'TMka';
const uint32 TM_RESTART_DESKTOP = 'TMrd';
const uint32 TM_SELECTED_TEAM = 'TMst';

#ifndef __HAIKU__
extern "C" void _kshutdown_(bool reboot);
#else
#	include <syscalls.h>
#	define _kshutdown_(x) _kern_shutdown(x)
#endif


TMWindow::TMWindow()
	: BWindow(BRect(0, 0, 350, 300), "Team Monitor", 
		B_TITLED_WINDOW_LOOK, B_MODAL_ALL_WINDOW_FEEL, 
		B_NOT_MINIMIZABLE | B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS,
		B_ALL_WORKSPACES),
	fQuitting(false)
{
	if (Lock()) {

		// ToDo: make this font sensitive
	
		fView = new TMView(Bounds(), "background", B_FOLLOW_ALL,
			B_WILL_DRAW, B_NO_BORDER);
		AddChild(fView);
	
		float width, height;
		fView->GetPreferredSize(&width, &height);
		ResizeTo(width, height);
	
		BRect screenFrame = BScreen(this).Frame();
		BPoint point;
		point.x = (screenFrame.Width() - Bounds().Width()) / 2;
		point.y = (screenFrame.Height() - Bounds().Height()) / 2;
	
		if (screenFrame.Contains(point))
			MoveTo(point);
		SetSizeLimits(Bounds().Width(), Bounds().Width()*2, Bounds().Height(), Bounds().Height()*2);
		Unlock();
	}
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
	if (Lock()) {
		SetPulseRate(1000000);
	
		if (IsHidden()) {
			fView->UpdateList();
			Show();
		}
		Unlock();
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
	BFont font = be_plain_font;
	BRect rect = bounds;
	rect.right -= 10;
	rect.left = rect.right - font.StringWidth("Cancel") - 40;
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

	BRect restartRect = rect;
	restartRect.left = rect.right + 10;
	restartRect.right = restartRect.left + font.StringWidth("Restart the Desktop") + 20;
	fRestartButton = new BButton(restartRect, "restart", "Restart the Desktop",
		new BMessage(TM_RESTART_DESKTOP), B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	AddChild(fRestartButton);
	fRestartButton->Hide();

	rect.left += 4;
	rect.bottom = rect.top - 8;
	rect.top = rect.bottom - 30;
	rect.right = bounds.right - 10;
	fDescView = new TMDescView(rect, B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM);
	AddChild(fDescView);
	fDescView->ResizeToPreferred();
	
	rect = fDescView->Frame();
	rect.left = 10;
	rect.right = rect.left + font.StringWidth("Kill Application") + 20;
	rect.bottom = rect.top - 8;
	rect.top = rect.bottom - 20;

	fKillButton = new BButton(rect, "kill", "Kill Application",
		new BMessage(TM_KILL_APPLICATION), B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	AddChild(fKillButton);
	fKillButton->SetEnabled(false);

	rect = bounds;
	rect.InsetBy(12, 12);
	rect.right -= B_V_SCROLL_BAR_WIDTH;
	rect.bottom = fKillButton->Frame().top - 10;

	fListView = new BListView(rect, "teams", B_SINGLE_SELECTION_LIST, 
		B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP_BOTTOM);
	fListView->SetSelectionMessage(new BMessage(TM_SELECTED_TEAM));

	BScrollView *scrollView = new BScrollView("scroll_teams", fListView, 
		B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP_BOTTOM, 0, false, true, B_FANCY_BORDER);
	AddChild(scrollView);

	
}


void
TMView::AttachedToWindow()
{
	if (BButton *cancel = dynamic_cast<BButton*>(FindView("cancel"))) {
		Window()->SetDefaultButton(cancel);
		cancel->SetTarget(this);
	}

	if (BButton *reboot = dynamic_cast<BButton*>(FindView("force")))
		reboot->SetTarget(this);

	fKillButton->SetTarget(this);
	fRestartButton->SetTarget(this);
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
			UpdateList();
			break;
		}
		case TM_RESTART_DESKTOP: {
			if (!be_roster->IsRunning(kTrackerSignature))
				be_roster->Launch(kTrackerSignature);
			if (!be_roster->IsRunning(kDeskbarSignature))
				be_roster->Launch(kDeskbarSignature);
			break;
		}
		case TM_SELECTED_TEAM: {
			fKillButton->SetEnabled(fListView->CurrentSelection() >= 0);
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

			fListView->AddItem(item, item->IsSystemServer() ? fListView->CountItems() : 0);
			item->fFound = true;
			changed = true;
		}
	}	

	for (int32 i = fListView->CountItems() - 1; i >= 0; i--) {
		TMListItem *item = (TMListItem*)fListView->ItemAt(i);
		if (!item->fFound) {
			if (item == fDescView->Item()) {
				fDescView->SetItem(NULL);
				fKillButton->SetEnabled(false);
			}

			delete fListView->RemoveItem(i);
			changed = true;
		}
	}

	if (changed)
		fListView->Invalidate();

	bool desktopRunning = be_roster->IsRunning(kTrackerSignature)
		&&  be_roster->IsRunning(kDeskbarSignature);
	if (!desktopRunning && fRestartButton->IsHidden())
		fRestartButton->Show();
	fRestartButton->SetEnabled(!desktopRunning);
}


void
TMView::GetPreferredSize(float *_width, float *_height)
{
	fDescView->GetPreferredSize(_width, _height);

	if (_width)
		*_width += 28;

	if (_height) {
		*_height += fKillButton->Bounds().Height() * 2
			+ TMListItem::MinimalHeight() * 8 + 50;
	}
}


void
TMDescView::ResizeToPreferred()
{
	float bottom = Bounds().bottom;
	BView::ResizeToPreferred();
	MoveBy(0, bottom - Bounds().bottom);
}

//	#pragma mark -


TMDescView::TMDescView(BRect rect, uint32 resizeMode)
	: BBox(rect, "descview", resizeMode, B_WILL_DRAW | B_PULSE_NEEDED, B_NO_BORDER),
	fItem(NULL)
{
	SetFont(be_plain_font);

	fText[0] = "Select an application from the list above and click the";
	fText[1] = "\"Kill Application\" button in order to close it.";
	fText[2] = "Hold CONTROL+ALT+DELETE for %ld seconds to reboot.";

	fKeysPressed = false;
	fSeconds = 4;
}


void
TMDescView::Pulse()
{
	// ToDo: connect this mechanism with the keyboard device - it can tell us
	//	if ctrl-alt-del is pressed
	if (fKeysPressed) {
		fSeconds--;
		Invalidate();
	}
}


void
TMDescView::Draw(BRect rect)
{
	rect = Bounds();

	font_height	fontInfo;
	GetFontHeight(&fontInfo);
	float height = ceil(fontInfo.ascent + fontInfo.descent + fontInfo.leading + 2);

	if (fItem) {
		// draw icon and application path
		BRect frame(rect);
		frame.Set(frame.left, frame.top, frame.left + 31, frame.top + 31);
		SetDrawingMode(B_OP_OVER);
		DrawBitmap(fItem->LargeIcon(), frame);
		SetDrawingMode(B_OP_COPY);

		BPoint line(frame.right + 9, frame.top + fontInfo.ascent);
		if (!fItem->IsSystemServer())
			line.y += (frame.Height() - height) / 2;
		else
			line.y += (frame.Height() - 2 * height) / 2;

		BString path = fItem->Path()->Path();
		TruncateString(&path, B_TRUNCATE_MIDDLE, rect.right - line.x);
		DrawString(path.String(), line);

		if (fItem->IsSystemServer()) {
			line.y += height;
			//SetFont(be_bold_font);
			DrawString("(This team is a system component)", line);
			//SetFont(be_plain_font);
		}
	} else {
		BPoint line(rect.left, rect.top + fontInfo.ascent);

		for (int32 i = 0; i < 2; i++) {
			DrawString(fText[i], line);
			line.y += height;
		}

		char text[256];
		if (fSeconds >= 0)
			snprintf(text, sizeof(text), fText[2], fSeconds);
		else
			strcpy(text, "Booom!");

		line.y += height;
		DrawString(text, line);
	}
}


void
TMDescView::GetPreferredSize(float *_width, float *_height)
{
	if (_width) {
		float width = 0;
		for (int32 i = 0; i < 3; i++) {
			float stringWidth = StringWidth(fText[i]);
			if (stringWidth > width)
				width = stringWidth;
		}

		if (width < 330)
			width = 330;

		*_width = width;
	}

	if (_height) {
		font_height	fontInfo;
		GetFontHeight(&fontInfo);

		float height = 4 * ceil(fontInfo.ascent + fontInfo.descent + fontInfo.leading + 2);
		if (height < 32)
			height = 32;

		*_height = height;
	}
}


void
TMDescView::SetItem(TMListItem *item)
{
	fItem = item;
	Invalidate();
}
