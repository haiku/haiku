/*
 * Copyright 2010, Stephan Aßmus <superstippi@gmx.de>.
 * Distributed under the terms of the MIT License.
 */


#include "DurationView.h"

#include <LayoutUtils.h>

#include "DurationToString.h"


DurationView::DurationView(const char* name)
	:
	BStringView(name, ""),
	fMode(kTimeToFinish),
	fPosition(0),
	fDuration(0),
	fDisplayDuration(0)
{
	SetSymbolScale(1.0f);

	SetAlignment(B_ALIGN_RIGHT);

	_Update();
}


void
DurationView::AttachedToWindow()
{
	BStringView::AttachedToWindow();
	SetHighColor(tint_color(ViewColor(), B_DARKEN_4_TINT));
}


void
DurationView::MouseDown(BPoint where)
{
	// Switch through the modes
	uint32 mode = fMode + 1;
	if (mode == kLastMode)
		mode = 0;
	SetMode(mode);
}


BSize
DurationView::MinSize()
{
	BSize size;
	char string[64];
	duration_to_string(int32(fDuration / -1000000LL), string, sizeof(string));
	size.width = StringWidth(string);
	font_height fontHeight;
	GetFontHeight(&fontHeight);
	size.height = ceilf(fontHeight.ascent) + ceilf(fontHeight.descent);
	return BLayoutUtils::ComposeSize(ExplicitMinSize(), size);
}


BSize
DurationView::MaxSize()
{
	return BLayoutUtils::ComposeSize(ExplicitMaxSize(), MinSize());
}


// #pragma mark -


void
DurationView::Update(bigtime_t position, bigtime_t duration)
{
	if (position == fPosition && duration == fDuration)
		return;

	fPosition = position;
	if (fDuration != duration) {
		fDuration = duration;
		InvalidateLayout();
	}
	_Update();
}


void
DurationView::SetMode(uint32 mode)
{
	if (mode == fMode)
		return;

	fMode = mode;
	_Update();
}


void
DurationView::SetSymbolScale(float scale)
{
	if (scale != 1.0f) {
		BFont font(be_bold_font);
		font.SetSize(font.Size() * scale * 1.2);
		SetFont(&font);
	} else
		SetFont(be_plain_font);

	InvalidateLayout();
}


void
DurationView::_Update()
{
	switch (fMode) {
		case kTimeElapsed:
			_GenerateString(fPosition);
			break;
		default:
		case kTimeToFinish:
			_GenerateString(fPosition - fDuration);
			break;
		case kDuration:
			_GenerateString(fDuration);
			break;
	}
}


void
DurationView::_GenerateString(bigtime_t duration)
{
	duration /= 1000000;
	if (fDisplayDuration == duration)
		return;

	fDisplayDuration = duration;

	char string[64];
	duration_to_string(duration, string, sizeof(string));

	SetText(string);
}

