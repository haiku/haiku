//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, Haiku, Inc.
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		ServerCursor.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Glorified ServerBitmap used for cursor work.
//  
//------------------------------------------------------------------------------
#include "ServerCursor.h"
#include <stdio.h>
/*!
	\brief Constructor
	\param r Size of the cursor
	\param cspace Color space of the cursor
	\param flags ServerBitmap flags. See Bitmap.h.
	\param hotspot Hotspot of the cursor
	\param bytesperline Bytes per row for the cursor. See ServerBitmap::ServerBitmap()
	
*/
ServerCursor::ServerCursor(BRect r, color_space cspace, int32 flags, BPoint hotspot, int32 bytesperrow, screen_id screen)
 : ServerBitmap(r,cspace,flags,bytesperrow,screen)
{
	fHotSpot=hotspot;
	fHotSpot.ConstrainTo(Bounds());
	fOwningTeam=-1;

	_AllocateBuffer();
}

/*!
	\brief Constructor
	\param data pointer to 68-byte cursor data array. See BeBook entry for BCursor for details
*/
ServerCursor::ServerCursor(int8 *data)
 : ServerBitmap(BRect(0,0,15,15),B_RGBA32,0)
{
	// 68-byte array used in R5 for holding cursors.
	// This API has serious problems and should be deprecated(but supported) in R2

	// Now that we have all the setup, we're going to map (for now) the cursor
	// to RGBA32. Eventually, there will be support for 16 and 8-bit depths
	if(data)
	{	
		fInitialized=true;
		fOwningTeam=-1;
		uint32 black=0xFF000000,
			white=0xFFFFFFFF,
			*bmppos;
		uint16	*cursorpos, *maskpos,cursorflip, maskflip,
				cursorval, maskval,powval;
		uint8 	i,j;
	
		cursorpos=(uint16*)(data+4);
		maskpos=(uint16*)(data+36);
		fHotSpot.Set(data[3],data[2]);
		
		_AllocateBuffer();
		
		// for each row in the cursor data
		for(j=0;j<16;j++)
		{
			bmppos=(uint32*)(fBuffer+ (j*BytesPerRow()) );
	
			// On intel, our bytes end up swapped, so we must swap them back
			cursorflip=(cursorpos[j] & 0xFF) << 8;
			cursorflip |= (cursorpos[j] & 0xFF00) >> 8;
			
			maskflip=(maskpos[j] & 0xFF) << 8;
			maskflip |= (maskpos[j] & 0xFF00) >> 8;
	
			// for each column in each row of cursor data
			for(i=0;i<16;i++)
			{
				// Get the values and dump them to the bitmap
				powval=1 << (15-i);
				cursorval=cursorflip & powval;
				maskval=maskflip & powval;
				bmppos[i]=((cursorval!=0)?black:white) & ((maskval>0)?0xFFFFFFFF:0x00FFFFFF);
			}
		}
	}
	else
	{
		fWidth=0;
		fHeight=0;
		fBytesPerRow=0;
		fSpace=B_NO_COLOR_SPACE;
	}
}

/*!
	\brief Copy constructor
	\param cursor cursor to copy
*/
ServerCursor::ServerCursor(const ServerCursor *cursor)
 : ServerBitmap(cursor)
{
	_AllocateBuffer();
	fInitialized=true;
	fOwningTeam=-1;

	if(cursor)
	{	
		if(cursor->fBuffer)
			memcpy(fBuffer, cursor->fBuffer, BitsLength());
		fHotSpot=cursor->fHotSpot;
	}
}

//!	Frees the heap space allocated for the cursor's image data
ServerCursor::~ServerCursor(void)
{
	_FreeBuffer();
}

/*!
	\brief Sets the cursor's hotspot
	\param pt New location of hotspot, constrained to the cursor's boundaries.
*/
void ServerCursor::SetHotSpot(BPoint pt)
{
	fHotSpot=pt;
	fHotSpot.ConstrainTo(Bounds());
}

void ServerCursor::SetAppSignature(const char *signature)
{
	fAppSignature.SetTo( (signature)?signature:"" );
}
