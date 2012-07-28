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

#include "Bitmaps.h"
#include "Utilities.h"

#include <Autolock.h>
#include <Bitmap.h>
#include <Debug.h>
#include <DataIO.h>
#include <File.h>
#include <String.h>
#include <SupportDefs.h>

#ifdef __HAIKU__
#	include <IconUtils.h>
#endif


BImageResources::BImageResources(void* memAddr)
{
	image_id image = find_image(memAddr);
	image_info info;
	if (get_image_info(image, &info) == B_OK) {
#if _SUPPORTS_RESOURCES
		BFile file(&info.name[0], B_READ_ONLY);
#else
		BString name(&info.name[0]);
		name += ".rsrc";
		BFile file(name.String(), B_READ_ONLY);
#endif
		if (file.InitCheck() == B_OK)
			fResources.SetTo(&file);
	}
}


BImageResources::~BImageResources()
{
}


const BResources*
BImageResources::ViewResources() const
{
	if (fLock.Lock() != B_OK)
		return NULL;

	return &fResources;
}


BResources*
BImageResources::ViewResources()
{
	if (fLock.Lock() != B_OK)
		return NULL;

	return &fResources;
}


status_t
BImageResources::FinishResources(BResources* res) const
{
	ASSERT(res == &fResources);
	if (res != &fResources)
		return B_BAD_VALUE;

	fLock.Unlock();
	return B_OK;
}


const void*
BImageResources::LoadResource(type_code type, int32 id,
	size_t* out_size) const
{
	// Serialize execution.
	// Looks like BResources is not really thread safe. We should
	// clean that up in the future and remove the locking from here.
	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return 0;

	// Return the resource.  Because we never change the BResources
	// object, the returned data will not change until TTracker is
	// destroyed.
	return const_cast<BResources*>(&fResources)->LoadResource(type, id,
		out_size);
}


const void*
BImageResources::LoadResource(type_code type, const char* name,
	size_t* out_size) const
{
	// Serialize execution.
	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return NULL;

	// Return the resource.  Because we never change the BResources
	// object, the returned data will not change until TTracker is
	// destroyed.
	return const_cast<BResources*>(&fResources)->LoadResource(type, name,
		out_size);
}


status_t
BImageResources::GetIconResource(int32 id, icon_size size,
	BBitmap* dest) const
{
	size_t length = 0;
	const void* data;

#ifdef __HAIKU__
	// try to load vector icon
	data = LoadResource(B_VECTOR_ICON_TYPE, id, &length);
	if (data != NULL
		&& BIconUtils::GetVectorIcon((uint8*)data, length, dest) == B_OK) {
		return B_OK;
	}
#endif

	// fall back to R5 icon
	if (size != B_LARGE_ICON && size != B_MINI_ICON)
		return B_ERROR;

	length = 0;
	data = LoadResource(size == B_LARGE_ICON ? 'ICON' : 'MICN', id, &length);

	if (data == NULL
		|| length != (size_t)(size == B_LARGE_ICON ? 1024 : 256)) {
		TRESPASS();
		return B_ERROR;
	}

#ifdef __HAIKU__
	if (dest->ColorSpace() != B_CMAP8) {
		return BIconUtils::ConvertFromCMAP8((uint8*)data, size, size,
			size, dest);
	}
#endif

	dest->SetBits(data, (int32)length, 0, B_CMAP8);
	return B_OK;
}


status_t
BImageResources::GetIconResource(int32 id, const uint8** iconData,
	size_t* iconSize) const
{
#ifdef __HAIKU__
	// try to load vector icon data from resources
	size_t length = 0;
	const void* data = LoadResource(B_VECTOR_ICON_TYPE, id, &length);
	if (data == NULL)
		return B_ERROR;

	*iconData = (const uint8*)data;
	*iconSize = length;

	return B_OK;
#else
	return B_ERROR;
#endif
}


image_id
BImageResources::find_image(void* memAddr) const
{
	image_info info;
	int32 cookie = 0;
	while (get_next_image_info(0, &cookie, &info) == B_OK)
		if ((info.text <= memAddr
			&& (((uint8*)info.text)+info.text_size) > memAddr)
				|| (info.data <= memAddr
				&& (((uint8*)info.data)+info.data_size) > memAddr)) {
			// Found the image.
			return info.id;
		}

	return -1;
}


status_t
BImageResources::GetBitmapResource(type_code type, int32 id,
	BBitmap** out) const
{
	*out = NULL;

	size_t len = 0;
	const void* data = LoadResource(type, id, &len);

	if (data == NULL) {
		TRESPASS();
		return B_ERROR;
	}

	BMemoryIO stream(data, len);

	// Try to read as an archived bitmap.
	stream.Seek(0, SEEK_SET);
	BMessage archive;
	status_t err = archive.Unflatten(&stream);
	if (err != B_OK)
		return err;

	*out = new BBitmap(&archive);
	if (!*out)
		return B_ERROR;

	err = (*out)->InitCheck();
	if (err != B_OK) {
		delete *out;
		*out = NULL;
	}

	return err;
}


static BLocker resLock;
static BImageResources* resources = NULL;

// This class is used as a static instance to delete the resources
// global object when the image is getting unloaded.
class _TTrackerCleanupResources {
public:
	_TTrackerCleanupResources() { }
	~_TTrackerCleanupResources()
	{
		delete resources;
		resources = NULL;
	}
};


namespace BPrivate {

static _TTrackerCleanupResources CleanupResources;


BImageResources* GetTrackerResources()
{
	if (!resources) {
		BAutolock lock(&resLock);
		resources = new BImageResources(&resources);
	}
	return resources;
}

}
