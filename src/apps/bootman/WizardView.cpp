/*
 * Copyright 2008, Michael Pfeiffer, laplace@users.sourceforge.net. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "WizardView.h"

#include "WizardPageView.h"

#include <Box.h>

static const float kSeparatorHeight = 2;
static const float kSeparatorDistance = 5;
static const float kBorderWidth = 10;
static const float kBorderHeight = 5;


WizardView::WizardView(BRect frame, const char* name, uint32 resizingMode)
	: BView(frame, name, resizingMode, 0)
	, fSeparator(NULL)
	, fPrevious(NULL)
	, fNext(NULL)
	, fPage(NULL)
{
	_BuildUI();
	SetPreviousButtonHidden(true);
}


WizardView::~WizardView()
{
}


BRect
WizardView::PageFrame()
{
	float left = kBorderWidth;
	float right = Bounds().right - kBorderWidth;
	float top = kBorderHeight;
	float bottom = fSeparator->Frame().top - kSeparatorDistance - 1;
	return BRect(left, top, right, bottom);	
}


void 
WizardView::SetPage(WizardPageView* page)
{
	if (fPage == page)
		return;
	
	if (fPage != NULL) {
		RemoveChild(fPage);
		delete fPage;
	}
	
	fPage = page;
	if (page == NULL)
		return;
	
	BRect frame = PageFrame();
	page->MoveTo(frame.left, frame.top);
	page->ResizeTo(frame.Width()+1, frame.Height()+1);
	AddChild(page);
}


void
WizardView::PageCompleted()
{
	if (fPage != NULL)
		fPage->PageCompleted();
}


void
WizardView::SetPreviousButtonEnabled(bool enabled)
{
	fPrevious->SetEnabled(enabled);
}


void
WizardView::SetNextButtonEnabled(bool enabled)
{
	fNext->SetEnabled(enabled);
}


void
WizardView::SetPreviousButtonLabel(const char* text)
{
	fPrevious->SetLabel(text);
	fPrevious->ResizeToPreferred();
}


void
WizardView::SetNextButtonLabel(const char* text)
{
	BRect frame = fNext->Frame();
	fNext->SetLabel(text);
	fNext->ResizeToPreferred();
	BRect newFrame = fNext->Frame();
	fNext->MoveBy(frame.Width() - newFrame.Width(), 
		frame.Height() - newFrame.Height());
}


void 
WizardView::SetPreviousButtonHidden(bool hide)
{
	if (hide) {
		if (!fPrevious->IsHidden())
			fPrevious->Hide();
	} else {
		if (fPrevious->IsHidden())
			fPrevious->Show();
	}
}


void
WizardView::_BuildUI()
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	float width = Bounds().Width();
	float height = Bounds().Height();

	fSeparator = new BBox(BRect(kBorderWidth, 0, 
		width - 1 - kBorderWidth, kSeparatorHeight - 1), 
		"separator",
		B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM);
	AddChild(fSeparator);
	
	fPrevious = new BButton(BRect(0, 0, 100, 20), "previous", "Previous",
		new BMessage(kMessagePrevious), 
		B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	AddChild(fPrevious);
	fPrevious->ResizeToPreferred();
	
	fNext = new BButton(BRect(0, 0, 100, 20), "next", "Next", 
		new BMessage(kMessageNext), 
		B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	AddChild(fNext);
	fNext->ResizeToPreferred();

	// layout views
	float buttonHeight = fPrevious->Bounds().Height();	
	float buttonTop = height - 1 - buttonHeight - kBorderHeight;
	fPrevious->MoveTo(kBorderWidth, buttonTop);
	
	fSeparator->MoveTo(kBorderWidth, buttonTop - kSeparatorDistance - kSeparatorHeight);
		
	fNext->MoveTo(width - fNext->Bounds().Width() - kBorderWidth - 1, buttonTop);
}
