/*
 * Copyright (c) 2001-2009, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Stephan Aßmus <superstippi@gmx.de>
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <Box.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ControlLook.h>
#include <Layout.h>
#include <LayoutUtils.h>
#include <Message.h>
#include <Region.h>

#include <binary_compatibility/Interface.h>


struct BBox::LayoutData {
	LayoutData()
		: valid(false)
	{
	}

	BRect	label_box;		// label box (label string or label view); in case
							// of a label string not including descent
	BRect	insets;			// insets induced by border and label
	BSize	min;
	BSize	max;
	BSize	preferred;
	bool	valid;			// validity the other fields
};


BBox::BBox(BRect frame, const char *name, uint32 resizingMode, uint32 flags,
		border_style border)
	: BView(frame, name, resizingMode, flags  | B_WILL_DRAW | B_FRAME_EVENTS),
	  fStyle(border)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	_InitObject();
}


BBox::BBox(const char* name, uint32 flags, border_style border, BView* child)
	: BView(name, flags | B_WILL_DRAW | B_FRAME_EVENTS),
	  fStyle(border)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	_InitObject();

	if (child)
		AddChild(child);
}


BBox::BBox(border_style border, BView* child)
	: BView(NULL, B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP),
	  fStyle(border)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	_InitObject();

	if (child)
		AddChild(child);
}


BBox::BBox(BMessage *archive)
	: BView(archive),
	  fStyle(B_FANCY_BORDER)
{
	_InitObject(archive);
}


BBox::~BBox()
{
	_ClearLabel();

	delete fLayoutData;
}


BArchivable *
BBox::Instantiate(BMessage *archive)
{
	if (validate_instantiation(archive, "BBox"))
		return new BBox(archive);

	return NULL;
}


status_t
BBox::Archive(BMessage *archive, bool deep) const
{
	status_t ret = BView::Archive(archive, deep);

	if (fLabel && ret == B_OK)
		 ret = archive->AddString("_label", fLabel);

	if (fLabelView && ret == B_OK)
		 ret = archive->AddBool("_lblview", true);

	if (fStyle != B_FANCY_BORDER && ret == B_OK)
		ret = archive->AddInt32("_style", fStyle);

	return ret;
}


void
BBox::SetBorder(border_style border)
{
	if (border == fStyle)
		return;

	fStyle = border;

	InvalidateLayout();

	if (Window() != NULL && LockLooper()) {
		Invalidate();
		UnlockLooper();
	}
}


border_style
BBox::Border() const
{
	return fStyle;
}


//! This function is not part of the R5 API and is not yet finalized yet
float
BBox::TopBorderOffset()
{
	_ValidateLayoutData();

	if (fLabel != NULL || fLabelView != NULL)
		return fLayoutData->label_box.Height() / 2;

	return 0;
}


//! This function is not part of the R5 API and is not yet finalized yet
BRect
BBox::InnerFrame()
{
	_ValidateLayoutData();

	BRect frame(Bounds());
	frame.left += fLayoutData->insets.left;
	frame.top += fLayoutData->insets.top;
	frame.right -= fLayoutData->insets.right;
	frame.bottom -= fLayoutData->insets.bottom;

	return frame;
}


void
BBox::SetLabel(const char *string)
{
	_ClearLabel();

	if (string)
		fLabel = strdup(string);

	InvalidateLayout();

	if (Window())
		Invalidate();
}


status_t
BBox::SetLabel(BView *viewLabel)
{
	_ClearLabel();

	if (viewLabel) {
		fLabelView = viewLabel;
		fLabelView->MoveTo(10.0f, 0.0f);
		AddChild(fLabelView, ChildAt(0));
	}

	InvalidateLayout();

	if (Window())
		Invalidate();

	return B_OK;
}


const char *
BBox::Label() const
{
	return fLabel;
}


BView *
BBox::LabelView() const
{
	return fLabelView;
}


void
BBox::Draw(BRect updateRect)
{
	_ValidateLayoutData();

	PushState();

	BRect labelBox = BRect(0, 0, 0, 0);
	if (fLabel != NULL) {
		labelBox = fLayoutData->label_box;
		BRegion update(updateRect);
		update.Exclude(labelBox);

		ConstrainClippingRegion(&update);
	} else if (fLabelView != NULL)
		labelBox = fLabelView->Bounds();

	switch (fStyle) {
		case B_FANCY_BORDER:
			_DrawFancy(labelBox);
			break;

		case B_PLAIN_BORDER:
			_DrawPlain(labelBox);
			break;

		default:
			break;
	}

	if (fLabel) {
		ConstrainClippingRegion(NULL);

		font_height fontHeight;
		GetFontHeight(&fontHeight);

		SetHighColor(0, 0, 0);
		DrawString(fLabel, BPoint(10.0f, ceilf(fontHeight.ascent)));
	}

	PopState();
}


void
BBox::AttachedToWindow()
{
	BView* parent = Parent();
	if (parent != NULL) {
		// inherit the color from parent
		rgb_color color = parent->ViewColor();
		if (color == B_TRANSPARENT_COLOR)
			color = ui_color(B_PANEL_BACKGROUND_COLOR);

		SetViewColor(color);
		SetLowColor(color);
	}

	// The box could have been resized in the mean time
	fBounds = Bounds();
}


void
BBox::DetachedFromWindow()
{
	BView::DetachedFromWindow();
}


void
BBox::AllAttached()
{
	BView::AllAttached();
}


void
BBox::AllDetached()
{
	BView::AllDetached();
}


void
BBox::FrameResized(float width, float height)
{
	BRect bounds(Bounds());

	// invalidate the regions that the app_server did not
	// (for removing the previous or drawing the new border)
	if (fStyle != B_NO_BORDER) {
		// TODO: this must be made part of the be_control_look stuff!
		int32 borderSize = fStyle == B_PLAIN_BORDER ? 0 : 2;

		BRect invalid(bounds);
		if (fBounds.right < bounds.right) {
			// enlarging
			invalid.left = fBounds.right - borderSize;
			invalid.right = fBounds.right;

			Invalidate(invalid);
		} else if (fBounds.right > bounds.right) {
			// shrinking
			invalid.left = bounds.right - borderSize;

			Invalidate(invalid);
		}

		invalid = bounds;
		if (fBounds.bottom < bounds.bottom) {
			// enlarging
			invalid.top = fBounds.bottom - borderSize;
			invalid.bottom = fBounds.bottom;

			Invalidate(invalid);
		} else if (fBounds.bottom > bounds.bottom) {
			// shrinking
			invalid.top = bounds.bottom - borderSize;

			Invalidate(invalid);
		}
	}

	fBounds.right = bounds.right;
	fBounds.bottom = bounds.bottom;
}


void
BBox::MessageReceived(BMessage *message)
{
	BView::MessageReceived(message);
}


void
BBox::MouseDown(BPoint point)
{
	BView::MouseDown(point);
}


void
BBox::MouseUp(BPoint point)
{
	BView::MouseUp(point);
}


void
BBox::WindowActivated(bool active)
{
	BView::WindowActivated(active);
}


void
BBox::MouseMoved(BPoint point, uint32 transit, const BMessage *message)
{
	BView::MouseMoved(point, transit, message);
}


void
BBox::FrameMoved(BPoint newLocation)
{
	BView::FrameMoved(newLocation);
}


BHandler *
BBox::ResolveSpecifier(BMessage *message, int32 index,
	BMessage *specifier, int32 what,
	const char *property)
{
	return BView::ResolveSpecifier(message, index, specifier, what, property);
}


void
BBox::ResizeToPreferred()
{
	float width, height;
	GetPreferredSize(&width, &height);

	// make sure the box don't get smaller than it already is
	if (width < Bounds().Width())
		width = Bounds().Width();
	if (height < Bounds().Height())
		height = Bounds().Height();

	BView::ResizeTo(width, height);
}


void
BBox::GetPreferredSize(float *_width, float *_height)
{
	_ValidateLayoutData();

	if (_width)
		*_width = fLayoutData->preferred.width;
	if (_height)
		*_height = fLayoutData->preferred.height;
}


void
BBox::MakeFocus(bool focused)
{
	BView::MakeFocus(focused);
}


status_t
BBox::GetSupportedSuites(BMessage *message)
{
	return BView::GetSupportedSuites(message);
}


status_t
BBox::Perform(perform_code code, void* _data)
{
	switch (code) {
		case PERFORM_CODE_MIN_SIZE:
			((perform_data_min_size*)_data)->return_value
				= BBox::MinSize();
			return B_OK;
		case PERFORM_CODE_MAX_SIZE:
			((perform_data_max_size*)_data)->return_value
				= BBox::MaxSize();
			return B_OK;
		case PERFORM_CODE_PREFERRED_SIZE:
			((perform_data_preferred_size*)_data)->return_value
				= BBox::PreferredSize();
			return B_OK;
		case PERFORM_CODE_LAYOUT_ALIGNMENT:
			((perform_data_layout_alignment*)_data)->return_value
				= BBox::LayoutAlignment();
			return B_OK;
		case PERFORM_CODE_HAS_HEIGHT_FOR_WIDTH:
			((perform_data_has_height_for_width*)_data)->return_value
				= BBox::HasHeightForWidth();
			return B_OK;
		case PERFORM_CODE_GET_HEIGHT_FOR_WIDTH:
		{
			perform_data_get_height_for_width* data
				= (perform_data_get_height_for_width*)_data;
			BBox::GetHeightForWidth(data->width, &data->min, &data->max,
				&data->preferred);
			return B_OK;
}
		case PERFORM_CODE_SET_LAYOUT:
		{
			perform_data_set_layout* data = (perform_data_set_layout*)_data;
			BBox::SetLayout(data->layout);
			return B_OK;
		}
		case PERFORM_CODE_LAYOUT_INVALIDATED:
		{
			perform_data_layout_invalidated* data
				= (perform_data_layout_invalidated*)_data;
			BBox::LayoutInvalidated(data->descendants);
			return B_OK;
		}
		case PERFORM_CODE_DO_LAYOUT:
		{
			BBox::DoLayout();
			return B_OK;
		}
	}

	return BView::Perform(code, _data);
}


BSize
BBox::MinSize()
{
	_ValidateLayoutData();

	BSize size = (GetLayout() ? GetLayout()->MinSize() : fLayoutData->min);
	return BLayoutUtils::ComposeSize(ExplicitMinSize(), size);
}


BSize
BBox::MaxSize()
{
	_ValidateLayoutData();

	BSize size = (GetLayout() ? GetLayout()->MaxSize() : fLayoutData->max);
	return BLayoutUtils::ComposeSize(ExplicitMaxSize(), size);
}


BSize
BBox::PreferredSize()
{
	_ValidateLayoutData();

	BSize size = (GetLayout() ? GetLayout()->PreferredSize()
		: fLayoutData->preferred);
	return BLayoutUtils::ComposeSize(ExplicitPreferredSize(), size);
}


void
BBox::LayoutInvalidated(bool descendants)
{
	fLayoutData->valid = false;
}


void
BBox::DoLayout()
{
	// Bail out, if we shan't do layout.
	if (!(Flags() & B_SUPPORTS_LAYOUT))
		return;

	bool layouted = GetLayout() ? true : false;

	// If the user set a layout, let the base class version call its
	// hook. In case when we have BView as a label, remove it from child list
	// so it won't be layouted with the rest of views and add it again
	// after that.
	if (layouted) {
		if (fLabelView)
			RemoveChild(fLabelView);

		BView::DoLayout();

		if (fLabelView)
			AddChild(fLabelView, ChildAt(0));
		else
			return;
	}

	_ValidateLayoutData();

	// Even if the user set a layout, restore label view to it's
	// desired position.

	// layout the label view
	if (fLabelView) {
		fLabelView->MoveTo(fLayoutData->label_box.LeftTop());
		fLabelView->ResizeTo(fLayoutData->label_box.Size());
	}

	// If we have layout return here and do not layout the child
	if (layouted)
		return;

	// layout the child
	if (BView* child = _Child()) {
		BRect frame(Bounds());
		frame.left += fLayoutData->insets.left;
		frame.top += fLayoutData->insets.top;
		frame.right -= fLayoutData->insets.right;
		frame.bottom -= fLayoutData->insets.bottom;

		BLayoutUtils::AlignInFrame(child, frame);
	}
}


void BBox::_ReservedBox1() {}
void BBox::_ReservedBox2() {}


BBox &
BBox::operator=(const BBox &)
{
	return *this;
}


void
BBox::_InitObject(BMessage* archive)
{
	fBounds = Bounds();

	fLabel = NULL;
	fLabelView = NULL;
	fLayoutData = new LayoutData;

	int32 flags = 0;

	BFont font(be_bold_font);

	if (!archive || !archive->HasString("_fname"))
		flags = B_FONT_FAMILY_AND_STYLE;

	if (!archive || !archive->HasFloat("_fflt"))
		flags |= B_FONT_SIZE;

	if (flags != 0)
		SetFont(&font, flags);

	if (archive != NULL) {
		const char *string;
		if (archive->FindString("_label", &string) == B_OK)
			SetLabel(string);

		bool fancy;
		int32 style;

		if (archive->FindBool("_style", &fancy) == B_OK)
			fStyle = fancy ? B_FANCY_BORDER : B_PLAIN_BORDER;
		else if (archive->FindInt32("_style", &style) == B_OK)
			fStyle = (border_style)style;

		bool hasLabelView;
		if (archive->FindBool("_lblview", &hasLabelView) == B_OK)
			fLabelView = ChildAt(0);
	}
}


void
BBox::_DrawPlain(BRect labelBox)
{
	BRect rect = Bounds();
	rect.top += TopBorderOffset();

	float lightTint;
	float shadowTint;
	if (be_control_look != NULL) {
		lightTint = B_LIGHTEN_1_TINT;
		shadowTint = B_DARKEN_1_TINT;
	} else {
		lightTint = B_LIGHTEN_MAX_TINT;
		shadowTint = B_DARKEN_3_TINT;
	}

	if (rect.Height() == 0.0 || rect.Width() == 0.0) {
		// used as separator
		rgb_color shadow = tint_color(ViewColor(), B_DARKEN_2_TINT);

		SetHighColor(shadow);
		StrokeLine(rect.LeftTop(),rect.RightBottom());
	} else {
		// used as box
		rgb_color light = tint_color(ViewColor(), lightTint);
		rgb_color shadow = tint_color(ViewColor(), shadowTint);

		BeginLineArray(4);
			AddLine(BPoint(rect.left, rect.bottom),
					BPoint(rect.left, rect.top), light);
			AddLine(BPoint(rect.left + 1.0f, rect.top),
					BPoint(rect.right, rect.top), light);
			AddLine(BPoint(rect.left + 1.0f, rect.bottom),
					BPoint(rect.right, rect.bottom), shadow);
			AddLine(BPoint(rect.right, rect.bottom - 1.0f),
					BPoint(rect.right, rect.top + 1.0f), shadow);
		EndLineArray();
	}
}


void
BBox::_DrawFancy(BRect labelBox)
{
	BRect rect = Bounds();
	rect.top += TopBorderOffset();

	if (be_control_look != NULL) {
		rgb_color base = ViewColor();
		if (rect.Height() == 1.0) {
			// used as horizontal separator
			be_control_look->DrawGroupFrame(this, rect, rect, base,
				BControlLook::B_TOP_BORDER);
		} else if (rect.Width() == 1.0) {
			// used as vertical separator
			be_control_look->DrawGroupFrame(this, rect, rect, base,
				BControlLook::B_LEFT_BORDER);
		} else {
			// used as box
			be_control_look->DrawGroupFrame(this, rect, rect, base);
		}
		return;
	}

	rgb_color light = tint_color(ViewColor(), B_LIGHTEN_MAX_TINT);
	rgb_color shadow = tint_color(ViewColor(), B_DARKEN_3_TINT);

	if (rect.Height() == 1.0) {
		// used as horizontal separator
		BeginLineArray(2);
			AddLine(BPoint(rect.left, rect.top),
					BPoint(rect.right, rect.top), shadow);
			AddLine(BPoint(rect.left, rect.bottom),
					BPoint(rect.right, rect.bottom), light);
		EndLineArray();
	} else if (rect.Width() == 1.0) {
		// used as vertical separator
		BeginLineArray(2);
			AddLine(BPoint(rect.left, rect.top),
					BPoint(rect.left, rect.bottom), shadow);
			AddLine(BPoint(rect.right, rect.top),
					BPoint(rect.right, rect.bottom), light);
		EndLineArray();
	} else {
		// used as box
		BeginLineArray(8);
			AddLine(BPoint(rect.left, rect.bottom - 1.0),
					BPoint(rect.left, rect.top), shadow);
			AddLine(BPoint(rect.left + 1.0, rect.top),
					BPoint(rect.right - 1.0, rect.top), shadow);
			AddLine(BPoint(rect.left, rect.bottom),
					BPoint(rect.right, rect.bottom), light);
			AddLine(BPoint(rect.right, rect.bottom - 1.0),
					BPoint(rect.right, rect.top), light);

			rect.InsetBy(1.0, 1.0);

			AddLine(BPoint(rect.left, rect.bottom - 1.0),
					BPoint(rect.left, rect.top), light);
			AddLine(BPoint(rect.left + 1.0, rect.top),
					BPoint(rect.right - 1.0, rect.top), light);
			AddLine(BPoint(rect.left, rect.bottom),
					BPoint(rect.right, rect.bottom), shadow);
			AddLine(BPoint(rect.right, rect.bottom - 1.0),
					BPoint(rect.right, rect.top), shadow);
		EndLineArray();
	}
}


void
BBox::_ClearLabel()
{
	fBounds.top = 0;

	if (fLabel) {
		free(fLabel);
		fLabel = NULL;
	} else if (fLabelView) {
		fLabelView->RemoveSelf();
		delete fLabelView;
		fLabelView = NULL;
	}
}


BView*
BBox::_Child() const
{
	for (int32 i = 0; BView* view = ChildAt(i); i++) {
		if (view != fLabelView)
			return view;
	}

	return NULL;
}


void
BBox::_ValidateLayoutData()
{
	if (fLayoutData->valid)
		return;

	// compute the label box, width and height
	bool label = true;
	float labelHeight = 0;	// height of the label (pixel count)
	if (fLabel) {
		// leave 6 pixels of the frame, and have a gap of 4 pixels between
		// the frame and the text on either side
		font_height fontHeight;
		GetFontHeight(&fontHeight);
		fLayoutData->label_box.Set(6.0f, 0, 14.0f + StringWidth(fLabel),
			ceilf(fontHeight.ascent));
		labelHeight = ceilf(fontHeight.ascent + fontHeight.descent) + 1;
	} else if (fLabelView) {
		// the label view is placed at (0, 10) at its preferred size
		BSize size = fLabelView->PreferredSize();
		fLayoutData->label_box.Set(10, 0, 10 + size.width, size.height);
		labelHeight = size.height + 1;
	} else {
		label = false;
	}

	// border
	switch (fStyle) {
		case B_PLAIN_BORDER:
			fLayoutData->insets.Set(1, 1, 1, 1);
			break;
		case B_FANCY_BORDER:
			fLayoutData->insets.Set(3, 3, 3, 3);
			break;
		case B_NO_BORDER:
		default:
			fLayoutData->insets.Set(0, 0, 0, 0);
			break;
	}

	// if there's a label, the top inset will be dictated by the label
	if (label && labelHeight > fLayoutData->insets.top)
		fLayoutData->insets.top = labelHeight;

	// total number of pixel the border adds
	float addWidth = fLayoutData->insets.left + fLayoutData->insets.right;
	float addHeight = fLayoutData->insets.top + fLayoutData->insets.bottom;

	// compute the minimal width induced by the label
	float minWidth;
	if (label)
		minWidth = fLayoutData->label_box.right + fLayoutData->insets.right;
	else
		minWidth = addWidth - 1;

	// finally consider the child constraints, if we shall support layout
	BView* child = _Child();
	if (child && (Flags() & B_SUPPORTS_LAYOUT)) {
		BSize min = child->MinSize();
		BSize max = child->MaxSize();
		BSize preferred = child->PreferredSize();

		min.width += addWidth;
		min.height += addHeight;
		preferred.width += addWidth;
		preferred.height += addHeight;
		max.width = BLayoutUtils::AddDistances(max.width, addWidth - 1);
		max.height = BLayoutUtils::AddDistances(max.height, addHeight - 1);

		if (min.width < minWidth)
			min.width = minWidth;
		BLayoutUtils::FixSizeConstraints(min, max, preferred);

		fLayoutData->min = min;
		fLayoutData->max = max;
		fLayoutData->preferred = preferred;
	} else {
		fLayoutData->min.Set(minWidth, addHeight - 1);
		fLayoutData->max.Set(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED);
		fLayoutData->preferred = fLayoutData->min;
	}

	fLayoutData->valid = true;
	ResetLayoutInvalidation();
}


#if __GNUC__ == 2


extern "C" void
InvalidateLayout__4BBoxb(BBox* box)
{
	box->Perform(PERFORM_CODE_LAYOUT_CHANGED, NULL);
}


#endif
