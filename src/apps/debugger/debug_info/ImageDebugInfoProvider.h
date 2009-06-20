/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef IMAGE_DEBUG_INFO_PROVIDER_H
#define IMAGE_DEBUG_INFO_PROVIDER_H

#include <SupportDefs.h>


class Image;
class ImageDebugInfo;


class ImageDebugInfoProvider {
public:
	virtual						~ImageDebugInfoProvider();

	virtual	status_t			GetImageDebugInfo(Image* image,
									ImageDebugInfo*& _info) = 0;
										// returns a reference
};


#endif	// IMAGE_DEBUG_INFO_PROVIDER_H
