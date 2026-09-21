/*
 * Copyright 2026 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Philippe Houdoin
 */

#include "ProgressStatusView.h"

#include <stdio.h>
#include <string.h>


#include <BarberPole.h>
#include <CardLayout.h>
#include <ControlLook.h>
#include <LayoutBuilder.h>
#include <StatusBar.h>
#include <StatusView.h>

#include "ShowImageApp.h"
#include "ShowImageConstants.h"


enum {
	BARBER_POLE,
	PROGRESS_BAR
};


ProgressStatusView::ProgressStatusView(const char* name)
	:
	BView(name, B_WILL_DRAW | B_PULSE_NEEDED | B_SUPPORTS_LAYOUT),
	fProgressBar(new BStatusBar("progress_bar")),
	fBarberPole(new BarberPole("barber_pole")),
	fProgressLayout(new BCardLayout()),
	fProgressView(new BView("progress_view", 0)),
	fLastPercentReceivedTime(0)
{
	fProgressView->SetLayout(fProgressLayout);
	fProgressLayout->AddView(fBarberPole);
	fProgressLayout->AddView(fProgressBar);

	fProgressBar->SetMaxValue(1.0f);

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.AddGroup(B_HORIZONTAL, 0)
			.SetInsets(5, 1)
			.Add(fProgressLayout)
			.AddGlue()
		.End();
}


void
ProgressStatusView::AttachedToWindow()
{
	fMaxHeight = be_control_look->GetScrollBarWidth();
	fBarSize = BSize(100, fMaxHeight * 0.70f);

	fBarberPole->SetExplicitSize(fBarSize);
	fProgressBar->SetExplicitSize(fBarSize);
	fProgressBar->SetBarHeight(fBarSize.Height() + 2);

	AdoptParentColors();

	SetFont(be_plain_font);
	BPrivate::AdoptScrollBarFontSize(this);

	SetIdle();
}


void
ProgressStatusView::Pulse()
{
	if (IsHidden() || fProgressLayout->VisibleIndex() == BARBER_POLE)
		return;

	if (system_time() - fLastPercentReceivedTime > 1000000) { // 1s
		// we received no new progress percent since too long,
		// switch back to barber pole
		fBarberPole->Start();
		fProgressLayout->SetVisibleItem(BARBER_POLE);
	}
}


void
ProgressStatusView::Draw(BRect updateRect)
{
	BRect bounds(Bounds());
	be_control_look->DrawMenuBarBackground(this, bounds, updateRect, ViewColor());

	bounds = Bounds(); // needed, as DrawMenuBarBackground() modified it
	SetHighColor(tint_color(ViewColor(), B_DARKEN_2_TINT));
	StrokeLine(bounds.LeftTop(), bounds.RightTop());

	if (fStatusText.IsEmpty())
		return;

	SetLowColor(ViewColor());
	SetHighColor(ui_color(B_PANEL_TEXT_COLOR));

	font_height fontHeight;
	GetFontHeight(&fontHeight);

	float x = fBarSize.width + 10.0f;
	float y
		= (bounds.bottom + bounds.top + ceilf(fontHeight.ascent) - ceilf(fontHeight.descent)) / 2;

	DrawString(fStatusText, BPoint(x, y));
}


void
ProgressStatusView::SetBusy(const BString& text)
{
	SetText(text);
	SetBusy();
}


void
ProgressStatusView::SetBusy()
{
	if (fLastPercentReceivedTime == 0) {
		fBarberPole->Start();
		if (fProgressLayout->VisibleIndex() != BARBER_POLE)
			fProgressLayout->SetVisibleItem(BARBER_POLE);
	}
}


void
ProgressStatusView::SetIdle()
{
	fBarberPole->Stop();
	fProgressLayout->SetVisibleItem(BARBER_POLE);
	SetText(NULL);
	fLastPercentReceivedTime = 0;
}


void
ProgressStatusView::SetProgress(float value)
{
	fBarberPole->Stop();
	fProgressBar->SetTo(value);
	fLastPercentReceivedTime = system_time();

	if (fProgressLayout->VisibleIndex() != PROGRESS_BAR)
		fProgressLayout->SetVisibleItem(PROGRESS_BAR);
}


void
ProgressStatusView::SetText(const BString& text)
{
	if (fStatusText != text) {
		fStatusText.SetTo(text);
		_ValidatePreferredSize();
		// InvalidateLayout();
		Invalidate();
	}
}


void
ProgressStatusView::_ValidatePreferredSize()
{
	fPreferredSize.Set(fBarSize.width + 10.0f, fMaxHeight);

	if (!fStatusText.IsEmpty()) {
		float statusTextWidth = ceilf(StringWidth(fStatusText));
		fPreferredSize.width += statusTextWidth + 5.0f;
	}
	SetExplicitMinSize(fPreferredSize);
	SetExplicitMaxSize(fPreferredSize);
}
