/*
 * Copyright 2003-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jonas Sundström, jonas@kirilla.com
 */


#include "ZipOMaticActivity.h"

#include <ControlLook.h>

#include <stdio.h>


Activity::Activity(const char* name)
	:
	BView(name, B_WILL_DRAW | B_FRAME_EVENTS),
	fIsRunning(false),
	fBitmap(NULL),
	fSpinSpeed(0.15),
	fColors(NULL),
	fNumColors(0),
	fScrollOffset(0.0),
	fStripeWidth(0.0),
	fNumStripes(0)
{
	_InactiveColors();
	SetExplicitMinSize(BSize(17, 17));
	SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, 17));
};


Activity::~Activity()
{
	delete fBitmap;
	delete[] fColors;
}


void 
Activity::AllAttached()
{
	_CreateBitmap();
	FrameResized(Bounds().Width(), Bounds().Height());
}


void 
Activity::Start()
{
	fIsRunning = true;
	_ActiveColors();
	Window()->SetPulseRate(100000);
	SetFlags(Flags() | B_PULSE_NEEDED);
	Invalidate();
}


void 
Activity::Pause()
{
	Window()->SetPulseRate(500000);
	SetFlags(Flags() & (~B_PULSE_NEEDED));
	Invalidate();
}


void 
Activity::Stop()
{
	fIsRunning = false;
	_InactiveColors();
	Window()->SetPulseRate(500000);
	SetFlags(Flags() & (~B_PULSE_NEEDED));
	Invalidate();
}


bool 
Activity::IsRunning()	
{
	return fIsRunning;
}


void 
Activity::Pulse()
{
	fScrollOffset += fStripeWidth / (1.0f / fSpinSpeed);
	if (fScrollOffset >= fStripeWidth * fNumColors) {
		// Cycle completed, jump back to where we started
		fScrollOffset = 0;
	}
	Invalidate();
}


void
Activity::SetColors(const rgb_color* colors, uint32 numColors)
{
	delete[] fColors;
	rgb_color* colorsCopy = new rgb_color[numColors];
	for (uint32 i = 0; i < numColors; i++)
		colorsCopy[i] = colors[i];

	fColors = colorsCopy;
	fNumColors = numColors;
}


void 
Activity::Draw(BRect rect)
{
	BRect viewRect = Bounds();
	BRect bitmapRect = fBitmap->Bounds();

	if (bitmapRect != viewRect) {
		delete fBitmap;
		_CreateBitmap();
	}

	_DrawOnBitmap(IsRunning());
	SetDrawingMode(B_OP_COPY);
	DrawBitmap(fBitmap);
}


void
Activity::_DrawOnBitmap(bool running)
{
	if (fBitmap->Lock()) {
		BRect bounds = fBitmap->Bounds();

		fBitmapView->SetDrawingMode(B_OP_COPY);
	
		// Draw color stripes
		float position = -fStripeWidth * (fNumColors + 0.5) + fScrollOffset;
		// Starting position: beginning of the second color cycle
		// The + 0.5 is so we start out without a partially visible stripe
		// on the left side (makes it simpler to loop)
		BRect innerFrame = bounds;
		innerFrame.InsetBy(-2, -2);

		be_control_look->DrawStatusBar(fBitmapView, innerFrame, innerFrame,
			ui_color(B_PANEL_BACKGROUND_COLOR),
			running ? ui_color(B_STATUS_BAR_COLOR)
				: ui_color(B_PANEL_BACKGROUND_COLOR),
			bounds.Width());
		fBitmapView->SetDrawingMode(B_OP_ALPHA);
		uint32 colorIndex = 0;
		for (uint32 i = 0; i < fNumStripes; i++) {
			fBitmapView->SetHighColor(fColors[colorIndex]);
			colorIndex++;
			if (colorIndex >= fNumColors)
				colorIndex = 0;

			BRect stripeFrame = fStripe.Frame();
			fStripe.MapTo(stripeFrame,
			stripeFrame.OffsetToCopy(position, 0.0));
			fBitmapView->FillPolygon(&fStripe);

			position += fStripeWidth;
		}

		fBitmapView->SetDrawingMode(B_OP_COPY);
		// Draw box around it
		be_control_look->DrawTextControlBorder(fBitmapView, bounds, bounds,
			ui_color(B_PANEL_BACKGROUND_COLOR), B_PLAIN_BORDER);
		
		fBitmapView->Sync();
		fBitmap->Unlock();
	}
}


void
Activity::_CreateBitmap(void)
{
	BRect rect = Bounds();
	fBitmap = new BBitmap(rect, B_RGBA32, true);
	fBitmapView = new BView(Bounds(), "buffer", B_FOLLOW_NONE, 0);
	fBitmap->AddChild(fBitmapView);
}


void
Activity::FrameResized(float width, float height)
{
	delete fBitmap;
	_CreateBitmap();
	// Choose stripe width so that at least 2 full stripes fit into the view,
	// but with a minimum of 5px. Larger views get wider stripes, but they
	// grow slower than the view and are capped to a maximum of 200px.
	fStripeWidth = (width / (fIsRunning ? 4 : 6)) + 5;
	if (fStripeWidth > 200)
		fStripeWidth = 200;

	BPoint stripePoints[4];
	stripePoints[0].Set(fStripeWidth * 0.5, 0.0); // top left
	stripePoints[1].Set(fStripeWidth * 1.5, 0.0); // top right
	stripePoints[2].Set(fStripeWidth, height);    // bottom right
	stripePoints[3].Set(0.0, height);             // bottom left

	fStripe = BPolygon(stripePoints, 4);

	fNumStripes = (int32)ceilf((width) / fStripeWidth) + 1 + fNumColors;
		// Number of color stripes drawn in total for the barber pole, the
		// user-visible part is a "window" onto the complete pole. We need
		// as many stripes as are visible, an extra one on the right side
		// (will be partially visible, that's the + 1); and then a whole color
		// cycle of strips extra which we scroll into until we loop.
		//
		// Example with 3 colors and a visible area of 2*fStripeWidth (which means
		// that 2 will be fully visible, and a third one partially):
		//               ........
		//   X___________v______v___
		//  / 1 / 2 / 3 / 1 / 2 / 3 /
		//  `````````````````````````
		// Pole is scrolled to the right into the visible region, which is marked
		// between the two 'v'. Once the left edge of the visible area reaches
		// point X, we can jump back to the initial region position.
	Invalidate();
}

void
Activity::_ActiveColors()
{
	// Default colors, chosen from system color scheme
	rgb_color defaultColors[2];
	rgb_color otherColor = tint_color(ui_color(B_STATUS_BAR_COLOR), 1.3);
	otherColor.alpha = 50;
	defaultColors[0] = otherColor;
	defaultColors[1] = B_TRANSPARENT_COLOR;
	SetColors(defaultColors, 2);

}


void
Activity::_InactiveColors()
{
	// Default colors, chosen from system color scheme
	rgb_color defaultColors[2];
	rgb_color otherColor = tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), 1.7);
	otherColor.alpha = 50;
	defaultColors[0] = otherColor;
	defaultColors[1] = B_TRANSPARENT_COLOR;
	SetColors(defaultColors, 2);
}
