//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
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
//	File Name:		BitmapManager.h
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Handler for allocating and freeing area memory for BBitmaps 
//					on the server side. Utilizes the BGET pool allocator.
//------------------------------------------------------------------------------
#ifndef BITMAP_MANAGER_H_
#define BITMAP_MANAGER_H_

#include <GraphicsDefs.h>
#include <List.h>
#include <Rect.h>
#include <OS.h>
#include "TokenHandler.h"

class ServerBitmap;
class AreaPool;

/*!
	\class BitmapManager BitmapManager.h
	\brief Handler for BBitmap allocation
	
	Whenever a ServerBitmap associated with a client-side BBitmap needs to be 
	created or destroyed, the BitmapManager needs to handle it. It takes care of 
	all memory management related to them.
*/
class BitmapManager
{
public:
	BitmapManager(void);
	~BitmapManager(void);
	ServerBitmap *CreateBitmap(BRect bounds, color_space space, int32 flags,
		int32 bytes_per_row=-1, screen_id screen=B_MAIN_SCREEN_ID);
	void DeleteBitmap(ServerBitmap *bitmap);
protected:
	BList *bmplist;
	area_id bmparea;
	int8 *buffer;
	TokenHandler tokenizer;
	sem_id lock;
	AreaPool *fMemPool;
};

extern BitmapManager *bitmapmanager;

#endif
