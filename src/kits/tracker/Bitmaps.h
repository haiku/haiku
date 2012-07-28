/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2000, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/
#ifndef __BITS__
#define __BITS__


#include <Locker.h>
#include <Resources.h>
#include <Mime.h>
#include <image.h>

#include "TrackerIcons.h"


class BBitmap;

namespace BPrivate {

class BImageResources
{
	// convenience class for accessing
public:
	BImageResources(void* memAddr);
	~BImageResources();

	BResources* ViewResources();
	const BResources* ViewResources() const;

	status_t FinishResources(BResources*) const;

	const void* LoadResource(type_code type, int32 id,
		size_t* outSize) const;
	const void* LoadResource(type_code type, const char* name,
		size_t* outSize) const;
		// load a resource from the Tracker executable, just like the
		// corresponding functions in BResources.  These methods are
		// thread-safe.

	status_t GetIconResource(int32 id, icon_size size, BBitmap* dest) const;
		// this is a wrapper around LoadResource(), for retrieving
		// B_LARGE_ICON and B_MINI_ICON ('ICON' and 'MICN' respectively)
		// resources.  this does sanity checking on the found data,
		// and if all is okay blasts it into the 'dest' bitmap.

	status_t GetIconResource(int32 id, const uint8** iconData,
							 size_t* iconSize) const;
		// this is a wrapper around LoadResource(), for retrieving
		// the vector icon data

 	status_t GetBitmapResource(type_code type, int32 id, BBitmap** out) const;
 		// this is a wrapper around LoadResource(), for retrieving
 		// arbitrary bitmaps.  the resource with the given type and
 		// id is looked up, and a BBitmap created from it and returned
 		// in <out>.  currently it can only create bitmaps from data
 		// that is an archived bitmap object.

private:
	image_id find_image(void* memAddr) const;
	
	mutable BLocker fLock;
	BResources fResources;
};


extern
#ifdef _IMPEXP_TRACKER
_IMPEXP_TRACKER
#endif
BImageResources* GetTrackerResources();


} // namespace BPrivate

using namespace BPrivate;

#endif	// __BITS__
