/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 */

#ifndef ICON_UTILS_H
#define ICON_UTILS_H

#include <SupportDefs.h>

class BBitmap;
class BNode;

// This class is a little different from many other classes.
// You don't create an instance of it; you just call its various
// static member functions for utility-like operations.
class BIconUtils {
								BIconUtils();
								~BIconUtils();
								BIconUtils(const BIconUtils&);
			BIconUtils&			operator=(const BIconUtils&);

 public:

	static	status_t			GetVectorIcon(BNode* node,
											  const char* attrName,
											  BBitmap* result);

	static	status_t			GetVectorIcon(const uint8* buffer,
											  size_t size,
											  BBitmap* result);


	static	status_t			ConvertFromCMAP8(BBitmap* source,
												 BBitmap* result);

	static	status_t			ConvertFromCMAP8(const uint8* data,
												 uint32 width, uint32 height,
												 uint32 bytesPerRow,
												 BBitmap* result);
};

#endif // ICON_UTILS_H
