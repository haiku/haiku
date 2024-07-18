/*
 * Copyright 2024, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef SHARED_ICONS_H
#define SHARED_ICONS_H


#include "BitmapHolder.h"


class SharedIcons
{

public:
	static	void			UnsetAllIcons();

	// icons from application resources
	static	BitmapHolderRef	IconStarBlue12Scaled();
	static	BitmapHolderRef	IconStarBlue16Scaled();
	static	BitmapHolderRef	IconStarGrey16Scaled();
	static	BitmapHolderRef	IconInstalled16Scaled();
	static	BitmapHolderRef	IconArrowLeft22Scaled();
	static	BitmapHolderRef	IconArrowRight22Scaled();

	// icons from mime types
	static	BitmapHolderRef	IconHTMLPackage16Scaled();

private:
	static	BitmapHolderRef	_CreateIconForResource(int32 resourceID, uint32 size);
	static	BitmapHolderRef _CreateIconForMimeType(const char* mimeType, uint32 size);

	static	status_t		_CreateIconForResourceChecked(int32 resourceID, uint32 size,
    								BitmapHolderRef* bitmapHolderRef);
   	static	status_t		_CreateIconForMimeTypeChecked(const char* mimeTypeStr, uint32 size,
       								BitmapHolderRef* bitmapHolderRef);

private:
	static	BitmapHolderRef	sIconStarBlue12Scaled;
	static	BitmapHolderRef	sIconStarBlue16Scaled;
	static	BitmapHolderRef	sIconStarGrey16Scaled;
	static	BitmapHolderRef	sIconInstalled16Scaled;
	static	BitmapHolderRef	sIconArrowLeft22Scaled;
	static	BitmapHolderRef	sIconArrowRight22Scaled;
	static	BitmapHolderRef	sIconHTMLPackage16Scaled;

};


#endif // SHARED_ICONS_H
