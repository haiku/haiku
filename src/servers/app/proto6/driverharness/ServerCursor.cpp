/*
	ServerCursor.cpp
		This is the server-side class used to handle cursors. Note that it
		will handle the R5 cursor API, but it will also handle taking a bitmap.

		ServerCursors are 32-bit. Eventually, we will be mapping colors to fit
		when the set is called.
		
		This is new code, and there may be changes to the API before it settles
		in.
*/

#include "ServerCursor.h"
#include "ServerBitmap.h"
#include <stdio.h>
#include <math.h>

int8 default_cursor[]={ 
	16,1,0,0,
	
	// data
	0xff, 0x80, 0x80, 0x80, 0x81, 0x00, 0x80, 0x80, 0x80, 0x40, 0x80, 0x80, 0x81, 0x00, 0xa2, 0x00, 
	0xd4, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

	// mask
	0xff, 0x80, 0xff, 0x80, 0xff, 0x00, 0xff, 0x80, 0xff, 0xc0, 0xff, 0x80, 0xff, 0x00, 0xfe, 0x00, 
	0xdc, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

long pow2(int power)
{
	if(power==0)
		return 1;
	return long(2 << (power-1));
}

ServerCursor::ServerCursor(void)
{
	// For when an application is started. This will probably disappear once I 
	// get the stuff for default cursors established
	is_initialized=false;
	position.Set(0,0);
}

ServerCursor::ServerCursor(ServerBitmap *bmp, ServerBitmap *cmask=NULL)
{
	// The OpenBeOS R1 API uses Bitmaps to create cursors - more flexibility that way.
	is_initialized=false;
	position.Set(0,0);
	SetCursor(bmp);
}

ServerCursor::ServerCursor(int8 *data)
{
	// For handling the idiot R5 API data format
	is_initialized=false;
	position.Set(0,0);
	SetCursor(data);
}

ServerCursor::~ServerCursor(void)
{
	if(is_initialized)
		delete bitmap;
}

void ServerCursor::SetCursor(int8 *data)
{
	// 68-byte array used in R5 for holding cursors.
	// This API has serious problems and should be deprecated(but supported)
	// in a future release, perhaps right after the first one

	if(is_initialized)
	{
		delete bitmap;
	}
	cspace=B_RGBA32;

	bitmap=new ServerBitmap(16,16,B_RGBA32);
	width=16;
	height=16;
	bounds.Set(0,0,15,15);

	// Now that we have all the setup, we're going to map (for now) the cursor
	// to RGBA32. Eventually, there will be support for 16 and 8-bit depths
	uint32 black=0xFF000000,
		white=0xFFFFFFFF,
		*bmppos;
	uint16	*cursorpos, *maskpos,cursorflip, maskflip,
			cursorval, maskval;
	uint8 	i,j;

	cursorpos=(uint16*)(data+4);
	maskpos=(uint16*)(data+36);
	
	// for each row in the cursor data
	for(j=0;j<16;j++)
	{
		bmppos=(uint32*)(bitmap->Buffer()+ (j*bitmap->bytesperline) );

		// On intel, our bytes end up swapped, so we must swap them back
		cursorflip=(cursorpos[j] & 0xFF) << 8;
		cursorflip |= (cursorpos[j] & 0xFF00) >> 8;
		maskflip=(maskpos[j] & 0xFF) << 8;
		maskflip |= (maskpos[j] & 0xFF00) >> 8;

		// for each column in each row of cursor data
		for(i=0;i<16;i++)
		{
			// Get the values and dump them to the bitmap
			cursorval=cursorflip & pow2(15-i);
			maskval=maskflip & pow2(15-i);
			bmppos[i]=((cursorval!=0)?black:white) & ((maskval>0)?0xFFFFFFFF:0x00FFFFFF);
		}
	}
}

// Currently unimplemented
void ServerCursor::SetCursor(ServerBitmap *bmp, ServerBitmap *cmask=NULL)
{
/*	if(is_initialized)
		delete bitmap;
	cspace=bmp->cspace;

	bitmap=new ServerBitmap(bmp->width,bmp->height,cspace);

	width=bmp->width;
	height=bmp->height;
	bounds.Set(0,0,bmp->width-1,bmp->height-1);
*/
}

void ServerCursor::MoveTo(int32 x, int32 y)
{
	position.Set(x,y);
}
