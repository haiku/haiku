/*
 * Copyright 2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "StringView.h"

//#include <Region.h>


StringView::StringView(BRect frame, const char* name, const char* label,
	const char* text, uint32 resizeMask, uint32 flags)
	: BView(frame, name, resizeMask, flags),
	fLabel(label),
	fText(text),
	fLabelAlignment(B_ALIGN_RIGHT),
	fTextAlignment(B_ALIGN_LEFT)
{
	fDivider = StringWidth(label) + 4.0f;
}


StringView::~StringView()
{
}


void
StringView::SetAlignment(alignment labelAlignment, alignment textAlignment)
{
	if (labelAlignment == fLabelAlignment && textAlignment == fTextAlignment)
		return;

	fLabelAlignment = labelAlignment;
	fTextAlignment = textAlignment;

	Invalidate();
}


void
StringView::GetAlignment(alignment* _label, alignment* _text) const
{
	if (_label)
		*_label = fLabelAlignment;
	if (_text)
		*_text = fTextAlignment;
}


void
StringView::SetDivider(float divider)
{
	fDivider = divider;
	_UpdateText();

	Invalidate();
}


void
StringView::AttachedToWindow()
{
	if (Parent() != NULL)
		SetViewColor(Parent()->ViewColor());
	else
		SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	SetLowColor(ViewColor());
	_UpdateText();
}


void
StringView::Draw(BRect updateRect)
{
	BRect rect = Bounds();

	font_height fontHeight;
	GetFontHeight(&fontHeight);

	float y = ceilf(fontHeight.ascent) + 1.0f;
	float x;

	SetHighColor(IsEnabled() ? ui_color(B_CONTROL_TEXT_COLOR)
		: tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_DISABLED_LABEL_TINT));

	if (Label()) {
		switch (fLabelAlignment) {
			case B_ALIGN_RIGHT:
				x = Divider() - StringWidth(Label()) - 3.0f;
				break;

			case B_ALIGN_CENTER:
				x = Divider() - StringWidth(Label()) / 2.0f;
				break;

			default:
				x = 1.0f;
				break;
		}

		DrawString(Label(), BPoint(x, y));
	}

	if (fTruncatedText.String() != NULL) {
		switch (fTextAlignment) {
			case B_ALIGN_RIGHT:
				x = rect.Width() - StringWidth(fTruncatedText.String());
				break;

			case B_ALIGN_CENTER:
				x = Divider() + (rect.Width() - Divider() - StringWidth(Label())) / 2.0f;
				break;

			default:
				x = Divider() + 3.0f;
				break;
		}

		DrawString(fTruncatedText.String(), BPoint(x, y));
	}
}


void
StringView::FrameResized(float width, float height)
{
	BString oldTruncated = fTruncatedText;
	_UpdateText();

	if (oldTruncated != fTruncatedText) {
		// invalidate text portion only
		BRect rect = Bounds();
		rect.left = Divider();
		Invalidate(rect);
	}
}


void
StringView::ResizeToPreferred()
{
	float width, height;
	GetPreferredSize(&width, &height);

	// Resize the width only for B_ALIGN_LEFT (if its large enough already, that is)
	if (Bounds().Width() > width
		&& (fLabelAlignment != B_ALIGN_LEFT || fTextAlignment != B_ALIGN_LEFT))
		width = Bounds().Width();

	BView::ResizeTo(width, height);
}


void
StringView::GetPreferredSize(float* _width, float* _height)
{
	if (!Text() && !Label()) {
		BView::GetPreferredSize(_width, _height);
		return;
	}

	if (_width)
		*_width = 7.0f + ceilf(StringWidth(Label()) + StringWidth(Text()));

	if (_height) {
		font_height fontHeight;
		GetFontHeight(&fontHeight);
		*_height = ceil(fontHeight.ascent + fontHeight.descent + fontHeight.leading) + 2.0f;
	}
}


void
StringView::SetEnabled(bool enabled)
{
	if (IsEnabled() == enabled)
		return;

	fEnabled = enabled;
	Invalidate();
}


void
StringView::SetLabel(const char* label)
{
	fLabel = label;

	// invalidate label portion only
	BRect rect = Bounds();
	rect.right = Divider();
	Invalidate(rect);
}


void
StringView::SetText(const char* text)
{
	fText = text;

	_UpdateText();

	// invalidate text portion only
	BRect rect = Bounds();
	rect.left = Divider();
	Invalidate(rect);
}


void
StringView::_UpdateText()
{
	fTruncatedText = fText;
	TruncateString(&fTruncatedText, B_TRUNCATE_MIDDLE, Bounds().Width() - Divider());
}
