//--------------------------------------------------------------------
//	
//	BitmapMenuItem.cpp
//
//	Written by: Owen Smith
//	
//--------------------------------------------------------------------

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "BitmapMenuItem.h"
#include "constants.h"

//====================================================================
//	BitmapMenuItem Implementation



//--------------------------------------------------------------------
//	BitmapMenuItem constructors, destructors, operators

BitmapMenuItem::BitmapMenuItem(const char* name, const BBitmap& bitmap,
	BMessage* message, char shortcut, uint32 modifiers)
	: BMenuItem(name, message, shortcut, modifiers),
	m_bitmap(bitmap.Bounds(), bitmap.ColorSpace())
{
	// Sadly, operator= for bitmaps is not yet implemented.
	// Half of m_bitmap's initialization is above; now we copy
	// the bits.
	m_bitmap.SetBits(bitmap.Bits(), bitmap.BitsLength(),
		0, bitmap.ColorSpace());
}



//--------------------------------------------------------------------
//	BitmapMenuItem constructors, destructors, operators

void BitmapMenuItem::Draw(void)
{
	BMenu* menu = Menu();
	if (menu) {
		BRect itemFrame = Frame();
		BRect bitmapFrame = itemFrame;
		bitmapFrame.InsetBy(2, 2); // account for 2-pixel margin
		
		menu->SetDrawingMode(B_OP_COPY);
		menu->SetHighColor(BKG_GREY);
		menu->FillRect(itemFrame);
		menu->DrawBitmap(&m_bitmap, bitmapFrame);
		
		if (IsSelected()) {
			// a nonstandard but simple way to draw highlights
			menu->SetDrawingMode(B_OP_INVERT);
			menu->SetHighColor(0,0,0);
			menu->FillRect(itemFrame);
		}
	}
}

void BitmapMenuItem::GetContentSize(float* width, float* height)
{
	GetBitmapSize(width, height);
}



//--------------------------------------------------------------------
//	BitmapMenuItem accessors

void BitmapMenuItem::GetBitmapSize(float* width, float* height)
{
	BRect r = m_bitmap.Bounds();
	*width = r.Width() + 4; // 2-pixel boundary on either side
	*height = r.Height() + 4; // 2-pixel boundary on top/bottom
}
