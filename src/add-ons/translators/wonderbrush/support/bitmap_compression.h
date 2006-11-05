/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef BITMAP_COMPRESSION_H
#define BITMAP_COMPRESSION_H

#include <SupportDefs.h>

class BBitmap;
class BMessage;

status_t
archive_bitmap(const BBitmap* bitmap, BMessage* into, const char* fieldName);

status_t
extract_bitmap(BBitmap** bitmap, const BMessage* from, const char* fieldName);

#endif // BITMAP_COMPRESSION_H
