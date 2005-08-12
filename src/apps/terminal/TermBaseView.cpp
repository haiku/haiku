/*
 * Copyright (c) 2001-2005, Haiku, Inc.
 * Copyright (c) 2003-4 Kian Duffy <myob@users.sourceforge.net>
 * Parts Copyright (C) 1998,99 Kazuho Okui and Takashi Murai. 
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Kian Duffy <myob@users.sourceforge.net>
 */
#include "TermBaseView.h"
#include "TermView.h"

TermBaseView::TermBaseView (BRect frame, TermView *inTermView)
 :BView(frame, "baseview", B_FOLLOW_ALL_SIDES,
		B_WILL_DRAW | B_FRAME_EVENTS)
{
	fTermView = inTermView;
}

TermBaseView::~TermBaseView ()
{
}

void
TermBaseView::FrameResized (float width, float height)
{
	int font_width, font_height;
	int cols, rows;
	
	fTermView->GetFontInfo (&font_width, &font_height);
	
	cols = (int)(width - VIEW_OFFSET * 2) / font_width;
	rows = (int)(height - VIEW_OFFSET * 2)/ font_height;
	
	fTermView->ResizeTo (cols * font_width - 1, rows * font_height - 1);
}
