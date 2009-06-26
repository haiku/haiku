/*
 * Copyright 2006-2009, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "TransformBox.h"

#include <stdio.h>

#include <agg_trans_affine.h>
#include <agg_math.h>

#include <View.h>

#include "support.h"

#include "TransformBoxStates.h"
#include "StateView.h"
#include "TransformCommand.h"


#define INSET 8.0


TransformBoxListener::TransformBoxListener()
{
}


TransformBoxListener::~TransformBoxListener()
{
}


// #pragma mark -


// constructor
TransformBox::TransformBox(StateView* view, BRect box)
	:
	ChannelTransform(),
	Manipulator(NULL),
	fOriginalBox(box),

	fLeftTop(box.LeftTop()),
	fRightTop(box.RightTop()),
	fLeftBottom(box.LeftBottom()),
	fRightBottom(box.RightBottom()),

	fPivot((fLeftTop.x + fRightBottom.x) / 2.0,
		(fLeftTop.y + fRightBottom.y) / 2.0),
	fPivotOffset(B_ORIGIN),
	fCurrentCommand(NULL),
	fCurrentState(NULL),

	fDragging(false),
	fMousePos(-10000.0, -10000.0),
	fModifiers(0),

	fNudging(false),

	fView(view),

	fDragLTState(new DragCornerState(this, DragCornerState::LEFT_TOP_CORNER)),
	fDragRTState(new DragCornerState(this, DragCornerState::RIGHT_TOP_CORNER)),
	fDragLBState(new DragCornerState(this, DragCornerState::LEFT_BOTTOM_CORNER)),
	fDragRBState(new DragCornerState(this, DragCornerState::RIGHT_BOTTOM_CORNER)),

	fDragLState(new DragSideState(this, DragSideState::LEFT_SIDE)),
	fDragRState(new DragSideState(this, DragSideState::RIGHT_SIDE)),
	fDragTState(new DragSideState(this, DragSideState::TOP_SIDE)),
	fDragBState(new DragSideState(this, DragSideState::BOTTOM_SIDE)),

	fRotateState(new RotateBoxState(this)),
	fTranslateState(new DragBoxState(this)),
	fOffsetCenterState(new OffsetCenterState(this))
{
}


// destructor
TransformBox::~TransformBox()
{
	_NotifyDeleted();

	delete fCurrentCommand;

	delete fDragLTState;
	delete fDragRTState;
	delete fDragLBState;
	delete fDragRBState;

	delete fDragLState;
	delete fDragRState;
	delete fDragTState;
	delete fDragBState;

	delete fRotateState;
	delete fTranslateState;
	delete fOffsetCenterState;
}


// Draw
void
TransformBox::Draw(BView* into, BRect updateRect)
{
	// convert to canvas view coordinates
	BPoint lt = fLeftTop;
	BPoint rt = fRightTop;
	BPoint lb = fLeftBottom;
	BPoint rb = fRightBottom;
	BPoint c = fPivot;

	TransformFromCanvas(lt);
	TransformFromCanvas(rt);
	TransformFromCanvas(lb);
	TransformFromCanvas(rb);
	TransformFromCanvas(c);

	into->SetDrawingMode(B_OP_COPY);
	into->SetHighColor(255, 255, 255, 255);
	into->SetLowColor(0, 0, 0, 255);
	_StrokeBWLine(into, lt, rt);
	_StrokeBWLine(into, rt, rb);
	_StrokeBWLine(into, rb, lb);
	_StrokeBWLine(into, lb, lt);

	double rotation = ViewSpaceRotation();
	_StrokeBWPoint(into, lt, rotation);
	_StrokeBWPoint(into, rt, rotation + 90.0);
	_StrokeBWPoint(into, rb, rotation + 180.0);
	_StrokeBWPoint(into, lb, rotation + 270.0);

	BRect cr(c, c);
	cr.InsetBy(-3.0, -3.0);
	into->StrokeEllipse(cr, B_SOLID_HIGH);
	cr.InsetBy(1.0, 1.0);
	into->StrokeEllipse(cr, B_SOLID_LOW);
	into->SetDrawingMode(B_OP_COPY);
}


// #pragma mark -


// MouseDown
bool
TransformBox::MouseDown(BPoint where)
{
	fView->FilterMouse(&where);
		// NOTE: filter mouse here and in MouseMoved only
	TransformToCanvas(where);

	fDragging = true;
	if (fCurrentState) {
		fCurrentState->SetOrigin(where);

		delete fCurrentCommand;
		fCurrentCommand = MakeCommand(fCurrentState->ActionName(),
			fCurrentState->ActionNameIndex());
	}

	return true;
}


// MouseMoved
void
TransformBox::MouseMoved(BPoint where)
{
	fView->FilterMouse(&where);
		// NOTE: filter mouse here and in MouseDown only
	TransformToCanvas(where);

	if (fMousePos != where) {
		fMousePos = where;
		if (fCurrentState) {
			fCurrentState->DragTo(fMousePos, fModifiers);
			fCurrentState->UpdateViewCursor(fView, fMousePos);
		}
	}
}


// MouseUp
Command*
TransformBox::MouseUp()
{
	fDragging = false;
	return FinishTransaction();
}


// MouseOver
bool
TransformBox::MouseOver(BPoint where)
{
	TransformToCanvas(where);

	_SetState(_DragStateFor(where, ZoomLevel()));
	fMousePos = where;
	if (fCurrentState) {
		fCurrentState->UpdateViewCursor(fView, fMousePos);
		return true;
	}
	return false;
}


// DoubleClicked
bool
TransformBox::DoubleClicked(BPoint where)
{
	return false;
}


// #pragma mark -


// Bounds
BRect
TransformBox::Bounds()
{
	// convert from canvas view coordinates
	BPoint lt = fLeftTop;
	BPoint rt = fRightTop;
	BPoint lb = fLeftBottom;
	BPoint rb = fRightBottom;
	BPoint c = fPivot;

	TransformFromCanvas(lt);
	TransformFromCanvas(rt);
	TransformFromCanvas(lb);
	TransformFromCanvas(rb);
	TransformFromCanvas(c);

	BRect r;
	r.left = min5(lt.x, rt.x, lb.x, rb.x, c.x);
	r.top = min5(lt.y, rt.y, lb.y, rb.y, c.y);
	r.right = max5(lt.x, rt.x, lb.x, rb.x, c.x);
	r.bottom = max5(lt.y, rt.y, lb.y, rb.y, c.y);
	return r;
}


// TrackingBounds
BRect
TransformBox::TrackingBounds(BView* withinView)
{
	return withinView->Bounds();
}


// #pragma mark -


// ModifiersChanged
void
TransformBox::ModifiersChanged(uint32 modifiers)
{
	fModifiers = modifiers;
	if (fDragging && fCurrentState) {
		fCurrentState->DragTo(fMousePos, fModifiers);
	}
}


// HandleKeyDown
bool
TransformBox::HandleKeyDown(uint32 key, uint32 modifiers, Command** _command)
{
	bool handled = true;
	BPoint translation(B_ORIGIN);

	float offset = 1.0;
	if (modifiers & B_SHIFT_KEY)
		offset /= ZoomLevel();

	switch (key) {
		case B_UP_ARROW:
			translation.y = -offset;
			break;
		case B_DOWN_ARROW:
			translation.y = offset;
			break;
		case B_LEFT_ARROW:
			translation.x = -offset;
			break;
		case B_RIGHT_ARROW:
			translation.x = offset;
			break;

		default:
			handled = false;
			break;
	}

	if (!handled)
		return false;

	if (!fCurrentCommand) {
		fCurrentCommand = MakeCommand("Translate", -1);
	}

	TranslateBy(translation);

	return true;
}


// HandleKeyUp
bool
TransformBox::HandleKeyUp(uint32 key, uint32 modifiers, Command** _command)
{
	if (fCurrentCommand) {
		*_command = FinishTransaction();
		return true;
	}
	return false;
}


// UpdateCursor
bool
TransformBox::UpdateCursor()
{
	if (fCurrentState) {
		fCurrentState->UpdateViewCursor(fView, fMousePos);
		return true;
	}
	return false;
}


// #pragma mark -


// AttachedToView
void
TransformBox::AttachedToView(BView* view)
{
	view->Invalidate(Bounds().InsetByCopy(-INSET, -INSET));
}


// DetachedFromView
void
TransformBox::DetachedFromView(BView* view)
{
	view->Invalidate(Bounds().InsetByCopy(-INSET, -INSET));
}


// pragma mark -


// Update
void
TransformBox::Update(bool deep)
{
	// recalculate the points from the original box
	fLeftTop = fOriginalBox.LeftTop();
	fRightTop = fOriginalBox.RightTop();
	fLeftBottom = fOriginalBox.LeftBottom();
	fRightBottom = fOriginalBox.RightBottom();

	fPivot.x = (fLeftTop.x + fRightBottom.x) / 2.0;
	fPivot.y = (fLeftTop.y + fRightBottom.y) / 2.0;

	fPivot += fPivotOffset;

	// transform the points for display
	Transform(&fLeftTop);
	Transform(&fRightTop);
	Transform(&fLeftBottom);
	Transform(&fRightBottom);

	Transform(&fPivot);
}


// OffsetCenter
void
TransformBox::OffsetCenter(BPoint offset)
{
	if (offset != BPoint(0.0, 0.0)) {
		fPivotOffset += offset;
		Update(false);
	}
}


// Center
BPoint
TransformBox::Center() const
{
	return fPivot;
}


// SetBox
void
TransformBox::SetBox(BRect box)
{
	if (fOriginalBox != box) {
		fOriginalBox = box;
		Update(false);
	}
}


// FinishTransaction
Command*
TransformBox::FinishTransaction()
{
	Command* command = fCurrentCommand;
	if (fCurrentCommand) {
		fCurrentCommand->SetNewTransformation(Pivot(), Translation(),
			LocalRotation(), LocalXScale(), LocalYScale());
		fCurrentCommand = NULL;
	}
	return command;
}


// NudgeBy
void
TransformBox::NudgeBy(BPoint offset)
{
	if (!fNudging && !fCurrentCommand) {
		fCurrentCommand = MakeCommand("Move", 0/*MOVE*/);
		fNudging = true;
	}
	if (fNudging) {
		TranslateBy(offset);
	}
}


// FinishNudging
Command*
TransformBox::FinishNudging()
{
	fNudging = false;
	return FinishTransaction();
}


// TransformFromCanvas
void
TransformBox::TransformFromCanvas(BPoint& point) const
{
}


// TransformToCanvas
void
TransformBox::TransformToCanvas(BPoint& point) const
{
}


// ZoomLevel
float
TransformBox::ZoomLevel() const
{
	return 1.0;
}


// ViewSpaceRotation
double
TransformBox::ViewSpaceRotation() const
{
	// assume no inherited transformation
	return LocalRotation();
}


// #pragma mark -


// AddListener
bool
TransformBox::AddListener(TransformBoxListener* listener)
{
	if (listener && !fListeners.HasItem((void*)listener))
		return fListeners.AddItem((void*)listener);
	return false;
}


// RemoveListener
bool
TransformBox::RemoveListener(TransformBoxListener* listener)
{
	return fListeners.RemoveItem((void*)listener);
}


// #pragma mark -


// TODO: why another version?
// point_line_dist
float
point_line_dist(BPoint start, BPoint end, BPoint p, float radius)
{
	BRect r(min_c(start.x, end.x), min_c(start.y, end.y), max_c(start.x, end.x),
		max_c(start.y, end.y));
	r.InsetBy(-radius, -radius);
	if (r.Contains(p)) {
		return fabs(agg::calc_line_point_distance(start.x, start.y, end.x, end.y,
			p.x, p.y));
	}

	return min_c(point_point_distance(start, p), point_point_distance(end, p));
}


// _DragStateFor
//! where is expected in canvas view coordinates
DragState*
TransformBox::_DragStateFor(BPoint where, float canvasZoom)
{
	DragState* state = NULL;
	// convert to canvas zoom level
	//
	// the conversion is necessary, because the "hot regions"
	// around a point should be the same size no matter what
	// zoom level the canvas is displayed at

	float inset = INSET / canvasZoom;

	// priorities:
	// transformation center point has highest priority ?!?
	if (point_point_distance(where, fPivot) < inset)
		state = fOffsetCenterState;

	if (!state) {
		// next, the inner area of the box

		// for the following calculations
		// we can apply the inverse transformation to all points
		// this way we have to consider BRects only, not transformed polygons
		BPoint lt = fLeftTop;
		BPoint rb = fRightBottom;
		BPoint w = where;

		InverseTransform(&w);
		InverseTransform(&lt);
		InverseTransform(&rb);

		// next priority has the inside of the box
		BRect iR(lt, rb);
		float hInset = min_c(inset, max_c(0, (iR.Width() - inset) / 2.0));
		float vInset = min_c(inset, max_c(0, (iR.Height() - inset) / 2.0));

		iR.InsetBy(hInset, vInset);
		if (iR.Contains(w))
			state = fTranslateState;
	}

	if (!state) {
		// next priority have the corners
		float dLT = point_point_distance(fLeftTop, where);
		float dRT = point_point_distance(fRightTop, where);
		float dLB = point_point_distance(fLeftBottom, where);
		float dRB = point_point_distance(fRightBottom, where);
		float d = min4(dLT, dRT, dLB, dRB);
		if (d < inset) {
			if (d == dLT)
				state = fDragLTState;
			else if (d == dRT)
				state = fDragRTState;
			else if (d == dLB)
				state = fDragLBState;
			else if (d == dRB)
				state = fDragRBState;
		}
	}

	if (!state) {
		// next priority have the sides
		float dL = point_line_dist(fLeftTop, fLeftBottom, where, inset);
		float dR = point_line_dist(fRightTop, fRightBottom, where, inset);
		float dT = point_line_dist(fLeftTop, fRightTop, where, inset);
		float dB = point_line_dist(fLeftBottom, fRightBottom, where, inset);
		float d = min4(dL, dR, dT, dB);
		if (d < inset) {
			if (d == dL)
				state = fDragLState;
			else if (d == dR)
				state = fDragRState;
			else if (d == dT)
				state = fDragTState;
			else if (d == dB)
				state = fDragBState;
		}
	}

	if (!state) {
		BPoint lt = fLeftTop;
		BPoint rb = fRightBottom;
		BPoint w = where;

		InverseTransform(&w);
		InverseTransform(&lt);
		InverseTransform(&rb);

		// check inside of the box again
		BRect iR(lt, rb);
		if (iR.Contains(w)) {
			state = fTranslateState;
		} else {
			// last priority has the rotate state
			state = fRotateState;
		}
	}

	return state;
}


// _StrokeBWLine
void
TransformBox::_StrokeBWLine(BView* into, BPoint from, BPoint to) const
{
	// find out how to offset the second line optimally
	BPoint offset(0.0, 0.0);
	// first, do we have a more horizontal line or a more vertical line?
	float xDiff = to.x - from.x;
	float yDiff = to.y - from.y;
	if (fabs(xDiff) > fabs(yDiff)) {
		// horizontal
		if (xDiff > 0.0) {
			offset.y = -1.0;
		} else {
			offset.y = 1.0;
		}
	} else {
		// vertical
		if (yDiff < 0.0) {
			offset.x = -1.0;
		} else {
			offset.x = 1.0;
		}
	}
	// stroke two lines in high and low color of the view
	into->StrokeLine(from, to, B_SOLID_LOW);
	from += offset;
	to += offset;
	into->StrokeLine(from, to, B_SOLID_HIGH);
}


// _StrokeBWPoint
void
TransformBox::_StrokeBWPoint(BView* into, BPoint point, double angle) const
{
	double x = point.x;
	double y = point.y;

	double x1 = x;
	double y1 = y - 5.0;

	double x2 = x - 5.0;
	double y2 = y - 5.0;

	double x3 = x - 5.0;
	double y3 = y;

	agg::trans_affine m;

	double xOffset = -x;
	double yOffset = -y;

	agg::trans_affine_rotation r(angle * M_PI / 180.0);

	r.transform(&xOffset, &yOffset);
	xOffset = x + xOffset;
	yOffset = y + yOffset;

	m.multiply(r);
	m.multiply(agg::trans_affine_translation(xOffset, yOffset));

	m.transform(&x, &y);
	m.transform(&x1, &y1);
	m.transform(&x2, &y2);
	m.transform(&x3, &y3);

	BPoint p[4];
	p[0] = BPoint(x, y);
	p[1] = BPoint(x1, y1);
	p[2] = BPoint(x2, y2);
	p[3] = BPoint(x3, y3);

	into->FillPolygon(p, 4, B_SOLID_HIGH);

	into->StrokeLine(p[0], p[1], B_SOLID_LOW);
	into->StrokeLine(p[1], p[2], B_SOLID_LOW);
	into->StrokeLine(p[2], p[3], B_SOLID_LOW);
	into->StrokeLine(p[3], p[0], B_SOLID_LOW);
}


// #pragma mark -


// _NotifyDeleted
void
TransformBox::_NotifyDeleted() const
{
	BList listeners(fListeners);
	int32 count = listeners.CountItems();
	for (int32 i = 0; i < count; i++) {
		TransformBoxListener* listener
			= (TransformBoxListener*)listeners.ItemAtFast(i);
		listener->TransformBoxDeleted(this);
	}
}


// #pragma mark -


// _SetState
void
TransformBox::_SetState(DragState* state)
{
	if (state != fCurrentState) {
		fCurrentState = state;
		fCurrentState->UpdateViewCursor(fView, fMousePos);
	}
}

