/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 */
#ifndef _SERVER_BITMAP_H_
#define _SERVER_BITMAP_H_


#include <GraphicsDefs.h>
#include <Rect.h>
#include <OS.h>


class BitmapManager;

/*!
	\class ServerBitmap ServerBitmap.h
	\brief Bitmap class used inside the server.
	
	This class is not directly allocated or freed. Instead, it is 
	managed by the BitmapManager class. It is also the base class for 
	all cursors. Every BBitmap has a shadow ServerBitmap object.
*/
class ServerBitmap {
 public:
			void			Acquire();

	/*!
		\brief Returns the area in which the buffer resides
		\return
		- \c B_ERROR if the buffer is not allocated in an area
		- area_id for the buffer
	*/
	inline	area_id			Area() const
								{ return fArea; }

	// Returns the offset of the bitmap in its area
	inline	int32			AreaOffset() const
								{ return fOffset; }

	//! Returns the bitmap's buffer
	inline	uint8*			Bits() const
								{ return fBuffer; }

	inline	uint32			BitsLength() const
								{ return (uint32)(fBytesPerRow * fHeight); }

	inline	BRect			Bounds() const
								{ return BRect(0, 0, fWidth - 1, fHeight - 1); }

	//! Returns the number of bytes in each row, including padding
	inline	int32			BytesPerRow() const
								{ return fBytesPerRow; }

	inline	uint8			BitsPerPixel() const
								{ return fBitsPerPixel; } 

	inline	color_space		ColorSpace() const
								{ return fSpace; }

	//! Returns the bitmap's width in pixels per row
	inline	int32			Width() const
								{ return fWidth; }

	//! Returns the bitmap's row count
	inline	int32			Height() const
								{ return fHeight; }

	inline	bool			IsValid() const
								{ return fInitialized; }

	//! Returns the identifier token for the bitmap
	inline	int32			Token() const
								{ return fToken; }

	//! Does a shallow copy of the bitmap passed to it
	inline	void			ShallowCopy(const ServerBitmap *from);

protected:
	friend class BitmapManager;
	friend class PicturePlayer;

							ServerBitmap(BRect rect,
										 color_space space,
										 int32 flags,
										 int32 bytesperline = -1,
										 screen_id screen = B_MAIN_SCREEN_ID);
							ServerBitmap(const ServerBitmap* bmp);
	virtual					~ServerBitmap();

	//! used by the BitmapManager
	inline	void			_SetArea(area_id ID)
								{ fArea = ID; }

	//! used by the BitmapManager
	inline	void			_SetBuffer(void *ptr)
								{ fBuffer = (uint8*)ptr; }

			bool			_Release();

			void			_AllocateBuffer();
			void			_FreeBuffer();

			void			_HandleSpace(color_space space,
										 int32 bytesperline = -1);

			bool			fInitialized;
			area_id			fArea;
			uint8*			fBuffer;
			int32			fReferenceCount;

			int32			fWidth;
			int32			fHeight;
			int32			fBytesPerRow;
			color_space		fSpace;
			int32			fFlags;
			int				fBitsPerPixel;
			int32			fToken;
			int32			fOffset;
};

class UtilityBitmap : public ServerBitmap {
 public:
							UtilityBitmap(BRect rect,
										  color_space space,
										  int32 flags,
										  int32 bytesperline = -1,
										  screen_id screen = B_MAIN_SCREEN_ID);
							UtilityBitmap(const ServerBitmap* bmp);

							UtilityBitmap(const uint8* alreadyPaddedData,
										  uint32 width,
										  uint32 height,
										  color_space format);


	virtual					~UtilityBitmap();
};

// ShallowCopy
void
ServerBitmap::ShallowCopy(const ServerBitmap* from)
{
	if (!from)
		return;

	fInitialized	= from->fInitialized;
	fArea			= from->fArea;
	fBuffer			= from->fBuffer;
	fWidth			= from->fWidth;
	fHeight			= from->fHeight;
	fBytesPerRow	= from->fBytesPerRow;
	fSpace			= from->fSpace;
	fFlags			= from->fFlags;
	fBitsPerPixel	= from->fBitsPerPixel;
	fToken			= from->fToken;
	fOffset			= from->fOffset;
}


#endif // _SERVER_BITMAP_H_
