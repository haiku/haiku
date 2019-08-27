/*
 * Copyright 2001-2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2001-2009, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "SeparatorView.h"

#include <new>

#include <math.h>
#include <stdio.h>

#include <ControlLook.h>
#include <LayoutUtils.h>
#include <Region.h>


static const float kMinBorderLength = 5.0f;


// TODO: Actually implement alignment support!
// TODO: More testing, especially archiving.


BSeparatorView::BSeparatorView(orientation orientation, border_style border)
	:
	BView(NULL, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE)
{
	_Init(NULL, NULL, orientation, BAlignment(B_ALIGN_HORIZONTAL_CENTER,
		B_ALIGN_VERTICAL_CENTER), border);
}


BSeparatorView::BSeparatorView(const char* name, const char* label,
	orientation orientation, border_style border, const BAlignment& alignment)
	:
	BView(name, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE)
{
	_Init(label, NULL, orientation, alignment, border);
}


BSeparatorView::BSeparatorView(const char* name, BView* labelView,
	orientation orientation, border_style border, const BAlignment& alignment)
	:
	BView(name, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE)
{
	_Init(NULL, labelView, orientation, alignment, border);
}


BSeparatorView::BSeparatorView(const char* label,
	orientation orientation, border_style border, const BAlignment& alignment)
	:
	BView("", B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE)
{
	_Init(label, NULL, orientation, alignment, border);
}


BSeparatorView::BSeparatorView(BView* labelView,
	orientation orientation, border_style border, const BAlignment& alignment)
	:
	BView("", B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE)
{
	_Init(NULL, labelView, orientation, alignment, border);
}


BSeparatorView::BSeparatorView(BMessage* archive)
	:
	BView(archive),
	fLabel(),
	fLabelView(NULL),
	fOrientation(B_HORIZONTAL),
	fAlignment(BAlignment(B_ALIGN_HORIZONTAL_CENTER,
		B_ALIGN_VERTICAL_CENTER)),
	fBorder(B_FANCY_BORDER)
{
	// NULL archives will be caught by BView.

	const char* label;
	if (archive->FindString("_labelview", &label) == B_OK) {
		fLabelView = FindView(label);
	} else if (archive->FindString("_label", &label) == B_OK) {
		fLabel.SetTo(label);
	}

	int32 orientation;
	if (archive->FindInt32("_orientation", &orientation) == B_OK)
		fOrientation = (enum orientation)orientation;

	int32 hAlignment;
	int32 vAlignment;
	if (archive->FindInt32("_halignment", &hAlignment) == B_OK
		&& archive->FindInt32("_valignment", &vAlignment) == B_OK) {
		fAlignment.horizontal = (alignment)hAlignment;
		fAlignment.vertical = (vertical_alignment)vAlignment;
	}

	int32 borderStyle;
	if (archive->FindInt32("_border", &borderStyle) != B_OK)
		fBorder = (border_style)borderStyle;
}


BSeparatorView::~BSeparatorView()
{
}


// #pragma mark - Archiving


BArchivable*
BSeparatorView::Instantiate(BMessage* archive)
{
	if (validate_instantiation(archive, "BSeparatorView"))
		return new (std::nothrow) BSeparatorView(archive);

	return NULL;
}


status_t
BSeparatorView::Archive(BMessage* into, bool deep) const
{
	// TODO: Test this.
	status_t result = BView::Archive(into, deep);
	if (result != B_OK)
		return result;

	if (fLabelView != NULL)
		result = into->AddString("_labelview", fLabelView->Name());
	else
		result = into->AddString("_label", fLabel.String());

	if (result == B_OK)
		result = into->AddInt32("_orientation", fOrientation);

	if (result == B_OK)
		result = into->AddInt32("_halignment", fAlignment.horizontal);

	if (result == B_OK)
		result = into->AddInt32("_valignment", fAlignment.vertical);

	if (result == B_OK)
		result = into->AddInt32("_border", fBorder);

	return result;
}


// #pragma mark -


void
BSeparatorView::Draw(BRect updateRect)
{
	rgb_color bgColor = ui_color(B_PANEL_BACKGROUND_COLOR);
	rgb_color highColor = ui_color(B_PANEL_TEXT_COLOR);

	if (BView* parent = Parent()) {
		if (parent->ViewColor() != B_TRANSPARENT_COLOR) {
			bgColor = parent->ViewColor();
			highColor = parent->HighColor();
		}
	}

	BRect labelBounds;
	if (fLabelView != NULL) {
		labelBounds = fLabelView->Frame();
	} else if (fLabel.CountChars() > 0) {
		labelBounds = _MaxLabelBounds();
		float labelWidth = StringWidth(fLabel.String());
		if (fOrientation == B_HORIZONTAL) {
			switch (fAlignment.horizontal) {
				case B_ALIGN_LEFT:
				default:
					labelBounds.right = labelBounds.left + labelWidth;
					break;

				case B_ALIGN_RIGHT:
					labelBounds.left = labelBounds.right - labelWidth;
					break;

				case B_ALIGN_CENTER:
					labelBounds.left = (labelBounds.left + labelBounds.right
						- labelWidth) / 2;
					labelBounds.right = labelBounds.left + labelWidth;
					break;
			}
		} else {
			switch (fAlignment.vertical) {
				case B_ALIGN_TOP:
				default:
					labelBounds.bottom = labelBounds.top + labelWidth;
					break;

				case B_ALIGN_BOTTOM:
					labelBounds.top = labelBounds.bottom - labelWidth;
					break;

				case B_ALIGN_MIDDLE:
					labelBounds.top = (labelBounds.top + labelBounds.bottom
						- labelWidth) / 2;
					labelBounds.bottom = labelBounds.top + labelWidth;
					break;
			}
		}
	}

	BRect bounds = Bounds();
	BRegion region(bounds);
	if (labelBounds.IsValid()) {
		region.Exclude(labelBounds);
		ConstrainClippingRegion(&region);
	}

	float borderSize = _BorderSize();
	if (borderSize > 0.0f) {
		if (fOrientation == B_HORIZONTAL) {
			bounds.top = floorf((bounds.top + bounds.bottom + 1 - borderSize)
				/ 2);
			bounds.bottom = bounds.top + borderSize - 1;
			region.Exclude(bounds);
			be_control_look->DrawBorder(this, bounds, updateRect, bgColor,
				fBorder, 0, BControlLook::B_TOP_BORDER);
		} else {
			bounds.left = floorf((bounds.left + bounds.right + 1 - borderSize)
				/ 2);
			bounds.right = bounds.left + borderSize - 1;
			region.Exclude(bounds);
			be_control_look->DrawBorder(this, bounds, updateRect, bgColor,
				fBorder, 0, BControlLook::B_LEFT_BORDER);
		}
		if (labelBounds.IsValid())
			region.Include(labelBounds);

		ConstrainClippingRegion(&region);
	}

	SetLowColor(bgColor);
	FillRect(updateRect, B_SOLID_LOW);

	if (fLabel.CountChars() > 0) {
		font_height fontHeight;
		GetFontHeight(&fontHeight);

		SetHighColor(highColor);

		if (fOrientation == B_HORIZONTAL) {
			DrawString(fLabel.String(), BPoint(labelBounds.left,
				labelBounds.top + ceilf(fontHeight.ascent)));
		} else {
			DrawString(fLabel.String(), BPoint(labelBounds.left
				+ ceilf(fontHeight.ascent), labelBounds.bottom));
		}
	}
}


void
BSeparatorView::GetPreferredSize(float* _width, float* _height)
{
	float width = 0.0f;
	float height = 0.0f;

	if (fLabelView != NULL) {
		fLabelView->GetPreferredSize(&width, &height);
	} else if (fLabel.CountChars() > 0) {
		width = StringWidth(fLabel.String());
		font_height fontHeight;
		GetFontHeight(&fontHeight);
		height = ceilf(fontHeight.ascent) + ceilf(fontHeight.descent);
		if (fOrientation == B_VERTICAL) {
			// swap width and height
			float temp = height;
			height = width;
			width = temp;
		}
	}

	float borderSize = _BorderSize();

	// Add some room for the border
	if (fOrientation == B_HORIZONTAL) {
		width += kMinBorderLength * 2.0f;
		height = max_c(height, borderSize - 1.0f);
	} else {
		height += kMinBorderLength * 2.0f;
		width = max_c(width, borderSize - 1.0f);
	}

	if (_width != NULL)
		*_width = width;

	if (_height != NULL)
		*_height = height;
}


BSize
BSeparatorView::MinSize()
{
	BSize size;
	GetPreferredSize(&size.width, &size.height);
	return BLayoutUtils::ComposeSize(ExplicitMinSize(), size);
}


BSize
BSeparatorView::MaxSize()
{
	BSize size(MinSize());
	if (fOrientation == B_HORIZONTAL)
		size.width = B_SIZE_UNLIMITED;
	else
		size.height = B_SIZE_UNLIMITED;

	return BLayoutUtils::ComposeSize(ExplicitMaxSize(), size);
}


BSize
BSeparatorView::PreferredSize()
{
	BSize size;
	GetPreferredSize(&size.width, &size.height);

	return BLayoutUtils::ComposeSize(ExplicitPreferredSize(), size);
}


// #pragma mark -


void
BSeparatorView::SetOrientation(orientation orientation)
{
	if (orientation == fOrientation)
		return;

	fOrientation = orientation;

	BFont font;
	GetFont(&font);
	if (fOrientation == B_VERTICAL)
		font.SetRotation(90.0f);

	SetFont(&font);

	Invalidate();
	InvalidateLayout();
}


void
BSeparatorView::SetAlignment(const BAlignment& aligment)
{
	if (aligment == fAlignment)
		return;

	fAlignment = aligment;

	Invalidate();
	InvalidateLayout();
}


void
BSeparatorView::SetBorderStyle(border_style border)
{
	if (border == fBorder)
		return;

	fBorder = border;

	Invalidate();
}


void
BSeparatorView::SetLabel(const char* label)
{
	if (label == NULL)
		label = "";

	if (fLabel == label)
		return;

	fLabel.SetTo(label);

	Invalidate();
	InvalidateLayout();
}


void
BSeparatorView::SetLabel(BView* view, bool deletePrevious)
{
	if (fLabelView == view)
		return;

	if (fLabelView != NULL) {
		fLabelView->RemoveSelf();
		if (deletePrevious)
			delete fLabelView;
	}

	fLabelView = view;

	if (fLabelView != NULL)
		AddChild(view);
}


status_t
BSeparatorView::Perform(perform_code code, void* data)
{
	return BView::Perform(code, data);
}


// #pragma mark - protected/private


void
BSeparatorView::DoLayout()
{
	BView::DoLayout();

	if (fLabelView == NULL)
		return;

	BRect bounds = _MaxLabelBounds();
	BRect frame = BLayoutUtils::AlignInFrame(bounds, fLabelView->MaxSize(),
		fAlignment);

	fLabelView->MoveTo(frame.LeftTop());
	fLabelView->ResizeTo(frame.Width(), frame.Height());
}


void
BSeparatorView::_Init(const char* label, BView* labelView,
	orientation orientation, BAlignment alignment, border_style border)
{
	fLabel = "";
	fLabelView = NULL;

	fOrientation = B_HORIZONTAL;
	fAlignment = alignment;
	fBorder = border;

	SetFont(be_bold_font);
	SetViewColor(B_TRANSPARENT_32_BIT);

	SetLabel(label);
	SetLabel(labelView, true);
	SetOrientation(orientation);
}


float
BSeparatorView::_BorderSize() const
{
	// TODO: Get these values from the BControlLook class.
	switch (fBorder) {
		case B_PLAIN_BORDER:
			return 1.0f;

		case B_FANCY_BORDER:
			return 2.0f;

		case B_NO_BORDER:
		default:
			return 0.0f;
	}
}


BRect
BSeparatorView::_MaxLabelBounds() const
{
	BRect bounds = Bounds();
	if (fOrientation == B_HORIZONTAL)
		bounds.InsetBy(kMinBorderLength, 0.0f);
	else
		bounds.InsetBy(0.0f, kMinBorderLength);

	return bounds;
}


// #pragma mark - FBC padding


void BSeparatorView::_ReservedSeparatorView1() {}
void BSeparatorView::_ReservedSeparatorView2() {}
void BSeparatorView::_ReservedSeparatorView3() {}
void BSeparatorView::_ReservedSeparatorView4() {}
void BSeparatorView::_ReservedSeparatorView5() {}
void BSeparatorView::_ReservedSeparatorView6() {}
void BSeparatorView::_ReservedSeparatorView7() {}
void BSeparatorView::_ReservedSeparatorView8() {}
void BSeparatorView::_ReservedSeparatorView9() {}
void BSeparatorView::_ReservedSeparatorView10() {}
