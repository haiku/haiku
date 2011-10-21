/*
 * Copyright 2004-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <ScrollView.h>

#include <ControlLook.h>
#include <LayoutUtils.h>
#include <Message.h>
#include <Region.h>
#include <Window.h>

#include <binary_compatibility/Interface.h>

static const float kFancyBorderSize = 2;
static const float kPlainBorderSize = 1;


BScrollView::BScrollView(const char *name, BView *target, uint32 resizingMode,
		uint32 flags, bool horizontal, bool vertical, border_style border)
	: BView(_ComputeFrame(target, horizontal, vertical, border), name,
		resizingMode, _ModifyFlags(flags, border)),
	fTarget(target),
	fBorder(border)
{
	_Init(horizontal, vertical);
}


BScrollView::BScrollView(const char* name, BView* target, uint32 flags,
		bool horizontal, bool vertical, border_style border)
	: BView(name, _ModifyFlags(flags, border)),
	fTarget(target),
	fBorder(border)
{
	_Init(horizontal, vertical);
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


// #pragma mark -


BArchivable*
BScrollView::Instantiate(BMessage* archive)
{
	if (validate_instantiation(archive, "BScrollView"))
		return new BScrollView(archive);

	return NULL;
}


status_t
BScrollView::Archive(BMessage* archive, bool deep) const
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
		|| (fHorizontalScrollBar != NULL && fVerticalScrollBar != NULL)
		|| Window()->Look() != B_DOCUMENT_WINDOW_LOOK)
		return;

	// If we have only one bar, we need to check if we are in the
	// bottom right edge of a window with the B_DOCUMENT_LOOK to
	// adjust the size of the bar to acknowledge the resize knob.

	BRect bounds = ConvertToScreen(Bounds());
	BRect windowBounds = Window()->Frame();

	if (bounds.right - _BorderSize() != windowBounds.right
		|| bounds.bottom - _BorderSize() != windowBounds.bottom)
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
	if (be_control_look != NULL) {
		uint32 flags = 0;
		if (fHighlighted && Window()->IsActive())
			flags |= BControlLook::B_FOCUSED;
		BRect rect(Bounds());
		rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);

		BRect verticalScrollBarFrame(0, 0, -1, -1);
		if (fVerticalScrollBar)
			verticalScrollBarFrame = fVerticalScrollBar->Frame();
		BRect horizontalScrollBarFrame(0, 0, -1, -1);
		if (fHorizontalScrollBar)
			horizontalScrollBarFrame = fHorizontalScrollBar->Frame();

		be_control_look->DrawScrollViewFrame(this, rect, updateRect,
			verticalScrollBarFrame, horizontalScrollBarFrame, base, fBorder,
			flags);

		return;
	}

	if (fBorder == B_PLAIN_BORDER) {
		SetHighColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
			B_DARKEN_2_TINT));
		StrokeRect(Bounds());
		return;
	} else if (fBorder != B_FANCY_BORDER)
		return;

	BRect bounds = Bounds();
	SetHighColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
		B_DARKEN_2_TINT));
	StrokeRect(bounds.InsetByCopy(1, 1));

	if (fHighlighted && Window()->IsActive()) {
		SetHighColor(ui_color(B_NAVIGATION_BASE_COLOR));
		StrokeRect(bounds);
	} else {
		SetHighColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
			B_DARKEN_1_TINT));
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


// #pragma mark -


void
BScrollView::WindowActivated(bool active)
{
	if (fHighlighted)
		Invalidate();

	BView::WindowActivated(active);
}


void
BScrollView::MakeFocus(bool state)
{
	BView::MakeFocus(state);
}


// #pragma mark -


void
BScrollView::GetPreferredSize(float *_width, float *_height)
{
	BSize size = PreferredSize();

	if (_width)
		*_width = size.width;
	if (_height)
		*_height = size.height;
}


BSize
BScrollView::MinSize()
{
	BSize size = _ComputeSize(fTarget != NULL ? fTarget->MinSize()
		: BSize(16, 16));

	return BLayoutUtils::ComposeSize(ExplicitMinSize(), size);
}


BSize
BScrollView::MaxSize()
{
	BSize size = _ComputeSize(fTarget != NULL ? fTarget->MaxSize()
		: BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));

	return BLayoutUtils::ComposeSize(ExplicitMaxSize(), size);
}


BSize
BScrollView::PreferredSize()
{
	BSize size = _ComputeSize(fTarget != NULL ? fTarget->PreferredSize()
		: BSize(32, 32));

	return BLayoutUtils::ComposeSize(ExplicitPreferredSize(), size);
}


void
BScrollView::ResizeToPreferred()
{
	if (Window() == NULL)
		return;
	BView::ResizeToPreferred();
}


void
BScrollView::InvalidateLayout(bool descendants)
{
	BView::InvalidateLayout(descendants);
}


void
BScrollView::DoLayout()
{
	if (!(Flags() & B_SUPPORTS_LAYOUT))
		return;

	// If the user set a layout, we let the base class version call its hook.
	if (GetLayout()) {
		BView::DoLayout();
		return;
	}

	BRect innerFrame = _InnerFrame();

	if (fTarget != NULL) {
		fTarget->MoveTo(innerFrame.left, innerFrame.top);
		fTarget->ResizeTo(innerFrame.Width(), innerFrame.Height());

		//BLayoutUtils::AlignInFrame(fTarget, fTarget->Bounds());
	}

	_AlignScrollBars(fHorizontalScrollBar != NULL, fVerticalScrollBar != NULL,
		innerFrame);
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

	BRect bounds = Bounds();
	float border = _BorderSize() - 1;

	if (be_control_look && fHorizontalScrollBar && fVerticalScrollBar) {
		BRect scrollCorner(bounds);
		scrollCorner.left = min_c(
			fPreviousWidth - fVerticalScrollBar->Frame().Height(),
			fHorizontalScrollBar->Frame().right + 1);
		scrollCorner.top = min_c(
			fPreviousHeight - fHorizontalScrollBar->Frame().Width(),
			fVerticalScrollBar->Frame().bottom + 1);
		Invalidate(scrollCorner);
	}

	// changes in width

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


// #pragma mark -


void
BScrollView::MessageReceived(BMessage* message)
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


// #pragma mark -


BScrollBar*
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

	if (Flags() & B_SUPPORTS_LAYOUT) {
		fBorder = border;
		SetFlags(_ModifyFlags(Flags(), border));

		DoLayout();

		BRect bounds(Bounds());
		Invalidate(BRect(bounds.LeftTop(), bounds.RightBottom()));
		Invalidate(BRect(bounds.LeftBottom(), bounds.RightBottom()));
		return;
	}

	float offset = _BorderSize() - _BorderSize(border);
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

	SetFlags(_ModifyFlags(Flags(), border));
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

	if (fHorizontalScrollBar != NULL)
		fHorizontalScrollBar->SetBorderHighlighted(state);
	if (fVerticalScrollBar != NULL)
		fVerticalScrollBar->SetBorderHighlighted(state);

	BRect bounds = Bounds();
	if (be_control_look != NULL)
		bounds.InsetBy(1, 1);

	Invalidate(BRect(bounds.left, bounds.top, bounds.right, bounds.top));
	Invalidate(BRect(bounds.left, bounds.top + 1, bounds.left,
		bounds.bottom - 1));
	Invalidate(BRect(bounds.right, bounds.top + 1, bounds.right,
		bounds.bottom - 1));
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
		target->MoveTo(_BorderSize(), _BorderSize());
		BRect innerFrame = _InnerFrame();
		target->ResizeTo(innerFrame.Width() - 1, innerFrame.Height() - 1);
		target->TargetedByScrollView(this);

		AddChild(target, ChildAt(0));
			// This way, we are making sure that the target will
			// be added top most in the list (which is important
			// for unarchiving)
	}
}


BView*
BScrollView::Target() const
{
	return fTarget;
}


// #pragma mark -



BHandler*
BScrollView::ResolveSpecifier(BMessage* msg, int32 index, BMessage* specifier,
	int32 form, const char* property)
{
	return BView::ResolveSpecifier(msg, index, specifier, form, property);
}


status_t
BScrollView::GetSupportedSuites(BMessage *data)
{
	return BView::GetSupportedSuites(data);
}


status_t
BScrollView::Perform(perform_code code, void* _data)
{
	switch (code) {
		case PERFORM_CODE_MIN_SIZE:
			((perform_data_min_size*)_data)->return_value
				= BScrollView::MinSize();
			return B_OK;
		case PERFORM_CODE_MAX_SIZE:
			((perform_data_max_size*)_data)->return_value
				= BScrollView::MaxSize();
			return B_OK;
		case PERFORM_CODE_PREFERRED_SIZE:
			((perform_data_preferred_size*)_data)->return_value
				= BScrollView::PreferredSize();
			return B_OK;
		case PERFORM_CODE_LAYOUT_ALIGNMENT:
			((perform_data_layout_alignment*)_data)->return_value
				= BScrollView::LayoutAlignment();
			return B_OK;
		case PERFORM_CODE_HAS_HEIGHT_FOR_WIDTH:
			((perform_data_has_height_for_width*)_data)->return_value
				= BScrollView::HasHeightForWidth();
			return B_OK;
		case PERFORM_CODE_GET_HEIGHT_FOR_WIDTH:
		{
			perform_data_get_height_for_width* data
				= (perform_data_get_height_for_width*)_data;
			BScrollView::GetHeightForWidth(data->width, &data->min, &data->max,
				&data->preferred);
			return B_OK;
		}
		case PERFORM_CODE_SET_LAYOUT:
		{
			perform_data_set_layout* data = (perform_data_set_layout*)_data;
			BScrollView::SetLayout(data->layout);
			return B_OK;
		}
		case PERFORM_CODE_LAYOUT_INVALIDATED:
		{
			perform_data_layout_invalidated* data
				= (perform_data_layout_invalidated*)_data;
			BScrollView::LayoutInvalidated(data->descendants);
			return B_OK;
		}
		case PERFORM_CODE_DO_LAYOUT:
		{
			BScrollView::DoLayout();
			return B_OK;
		}
	}

	return BView::Perform(code, _data);
}


// #pragma mark -


void
BScrollView::_Init(bool horizontal, bool vertical)
{
	fHorizontalScrollBar = NULL;
	fVerticalScrollBar = NULL;
	fHighlighted = false;

	if (be_control_look != NULL)
		SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	if (horizontal) {
		fHorizontalScrollBar = new BScrollBar(BRect(0, 0, 14, 14), "_HSB_",
			fTarget, 0, 1000, B_HORIZONTAL);
		AddChild(fHorizontalScrollBar);
	}

	if (vertical) {
		fVerticalScrollBar = new BScrollBar(BRect(0, 0, 14, 14), "_VSB_",
			fTarget, 0, 1000, B_VERTICAL);
		AddChild(fVerticalScrollBar);
	}
	
	BRect targetFrame;
	if (fTarget) {
		// layout target and add it
		fTarget->TargetedByScrollView(this);
		fTarget->MoveTo(B_ORIGIN);

		if (fBorder != B_NO_BORDER)
			fTarget->MoveBy(_BorderSize(), _BorderSize());

		AddChild(fTarget);
		targetFrame = fTarget->Frame();
	} else {
		// no target specified
		targetFrame = Bounds();
		if (horizontal)
			targetFrame.bottom -= B_H_SCROLL_BAR_HEIGHT + 1;
		if (vertical)
			targetFrame.right -= B_V_SCROLL_BAR_WIDTH + 1;
		if (fBorder == B_FANCY_BORDER) {
			targetFrame.bottom--;
			targetFrame.right--;
		}
	}

	_AlignScrollBars(horizontal, vertical, targetFrame);

	fPreviousWidth = uint16(Bounds().Width());
	fPreviousHeight = uint16(Bounds().Height());
}


float
BScrollView::_BorderSize() const
{
	return _BorderSize(fBorder);
}


BRect
BScrollView::_InnerFrame() const
{
	BRect frame = Bounds();

	float borderSize = _BorderSize();
	frame.InsetBy(borderSize, borderSize);

	if (fHorizontalScrollBar != NULL) {
		frame.bottom -= B_H_SCROLL_BAR_HEIGHT;
		if (borderSize == 0)
			frame.bottom--;
	}
	if (fVerticalScrollBar != NULL) {
		frame.right -= B_V_SCROLL_BAR_WIDTH;
		if (borderSize == 0)
			frame.right--;
	}

	return frame;
}


BSize
BScrollView::_ComputeSize(BSize targetSize) const
{
	BRect frame = _ComputeFrame(
		BRect(0, 0, targetSize.width, targetSize.height));

	return BSize(frame.Width(), frame.Height());
}


BRect
BScrollView::_ComputeFrame(BRect targetRect) const
{
	return _ComputeFrame(targetRect, fHorizontalScrollBar != NULL,
		fVerticalScrollBar != NULL, fBorder);
}


void
BScrollView::_AlignScrollBars(bool horizontal, bool vertical, BRect targetFrame)
{
	if (horizontal) {
		BRect rect = targetFrame;
		rect.top = rect.bottom + 1;
		rect.bottom = rect.top + B_H_SCROLL_BAR_HEIGHT;
		if (fBorder != B_NO_BORDER || vertical) {
			// extend scrollbar so that it overlaps one pixel with vertical
			// scrollbar
			rect.right++;
		}

		if (fBorder != B_NO_BORDER) {
			// the scrollbar draws part of the surrounding frame on the left
			rect.left--;
		}

		fHorizontalScrollBar->MoveTo(rect.left, rect.top);
		fHorizontalScrollBar->ResizeTo(rect.Width(), rect.Height());
	}

	if (vertical) {
		BRect rect = targetFrame;
		rect.left = rect.right + 1;
		rect.right = rect.left + B_V_SCROLL_BAR_WIDTH;
		if (fBorder != B_NO_BORDER || horizontal) {
			// extend scrollbar so that it overlaps one pixel with vertical
			// scrollbar
			rect.bottom++;
		}

		if (fBorder != B_NO_BORDER) {
			// the scrollbar draws part of the surrounding frame on the left
			rect.top--;
		}

		fVerticalScrollBar->MoveTo(rect.left, rect.top);
		fVerticalScrollBar->ResizeTo(rect.Width(), rect.Height());
	}
}


/*!	This static method is used to calculate the frame that the
	ScrollView will cover depending on the frame of its target
	and which border style is used.
	It is used in the constructor and at other places.
*/
/*static*/ BRect
BScrollView::_ComputeFrame(BRect frame, bool horizontal, bool vertical,
	border_style border)
{
	if (vertical)
		frame.right += B_V_SCROLL_BAR_WIDTH;
	if (horizontal)
		frame.bottom += B_H_SCROLL_BAR_HEIGHT;

	float borderSize = _BorderSize(border);
	frame.InsetBy(-borderSize, -borderSize);

	if (borderSize == 0) {
		if (vertical)
			frame.right++;
		if (horizontal)
			frame.bottom++;
	}

	return frame;
}


/*static*/ BRect
BScrollView::_ComputeFrame(BView *target, bool horizontal, bool vertical,
	border_style border)
{
	return _ComputeFrame(target != NULL ? target->Frame()
		: BRect(0, 0, 16, 16), horizontal, vertical, border);
}


/*! This method returns the size of the specified border.
*/
/*static*/ float
BScrollView::_BorderSize(border_style border)
{
	if (border == B_FANCY_BORDER)
		return kFancyBorderSize;
	if (border == B_PLAIN_BORDER)
		return kPlainBorderSize;

	return 0;
}


/*!	This method changes the "flags" argument as passed on to
	the BView constructor.
*/
/*static*/ int32
BScrollView::_ModifyFlags(int32 flags, border_style border)
{
	// We either need B_FULL_UPDATE_ON_RESIZE or
	// B_FRAME_EVENTS if we have to draw a border
	if (border != B_NO_BORDER)
		return flags | B_WILL_DRAW | (flags & B_FULL_UPDATE_ON_RESIZE ? 0 : B_FRAME_EVENTS);

	return flags & ~(B_WILL_DRAW | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE);
}


//	#pragma mark -


BScrollView &
BScrollView::operator=(const BScrollView &)
{
	return *this;
}


void BScrollView::_ReservedScrollView1() {}
void BScrollView::_ReservedScrollView2() {}
void BScrollView::_ReservedScrollView3() {}
void BScrollView::_ReservedScrollView4() {}

