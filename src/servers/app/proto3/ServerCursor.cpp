/*
		ServerCursor.cpp
			This is the server-side class used to handle cursors. Note that it
			will handle the R5 cursors, but it will also handle taking a bitmap.
			
			This is new code, and there may be changes to the API before it settles
			in.
*/

#include "ServerCursor.h"
#include "ServerBitmap.h"

ServerCursor::ServerCursor(ServerBitmap *bmp)
{
	is_initialized=false;
	position.Set(0,0);
	SetCursor(bmp);
}

ServerCursor::ServerCursor(int8 *data, color_space space)
{
	// For handling the idiot R5 API data format

	is_initialized=false;
	position.Set(0,0);
	SetCursor(data,space);
}

ServerCursor::~ServerCursor(void)
{
	if(is_initialized)
	{
		delete bitmap;
		delete mask;
	}
}

void ServerCursor::SetCursor(int8 *data, color_space space)
{
	// 68-byte array used in R5 for holding cursors.
	// This API has serious problems and should be deprecated(but supported)
	// in a future release, perhaps right after the first one
	if(is_initialized)
	{
		delete bitmap;
		delete mask;
	}
	cspace=space;

	bitmap=new ServerBitmap(16,16,B_GRAY1);
	mask=new ServerBitmap(16,16,B_GRAY1);
	width=16;
	height=16;
	bounds.Set(0,0,15,15);
}

void ServerCursor::SetCursor(ServerBitmap *bmp)
{
	if(is_initialized)
	{
		delete bitmap;
		delete mask;
	}
	cspace=bmp->cspace;

	bitmap=new ServerBitmap(16,16,B_GRAY1);
	mask=new ServerBitmap(16,16,B_GRAY1);
	width=bmp->width;
	height=bmp->height;
	bounds.Set(0,0,bmp->width-1,bmp->height-1);
}

void ServerCursor::MoveTo(int32 x, int32 y)
{
	position.Set(x,y);
}
