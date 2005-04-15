//------------------------------------------------------------------------------
//	Copyright (c) 2001-2005, Haiku, Inc.
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		ServerBitmap.h
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Bitmap class used inside the server
//  
//------------------------------------------------------------------------------
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
							ServerBitmap(BRect rect,
										 color_space space,
										 int32 flags,
										 int32 bytesperline = -1,
										 screen_id screen = B_MAIN_SCREEN_ID);
							ServerBitmap(const ServerBitmap* bmp);

	virtual					~ServerBitmap();

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

	inline	bool			InitCheck() const
								{ return fInitialized; }

	//! Returns the identifier token for the bitmap
	inline	int32			Token() const
								{ return fToken; }
	
	//! Does a shallow copy of the bitmap passed to it
	inline	void			ShallowCopy(const ServerBitmap *from);
	
protected:
	friend class BitmapManager;
	friend class PicturePlayer;

	//! used by the BitmapManager
	inline	void			_SetArea(area_id ID)
								{ fArea=ID; }
	
	//! used by the BitmapManager
	inline	void			_SetBuffer(void *ptr)
								{ fBuffer=(uint8*)ptr; }

			void			_AllocateBuffer();
			void			_FreeBuffer();
	
			void			_HandleSpace(color_space space,
										 int32 bytesperline = -1);

			bool			fInitialized;
			area_id			fArea;
			uint8*			fBuffer;
		
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
