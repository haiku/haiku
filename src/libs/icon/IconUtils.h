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

	// Utility functions to import a vector icon in "flat icon"
	// format from a BNode attribute or from a flat buffer in
	// memory into the preallocated BBitmap "result".
	// The colorspace of result needs to be B_RGBA32 or at
	// least B_RGB32 (though that makes less sense). The icon
	// will be scaled from it's "native" size of 64x64 to the
	// size of the bitmap, the scale is derived from the bitmap
	// width, the bitmap should have square dimension, or the
	// icon will be cut off at the bottom (or have room left).
	static	status_t			GetVectorIcon(BNode* node,
											  const char* attrName,
											  BBitmap* result);

	static	status_t			GetVectorIcon(const uint8* buffer,
											  size_t size,
											  BBitmap* result);


	// Utility functions to convert from old icon colorspace
	// into colorspace of BBitmap "result" (should be B_RGBA32
	// to make any sense).
	static	status_t			ConvertFromCMAP8(BBitmap* source,
												 BBitmap* result);

	static	status_t			ConvertFromCMAP8(const uint8* data,
												 uint32 width, uint32 height,
												 uint32 bytesPerRow,
												 BBitmap* result);
};

#endif // ICON_UTILS_H
