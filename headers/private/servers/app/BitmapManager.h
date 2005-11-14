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
#include <Locker.h>
#include <OS.h>
#include <Rect.h>

#include "BGet++.h"

class ServerBitmap;


class BitmapManager {
	public:
						BitmapManager();
		virtual			~BitmapManager();

		ServerBitmap*	CreateBitmap(BRect bounds, color_space space,
							int32 flags, int32 bytesPerRow = -1,
							screen_id screen = B_MAIN_SCREEN_ID);
		void			DeleteBitmap(ServerBitmap* bitmap);

	protected:
		BList			fBitmapList;
		int8*			fBuffer;
		BLocker			fLock;
		AreaPool		fMemPool;
};

extern BitmapManager *gBitmapManager;

#endif	/* BITMAP_MANAGER_H_ */
