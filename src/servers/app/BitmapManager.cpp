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
//	File Name:		BitmapManager.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Handler for allocating and freeing area memory for BBitmaps 
//					on the server side. Utilizes the BGET pool allocator.
//------------------------------------------------------------------------------

#include <new>
#include <stdio.h>
#include <string.h>

#include "ServerBitmap.h"

#include "BitmapManager.h"

//! The bitmap allocator for the server. Memory is allocated/freed by the AppServer class
BitmapManager *bitmapmanager = NULL;

//! Number of bytes to allocate to each area used for bitmap storage
#define BITMAP_AREA_SIZE	B_PAGE_SIZE * 2

//! Sets up stuff to be ready to allocate space for bitmaps
BitmapManager::BitmapManager()
	: fBitmapList(1024),
	  fBitmapArea(B_ERROR),
	  fBuffer(NULL),
	  fTokenizer(),
	  fLock(B_ERROR),
	  fMemPool()

{
	// When create_area is passed the B_ANY_ADDRESS flag, the address of the area
	// is stored in the pointer to a pointer.
	fBitmapArea = create_area("bitmap_area", (void**)&fBuffer,
						  B_ANY_ADDRESS, BITMAP_AREA_SIZE,
						  B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);

	if (fBitmapArea < B_OK)
	   	printf("PANIC: BitmapManager couldn't allocate area: %s\n", strerror(fBitmapArea));
	
	fLock = create_sem(1,"bmpmanager_fLock");
	if (fLock < B_OK)
		printf("PANIC: BitmapManager couldn't allocate fLocking semaphore!!\n");
	
	fMemPool.AddToPool(fBuffer, BITMAP_AREA_SIZE);
}

//! Deallocates everything associated with the manager
BitmapManager::~BitmapManager()
{
	int32 count = fBitmapList.CountItems();
	for (int32 i = 0; i < count; i++) {
		if (ServerBitmap* bitmap = (ServerBitmap*)fBitmapList.ItemAt(i)) {
			fMemPool.ReleaseBuffer(bitmap->fBuffer);
			delete bitmap;
		}
	}
	delete_area(fBitmapArea);
	delete_sem(fLock);
}

/*!
	\brief Allocates a new ServerBitmap.
	\param bounds Size of the bitmap
	\param space Color space of the bitmap
	\param flags Bitmap flags as defined in Bitmap.h
	\param bytesPerRow Number of bytes per row.
	\param screen Screen id of the screen associated with it. Unused.
	\return A new ServerBitmap or NULL if unable to allocate one.
*/
ServerBitmap*
BitmapManager::CreateBitmap(BRect bounds, color_space space, int32 flags,
							int32 bytesPerRow, screen_id screen)
{
	ServerBitmap* bitmap = NULL;
	if (acquire_sem(fLock) >= B_OK) {
		bitmap = new(nothrow) ServerBitmap(bounds, space, flags, bytesPerRow);

		// Server version of this code will also need to handle such things as
		// bitmaps which accept child views by checking the flags.
		uint8* buffer = (uint8*)fMemPool.GetBuffer(bitmap->BitsLength());

		if (buffer && fBitmapList.AddItem(bitmap)) {
			bitmap->fArea = area_for(buffer);
			bitmap->fBuffer = buffer;
			bitmap->fToken = fTokenizer.GetToken();
			bitmap->fInitialized = true;
			
			// calculate area offset
			area_info ai;
			get_area_info(bitmap->fArea, &ai);
			bitmap->fOffset = buffer - (uint8*)ai.address;

		} else {
			// Allocation failed for buffer or bitmap list
			fMemPool.ReleaseBuffer(buffer);
			delete bitmap;
			bitmap = NULL;
		}

		release_sem(fLock);
	}
	return bitmap;
}

/*!
	\brief Deletes a ServerBitmap.
	\param bitmap The bitmap to delete
*/
void
BitmapManager::DeleteBitmap(ServerBitmap *bitmap)
{
	if (acquire_sem(fLock) >= B_OK) {
		if (fBitmapList.RemoveItem(bitmap)) {
			// Server code will require a check to ensure bitmap doesn't have its own area		
			fMemPool.ReleaseBuffer(bitmap->fBuffer);
			delete bitmap;
		}
		release_sem(fLock);
	}
}
