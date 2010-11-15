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

	struct IconSet {
								IconSet();

			BBitmap				inputIcon;
			BBitmap				outputIcon;
	};


			BBitmap				devicesIcon;
			BBitmap				mixerIcon;

			IconSet				audioIcons;
			IconSet				videoIcons;

	static	BRect				IconRectAt(const BPoint& topLeft);

	static	const BRect			sBounds;

private:

			void				_LoadBitmap(BResources* resources, int32 id,
									BBitmap* bitmap);
};

#endif
