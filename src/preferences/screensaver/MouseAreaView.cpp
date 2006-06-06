/*
 * Copyright 2003-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Phipps
 */

#include "MouseAreaView.h"
#include "Constants.h"
#include "Utility.h"

#include <Rect.h>
#include <Point.h>
#include <Shape.h>

#include <stdio.h>



inline BPoint
scale_arrow(int x, int y, BRect area, bool invertX, bool invertY)
{
	float arrowX[] = {0, .25, .5, .66667, .90, .9};
	float arrowY[] = {0, .15, .25, .3333333, .6666667, 1.0};

	return scale_direct(invertX ? 1 - arrowX[x] : arrowX[x],
		invertY ? 1 - arrowY[y] : arrowY[y], area);
}


//	#pragma mark -


MouseAreaView::MouseAreaView(BRect frame, const char *name) 
	: BView (frame, name, B_FOLLOW_NONE, B_WILL_DRAW),
	fCurrentCorner(NONE)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


void
MouseAreaView::Draw(BRect update) 
{
	// Top of monitor
	SetHighColor(kGrey);
	FillRoundRect(scale(0, 7, 0, 3, Bounds()), 4, 4);
	SetHighColor(kBlack);
	StrokeRoundRect(scale(0, 7, 0, 3, Bounds()), 4, 4);
	SetHighColor(kLightBlue);
	fMonitorFrame = scale(1, 6, 1, 2, Bounds());
	FillRect(fMonitorFrame);
	SetHighColor(kDarkGrey);
	StrokeRect(fMonitorFrame);

	// Base of monitor
	SetHighColor(kGrey);
	FillTriangle(scale(1,5,Bounds()),scale(3,4,Bounds()),scale(3,5,Bounds()));
	FillTriangle(scale(4,5,Bounds()),scale(4,4,Bounds()),scale(6,5,Bounds()));
	FillRect(scale(3,4,4,5,Bounds()));
	FillRect(scale(3,4,3,4,Bounds()));
	SetHighColor(kBlack);
	StrokeRect(scale(3,4,3,4,Bounds()));
	StrokeLine(scale(2,4,Bounds()),scale(1,5,Bounds()));
	StrokeLine(scale(6,5,Bounds()),scale(1,5,Bounds()));
	StrokeLine(scale(6,5,Bounds()),scale(5,4,Bounds()));
	DrawArrow();
}


void
MouseAreaView::SetCorner(screen_corner corner)
{
	fCurrentCorner = corner;
	Invalidate();
}


BRect
MouseAreaView::_ArrowSize(BRect monitorRect, bool isCentered)
{
	int arrowSize = monitorRect.IntegerWidth();
	if (monitorRect.IntegerHeight() < arrowSize)
		arrowSize = monitorRect.IntegerHeight();

	arrowSize /= 3;

	BRect arrowRect(0, 0, arrowSize, arrowSize);
	if (isCentered) {
		arrowRect.OffsetBy(monitorRect.left + (monitorRect.Width() - arrowSize) / 2,
			monitorRect.top + (monitorRect.Height() - arrowSize) / 2);
	}

	return arrowRect;
}


void
MouseAreaView::DrawArrow() 
{
	PushState();

	BRect rect(_ArrowSize(fMonitorFrame, fCurrentCorner == NONE));

	if (fCurrentCorner != NONE) {
		bool invertX = fCurrentCorner == UPRIGHT || fCurrentCorner == DOWNRIGHT;
		bool invertY = fCurrentCorner == UPRIGHT || fCurrentCorner == UPLEFT;

		float size = rect.Width();
		MovePenTo(invertX ? fMonitorFrame.right - size - 2 : 2 + fMonitorFrame.left,
			!invertY ? fMonitorFrame.bottom - size - 2 : 2 + fMonitorFrame.top);

		BShape arrow;
		arrow.MoveTo(scale_arrow(0, 1, rect, invertX, invertY));
		arrow.LineTo(scale_arrow(0, 5, rect, invertX, invertY));
		arrow.LineTo(scale_arrow(4, 5, rect, invertX, invertY));
		arrow.LineTo(scale_arrow(3, 4, rect, invertX, invertY));
		arrow.LineTo(scale_arrow(5, 3, rect, invertX, invertY));
		arrow.LineTo(scale_arrow(2, 0, rect, invertX, invertY));
		arrow.LineTo(scale_arrow(1, 2, rect, invertX, invertY));
		arrow.LineTo(scale_arrow(0, 1, rect, invertX, invertY));
		arrow.Close();
		SetHighColor(kBlack);
		FillShape(&arrow, B_SOLID_HIGH);
	} else {
		SetHighColor(255, 0, 0);
		SetPenSize(2);
		StrokeEllipse(rect);
		StrokeLine(BPoint(rect.right, rect.top), BPoint(rect.left, rect.bottom));
	}

	PopState();
}


void
MouseAreaView::MouseUp(BPoint point) 
{
	if (fMonitorFrame.Contains(point)) {
		if (_ArrowSize(fMonitorFrame, true).Contains(point))
			fCurrentCorner = NONE;
		else {
			float centerX = fMonitorFrame.left + fMonitorFrame.Width() / 2;
			float centerY = fMonitorFrame.top + fMonitorFrame.Height() / 2;
			if (point.x < centerX)
				fCurrentCorner = point.y < centerY ? UPLEFT : DOWNLEFT;
			else
				fCurrentCorner = point.y < centerY ? UPRIGHT : DOWNRIGHT;
		}
		Draw(fMonitorFrame);
	}
}
