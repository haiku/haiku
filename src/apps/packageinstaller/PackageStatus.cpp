/*
 * Copyright (c) 2007, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		Łukasz 'Sil2100' Zemczak <sil2100@vexillium.org>
 */

#include "PackageStatus.h"

#include <Autolock.h>
#include <Catalog.h>
#include <GroupLayoutBuilder.h>
#include <GroupLayout.h>
#include <Locale.h>

#include <stdio.h>
#include <string.h>

#undef TR_CONTEXT
#define TR_CONTEXT "PackageStatus"

StopButton::StopButton()
	:
	BButton(BRect(0, 0, 22, 18), "stop", TR("Stop"), new BMessage(P_MSG_STOP))
{
}


void
StopButton::Draw(BRect updateRect)
{
	BButton::Draw(updateRect);

	updateRect = Bounds();
	updateRect.InsetBy((updateRect.Width() - 4) / 2,
		(updateRect.Height() - 4) / 2);
	//updateRect.InsetBy(9, 7);
	SetHighColor(0, 0, 0);
	FillRect(updateRect);
}





// #pragma mark -


/*PackageStatus::PackageStatus(BHandler *parent, const char *title, 
		const char *label, const char *trailing)
	:	BWindow(BRect(200, 200, 550, 275), title, B_TITLED_WINDOW,
			B_NOT_CLOSABLE | B_NOT_RESIZABLE | B_NOT_ZOOMABLE, 0)
{
	SetSizeLimits(0, 100000, 0, 100000);
	fBackground = new BView(Bounds(), "background", B_FOLLOW_NONE, B_WILL_DRAW);
	fBackground->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	BRect rect(Bounds());
	float width, height;
	rect.left += 6;
	rect.right -= 40;
	rect.top += 6;
	rect.bottom = rect.top + 15;
	fStatus = new BStatusBar(rect, "status_bar", T("Installing package"));
	fStatus->SetBarHeight(12);
	fStatus->GetPreferredSize(&width, &height);
	fStatus->ResizeTo(fStatus->Frame().Width(), height);
	fBackground->AddChild(fStatus);

	font_height fontHeight;
	fBackground->GetFontHeight(&fontHeight);
	BRect frame = fStatus->Frame();
	fBackground->ResizeTo(Bounds().Width(), (2 * frame.top) + frame.Height() + 
			fontHeight.leading + fontHeight.ascent + fontHeight.descent);

	rect = Bounds();
	rect.left = rect.right - 32;
	//rect.right = rect.left + 17;
	rect.top += 18;
	//rect.bottom = rect.top + 10;
	fButton = new StopButton();
	fButton->MoveTo(BPoint(rect.left, rect.top));
	fButton->ResizeTo(22, 18);
	fBackground->AddChild(fButton);

	AddChild(fBackground);
	fButton->SetTarget(parent);

	ResizeTo(Bounds().Width(), fBackground->Bounds().Height());
	Run();
}*/


PackageStatus::PackageStatus(const char *title, const char *label, 
		const char *trailing, BHandler *parent)
	:
	BWindow(BRect(200, 200, 550, 255), title, B_TITLED_WINDOW,
		B_NOT_CLOSABLE | B_NOT_RESIZABLE | B_NOT_ZOOMABLE, 0),
	fIsStopped(false),
	fParent(parent)
{
	SetLayout(new BGroupLayout(B_VERTICAL));

	fStatus = new BStatusBar("status_bar", TR("Installing package"));
	fStatus->SetBarHeight(12);

	fButton = new StopButton();
	fButton->SetExplicitMaxSize(BSize(22, 18));

	fBackground = BGroupLayoutBuilder(B_HORIZONTAL)
		.AddStrut(5.0f)
		.Add(fStatus)
		.Add(fButton);
	fBackground->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	AddChild(fBackground);
	
	fButton->SetTarget(this);
	Run();
}


PackageStatus::~PackageStatus()
{
}


void
PackageStatus::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case P_MSG_STOP:
			fIsStopped = true;
			if (fParent != NULL) {
				// If we have a parent defined, forward this message
				BLooper *loop = fParent->Looper();
				if (loop != NULL) {
					loop->PostMessage(msg, fParent);
				}
			}
			break;
		default:
			BWindow::MessageReceived(msg);
	}
}


void
PackageStatus::Reset(uint32 stages, const char *label, const char *trailing)
{
	BAutolock lock(this);

	if (lock.IsLocked()) {
		fStatus->Reset(label, trailing);
		fStatus->SetMaxValue(stages);
		fIsStopped = false;
	}
}


void
PackageStatus::StageStep(uint32 count, const char *text, const char *trailing)
{
	BAutolock lock(this);

	if (lock.IsLocked()) {
		fStatus->Update(count, text, trailing);
	}
}


bool
PackageStatus::Stopped()
{
	BAutolock lock(this);
	return fIsStopped;
}

