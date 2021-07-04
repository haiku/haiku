/*
 * Copyright 2017 Julian Harnath <julian.harnath@rwth-aachen.de>
 * Copyright 2020-2021 Andrew Lindesay <apl@lindesay.co.nz>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "WorkStatusView.h"

#include <CardLayout.h>
#include <Catalog.h>
#include <LayoutBuilder.h>
#include <SeparatorView.h>
#include <StatusBar.h>
#include <StringFormat.h>
#include <StringView.h>

#include <stdio.h>

#include "BarberPole.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "WorkStatusView"

#define VIEW_INDEX_BARBER_POLE	(int32) 0
#define VIEW_INDEX_PROGRESS_BAR	(int32) 1


static const BSize kStatusBarSize = BSize(100,20);


WorkStatusView::WorkStatusView(const char* name)
	:
	BView(name, 0),
	fProgressBar(new BStatusBar("progress bar")),
	fBarberPole(new BarberPole("barber pole")),
	fProgressLayout(new BCardLayout()),
	fProgressView(new BView("progress view", 0)),
	fStatusText(new BStringView("status text", NULL))
{
	fProgressView->SetLayout(fProgressLayout);
	fProgressLayout->AddView(fBarberPole);
	fProgressLayout->AddView(fProgressBar);

	fBarberPole->SetExplicitSize(kStatusBarSize);
	fProgressBar->SetMaxValue(1.0f);
	fProgressBar->SetBarHeight(kStatusBarSize.Height());
	fProgressBar->SetExplicitSize(kStatusBarSize);

	fStatusText->SetFontSize(be_plain_font->Size() * 0.9f);

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.Add(new BSeparatorView())
		.AddGroup(B_HORIZONTAL)
			.SetInsets(5, 2, 5, 2)
			.Add(fProgressLayout, 0.2f)
			.Add(fStatusText)
			.AddGlue()
			.End()
	;
}


WorkStatusView::~WorkStatusView()
{
}


void
WorkStatusView::SetBusy(const BString& text)
{
	SetText(text);
	SetBusy();
}


void
WorkStatusView::SetBusy()
{
	fBarberPole->Start();
	if (fProgressLayout->VisibleIndex() != VIEW_INDEX_BARBER_POLE)
		fProgressLayout->SetVisibleItem(VIEW_INDEX_BARBER_POLE);
}


void
WorkStatusView::SetIdle()
{
	fBarberPole->Stop();
	fProgressLayout->SetVisibleItem(VIEW_INDEX_BARBER_POLE);
	SetText(NULL);
}


void
WorkStatusView::SetProgress(float value)
{
	fProgressBar->SetTo(value);
	if (fProgressLayout->VisibleIndex() != VIEW_INDEX_PROGRESS_BAR)
		fProgressLayout->SetVisibleItem(VIEW_INDEX_PROGRESS_BAR);
}


void
WorkStatusView::SetText(const BString& text)
{
	fStatusText->SetText(text);
}
