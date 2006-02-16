/*
 * Copyright (c) 2001-2006, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Stephan Aßmus <superstippi@gmx.de>
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <Box.h>
#include <Message.h>
#include <Region.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


BBox::BBox(BRect frame, const char *name, uint32 resizingMode, uint32 flags,
		border_style border)
	: BView(frame, name, resizingMode, flags | B_FRAME_EVENTS),
	fStyle(border)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	_InitObject();
}


BBox::BBox(BMessage *archive)
	: BView(archive)
{
	_InitObject(archive);
}


BBox::~BBox()
{
	_ClearLabel();
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
	BView::Archive(archive, deep);

	if (fLabel)
		 archive->AddString("_label", fLabel);

	if (fLabelView)
		 archive->AddBool("_lblview", true);

	if (fStyle != B_FANCY_BORDER)
		archive->AddInt32("_style", fStyle);

	return B_OK;
}


void
BBox::SetBorder(border_style border)
{
	fStyle = border;

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
	float labelHeight = 0;

	if (fLabel != NULL)
		labelHeight = fLabelBox->Height();
	else if (fLabelView != NULL)
		labelHeight = fLabelView->Bounds().Height();

	return ceilf(labelHeight / 2.0f);
}


//! This function is not part of the R5 API and is not yet finalized yet
BRect
BBox::InnerFrame()
{
	float borderSize = Border() == B_FANCY_BORDER ? 2.0f
		: Border() == B_PLAIN_BORDER ? 1.0f : 0.0f;
	float labelHeight = 0.0f;

	if (fLabel != NULL) {
		// fLabelBox doesn't contain the font's descent, but we want it here
		font_height fontHeight;
		GetFontHeight(&fontHeight);

		labelHeight = ceilf(fontHeight.ascent + fontHeight.descent);
	} else if (fLabelView != NULL)
		labelHeight = fLabelView->Bounds().Height();

	BRect rect = Bounds().InsetByCopy(borderSize, borderSize);
	if (labelHeight)
		rect.top = Bounds().top + labelHeight;

	return rect;
}


void
BBox::SetLabel(const char *string)
{ 
	_ClearLabel();

	if (string) {
		font_height fontHeight;
		GetFontHeight(&fontHeight);

		fLabel = strdup(string);

		// leave 6 pixels of the frame, and have a gap of 4 pixels between
		// the frame and the text on both sides
		fLabelBox = new BRect(6.0f, 0, StringWidth(string) + 14.0f,
			ceilf(fontHeight.ascent));
	}

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
	PushState();

	BRect labelBox = BRect(0, 0, 0, 0);
	if (fLabel != NULL) {
		labelBox = *fLabelBox;
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
	if (Parent()) {
		SetViewColor(Parent()->ViewColor());
		SetLowColor(Parent()->ViewColor());
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
	
		int32 borderSize = fStyle == B_PLAIN_BORDER ? 0 : 1;
	
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
	BView::ResizeToPreferred();
}


void
BBox::GetPreferredSize(float *_width, float *_height)
{
	float width, height;
	bool label = true;

	// acount for label
	if (fLabelView) {
		fLabelView->GetPreferredSize(&width, &height);
		width += 10.0;
			// the label view is placed 10 pixels from the left
	} else if (fLabel) {
		font_height fh;
		GetFontHeight(&fh);
		width += ceilf(StringWidth(fLabel));
		height += ceilf(fh.ascent + fh.descent);
	} else {
		label = false;
		width = 0;
		height = 0;
	}

	// acount for border
	switch (fStyle) {
		case B_NO_BORDER:
			break;
		case B_PLAIN_BORDER:
			// label: (1 pixel for border + 1 pixel for padding) * 2
			// no label: (1 pixel for border) * 2 + 1 pixel for padding
			width += label ? 4 : 3;
			// label: 1 pixel for bottom border + 1 pixel for padding
			// no label: (1 pixel for border) * 2 + 1 pixel for padding
			height += label ? 2 : 3;
			break;
		case B_FANCY_BORDER:
			// label: (2 pixel for border + 1 pixel for padding) * 2
			// no label: (2 pixel for border) * 2 + 1 pixel for padding
			width += label ? 6 : 5;
			// label: 2 pixel for bottom border + 1 pixel for padding
			// no label: (2 pixel for border) * 2 + 1 pixel for padding
			height += label ? 3 : 5;
			break;
	}
	// NOTE: children are ignored, you can use BBox::GetPreferredSize()
	// to get the minimum size of this object, then add the size
	// of your child(ren) plus inner padding for the final size

	if (_width)
		*_width = width;
	if (_height)
		*_height = height;
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
BBox::Perform(perform_code d, void *arg)
{
	return BView::Perform(d, arg);
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
	fLabelBox = NULL;

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

	rgb_color light = tint_color(ViewColor(), B_LIGHTEN_MAX_TINT);
	rgb_color shadow = tint_color(ViewColor(), B_DARKEN_3_TINT);

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


void
BBox::_DrawFancy(BRect labelBox)
{
	BRect rect = Bounds();
	rect.top += TopBorderOffset();

	rgb_color light = tint_color(ViewColor(), B_LIGHTEN_MAX_TINT);
	rgb_color shadow = tint_color(ViewColor(), B_DARKEN_3_TINT);

	BeginLineArray(8);
		AddLine(BPoint(rect.left, rect.bottom),
				BPoint(rect.left, rect.top), shadow);
		AddLine(BPoint(rect.left + 1.0f, rect.top),
				BPoint(rect.right, rect.top), shadow);
		AddLine(BPoint(rect.left + 1.0f, rect.bottom),
				BPoint(rect.right, rect.bottom), light);
		AddLine(BPoint(rect.right, rect.bottom - 1.0f),
				BPoint(rect.right, rect.top + 1.0f), light);

		rect.InsetBy(1.0, 1.0);

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


void
BBox::_ClearLabel()
{
	fBounds.top = 0;

	if (fLabel) {
		delete fLabelBox;
		free(fLabel);
		fLabel = NULL;
		fLabelBox = NULL;
	} else if (fLabelView) {
		fLabelView->RemoveSelf();
		delete fLabelView;
		fLabelView = NULL;
	}
}
