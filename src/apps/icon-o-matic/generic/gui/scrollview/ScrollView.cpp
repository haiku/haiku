/*
 * Copyright 2006-2009, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "ScrollView.h"

#include <algorithm>
#include <stdio.h>
#include <string.h>

#include <Bitmap.h>
#ifdef __HAIKU__
#  include <LayoutUtils.h>
#endif
#include <Message.h>
#include <ScrollBar.h>
#include <Window.h>

#include "Scrollable.h"
#include "ScrollCornerBitmaps.h"

using namespace std;

// #pragma mark - InternalScrollBar

class InternalScrollBar : public BScrollBar {
 public:
								InternalScrollBar(ScrollView* scrollView,
												  BRect frame,
												  orientation posture);
	virtual						~InternalScrollBar();

	virtual	void				ValueChanged(float value);

	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseUp(BPoint where);

 private:
	ScrollView*					fScrollView;
};

// constructor
InternalScrollBar::InternalScrollBar(ScrollView* scrollView, BRect frame,
									 orientation posture)
	: BScrollBar(frame, NULL, NULL, 0, 0, posture),
	  fScrollView(scrollView)
{
}

// destructor
InternalScrollBar::~InternalScrollBar()
{
}

// ValueChanged
void
InternalScrollBar::ValueChanged(float value)
{
	// Notify our parent scroll view. Note: the value already has changed,
	// so that we can't check, if it really has changed.
	if (fScrollView)
		fScrollView->_ScrollValueChanged(this, value);
}

// MouseDown
void
InternalScrollBar::MouseDown(BPoint where)
{
	if (fScrollView)
		fScrollView->_SetScrolling(true);
	BScrollBar::MouseDown(where);
}

// MouseUp
void
InternalScrollBar::MouseUp(BPoint where)
{
	BScrollBar::MouseUp(where);
	if (fScrollView)
		fScrollView->_SetScrolling(false);
}



// #pragma mark -ScrollCorner

class ScrollCorner : public BView {
 public:
								ScrollCorner(ScrollView* scrollView);
	virtual						~ScrollCorner();

	virtual	void				MouseDown(BPoint point);
	virtual	void				MouseUp(BPoint point);
	virtual	void				MouseMoved(BPoint point, uint32 transit,
										   const BMessage* message);

	virtual	void				Draw(BRect updateRect);
	virtual	void				WindowActivated(bool active);

			void				SetActive(bool active);
	inline	bool				IsActive() const
									{ return fState & STATE_ACTIVE; }

 private:
			ScrollView*			fScrollView;
			uint32				fState;
			BPoint				fStartPoint;
			BPoint				fStartScrollOffset;
			BBitmap*			fBitmaps[3];

	inline	bool				IsEnabled() const
									{ return ((fState & STATE_ENABLED) ==
											  STATE_ENABLED); }

			void				SetDragging(bool dragging);
	inline	bool				IsDragging() const
									{ return (fState & STATE_DRAGGING); }

	enum {
		STATE_DRAGGING			= 0x01,
		STATE_WINDOW_ACTIVE		= 0x02,
		STATE_ACTIVE			= 0x04,
		STATE_ENABLED			= STATE_WINDOW_ACTIVE | STATE_ACTIVE,
	};
};

// constructor
ScrollCorner::ScrollCorner(ScrollView* scrollView)
	: BView(BRect(0.0, 0.0, B_V_SCROLL_BAR_WIDTH - 1.0f, B_H_SCROLL_BAR_HEIGHT - 1.0f), NULL,
			0, B_WILL_DRAW),
	  fScrollView(scrollView),
	  fState(0),
	  fStartPoint(0, 0),
	  fStartScrollOffset(0, 0)
{
	SetViewColor(B_TRANSPARENT_32_BIT);

	fBitmaps[0] = new BBitmap(BRect(0.0f, 0.0f, sBitmapWidth - 1,
		sBitmapHeight - 1), sColorSpace);
	char* bits = (char*)fBitmaps[0]->Bits();
	int32 bpr = fBitmaps[0]->BytesPerRow();
	for (int i = 0; i < sBitmapHeight; i++, bits += bpr) {
		memcpy(bits, &sScrollCornerNormalBits[i * sBitmapHeight * 4],
			sBitmapWidth * 4);
	}

	fBitmaps[1] = new BBitmap(BRect(0.0f, 0.0f, sBitmapWidth - 1,
		sBitmapHeight - 1), sColorSpace);
	bits = (char*)fBitmaps[1]->Bits();
	bpr = fBitmaps[1]->BytesPerRow();
	for (int i = 0; i < sBitmapHeight; i++, bits += bpr) {
		memcpy(bits, &sScrollCornerPushedBits[i * sBitmapHeight * 4],
			sBitmapWidth * 4);
	}

	fBitmaps[2] = new BBitmap(BRect(0.0f, 0.0f, sBitmapWidth - 1,
		sBitmapHeight - 1), sColorSpace);
	bits = (char*)fBitmaps[2]->Bits();
	bpr = fBitmaps[2]->BytesPerRow();
	for (int i = 0; i < sBitmapHeight; i++, bits += bpr) {
		memcpy(bits, &sScrollCornerDisabledBits[i * sBitmapHeight * 4],
			sBitmapWidth * 4);
	}
}

// destructor
ScrollCorner::~ScrollCorner()
{
	for (int i = 0; i < 3; i++)
		delete fBitmaps[i];
}

// MouseDown
void
ScrollCorner::MouseDown(BPoint point)
{
	BView::MouseDown(point);
	uint32 buttons = 0;
	Window()->CurrentMessage()->FindInt32("buttons", (int32 *)&buttons);
	if (buttons & B_PRIMARY_MOUSE_BUTTON) {
		SetMouseEventMask(B_POINTER_EVENTS);
		if (fScrollView && IsEnabled() && Bounds().Contains(point)) {
			SetDragging(true);
			fStartPoint = point;
			fStartScrollOffset = fScrollView->ScrollOffset();
		}
	}
}

// MouseUp
void
ScrollCorner::MouseUp(BPoint point)
{
	BView::MouseUp(point);
	uint32 buttons = 0;
	Window()->CurrentMessage()->FindInt32("buttons", (int32 *)&buttons);
	if (!(buttons & B_PRIMARY_MOUSE_BUTTON))
		SetDragging(false);
}

// MouseMoved
void
ScrollCorner::MouseMoved(BPoint point, uint32 transit, const BMessage* message)
{
	BView::MouseMoved(point, transit, message);
	if (IsDragging()) {
		uint32 buttons = 0;
		Window()->CurrentMessage()->FindInt32("buttons", (int32 *)&buttons);
		// This is a work-around for a BeOS bug: We sometimes don't get a
		// MouseUp(), but fortunately it seems, that within the last
		// MouseMoved() the button is not longer pressed.
		if (buttons & B_PRIMARY_MOUSE_BUTTON) {
			BPoint diff = point - fStartPoint;
			if (fScrollView) {
				fScrollView->_ScrollCornerValueChanged(fStartScrollOffset
													   - diff);
//													   + diff);
			}
		} else
			SetDragging(false);
	}
}

// Draw
void
ScrollCorner::Draw(BRect updateRect)
{
	if (IsEnabled()) {
		if (IsDragging())
			DrawBitmap(fBitmaps[1], BPoint(0.0f, 0.0f));
		else
			DrawBitmap(fBitmaps[0], BPoint(0.0f, 0.0f));
	}
	else
		DrawBitmap(fBitmaps[2], BPoint(0.0f, 0.0f));
}

// WindowActivated
void
ScrollCorner::WindowActivated(bool active)
{
	if (active != (fState & STATE_WINDOW_ACTIVE)) {
		bool enabled = IsEnabled();
		if (active)
			fState |= STATE_WINDOW_ACTIVE;
		else
			fState &= ~STATE_WINDOW_ACTIVE;
		if (enabled != IsEnabled())
			Invalidate();
	}
}

// SetActive
void
ScrollCorner::SetActive(bool active)
{
	if (active != IsActive()) {
		bool enabled = IsEnabled();
		if (active)
			fState |= STATE_ACTIVE;
		else
			fState &= ~STATE_ACTIVE;
		if (enabled != IsEnabled())
			Invalidate();
	}
}

// SetDragging
void
ScrollCorner::SetDragging(bool dragging)
{
	if (dragging != IsDragging()) {
		if (dragging)
			fState |= STATE_DRAGGING;
		else
			fState &= ~STATE_DRAGGING;
		Invalidate();
	}
}


// #pragma mark - ScrollView


// constructor
ScrollView::ScrollView(BView* child, uint32 scrollingFlags, BRect frame,
		const char* name, uint32 resizingMode, uint32 viewFlags,
		uint32 borderStyle, uint32 borderFlags)
	: BView(frame, name, resizingMode,
		viewFlags | B_FRAME_EVENTS | B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE),
	Scroller()
{
	_Init(child, scrollingFlags, borderStyle, borderFlags);
}

#ifdef __HAIKU__

// constructor
ScrollView::ScrollView(BView* child, uint32 scrollingFlags, const char* name,
		uint32 viewFlags, uint32 borderStyle, uint32 borderFlags)
	: BView(name, viewFlags | B_FRAME_EVENTS | B_WILL_DRAW
		| B_FULL_UPDATE_ON_RESIZE),
	Scroller()
{
	_Init(child, scrollingFlags, borderStyle, borderFlags);
}

#endif // __HAIKU__

// destructor
ScrollView::~ScrollView()
{
}

// AllAttached
void
ScrollView::AllAttached()
{
	// do a first layout
	_Layout(_UpdateScrollBarVisibility());
}

// Draw
void ScrollView::Draw(BRect updateRect)
{
	if (fBorderStyle == B_NO_BORDER)
		return;

	rgb_color keyboardFocus = keyboard_navigation_color();
	rgb_color light = tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
								 B_LIGHTEN_MAX_TINT);
	rgb_color shadow = tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
								  B_DARKEN_1_TINT);
	rgb_color darkShadow = tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
								  B_DARKEN_2_TINT);

	BRect r = Bounds();

	if (fChildFocused && fWindowActive) {
		SetHighColor(keyboardFocus);
		StrokeRect(r);
	} else {
		if (fBorderStyle == B_PLAIN_BORDER) {
			SetHighColor(darkShadow);
			StrokeRect(r);
		} else {
			BeginLineArray(4);
			AddLine(BPoint(r.left, r.bottom),
					BPoint(r.left, r.top), shadow);
			AddLine(BPoint(r.left + 1.0, r.top),
					BPoint(r.right, r.top), shadow);
			AddLine(BPoint(r.right, r.top + 1.0),
					BPoint(r.right, r.bottom), light);
			AddLine(BPoint(r.right - 1.0, r.bottom),
					BPoint(r.left + 1.0, r.bottom), light);
			EndLineArray();
		}
	}
	if (fBorderStyle == B_PLAIN_BORDER)
		return;

	// The right and bottom lines will be hidden if the scroll views are
	// visible. But that doesn't harm.
	r.InsetBy(1, 1);
	SetHighColor(darkShadow);
	StrokeRect(r);
}

// FrameResized
void
ScrollView::FrameResized(float width, float height)
{
	_Layout(0);
}

// WindowActivated
void ScrollView::WindowActivated(bool activated)
{
	fWindowActive = activated;
	if (fChildFocused)
		Invalidate();
}

#ifdef __HAIKU__

// MinSize
BSize
ScrollView::MinSize()
{
	BSize size = (fChild ? fChild->MinSize() : BSize(-1, -1));
	return _Size(size);
}

// PreferredSize
BSize
ScrollView::PreferredSize()
{
	BSize size = (fChild ? fChild->PreferredSize() : BSize(-1, -1));
	return _Size(size);
}

#endif // __HAIKU__

// #pragma mark -

// ScrollingFlags
uint32
ScrollView::ScrollingFlags() const
{
	return fScrollingFlags;
}

// SetVisibleRectIsChildBounds
void
ScrollView::SetVisibleRectIsChildBounds(bool flag)
{
	if (flag != VisibleRectIsChildBounds()) {
		if (flag)
			fScrollingFlags |= SCROLL_VISIBLE_RECT_IS_CHILD_BOUNDS;
		else
			fScrollingFlags &= ~SCROLL_VISIBLE_RECT_IS_CHILD_BOUNDS;
		if (fChild && _UpdateScrollBarVisibility())
			_Layout(0);
	}
}

// VisibleRectIsChildBounds
bool
ScrollView::VisibleRectIsChildBounds() const
{
	return (fScrollingFlags & SCROLL_VISIBLE_RECT_IS_CHILD_BOUNDS);
}

// Child
BView*
ScrollView::Child() const
{
	return fChild;
}

// ChildFocusChanged
//
// To be called by the scroll child, when its has got or lost the focus.
// We need this to know, when to draw the blue focus frame.
void
ScrollView::ChildFocusChanged(bool focused)
{
	if (fChildFocused != focused) {
		fChildFocused = focused;
		Invalidate();
	}
}

// HScrollBar
BScrollBar*
ScrollView::HScrollBar() const
{
	return fHScrollBar;
}

// VScrollBar
BScrollBar*
ScrollView::VScrollBar() const
{
	return fVScrollBar;
}

// HVScrollCorner
BView*
ScrollView::HVScrollCorner() const
{
	return fScrollCorner;
}

// #pragma mark -

// SetHSmallStep
void
ScrollView::SetHSmallStep(float hStep)
{
	SetSmallSteps(hStep, fVSmallStep);
}

// SetVSmallStep
void
ScrollView::SetVSmallStep(float vStep)
{
	SetSmallSteps(fHSmallStep, vStep);
}

// SetSmallSteps
void
ScrollView::SetSmallSteps(float hStep, float vStep)
{
	if (fHSmallStep != hStep || fVSmallStep != vStep) {
		fHSmallStep = hStep;
		fVSmallStep = vStep;
		_UpdateScrollBars();
	}
}

// GetSmallSteps
void
ScrollView::GetSmallSteps(float* hStep, float* vStep) const
{
	*hStep = fHSmallStep;
	*vStep = fVSmallStep;
}

// HSmallStep
float
ScrollView::HSmallStep() const
{
	return fHSmallStep;
}

// VSmallStep
float
ScrollView::VSmallStep() const
{
	return fVSmallStep;
}

// IsScrolling
bool
ScrollView::IsScrolling() const
{
	return fScrolling;
}

void
ScrollView::SetScrollingEnabled(bool enabled)
{
	Scroller::SetScrollingEnabled(enabled);
	if (IsScrollingEnabled())
		SetScrollOffset(ScrollOffset());
}

// #pragma mark -

// DataRectChanged
void
ScrollView::DataRectChanged(BRect /*oldDataRect*/, BRect /*newDataRect*/)
{
	if (ScrollTarget()) {
		if (_UpdateScrollBarVisibility())
			_Layout(0);
		else
			_UpdateScrollBars();
	}
}

// ScrollOffsetChanged
void
ScrollView::ScrollOffsetChanged(BPoint /*oldOffset*/, BPoint newOffset)
{
	if (fHScrollBar && fHScrollBar->Value() != newOffset.x)
		fHScrollBar->SetValue(newOffset.x);
	if (fVScrollBar && fVScrollBar->Value() != newOffset.y)
		fVScrollBar->SetValue(newOffset.y);
}

// VisibleSizeChanged
void
ScrollView::VisibleSizeChanged(float /*oldWidth*/, float /*oldHeight*/,
							   float /*newWidth*/, float /*newHeight*/)
{
	if (ScrollTarget()) {
		if (_UpdateScrollBarVisibility())
			_Layout(0);
		else
			_UpdateScrollBars();
	}
}

// ScrollTargetChanged
void
ScrollView::ScrollTargetChanged(Scrollable* /*oldTarget*/,
								Scrollable* newTarget)
{
/*	// remove the old child
	if (fChild)
		RemoveChild(fChild);
	// add the new child
	BView* view = dynamic_cast<BView*>(newTarget);
	fChild = view;
	if (view)
		AddChild(view);
	else if (newTarget)	// set the scroll target to NULL, if it isn't a BView
		SetScrollTarget(NULL);
*/
}

// _Init
void
ScrollView::_Init(BView* child, uint32 scrollingFlags, uint32 borderStyle,
	uint32 borderFlags)
{
	fChild = NULL;
	fScrollingFlags = scrollingFlags;

	fHScrollBar = NULL;
	fVScrollBar = NULL;
	fScrollCorner = NULL;

	fHVisible = true;
	fVVisible = true;
	fCornerVisible = true;

	fWindowActive = false;
	fChildFocused = false;

	fScrolling = false;

	fHSmallStep = 1;
	fVSmallStep = 1;

	fBorderStyle = borderStyle;
	fBorderFlags = borderFlags;

	// Set transparent view color -- our area is completely covered by
	// our children.
	SetViewColor(B_TRANSPARENT_32_BIT);
	// create scroll bars
	if (fScrollingFlags & (SCROLL_HORIZONTAL | SCROLL_HORIZONTAL_MAGIC)) {
		fHScrollBar = new InternalScrollBar(this,
				BRect(0.0, 0.0, 100.0, B_H_SCROLL_BAR_HEIGHT), B_HORIZONTAL);
		AddChild(fHScrollBar);
	}
	if (fScrollingFlags & (SCROLL_VERTICAL | SCROLL_VERTICAL_MAGIC)) {
		fVScrollBar = new InternalScrollBar(this,
				BRect(0.0, 0.0, B_V_SCROLL_BAR_WIDTH, 100.0), B_VERTICAL);
		AddChild(fVScrollBar);
	}
	// Create a scroll corner, if we can scroll into both direction.
	if (fHScrollBar && fVScrollBar) {
		fScrollCorner = new ScrollCorner(this);
		AddChild(fScrollCorner);
	}
	// add child
	if (child) {
		fChild = child;
		AddChild(child);
		if (Scrollable* scrollable = dynamic_cast<Scrollable*>(child))
			SetScrollTarget(scrollable);
	}
}


// _ScrollValueChanged
void
ScrollView::_ScrollValueChanged(InternalScrollBar* scrollBar, float value)
{
	if (!IsScrollingEnabled())
		return;

	switch (scrollBar->Orientation()) {
		case B_HORIZONTAL:
			if (fHScrollBar)
				SetScrollOffset(BPoint(value, ScrollOffset().y));
			break;
		case B_VERTICAL:
			if (fVScrollBar)
				SetScrollOffset(BPoint(ScrollOffset().x, value));
			break;
		default:
			break;
	}
}

// _ScrollCornerValueChanged
void
ScrollView::_ScrollCornerValueChanged(BPoint offset)
{
	// The logic in Scrollable::SetScrollOffset() handles offsets, that
	// are out of range.
	SetScrollOffset(offset);
}

// #pragma mark -

// _Layout
//
// Relayouts all children (fChild, scroll bars).
// flags indicates which scrollbars' visibility has changed.
// May be overridden to do a custom layout -- the SCROLL_*_MAGIC must
// be disabled in this case, or strange things happen.
void
ScrollView::_Layout(uint32 flags)
{
	bool hbar = (fHScrollBar && fHVisible);
	bool vbar = (fVScrollBar && fVVisible);
	bool corner = (fScrollCorner && fCornerVisible);
	BRect childRect(_ChildRect());
	float innerWidth = childRect.Width();
	float innerHeight = childRect.Height();
	BPoint scrollLT(_InnerRect().LeftTop());
	scrollLT.x--;
	scrollLT.y--;

	BPoint scrollRB(childRect.RightBottom() + BPoint(1.0f, 1.0f));

	// layout scroll bars and scroll corner
	if (corner) {
		// In this case the scrollbars overlap one pixel.
		fHScrollBar->MoveTo(scrollLT.x, scrollRB.y);
		fHScrollBar->ResizeTo(innerWidth + 2.0, B_H_SCROLL_BAR_HEIGHT);
		fVScrollBar->MoveTo(scrollRB.x, scrollLT.y);
		fVScrollBar->ResizeTo(B_V_SCROLL_BAR_WIDTH, innerHeight + 2.0);
		fScrollCorner->MoveTo(childRect.right + 2.0, childRect.bottom + 2.0);
	} else if (hbar) {
		fHScrollBar->MoveTo(scrollLT.x, scrollRB.y);
		fHScrollBar->ResizeTo(innerWidth + 2.0, B_H_SCROLL_BAR_HEIGHT);
	} else if (vbar) {
		fVScrollBar->MoveTo(scrollRB.x, scrollLT.y);
		fVScrollBar->ResizeTo(B_V_SCROLL_BAR_WIDTH, innerHeight + 2.0);
	}
	// layout child
	if (fChild) {
		fChild->MoveTo(childRect.LeftTop());
		fChild->ResizeTo(innerWidth, innerHeight);
		if (VisibleRectIsChildBounds())
			SetVisibleSize(innerWidth, innerHeight);
		// Due to a BeOS bug sometimes the area under a recently hidden
		// scroll bar isn't updated correctly.
		// We force this manually: The position of hidden scroll bar isn't
		// updated any longer, so we can't just invalidate it.
		if (fChild->Window()) {
			if (flags & SCROLL_HORIZONTAL && !fHVisible)
				fChild->Invalidate(fHScrollBar->Frame());
			if (flags & SCROLL_VERTICAL && !fVVisible)
				fChild->Invalidate(fVScrollBar->Frame());
		}
	}
}

// _UpdateScrollBars
//
// Probably somewhat misnamed. This function updates the scroll bars'
// proportion, range attributes and step widths according to the scroll
// target's DataRect() and VisibleBounds(). May also be called, if there's
// no scroll target -- then the scroll bars are disabled.
void
ScrollView::_UpdateScrollBars()
{
	BRect dataRect = DataRect();
	BRect visibleBounds = VisibleBounds();
	if (!fScrollTarget) {
		dataRect.Set(0.0, 0.0, 0.0, 0.0);
		visibleBounds.Set(0.0, 0.0, 0.0, 0.0);
	}
	float hProportion = min_c(1.0f, (visibleBounds.Width() + 1.0f)
		/ (dataRect.Width() + 1.0f));
	float hMaxValue = max_c(dataRect.left,
		dataRect.right - visibleBounds.Width());
	float vProportion = min_c(1.0f, (visibleBounds.Height() + 1.0f)
		/ (dataRect.Height() + 1.0f));
	float vMaxValue = max_c(dataRect.top,
		dataRect.bottom - visibleBounds.Height());
	// update horizontal scroll bar
	if (fHScrollBar) {
		fHScrollBar->SetProportion(hProportion);
		fHScrollBar->SetRange(dataRect.left, hMaxValue);
		// This obviously ineffective line works around a BScrollBar bug:
		// As documented the scrollbar's value is adjusted, if the range
		// has been changed and it therefore falls out of the range. But if,
		// after resetting the range to what it has been before, the user
		// moves the scrollbar to the original value via one click
		// it is failed to invoke BScrollBar::ValueChanged().
		fHScrollBar->SetValue(fHScrollBar->Value());
		fHScrollBar->SetSteps(fHSmallStep, visibleBounds.Width());
	}
	// update vertical scroll bar
	if (fVScrollBar) {
		fVScrollBar->SetProportion(vProportion);
		fVScrollBar->SetRange(dataRect.top, vMaxValue);
		// This obviously ineffective line works around a BScrollBar bug.
		fVScrollBar->SetValue(fVScrollBar->Value());
		fVScrollBar->SetSteps(fVSmallStep, visibleBounds.Height());
	}
	// update scroll corner
	if (fScrollCorner) {
		fScrollCorner->SetActive(hProportion < 1.0f || vProportion < 1.0f);
	}
}

// set_visible_state
//
// Convenience function: Sets a view's visibility state to /visible/.
// Returns true, if the state was actually changed, false otherwise.
// This function never calls Hide() on a hidden or Show() on a visible
// view. /view/ must be valid.
static inline
bool
set_visible_state(BView* view, bool visible, bool* currentlyVisible)
{
	bool changed = false;
	if (*currentlyVisible != visible) {
		if (visible)
			view->Show();
		else
			view->Hide();
		*currentlyVisible = visible;
		changed = true;
	}
	return changed;
}

// _UpdateScrollBarVisibility
//
// Checks which of scroll bars need to be visible according to
// SCROLL_*_MAGIG and shows/hides them, if necessary.
// Returns a bitwise combination of SCROLL_HORIZONTAL and SCROLL_VERTICAL
// according to which scroll bar's visibility state has changed, 0 if none.
// A return value != 0 usually means that the layout isn't valid any longer.
uint32
ScrollView::_UpdateScrollBarVisibility()
{
	uint32 changed = 0;
	BRect childRect(_MaxVisibleRect());
	float width = childRect.Width();
	float height = childRect.Height();
	BRect dataRect = DataRect();			// Invalid if !ScrollTarget(),
	float dataWidth = dataRect.Width();		// but that doesn't harm.
	float dataHeight = dataRect.Height();	//
	bool hbar = (fScrollingFlags & SCROLL_HORIZONTAL_MAGIC);
	bool vbar = (fScrollingFlags & SCROLL_VERTICAL_MAGIC);
	if (!ScrollTarget()) {
		if (hbar) {
			if (set_visible_state(fHScrollBar, false, &fHVisible))
				changed |= SCROLL_HORIZONTAL;
		}
		if (vbar) {
			if (set_visible_state(fVScrollBar, false, &fVVisible))
				changed |= SCROLL_VERTICAL;
		}
	} else if (hbar && width >= dataWidth && vbar && height >= dataHeight) {
		// none
		if (set_visible_state(fHScrollBar, false, &fHVisible))
			changed |= SCROLL_HORIZONTAL;
		if (set_visible_state(fVScrollBar, false, &fVVisible))
			changed |= SCROLL_VERTICAL;
	} else {
		// The case, that both scroll bars are magic and invisible is catched,
		// so that while checking one bar we can suppose, that the other one
		// is visible (if it does exist at all).
		BRect innerRect(_GuessVisibleRect(fHScrollBar, fVScrollBar));
		float innerWidth = innerRect.Width();
		float innerHeight = innerRect.Height();
		// the horizontal one?
		if (hbar) {
			if (innerWidth >= dataWidth) {
				if (set_visible_state(fHScrollBar, false, &fHVisible))
					changed |= SCROLL_HORIZONTAL;
			} else {
				if (set_visible_state(fHScrollBar, true, &fHVisible))
					changed |= SCROLL_HORIZONTAL;
			}
		}
		// the vertical one?
		if (vbar) {
			if (innerHeight >= dataHeight) {
				if (set_visible_state(fVScrollBar, false, &fVVisible))
					changed |= SCROLL_VERTICAL;
			} else {
				if (set_visible_state(fVScrollBar, true, &fVVisible))
					changed |= SCROLL_VERTICAL;
			}
		}
	}
	// If anything has changed, update the scroll corner as well.
	if (changed && fScrollCorner)
		set_visible_state(fScrollCorner, fHVisible && fVVisible, &fCornerVisible);
	return changed;
}

// _InnerRect
//
// Returns the rectangle that actually can be used for the child and the
// scroll bars, i.e. the view's Bounds() subtracted the space for the
// decorative frame.
BRect
ScrollView::_InnerRect() const
{
	BRect r = Bounds();
	float borderWidth = 0;
	switch (fBorderStyle) {
		case B_NO_BORDER:
			break;
		case B_PLAIN_BORDER:
			borderWidth = 1;
			break;
		case B_FANCY_BORDER:
		default:
			borderWidth = 2;
			break;
	}
	if (fBorderFlags & BORDER_LEFT)
		r.left += borderWidth;
	if (fBorderFlags & BORDER_TOP)
		r.top += borderWidth;
	if (fBorderFlags & BORDER_RIGHT)
		r.right -= borderWidth;
	if (fBorderFlags & BORDER_BOTTOM)
		r.bottom -= borderWidth;
	return r;
}

// _ChildRect
//
// Returns the rectangle, that should be the current child frame.
// `should' because 1. we might not have a child at all or 2. a
// relayout is pending.
BRect
ScrollView::_ChildRect() const
{
	return _ChildRect(fHScrollBar && fHVisible, fVScrollBar && fVVisible);
}

// _ChildRect
//
// The same as _ChildRect() with the exception that not the current
// scroll bar visibility, but a fictitious one given by /hbar/ and /vbar/
// is considered.
BRect
ScrollView::_ChildRect(bool hbar, bool vbar) const
{
	BRect rect(_InnerRect());
	if (vbar)
		rect.right -= B_V_SCROLL_BAR_WIDTH;
	if (hbar)
		rect.bottom -= B_H_SCROLL_BAR_HEIGHT;

	return rect;
}

// _GuessVisibleRect
//
// Returns an approximation of the visible rect for the
// fictitious scroll bar visibility given by /hbar/ and /vbar/.
// In the case !VisibleRectIsChildBounds() it is simply the current
// visible rect.
BRect
ScrollView::_GuessVisibleRect(bool hbar, bool vbar) const
{
	if (VisibleRectIsChildBounds())
		return _ChildRect(hbar, vbar).OffsetToCopy(ScrollOffset());
	return VisibleRect();
}

// _MaxVisibleRect
//
// Returns the maximal possible visible rect in the current situation, that
// is depending on if the visible rect is the child's bounds either the
// rectangle the child covers when both scroll bars are hidden (offset to
// the scroll offset) or the current visible rect.
BRect
ScrollView::_MaxVisibleRect() const
{
	return _GuessVisibleRect(true, true);
}

#ifdef __HAIKU__

BSize
ScrollView::_Size(BSize size)
{
	if (fVVisible)
		size.width += B_V_SCROLL_BAR_WIDTH;
	if (fHVisible)
		size.height += B_H_SCROLL_BAR_HEIGHT;

	switch (fBorderStyle) {
		case B_NO_BORDER:
			// one line of pixels from scrollbar possibly hidden
			if (fBorderFlags & BORDER_RIGHT)
				size.width += fVVisible ? -1 : 0;
			if (fBorderFlags & BORDER_BOTTOM)
				size.height += fHVisible ? -1 : 0;
			break;

		case B_PLAIN_BORDER:
			if (fBorderFlags & BORDER_LEFT)
				size.width += 1;
			if (fBorderFlags & BORDER_TOP)
				size.height += 1;
			// one line of pixels in frame possibly from scrollbar
			if (fBorderFlags & BORDER_RIGHT)
				size.width += fVVisible ? 0 : 1;
			if (fBorderFlags & BORDER_BOTTOM)
				size.height += fHVisible ? 0 : 1;
			break;

		case B_FANCY_BORDER:
		default:
			if (fBorderFlags & BORDER_LEFT)
				size.width += 2;
			if (fBorderFlags & BORDER_TOP)
				size.height += 2;
			// one line of pixels in frame possibly from scrollbar
			if (fBorderFlags & BORDER_RIGHT)
				size.width += fVVisible ? 1 : 2;
			if (fBorderFlags & BORDER_BOTTOM)
				size.height += fHVisible ? 1 : 2;
			break;
	}

	return BLayoutUtils::ComposeSize(ExplicitMinSize(), size);
}

#endif // __HAIKU__

// _SetScrolling
void
ScrollView::_SetScrolling(bool scrolling)
{
	fScrolling = scrolling;
}
