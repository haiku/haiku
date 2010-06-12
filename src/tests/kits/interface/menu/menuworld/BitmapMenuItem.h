//--------------------------------------------------------------------
//	
//	BitmapMenuItem.h
//
//	Written by: Owen Smith
//	
//--------------------------------------------------------------------

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _BitmapMenuItem_h
#define _BitmapMenuItem_h

#include <Bitmap.h>
#include <MenuItem.h>

//====================================================================
//	CLASS: BitmapMenuItem

class BitmapMenuItem : public BMenuItem
{
	//----------------------------------------------------------------
	//	Constructors, destructors, operators

public:
					BitmapMenuItem(const char* name, const BBitmap& bitmap,
						BMessage* message, char shortcut = 0,
						uint32 modifiers = 0);
				
				
	//----------------------------------------------------------------
	//	Virtual member function overrides

protected:	
	void				Draw(void);
	void				GetContentSize(float* width, float* height);

	//----------------------------------------------------------------
	//	Accessors

public:
	void				GetBitmapSize(float* width, float* height);
	
	//----------------------------------------------------------------
	//	Member variables
	
private:
	BBitmap m_bitmap;	
};

#endif /* _BitmapMenuItem_h */