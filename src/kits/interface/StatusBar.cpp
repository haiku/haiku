/*
 * Copyright 2001-2008, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Axel Dörfler, axeld@pinc-software.de
 *		Stephan Aßmus <superstippi@gmx.de>
 */

/*! BStatusBar displays a "percentage-of-completion" gauge. */
#include <StatusBar.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ControlLook.h>
#include <Layout.h>
#include <LayoutUtils.h>
#include <Message.h>
#include <Region.h>

#include <binary_compatibility/Interface.h>


static const rgb_color kDefaultBarColor = {50, 150, 255, 255};


BStatusBar::BStatusBar(BRect frame, const char *name, const char *label,
		const char *trailingLabel)
	:
	BView(frame, name, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW),
	fLabel(label),
	fTrailingLabel(trailingLabel)
{
	_InitObject();
}


BStatusBar::BStatusBar(const char *name, const char *label,
		const char *trailingLabel)
	:
	BView(BRect(0, 0, -1, -1), name, B_FOLLOW_LEFT | B_FOLLOW_TOP, 
		B_WILL_DRAW | B_SUPPORTS_LAYOUT),
	fLabel(label),
	fTrailingLabel(trailingLabel)
{
	_InitObject();
}


BStatusBar::BStatusBar(BMessage *archive)
	:
	BView(archive)
{
	_InitObject();

	archive->FindString("_label", &fLabel);
	archive->FindString("_tlabel", &fTrailingLabel);

	archive->FindString("_text", &fText);
	archive->FindString("_ttext", &fTrailingText);

	float floatValue;
	if (archive->FindFloat("_high", &floatValue) == B_OK) {
		fBarHeight = floatValue;
		fCustomBarHeight = true;
	}

	int32 color;
	if (archive->FindInt32("_bcolor", (int32 *)&color) == B_OK)
		fBarColor = *(rgb_color *)&color;

	if (archive->FindFloat("_val", &floatValue) == B_OK)
		fCurrent = floatValue;
	if (archive->FindFloat("_max", &floatValue) == B_OK)
		fMax = floatValue;
}


BStatusBar::~BStatusBar()
{
}


BArchivable *
BStatusBar::Instantiate(BMessage *archive)
{
	if (validate_instantiation(archive, "BStatusBar"))
		return new BStatusBar(archive);

	return NULL;
}


status_t
BStatusBar::Archive(BMessage *archive, bool deep) const
{
	status_t err = BView::Archive(archive, deep);
	if (err < B_OK)
		return err;

	if (fCustomBarHeight)
		err = archive->AddFloat("_high", fBarHeight);

	if (err == B_OK && fBarColor != kDefaultBarColor)
		err = archive->AddInt32("_bcolor", (const uint32 &)fBarColor);

	if (err == B_OK && fCurrent != 0)
		err = archive->AddFloat("_val", fCurrent);
	if (err == B_OK && fMax != 100 )
		err = archive->AddFloat("_max", fMax);

	if (err == B_OK && fText.Length())
		err = archive->AddString("_text", fText);
	if (err == B_OK && fTrailingText.Length())
		err = archive->AddString("_ttext", fTrailingText);

	if (err == B_OK && fLabel.Length())
		err = archive->AddString("_label", fLabel);
	if (err == B_OK && fTrailingLabel.Length())
		err = archive->AddString ("_tlabel", fTrailingLabel);

	return err;
}


// #pragma mark -


void
BStatusBar::AttachedToWindow()
{
	// resize so that the height fits
	float width, height;
	GetPreferredSize(&width, &height);
	ResizeTo(Bounds().Width(), height);

	SetViewColor(B_TRANSPARENT_COLOR);
	rgb_color lowColor = B_TRANSPARENT_COLOR;

	BView* parent = Parent();
	if (parent != NULL)
		lowColor = parent->ViewColor();

	if (lowColor == B_TRANSPARENT_COLOR)
		lowColor = ui_color(B_PANEL_BACKGROUND_COLOR);

	SetLowColor(lowColor);

	fTextDivider = Bounds().Width();
}


void
BStatusBar::DetachedFromWindow()
{
	BView::DetachedFromWindow();
}


void
BStatusBar::AllAttached()
{
	BView::AllAttached();
}


void
BStatusBar::AllDetached()
{
	BView::AllDetached();
}


// #pragma mark -


void
BStatusBar::WindowActivated(bool state)
{
	BView::WindowActivated(state);
}


void
BStatusBar::MakeFocus(bool state)
{
	BView::MakeFocus(state);
}


// #pragma mark -


void
BStatusBar::GetPreferredSize(float* _width, float* _height)
{
	if (_width) {
		// AttachedToWindow() might not have been called yet
		*_width = ceilf(StringWidth(fLabel.String()))
			+ ceilf(StringWidth(fTrailingLabel.String()))
			+ ceilf(StringWidth(fText.String()))
			+ ceilf(StringWidth(fTrailingText.String()))
			+ 5;
	}

	if (_height) {
		float labelHeight = 0;
		if (_HasText()) {
			font_height fontHeight;
			GetFontHeight(&fontHeight);
			labelHeight = ceilf(fontHeight.ascent + fontHeight.descent) + 6;
		}

		*_height = labelHeight + BarHeight();
	}
}


BSize
BStatusBar::MinSize()
{
	float width, height;
	GetPreferredSize(&width, &height);

	return BLayoutUtils::ComposeSize(ExplicitMinSize(), BSize(width, height));
}


BSize
BStatusBar::MaxSize()
{
	float width, height;
	GetPreferredSize(&width, &height);

	return BLayoutUtils::ComposeSize(ExplicitMaxSize(), 
		BSize(B_SIZE_UNLIMITED, height));
}


BSize
BStatusBar::PreferredSize()
{
	float width, height;
	GetPreferredSize(&width, &height);

	return BLayoutUtils::ComposeSize(ExplicitPreferredSize(),
		BSize(width, height));
}


void
BStatusBar::ResizeToPreferred()
{
	BView::ResizeToPreferred();
}


void
BStatusBar::FrameMoved(BPoint newPosition)
{
	BView::FrameMoved(newPosition);
}


void
BStatusBar::FrameResized(float newWidth, float newHeight)
{
	BView::FrameResized(newWidth, newHeight);
	Invalidate();
}


// #pragma mark -


void
BStatusBar::Draw(BRect updateRect)
{
	rgb_color backgroundColor = LowColor();

	font_height fontHeight;
	GetFontHeight(&fontHeight);
	BRect barFrame = _BarFrame(&fontHeight);
	BRect outerFrame = barFrame.InsetByCopy(-2, -2);

	BRegion background(updateRect);
	background.Exclude(outerFrame);
	FillRegion(&background, B_SOLID_LOW);

	// Draw labels/texts

	BRect rect = outerFrame;
	rect.top = 0;
	rect.bottom = outerFrame.top - 1;

	if (updateRect.Intersects(rect)) {
		// update labels
		BString leftText;
		leftText << fLabel << fText;

		BString rightText;
		rightText << fTrailingText << fTrailingLabel;

		float baseLine = ceilf(fontHeight.ascent) + 1;
		fTextDivider = rect.right;

		BFont font;
		GetFont(&font);

		if (rightText.Length()) {
			font.TruncateString(&rightText, B_TRUNCATE_BEGINNING, rect.Width());
			fTextDivider -= StringWidth(rightText.String());
		}

		if (leftText.Length()) {
			float width = max_c(0.0, fTextDivider - rect.left);
			font.TruncateString(&leftText, B_TRUNCATE_END, width);
		}

		SetHighColor(ui_color(B_CONTROL_TEXT_COLOR));

		if (leftText.Length())
			DrawString(leftText.String(), BPoint(rect.left, baseLine));

		if (rightText.Length())
			DrawString(rightText.String(), BPoint(fTextDivider, baseLine));

	}

	// Draw bar

	if (!updateRect.Intersects(outerFrame))
		return;

	rect = outerFrame;

	if (be_control_look != NULL) {
		be_control_look->DrawStatusBar(this, rect, updateRect,
			backgroundColor, fBarColor, _BarPosition(barFrame));
		return;
	}

	// First bevel
	SetHighColor(tint_color(ui_color ( B_PANEL_BACKGROUND_COLOR ), B_DARKEN_1_TINT));
	StrokeLine(rect.LeftBottom(), rect.LeftTop());
	StrokeLine(rect.RightTop());

	SetHighColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_LIGHTEN_2_TINT));
	StrokeLine(BPoint(rect.left + 1, rect.bottom), rect.RightBottom());
	StrokeLine(BPoint(rect.right, rect.top + 1));

	rect.InsetBy(1, 1);

	// Second bevel
	SetHighColor(tint_color(ui_color ( B_PANEL_BACKGROUND_COLOR ), B_DARKEN_4_TINT));
	StrokeLine(rect.LeftBottom(), rect.LeftTop());
	StrokeLine(rect.RightTop());

	SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	StrokeLine(BPoint(rect.left + 1, rect.bottom), rect.RightBottom());
	StrokeLine(BPoint(rect.right, rect.top + 1));

	rect = barFrame;
	rect.right = _BarPosition(barFrame);

	// draw bar itself

	if (rect.right >= rect.left) {
		// Bevel
		SetHighColor(tint_color(fBarColor, B_LIGHTEN_2_TINT));
		StrokeLine(rect.LeftBottom(), rect.LeftTop());
		StrokeLine(rect.RightTop());

		SetHighColor(tint_color(fBarColor, B_DARKEN_2_TINT));
		StrokeLine(BPoint(rect.left + 1, rect.bottom), rect.RightBottom());
		StrokeLine(BPoint(rect.right, rect.top + 1));

		// filling
		SetHighColor(fBarColor);
		FillRect(rect.InsetByCopy(1, 1));
	}

	if (rect.right < barFrame.right) {
		// empty space
		rect.left = rect.right + 1;
		rect.right = barFrame.right;
		SetHighColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_LIGHTEN_MAX_TINT));
		FillRect(rect);
	}
}


void
BStatusBar::MessageReceived(BMessage *message)
{
	switch(message->what) {
		case B_UPDATE_STATUS_BAR:
		{
			float delta;
			const char *text = NULL, *trailing_text = NULL;

			message->FindFloat("delta", &delta);
			message->FindString("text", &text);
			message->FindString("trailing_text", &trailing_text);

			Update(delta, text, trailing_text);

			break;
		}

		case B_RESET_STATUS_BAR: 
		{
			const char *label = NULL, *trailing_label = NULL;

			message->FindString("label", &label);
			message->FindString("trailing_label", &trailing_label);

			Reset(label, trailing_label);

			break;
		}

		default:
			BView::MessageReceived(message);
			break;
	}
}


void
BStatusBar::MouseDown(BPoint point)
{
	BView::MouseDown(point);
}


void
BStatusBar::MouseUp(BPoint point)
{
	BView::MouseUp(point);
}


void
BStatusBar::MouseMoved(BPoint point, uint32 transit, const BMessage *message)
{
	BView::MouseMoved(point, transit, message);
}


// #pragma mark -


void
BStatusBar::SetBarColor(rgb_color color)
{
	fBarColor = color;

	Invalidate();
}


void
BStatusBar::SetBarHeight(float barHeight)
{
	float oldHeight = BarHeight();

	fCustomBarHeight = true;
	fBarHeight = barHeight;

	if (barHeight == oldHeight)
		return;

	// resize so that the height fits
	if ((Flags() & B_SUPPORTS_LAYOUT) != 0)
		InvalidateLayout();
	else {
		float width, height;
		GetPreferredSize(&width, &height);
		ResizeTo(Bounds().Width(), height);
	}
}


void
BStatusBar::SetText(const char* string)
{
	_SetTextData(fText, string, fLabel, false);
}


void
BStatusBar::SetTrailingText(const char* string)
{
	_SetTextData(fTrailingText, string, fTrailingLabel, true);
}


void
BStatusBar::SetMaxValue(float max)
{
	// R5 and/or Zeta's SetMaxValue does not trigger an invalidate here.
	// this is probably not ideal behavior, but it does break apps in some cases 
	// as observed with SpaceMonitor.
	// TODO: revisit this when we break binary compatibility
	fMax = max;
}


void
BStatusBar::Update(float delta, const char* text, const char* trailingText)
{
	// If any of these are NULL, the existing text remains (BeBook)
	if (text == NULL)
		text = fText.String();
	if (trailingText == NULL)
		trailingText = fTrailingText.String();
	BStatusBar::SetTo(fCurrent + delta, text, trailingText);
}


void
BStatusBar::Reset(const char *label, const char *trailingLabel)
{
	// Reset replaces the label and trailing label with copies of the
	// strings passed as arguments. If either argument is NULL, the
	// label or trailing label will be deleted and erased.
	fLabel = label ? label : "";
	fTrailingLabel = trailingLabel ? trailingLabel : "";

	// Reset deletes and erases any text or trailing text
	fText = "";
	fTrailingText = "";

	fCurrent = 0;
	fMax = 100;

	Invalidate();
}


void
BStatusBar::SetTo(float value, const char* text, const char* trailingText)
{
	SetText(text);
	SetTrailingText(trailingText);

	if (value > fMax)
		value = fMax;
	else if (value < 0)
		value = 0;
	if (value == fCurrent)
		return;

	BRect barFrame = _BarFrame();
	float oldPosition = _BarPosition(barFrame);

	fCurrent = value;

	float newPosition = _BarPosition(barFrame);
	if (oldPosition == newPosition)
		return;

	// update only the part of the status bar with actual changes
	BRect update = barFrame;
	if (oldPosition < newPosition) {
		update.left = floorf(max_c(oldPosition - 1, update.left));
		update.right = ceilf(newPosition);
	} else {
		update.left = floorf(max_c(newPosition - 1, update.left));
		update.right = ceilf(oldPosition);
	}

	// TODO: Ask the BControlLook in the first place about dirty rect.
	if (be_control_look != NULL)
		update.InsetBy(-1, -1);

	Invalidate(update);
}


float
BStatusBar::CurrentValue() const
{
	return fCurrent;
}


float
BStatusBar::MaxValue() const
{
	return fMax;
}


rgb_color
BStatusBar::BarColor() const
{
	return fBarColor;
}


float
BStatusBar::BarHeight() const
{
	if (!fCustomBarHeight && fBarHeight == -1) {
		// the default bar height is as height as the label
		font_height fontHeight;
		GetFontHeight(&fontHeight);
		const_cast<BStatusBar *>(this)->fBarHeight = fontHeight.ascent
			+ fontHeight.descent + 5;
	}

	return ceilf(fBarHeight);
}


const char *
BStatusBar::Text() const
{
	return fText.String();
}


const char *
BStatusBar::TrailingText() const
{
	return fTrailingText.String();
}


const char *
BStatusBar::Label() const
{
	return fLabel.String();
}


const char *
BStatusBar::TrailingLabel() const
{
	return fTrailingLabel.String();
}


// #pragma mark -


BHandler *
BStatusBar::ResolveSpecifier(BMessage* message, int32 index,
	BMessage* specifier, int32 what, const char *property)
{
	return BView::ResolveSpecifier(message, index, specifier, what, property);
}


status_t
BStatusBar::GetSupportedSuites(BMessage* data)
{
	return BView::GetSupportedSuites(data);
}


status_t
BStatusBar::Perform(perform_code code, void* _data)
{
	switch (code) {
		case PERFORM_CODE_MIN_SIZE:
			((perform_data_min_size*)_data)->return_value
				= BStatusBar::MinSize();
			return B_OK;
		case PERFORM_CODE_MAX_SIZE:
			((perform_data_max_size*)_data)->return_value
				= BStatusBar::MaxSize();
			return B_OK;
		case PERFORM_CODE_PREFERRED_SIZE:
			((perform_data_preferred_size*)_data)->return_value
				= BStatusBar::PreferredSize();
			return B_OK;
		case PERFORM_CODE_LAYOUT_ALIGNMENT:
			((perform_data_layout_alignment*)_data)->return_value
				= BStatusBar::LayoutAlignment();
			return B_OK;
		case PERFORM_CODE_HAS_HEIGHT_FOR_WIDTH:
			((perform_data_has_height_for_width*)_data)->return_value
				= BStatusBar::HasHeightForWidth();
			return B_OK;
		case PERFORM_CODE_GET_HEIGHT_FOR_WIDTH:
		{
			perform_data_get_height_for_width* data
				= (perform_data_get_height_for_width*)_data;
			BStatusBar::GetHeightForWidth(data->width, &data->min, &data->max,
				&data->preferred);
			return B_OK;
		}
		case PERFORM_CODE_SET_LAYOUT:
		{
			perform_data_set_layout* data = (perform_data_set_layout*)_data;
			BStatusBar::SetLayout(data->layout);
			return B_OK;
		}
		case PERFORM_CODE_LAYOUT_INVALIDATED:
		{
			perform_data_layout_invalidated* data
				= (perform_data_layout_invalidated*)_data;
			BStatusBar::LayoutInvalidated(data->descendants);
			return B_OK;
		}
		case PERFORM_CODE_DO_LAYOUT:
		{
			BStatusBar::DoLayout();
			return B_OK;
		}
	}

	return BView::Perform(code, _data);
}


// #pragma mark -


extern "C" void
_ReservedStatusBar1__10BStatusBar(BStatusBar* self, float value,
	const char* text, const char* trailingText)
{
	self->BStatusBar::SetTo(value, text, trailingText);
}


void BStatusBar::_ReservedStatusBar2() {}
void BStatusBar::_ReservedStatusBar3() {}
void BStatusBar::_ReservedStatusBar4() {}


BStatusBar &
BStatusBar::operator=(const BStatusBar& other)
{
	return *this;
}


// #pragma mark -


void
BStatusBar::_InitObject()
{
	fMax = 100.0;
	fCurrent = 0.0;

	fBarHeight = -1.0;
	fTextDivider = Bounds().Width();

	fBarColor = kDefaultBarColor;
	fCustomBarHeight = false;

	SetFlags(Flags() | B_FRAME_EVENTS);
}


void
BStatusBar::_SetTextData(BString& text, const char* source,
	const BString& combineWith, bool rightAligned)
{
	if (source == NULL)
		source = "";

	// If there were no changes, we don't have to do anything
	if (text == source)
		return;

	bool oldHasText = _HasText();
	text = source;

	BString newString;
	if (rightAligned)
		newString << text << combineWith;
	else
		newString << combineWith << text;

	if (oldHasText != _HasText())
		InvalidateLayout();

	font_height fontHeight;
	GetFontHeight(&fontHeight);

//	Invalidate(BRect(position, 0, position + invalidateWidth,
//		ceilf(fontHeight.ascent) + ceilf(fontHeight.descent)));
// TODO: redrawing the entire area takes care of the edge case
// where the left side string changes because of truncation and
// part of it needs to be redrawn as well.
	Invalidate(BRect(0, 0, Bounds().right,
		ceilf(fontHeight.ascent) + ceilf(fontHeight.descent)));
}


/*!
	Returns the inner bar frame without the surrounding bevel.
*/
BRect
BStatusBar::_BarFrame(const font_height* fontHeight) const
{
	float top = 2;
	if (_HasText()) {
		if (fontHeight == NULL) {
			font_height height;
			GetFontHeight(&height);
			top = ceilf(height.ascent + height.descent) + 6;
		} else
			top = ceilf(fontHeight->ascent + fontHeight->descent) + 6;
	}

	return BRect(2, top, Bounds().right - 2, top + BarHeight() - 4);
}


float
BStatusBar::_BarPosition(const BRect& barFrame) const
{
	if (fCurrent == 0)
		return barFrame.left - 1;

	return roundf(barFrame.left - 1
		+ (fCurrent * (barFrame.Width() + 3) / fMax));
}


bool
BStatusBar::_HasText() const
{
	// Force BeOS behavior where the size of the BStatusBar always included
	// room for labels, even when there weren't any.
	if ((Flags() & B_SUPPORTS_LAYOUT) == 0)
		return true;
	return fLabel.Length() > 0 || fTrailingLabel.Length() > 0
		|| fTrailingText.Length() > 0 || fText.Length() > 0;
}
