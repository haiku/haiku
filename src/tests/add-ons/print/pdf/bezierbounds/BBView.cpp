#include "BezierBounds.h"
#include "BBView.h"
#include <InterfaceKit.h>

BBView::BBView(BRect rect) 
	: BView(rect, NULL, B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE | B_SUBPIXEL_PRECISE) {
	//SetViewColor(B_TRANSPARENT_COLOR);
	fMode = kStroke;
	fCurPoint = -1;
	fWidth = 16;
}

void BBView::Draw(BRect updateRect) {
	if (fMode == kDrawOutline) {

	} else if (fMode == kStroke) {
		int i;
		if (fPath.CountPoints() > 3) {
			const int n = 3 * ((fPath.CountPoints()-1) / 3);
			BShape shape;
			shape.MoveTo(fPath.PointAt(0));
			for (i = 1; i < n; i += 3) {
				BPoint bezier[3] = { fPath.PointAt(i), fPath.PointAt(i+1), fPath.PointAt(i+2) };
				shape.BezierTo(bezier);
			}
			if (fPath.IsClosed()) shape.Close();
			SetPenSize(fWidth);
			StrokeShape(&shape);
	
			BRect bounds[2];
			SetPenSize(1);
			SetHighColor(255, 0, 0);
			SetLowColor(0, 255, 0);
			for (i = 0; i < n; i += 3) {
				BPoint bezier[4] = { fPath.PointAt(i), fPath.PointAt(i+1), fPath.PointAt(i+2), fPath.PointAt(i+3) };
				BRect r = BezierBounds(bezier, 4);
				StrokeRect(r);
				if (i == 0) bounds[0] = r; else bounds[0] = bounds[0] | r;
				r = BezierBounds(bezier, 4, fWidth, LineCapMode(), LineJoinMode(), LineMiterLimit());
				StrokeRect(r, B_SOLID_LOW);
				if (i == 0) bounds[1] = r; else bounds[1] = bounds[1] | r;
			}
			if (fPath.IsClosed()) {
				StrokeRect(bounds[0]);
				StrokeRect(bounds[1], B_SOLID_LOW);
			}
		}
		
		SetHighColor(0, 0, 255);
		for (i = 0; i < fPath.CountPoints(); i++) {
			BPoint p = fPath.PointAt(i);
			StrokeLine(BPoint(p.x-2, p.y), BPoint(p.x+2, p.y));
			StrokeLine(BPoint(p.x, p.y-2), BPoint(p.x, p.y+2));
		}
	}
}

void BBView::MouseDown(BPoint point) {
	uint32 buttons;
	GetMouse(&point, &buttons, false);

	if (buttons == B_SECONDARY_MOUSE_BUTTON) {
		fCurPoint = fPath.CountPoints();
		fPath.AddPoint(point);
	} else {
		float d = 100000000000.0;
		for (int i = 0; i < fPath.CountPoints(); i++) {
			BPoint p = point - fPath.PointAt(i);
			float e = p.x*p.x + p.y*p.y;
			if (e < d) { fCurPoint = i; d = e; }
		}
		fPath.AtPut(fCurPoint, point);
	}
	Invalidate();
}

void BBView::MouseMoved(BPoint point, uint32 transit, const BMessage *message) {
	if (fCurPoint != -1) {
		fPath.AtPut(fCurPoint, point);
		Invalidate();
	}
}

void BBView::MouseUp(BPoint point) {
	fCurPoint = -1;
}

void BBView::SetClose(bool close) {
	if (close) fPath.Close();
	else fPath.Open();
}
