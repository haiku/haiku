#include "IconMenuItem.h"

#include <Bitmap.h>

/***********************************************************
 * Constructor.
 ***********************************************************/
IconMenuItem::IconMenuItem(const char* label,BMessage *message,char shortcut,uint32 modifiers,BBitmap *bitmap,bool copy,bool free)
	:BMenuItem(label,message,shortcut,modifiers)
	,fBitmap(NULL)
	,fCopy(copy)
{
	SetBitmap(bitmap,free);
	fHeightDelta = 0;
}

/***********************************************************
 * Constructor.
 ***********************************************************/
IconMenuItem::IconMenuItem(BMenu *submenu,BMessage *message,char shortcut,uint32 modifiers,BBitmap *bitmap,bool copy,bool free)
	:BMenuItem(submenu,message)
	,fBitmap(NULL)
	,fCopy(copy)
{
	SetBitmap(bitmap,free);
	fHeightDelta = 0;
	SetShortcut(shortcut,modifiers);
}

/***********************************************************
 * Destructor.
 ***********************************************************/
IconMenuItem::~IconMenuItem()
{
	if(fCopy)
		delete fBitmap;
}

/***********************************************************
 * Draw menu icon.
 ***********************************************************/
void
IconMenuItem::DrawContent()
{
		
	if(fBitmap != NULL)
	{
		
		BPoint drawPoint(ContentLocation());
		// center text and icon.	
		drawing_mode mode = Menu()->DrawingMode();
		Menu()->SetDrawingMode(B_OP_ALPHA);
		//Menu()->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);	
		Menu()->SetLowColor( Menu()->ViewColor());
		//Menu()->SetHighColor(B_TRANSPARENT_32_BIT);
		if( !IsEnabled() )
		{
			Menu()->SetDrawingMode(B_OP_BLEND);		
			Menu()->DrawBitmap(fBitmap,drawPoint);
			Menu()->SetDrawingMode(B_OP_OVER);
		}else
			Menu()->DrawBitmap(fBitmap,drawPoint);
		// offset to title point.
		drawPoint.y += ceil( fHeightDelta/2 );	
		drawPoint.x += 20;
		// Move draw point.
		Menu()->MovePenTo(drawPoint);
		Menu()->SetDrawingMode(mode);
	}
	
	BMenuItem::DrawContent();
}

/***********************************************************
 * Extruct content width
 ***********************************************************/
void
IconMenuItem::GetContentSize(float *width, float *height)
{
	BMenuItem::GetContentSize(width,height);
	(*width) += 20;
	fHeightDelta = 16 - (*height);
	if( (*height) < 16)
		(*height) = 16;
}

/***********************************************************
 * Set the other bitmap.
 ***********************************************************/
void
IconMenuItem::SetBitmap(BBitmap *bitmap,bool free)
{
	if(fCopy)
		delete fBitmap;
	
	if(!fCopy)
		fBitmap = bitmap;
	else{
		if(bitmap)
		{
			fBitmap = new BBitmap(bitmap);
			if(free) delete bitmap;
		}else
			fBitmap = NULL;
	}
}

