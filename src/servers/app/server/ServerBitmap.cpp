//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, Haiku, Inc.
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
//	File Name:		ServerBitmap.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Bitmap class used by the server
//  
//------------------------------------------------------------------------------
#include "ServerBitmap.h"

/*!
	\brief Constructor called by the BitmapManager (only).
	\param rect Size of the bitmap.
	\param space Color space of the bitmap
	\param flags Various bitmap flags to tweak the bitmap as defined in Bitmap.h
	\param bytesperline Number of bytes in each row. -1 implies the default value. Any
	value less than the the default will less than the default will be overridden, but any value
	greater than the default will result in the number of bytes specified.
	\param screen Screen assigned to the bitmap.
*/
ServerBitmap::ServerBitmap(BRect rect,color_space space, int32 flags,
		int32 bytesperline, screen_id screen)
{
	fInitialized=false;

	// WARNING: '1' is added to the width and height. Same is done in FBBitmap
	// subclass, so if you modify here make sure to do the same under FBBitmap::SetSize(...)
	fWidth=rect.IntegerWidth()+1;
	fHeight=rect.IntegerHeight()+1;
	fSpace=space;
	fArea=B_ERROR;
	fBuffer=NULL;
	fFlags=flags;

	_HandleSpace(space, bytesperline);
}

//! Copy constructor does not copy the buffer.
ServerBitmap::ServerBitmap(const ServerBitmap *bmp)
{
	fInitialized=false;
	fArea=B_ERROR;
	fBuffer=NULL;

	if(bmp)
	{
		fWidth=bmp->fWidth;
		fHeight=bmp->fHeight;
		fBytesPerRow=bmp->fBytesPerRow;
		fSpace=bmp->fSpace;
		fFlags=bmp->fFlags;
		fBitsPerPixel=bmp->fBitsPerPixel;
	}
	else
	{
		fWidth=0;
		fHeight=0;
		fBytesPerRow=0;
		fSpace=B_NO_COLOR_SPACE;
		fFlags=0;
		fBitsPerPixel=0;
	}
}

/*!
	\brief Empty. Defined for subclasses.
*/
ServerBitmap::~ServerBitmap(void)
{
}

/*!
	\brief Gets the number of bytes occupied by the bitmap, including padding bytes.
	\return The number of bytes occupied by the bitmap, including padding.
*/
uint32 ServerBitmap::BitsLength(void) const
{
	return (uint32)(fBytesPerRow*fHeight);
}

/*! 
	\brief Internal function used by subclasses
	
	Subclasses should call this so the buffer can automagically
	be allocated on the heap.
*/
void ServerBitmap::_AllocateBuffer(void)
{
	if(fBuffer!=NULL)
		delete fBuffer;
	fBuffer=new uint8[BitsLength()];
}

/*!
	\brief Internal function used by subclasses
	
	Subclasses should call this to free the internal buffer.
*/
void ServerBitmap::_FreeBuffer(void)
{
	if(fBuffer!=NULL)
	{
		delete fBuffer;
		fBuffer=NULL;
	}
}

/*!
	\brief Internal function used to translate color space values to appropriate internal
	values. 
	\param space Color space for the bitmap.
	\param bytesperline Number of bytes per row.
*/
void ServerBitmap::_HandleSpace(color_space space, int32 bytesperline)
{
	// Big convoluted mess just to handle every color space and dword align
	// the buffer	
	switch(space)
	{
		// Buffer is dword-aligned, so nothing need be done
		// aside from allocate the memory
		case B_RGB32:
		case B_RGBA32:
		case B_RGB32_BIG:
		case B_RGBA32_BIG:
		case B_UVL32:
		case B_UVLA32:
		case B_LAB32:
		case B_LABA32:
		case B_HSI32:
		case B_HSIA32:
		case B_HSV32:
		case B_HSVA32:
		case B_HLS32:
		case B_HLSA32:
		case B_CMY32:
		case B_CMYA32:
		case B_CMYK32:

		// 24-bit = 32-bit with extra 8 bits ignored
		case B_RGB24_BIG:
		case B_RGB24:
		case B_LAB24:
		case B_UVL24:
		case B_HSI24:
		case B_HSV24:
		case B_HLS24:
		case B_CMY24:
		{
			if(bytesperline<(fWidth*4))
				fBytesPerRow=fWidth*4;
			else
				fBytesPerRow=bytesperline;
			fBitsPerPixel=32;
			break;
		}
		// Calculate size and dword-align

		// 1-bit
		case B_GRAY1:
		{
			int32 numbytes=fWidth>>3;
			if((fWidth % 8) != 0)
				numbytes++;
			if(bytesperline<numbytes)
			{
				for(int8 i=0;i<4;i++)
				{
					if( (numbytes+i)%4==0)
					{
						fBytesPerRow=numbytes+i;
						break;
					}
				}
			}
			else
				fBytesPerRow=bytesperline;
			fBitsPerPixel=1;
		}		

		// 8-bit
		case B_CMAP8:
		case B_GRAY8:
		case B_YUV411:
		case B_YUV420:
		case B_YCbCr422:
		case B_YCbCr411:
		case B_YCbCr420:
		case B_YUV422:
		{
			if(bytesperline<fWidth)
			{
				for(int8 i=0;i<4;i++)
				{
					if( (fWidth+i)%4==0)
					{
						fBytesPerRow=fWidth+i;
						break;
					}
				}
			}
			else
				fBytesPerRow=bytesperline;
			fBitsPerPixel=8;
			break;
		}

		// 16-bit
		case B_YUV9:
		case B_YUV12:
		case B_RGB15:
		case B_RGBA15:
		case B_RGB16:
		case B_RGB16_BIG:
		case B_RGB15_BIG:
		case B_RGBA15_BIG:
		case B_YCbCr444:
		case B_YUV444:
		{
			if(bytesperline<fWidth*2)
			{
				if( (fWidth*2) % 4 !=0)
					fBytesPerRow=(fWidth+1)*2;
				else
					fBytesPerRow=fWidth*2;
			}
			else
				fBytesPerRow=bytesperline;
			fBitsPerPixel=16;
			break;
		}
		case B_NO_COLOR_SPACE:
			fBitsPerPixel=0;
			break;
	}
}

UtilityBitmap::UtilityBitmap(BRect rect,color_space space, int32 flags,
	int32 bytesperline, screen_id screen)
: ServerBitmap(rect,space,flags,bytesperline,screen)
{
	_AllocateBuffer();
}

UtilityBitmap::UtilityBitmap(const ServerBitmap *bmp)
: ServerBitmap(bmp)
{
	_AllocateBuffer();
	if(bmp->Bits())
		memcpy(Bits(),bmp->Bits(),bmp->BitsLength());
}

UtilityBitmap::~UtilityBitmap(void)
{
	_FreeBuffer();
}
