//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
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
class ServerBitmap
{
public:
	ServerBitmap(BRect rect,color_space space, int32 flags,
		int32 bytesperline=-1, screen_id screen=B_MAIN_SCREEN_ID);
	ServerBitmap(const ServerBitmap *bmp);
	~ServerBitmap(void);

	/*!
		\brief Returns the area in which the buffer resides
		\return
		- \c B_ERROR if the buffer is not allocated in an area
		- area_id for the buffer
	*/
	area_id Area(void) const { return _area; }
	
	// Returns the offset of the bitmap in its area
	int32 AreaOffset(void) { return _offset; }
	
	//! Returns the bitmap's buffer
	uint8 *Bits(void) const { return _buffer; }
	uint32 BitsLength(void) const;

	//! Returns the size of the bitmap
	BRect Bounds() const { return BRect(0,0,_width-1,_height-1); };
	
	//! Returns the number of bytes in each row, including padding
	int32 BytesPerRow(void) const { return _bytesperrow; };
	
	//! Returns the pixel color depth
	uint8 BitsPerPixel(void) const { return _bpp; } 
	
	//! Returns the color space of the bitmap
	color_space ColorSpace(void) const { return _space; }
	
	//! Returns the width of the bitmap
	int32 Width(void) const { return _width; }
	
	//! Returns the height of the bitmap
	int32 Height(void) const { return _height; }

	//! Returns whether the bitmap is valid
	bool InitCheck(void) const { return _initialized; }

	//! Returns the identifier token for the bitmap
	int32 Token(void) const { return _token; }
	
protected:
	friend class BitmapManager;
	friend class PicturePlayer;

	//! Internal function used by the BitmapManager.
	void _SetArea(area_id ID) { _area=ID; }
	
	//! Internal function used by the BitmapManager.
	void _SetBuffer(void *ptr) { _buffer=(uint8*)ptr; }
	void _AllocateBuffer(void);
	void _FreeBuffer(void);
	
	void _HandleSpace(color_space space, int32 bytesperline=-1);

	bool _initialized;
	area_id _area;
	uint8 *_buffer;

	int32 _width,_height;
	int32 _bytesperrow;
	color_space _space;
	int32 _flags;
	int _bpp;
	int32 _token;
	int32 _offset;
};

class UtilityBitmap : public ServerBitmap
{
public:
	UtilityBitmap(BRect rect,color_space space, int32 flags,
		int32 bytesperline=-1, screen_id screen=B_MAIN_SCREEN_ID);
	UtilityBitmap(const ServerBitmap *bmp);
	~UtilityBitmap(void);
};

#endif
