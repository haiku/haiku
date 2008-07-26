/*
 * Copyright 2001-2007, Haiku.
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
							color_space space, uint32 flags,
							int32 bytesPerRow = -1,
							screen_id screen = B_MAIN_SCREEN_ID,
							uint8* _allocationFlags = NULL);
		void			DeleteBitmap(ServerBitmap* bitmap);

		void			SuspendOverlays();
		void			ResumeOverlays();

	protected:
		BList			fBitmapList;
		BList			fOverlays;
		BLocker			fLock;
};

extern BitmapManager *gBitmapManager;

#endif	/* BITMAP_MANAGER_H */
