#include "PathView.h"
#include "LinePathBuilder.h"
#include <InterfaceKit.h>

class ShapeLPB : public LinePathBuilder
{
	BShape fShape;
	
protected:
	virtual void MoveTo(BPoint p);
	virtual void LineTo(BPoint p);
	virtual void BezierTo(BPoint* p);
	virtual void ClosePath(void);

public:
	ShapeLPB(SubPath *subPath, float penSize, cap_mode capMode, join_mode joinMode, float miterLimit);
	BShape *Shape() { return &fShape; }
};

ShapeLPB::ShapeLPB(SubPath *subPath, float penSize, cap_mode capMode, join_mode joinMode, float miterLimit)
	: LinePathBuilder(subPath, penSize, capMode, joinMode, miterLimit)
{
}

void ShapeLPB::MoveTo(BPoint p)
{
	fShape.MoveTo(p);
}

void ShapeLPB::LineTo(BPoint p)
{
	fShape.LineTo(p);
}

void ShapeLPB::BezierTo(BPoint p[3])
{
	fShape.BezierTo(p);
}

void ShapeLPB::ClosePath(void)
{
	fShape.Close();
}

PathView::PathView(BRect rect) 
	: BView(rect, NULL, B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE | B_SUBPIXEL_PRECISE) {
	//SetViewColor(B_TRANSPARENT_COLOR);
	fMode = kStroke;
	fCurPoint = -1;
	fWidth = 16;
}

void PathView::Draw(BRect updateRect) {
	if (fMode == kDrawOutline) {

	} else if (fMode == kStroke) {
		const int n = fPath.CountPoints();
		BShape shape;
		for (int i = 0; i < n; i++) {
			if (i == 0)
				shape.MoveTo(fPath.PointAt(i));
			else
				shape.LineTo(fPath.PointAt(i));
		}
		if (fPath.IsClosed()) shape.Close();
		SetPenSize(fWidth);
		StrokeShape(&shape);

		ShapeLPB path(&fPath, fWidth, LineCapMode(), LineJoinMode(), LineMiterLimit()); 
		path.CreateLinePath();
		SetPenSize(1);

		BPicture picture;
		BeginPicture(&picture);
		FillShape(path.Shape());
		EndPicture();

		PushState();
		ClipToPicture(&picture);
		SetHighColor(0, 255, 0);
		FillRect(Bounds());
		PopState();
		
		SetOrigin(200, 0);
		SetHighColor(255, 0, 0);
		StrokeShape(path.Shape());
		Flush();
	}
}

void PathView::MouseDown(BPoint point) {
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

void PathView::MouseMoved(BPoint point, uint32 transit, const BMessage *message) {
	if (fCurPoint != -1) {
		fPath.AtPut(fCurPoint, point);
		Invalidate();
	}
}

void PathView::MouseUp(BPoint point) {
	fCurPoint = -1;
}

void PathView::SetClose(bool close) {
	if (close) fPath.Close();
	else fPath.Open();
}
