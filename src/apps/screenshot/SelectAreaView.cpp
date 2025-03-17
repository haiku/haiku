/*
 * Copyright 2025, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *     Pawan Yerramilli <me@pawanyerramilli.com>
 */


#include "SelectAreaView.h"

#include <Application.h>
#include <Bitmap.h>
#include <ControlLook.h>
#include <Screen.h>
#include <Window.h>

#include "Cursor.h"
#include "GraphicsDefs.h"
#include "InterfaceDefs.h"
#include "Point.h"
#include "Rect.h"
#include "Utility.h"


SelectAreaView::SelectAreaView(BBitmap* frame)
	:
	BView(BScreen().Frame(), "", B_FOLLOW_NONE, B_WILL_DRAW | B_NAVIGABLE),
	fScreenShot(frame),
	fCursor(BCursor(B_CURSOR_ID_CROSS_HAIR)),
	fIsCurrentlyDragging(false),
	fStartCorner(B_ORIGIN),
	fEndCorner(B_ORIGIN),
	fResizable(false),
	fCurrentHandle(DRAG_NO_HANDLE),
	fMoveDelta(-1, -1)
{
}


void
SelectAreaView::AttachedToWindow()
{
	MakeFocus(true);

	BBitmap* darkenedShot = new BBitmap(Bounds(), fScreenShot->ColorSpace(), true);
	BView* darkenedView = new BView(darkenedShot->Bounds(), "", B_FOLLOW_NONE, 0);
	darkenedShot->AddChild(darkenedView);
	darkenedShot->Lock();
	darkenedView->DrawBitmap(fScreenShot);
	darkenedView->SetDrawingMode(B_OP_ALPHA);
	if (ui_color(B_PANEL_BACKGROUND_COLOR).IsDark()) {
		darkenedView->SetHighColor(255, 255, 255, 128);
		SetHighColor(255, 255, 255, 255);
	} else {
		darkenedView->SetHighColor(0, 0, 0, 128);
		SetHighColor(0, 0, 0, 0);
	}
	darkenedView->FillRect(Bounds());
	darkenedView->Sync();
	SetViewBitmap(darkenedShot);
	darkenedShot->RemoveChild(darkenedView);
	delete darkenedView;
	delete darkenedShot;
}


void
SelectAreaView::Draw(BRect updateRect)
{
	BRect frame = _CurrentFrame();
	SetViewCursor(&fCursor);
	DrawBitmap(fScreenShot, frame, frame);

	rgb_color currentLow = LowColor();
	SetLowColor(255, 0, 255, 255);
	StrokeRect(frame, B_MIXED_COLORS);
	if (fResizable && frame.Width() > 20 && frame.Height() > 20) {
		FillRect(_GetHandle(DRAG_TOP_LEFT), B_MIXED_COLORS);
		FillRect(_GetHandle(DRAG_TOP_RIGHT), B_MIXED_COLORS);
		FillRect(_GetHandle(DRAG_BOTTOM_LEFT), B_MIXED_COLORS);
		FillRect(_GetHandle(DRAG_BOTTOM_RIGHT), B_MIXED_COLORS);
	}
	SetLowColor(currentLow);
}


void
SelectAreaView::MessageReceived(BMessage* message)
{
	int32 clicks = 0;
	if (message->what == B_MOUSE_DOWN && message->FindInt32("clicks", &clicks) == B_OK
		&& clicks == 2) {
		_SaveSelection();
	} else {
		BView::MessageReceived(message);
	}
}


void
SelectAreaView::KeyDown(const char* bytes, int32 numBytes)
{
	if (bytes[0] == B_ESCAPE) {
		BMessage message(SS_SELECT_AREA_FRAME);
		message.AddRect("selectAreaFrame", BRect(0, 0, -1, -1));
		be_app->PostMessage(&message);
		Window()->Quit();
	} else if (bytes[0] == B_ENTER)
		_SaveSelection();
}


void
SelectAreaView::MouseDown(BPoint point)
{
	BPoint mouseLoc;
	uint32 buttons;
	GetMouse(&mouseLoc, &buttons);
	uint32 shiftDown = modifiers() & B_SHIFT_KEY;
	Handle handle = _FindSelectedHandle(point);

	if (fStartCorner == fEndCorner) {
		fResizable = (buttons & B_SECONDARY_MOUSE_BUTTON) || shiftDown;
		fIsCurrentlyDragging = true;
		fStartCorner = fEndCorner = point;
	} else if (fResizable && handle != DRAG_NO_HANDLE) {
		fCurrentHandle = handle;
	} else if (fResizable && _CurrentFrame().Contains(point)) {
		fMoveDelta = point;
		fCursor = B_CURSOR_ID_GRABBING;
	}

	Invalidate();
}


void
SelectAreaView::MouseMoved(BPoint point, uint32 transit, const BMessage* message)
{
	if (fIsCurrentlyDragging && point != fEndCorner)
		fEndCorner = point;

	Handle hoveredHandle = fCurrentHandle == DRAG_NO_HANDLE ?
		_FindSelectedHandle(point) : fCurrentHandle;
	if (fResizable && (hoveredHandle != DRAG_NO_HANDLE)) {
		switch(hoveredHandle) {
			case DRAG_TOP_LEFT:
				fCursor = B_CURSOR_ID_RESIZE_NORTH_WEST;
				break;
			case DRAG_TOP_RIGHT:
				fCursor = B_CURSOR_ID_RESIZE_NORTH_EAST;
				break;
			case DRAG_BOTTOM_LEFT:
				fCursor = B_CURSOR_ID_RESIZE_SOUTH_WEST;
				break;
			case DRAG_BOTTOM_RIGHT:
				fCursor = B_CURSOR_ID_RESIZE_SOUTH_EAST;
			default:
				break;
		}
	} else if (fResizable && !_CurrentFrame().Contains(point))
		fCursor = B_CURSOR_ID_SYSTEM_DEFAULT;
	else if (fMoveDelta != BPoint(-1, -1))
		fCursor = B_CURSOR_ID_GRABBING;
	else if (fResizable && _CurrentFrame().Contains(point))
		fCursor = B_CURSOR_ID_GRAB;
	else
		fCursor = B_CURSOR_ID_CROSS_HAIR;

	if (fCurrentHandle != DRAG_NO_HANDLE) {
		switch(fCurrentHandle) {
			case DRAG_TOP_LEFT:
				fStartCorner = point;
				break;
			case DRAG_TOP_RIGHT:
				fStartCorner.y = point.y;
				fEndCorner.x = point.x;
				break;
			case DRAG_BOTTOM_LEFT:
				fStartCorner.x = point.x;
				fEndCorner.y = point.y;
				break;
			case DRAG_BOTTOM_RIGHT:
				fEndCorner = point;
			default:
				break;
		}
	}

	if (fMoveDelta != BPoint(-1, -1)) {
		fStartCorner += (point - fMoveDelta);
		fEndCorner += (point - fMoveDelta);
		fStartCorner.ConstrainTo(Window()->Frame());
		fEndCorner.ConstrainTo(Window()->Frame());
		fMoveDelta = point;
	}

	Invalidate();
}


void
SelectAreaView::MouseUp(BPoint point)
{
	if (!fResizable && fIsCurrentlyDragging && fStartCorner != fEndCorner) {
		_SaveSelection();
	} else if (fResizable) {
		fIsCurrentlyDragging = false;
		BRect frame = _CurrentFrame();
		fStartCorner = frame.LeftTop();
		fEndCorner = frame.RightBottom();
		fCurrentHandle = DRAG_NO_HANDLE;
		if (fMoveDelta != BPoint(-1, -1))
			fCursor = B_CURSOR_ID_GRAB;
		fMoveDelta = BPoint(-1, -1);
		Invalidate();
	}
}


void
SelectAreaView::_SaveSelection()
{
	BMessage message(SS_SELECT_AREA_FRAME);
	message.AddRect("selectAreaFrame", _CurrentFrame());
	be_app->PostMessage(&message);
	Window()->Quit();
}


BRect
SelectAreaView::_CurrentFrame()
{
	BPoint topLeft = BPoint(min_c(fStartCorner.x, fEndCorner.x),
		min_c(fStartCorner.y, fEndCorner.y));
	BPoint bottomRight = BPoint(max_c(fStartCorner.x, fEndCorner.x),
		max_c(fStartCorner.y, fEndCorner.y));
	return BRect(topLeft, bottomRight);
}


Handle
SelectAreaView::_FindSelectedHandle(BPoint point)
{
	for (int i = DRAG_TOP_LEFT; i < DRAG_NO_HANDLE; i++) {
		if (_GetHandle((Handle)i).Contains(point))
			return (Handle)i;
	}

	return DRAG_NO_HANDLE;
}


BRect
SelectAreaView::_GetHandle(Handle handle)
{
	BRect frame = _CurrentFrame();
	int size = be_control_look->ComposeIconSize(8).IntegerWidth();
	switch (handle) {
		case DRAG_TOP_LEFT:
			return BRect(frame.LeftTop(), frame.LeftTop() + BPoint(size, size));
		case DRAG_TOP_RIGHT:
			return BRect(frame.RightTop() - BPoint(size, 0), frame.RightTop() + BPoint(0, size));
		case DRAG_BOTTOM_LEFT:
			return BRect(frame.LeftBottom() - BPoint(0, size), frame.LeftBottom() + BPoint(size, 0));
		case DRAG_BOTTOM_RIGHT:
			return BRect(frame.RightBottom() - BPoint(size, size), frame.RightBottom());
		default:
			return BRect(0, 0, -1, -1);
	}
}
