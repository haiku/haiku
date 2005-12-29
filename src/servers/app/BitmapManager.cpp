/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 */

/**	Handler for allocating and freeing area memory for BBitmaps 
 *	on the server side. Utilizes the BGET pool allocator.
 *
 *	Whenever a ServerBitmap associated with a client-side BBitmap needs to be 
 *	created or destroyed, the BitmapManager needs to handle it. It takes care of 
 *	all memory management related to them.
 */


#include <new>
#include <stdio.h>
#include <string.h>

#include <Autolock.h>

#include "BitmapManager.h"
#include "ServerBitmap.h"
#include "ServerTokenSpace.h"

using std::nothrow;


//! The bitmap allocator for the server. Memory is allocated/freed by the AppServer class
BitmapManager *gBitmapManager = NULL;

//! Number of bytes to allocate to each area used for bitmap storage
#define BITMAP_AREA_SIZE	B_PAGE_SIZE * 16


//! Sets up stuff to be ready to allocate space for bitmaps
BitmapManager::BitmapManager()
	:
	fBitmapList(1024),
	fBuffer(NULL),
	fLock("BitmapManager Lock"),
	fMemPool("bitmap pool", BITMAP_AREA_SIZE)
{
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
	BAutolock locker(fLock);

	if (!locker.IsLocked())
		return NULL;

	ServerBitmap* bitmap = new(nothrow) ServerBitmap(bounds, space, flags, bytesPerRow);
	if (bitmap == NULL)
		return NULL;

	// Server version of this code will also need to handle such things as
	// bitmaps which accept child views by checking the flags.
	uint8* buffer = (uint8*)fMemPool.GetBuffer(bitmap->BitsLength());

	if (buffer && fBitmapList.AddItem(bitmap)) {
		bitmap->fArea = area_for(buffer);
		bitmap->fBuffer = buffer;
		bitmap->fToken = gTokenSpace.NewToken(kBitmapToken, bitmap);
		bitmap->fInitialized = true;

		// calculate area offset
		area_info info;
		get_area_info(bitmap->fArea, &info);
		bitmap->fOffset = buffer - (uint8*)info.address;
	} else {
		// Allocation failed for buffer or bitmap list
		fMemPool.ReleaseBuffer(buffer);
		delete bitmap;
		bitmap = NULL;
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
	if (!bitmap->_Release()) {
		// there are other references to this bitmap, we don't have to delete it yet
		return;
	}

	BAutolock locker(fLock);
	if (!locker.IsLocked())
		return;

	if (fBitmapList.RemoveItem(bitmap)) {
		// Server code will require a check to ensure bitmap doesn't have its own area		
		fMemPool.ReleaseBuffer(bitmap->fBuffer);
		delete bitmap;
	}
}
