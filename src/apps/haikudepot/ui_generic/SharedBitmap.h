/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2020, Andrew Lindesay <apl@lindesay.co.nz>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef SHARED_BITMAP_H
#define SHARED_BITMAP_H


#include <Referenceable.h>
#include <String.h>

#include "HaikuDepotConstants.h"
#include "List.h"


class BBitmap;
class BDataIO;
class BPositionIO;


class SharedBitmap : public BReferenceable {
public:
								SharedBitmap(BBitmap* bitmap);
								SharedBitmap(int32 resourceID);
								SharedBitmap(const char* mimeType);
								SharedBitmap(BPositionIO& data);
								SharedBitmap(BDataIO& data, size_t size);
								~SharedBitmap();

			const BBitmap*		Bitmap(BitmapSize which);

private:
			void				_InitWithData(BDataIO& data, size_t size);
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
			BBitmap*			fBitmap[4];
};


typedef BReference<SharedBitmap> BitmapRef;
typedef List<BitmapRef, false> BitmapList;


#endif // SHARED_BITMAP_H
