/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 */
#ifndef BITMAP_MANAGER_H_
#define BITMAP_MANAGER_H_


#include <GraphicsDefs.h>
#include <List.h>
#include <OS.h>
#include <Rect.h>
#include <Locker.h>

#include "BGet++.h"
#include "TokenHandler.h"

class ServerBitmap;

/*!
	\class BitmapManager BitmapManager.h
	\brief Handler for BBitmap allocation
	
	Whenever a ServerBitmap associated with a client-side BBitmap needs to be 
	created or destroyed, the BitmapManager needs to handle it. It takes care of 
	all memory management related to them.

	NOTE: The allocator used is not thread-safe, it is currently protected
	by the BitmapManager lock.
*/
class BitmapManager {
 public:
								BitmapManager();
	virtual						~BitmapManager();

			ServerBitmap*		CreateBitmap(BRect bounds,
											 color_space space,
											 int32 flags,
											 int32 bytesPerRow = -1,
											 screen_id screen = B_MAIN_SCREEN_ID);
			void				DeleteBitmap(ServerBitmap* bitmap);
 protected:
			BList				fBitmapList;
			area_id				fBitmapArea;
			int8*				fBuffer;
			TokenHandler		fTokenizer;
			BLocker				fLock;
			AreaPool			fMemPool;
};

extern BitmapManager *gBitmapManager;

#endif	/* BITMAP_MANAGER_H_ */
