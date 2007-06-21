// Copyright 1999, Be Incorporated. All Rights Reserved.
// Copyright 2000-2004, Jun Suzuki. All Rights Reserved.
// Copyright 2007, Stephan Aßmus. All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.
#include "StatusView.h"


StatusView::StatusView(BRect frame, uint32 resizingMode)
	: BView(frame, "StatusView", resizingMode,
		B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE)
	, fStatusString(B_EMPTY_STRING)
{
	frame.OffsetTo(0, 0);
	fStatusRect = frame;

	// TODO: bad!
	BFont font;
	GetFont(&font);
	font.SetSize(12);
	SetFont(&font);
}


StatusView::~StatusView()
{
}


void
StatusView::AttachedToWindow()
{
	rgb_color c = Parent()->LowColor();
	SetViewColor(c);
	SetLowColor(c);
}


void
StatusView::Draw(BRect /*update*/)
{
	// TODO: placement information should be cached here
	// for better performance
	font_height fh;
	GetFontHeight(&fh);
	DrawString(fStatusString.String(),
		fStatusRect.LeftBottom() + BPoint(1, -fh.descent));
}


void
StatusView::MessageReceived(BMessage* message)
{
	BView::MessageReceived(message);
}


void
StatusView::SetStatus(const char* text)
{
	fStatusString.SetTo(text);
	Invalidate();
}


const char*
StatusView::Status()
{
	return fStatusString.String();
}

