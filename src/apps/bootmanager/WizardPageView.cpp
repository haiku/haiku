/*
 * Copyright 2008-2011, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */


#include "WizardPageView.h"

#include <math.h>
#include <string.h>

#include <TextView.h>


WizardPageView::WizardPageView(BMessage* settings, BRect frame,
	const char* name, uint32 resizingMode, uint32 flags)
	:
	BView(frame, name, resizingMode, flags),
	fSettings(settings)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


WizardPageView::WizardPageView(BMessage* settings, const char* name)
	:
	BView(name, B_WILL_DRAW),
	fSettings(settings)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


WizardPageView::~WizardPageView()
{
}


void
WizardPageView::PageCompleted()
{
}


BTextView*
WizardPageView::CreateDescription(BRect frame, const char* name,
	const char* description)
{
	BTextView* view = new BTextView(frame, "text",
		frame.OffsetToCopy(0, 0),
		B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP,
		B_WILL_DRAW | B_PULSE_NEEDED | B_FRAME_EVENTS);
	view->MakeEditable(false);
	view->SetViewColor(ViewColor());
	view->SetStylable(true);
	view->SetText(description);
	return view;
}


BTextView*
WizardPageView::CreateDescription(const char* name,
	const char* description)
{
	BTextView* view = new BTextView("text");
	view->MakeEditable(false);
	view->SetViewColor(ViewColor());
	view->SetStylable(true);
	view->SetText(description);
	return view;
}


void
WizardPageView::MakeHeading(BTextView* view)
{
	const char* text = view->Text();
	const char* firstLineEnd = strchr(text, '\n');
	if (firstLineEnd != NULL) {
		int indexFirstLineEnd = firstLineEnd - text;
		BFont font;
		view->GetFont(&font);
		font.SetFace(B_BOLD_FACE);
		view->SetFontAndColor(0, indexFirstLineEnd, &font);
	}
}


void
WizardPageView::LayoutDescriptionVertically(BTextView* view)
{
	view->SetTextRect(view->Bounds());

	float height = view->TextHeight(0, 32000);
	float width = view->Bounds().Width();

	view->ResizeTo(width, height);
	view->SetTextRect(view->Bounds());
}

