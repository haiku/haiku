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
	if (fBitmap)
		DrawBitmap(fBitmap, Bounds().LeftTop());
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
