//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		ScrollView.cpp
//	Author:			Axel Dörfler <axeld@pinc-software.de>
//	Description:	BView with two scrollbars
//------------------------------------------------------------------------------

#include <ScrollView.h>
#include <Message.h>
#include <Window.h>


static const float kFancyBorderSize = 2;
static const float kPlainBorderSize = 1;


BScrollView::BScrollView(const char *name, BView *target, uint32 resizingMode,
	uint32 flags, bool horizontal, bool vertical, border_style border)
	: BView(CalcFrame(target, horizontal, vertical, border), name,
		ModifyFlags(flags, border), resizingMode),
	fTarget(target),
	fHorizontalScrollBar(NULL),
	fVerticalScrollBar(NULL),
	fBorder(border),
	fHighlighted(false)
{
	fTarget->TargetedByScrollView(this);
	fTarget->MoveTo(B_ORIGIN);

	if (border != B_NO_BORDER)
		fTarget->MoveBy(BorderSize(border), BorderSize(border));

	AddChild(fTarget);

	BRect frame = fTarget->Frame();
	if (horizontal) {
		BRect rect = frame;
		rect.top = rect.bottom + 1;
		rect.bottom += B_H_SCROLL_BAR_HEIGHT + 1;
		if (border != B_NO_BORDER || vertical)
			rect.right++;
		if (border != B_NO_BORDER)
			rect.left--;
		fHorizontalScrollBar = new BScrollBar(rect, "_HSB_", fTarget, 0, 1000, B_HORIZONTAL);
		AddChild(fHorizontalScrollBar);
	}

	if (vertical) {
		BRect rect = frame;
		rect.left = rect.right + 1;
		rect.right += B_V_SCROLL_BAR_WIDTH + 1;
		if (border != B_NO_BORDER || horizontal)
			rect.bottom++;
		if (border != B_NO_BORDER)
			rect.top--;
		fVerticalScrollBar = new BScrollBar(rect, "_VSB_", fTarget, 0, 1000, B_VERTICAL);
		AddChild(fVerticalScrollBar);
	}

	fPreviousWidth = uint16(Bounds().Width());
	fPreviousHeight = uint16(Bounds().Height());
}


BScrollView::BScrollView(BMessage *archive)
	: BView(archive),
	fHighlighted(false)
{
	int32 border;
	fBorder = archive->FindInt32("_style", &border) == B_OK ?
		(border_style)border : B_FANCY_BORDER;

	// In a shallow archive, we may not have a target anymore. We must
	// be prepared for this case

	// don't confuse our scroll bars with our (eventual) target
	int32 firstBar = 0;
	if (!archive->FindBool("_no_target_")) {
		fTarget = ChildAt(0);
		firstBar++;
	} else
		fTarget = NULL;

	// search for our scroll bars

	fHorizontalScrollBar = NULL;
	fVerticalScrollBar = NULL;

	BView *view;
	while ((view = ChildAt(firstBar++)) != NULL) {
		BScrollBar *bar = dynamic_cast<BScrollBar *>(view);
		if (bar == NULL)
			continue;
		
		if (bar->Orientation() == B_HORIZONTAL)
			fHorizontalScrollBar = bar;
		else if (bar->Orientation() == B_VERTICAL)
			fVerticalScrollBar = bar;
	}

	fPreviousWidth = uint16(Bounds().Width());
	fPreviousHeight = uint16(Bounds().Height());
}


BScrollView::~BScrollView()
{
}


BArchivable *
BScrollView::Instantiate(BMessage *archive)
{
	if (validate_instantiation(archive, "BScrollView"))
		return new BScrollView(archive);

	return NULL;
}


status_t
BScrollView::Archive(BMessage *archive, bool deep) const
{
	status_t status = BView::Archive(archive, deep);
	if (status != B_OK)
		return status;

	// If this is a deep archive, the BView class will take care
	// of our children.

	if (status == B_OK && fBorder != B_FANCY_BORDER)
		status = archive->AddInt32("_style", fBorder);
	if (status == B_OK && fTarget == NULL)
		status = archive->AddBool("_no_target_", true);

	// The highlighted state is not archived, but since it is
	// usually (or should be) used to indicate focus, this
	// is probably the right thing to do.

	return status;
}


void
BScrollView::AttachedToWindow()
{
	BView::AttachedToWindow();

	if ((fHorizontalScrollBar == NULL && fVerticalScrollBar == NULL)
		|| (fHorizontalScrollBar != NULL && fVerticalScrollBar != NULL))
		return;

	// If we have only one bar, we need to check if we are in the
	// bottom right edge of a window with the B_DOCUMENT_LOOK to
	// adjust the size of the bar to acknowledge the resize knob.

	BRect bounds = ConvertToScreen(Bounds());
	BRect windowBounds = Window()->Frame();

	if (bounds.right - BorderSize(fBorder) != windowBounds.right
		|| bounds.bottom - BorderSize(fBorder) != windowBounds.bottom)
		return;

	if (fHorizontalScrollBar)
		fHorizontalScrollBar->ResizeBy(-B_V_SCROLL_BAR_WIDTH, 0);
	else if (fVerticalScrollBar)
		fVerticalScrollBar->ResizeBy(0, -B_H_SCROLL_BAR_HEIGHT);
}


void
BScrollView::DetachedFromWindow()
{
	BView::DetachedFromWindow();
}


void
BScrollView::AllAttached()
{
	BView::AllAttached();
}


void
BScrollView::AllDetached()
{
	BView::AllDetached();
}


void
BScrollView::Draw(BRect updateRect)
{
	if (fBorder == B_PLAIN_BORDER) {
		SetHighColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_DARKEN_2_TINT));
		StrokeRect(Bounds());
		return;
	} else if (fBorder != B_FANCY_BORDER)
		return;

	BRect bounds = Bounds();
	SetHighColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_DARKEN_2_TINT));
	StrokeRect(bounds.InsetByCopy(1, 1));

	if (fHighlighted) {
		SetHighColor(ui_color(B_NAVIGATION_BASE_COLOR));
		StrokeRect(bounds);
	} else {
		SetHighColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_DARKEN_1_TINT));
		StrokeLine(bounds.LeftBottom(), bounds.LeftTop());
		bounds.left++;
		StrokeLine(bounds.LeftTop(), bounds.RightTop());
	
		SetHighColor(ui_color(B_SHINE_COLOR));
		StrokeLine(bounds.LeftBottom(), bounds.RightBottom());
		bounds.top++;
		bounds.bottom--;
		StrokeLine(bounds.RightBottom(), bounds.RightTop());
	}
}


BScrollBar *
BScrollView::ScrollBar(orientation posture) const
{
	if (posture == B_HORIZONTAL)
		return fHorizontalScrollBar;

	return fVerticalScrollBar;
}


void
BScrollView::SetBorder(border_style border)
{
	if (fBorder == border)
		return;

	float offset = BorderSize(fBorder) - BorderSize(border);
	float resize = 2 * offset;

	float horizontalGap = 0, verticalGap = 0;
	float change = 0;
	if (border == B_NO_BORDER || fBorder == B_NO_BORDER) {
		if (fHorizontalScrollBar != NULL)
			verticalGap = border != B_NO_BORDER ? 1 : -1;
		if (fVerticalScrollBar != NULL)
			horizontalGap = border != B_NO_BORDER ? 1 : -1;

		change = border != B_NO_BORDER ? -1 : 1;
		if (fHorizontalScrollBar == NULL || fVerticalScrollBar == NULL)
			change *= 2;
	}

	fBorder = border;

	int32 savedResizingMode = 0;
	if (fTarget != NULL) {
		savedResizingMode = fTarget->ResizingMode();
		fTarget->SetResizingMode(B_FOLLOW_NONE);
	}

	MoveBy(offset, offset);
	ResizeBy(-resize - horizontalGap, -resize - verticalGap);

	if (fTarget != NULL) {
		fTarget->MoveBy(-offset, -offset);
		fTarget->SetResizingMode(savedResizingMode);
	}

	if (fHorizontalScrollBar != NULL) {
		fHorizontalScrollBar->MoveBy(-offset - verticalGap, offset + verticalGap);
		fHorizontalScrollBar->ResizeBy(resize + horizontalGap - change, 0);
	}
	if (fVerticalScrollBar != NULL) {
		fVerticalScrollBar->MoveBy(offset + horizontalGap, -offset - horizontalGap);
		fVerticalScrollBar->ResizeBy(0, resize + verticalGap - change);
	}

	SetFlags(ModifyFlags(Flags(), border));
}


border_style
BScrollView::Border() const
{
	return fBorder;
}


status_t
BScrollView::SetBorderHighlighted(bool state)
{
	if (fHighlighted == state)
		return B_OK;

	if (fBorder != B_FANCY_BORDER)
		// highlighting only works for B_FANCY_BORDER
		return B_ERROR;

	fHighlighted = state;

	/* The BeBook describes something like this:
	
	if (LockLooper()) {
		Draw(Bounds());
		UnlockLooper();
	}
	*/
	
	// but this is much cleaner, I think:
	BRect bounds = Bounds();
	Invalidate(BRect(bounds.left, bounds.top, bounds.right, bounds.top));
	Invalidate(BRect(bounds.left, bounds.top + 1, bounds.left, bounds.bottom - 1));
	Invalidate(BRect(bounds.right, bounds.top + 1, bounds.right, bounds.bottom - 1));
	Invalidate(BRect(bounds.left, bounds.bottom, bounds.right, bounds.bottom));

	return B_OK;
}


bool
BScrollView::IsBorderHighlighted() const
{
	return fHighlighted;
}


void
BScrollView::SetTarget(BView *target)
{
	if (fTarget == target)
		return;

	if (fTarget != NULL) {
		fTarget->TargetedByScrollView(NULL);
		RemoveChild(fTarget);

		// we are not supposed to delete the view
	}

	fTarget = target;
	if (fHorizontalScrollBar != NULL)
		fHorizontalScrollBar->SetTarget(target);
	if (fVerticalScrollBar != NULL)
		fVerticalScrollBar->SetTarget(target);

	if (target != NULL) {
		target->MoveTo(BorderSize(fBorder), BorderSize(fBorder));
		target->TargetedByScrollView(this);

		AddChild(target, ChildAt(0));
			// This way, we are making sure that the target will
			// be added top most in the list (which is important
			// for unarchiving)
	}
}


BView *
BScrollView::Target() const
{
	return fTarget;
}


void
BScrollView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		default:
			BView::MessageReceived(message);
	}
}


void
BScrollView::MouseDown(BPoint point)
{
	BView::MouseDown(point);
}


void
BScrollView::MouseUp(BPoint point)
{
	BView::MouseUp(point);
}


void
BScrollView::MouseMoved(BPoint point, uint32 code, const BMessage *dragMessage)
{
	BView::MouseMoved(point, code, dragMessage);
}


void
BScrollView::FrameMoved(BPoint position)
{
	BView::FrameMoved(position);
}


void
BScrollView::FrameResized(float width, float height)
{
	BView::FrameResized(width, height);

	if (fBorder == B_NO_BORDER)
		return;

	// changes in width

	BRect bounds = Bounds();
	float border = BorderSize(fBorder) - 1;

	if (bounds.Width() > fPreviousWidth) {
		// invalidate the region between the old and the new right border
		BRect rect = bounds;
		rect.left += fPreviousWidth - border;
		rect.right--;
		Invalidate(rect);
	} else if (bounds.Width() < fPreviousWidth) {
		// invalidate the region of the new right border
		BRect rect = bounds;
		rect.left = rect.right - border;
		Invalidate(rect);
	}

	// changes in height

	if (bounds.Height() > fPreviousHeight) {
		// invalidate the region between the old and the new bottom border
		BRect rect = bounds;
		rect.top += fPreviousHeight - border;
		rect.bottom--;
		Invalidate(rect);
	} else if (bounds.Height() < fPreviousHeight) {
		// invalidate the region of the new bottom border
		BRect rect = bounds;
		rect.top = rect.bottom - border;
		Invalidate(rect);
	}

	fPreviousWidth = uint16(bounds.Width());
	fPreviousHeight = uint16(bounds.Height());
}


void 
BScrollView::ResizeToPreferred()
{
	BView::ResizeToPreferred();
}


void 
BScrollView::GetPreferredSize(float *_width, float *_height)
{
	BRect frame = CalcFrame(fTarget, fHorizontalScrollBar, fVerticalScrollBar, fBorder);

	if (fTarget != NULL) {
		float width, height;
		fTarget->GetPreferredSize(&width, &height);

		frame.right += width - fTarget->Frame().Width();
		frame.bottom += height - fTarget->Frame().Height();
	}

	if (_width)
		*_width = frame.Width();
	if (_height)
		*_height = frame.Height();
}




/** This static method is used to calculate the frame that the
 *	ScrollView will cover depending on the frame of its target
 *	and which border style is used.
 *	It is used in the constructor and at other places.
 */

BRect
BScrollView::CalcFrame(BView *target, bool horizontal, bool vertical, border_style border)
{
	BRect frame = target != NULL ? frame = target->Frame() : BRect(0, 0, 80, 80);

	if (vertical)
		frame.right += B_V_SCROLL_BAR_WIDTH;
	if (horizontal)
		frame.bottom += B_H_SCROLL_BAR_HEIGHT;

	float borderSize = BorderSize(border);
	frame.InsetBy(-borderSize, -borderSize);

	if (borderSize == 0) {
		if (vertical)
			frame.right++;
		if (horizontal)
			frame.bottom++;
	}

	return frame;
}


/** This method returns the size of the specified border
 */

float 
BScrollView::BorderSize(border_style border)
{
	if (border == B_FANCY_BORDER)
		return kFancyBorderSize;
	if (border == B_PLAIN_BORDER)
		return kPlainBorderSize;

	return 0;
}


/** This method changes the "flags" argument as passed on to
 *	the BView constructor.
 */

int32
BScrollView::ModifyFlags(int32 flags, border_style border)
{
	// We either need B_FULL_UPDATE_ON_RESIZE or
	// B_FRAME_EVENTS if we have to draw a border
	if (border != B_NO_BORDER)
		return flags | B_WILL_DRAW | (flags & B_FULL_UPDATE_ON_RESIZE ? 0 : B_FRAME_EVENTS);

	return flags & ~(B_WILL_DRAW | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE);
}


void
BScrollView::WindowActivated(bool active)
{
	BView::WindowActivated(active);
}


void 
BScrollView::MakeFocus(bool state)
{
	BView::MakeFocus(state);
}


BHandler *
BScrollView::ResolveSpecifier(BMessage *msg, int32 index, BMessage *specifier, int32 form, const char *property)
{
	return BView::ResolveSpecifier(msg, index, specifier, form, property);
}


status_t 
BScrollView::GetSupportedSuites(BMessage *data)
{
	return BView::GetSupportedSuites(data);
}


status_t
BScrollView::Perform(perform_code d, void *arg)
{
	return BView::Perform(d, arg);
}


BScrollView &
BScrollView::operator=(const BScrollView &)
{
	return *this;
}


/** Although BScrollView::InitObject() was defined in the original ScrollView.h,
 *	it is not exported by the R5 libbe.so, so we don't have to export it as well.
 */

#if 0
void
BScrollView::InitObject()
{
}
#endif


//	#pragma mark -
//	Reserved virtuals


void BScrollView::_ReservedScrollView1() {}
void BScrollView::_ReservedScrollView2() {}
void BScrollView::_ReservedScrollView3() {}
void BScrollView::_ReservedScrollView4() {}

