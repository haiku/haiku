/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef SHARED_BITMAP_H
#define SHARED_BITMAP_H


#include <Referenceable.h>
#include <String.h>

#include "List.h"


class BBitmap;
class BPositionIO;


class SharedBitmap : public BReferenceable {
public:
		enum Size {
			SIZE_ANY = -1,
			SIZE_16 = 0,
			SIZE_32 = 1,
			SIZE_64 = 2
		};

								SharedBitmap(BBitmap* bitmap);
								SharedBitmap(int32 resourceID);
								SharedBitmap(const char* mimeType);
								SharedBitmap(BPositionIO& data);
								~SharedBitmap();

			const BBitmap*		Bitmap(Size which);

private:
			BBitmap*			_CreateBitmapFromResource(int32 size) const;
			BBitmap*			_CreateBitmapFromBuffer(int32 size) const;
			BBitmap*			_CreateBitmapFromMimeType(int32 size) const;

			BBitmap*			_LoadBitmapFromBuffer(const void* buffer,
									size_t dataSize) const;
			BBitmap*			_LoadArchivedBitmapFromStream(
									BPositionIO& stream) const;
			BBitmap*			_LoadTranslatorBitmapFromStream(
									BPositionIO& stream) const;
			BBitmap*			_LoadIconFromBuffer(const void* buffer,
									size_t dataSize, int32 size) const;

private:
			int32				fResourceID;
			uint8*				fBuffer;
			off_t				fSize;
			BString				fMimeType;
			BBitmap*			fBitmap[3];
};


typedef BReference<SharedBitmap> BitmapRef;
typedef List<BitmapRef, false> BitmapList;


#endif // SHARED_BITMAP_H
