/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "SharedBitmap.h"

#include <algorithm>
#include <stdio.h>

#include <Application.h>
#include <Bitmap.h>
#include <DataIO.h>
#include <IconUtils.h>
#include <Message.h>
#include <MimeType.h>
#include <Resources.h>
#include <TranslationUtils.h>

#include "support.h"


SharedBitmap::SharedBitmap(BBitmap* bitmap)
	:
	BReferenceable(),
	fResourceID(-1),
	fBuffer(NULL),
	fSize(0),
	fMimeType()
{
	fBitmap[0] = bitmap;
	fBitmap[1] = NULL;
	fBitmap[2] = NULL;
	fBitmap[3] = NULL;
}


SharedBitmap::SharedBitmap(int32 resourceID)
	:
	BReferenceable(),
	fResourceID(resourceID),
	fBuffer(NULL),
	fSize(0),
	fMimeType()
{
	fBitmap[0] = NULL;
	fBitmap[1] = NULL;
	fBitmap[2] = NULL;
	fBitmap[3] = NULL;
}


SharedBitmap::SharedBitmap(const char* mimeType)
	:
	BReferenceable(),
	fResourceID(-1),
	fBuffer(NULL),
	fSize(0),
	fMimeType(mimeType)
{
	fBitmap[0] = NULL;
	fBitmap[1] = NULL;
	fBitmap[2] = NULL;
	fBitmap[3] = NULL;
}


SharedBitmap::SharedBitmap(BPositionIO& data)
	:
	BReferenceable(),
	fResourceID(-1),
	fBuffer(NULL),
	fSize(0),
	fMimeType()
{
	status_t status = data.GetSize(&fSize);
	const off_t kMaxSize = 1024 * 1024;
	if (status == B_OK && fSize > 0 && fSize <= kMaxSize) {
		fBuffer = new(std::nothrow) uint8[fSize];
		if (fBuffer != NULL) {
			data.Seek(0, SEEK_SET);

			off_t bytesRead = 0;
			size_t chunkSize = std::min((off_t)4096, fSize);
			while (bytesRead < fSize) {
				ssize_t read = data.Read(fBuffer + bytesRead, chunkSize);
				if (read > 0)
					bytesRead += read;
				else
					break;
			}

			if (bytesRead != fSize) {
				delete[] fBuffer;
				fBuffer = NULL;
				fSize = 0;
			}
		} else
			fSize = 0;
	} else {
		fprintf(stderr, "SharedBitmap(): Stream too large: %" B_PRIi64
			", max: %" B_PRIi64 "\n", fSize, kMaxSize);
	}

	fBitmap[0] = NULL;
	fBitmap[1] = NULL;
	fBitmap[2] = NULL;
	fBitmap[3] = NULL;
}


SharedBitmap::~SharedBitmap()
{
	delete fBitmap[0];
	delete fBitmap[1];
	delete fBitmap[2];
	delete fBitmap[3];
	delete[] fBuffer;
}


const BBitmap*
SharedBitmap::Bitmap(Size which)
{
	if (fResourceID == -1 && fMimeType.Length() == 0 && fBuffer == NULL)
		return fBitmap[0];

	int32 index = 0;
	int32 size = 16;

	switch (which) {
		default:
		case SIZE_16:
			break;

		case SIZE_22:
			index = 1;
			size = 22;
			break;

		case SIZE_32:
			index = 2;
			size = 32;
			break;

		case SIZE_64:
			index = 3;
			size = 64;
			break;
	}

	if (fBitmap[index] == NULL) {
		if (fResourceID >= 0)
			fBitmap[index] = _CreateBitmapFromResource(size);
		else if (fBuffer != NULL)
			fBitmap[index] = _CreateBitmapFromBuffer(size);
		else if (fMimeType.Length() > 0)
			fBitmap[index] = _CreateBitmapFromMimeType(size);
	}

	return fBitmap[index];
}


BBitmap*
SharedBitmap::_CreateBitmapFromResource(int32 size) const
{
	BResources resources;
	status_t status = get_app_resources(resources);
	if (status != B_OK)
		return NULL;

	size_t dataSize;
	const void* data = resources.LoadResource(B_VECTOR_ICON_TYPE, fResourceID,
		&dataSize);
	if (data != NULL)
		return _LoadIconFromBuffer(data, dataSize, size);

	data = resources.LoadResource(B_MESSAGE_TYPE, fResourceID, &dataSize);
	if (data != NULL)
		return _LoadBitmapFromBuffer(data, dataSize);

	return NULL;
}


BBitmap*
SharedBitmap::_CreateBitmapFromBuffer(int32 size) const
{
	BBitmap* bitmap = _LoadIconFromBuffer(fBuffer, fSize, size);

	if (bitmap == NULL)
		bitmap = _LoadBitmapFromBuffer(fBuffer, fSize);

	return bitmap;
}


BBitmap*
SharedBitmap::_CreateBitmapFromMimeType(int32 size) const
{
	BMimeType mimeType(fMimeType.String());
	status_t status = mimeType.InitCheck();
	if (status != B_OK)
		return NULL;

	BBitmap* bitmap = new BBitmap(BRect(0, 0, size - 1, size - 1), 0, B_RGBA32);
	status = bitmap->InitCheck();
	if (status == B_OK)
		status = mimeType.GetIcon(bitmap, B_MINI_ICON);

	if (status != B_OK) {
		delete bitmap;
		bitmap = NULL;
	}

	return bitmap;
}


BBitmap*
SharedBitmap::_LoadBitmapFromBuffer(const void* buffer, size_t size) const
{
	BMemoryIO stream(buffer, size);

	// Try to read as an archived bitmap.
	BBitmap* bitmap = _LoadArchivedBitmapFromStream(stream);

	if (bitmap == NULL) {
		// Try to read as a translator bitmap
		stream.Seek(0, SEEK_SET);
		bitmap = _LoadTranslatorBitmapFromStream(stream);
	}

	if (bitmap != NULL) {
		status_t status = bitmap->InitCheck();
		if (status != B_OK) {
			delete bitmap;
			bitmap = NULL;
		}
	}

	return bitmap;
}


BBitmap*
SharedBitmap::_LoadArchivedBitmapFromStream(BPositionIO& stream) const
{
	BMessage archive;
	status_t status = archive.Unflatten(&stream);
	if (status != B_OK)
		return NULL;

	return new BBitmap(&archive);
}


BBitmap*
SharedBitmap::_LoadTranslatorBitmapFromStream(BPositionIO& stream) const
{
	return BTranslationUtils::GetBitmap(&stream);
}


BBitmap*
SharedBitmap::_LoadIconFromBuffer(const void* data, size_t dataSize,
	int32 size) const
{
	BBitmap* bitmap = new BBitmap(BRect(0, 0, size - 1, size - 1), 0,
		B_RGBA32);
	status_t status = bitmap->InitCheck();
	if (status == B_OK) {
		status = BIconUtils::GetVectorIcon(
			reinterpret_cast<const uint8*>(data), dataSize, bitmap);
	};

	if (status != B_OK) {
		delete bitmap;
		bitmap = NULL;
	}

	return bitmap;
}

