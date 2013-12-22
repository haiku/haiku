/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _INTERFACE__ICON_H_
#define _INTERFACE__ICON_H_


#include <InterfaceDefs.h>
#include <ObjectList.h>
#include <Rect.h>


class BBitmap;


namespace BPrivate {


class BIcon {
public:
								BIcon();
								~BIcon();

			status_t			SetTo(const BBitmap* bitmap, uint32 flags = 0);

			bool				SetBitmap(BBitmap* bitmap, uint32 which);
			BBitmap*			Bitmap(uint32 which) const;

			status_t			SetExternalBitmap(const BBitmap* bitmap,
									uint32 which, uint32 flags);

			BBitmap*			CreateBitmap(const BRect& bounds,
									color_space colorSpace, uint32 which);
			BBitmap*			CopyBitmap(const BBitmap& bitmapToClone,
									uint32 which);
			void				DeleteBitmaps();

	// convenience methods for icon owners
	static	status_t			UpdateIcon(const BBitmap* bitmap, uint32 flags,
									BIcon*& _icon);
	static	status_t			SetIconBitmap(const BBitmap* bitmap,
									uint32 which, uint32 flags, BIcon*& _icon);

private:
			typedef BObjectList<BBitmap> BitmapList;

private:
	static	BBitmap*			_ConvertToRGB32(const BBitmap* bitmap,
									bool noAppServerLink = false);
	static	status_t			_TrimBitmap(const BBitmap* bitmap,
									bool keepAspect, BBitmap*& _trimmedBitmap);
			status_t			_MakeBitmaps(const BBitmap* bitmap,
									uint32 flags);

private:
			BitmapList			fEnabledBitmaps;
			BitmapList			fDisabledBitmaps;
};


}	// namespace BPrivate


using BPrivate::BIcon;


#endif	// _INTERFACE__ICON_H_
