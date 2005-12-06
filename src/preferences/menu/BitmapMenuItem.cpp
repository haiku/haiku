#include <Bitmap.h>
#include <NodeInfo.h>

// Project Headers
#include "BitmapMenuItem.h"


// BitmapMenuItem class definition
BitmapMenuItem::BitmapMenuItem(const char* name, BMessage* message, 
				BBitmap* bmp, char shortcut, uint32 modifiers)
	: 
	BMenuItem(name, message, shortcut, modifiers),
	fBitmap(bmp),
	fName(name)
{
}


BitmapMenuItem::~BitmapMenuItem()
{
	delete fBitmap;
}


void
BitmapMenuItem::DrawContent()
{
	BMenu* menu = Menu();
	
	// if we don't have a menu, get out...
	if (!menu)
		return;
	
	BRect itemFrame = Frame();
			
	menu->MovePenTo(itemFrame.left + 38, itemFrame.top + 2);		
	BMenuItem::DrawContent();
	if (fBitmap != NULL) {
		BRect bitmapFrame = fBitmap->Bounds();
		BRect dr(itemFrame.left + 14, itemFrame.top + 2, itemFrame.left + 14 + bitmapFrame.right, itemFrame.top + 17);
		menu->SetDrawingMode(B_OP_OVER);
		menu->DrawBitmap(fBitmap, bitmapFrame, dr);
		menu->SetDrawingMode(B_OP_COPY);
	}
}
