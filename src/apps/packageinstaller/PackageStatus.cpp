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
#include <GroupLayout.h>
#include <LayoutBuilder.h>
#include <Locale.h>

#include <stdio.h>
#include <string.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PackageStatus"

StopButton::StopButton()
	:
	BButton(BRect(0, 0, 22, 18), "stop", B_TRANSLATE("Stop"),
		new BMessage(P_MSG_STOP))
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


PackageStatus::PackageStatus(const char *title, const char *label,
		const char *trailing, BHandler *parent)
	:
	BWindow(BRect(200, 200, 550, 255), title, B_TITLED_WINDOW,
		B_NOT_CLOSABLE | B_NOT_RESIZABLE | B_NOT_ZOOMABLE, 0),
	fIsStopped(false),
	fParent(parent)
{
	fStatus = new BStatusBar("status_bar", B_TRANSLATE("Installing package"));
	fStatus->SetBarHeight(12);

	fButton = new StopButton();
	fButton->SetExplicitMaxSize(BSize(22, 18));

	BLayoutBuilder::Group<>(this, B_HORIZONTAL, 0)
		.AddStrut(5.0f)
		.Add(fStatus)
		.Add(fButton);

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

