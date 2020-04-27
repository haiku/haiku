//*****************************************************************************
//
//	File:		ProgressBar.cpp
//
//	Written by:	David Ramsey and Daniel Switkin
//
//	Copyright 1999, Be Incorporated
//
//*****************************************************************************


#include "ProgressBar.h"
#include "PulseApp.h"


ProgressBar::ProgressBar(BRect r, char *name) :	BView(r, name, B_FOLLOW_NONE,
	B_WILL_DRAW)
{
	previous_value = current_value = 0;

	// Create 20 segments
	float height = (r.bottom - r.top) - 8;
	for (int32 counter = 0; counter < 20; counter++) {
		segments[counter].rect.Set(r.left + (counter * 7), r.top,
			(r.left + (counter * 7) + 5), r.top + height);
		segments[counter].rect.OffsetTo(BPoint((counter * 7) + 4, 4));
	}
	SetLowColor(255, 0, 0);
	SetViewColor(B_TRANSPARENT_COLOR);
}


// New - real time updating of bar colors
void
ProgressBar::UpdateColors(int32 color, bool fade)
{
	unsigned char red = (color & 0xff000000) >> 24;
	unsigned char green = (color & 0x00ff0000) >> 16;
	unsigned char blue = (color & 0x0000ff00) >> 8;

	if (fade) {
		unsigned char red_base = red / 3;
		unsigned char green_base = green / 3;
		unsigned char blue_base = blue / 3;

		for (int x = 0; x < 20; x++) {
			segments[x].color.red = (uint8)(red_base + ((red - red_base)
				* ((float)x / 19.0)));
			segments[x].color.green = (uint8)(green_base
				+ ((green - green_base) * ((float)x / 19.0)));
			segments[x].color.blue = (uint8)(blue_base + ((blue - blue_base)
				* ((float)x / 19.0)));
			segments[x].color.alpha = 0xff;
		}
	} else {
		for (int x = 0; x < 20; x++) {
			segments[x].color.red = red;
			segments[x].color.green = green;
			segments[x].color.blue = blue;
			segments[x].color.alpha = 0xff;
		}
	}
	Render(true);
}


void
ProgressBar::AttachedToWindow()
{
	Prefs *prefs = ((PulseApp *)be_app)->fPrefs;
	UpdateColors(prefs->normal_bar_color, prefs->normal_fade_colors);
}


void
ProgressBar::MouseDown(BPoint point)
{
	point = ConvertToParent(point);
	Parent()->MouseDown(point);
}


void
ProgressBar::Set(int32 value)
{
	// How many segments to light up
	current_value = (int32)(value / 4.9);
	if (current_value > 20)
		current_value = 20;

	Render(false);
}


// Draws the progress bar. If "all" is true the entire bar is redrawn rather
// than just the part that changed.
void
ProgressBar::Render(bool all)
{
	if (all) {
		// Black border
		BRect bounds = Bounds();
		bounds.InsetBy(2, 2);
		SetHighColor(0, 0, 0);
		StrokeRect(bounds);
		bounds.InsetBy(1, 1);
		StrokeRect(bounds);

		// Black dividers
		float left = bounds.left;
		BPoint start, end;
		for (int x = 0; x < 19; x++) {
			left += 7;
			start.Set(left, bounds.top);
			end.Set(left, bounds.bottom);
			StrokeLine(start, end);
		}

		for (int x = 0; x < current_value; x++) {
			SetHighColor(segments[x].color.red, segments[x].color.green,
				segments[x].color.blue);
			FillRect(segments[x].rect);
		}

		SetHighColor(75, 75, 75);
		if (current_value < 20) {
			for (int x = 19; x >= current_value; x--) {
				FillRect(segments[x].rect);
			}
		}
	} else if (current_value > previous_value) {
		for (int x = previous_value; x < current_value; x++) {
			SetHighColor(segments[x].color.red, segments[x].color.green,
				segments[x].color.blue);
			FillRect(segments[x].rect);
		}
	} else if (current_value < previous_value) {
		SetHighColor(75, 75, 75);
		for (int x = previous_value - 1; x >= current_value; x--) {
			FillRect(segments[x].rect);
		}
		// Special case to make sure the lowest light gets turned off
		if (current_value == 0) FillRect(segments[0].rect);
	}

	Sync();
	previous_value = current_value;
}


void
ProgressBar::Draw(BRect rect)
{
	// Add bevels
	SetHighColor(dkgray, dkgray, dkgray);
	BRect frame = Bounds();
	StrokeLine(BPoint(frame.left, frame.top), BPoint(frame.right, frame.top));
	StrokeLine(BPoint(frame.left, frame.top + 1), BPoint(frame.right,
		frame.top + 1));
	StrokeLine(BPoint(frame.left, frame.top), BPoint(frame.left,
		frame.bottom));
	StrokeLine(BPoint(frame.left + 1, frame.top),
		BPoint(frame.left + 1, frame.bottom));

	SetHighColor(ltgray, ltgray, ltgray);
	StrokeLine(BPoint(frame.right-1, frame.top + 2),
		BPoint(frame.right - 1, frame.bottom));
	StrokeLine(BPoint(frame.right, frame.top + 1),
		BPoint(frame.right, frame.bottom));
	StrokeLine(BPoint(frame.left+1, frame.bottom - 1),
		BPoint(frame.right - 1, frame.bottom - 1));
	StrokeLine(BPoint(frame.left, frame.bottom),
		BPoint(frame.right, frame.bottom));

	Render(true);
}
