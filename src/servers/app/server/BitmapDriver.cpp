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
//	File Name:		BitmapDriver.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//				Gabe Yoder <gyoder@stny.rr.com>
//	Description:	Driver to draw on ServerBitmaps
//  
//------------------------------------------------------------------------------
#include "Angle.h"
#include "PatternHandler.h"
#include "BitmapDriver.h"
#include "ServerProtocol.h"
#include "ServerBitmap.h"
#include "ServerCursor.h"
#include "ServerConfig.h"
#include "SystemPalette.h"
#include "ColorUtils.h"
#include "PortLink.h"
#include "FontFamily.h"
#include "RGBColor.h"
#include "LayerData.h"
#include <View.h>
#include <stdio.h>
#include <string.h>
#include <String.h>
#include <math.h>

extern RGBColor workspace_default_color;	// defined in AppServer.cpp

/*!
	\brief Sets up internal variables needed by all DisplayDriver subclasses
	
	Subclasses should follow DisplayDriver's lead and use this function mostly
	for initializing data members.
*/
BitmapDriver::BitmapDriver(void) : DisplayDriver()
{
	fTarget=NULL;
	fGraphicsBuffer=NULL;
	fPixelRenderer=NULL;
}

/*!
	\brief Deletes the locking semaphore
	
	Subclasses should use the destructor mostly for freeing allocated heap space.
*/
BitmapDriver::~BitmapDriver(void)
{
}

/*!
	\brief Initializes the driver object.
	\return true if successful, false if not
	
	Initialize sets up the driver for display, including the initial clearing
	of the screen. If things do not go as they should, false should be returned.
*/
bool BitmapDriver::Initialize(void)
{
	// Nothing is needed because of SetTarget taking care of the work for us.
	return true;
}

/*!
	\brief Shuts down the driver's video subsystem
	
	Any work done by Initialize() should be undone here. Note that Shutdown() is
	called even if Initialize() was unsuccessful.
*/
void BitmapDriver::Shutdown(void)
{
	// Nothing is needed here
}

void BitmapDriver::SetTarget(ServerBitmap *target)
{
	Lock();
	
	if(fGraphicsBuffer)
	{
		delete fGraphicsBuffer;
		fGraphicsBuffer=NULL;
	}
	
	if(fPixelRenderer)
	{
		delete fPixelRenderer;
		fPixelRenderer=NULL;
	}
	
	fTarget=target;
	
	if(target)
	{
		_displaymode.virtual_width=target->Width();
		_displaymode.virtual_height=target->Height();
		_displaymode.space=target->ColorSpace();
		
		fGraphicsBuffer=new GraphicsBuffer((uint8*)fTarget->Bits(),fTarget->Bounds().Width(),
				fTarget->Bounds().Height(),fTarget->BytesPerRow());
		
		switch(fTarget->ColorSpace())
		{
			case B_RGB32:
			case B_RGBA32:
			{
				fPixelRenderer=new PixelRendererRGBA32(*fGraphicsBuffer);
				break;
			}
			case B_RGB16:
			{
				fPixelRenderer=new PixelRendererRGB16(*fGraphicsBuffer);
				break;
			}
			case B_RGB15:
			case B_RGBA15:
			{
				fPixelRenderer=new PixelRendererRGBA15(*fGraphicsBuffer);
				break;
			}
			case B_CMAP8:
			case B_GRAY8:
			{
				fPixelRenderer=new PixelRendererCMAP8(*fGraphicsBuffer);
				break;
			}
			default:
				break;
		}
		// Setting mode not necessary. Can get color space stuff via ServerBitmap->ColorSpace
	}
	
	Unlock();
}

//! Empty
void BitmapDriver::SetMode(const int32 &space)
{
	// No need to reset a bitmap's color space
}

//! Empty
void BitmapDriver::SetMode(const display_mode &mode)
{
	// No need to reset a bitmap's color space
}

void BitmapDriver::InvertRect(const BRect &r)
{
	Lock();
	if(fTarget)
	{
		if(r.top<0 || r.left<0 || 
			r.right>fTarget->Width()-1 || r.bottom>fTarget->Height()-1)
		{
			Unlock();
			return;
		}
		
		switch(fTarget->BitsPerPixel())
		{
			case 32:
			case 24:
			{
				uint16 width=r.IntegerWidth(), height=r.IntegerHeight();
				uint32 *start=(uint32*)fTarget->Bits(), *index;
				start+=int32(r.top)*fTarget->Width();
				start+=int32(r.left);
				
				for(int32 i=0;i<height;i++)
				{
					index=start + (i*fTarget->Width());
					for(int32 j=0; j<width; j++)
						index[j]^=0xFFFFFF00L;
				}
				break;
			}
			case 16:
				// TODO: Implement
				printf("BitmapDriver::16-bit mode unimplemented\n");
				break;
			case 15:
				// TODO: Implement
				printf("BitmapDriver::15-bit mode unimplemented\n");
				break;
			case 8:
				// TODO: Implement
				printf("BitmapDriver::8-bit mode unimplemented\n");
				break;
			default:
				break;
		}

	}
	Unlock();
}

rgb_color BitmapDriver::GetBlitColor(rgb_color src, rgb_color dest, DrawData *d, bool use_high)
{
	rgb_color returncolor={0,0,0,0};
	int16 value;
	if(!d)
		return returncolor;
	
	switch(d->draw_mode)
	{
		case B_OP_COPY:
		{
			return src;
		}
		case B_OP_ADD:
		{
			value=src.red+dest.red;
			returncolor.red=(value>255)?255:value;

			value=src.green+dest.green;
			returncolor.green=(value>255)?255:value;

			value=src.blue+dest.blue;
			returncolor.blue=(value>255)?255:value;
			return returncolor;
		}
		case B_OP_SUBTRACT:
		{
			value=src.red-dest.red;
			returncolor.red=(value<0)?0:value;

			value=src.green-dest.green;
			returncolor.green=(value<0)?0:value;

			value=src.blue-dest.blue;
			returncolor.blue=(value<0)?0:value;
			return returncolor;
		}
		case B_OP_BLEND:
		{
			value=int16(src.red+dest.red)>>1;
			returncolor.red=value;

			value=int16(src.green+dest.green)>>1;
			returncolor.green=value;

			value=int16(src.blue+dest.blue)>>1;
			returncolor.blue=value;
			return returncolor;
		}
		case B_OP_MIN:
		{
			
			return ( uint16(src.red+src.blue+src.green) > 
				uint16(dest.red+dest.blue+dest.green) )?dest:src;
		}
		case B_OP_MAX:
		{
			return ( uint16(src.red+src.blue+src.green) < 
				uint16(dest.red+dest.blue+dest.green) )?dest:src;
		}
		case B_OP_OVER:
		{
			return (use_high && src.alpha>127)?src:dest;
		}
		case B_OP_INVERT:
		{
			returncolor.red=dest.red ^ 255;
			returncolor.green=dest.green ^ 255;
			returncolor.blue=dest.blue ^ 255;
			return (use_high && src.alpha>127)?returncolor:dest;
		}
		// This is a pain in the arse to implement, so I'm saving it for the real
		// server
		case B_OP_ALPHA:
		{
			return src;
		}
		case B_OP_ERASE:
		{
			// This one's tricky. 
			return (use_high && src.alpha>127)?d->lowcolor.GetColor32():dest;
		}
		case B_OP_SELECT:
		{
			// This one's tricky, too. We are passed a color in src. If it's the layer's
			// high color or low color, we check for a swap.
			if(d->highcolor==src)
				return (use_high && d->highcolor==dest)?d->lowcolor.GetColor32():dest;
			
			if(d->lowcolor==src)
				return (use_high && d->lowcolor==dest)?d->highcolor.GetColor32():dest;

			return dest;
		}
		default:
		{
			break;
		}
	}
	Unlock();
	return returncolor;
}

/*!
	\brief Draws a point using the display driver's specified thickness and pattern
	\param x The x coordinate (not guaranteed to be in bounds)
	\param y The y coordinate (not guaranteed to be in bounds)
*/
void BitmapDriver::SetThickPatternPixel(int x, int y)
{
	int left, right, top, bottom;
	int bytes_per_row = fTarget->BytesPerRow();
	left = x - fLineThickness/2;
	right = x + fLineThickness/2;
	top = y - fLineThickness/2;
	bottom = y + fLineThickness/2;
	switch(fTarget->BitsPerPixel())
	{
		case 8:
			{
				int x,y;
				uint8 *fb = (uint8 *)fTarget->Bits() + top*bytes_per_row;
				for (y=top; y<=bottom; y++)
				{
					for (x=left; x<=right; x++)
						fb[x] = fDrawPattern.GetColor(x,y).GetColor8();
					fb += bytes_per_row;
				}
			} break;
		case 15:
			{
				int x,y;
				uint16 *fb = (uint16 *)((uint8 *)fTarget->Bits() + top*bytes_per_row);
				for (y=top; y<=bottom; y++)
				{
					for (x=left; x<=right; x++)
						fb[x] = fDrawPattern.GetColor(x,y).GetColor15();
					fb = (uint16 *)((uint8 *)fb + bytes_per_row);
				}
			} break;
		case 16:
			{
				int x,y;
				uint16 *fb = (uint16 *)((uint8 *)fTarget->Bits() + top*bytes_per_row);
				for (y=top; y<=bottom; y++)
				{
					for (x=left; x<=right; x++)
						fb[x] = fDrawPattern.GetColor(x,y).GetColor16();
					fb = (uint16 *)((uint8 *)fb + bytes_per_row);
				}
			} break;
		case 24:
		case 32:
			{
				int x,y;
				uint32 *fb = (uint32 *)((uint8 *)fTarget->Bits() + top*bytes_per_row);
				rgb_color color;
				for (y=top; y<=bottom; y++)
				{
					for (x=left; x<=right; x++)
					{
						color = fDrawPattern.GetColor(x,y).GetColor32();
						fb[x] = (color.alpha << 24) | (color.red << 16) | (color.green << 8) | (color.blue);
					}
					fb = (uint32 *)((uint8 *)fb + bytes_per_row);
				}
			} break;
		default:
			printf("Error: Unknown color space\n");
	}
}

/*!
	\brief Draws a horizontal line using the display driver's line thickness and pattern
	\param x1 The first x coordinate (not guaranteed to be in bounds)
	\param x2 The second x coordinate (not guaranteed to be in bounds)
	\param y The y coordinate (not guaranteed to be in bounds)
*/
void BitmapDriver::HLinePatternThick(int32 x1, int32 x2, int32 y)
{
	int x, y1, y2;
	int bytes_per_row = fTarget->BytesPerRow();

	if ( x1 > x2 )
	{
		x = x2;
		x2 = x1;
		x1 = x;
	}
	y1 = y - fLineThickness/2;
	y2 = y + fLineThickness/2;
	switch(fTarget->BitsPerPixel())
	{
		case 8:
			{
				uint8 *fb = (uint8 *)fTarget->Bits() + y1*bytes_per_row;
				for (y=y1; y<=y2; y++)
				{
					for (x=x1; x<=x2; x++)
						fb[x] = fDrawPattern.GetColor(x,y).GetColor8();
					fb += bytes_per_row;
				}
			} break;
		case 15:
			{
				uint16 *fb = (uint16 *)((uint8 *)fTarget->Bits() + y1*bytes_per_row);
				for (y=y1; y<=y2; y++)
				{
					for (x=x1; x<=x2; x++)
						fb[x] = fDrawPattern.GetColor(x,y).GetColor15();
					fb = (uint16 *)((uint8 *)fb + bytes_per_row);
				}
			} break;
		case 16:
			{
				uint16 *fb = (uint16 *)((uint8 *)fTarget->Bits() + y1*bytes_per_row);
				for (y=y1; y<=y2; y++)
				{
					for (x=x1; x<=x2; x++)
						fb[x] = fDrawPattern.GetColor(x,y).GetColor16();
					fb = (uint16 *)((uint8 *)fb + bytes_per_row);
				}
			} break;
		case 24:
		case 32:
			{
				uint32 *fb = (uint32 *)((uint8 *)fTarget->Bits() + y1*bytes_per_row);
				rgb_color color;
				for (y=y1; y<=y2; y++)
				{
					for (x=x1; x<=x2; x++)
					{
						color = fDrawPattern.GetColor(x,y).GetColor32();
						fb[x] = (color.alpha << 24) | (color.red << 16) | (color.green << 8) | (color.blue);
					}
					fb = (uint32 *)((uint8 *)fb + bytes_per_row);
				}
			} break;
		default:
			printf("Error: Unknown color space\n");
	}
}

/*!
	\brief Draws a vertical line using the display driver's line thickness and pattern
	\param x The x coordinate
	\param y1 The first y coordinate
	\param y2 The second y coordinate
*/
void BitmapDriver::VLinePatternThick(int32 x, int32 y1, int32 y2)
{
	int y, x1, x2;
	int bytes_per_row = fTarget->BytesPerRow();

	if ( y1 > y2 )
	{
		y = y2;
		y2 = y1;
		y1 = y;
	}
	x1 = x - fLineThickness/2;
	x2 = x + fLineThickness/2;
	switch(fTarget->BitsPerPixel())
	{
		case 8:
			{
				uint8 *fb = (uint8 *)fTarget->Bits() + y1*bytes_per_row;
				for (y=y1; y<=y2; y++)
				{
					for (x=x1; x<=x2; x++)
						fb[x] = fDrawPattern.GetColor(x,y).GetColor8();
					fb += bytes_per_row;
				}
			} break;
		case 15:
			{
				uint16 *fb = (uint16 *)((uint8 *)fTarget->Bits() + y1*bytes_per_row);
				for (y=y1; y<=y2; y++)
				{
					for (x=x1; x<=x2; x++)
						fb[x] = fDrawPattern.GetColor(x,y).GetColor15();
					fb = (uint16 *)((uint8 *)fb + bytes_per_row);
				}
			} break;
		case 16:
			{
				uint16 *fb = (uint16 *)((uint8 *)fTarget->Bits() + y1*bytes_per_row);
				for (y=y1; y<=y2; y++)
				{
					for (x=x1; x<=x2; x++)
						fb[x] = fDrawPattern.GetColor(x,y).GetColor16();
					fb = (uint16 *)((uint8 *)fb + bytes_per_row);
				}
			} break;
		case 24:
		case 32:
			{
				uint32 *fb = (uint32 *)((uint8 *)fTarget->Bits() + y1*bytes_per_row);
				rgb_color color;
				for (y=y1; y<=y2; y++)
				{
					for (x=x1; x<=x2; x++)
					{
						color = fDrawPattern.GetColor(x,y).GetColor32();
						fb[x] = (color.alpha << 24) | (color.red << 16) | (color.green << 8) | (color.blue);
					}
					fb = (uint32 *)((uint8 *)fb + bytes_per_row);
				}
			} break;
		default:
			printf("Error: Unknown color space\n");
	}
}


void BitmapDriver::DrawBitmap(ServerBitmap *sourcebmp, const BRect &source, 
	const BRect &dest, DrawData *d)
{
	// Another internal function called from other functions.
	
	if(!sourcebmp | !d)
		return;

	if(sourcebmp->BitsPerPixel() != fTarget->BitsPerPixel())
		return;

	uint8 colorspace_size=sourcebmp->BitsPerPixel()/8;
	
	BRect sourcerect(source),destrect(dest);
	
	// First, clip source rect to destination
	if(sourcerect.Width() > destrect.Width())
		sourcerect.right=sourcerect.left+destrect.Width();
	

	if(sourcerect.Height() > destrect.Height())
		sourcerect.bottom=sourcerect.top+destrect.Height();
	

	// Second, check rectangle bounds against their own bitmaps
	BRect work_rect;

	work_rect=sourcebmp->Bounds();
	
	if( !(work_rect.Contains(sourcerect)) )
	{	// something in selection must be clipped
		if(sourcerect.left < 0)
			sourcerect.left = 0;
		if(sourcerect.right > work_rect.right)
			sourcerect.right = work_rect.right;
		if(sourcerect.top < 0)
			sourcerect.top = 0;
		if(sourcerect.bottom > work_rect.bottom)
			sourcerect.bottom = work_rect.bottom;
	}

	work_rect.Set(0,0,fTarget->Width()-1,fTarget->Height()-1);

	// Check to see if we actually need to copy anything
	if( (destrect.right<work_rect.left) || (destrect.left>work_rect.right) ||
			(destrect.bottom<work_rect.top) || (destrect.top>work_rect.bottom) )
		return;

	// something in selection must be clipped
	if(destrect.left < 0)
		destrect.left = 0;
	if(destrect.right > work_rect.right)
		destrect.right = work_rect.right;
	if(destrect.top < 0)
		destrect.top = 0;
	if(destrect.bottom > work_rect.bottom)
		destrect.bottom = work_rect.bottom;

	// Set pointers to the actual data
	uint8 *src_bits  = (uint8*) sourcebmp->Bits();	
	uint8 *dest_bits = (uint8*) fTarget->Bits();

	// Get row widths for offset looping
	uint32 src_width  = uint32 (sourcebmp->BytesPerRow());
	uint32 dest_width = uint32 (fTarget->BytesPerRow());

	// Offset bitmap pointers to proper spot in each bitmap
	src_bits += uint32 ( (sourcerect.top  * src_width)  + (sourcerect.left  * colorspace_size) );
	dest_bits += uint32 ( (destrect.top * dest_width) + (destrect.left * colorspace_size) );

	uint32 line_length = uint32 ((destrect.right - destrect.left+1)*colorspace_size);
	uint32 lines = uint32 (destrect.bottom-destrect.top+1);

	switch(d->draw_mode)
	{
		case B_OP_OVER:
		{
//			uint32 srow_pixels=src_width>>2;
			uint32 srow_pixels=((destrect.IntegerWidth()>=sourcerect.IntegerWidth())?src_width:destrect.IntegerWidth()+1)>>2;
			uint8 *srow_index, *drow_index;
			
			
			// This could later be optimized to use uint32's for faster copying
			for (uint32 pos_y=0; pos_y!=lines; pos_y++)
			{
				
				srow_index=src_bits;
				drow_index=dest_bits;
				
				for(uint32 pos_x=0; pos_x!=srow_pixels;pos_x++)
				{
					// 32-bit RGBA32 mode byte order is BGRA
					if(srow_index[3]>127)
					{
						*drow_index=*srow_index; drow_index++; srow_index++;
						*drow_index=*srow_index; drow_index++; srow_index++;
						*drow_index=*srow_index; drow_index++; srow_index++;
						// we don't copy the alpha channel
						drow_index++; srow_index++;
					}
					else
					{
						srow_index+=4;
						drow_index+=4;
					}
				}
		
				// Increment offsets
		   		src_bits += src_width;
		   		dest_bits += dest_width;
			}
			break;
		}
		default:	// B_OP_COPY
		{
			for (uint32 pos_y = 0; pos_y != lines; pos_y++)
			{
				memcpy(dest_bits,src_bits,line_length);
		
				// Increment offsets
				src_bits += src_width;
				dest_bits += dest_width;
			}
			break;
		}
	}
}


bool BitmapDriver::AcquireBuffer(FBBitmap *fbmp)
{
	if(!fbmp)
		return false;
	
	fbmp->ServerBitmap::ShallowCopy(fTarget);

	return true;
}

void BitmapDriver::ReleaseBuffer(void)
{
}

void BitmapDriver::Blit(const BRect &src, const BRect &dest, const DrawData *d)
{
}

void BitmapDriver::FillSolidRect(const BRect &rect, const RGBColor &color)
{
	int bytes_per_row = fTarget->BytesPerRow();
	int top = (int)rect.top;
	int left = (int)rect.left;
	int right = (int)rect.right;
	int bottom = (int)rect.bottom;
	RGBColor col(color);	// to avoid GetColor8/15/16() const issues

	switch(fTarget->BitsPerPixel())
	{
		case 8:
			{
				uint8 *fb = (uint8 *)fTarget->Bits() + top*bytes_per_row;
				uint8 color8 = col.GetColor8();
				int x,y;
				for (y=top; y<=bottom; y++)
				{
					for (x=left; x<=right; x++)
						fb[x] = color8;
					fb += bytes_per_row;
				}
			} break;
		case 15:
			{
				uint16 *fb = (uint16 *)((uint8 *)fTarget->Bits() + top*bytes_per_row);
				uint16 color15 = col.GetColor15();
				int x,y;
				for (y=top; y<=bottom; y++)
				{
					for (x=left; x<=right; x++)
						fb[x] = color15;
					fb = (uint16 *)((uint8 *)fb + bytes_per_row);
				}
			} break;
		case 16:
			{
				uint16 *fb = (uint16 *)((uint8 *)fTarget->Bits() + top*bytes_per_row);
				uint16 color16 = col.GetColor16();
				int x,y;
				for (y=top; y<=bottom; y++)
				{
					for (x=left; x<=right; x++)
						fb[x] = color16;
					fb = (uint16 *)((uint8 *)fb + bytes_per_row);
				}
			} break;
		case 24:
		case 32:
			{
				uint32 *fb = (uint32 *)((uint8 *)fTarget->Bits() + top*bytes_per_row);
				rgb_color fill_color = color.GetColor32();
				uint32 color32 = (fill_color.alpha << 24) | (fill_color.red << 16) | (fill_color.green << 8) | (fill_color.blue);
				int x,y;
				for (y=top; y<=bottom; y++)
				{
					for (x=left; x<=right; x++)
						fb[x] = color32;
					fb = (uint32 *)((uint8 *)fb + bytes_per_row);
				}
			} break;
		default:
			printf("Error: Unknown color space\n");
	}
}

void BitmapDriver::FillPatternRect(const BRect &rect, const DrawData *d)
{
	int bytes_per_row = fTarget->BytesPerRow();
	int top = (int)rect.top;
	int left = (int)rect.left;
	int right = (int)rect.right;
	int bottom = (int)rect.bottom;
	
	switch(fTarget->BitsPerPixel())
	{
		case 8:
			{
				uint8 *fb = (uint8 *)fTarget->Bits() + top*bytes_per_row;
				int x,y;
				for (y=top; y<=bottom; y++)
				{
					for (x=left; x<=right; x++)
						fb[x] = fDrawPattern.GetColor(x,y).GetColor8();
					fb += bytes_per_row;
				}
			} break;
		case 15:
			{
				uint16 *fb = (uint16 *)((uint8 *)fTarget->Bits() + top*bytes_per_row);
				int x,y;
				for (y=top; y<=bottom; y++)
				{
					for (x=left; x<=right; x++)
						fb[x] = fDrawPattern.GetColor(x,y).GetColor15();
					fb = (uint16 *)((uint8 *)fb + bytes_per_row);
				}
			} break;
		case 16:
			{
				uint16 *fb = (uint16 *)((uint8 *)fTarget->Bits() + top*bytes_per_row);
				int x,y;
				for (y=top; y<=bottom; y++)
				{
					for (x=left; x<=right; x++)
						fb[x] = fDrawPattern.GetColor(x,y).GetColor16();
					fb = (uint16 *)((uint8 *)fb + bytes_per_row);
				}
			} break;
		case 24:
		case 32:
			{
				uint32 *fb = (uint32 *)((uint8 *)fTarget->Bits() + top*bytes_per_row);
				int x,y;
				rgb_color color;
				for (y=top; y<=bottom; y++)
				{
					for (x=left; x<=right; x++)
					{
						color = fDrawPattern.GetColor(x,y).GetColor32();
						fb[x] = (color.alpha << 24) | (color.red << 16) | (color.green << 8) | (color.blue);
					}
					fb = (uint32 *)((uint8 *)fb + bytes_per_row);
				}
			} break;
		default:
			printf("Error: Unknown color space\n");
	}
}

void BitmapDriver::StrokeSolidLine(int32 x1, int32 y1, int32 x2, int32 y2, const RGBColor &color)
{
}

void BitmapDriver::StrokePatternLine(int32 x1, int32 y1, int32 x2, int32 y2, const DrawData *d)
{
}

void BitmapDriver::StrokeSolidRect(const BRect &rect, const RGBColor &color)
{
}


/*!
	\brief Copy a ServerBitmap to the BitmapDriver's internal ServerBitmap
	\param bitmap The ServerBitmap source of the copy
	\param sourcerect The source rectangle of the copy
	\param dest The destination position of the copy (no scaling occurs)
	\param d The DrawData (currently unused)
*/

// TODO: dest should really become a BPoint to avoid confusion
// TODO: can we unify CopyBitmap and CopyToBitmap somehow, to avoid the
//       near-total code duplication?

void BitmapDriver::CopyBitmap(ServerBitmap *bitmap, const BRect &sourcerect, const BRect &dest, const DrawData *d)
{
	if(!bitmap)
	{
		printf("CopyBitmap returned - not init or NULL bitmap\n");
		return;
	}
	
	if(((uint32)bitmap->ColorSpace() & 0x000F) != (_displaymode.space & 0x000F))
	{
		printf("CopyBitmap returned - unequal buffer pixel depth\n");
		return;
	}

	// dest shows position only, not size (i.e. no scaling), so we
	// emphasize that point by using the source rect and offsetting it
	BRect destrect(sourcerect), source(sourcerect);
	destrect.OffsetTo(dest.left, dest.top);
	
	uint8 colorspace_size=bitmap->BitsPerPixel()/8;
	
	// First, clip source rect to destination
	if(source.Width() > destrect.Width())
		source.right=source.left+destrect.Width();
	
	if(source.Height() > destrect.Height())
		source.bottom=source.top+destrect.Height();
	

	// Second, check rectangle bounds against their own bitmaps
	BRect work_rect(bitmap->Bounds());
	
	// Is there anything at all to copy?
	if(!work_rect.Contains(sourcerect))
		return;

	if( !(work_rect.Contains(source)) )
	{
		// something in selection must be clipped
		if(source.left < 0)
			source.left = 0;
		if(source.right > work_rect.right)
			source.right = work_rect.right;
		if(source.top < 0)
			source.top = 0;
		if(source.bottom > work_rect.bottom)
			source.bottom = work_rect.bottom;
	}
	
	work_rect.Set(0,0,_displaymode.virtual_width-1,_displaymode.virtual_height-1);
	
	if( !(work_rect.Contains(destrect)) )
	{
		// something in selection must be clipped
		if(destrect.left < 0)
			destrect.left = 0;
		if(destrect.right > work_rect.right)
			destrect.right = work_rect.right;
		if(destrect.top < 0)
			destrect.top = 0;
		if(destrect.bottom > work_rect.bottom)
			destrect.bottom = work_rect.bottom;
	}

	// Set pointers to the actual data
	uint8 *dest_bits  = (uint8*) fTarget->Bits();	
	uint8 *src_bits = (uint8*) bitmap->Bits();

	// Get row widths for offset looping
	uint32 dest_width  = uint32 (fTarget->BytesPerRow());
	uint32 src_width = uint32 (bitmap->BytesPerRow());

	// Offset bitmap pointers to proper spot in each bitmap
	src_bits += uint32 ( (source.top  * src_width)  + (source.left  * colorspace_size) );
	dest_bits += uint32 ( (destrect.top * dest_width) + (destrect.left * colorspace_size) );
	
	
	uint32 line_length = uint32 ((destrect.right - destrect.left+1)*colorspace_size);
	uint32 lines = uint32 (destrect.bottom-destrect.top+1);

	for (uint32 pos_y=0; pos_y<lines; pos_y++)
	{
		memcpy(dest_bits,src_bits,line_length);

		// Increment offsets
 		src_bits += src_width;
 		dest_bits += dest_width;
	}
}


/*!
	\brief Copy from the BitmapDriver's internal ServerBitmap to a ServerBitmap
	\param destbmp The ServerBitmap destination of the copy
	\param sourcerect The source rectangle of the copy
*/
void BitmapDriver::CopyToBitmap(ServerBitmap *destbmp, const BRect &sourcerect)
{
	if(!destbmp)
	{
		printf("CopyToBitmap returned - not init or NULL bitmap\n");
		return;
	}
	
	if(((uint32)destbmp->ColorSpace() & 0x000F) != (_displaymode.space & 0x000F))
	{
		printf("CopyToBitmap returned - unequal buffer pixel depth\n");
		return;
	}
	
	BRect destrect(destbmp->Bounds()), source(sourcerect);
	
	uint8 colorspace_size=destbmp->BitsPerPixel()/8;
	
	// First, clip source rect to destination
	if(source.Width() > destrect.Width())
		source.right=source.left+destrect.Width();
	
	if(source.Height() > destrect.Height())
		source.bottom=source.top+destrect.Height();
	

	// Second, check rectangle bounds against their own bitmaps
	BRect work_rect(destbmp->Bounds());
	
	if( !(work_rect.Contains(destrect)) )
	{
		// something in selection must be clipped
		if(destrect.left < 0)
			destrect.left = 0;
		if(destrect.right > work_rect.right)
			destrect.right = work_rect.right;
		if(destrect.top < 0)
			destrect.top = 0;
		if(destrect.bottom > work_rect.bottom)
			destrect.bottom = work_rect.bottom;
	}

	work_rect.Set(0,0,_displaymode.virtual_width-1,_displaymode.virtual_height-1);

	// Is there anything at all to copy?
	if(!work_rect.Contains(sourcerect))
		return;

	if( !(work_rect.Contains(source)) )
	{
		// something in selection must be clipped
		if(source.left < 0)
			source.left = 0;
		if(source.right > work_rect.right)
			source.right = work_rect.right;
		if(source.top < 0)
			source.top = 0;
		if(source.bottom > work_rect.bottom)
			source.bottom = work_rect.bottom;
	}

	// Set pointers to the actual data
	uint8 *dest_bits  = (uint8*) destbmp->Bits();	
	uint8 *src_bits = (uint8*) fTarget->Bits();

	// Get row widths for offset looping
	uint32 dest_width  = uint32 (destbmp->BytesPerRow());
	uint32 src_width = uint32 (fTarget->BytesPerRow());

	// Offset bitmap pointers to proper spot in each bitmap
	src_bits += uint32 ( (source.top  * src_width)  + (source.left  * colorspace_size) );
	dest_bits += uint32 ( (destrect.top * dest_width) + (destrect.left * colorspace_size) );
	
	
	uint32 line_length = uint32 ((destrect.right - destrect.left+1)*colorspace_size);
	uint32 lines = uint32 (source.bottom-source.top+1);

	for (uint32 pos_y=0; pos_y<lines; pos_y++)
	{
		memcpy(dest_bits,src_bits,line_length);

		// Increment offsets
 		src_bits += src_width;
 		dest_bits += dest_width;
	}

}

