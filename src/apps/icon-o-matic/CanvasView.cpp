/*
 * Copyright 2006-2007, 2011, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "CanvasView.h"

#include <Bitmap.h>
#include <Cursor.h>
#include <Message.h>
#include <Region.h>
#include <Window.h>

#include <stdio.h>

#include "cursors.h"
#include "ui_defines.h"

#include "CommandStack.h"
#include "IconRenderer.h"


using std::nothrow;


CanvasView::CanvasView(BRect frame)
	:
	StateView(frame, "canvas view", B_FOLLOW_ALL,
		B_WILL_DRAW | B_FRAME_EVENTS),
	fBitmap(new BBitmap(BRect(0, 0, 63, 63), 0, B_RGB32)),
	fBackground(new BBitmap(BRect(0, 0, 63, 63), 0, B_RGB32)),
	fIcon(NULL),
	fRenderer(new IconRenderer(fBitmap)),
	fDirtyIconArea(fBitmap->Bounds()),

	fCanvasOrigin(0.0, 0.0),
	fZoomLevel(8.0),

	fSpaceHeldDown(false),
	fInScrollTo(false),
	fScrollTracking(false),
	fScrollTrackingStart(0.0, 0.0),

	fMouseFilterMode(SNAPPING_OFF)
{
	_MakeBackground();
	fRenderer->SetBackground(fBackground);
}


CanvasView::~CanvasView()
{
	SetIcon(NULL);
	delete fRenderer;
	delete fBitmap;
	delete fBackground;
}


// #pragma mark -


void
CanvasView::AttachedToWindow()
{
	StateView::AttachedToWindow();

	SetViewColor(B_TRANSPARENT_COLOR);
	SetLowColor(kStripesHigh);
	SetHighColor(kStripesLow);

	// init data rect for scrolling and center bitmap in the view
	BRect dataRect = _LayoutCanvas();
	SetDataRect(dataRect);
	BRect bounds(Bounds());
	BPoint dataRectCenter((dataRect.left + dataRect.right) / 2,
		(dataRect.top + dataRect.bottom) / 2);
	BPoint boundsCenter((bounds.left + bounds.right) / 2,
		(bounds.top + bounds.bottom) / 2);
	BPoint offset = ScrollOffset();
	offset.x = roundf(offset.x + dataRectCenter.x - boundsCenter.x);
	offset.y = roundf(offset.y + dataRectCenter.y - boundsCenter.y);
	SetScrollOffset(offset);
}


void
CanvasView::FrameResized(float width, float height)
{
	// keep canvas centered
	BPoint oldCanvasOrigin = fCanvasOrigin;
	SetDataRect(_LayoutCanvas());
	if (oldCanvasOrigin != fCanvasOrigin)
		Invalidate();
}


void
CanvasView::Draw(BRect updateRect)
{
	_DrawInto(this, updateRect);
}


// #pragma mark -


void
CanvasView::MouseDown(BPoint where)
{
	if (!IsFocus())
		MakeFocus(true);

	int32 buttons;
	if (Window()->CurrentMessage()->FindInt32("buttons", &buttons) < B_OK)
		buttons = 0;

	// handle clicks of the third mouse button ourselves (panning),
	// otherwise have StateView handle it (normal clicks)
	if (fSpaceHeldDown || (buttons & B_TERTIARY_MOUSE_BUTTON) != 0) {
		// switch into scrolling mode and update cursor
		fScrollTracking = true;
		where.x = roundf(where.x);
		where.y = roundf(where.y);
		fScrollOffsetStart = ScrollOffset();
		fScrollTrackingStart = where - fScrollOffsetStart;
		_UpdateToolCursor();
		SetMouseEventMask(B_POINTER_EVENTS,
			B_LOCK_WINDOW_FOCUS | B_SUSPEND_VIEW_FOCUS);
	} else {
		StateView::MouseDown(where);
	}
}


void
CanvasView::MouseUp(BPoint where)
{
	if (fScrollTracking) {
		// stop scroll tracking and update cursor
		fScrollTracking = false;
		_UpdateToolCursor();
		// update StateView mouse position
		uint32 transit = Bounds().Contains(where) ?
			B_INSIDE_VIEW : B_OUTSIDE_VIEW;
		StateView::MouseMoved(where, transit, NULL);
	} else {
		StateView::MouseUp(where);
	}
}


void
CanvasView::MouseMoved(BPoint where, uint32 transit, const BMessage* dragMessage)
{
	if (fScrollTracking) {
		uint32 buttons;
		GetMouse(&where, &buttons, false);
		if (!buttons) {
			MouseUp(where);
			return;
		}
		where.x = roundf(where.x);
		where.y = roundf(where.y);
		where -= ScrollOffset();
		BPoint offset = where - fScrollTrackingStart;
		SetScrollOffset(fScrollOffsetStart - offset);
	} else {
		// normal mouse movement handled by StateView
		if (!fSpaceHeldDown)
			StateView::MouseMoved(where, transit, dragMessage);
	}
}


void
CanvasView::FilterMouse(BPoint* where) const
{
	switch (fMouseFilterMode) {

		case SNAPPING_64:
			ConvertToCanvas(where);
			where->x = floorf(where->x + 0.5);
			where->y = floorf(where->y + 0.5);
			ConvertFromCanvas(where);
			break;

		case SNAPPING_32:
			ConvertToCanvas(where);
			where->x /= 2.0;
			where->y /= 2.0;
			where->x = floorf(where->x + 0.5);
			where->y = floorf(where->y + 0.5);
			where->x *= 2.0;
			where->y *= 2.0;
			ConvertFromCanvas(where);
			break;

		case SNAPPING_16:
			ConvertToCanvas(where);
			where->x /= 4.0;
			where->y /= 4.0;
			where->x = floorf(where->x + 0.5);
			where->y = floorf(where->y + 0.5);
			where->x *= 4.0;
			where->y *= 4.0;
			ConvertFromCanvas(where);
			break;

		case SNAPPING_OFF:
		default:
			break;
	}
}


bool
CanvasView::MouseWheelChanged(BPoint where, float x, float y)
{
	if (!Bounds().Contains(where))
		return false;

	if (y > 0.0) {
		_SetZoom(_NextZoomOutLevel(fZoomLevel), true);
		return true;
	} else if (y < 0.0) {
		_SetZoom(_NextZoomInLevel(fZoomLevel), true);
		return true;
	}
	return false;
}


// #pragma mark -


void
CanvasView::SetScrollOffset(BPoint newOffset)
{
	if (fInScrollTo)
		return;

	fInScrollTo = true;

	newOffset = ValidScrollOffsetFor(newOffset);
	if (!fScrollTracking) {
		BPoint mouseOffset = newOffset - ScrollOffset();
		MouseMoved(fMouseInfo.position + mouseOffset, fMouseInfo.transit,
			NULL);
	}

	Scrollable::SetScrollOffset(newOffset);

	fInScrollTo = false;
}


void
CanvasView::ScrollOffsetChanged(BPoint oldOffset, BPoint newOffset)
{
	BPoint offset = newOffset - oldOffset;

	if (offset == B_ORIGIN) {
		// prevent circular code (MouseMoved might call ScrollBy...)
		return;
	}

	ScrollBy(offset.x, offset.y);
}


void
CanvasView::VisibleSizeChanged(float oldWidth, float oldHeight, float newWidth,
	float newHeight)
{
	BRect dataRect(_LayoutCanvas());
	SetDataRect(dataRect);
}


// #pragma mark -


void
CanvasView::AreaInvalidated(const BRect& area)
{
	if (fDirtyIconArea.Contains(area))
		return;

	fDirtyIconArea = fDirtyIconArea | area;

	BRect viewArea(area);
	ConvertFromCanvas(&viewArea);
	Invalidate(viewArea);
}


// #pragma mark -


void
CanvasView::SetIcon(Icon* icon)
{
	if (fIcon == icon)
		return;

	if (fIcon)
		fIcon->RemoveListener(this);

	fIcon = icon;
	fRenderer->SetIcon(icon);

	if (fIcon)
		fIcon->AddListener(this);
}


void
CanvasView::SetMouseFilterMode(uint32 mode)
{
	if (fMouseFilterMode == mode)
		return;

	fMouseFilterMode = mode;
	Invalidate(_CanvasRect());
}


void
CanvasView::ConvertFromCanvas(BPoint* point) const
{
	point->x = point->x * fZoomLevel + fCanvasOrigin.x;
	point->y = point->y * fZoomLevel + fCanvasOrigin.y;
}


void
CanvasView::ConvertToCanvas(BPoint* point) const
{
	point->x = (point->x - fCanvasOrigin.x) / fZoomLevel;
	point->y = (point->y - fCanvasOrigin.y) / fZoomLevel;
}


void
CanvasView::ConvertFromCanvas(BRect* r) const
{
	r->left = r->left * fZoomLevel + fCanvasOrigin.x;
	r->top = r->top * fZoomLevel + fCanvasOrigin.y;
	r->right++;
	r->bottom++;
	r->right = r->right * fZoomLevel + fCanvasOrigin.x;
	r->bottom = r->bottom * fZoomLevel + fCanvasOrigin.y;
	r->right--;
	r->bottom--;
}


void
CanvasView::ConvertToCanvas(BRect* r) const
{
	r->left = (r->left - fCanvasOrigin.x) / fZoomLevel;
	r->top = (r->top - fCanvasOrigin.y) / fZoomLevel;
	r->right = (r->right - fCanvasOrigin.x) / fZoomLevel;
	r->bottom = (r->bottom - fCanvasOrigin.y) / fZoomLevel;
}


// #pragma mark -


bool
CanvasView::_HandleKeyDown(uint32 key, uint32 modifiers)
{
	switch (key) {
		case 'z':
		case 'y':
			if (modifiers & B_SHIFT_KEY)
				CommandStack()->Redo();
			else
				CommandStack()->Undo();
			break;

		case '+':
			_SetZoom(_NextZoomInLevel(fZoomLevel));
			break;
		case '-':
			_SetZoom(_NextZoomOutLevel(fZoomLevel));
			break;

		case B_SPACE:
			fSpaceHeldDown = true;
			_UpdateToolCursor();
			break;

		default:
			return StateView::_HandleKeyDown(key, modifiers);
	}

	return true;
}


bool
CanvasView::_HandleKeyUp(uint32 key, uint32 modifiers)
{
	switch (key) {
		case B_SPACE:
			fSpaceHeldDown = false;
			_UpdateToolCursor();
			break;

		default:
			return StateView::_HandleKeyUp(key, modifiers);
	}

	return true;
}


BRect
CanvasView::_CanvasRect() const
{
	BRect r;
	if (fBitmap == NULL)
		return r;
	r = fBitmap->Bounds();
	ConvertFromCanvas(&r);
	return r;
}


void
CanvasView::_DrawInto(BView* view, BRect updateRect)
{
	if (fDirtyIconArea.IsValid()) {
		fRenderer->Render(fDirtyIconArea);
		fDirtyIconArea.Set(LONG_MAX, LONG_MAX, LONG_MIN, LONG_MIN);
	}

	// icon
	BRect canvas(_CanvasRect());
	view->DrawBitmap(fBitmap, fBitmap->Bounds(), canvas);

	// grid
	int32 gridLines = 0;
	int32 scale = 1;
	switch (fMouseFilterMode) {
		case SNAPPING_64:
			gridLines = 63;
			break;
		case SNAPPING_32:
			gridLines = 31;
			scale = 2;
			break;
		case SNAPPING_16:
			gridLines = 15;
			scale = 4;
			break;
		case SNAPPING_OFF:
		default:
			break;
	}
	view->SetDrawingMode(B_OP_BLEND);
	for (int32 i = 1; i <= gridLines; i++) {
		BPoint cross(i * scale, i * scale);
		ConvertFromCanvas(&cross);
		view->StrokeLine(BPoint(canvas.left, cross.y),
						 BPoint(canvas.right, cross.y));
		view->StrokeLine(BPoint(cross.x, canvas.top),
						 BPoint(cross.x, canvas.bottom));
	}
	view->SetDrawingMode(B_OP_COPY);

	// outside icon
	BRegion outside(Bounds() & updateRect);
	outside.Exclude(canvas);
	view->FillRegion(&outside, kStripes);

	StateView::Draw(view, updateRect);
}


void
CanvasView::_MakeBackground()
{
	uint8* row = (uint8*)fBackground->Bits();
	uint32 bpr = fBackground->BytesPerRow();
	uint32 width = fBackground->Bounds().IntegerWidth() + 1;
	uint32 height = fBackground->Bounds().IntegerHeight() + 1;

	const GammaTable& lut = fRenderer->GammaTable();
	uint8 redLow = lut.dir(kAlphaLow.red);
	uint8 greenLow = lut.dir(kAlphaLow.blue);
	uint8 blueLow = lut.dir(kAlphaLow.green);
	uint8 redHigh = lut.dir(kAlphaHigh.red);
	uint8 greenHigh = lut.dir(kAlphaHigh.blue);
	uint8 blueHigh = lut.dir(kAlphaHigh.green);

	for (uint32 y = 0; y < height; y++) {
		uint8* p = row;
		for (uint32 x = 0; x < width; x++) {
			p[3] = 255;
			if (x % 8 >= 4) {
				if (y % 8 >= 4) {
					p[0] = blueLow;
					p[1] = greenLow;
					p[2] = redLow;
				} else {
					p[0] = blueHigh;
					p[1] = greenHigh;
					p[2] = redHigh;
				}
			} else {
				if (y % 8 >= 4) {
					p[0] = blueHigh;
					p[1] = greenHigh;
					p[2] = redHigh;
				} else {
					p[0] = blueLow;
					p[1] = greenLow;
					p[2] = redLow;
				}
			}
			p += 4;
		}
		row += bpr;
	}
}


void
CanvasView::_UpdateToolCursor()
{
	if (fIcon) {
		if (fScrollTracking || fSpaceHeldDown) {
			// indicate scrolling mode
			const uchar* cursorData = fScrollTracking ? kGrabCursor : kHandCursor;
			BCursor cursor(cursorData);
			SetViewCursor(&cursor, true);
		} else {
			// pass on to current state of StateView
			UpdateStateCursor();
		}
	} else {
		BCursor cursor(kStopCursor);
		SetViewCursor(&cursor, true);
	}
}


// #pragma mark -


double
CanvasView::_NextZoomInLevel(double zoom) const
{
	if (zoom < 1)
		return 1;
	if (zoom < 1.5)
		return 1.5;
	if (zoom < 2)
		return 2;
	if (zoom < 3)
		return 3;
	if (zoom < 4)
		return 4;
	if (zoom < 6)
		return 6;
	if (zoom < 8)
		return 8;
	if (zoom < 16)
		return 16;
	if (zoom < 32)
		return 32;
	return 64;
}


double
CanvasView::_NextZoomOutLevel(double zoom) const
{
	if (zoom > 32)
		return 32;
	if (zoom > 16)
		return 16;
	if (zoom > 8)
		return 8;
	if (zoom > 6)
		return 6;
	if (zoom > 4)
		return 4;
	if (zoom > 3)
		return 3;
	if (zoom > 2)
		return 2;
	if (zoom > 1.5)
		return 1.5;
	return 1;
}


void
CanvasView::_SetZoom(double zoomLevel, bool mouseIsAnchor)
{
	if (fZoomLevel == zoomLevel)
		return;

	BPoint anchor;
	if (mouseIsAnchor) {
		// zoom into mouse position
		anchor = MouseInfo()->position;
	} else {
		// zoom into center of view
		BRect bounds(Bounds());
		anchor.x = (bounds.left + bounds.right + 1) / 2.0;
		anchor.y = (bounds.top + bounds.bottom + 1) / 2.0;
	}

	BPoint canvasAnchor = anchor;
	ConvertToCanvas(&canvasAnchor);

	fZoomLevel = zoomLevel;
	BRect dataRect = _LayoutCanvas();

	ConvertFromCanvas(&canvasAnchor);

	BPoint offset = ScrollOffset();
	offset.x = roundf(offset.x + canvasAnchor.x - anchor.x);
	offset.y = roundf(offset.y + canvasAnchor.y - anchor.y);

	Invalidate();

	SetDataRectAndScrollOffset(dataRect, offset);
}


BRect
CanvasView::_LayoutCanvas()
{
	// size of zoomed bitmap
	BRect r(_CanvasRect());
	r.OffsetTo(B_ORIGIN);

	// ask current view state to extend size
	// TODO: Ask StateViewState to extend bounds...
	BRect stateBounds = r; //ViewStateBounds();

	// resize for empty area around bitmap
	// (the size we want, but might still be much smaller than view)
	r.InsetBy(-50, -50);

	// center data rect in bounds
	BRect bounds(Bounds());
	if (bounds.Width() > r.Width())
		r.InsetBy(-ceilf((bounds.Width() - r.Width()) / 2), 0);
	if (bounds.Height() > r.Height())
		r.InsetBy(0, -ceilf((bounds.Height() - r.Height()) / 2));

	if (stateBounds.IsValid()) {
		stateBounds.InsetBy(-20, -20);
		r = r | stateBounds;
	}

	return r;
}

