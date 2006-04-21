/*
 * Copyright 2001-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 */
#ifndef BITMAP_MANAGER_H
#define BITMAP_MANAGER_H


#include <GraphicsDefs.h>
#include <List.h>
#include <Locker.h>
#include <OS.h>
#include <Rect.h>

class ClientMemoryAllocator;
class HWInterface;
class ServerBitmap;

class BitmapManager {
	public:
						BitmapManager();
		virtual			~BitmapManager();

		ServerBitmap*	CreateBitmap(ClientMemoryAllocator* allocator,
							HWInterface& hwInterface, BRect bounds,
							color_space space, int32 flags, int32 bytesPerRow = -1,
							screen_id screen = B_MAIN_SCREEN_ID,
							int8* _allocationType = NULL);
		void			DeleteBitmap(ServerBitmap* bitmap);

	protected:
		BList			fBitmapList;
		BLocker			fLock;
};

extern BitmapManager *gBitmapManager;

#endif	/* BITMAP_MANAGER_H */
