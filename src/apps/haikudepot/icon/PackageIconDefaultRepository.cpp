/*
 * Copyright 2025, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "PackageIconDefaultRepository.h"


#include <IconUtils.h>

#include "Logger.h"


static const int32 kCacheLimit = 50;


PackageIconDefaultRepository::PackageIconDefaultRepository()
	:
	fVectorData(NULL),
	fVectorDataSize(0),
	fCache(kCacheLimit)
{
	_InitDefaultVectorIcon();
}


PackageIconDefaultRepository::~PackageIconDefaultRepository()
{
	delete fVectorData;
}


status_t
PackageIconDefaultRepository::GetIcon(const BString& pkgName, uint32 size,
	BitmapHolderRef& bitmapHolderRef)
{

	if (fVectorData == NULL)
		return B_NOT_INITIALIZED;

	bitmapHolderRef.Unset();

	status_t status = B_OK;
	HashString key(BString() << size);

	if (!fCache.ContainsKey(key)) {
		BBitmap* bitmap = NULL;

		if (status == B_OK) {
			bitmap = new BBitmap(BRect(0, 0, size - 1, size - 1), 0, B_RGBA32);
			status = bitmap->InitCheck();
		}

		if (status == B_OK)
			status = BIconUtils::GetVectorIcon(fVectorData, fVectorDataSize, bitmap);

		if (status == B_OK) {
			HDINFO("did create default package icon size %" B_PRId32, size);
			BitmapHolderRef bitmapHolder(new(std::nothrow) BitmapHolder(bitmap), true);
			fCache.Put(key, bitmapHolder);
		} else {
			delete bitmap;
			bitmap = NULL;
		}
	}

	if (status == B_OK)
		bitmapHolderRef.SetTo(fCache.Get(key).Get());
	else
		HDERROR("failed to create default package icon size %" B_PRId32, size);

	return status;
}


void
PackageIconDefaultRepository::_InitDefaultVectorIcon()
{
	if (fVectorData != NULL) {
		delete fVectorData;
		fVectorData = NULL;
	}

	fVectorDataSize = 0;

	BMimeType mimeType("application/x-vnd.haiku-package");
	status_t status = mimeType.InitCheck();

	if (status != B_OK)
		return;

	uint8* data;
	size_t dataSize;

	if (mimeType.GetIcon(&data, &dataSize) != B_OK)
		debugger("the default icon was unable to be acquired");

	fVectorData = new(std::nothrow) uint8[dataSize];

	if (fVectorData == NULL)
		HDFATAL("unable to allocate memory for the default icon");

	memcpy(fVectorData, data, dataSize);
	fVectorDataSize = dataSize;
}
