// BitmapView.cpp

#include <stdio.h>

#include <Bitmap.h>

#include "BitmapView.h"

// constructor
BitmapView::BitmapView(BRect frame, const char* name,
					   BBitmap* bitmap)
	: BView(frame, name, B_FOLLOW_NONE, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE),
	  fBitmap(bitmap)
{
	SetViewColor(B_TRANSPARENT_32_BIT);
	SetHighColor(255, 0, 0, 255);
}

// destructor
BitmapView::~BitmapView()
{
	delete fBitmap;
}

// Draw
void
BitmapView::Draw(BRect updateRect)
{
	if (fBitmap) {
		if (fBitmap->ColorSpace() == B_RGBA32) {
			// draw the bitmap with pixel alpha
			// against a red background
			FillRect(updateRect);
			SetDrawingMode(B_OP_ALPHA);
			SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
		}
		DrawBitmap(fBitmap, Bounds().LeftTop());
	}
}

// MouseDown
void
BitmapView::MouseDown(BPoint where)
{
}

// MouseUp
void
BitmapView::MouseUp(BPoint where)
{
}

// MouseMoved
void
BitmapView::MouseMoved(BPoint where, uint32 transit,
					   const BMessage* dragMessage)
{
}
