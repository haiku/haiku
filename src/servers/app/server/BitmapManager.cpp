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
//	File Name:		BitmapManager.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Handler for allocating and freeing area memory for BBitmaps 
//					on the server side. Utilizes the BGET pool allocator.
//------------------------------------------------------------------------------
#include "BitmapManager.h"
#include "ServerBitmap.h"
#include <stdio.h>
#include "BGet++.h"

//! The bitmap allocator for the server. Memory is allocated/freed by the AppServer class
BitmapManager *bitmapmanager=NULL;

//! Number of bytes to allocate to each area used for bitmap storage
#define BITMAP_AREA_SIZE	B_PAGE_SIZE * 2

//! Sets up stuff to be ready to allocate space for bitmaps
BitmapManager::BitmapManager(void)
{
	fMemPool=new AreaPool;
	bmplist=new BList(0);

	// When create_area is passed the B_ANY_ADDRESS flag, the address of the area
	// is stored in the pointer to a pointer.
	bmparea=create_area("bitmap_area",(void**)&buffer,B_ANY_ADDRESS,BITMAP_AREA_SIZE,
			B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
	if(bmparea==B_BAD_VALUE ||
	   bmparea==B_NO_MEMORY ||
	   bmparea==B_ERROR)
	   	printf("PANIC: BitmapManager couldn't allocate area!!\n");
	
	lock=create_sem(1,"bmpmanager_lock");
	if(lock<0)
		printf("PANIC: BitmapManager couldn't allocate locking semaphore!!\n");
	
	fMemPool->AddToPool(buffer,BITMAP_AREA_SIZE);
}

//! Deallocates everything associated with the manager
BitmapManager::~BitmapManager(void)
{
	if(bmplist->CountItems()>0)
	{
		ServerBitmap *tbmp=NULL;
		int32 itemcount=bmplist->CountItems();
		for(int32 i=0; i<itemcount; i++)
		{
			tbmp=(ServerBitmap*)bmplist->RemoveItem(0L);
			if(tbmp)
			{
				fMemPool->ReleaseBuffer(tbmp->_buffer);
				delete tbmp;
				tbmp=NULL;
			}
		}
	}
	delete fMemPool;
	delete bmplist;
	delete_area(bmparea);
	delete_sem(lock);
}

/*!
	\brief Allocates a new ServerBitmap.
	\param bounds Size of the bitmap
	\param space Color space of the bitmap
	\param flags Bitmap flags as defined in Bitmap.h
	\param bytes_per_row Number of bytes per row.
	\param screen Screen id of the screen associated with it. Unused.
	\return A new ServerBitmap or NULL if unable to allocate one.
*/
ServerBitmap * BitmapManager::CreateBitmap(BRect bounds, color_space space, int32 flags,
	int32 bytes_per_row, screen_id screen)
{
	acquire_sem(lock);
	ServerBitmap *bmp=new ServerBitmap(bounds, space, flags, bytes_per_row);
	
	// Server version of this code will also need to handle such things as
	// bitmaps which accept child views by checking the flags.
	uint8 *bmpbuffer=(uint8 *)fMemPool->GetBuffer(bmp->BitsLength());

	if(!bmpbuffer)
	{
		delete bmp;
		return NULL;
	}
	bmp->_area=area_for(bmpbuffer);
	bmp->_buffer=bmpbuffer;
	bmp->_token=tokenizer.GetToken();
	bmp->_initialized=true;
	
	// calculate area offset
	area_info ai;
	get_area_info(bmp->_area,&ai);
	bmp->_offset=bmpbuffer-(uint8*)ai.address;
	
	bmplist->AddItem(bmp);
	release_sem(lock);
	return bmp;
}

/*!
	\brief Deletes a ServerBitmap.
	\param bitmap The bitmap to delete
*/
void BitmapManager::DeleteBitmap(ServerBitmap *bitmap)
{
	acquire_sem(lock);
	ServerBitmap *tbmp=(ServerBitmap*)bmplist->RemoveItem(bmplist->IndexOf(bitmap));

	if(!tbmp)
	{
		release_sem(lock);
		return;
	}

	// Server code will require a check to ensure bitmap doesn't have its own area		
	fMemPool->ReleaseBuffer(tbmp->_buffer);
	delete tbmp;

	release_sem(lock);
}
