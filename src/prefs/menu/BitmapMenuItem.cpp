// System Headers
#ifndef _NODE_INFO_H
#include <NodeInfo.h>
#endif

// Project Headers
#include "BitmapMenuItem.h"


// BitmapMenuItem class definition
BitmapMenuItem::BitmapMenuItem(const char* name, BMessage* message, 
							   BBitmap* bmp, char shortcut, uint32 modifiers)
	: BMenuItem(name, message, shortcut, modifiers)
{
	fBmp = bmp;
	fName.SetTo(name);

	fCheckBmp = BTranslationUtils::GetBitmap(B_RAW_TYPE, "CHECK");
}

void BitmapMenuItem::DrawContent(void)
{
	BRect		dr;
		
	BMenu* menu = Menu();
	
	// if we don't have a menu, get out...
	if (!menu) return;
	
	BRect itemFrame = Frame();
			
	menu->MovePenTo(itemFrame.left + 38, itemFrame.top + 2);		
	BMenuItem::DrawContent();
	
	BRect bitmapFrame = fBmp->Bounds();
	dr.Set(itemFrame.left + 14, itemFrame.top + 2, itemFrame.left + 14 + bitmapFrame.right, itemFrame.top + 17);
	menu->SetDrawingMode(B_OP_OVER);
	menu->DrawBitmap(fBmp, bitmapFrame, dr);
	menu->SetDrawingMode(B_OP_COPY);
}
