/*
 * Copyright 2003-2026, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Fernando Francisco de Oliveira
 *		Michael Wilber
 *		Axel Dörfler, axeld@pinc-software.de
 *		Philippe Houdoin
 */


#include "ShowImageStatusView.h"

#include <GroupLayout.h>

#include "InfoStatusView.h"
#include "ProgressStatusView.h"


ShowImageStatusView::ShowImageStatusView(const char* name)
	:
	BView(name, B_SUPPORTS_LAYOUT),
	fBusy(false),
	fInfoStatusView(new InfoStatusView),
	fProgressStatusView(new ProgressStatusView)
{
	SetLayout(new BGroupLayout(B_VERTICAL, 0));

	fProgressStatusView->Hide();

	GetLayout()->AddView(fInfoStatusView);
	GetLayout()->AddView(fProgressStatusView);
}


void
ShowImageStatusView::AttachedToWindow()
{
	SetBusy(false);
}


void
ShowImageStatusView::SetBusy(bool busy)
{
	if (busy == fBusy)
		return;

	if (busy) {
		fInfoStatusView->Hide();
		fProgressStatusView->Show();
	} else {
		fInfoStatusView->Show();
		fProgressStatusView->Hide();
	}
	fBusy = busy;
}


void
ShowImageStatusView::SetBusyText(const BString& text)
{
	fProgressStatusView->SetBusy(text);
}


void
ShowImageStatusView::SetBusyProgress(float progress)
{
	fProgressStatusView->SetProgress(progress);
}


void
ShowImageStatusView::Update(const entry_ref& ref, const BString& text,
	const BString& pages, const BString& imageType, float zoom)
{
	SetBusy(false);
	fInfoStatusView->Update(ref, text, pages, imageType, zoom);
}


void
ShowImageStatusView::SetZoom(float zoom)
{
	SetBusy(false);
	fInfoStatusView->SetZoom(zoom);
}
