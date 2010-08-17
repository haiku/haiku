/*
 * Copyright 2010, Haiku, Inc. All Rights Reserved.
 * Copyright 2009, Pier Luigi Fiorini.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Pier Luigi Fiorini, pierluigi.fiorini@gmail.com
 */

#include <stdio.h>

#include <Bitmap.h>

#include "IconItem.h"
#include "IconRule.h"

const int32 kEdgeOffset = 4;


BIconItem::BIconItem(BView* owner, const char* label, BBitmap* icon)
	:
	fLabel(label),
	fIcon(icon),
	fSelected(false),
	fOwner(owner)
{
}


BIconItem::~BIconItem()
{
	delete fIcon;
}

void BIconItem::Draw()
{
	if (IsSelected()) {
		rgb_color color;
		rgb_color origHigh;

		origHigh = fOwner->HighColor();

		if (IsSelected())
			color = ui_color(B_CONTROL_HIGHLIGHT_COLOR);
		else
			color = fOwner->ViewColor();

		fOwner->SetHighColor(color);
		fOwner->FillRect(fFrame);
		fOwner->SetHighColor(origHigh);
	}

	if (fIcon)
	{
		fOwner->SetDrawingMode(B_OP_ALPHA);
		fOwner->DrawBitmap(fIcon, BPoint(fFrame.top + kEdgeOffset,
			fFrame.left + kEdgeOffset));
		fOwner->SetDrawingMode(B_OP_COPY);
	}

	if (IsSelected())
		fOwner->SetDrawingMode(B_OP_OVER);

#if 0
	fOwner->MovePenTo(frame.left + kEdgeOffset + fIconWidth + kEdgeOffset,
		frame.bottom - fFontOffset);
#else
	fOwner->MovePenTo(fFrame.left, fFrame.bottom - 10);
#endif
	fOwner->SetHighColor(ui_color(B_CONTROL_TEXT_COLOR));
	fOwner->DrawString(" ");
	fOwner->DrawString(fLabel.String());
}


const char*
BIconItem::Label() const
{
	return fLabel.String();
}


void
BIconItem::SetFrame(BRect frame)
{
	fFrame = frame;
}


BRect
BIconItem::Frame() const
{
	return fFrame;
}


void
BIconItem::Select()
{
	fSelected = true;
	Draw();
}


void
BIconItem::Deselect()
{
	fSelected = false;
	Draw();
}


bool
BIconItem::IsSelected() const
{
	return fSelected;
}
