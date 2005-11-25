/*
 * Copyright (c) 2001-2005, Haiku, Inc.
 * Copyright (c) 2003-4 Kian Duffy <myob@users.sourceforge.net>
 * Parts Copyright (C) 1998,99 Kazuho Okui and Takashi Murai. 
 * Distributed under the terms of the MIT license.
 */

/*!	This view is a shell for the TermView and makes sure it's always
	resized in whole character sizes.
	It doesn't serve any other purpose (and could possibly removed
	with a tiny bit smarter programming).
*/


#include "TermBaseView.h"
#include "TermView.h"


TermBaseView::TermBaseView(BRect frame, TermView *inTermView)
	: BView(frame, "baseview", B_FOLLOW_ALL_SIDES,
		B_WILL_DRAW | B_FRAME_EVENTS)
{
	fTermView = inTermView;
}


TermBaseView::~TermBaseView()
{
}


void
TermBaseView::FrameResized(float width, float height)
{
	int fontWidth, fontHeight;
	int cols, rows;

	fTermView->GetFontInfo(&fontWidth, &fontHeight);

	cols = (int)(width - VIEW_OFFSET * 2) / fontWidth;
	rows = (int)(height - VIEW_OFFSET * 2) / fontHeight;

	fTermView->ResizeTo(cols * fontWidth - 1, rows * fontHeight - 1);
}
