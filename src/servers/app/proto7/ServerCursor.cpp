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
#include <stdio.h>
#include <math.h>

long pow2(int power)
{
	if(power==0)
		return 1;
	return long(2 << (power-1));
}

//#define DEBUG_SERVER_CURSOR

/*
ServerCursor::ServerCursor(ServerBitmap *bmp, ServerBitmap *cmask=NULL)
{
	// The OpenBeOS R1 API can use Bitmaps to create cursors - more flexibility that way.
}
*/
ServerCursor::ServerCursor(int8 *data)
{
	// 68-byte array used in R5 for holding cursors.
	// This API has serious problems and should be deprecated(but supported) in R2

	// Now that we have all the setup, we're going to map (for now) the cursor
	// to RGBA32. Eventually, there will be support for 16 and 8-bit depths
	if(data)
	{
		uint32 black=0xFF000000,
			white=0xFFFFFFFF,
			*bmppos;
		uint16	*cursorpos, *maskpos,cursorflip, maskflip,
				cursorval, maskval;
		uint8 	i,j;
	
		cursorpos=(uint16*)(data+4);
		maskpos=(uint16*)(data+36);
		
		buffer=new uint8[1024];	// 16x16x4 - RGBA32 space
		width=16;
		height=16;
		bytesperline=64;
		
		// for each row in the cursor data
		for(j=0;j<16;j++)
		{
			bmppos=(uint32*)(buffer+ (j*64) );
	
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
	else
	{
		buffer=NULL;
		width=0;
		height=0;
		bytesperline=0;
	}
	
#ifdef DEBUG_SERVER_CURSOR
	PrintToStream();
#endif
}

ServerCursor::~ServerCursor(void)
{
	if(buffer)
		delete buffer;
}

void ServerCursor::PrintToStream(void)
{
	if(!buffer)
	{
		printf("ServerCursor with NULL buffer\n");
		return;
	}
	
	rgb_color *color=(rgb_color*)buffer;
	printf("Server Cursor\n");
	for(int32 i=0; i<height; i++)
	{
		for(int32 j=0; j<width; j++)
			printf("%c ", (color[(i*height)+j].alpha>0)?'*':'.');

		printf("\n");
	}
}
