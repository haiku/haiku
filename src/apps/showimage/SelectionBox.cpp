/*
 * Copyright 2003-2010, Haiku, Inc. All Rights Reserved.
 * Copyright 2004-2005 yellowTAB GmbH. All Rights Reserverd.
 * Copyright 2006 Bernd Korz. All Rights Reserved
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Fernando Francisco de Oliveira
 *		Michael Wilber
 *		Michael Pfeiffer
 *		Ryan Leavengood
 *		yellowTAB GmbH
 *		Bernd Korz
 *		Stephan Aßmus <superstippi@gmx.de>
 */


#include "SelectionBox.h"

#include <math.h>
#include <new>
#include <stdio.h>

#include "ShowImageView.h"


SelectionBox::SelectionBox()
	:
	fBounds()
{
	_InitPatterns();
}


SelectionBox::~SelectionBox()
{
}


void
SelectionBox::SetBounds(ShowImageView* view, BRect bounds)
{
	view->ConstrainToImage(bounds);

	if (fBounds == bounds)
		return;

	BRect dirtyOld = _RectInView(view);

	fBounds = bounds;

	BRect dirtyNew = _RectInView(view);

	if (dirtyOld.IsValid() && dirtyNew.IsValid())
		view->Invalidate(dirtyOld | dirtyNew);
	else if (dirtyOld.IsValid())
		view->Invalidate(dirtyOld);
	else if (dirtyNew.IsValid())
		view->Invalidate(dirtyNew);
}


BRect
SelectionBox::Bounds() const
{
	return fBounds;
}


void
SelectionBox::MouseDown(ShowImageView* view, BPoint where)
{
	// TODO: Allow to re-adjust corners.
	where = view->ViewToImage(where);
	SetBounds(view, BRect(where, where));
}


void
SelectionBox::MouseMoved(ShowImageView* view, BPoint where)
{
	// TODO: Allow to re-adjust corners.
	where = view->ViewToImage(where);

	BRect bounds(fBounds);

	if (where.x >= bounds.left)
		bounds.right = where.x;
	else
		bounds.left = where.x;

	if (where.y >= bounds.top)
		bounds.bottom = where.y;
	else
		bounds.top = where.y;

	SetBounds(view, bounds);
}


void
SelectionBox::MouseUp(ShowImageView* view, BPoint where)
{
}


void
SelectionBox::Animate()
{
	// rotate up
	uchar p = fPatternUp.data[0];
	for (int i = 0; i <= 6; i++)
		fPatternUp.data[i] = fPatternUp.data[i + 1];
	fPatternUp.data[7] = p;

	// rotate down
	p = fPatternDown.data[7];
	for (int i = 7; i >= 1; i--)
		fPatternDown.data[i] = fPatternDown.data[i - 1];
	fPatternDown.data[0] = p;

	// rotate to left
	p = fPatternLeft.data[0];
	bool set = (p & 0x80) != 0;
	p <<= 1;
	p &= 0xfe;
	if (set)
		p |= 1;
	memset(fPatternLeft.data, p, 8);

	// rotate to right
	p = fPatternRight.data[0];
	set = (p & 1) != 0;
	p >>= 1;
	if (set)
		p |= 0x80;
	memset(fPatternRight.data, p, 8);
}


void
SelectionBox::Draw(ShowImageView* view, const BRect& updateRect) const
{
	BRect r = _RectInView(view);
	if (!r.IsValid() || !updateRect.Intersects(r))
		return;

	view->PushState();

	view->SetLowColor(255, 255, 255);
	view->StrokeLine(BPoint(r.left, r.top), BPoint(r.right, r.top),
		fPatternLeft);
	view->StrokeLine(BPoint(r.right, r.top + 1), BPoint(r.right, r.bottom - 1),
		fPatternUp);
	view->StrokeLine(BPoint(r.left, r.bottom), BPoint(r.right, r.bottom),
		fPatternRight);
	view->StrokeLine(BPoint(r.left, r.top + 1), BPoint(r.left, r.bottom - 1),
		fPatternDown);

	view->PopState();
}


// #pragma mark -


void
SelectionBox::_InitPatterns()
{
	uchar p;
	uchar p1 = 0x33;
	uchar p2 = 0xCC;
	for (int i = 0; i <= 7; i++) {
		fPatternLeft.data[i] = p1;
		fPatternRight.data[i] = p2;
		if ((i / 2) % 2 == 0)
			p = 255;
		else
			p = 0;
		fPatternUp.data[i] = p;
		fPatternDown.data[i] = ~p;
	}
}


BRect
SelectionBox::_RectInView(ShowImageView* view) const
{
	BRect r;
	if (fBounds.Height() <= 0.0 || fBounds.Width() <= 0.0)
		return r;

	r = fBounds;
	// Layout selection rect in view coordinate space
	view->ConstrainToImage(r);
	r = view->ImageToView(r);
	// draw selection box *around* selection
	r.InsetBy(-1, -1);

	return r;
}
