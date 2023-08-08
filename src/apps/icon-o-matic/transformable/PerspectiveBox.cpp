/*
 * Copyright 2006-2009, 2023, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
 */

#include "PerspectiveBox.h"

#include <stdio.h>

#include <agg_trans_affine.h>
#include <agg_math.h>

#include <View.h>

#include "CanvasView.h"
#include "StateView.h"
#include "support.h"
#include "PerspectiveBoxStates.h"
#include "PerspectiveCommand.h"
#include "PerspectiveTransformer.h"


#define INSET 8.0


using std::nothrow;
using namespace PerspectiveBoxStates;


PerspectiveBox::PerspectiveBox(CanvasView* view,
		PerspectiveTransformer* parent)
	:
	Manipulator(NULL),

	fLeftTop(parent->LeftTop()),
	fRightTop(parent->RightTop()),
	fLeftBottom(parent->LeftBottom()),
	fRightBottom(parent->RightBottom()),

	fCurrentCommand(NULL),
	fCurrentState(NULL),

	fDragging(false),
	fMousePos(-10000.0, -10000.0),
	fModifiers(0),

	fPreviousBox(LONG_MAX, LONG_MAX, LONG_MIN, LONG_MIN),

	fCanvasView(view),
	fPerspective(parent),

	fDragLTState(new DragCornerState(this, &fLeftTop)),
	fDragRTState(new DragCornerState(this, &fRightTop)),
	fDragLBState(new DragCornerState(this, &fLeftBottom)),
	fDragRBState(new DragCornerState(this, &fRightBottom))
{
}


PerspectiveBox::~PerspectiveBox()
{
	_NotifyDeleted();

	delete fCurrentCommand;
	delete fDragLTState;
	delete fDragRTState;
	delete fDragLBState;
	delete fDragRBState;
}


void
PerspectiveBox::Draw(BView* into, BRect updateRect)
{
	// convert to canvas view coordinates
	BPoint lt = fLeftTop;
	BPoint rt = fRightTop;
	BPoint lb = fLeftBottom;
	BPoint rb = fRightBottom;

	fCanvasView->ConvertFromCanvas(&lt);
	fCanvasView->ConvertFromCanvas(&rt);
	fCanvasView->ConvertFromCanvas(&lb);
	fCanvasView->ConvertFromCanvas(&rb);

	into->SetDrawingMode(B_OP_COPY);
	into->SetHighColor(255, 255, 255, 255);
	into->SetLowColor(0, 0, 0, 255);

	_StrokeBWLine(into, lt, rt);
	_StrokeBWLine(into, rt, rb);
	_StrokeBWLine(into, rb, lb);
	_StrokeBWLine(into, lb, lt);

	_StrokeBWPoint(into, lt, 0.0);
	_StrokeBWPoint(into, rt, 90.0);
	_StrokeBWPoint(into, rb, 180.0);
	_StrokeBWPoint(into, lb, 270.0);
}


// #pragma mark -


bool
PerspectiveBox::MouseDown(BPoint where)
{
	fCanvasView->FilterMouse(&where);
	fCanvasView->ConvertToCanvas(&where);

	fDragging = true;
	if (fCurrentState) {
		fCurrentState->SetOrigin(where);

		delete fCurrentCommand;
		fCurrentCommand = new (nothrow) PerspectiveCommand(this, fPerspective,
			fPerspective->LeftTop(), fPerspective->RightTop(),
			fPerspective->LeftBottom(), fPerspective->RightBottom());
	}

	return true;
}


void
PerspectiveBox::MouseMoved(BPoint where)
{
	fCanvasView->FilterMouse(&where);
	fCanvasView->ConvertToCanvas(&where);

	if (fMousePos != where) {
		fMousePos = where;
		if (fCurrentState) {
			fCurrentState->DragTo(fMousePos, fModifiers);
			fCurrentState->UpdateViewCursor(fCanvasView, fMousePos);
		}
	}
}


Command*
PerspectiveBox::MouseUp()
{
	fDragging = false;
	return FinishTransaction();
}


bool
PerspectiveBox::MouseOver(BPoint where)
{
	fCanvasView->ConvertToCanvas(&where);

	fMousePos = where;
	fCurrentState = _DragStateFor(where, fCanvasView->ZoomLevel());

	if (fCurrentState) {
		fCurrentState->UpdateViewCursor(fCanvasView, fMousePos);
		return true;
	}

	return false;
}


// #pragma mark -


BRect
PerspectiveBox::Bounds()
{
	// convert from canvas view coordinates
	BPoint lt = fLeftTop;
	BPoint rt = fRightTop;
	BPoint lb = fLeftBottom;
	BPoint rb = fRightBottom;

	fCanvasView->ConvertFromCanvas(&lt);
	fCanvasView->ConvertFromCanvas(&rt);
	fCanvasView->ConvertFromCanvas(&lb);
	fCanvasView->ConvertFromCanvas(&rb);

	BRect bounds;
	bounds.left = min4(lt.x, rt.x, lb.x, rb.x);
	bounds.top = min4(lt.y, rt.y, lb.y, rb.y);
	bounds.right = max4(lt.x, rt.x, lb.x, rb.x);
	bounds.bottom = max4(lt.y, rt.y, lb.y, rb.y);
	return bounds;
}


BRect
PerspectiveBox::TrackingBounds(BView* withinView)
{
	return withinView->Bounds();
}


// #pragma mark -


void
PerspectiveBox::ModifiersChanged(uint32 modifiers)
{
	fModifiers = modifiers;
	if (fDragging && fCurrentState) {
		fCurrentState->DragTo(fMousePos, fModifiers);
	}
}


bool
PerspectiveBox::UpdateCursor()
{
	if (fCurrentState) {
		fCurrentState->UpdateViewCursor(fCanvasView, fMousePos);
		return true;
	}
	return false;
}


// #pragma mark -


void
PerspectiveBox::AttachedToView(BView* view)
{
	view->Invalidate(Bounds().InsetByCopy(-INSET, -INSET));
}


void
PerspectiveBox::DetachedFromView(BView* view)
{
	view->Invalidate(Bounds().InsetByCopy(-INSET, -INSET));
}


// pragma mark -


void
PerspectiveBox::ObjectChanged(const Observable* object)
{
}


// pragma mark -


void
PerspectiveBox::TransformTo(
	BPoint leftTop, BPoint rightTop, BPoint leftBottom, BPoint rightBottom)
{
	if (fLeftTop == leftTop
		&& fRightTop == rightTop
		&& fLeftBottom == leftBottom
		&& fRightBottom == rightBottom)
		return;

	fLeftTop = leftTop;
	fRightTop = rightTop;
	fLeftBottom = leftBottom;
	fRightBottom = rightBottom;

	Update();
}


void
PerspectiveBox::Update(bool deep)
{
	BRect r = Bounds();
	BRect dirty(r | fPreviousBox);
	dirty.InsetBy(-INSET, -INSET);
	fCanvasView->Invalidate(dirty);
	fPreviousBox = r;

	if (deep)
		fPerspective->TransformTo(fLeftTop, fRightTop, fLeftBottom, fRightBottom);
}


Command*
PerspectiveBox::FinishTransaction()
{
	Command* command = fCurrentCommand;
	if (fCurrentCommand) {
		fCurrentCommand->SetNewPerspective(
			fPerspective->LeftTop(), fPerspective->RightTop(),
			fPerspective->LeftBottom(), fPerspective->RightBottom());
		fCurrentCommand = NULL;
	}
	return command;
}


// #pragma mark -


bool
PerspectiveBox::AddListener(PerspectiveBoxListener* listener)
{
	if (listener && !fListeners.HasItem((void*)listener))
		return fListeners.AddItem((void*)listener);
	return false;
}


bool
PerspectiveBox::RemoveListener(PerspectiveBoxListener* listener)
{
	return fListeners.RemoveItem((void*)listener);
}


// #pragma mark -


void
PerspectiveBox::_NotifyDeleted() const
{
	BList listeners(fListeners);
	int32 count = listeners.CountItems();
	for (int32 i = 0; i < count; i++) {
		PerspectiveBoxListener* listener
			= (PerspectiveBoxListener*)listeners.ItemAtFast(i);
		listener->PerspectiveBoxDeleted(this);
	}
}


//! where is expected in canvas view coordinates
DragState*
PerspectiveBox::_DragStateFor(BPoint where, float canvasZoom)
{
	DragState* state = NULL;

	// convert to canvas zoom level
	//
	// the conversion is necessary, because the "hot regions"
	// around a point should be the same size no matter what
	// zoom level the canvas is displayed at
	float inset = INSET / canvasZoom;

	// check if the cursor is over the corners
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

	return state;
}


void
PerspectiveBox::_StrokeBWLine(BView* into, BPoint from, BPoint to) const
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


void
PerspectiveBox::_StrokeBWPoint(BView* into, BPoint point, double angle) const
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
