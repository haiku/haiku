/*
 * Copyright 2010, Stephan Aßmus <superstippi@gmx.de>.
 * Distributed under the terms of the MIT License.
 */


#include "PlayPauseButton.h"

#include <GradientLinear.h>
#include <LayoutUtils.h>
#include <Shape.h>


static const rgb_color kGreen = (rgb_color){ 116, 224, 0, 255 };


// constructor
PlayPauseButton::PlayPauseButton(const char* name,
		BShape* playSymbolShape, BShape* pauseSymbolShape, BMessage* message,
		uint32 borders)
	:
	SymbolButton(name, NULL, message, borders),
	fPlaySymbol(playSymbolShape),
	fPauseSymbol(pauseSymbolShape),
	fPlaybackState(kStopped)
{
}


PlayPauseButton::~PlayPauseButton()
{
	delete fPlaySymbol;
	delete fPauseSymbol;
}


void
PlayPauseButton::Draw(BRect updateRect)
{
	SymbolButton::Draw(updateRect);

	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
	rgb_color active = (rgb_color){ 116, 224, 0, 255 };

	if (IsEnabled()) {
		if (Value() == B_CONTROL_ON)
			base = tint_color(base, (B_DARKEN_4_TINT + B_DARKEN_MAX_TINT) / 2);
		else
			base = tint_color(base, B_DARKEN_4_TINT);
	} else {
		if (Value() == B_CONTROL_ON)
			base = tint_color(base, B_DARKEN_2_TINT);
		else
			base = tint_color(base, B_DARKEN_1_TINT);
	}
	BRect bounds(Bounds());
	BRect pauseBounds = fPauseSymbol->Bounds();
	BRect playBounds = fPlaySymbol->Bounds();

	BPoint offset;
	float spacing = pauseBounds.Height() / 4;
	offset.x = (bounds.left + bounds.right) / 2;
	offset.y = (bounds.top + bounds.bottom) / 2;
	offset.x -= (pauseBounds.Width() + playBounds.Width() + spacing) / 2;
	offset.y -= pauseBounds.Height() / 2;
	offset.x = floorf(offset.x - playBounds.left + 0.5);
	offset.y = ceilf(offset.y - pauseBounds.top);
	if (Value() == B_CONTROL_ON) {
		offset.x += 1;
		offset.y += 1;
	}

	bool playActive = IsEnabled()
		&& ((fPlaybackState == kPlaying && Value() == B_CONTROL_OFF)
			|| (fPlaybackState == kPaused && Value() == B_CONTROL_ON));
	bool pauseActive = IsEnabled()
		&& ((fPlaybackState == kPaused && Value() == B_CONTROL_OFF)
			|| (fPlaybackState == kPlaying && Value() == B_CONTROL_ON));

	MovePenTo(offset);
	BGradientLinear gradient;
	if (playActive) {
		gradient.AddColor(active, 0);
		gradient.AddColor(tint_color(active, B_LIGHTEN_1_TINT), 255);
	} else {
		gradient.AddColor(tint_color(base, B_DARKEN_1_TINT), 0);
		gradient.AddColor(base, 255);
	}
	gradient.SetStart(offset);
	offset.y += playBounds.Height();
	gradient.SetEnd(offset);
	FillShape(fPlaySymbol, gradient);
	if (playActive) {
		SetHighColor(tint_color(active, B_DARKEN_3_TINT));
		MovePenBy(0.5, 0.5);
		StrokeShape(fPlaySymbol);
	}

	offset.y -= playBounds.Height();
	offset.x += ceilf(playBounds.Width() + spacing);
	MovePenTo(offset);
	gradient.MakeEmpty();
	if (pauseActive) {
		gradient.AddColor(active, 0);
		gradient.AddColor(tint_color(active, B_LIGHTEN_1_TINT), 255);
	} else {
		gradient.AddColor(tint_color(base, B_DARKEN_1_TINT), 0);
		gradient.AddColor(base, 255);
	}
	gradient.SetStart(offset);
	offset.y += playBounds.Height();
	gradient.SetEnd(offset);
	FillShape(fPauseSymbol, gradient);
	if (pauseActive) {
		SetHighColor(tint_color(active, B_DARKEN_3_TINT));
		MovePenBy(0.5, 0.5);
		StrokeShape(fPauseSymbol);
	}
}


BSize
PlayPauseButton::MinSize()
{
	if (fPauseSymbol == NULL || fPlaySymbol == NULL)
		return BButton::MinSize();

	BSize size;
	size.width = ceilf(
		(fPlaySymbol->Bounds().Width() + fPauseSymbol->Bounds().Width())
			* 2.5f);
	size.height = ceilf(fPauseSymbol->Bounds().Height() * 2.5f);
	return BLayoutUtils::ComposeSize(ExplicitMinSize(), size);
}


BSize
PlayPauseButton::MaxSize()
{
	BSize size(MinSize());
	size.width = ceilf(size.width * 1.5f);
	return BLayoutUtils::ComposeSize(ExplicitMaxSize(), size);
}


void
PlayPauseButton::SetPlaying()
{
	_SetPlaybackState(kPlaying);
}


void
PlayPauseButton::SetPaused()
{
	_SetPlaybackState(kPaused);
}


void
PlayPauseButton::SetStopped()
{
	_SetPlaybackState(kStopped);
}


void
PlayPauseButton::SetSymbols(BShape* playSymbolShape, BShape* pauseSymbolShape)
{
	BSize oldSize = MinSize();

	delete fPlaySymbol;
	fPlaySymbol = playSymbolShape;
	delete fPauseSymbol;
	fPauseSymbol = pauseSymbolShape;

	if (MinSize() != oldSize)
		InvalidateLayout();

	Invalidate();
}


void
PlayPauseButton::_SetPlaybackState(uint32 state)
{
	if (fPlaybackState == state)
		return;
	fPlaybackState = state;
	Invalidate();
}
