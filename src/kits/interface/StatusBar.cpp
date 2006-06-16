/*
 * Copyright 2001-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Axel Dörfler, axeld@pinc-software.de
 */

/*! BStatusBar displays a "percentage-of-completion" gauge. */

#include <Message.h>
#include <Region.h>
#include <StatusBar.h>

#include <stdlib.h>
#include <string.h>


static const rgb_color kDefaultBarColor = {50, 150, 255, 255};


BStatusBar::BStatusBar(BRect frame, const char *name, const char *label,
					   const char *trailingLabel)
	: BView(frame, name, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW),
	fLabel(label),
	fTrailingLabel(trailingLabel)
{
	_InitObject();
}


BStatusBar::BStatusBar(BMessage *archive)
	: BView(archive)
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


void
BStatusBar::_InitObject()
{
	fMax = 100.0f;
	fCurrent = 0.0f;

	fBarHeight = -1.0f;
	fTextWidth = 0.0f;
	fLabelWidth = 0.0f;
	fTrailingTextWidth = 0.0f;
	fTrailingLabelWidth = 0.0f;

	fBarColor = kDefaultBarColor;
	fCustomBarHeight = false;
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

	if (err == B_OK && fCurrent != 0.0f)
		err = archive->AddFloat("_val", fCurrent);
	if (err == B_OK && fMax != 100.0f )
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


void
BStatusBar::AttachedToWindow()
{
	// resize so that the height fits
	float width, height;
	GetPreferredSize(&width, &height);
	ResizeTo(Bounds().Width(), height);

	SetViewColor(B_TRANSPARENT_COLOR);

	if (Parent())
		SetLowColor(Parent()->ViewColor());

	fLabelWidth = ceilf(StringWidth(fLabel.String()));
	fTrailingLabelWidth = ceilf(StringWidth(fTrailingLabel.String()));
	fTextWidth = ceilf(StringWidth(fText.String()));
	fTrailingTextWidth = ceilf(StringWidth(fTrailingText.String()));
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
BStatusBar::Draw(BRect updateRect)
{
	rgb_color backgroundColor;
	if (Parent())
		backgroundColor = Parent()->ViewColor();
	else
		backgroundColor = ui_color(B_PANEL_BACKGROUND_COLOR);

	font_height fontHeight;
	GetFontHeight(&fontHeight);
	BRect barFrame = _BarFrame(&fontHeight);
	BRect outerFrame = barFrame.InsetByCopy(-2.0f, -2.0f);

	BRegion background(updateRect);
	background.Exclude(outerFrame);
	FillRegion(&background, B_SOLID_LOW);

	// Draw labels/texts

	BRect rect = outerFrame;
	rect.top = 0.0f;
	rect.bottom = outerFrame.top - 1.0f;

	if (updateRect.Intersects(rect)) {
		// update labels
		SetHighColor(ui_color(B_CONTROL_TEXT_COLOR));
		MovePenTo(0.0f, ceilf(fontHeight.ascent) + 1.0f);

		if (fLabel.Length())
			DrawString(fLabel.String());
		if (fText.Length())
			DrawString(fText.String());

		if (fTrailingText.Length() || fTrailingLabel.Length()) {
			MovePenTo(rect.right - fTrailingTextWidth - fTrailingLabelWidth,
				ceilf(fontHeight.ascent) + 1.0f);

			if (fTrailingText.Length())
				DrawString(fTrailingText.String());

			if (fTrailingLabel.Length())
				DrawString(fTrailingLabel.String());
		}
	}

	// Draw bar

	if (!updateRect.Intersects(outerFrame))
		return;

	rect = outerFrame;

	// First bevel
	SetHighColor(tint_color(ui_color ( B_PANEL_BACKGROUND_COLOR ), B_DARKEN_1_TINT));
	StrokeLine(rect.LeftBottom(), rect.LeftTop());
	StrokeLine(rect.RightTop());

	SetHighColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_LIGHTEN_2_TINT));
	StrokeLine(BPoint(rect.left + 1.0f, rect.bottom), rect.RightBottom());
	StrokeLine(BPoint(rect.right, rect.top + 1.0f));

	rect.InsetBy(1.0f, 1.0f);

	// Second bevel
	SetHighColor(tint_color(ui_color ( B_PANEL_BACKGROUND_COLOR ), B_DARKEN_4_TINT));
	StrokeLine(rect.LeftBottom(), rect.LeftTop());
	StrokeLine(rect.RightTop());

	SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	StrokeLine(BPoint(rect.left + 1.0f, rect.bottom), rect.RightBottom());
	StrokeLine(BPoint(rect.right, rect.top + 1.0f));

	rect = barFrame;
	rect.right = _BarPosition(barFrame);

	// draw bar itself

	if (rect.right >= rect.left) {
		// Bevel
		SetHighColor(tint_color(fBarColor, B_LIGHTEN_2_TINT));
		StrokeLine(rect.LeftBottom(), rect.LeftTop());
		StrokeLine(rect.RightTop());

		SetHighColor(tint_color(fBarColor, B_DARKEN_2_TINT));
		StrokeLine(BPoint(rect.left + 1.0f, rect.bottom), rect.RightBottom());
		StrokeLine(BPoint(rect.right, rect.top + 1.0f));

		// filling
		SetHighColor(fBarColor);
		FillRect(rect.InsetByCopy(1.0f, 1.0f));
	}

	if (rect.right < barFrame.right) {
		// empty space
		rect.left = rect.right + 1.0f;
		rect.right = barFrame.right;
		SetHighColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_LIGHTEN_MAX_TINT));
		FillRect(rect);
	}
}


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
	float width, height;
	GetPreferredSize(&width, &height);
	ResizeTo(Bounds().Width(), height);
}


void
BStatusBar::SetText(const char* string)
{
	_SetTextData(fText, fTextWidth, string, fLabelWidth, false);
}


void
BStatusBar::SetTrailingText(const char* string)
{
	_SetTextData(fTrailingText, fTrailingTextWidth, string,
		fTrailingLabelWidth, true);
}


void
BStatusBar::SetMaxValue(float max)
{
	fMax = max;
	Invalidate(_BarFrame());
}


void
BStatusBar::Update(float delta, const char* text, const char* trailingText)
{
	BStatusBar::SetTo(fCurrent + delta, text, trailingText);
}


void
BStatusBar::SetTo(float value, const char* text, const char* trailingText)
{
	if (text != NULL)
		_SetTextData(fText, fTextWidth, text, fLabelWidth, false);
	if (trailingText != NULL) {
		_SetTextData(fTrailingText, fTrailingTextWidth, trailingText,
			fTrailingLabelWidth, true);
	}

	if (value > fMax)
		value = fMax;
	else if (value < 0.0f)
		value = 0.0f;
	if (value == fCurrent)
		return;

	BRect barFrame = _BarFrame();
	float oldPosition = _BarPosition(barFrame);

	fCurrent = value;
	float newPosition = _BarPosition(barFrame);

	// update only the part of the status bar with actual changes

	if (oldPosition == newPosition)
		return;

	BRect update = barFrame;
	update.InsetBy(-1, -1);
		// TODO: this shouldn't be necessary, investigate - app_server bug?!

	if (oldPosition < newPosition) {
		update.left = max_c(oldPosition - 1, update.left);
		update.right = newPosition;
	} else {
		update.left = max_c(newPosition - 1, update.left);
		update.right = oldPosition;
	}

	Invalidate(update);
}


void
BStatusBar::Reset(const char *label, const char *trailingLabel)
{
	// Reset replaces the label and trailing label with copies of the
	// strings passed as arguments. If either argument is NULL, the
	// label or trailing label will be deleted and erased.
	_SetTextData(fLabel, fLabelWidth, label, 0.0f, false);
	_SetTextData(fTrailingLabel, fTrailingLabelWidth, trailingLabel, 0.0f, true);

	// Reset deletes and erases any text or trailing text
	_SetTextData(fText, fTextWidth, NULL, fLabelWidth, false);
	_SetTextData(fTrailingText, fTrailingTextWidth, NULL, fTrailingLabelWidth, true);

	fCurrent = 0.0f;
	fMax = 100.0f;

	Invalidate();
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
	if (!fCustomBarHeight && fBarHeight == -1.0f) {
		// the default bar height is as height as the label
		font_height fontHeight;
		GetFontHeight(&fontHeight);
		const_cast<BStatusBar *>(this)->fBarHeight = fontHeight.ascent
			+ fontHeight.descent + 5.0f;
	}

	return fBarHeight;
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
BStatusBar::WindowActivated(bool state)
{
	BView::WindowActivated(state);
}


void
BStatusBar::MouseMoved(BPoint point, uint32 transit, const BMessage *message)
{
	BView::MouseMoved(point, transit, message);
}


void
BStatusBar::DetachedFromWindow()
{
	BView::DetachedFromWindow();
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
}


BHandler *
BStatusBar::ResolveSpecifier(BMessage* message, int32 index,
	BMessage* specifier, int32 what, const char *property)
{
	return BView::ResolveSpecifier(message, index, specifier, what, property);
}


void
BStatusBar::ResizeToPreferred()
{
	BView::ResizeToPreferred();
}


void
BStatusBar::GetPreferredSize(float* _width, float* _height)
{
	if (_width) {
		// AttachedToWindow() might not have been called yet
		*_width = ceilf(StringWidth(fLabel.String()))
			+ ceilf(StringWidth(fTrailingLabel.String()))
			+ ceilf(StringWidth(fText.String()))
			+ ceilf(StringWidth(fTrailingText.String()))
			+ 5.0f;
	}

	if (_height) {
		font_height fontHeight;
		GetFontHeight(&fontHeight);

		*_height = ceilf(fontHeight.ascent + fontHeight.descent) + 6.0f + BarHeight();
	}
}


void
BStatusBar::MakeFocus(bool state)
{
	BView::MakeFocus(state);
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


status_t
BStatusBar::GetSupportedSuites(BMessage* data)
{
	return BView::GetSupportedSuites(data);
}


status_t
BStatusBar::Perform(perform_code d, void* arg)
{
	return BView::Perform(d, arg);
}


void
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


void
BStatusBar::_SetTextData(BString& text, float& width, const char* source,
	float position, bool rightAligned)
{
	// If there were no changes, we don't have to do anything
	if (text == source)
		return;

	float oldWidth = width;
	text = source;
	width = ceilf(StringWidth(text.String()));

	// determine which part of the view needs an update

	// if a label changed, we also need to update the texts
	float invalidateWidth = max_c(width, oldWidth);
	if (&text == &fLabel)
		invalidateWidth += fTextWidth;
	if (&text == &fTrailingLabel)
		invalidateWidth += fTrailingTextWidth;

	if (rightAligned)
		position = Bounds().Width() - position - invalidateWidth;

	font_height fontHeight;
	GetFontHeight(&fontHeight);

	Invalidate(BRect(position, 1.0f, position + invalidateWidth,
		ceilf(fontHeight.ascent + fontHeight.descent)));
}


/*!
	Returns the inner bar frame without the surrounding bevel.
*/
BRect
BStatusBar::_BarFrame(const font_height* fontHeight) const
{
	float top;
	if (fontHeight == NULL) {
		font_height height;
		GetFontHeight(&height);
		top = ceilf(height.ascent + height.descent) + 6.0f;
	} else
		top = ceilf(fontHeight->ascent + fontHeight->descent) + 6.0f;

	return BRect(2.0f, top, Bounds().right - 2.0f, top + BarHeight() - 4.0f);
}


float
BStatusBar::_BarPosition(const BRect& barFrame) const
{
	if (fCurrent == 0.0f)
		return barFrame.left - 1.0f;

	return roundf(barFrame.left + ceilf(fCurrent * barFrame.Width() / fMax));
}

