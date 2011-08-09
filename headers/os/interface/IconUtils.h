/*
 * Copyright 2006-2008, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ICON_UTILS_H
#define _ICON_UTILS_H


#include <Mime.h>

class BBitmap;
class BNode;


class BIconUtils {
								BIconUtils();
								~BIconUtils();
								BIconUtils(const BIconUtils&);
			BIconUtils&			operator=(const BIconUtils&);

public:
	static	status_t			GetIcon(BNode* node,
									const char* vectorIconAttrName,
									const char* smallIconAttrName,
									const char* largeIconAttrName,
									icon_size size, BBitmap* result);

	static	status_t			GetVectorIcon(BNode* node,
									const char* attrName, BBitmap* result);

	static	status_t			GetVectorIcon(const uint8* buffer,
									size_t size, BBitmap* result);

	static	status_t			GetCMAP8Icon(BNode* node,
									const char* smallIconAttrName,
									const char* largeIconAttrName,
									icon_size size, BBitmap* icon);

	static	status_t			ConvertFromCMAP8(BBitmap* source,
									BBitmap* result);
	static	status_t			ConvertToCMAP8(BBitmap* source,
									BBitmap* result);

	static	status_t			ConvertFromCMAP8(const uint8* data,
									uint32 width, uint32 height,
									uint32 bytesPerRow, BBitmap* result);

	static	status_t			ConvertToCMAP8(const uint8* data,
									uint32 width, uint32 height,
									uint32 bytesPerRow, BBitmap* result);
};

#endif	// _ICON_UTILS_H
