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

void BitmapMenuItem::Draw(void)
{
	BRect		dr;
		
	BMenu* menu = Menu();
	
	// if we don't have a menu, get out...
	if (!menu) return;
	
	BRect itemFrame = Frame();
			
	if (IsSelected()) {
		menu->SetHighColor(160, 160, 160);
		menu->SetLowColor(160, 160, 160);
		menu->FillRect(itemFrame);
	} else {
		menu->SetHighColor(219, 219, 219);
		menu->SetLowColor(219, 219, 219);
	}
	menu->SetHighColor(0, 0, 0);
		
	if (IsMarked()) {
		menu->MovePenTo(itemFrame.left + 2, itemFrame.bottom - 4);	
		BRect checkFrame(0, 0, 12, 12);
		dr.Set(itemFrame.left, itemFrame.top + 2, itemFrame.left + 12, itemFrame.top + 14);
		menu->SetDrawingMode(B_OP_OVER);
		menu->DrawBitmap(fCheckBmp, checkFrame, dr);
		menu->SetDrawingMode(B_OP_COPY);
	}
		
	menu->MovePenTo(itemFrame.left + 38, itemFrame.bottom - 5);		
	menu->DrawString(fName.String());
	
	BRect bitmapFrame = fBmp->Bounds();
	dr.Set(itemFrame.left + 14, itemFrame.top + 2, itemFrame.left + 14 + bitmapFrame.right, itemFrame.top + 17);
	menu->SetDrawingMode(B_OP_OVER);
	menu->DrawBitmap(fBmp, bitmapFrame, dr);
	menu->SetDrawingMode(B_OP_COPY);

}
