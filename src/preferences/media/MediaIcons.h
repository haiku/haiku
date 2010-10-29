/*
 * Copyright 2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef __MEDIA_ICONS_H
#define __MEDIA_ICONS_H

#include <Bitmap.h>


class BResources;


struct MediaIcons {
								MediaIcons();

			BBitmap				devicesIcon;
			BBitmap				mixerIcon;
			BBitmap				tvIcon;
			BBitmap				camIcon;
			BBitmap				micIcon;
			BBitmap				speakerIcon;

			BRect				IconRectAt(const BPoint& topLeft);
private:

	static	const BRect			sBounds;

			void				_LoadBitmap(BResources* resources, int32 id,
									BBitmap* bitmap);
};

#endif
