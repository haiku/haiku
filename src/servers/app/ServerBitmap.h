/*
 * Copyright 2001-2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef SERVER_BITMAP_H
#define SERVER_BITMAP_H


#include <GraphicsDefs.h>
#include <Rect.h>
#include <OS.h>

#include <Referenceable.h>

#include "ClientMemoryAllocator.h"


class BitmapManager;
class HWInterface;
class Overlay;
class ServerApp;


/*!	\class ServerBitmap ServerBitmap.h
	\brief Bitmap class used inside the server.

	This class is not directly allocated or freed. Instead, it is
	managed by the BitmapManager class. It is also the base class for
	all cursors. Every BBitmap has a shadow ServerBitmap object.
*/
class ServerBitmap : public BReferenceable {
public:
	inline	bool			IsValid() const
								{ return fBuffer != NULL; }

	inline	uint8*			Bits() const
								{ return fBuffer; }
	inline	uint32			BitsLength() const
								{ return (uint32)(fBytesPerRow * fHeight); }

	inline	BRect			Bounds() const
								{ return BRect(0, 0, fWidth - 1, fHeight - 1); }
	inline	int32			Width() const
								{ return fWidth; }
	inline	int32			Height() const
								{ return fHeight; }

	inline	int32			BytesPerRow() const
								{ return fBytesPerRow; }

	inline	color_space		ColorSpace() const
								{ return fSpace; }
	inline	uint32			Flags() const
								{ return fFlags; }

	//! Returns the identifier token for the bitmap
	inline	int32			Token() const
								{ return fToken; }

			area_id			Area() const;
			uint32			AreaOffset() const;

			void			SetOverlay(::Overlay* overlay);
			::Overlay*		Overlay() const;

			void			SetOwner(ServerApp* owner);
			ServerApp*		Owner() const;

	//! Does a shallow copy of the bitmap passed to it
	inline	void			ShallowCopy(const ServerBitmap *from);

			status_t		ImportBits(const void *bits, int32 bitsLength,
								int32 bytesPerRow, color_space colorSpace);
			status_t		ImportBits(const void *bits, int32 bitsLength,
								int32 bytesPerRow, color_space colorSpace,
								BPoint from, BPoint to, int32 width,
								int32 height);

			void			PrintToStream();

protected:
	friend class BitmapManager;

							ServerBitmap(BRect rect, color_space space,
								uint32 flags, int32 bytesPerRow = -1,
								screen_id screen = B_MAIN_SCREEN_ID);
							ServerBitmap(const ServerBitmap* bmp);
	virtual					~ServerBitmap();

			void			AllocateBuffer();

protected:
			ClientMemory	fClientMemory;
			AreaMemory*		fMemory;
			::Overlay*		fOverlay;
			uint8*			fBuffer;

			int32			fWidth;
			int32			fHeight;
			int32			fBytesPerRow;
			color_space		fSpace;
			uint32			fFlags;

			ServerApp*		fOwner;
			int32			fToken;
};

class UtilityBitmap : public ServerBitmap {
public:
							UtilityBitmap(BRect rect, color_space space,
								uint32 flags, int32 bytesperline = -1,
								screen_id screen = B_MAIN_SCREEN_ID);
							UtilityBitmap(const ServerBitmap* bmp);

							UtilityBitmap(const uint8* alreadyPaddedData,
								uint32 width, uint32 height,
								color_space format);

	virtual					~UtilityBitmap();
};


//! (only for server bitmaps)
void
ServerBitmap::ShallowCopy(const ServerBitmap* from)
{
	if (!from)
		return;

	fBuffer = from->fBuffer;
	fWidth = from->fWidth;
	fHeight = from->fHeight;
	fBytesPerRow = from->fBytesPerRow;
	fSpace = from->fSpace;
	fFlags = from->fFlags;
	fToken = from->fToken;
}

#endif	// SERVER_BITMAP_H
