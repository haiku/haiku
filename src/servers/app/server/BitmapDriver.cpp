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
	_SetMode(B_8_BIT_640x480);
	_SetWidth(640);
	_SetHeight(480);
	_SetDepth(8);
	_SetBytesPerRow(640);
	_target=NULL;
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
	_target=target;
	
	if(target)
	{
		_SetWidth(target->Width());
		_SetHeight(target->Height());
		_SetDepth(target->BitsPerPixel());
		_SetBytesPerRow(target->BytesPerRow());
		// Setting mode not necessary. Can get color space stuff via ServerBitmap->ColorSpace
	}
	
	Unlock();
}

/*!
	\brief Called for all BView::CopyBits calls
	\param src Source rectangle.
	\param dest Destination rectangle.
	
	Bounds checking must be done in this call. If the destination is not the same size 
	as the source, the source should be scaled to fit.
*/
void BitmapDriver::CopyBits(BRect src, BRect dest)
{
printf("BitmapDriver::CopyBits unimplemented\n");
}

/*!
	\brief Called for all BView::DrawBitmap calls
	\param bmp Bitmap to be drawn. It will always be non-NULL and valid. The color 
	space is not guaranteed to match.
	\param src Source rectangle
	\param dest Destination rectangle. Source will be scaled to fit if not the same size.
	\param d Data structure containing any other data necessary for the call. Always non-NULL.
	
	Bounds checking must be done in this call.
*/
void BitmapDriver::DrawBitmap(ServerBitmap *bitmap, BRect source, BRect dest, LayerData *d)
{
	Lock();

//TODO: Implement
	Unlock();
}


void BitmapDriver::FillArc(const BRect r, float angle, float span, RGBColor& color)
{
	Lock();
	fLineThickness = 1;
	fDrawPattern.SetTarget((int8*)&B_SOLID_HIGH);
	fDrawPattern.SetColors(color,color);
	DisplayDriver::FillArc(r,angle,span,this,(SetHorizontalLineFuncType)&BitmapDriver::HLinePatternThick);
	Unlock();
}

void BitmapDriver::FillArc(const BRect r, float angle, float span, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color)
{
	Lock();
	fLineThickness = 1;
	fDrawPattern.SetTarget(pattern);
	fDrawPattern.SetColors(high_color,low_color);
	DisplayDriver::FillArc(r,angle,span,this,(SetHorizontalLineFuncType)&BitmapDriver::HLinePatternThick);
	Unlock();
}

void BitmapDriver::FillBezier(BPoint *pts, RGBColor& color)
{
	Lock();
	fLineThickness = 1;
	fDrawPattern.SetTarget((int8*)&B_SOLID_HIGH);
	fDrawPattern.SetColors(color,color);
	DisplayDriver::FillBezier(pts,this,(SetHorizontalLineFuncType)&BitmapDriver::HLinePatternThick);
	Unlock();
}

void BitmapDriver::FillBezier(BPoint *pts, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color)
{
	Lock();
	fLineThickness = 1;
	fDrawPattern.SetTarget(pattern);
	fDrawPattern.SetColors(high_color,low_color);
	DisplayDriver::FillBezier(pts,this,(SetHorizontalLineFuncType)&BitmapDriver::HLinePatternThick);
	Unlock();
}

void BitmapDriver::FillEllipse(BRect r, RGBColor& color)
{
	Lock();
	fLineThickness = 1;
	fDrawPattern.SetTarget((int8*)&B_SOLID_HIGH);
	fDrawPattern.SetColors(color,color);
	DisplayDriver::FillEllipse(r,this,(SetHorizontalLineFuncType)&BitmapDriver::HLinePatternThick);
	Unlock();
}

void BitmapDriver::FillEllipse(BRect r, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color)
{
	Lock();
	fLineThickness = 1;
	fDrawPattern.SetTarget(pattern);
	fDrawPattern.SetColors(high_color,low_color);
	DisplayDriver::FillEllipse(r,this,(SetHorizontalLineFuncType)&BitmapDriver::HLinePatternThick);
	Unlock();
}

void BitmapDriver::FillPolygon(BPoint *ptlist, int32 numpts, RGBColor& color)
{
	Lock();
	fLineThickness = 1;
	fDrawPattern.SetTarget((int8*)&B_SOLID_HIGH);
	fDrawPattern.SetColors(color,color);
	DisplayDriver::FillPolygon(ptlist,numpts,this,(SetHorizontalLineFuncType)&BitmapDriver::HLinePatternThick);
	Unlock();
}

void BitmapDriver::FillPolygon(BPoint *ptlist, int32 numpts, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color)
{
	Lock();
	fLineThickness = 1;
	fDrawPattern.SetTarget(pattern);
	fDrawPattern.SetColors(high_color,low_color);
	DisplayDriver::FillPolygon(ptlist,numpts,this,(SetHorizontalLineFuncType)&BitmapDriver::HLinePatternThick);
	Unlock();
}

/*!
	\brief Called for all BView::FillRect calls
	\param r BRect to be filled. Guaranteed to be in the frame buffer's coordinate space
	\param color The color used when filling the rectangle
*/
void BitmapDriver::FillRect(const BRect r, RGBColor& color)
{
	Lock();
	fDrawColor = color;
	FillSolidRect((int32)r.left,(int32)r.top,(int32)r.right,(int32)r.bottom);
	Unlock();
}

/*!
	\brief Called for all BView::FillRect calls
	\param r BRect to be filled. Guaranteed to be in the frame buffer's coordinate space
	\param pattern The pattern to be used when filling the rectangle
	\param high_color The high color of the pattern to fill
	\param low_color  The low color of the pattern to fill
*/
void BitmapDriver::FillRect(const BRect r, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color)
{
	Lock();
	fDrawPattern.SetTarget(pattern);
	fDrawPattern.SetColors(high_color,low_color);
	FillPatternRect((int32)r.left,(int32)r.top,(int32)r.right,(int32)r.bottom);
	Unlock();
}

void BitmapDriver::FillRoundRect(BRect r, float xrad, float yrad, RGBColor& color)
{
	Lock();
	fLineThickness = 1;
	fDrawPattern.SetTarget((int8*)&B_SOLID_HIGH);
	fDrawPattern.SetColors(color,color);
	fDrawColor = color;
	DisplayDriver::FillRoundRect(r,xrad,yrad,this,(SetRectangleFuncType)&BitmapDriver::FillSolidRect,(SetHorizontalLineFuncType)&BitmapDriver::HLinePatternThick);
	Unlock();
}

void BitmapDriver::FillRoundRect(BRect r, float xrad, float yrad, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color)
{
	Lock();
	fLineThickness = 1;
	fDrawPattern.SetTarget((int8*)&B_SOLID_HIGH);
	fDrawPattern.SetColors(high_color,low_color);
	DisplayDriver::FillRoundRect(r,xrad,yrad,this,(SetRectangleFuncType)&BitmapDriver::FillPatternRect,(SetHorizontalLineFuncType)&BitmapDriver::HLinePatternThick);
	Unlock();
}

void BitmapDriver::FillTriangle(BPoint *pts, RGBColor& color)
{
	Lock();
	fLineThickness = 1;
	fDrawPattern.SetTarget((int8*)&B_SOLID_HIGH);
	fDrawPattern.SetColors(color,color);
	DisplayDriver::FillTriangle(pts,this,(SetHorizontalLineFuncType)&BitmapDriver::HLinePatternThick);
	Unlock();
}

void BitmapDriver::FillTriangle(BPoint *pts, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color)
{
	Lock();
	fLineThickness = 1;
	fDrawPattern.SetTarget(pattern);
	fDrawPattern.SetColors(high_color,low_color);
	DisplayDriver::FillTriangle(pts,this,(SetHorizontalLineFuncType)&BitmapDriver::HLinePatternThick);
	Unlock();
}

void BitmapDriver::StrokeArc(BRect r, float angle, float span, float pensize, RGBColor& color)
{
	Lock();
	fLineThickness = (int)pensize;
	fDrawPattern.SetTarget((int8*)&B_SOLID_HIGH);
	fDrawPattern.SetColors(color,color);
	DisplayDriver::StrokeArc(r,angle,span,this,(SetPixelFuncType)&BitmapDriver::SetThickPatternPixel);
	Unlock();
}

void BitmapDriver::StrokeArc(BRect r, float angle, float span, float pensize, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color)
{
	Lock();
	fLineThickness = (int)pensize;
	fDrawPattern.SetTarget(pattern);
	fDrawPattern.SetColors(high_color,low_color);
	DisplayDriver::StrokeArc(r,angle,span,this,(SetPixelFuncType)&BitmapDriver::SetThickPatternPixel);
	Unlock();
}

void BitmapDriver::StrokeBezier(BPoint *pts, float pensize, RGBColor& color)
{
	Lock();
	fLineThickness = (int)pensize;
	fDrawPattern.SetTarget((int8*)&B_SOLID_HIGH);
	fDrawPattern.SetColors(color,color);
	DisplayDriver::StrokeBezier(pts,this,(SetPixelFuncType)&BitmapDriver::SetThickPatternPixel);
	Unlock();
}

void BitmapDriver::StrokeBezier(BPoint *pts, float pensize, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color)
{
	Lock();
	fLineThickness = (int)pensize;
	fDrawPattern.SetTarget(pattern);
	fDrawPattern.SetColors(high_color,low_color);
	DisplayDriver::StrokeBezier(pts,this,(SetPixelFuncType)&BitmapDriver::SetThickPatternPixel);
	Unlock();
}

void BitmapDriver::StrokeEllipse(BRect r, float pensize, RGBColor& color)
{
	Lock();
	fLineThickness = (int)pensize;
	fDrawPattern.SetTarget((int8*)&B_SOLID_HIGH);
	fDrawPattern.SetColors(color,color);
	DisplayDriver::StrokeEllipse(r,this,(SetPixelFuncType)&BitmapDriver::SetThickPatternPixel);
	Unlock();
}

void BitmapDriver::StrokeEllipse(BRect r, float pensize, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color)
{
	Lock();
	fLineThickness = (int)pensize;
	fDrawPattern.SetTarget(pattern);
	fDrawPattern.SetColors(high_color,low_color);
	DisplayDriver::StrokeEllipse(r,this,(SetPixelFuncType)&BitmapDriver::SetThickPatternPixel);
	Unlock();
}

void BitmapDriver::StrokeLine(BPoint start, BPoint end, float pensize, RGBColor& color)
{
	Lock();
	fLineThickness = (int)pensize;
	fDrawPattern.SetTarget((int8*)&B_SOLID_HIGH);
	fDrawPattern.SetColors(color,color);
	DisplayDriver::StrokeLine(start,end,this,(SetPixelFuncType)&BitmapDriver::SetThickPatternPixel);
	Unlock();
}

void BitmapDriver::StrokeLine(BPoint start, BPoint end, float pensize, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color)
{
	Lock();
	fLineThickness = (int)pensize;
	fDrawPattern.SetTarget(pattern);
	fDrawPattern.SetColors(high_color,low_color);
	DisplayDriver::StrokeLine(start,end,this,(SetPixelFuncType)&BitmapDriver::SetThickPatternPixel);
	Unlock();
}

void BitmapDriver::StrokePoint(BPoint& pt, RGBColor& color)
{
	Lock();
	fLineThickness = 1;
	fDrawPattern.SetTarget((int8*)&B_SOLID_HIGH);
	fDrawPattern.SetColors(color,color);
	SetThickPatternPixel((int)pt.x,(int)pt.y);
	Unlock();
}

void BitmapDriver::StrokePolygon(BPoint *ptlist, int32 numpts, float pensize, RGBColor& color, bool is_closed)
{
	Lock();
	fLineThickness = (int)pensize;
	fDrawPattern.SetTarget((int8*)&B_SOLID_HIGH);
	fDrawPattern.SetColors(color,color);
	DisplayDriver::StrokePolygon(ptlist,numpts,this,(SetPixelFuncType)&BitmapDriver::SetThickPatternPixel,is_closed);
	Unlock();
}

void BitmapDriver::StrokePolygon(BPoint *ptlist, int32 numpts, float pensize, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color, bool is_closed)
{
	Lock();
	fLineThickness = (int)pensize;
	fDrawPattern.SetTarget(pattern);
	fDrawPattern.SetColors(high_color,low_color);
	DisplayDriver::StrokePolygon(ptlist,numpts,this,(SetPixelFuncType)&BitmapDriver::SetThickPatternPixel);
	Unlock();
}

void BitmapDriver::StrokeRect(BRect r, float pensize, RGBColor& color)
{
	Lock();
	fLineThickness = (int)pensize;
	fDrawPattern.SetTarget((int8*)&B_SOLID_HIGH);
	fDrawPattern.SetColors(color,color);
	DisplayDriver::StrokeRect(r,this,(SetHorizontalLineFuncType)&BitmapDriver::HLinePatternThick,(SetVerticalLineFuncType)&BitmapDriver::VLinePatternThick);
	Unlock();
}

void BitmapDriver::StrokeRect(BRect r, float pensize, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color)
{
	Lock();
	fLineThickness = (int)pensize;
	fDrawPattern.SetTarget(pattern);
	fDrawPattern.SetColors(high_color,low_color);
	DisplayDriver::StrokeRect(r,this,(SetHorizontalLineFuncType)&BitmapDriver::HLinePatternThick,(SetVerticalLineFuncType)&BitmapDriver::VLinePatternThick);
	Unlock();
}

void BitmapDriver::StrokeRoundRect(BRect r, float xrad, float yrad, float pensize, RGBColor& color)
{
	Lock();
	fLineThickness = (int)pensize;
	fDrawPattern.SetTarget((int8*)&B_SOLID_HIGH);
	fDrawPattern.SetColors(color,color);
	DisplayDriver::StrokeRoundRect(r,xrad,yrad,this,(SetHorizontalLineFuncType)&BitmapDriver::HLinePatternThick,(SetVerticalLineFuncType)&BitmapDriver::VLinePatternThick,(SetPixelFuncType)&BitmapDriver::SetThickPatternPixel);
	Unlock();
}

void BitmapDriver::StrokeRoundRect(BRect r, float xrad, float yrad, float pensize, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color)
{
	Lock();
	fLineThickness = (int)pensize;
	fDrawPattern.SetTarget(pattern);
	fDrawPattern.SetColors(high_color,low_color);
	DisplayDriver::StrokeRoundRect(r,xrad,yrad,this,(SetHorizontalLineFuncType)&BitmapDriver::HLinePatternThick,(SetVerticalLineFuncType)&BitmapDriver::VLinePatternThick,(SetPixelFuncType)&BitmapDriver::SetThickPatternPixel);
	Unlock();
}

/*!
	\brief Draws a series of lines - optimized for speed
	\param pts Array of BPoints pairs
	\param numlines Number of lines to be drawn
	\param pensize The thickness of the lines
	\param colors Array of colors for each respective line
*/
void BitmapDriver::StrokeLineArray(BPoint *pts, int32 numlines, float pensize, RGBColor *colors)
{
	int x1, y1, x2, y2, dx, dy;
	int steps, k;
	double xInc, yInc;
	double x,y;
	int i;

	Lock();
	fLineThickness = (int)pensize;
	fDrawPattern.SetTarget((int8*)&B_SOLID_HIGH);
	for (i=0; i<numlines; i++)
	{
		//fDrawColor = colors[i];
		fDrawPattern.SetColors(colors[i],colors[i]);
		x1 = ROUND(pts[i*2].x);
		y1 = ROUND(pts[i*2].y);
		x2 = ROUND(pts[i*2+1].x);
		y2 = ROUND(pts[i*2+1].y);
		dx = x2-x1;
		dy = y2-y1;
		x = x1;
		y = y1;
		if ( abs(dx) > abs(dy) )
			steps = abs(dx);
		else
			steps = abs(dy);
		xInc = dx / (double) steps;
		yInc = dy / (double) steps;

		SetThickPatternPixel(ROUND(x),ROUND(y));
		for (k=0; k<steps; k++)
		{
			x += xInc;
			y += yInc;
			SetThickPatternPixel(ROUND(x),ROUND(y));
		}
	}
	Unlock();
}


//! Empty
void BitmapDriver::SetMode(int32 space)
{
	// No need to reset a bitmap's color space
}

//! Empty
void BitmapDriver::SetMode(const display_mode &mode)
{
	// No need to reset a bitmap's color space
}

//! Empty
void BitmapDriver::HideCursor(void)
{
	// Nothing is done with cursor for this, so even the inherited versions need not be called.
}

//! Empty
void BitmapDriver::MoveCursorTo(float x, float y)
{
	// Nothing is done with cursor for this, so even the inherited versions need not be called.
}


//! Empty
void BitmapDriver::ShowCursor(void)
{
	// Nothing is done with cursor for this, so even the inherited versions need not be called.
}

//! Empty
void BitmapDriver::ObscureCursor(void)
{
	// Nothing is done with cursor for this, so even the inherited versions need not be called.
}

//! Empty
void BitmapDriver::SetCursor(ServerCursor *csr)
{
	// Nothing is done with cursor for this, so even the inherited versions need not be called.
}

// This function is intended to eventually take care of most of the heavy lifting for
// DrawBitmap in 32-bit mode, with others coming later. Right now, it is *just* used for
// the 
void BitmapDriver::BlitBitmap(ServerBitmap *sourcebmp,BRect sourcerect, BRect destrect, drawing_mode mode)
{
	// Another internal function called from other functions.
	
	if(!sourcebmp)
		return;

	if(sourcebmp->BitsPerPixel() != _target->BitsPerPixel())
		return;

	uint8 colorspace_size=sourcebmp->BitsPerPixel()/8;
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

	work_rect.Set(0,0,_target->Width()-1,_target->Height()-1);

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
	uint8 *dest_bits = (uint8*) _target->Bits();

	// Get row widths for offset looping
	uint32 src_width  = uint32 (sourcebmp->BytesPerRow());
	uint32 dest_width = uint32 (_target->BytesPerRow());

	// Offset bitmap pointers to proper spot in each bitmap
	src_bits += uint32 ( (sourcerect.top  * src_width)  + (sourcerect.left  * colorspace_size) );
	dest_bits += uint32 ( (destrect.top * dest_width) + (destrect.left * colorspace_size) );

	uint32 line_length = uint32 ((destrect.right - destrect.left+1)*colorspace_size);
	uint32 lines = uint32 (destrect.bottom-destrect.top+1);

	switch(mode)
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

void BitmapDriver::ExtractToBitmap(ServerBitmap *destbmp,BRect destrect, BRect sourcerect)
{
	// Another internal function called from other functions. Extracts data from
	// the framebuffer to a target ServerBitmap

	if(!destbmp)
		return;

	if(destbmp->BitsPerPixel() != _target->BitsPerPixel())
		return;

	uint8 colorspace_size=destbmp->BitsPerPixel()/8;
	// First, clip source rect to destination
	if(sourcerect.Width() > destrect.Width())
		sourcerect.right=sourcerect.left+destrect.Width();
	

	if(sourcerect.Height() > destrect.Height())
		sourcerect.bottom=sourcerect.top+destrect.Height();
	

	// Second, check rectangle bounds against their own bitmaps
	BRect work_rect;

	work_rect.Set(	destbmp->Bounds().left,
					destbmp->Bounds().top,
					destbmp->Bounds().right,
					destbmp->Bounds().bottom	);
	if( !(work_rect.Contains(destrect)) )
	{	// something in selection must be clipped
		if(destrect.left < 0)
			destrect.left = 0;
		if(destrect.right > work_rect.right)
			destrect.right = work_rect.right;
		if(destrect.top < 0)
			destrect.top = 0;
		if(destrect.bottom > work_rect.bottom)
			destrect.bottom = work_rect.bottom;
	}

	work_rect.Set(	0,0,_target->Width()-1,_target->Height()-1);

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

	// Set pointers to the actual data
	uint8 *dest_bits  = (uint8*) destbmp->Bits();	
	uint8 *src_bits = (uint8*) _target->Bits();

	// Get row widths for offset looping
	uint32 dest_width  = uint32 (destbmp->BytesPerRow());
	uint32 src_width = uint32 (_target->BytesPerRow());

	// Offset bitmap pointers to proper spot in each bitmap
	src_bits += uint32 ( (sourcerect.top  * src_width)  + (sourcerect.left  * colorspace_size) );
	dest_bits += uint32 ( (destrect.top * dest_width) + (destrect.left * colorspace_size) );

	uint32 line_length = uint32 ((destrect.right - destrect.left+1)*colorspace_size);
	uint32 lines = uint32 (destrect.bottom-destrect.top+1);

	for (uint32 pos_y = 0; pos_y != lines; pos_y++)
	{ 
		memcpy(dest_bits,src_bits,line_length);

		// Increment offsets
   		src_bits += src_width;
   		dest_bits += dest_width;
	}
}

void BitmapDriver::InvertRect(BRect r)
{
	Lock();
	if(_target)
	{
		if(r.top<0 || r.left<0 || 
			r.right>_target->Width()-1 || r.bottom>_target->Height()-1)
		{
			Unlock();
			return;
		}
		
		switch(_target->BitsPerPixel())
		{
			case 32:
			case 24:
			{
				uint16 width=r.IntegerWidth(), height=r.IntegerHeight();
				uint32 *start=(uint32*)_target->Bits(), *index;
				start+=int32(r.top)*_target->Width();
				start+=int32(r.left);
				
				for(int32 i=0;i<height;i++)
				{
					index=start + (i*_target->Width());
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


float BitmapDriver::StringWidth(const char *string, int32 length, LayerData *d)
{
	if(!string || !d )
		return 0.0;
	Lock();

	ServerFont *font=&(d->font);
	FontStyle *style=font->Style();

	if(!style)
	{
		Unlock();
		return 0.0;
	}

	FT_Face face;
	FT_GlyphSlot slot;
	FT_UInt glyph_index=0, previous=0;
	FT_Vector pen,delta;
	int16 error=0;
	int32 strlength,i;
	float returnval;

	error=FT_New_Face(ftlib, style->GetPath(), 0, &face);
	if(error)
	{
		Unlock();
		return 0.0;
	}

	slot=face->glyph;

	bool use_kerning=FT_HAS_KERNING(face) && font->Spacing()==B_STRING_SPACING;
	
	error=FT_Set_Char_Size(face, 0,int32(font->Size())*64,72,72);
	if(error)
	{
		Unlock();
		return 0.0;
	}

	// set the pen position in 26.6 cartesian space coordinates
	pen.x=0;
	
	slot=face->glyph;
	
	strlength=strlen(string);
	if(length<strlength)
		strlength=length;

	for(i=0;i<strlength;i++)
	{
		// get kerning and move pen
		if(use_kerning && previous && glyph_index)
		{
			FT_Get_Kerning(face, previous, glyph_index,ft_kerning_default, &delta);
			pen.x+=delta.x;
		}

		error=FT_Load_Char(face,string[i],FT_LOAD_MONOCHROME);

		// increment pen position
		pen.x+=slot->advance.x;
		previous=glyph_index;
	}

	FT_Done_Face(face);

	returnval=pen.x>>6;
	Unlock();
	return returnval;
}

float BitmapDriver::StringHeight(const char *string, int32 length, LayerData *d)
{
	if(!string || !d)
		return 0.0;
	Lock();

	ServerFont *font=&(d->font);
	FontStyle *style=font->Style();

	if(!style)
	{
		Unlock();
		return 0.0;
	}

	FT_Face face;
	FT_GlyphSlot slot;
	int16 error=0;
	int32 strlength,i;
	float returnval=0.0,ascent=0.0,descent=0.0;

	error=FT_New_Face(ftlib, style->GetPath(), 0, &face);
	if(error)
	{
		Unlock();
		return 0.0;
	}

	slot=face->glyph;
	
	error=FT_Set_Char_Size(face, 0,int32(font->Size())*64,72,72);
	if(error)
	{
		Unlock();
		return 0.0;
	}

	slot=face->glyph;
	
	strlength=strlen(string);
	if(length<strlength)
		strlength=length;

	for(i=0;i<strlength;i++)
	{
		FT_Load_Char(face,string[i],FT_LOAD_RENDER);
		if(slot->metrics.horiBearingY<slot->metrics.height)
			descent=MAX((slot->metrics.height-slot->metrics.horiBearingY)>>6,descent);
		else
			ascent=MAX(slot->bitmap.rows,ascent);
	}
	Unlock();

	FT_Done_Face(face);

	returnval=ascent+descent;
	Unlock();
	return returnval;
}

/*!
	\brief Utilizes the font engine to draw a string to the frame buffer
	\param string String to be drawn. Always non-NULL.
	\param length Number of characters in the string to draw. Always greater than 0. If greater
	than the number of characters in the string, draw the entire string.
	\param pt Point at which the baseline starts. Characters are to be drawn 1 pixel above
	this for backwards compatibility. While the point itself is guaranteed to be inside
	the frame buffers coordinate range, the clipping of each individual glyph must be
	performed by the driver itself.
	\param d Data structure containing any other data necessary for the call. Always non-NULL.
*/
void BitmapDriver::DrawString(const char *string, int32 length, BPoint pt, LayerData *d, escapement_delta *edelta)
{
	if(!string || !d)
		return;

	Lock();

	pt.y--;	// because of Be's backward compatibility hack

	ServerFont *font=&(d->font);
	FontStyle *style=font->Style();

	if(!style)
	{
		Unlock();
		return;
	}

	FT_Face face;
	FT_GlyphSlot slot;
	FT_Matrix rmatrix,smatrix;
	FT_UInt glyph_index=0, previous=0;
	FT_Vector pen,delta,space,nonspace;
	int16 error=0;
	int32 strlength,i;
	Angle rotation(font->Rotation()), shear(font->Shear());

	bool antialias=( (font->Size()<18 && font->Flags()& B_DISABLE_ANTIALIASING==0)
		|| font->Flags()& B_FORCE_ANTIALIASING)?true:false;

	// Originally, I thought to do this shear checking here, but it really should be
	// done in BFont::SetShear()
	float shearangle=shear.Value();
	if(shearangle>135)
		shearangle=135;
	if(shearangle<45)
		shearangle=45;

	if(shearangle>90)
		shear=90+((180-shearangle)*2);
	else
		shear=90-(90-shearangle)*2;
	
	error=FT_New_Face(ftlib, style->GetPath(), 0, &face);
	if(error)
	{
		printf("Couldn't create face object\n");
		Unlock();
		return;
	}

	slot=face->glyph;

	bool use_kerning=FT_HAS_KERNING(face) && font->Spacing()==B_STRING_SPACING;
	
	error=FT_Set_Char_Size(face, 0,int32(font->Size())*64,72,72);
	if(error)
	{
		Unlock();
		return;
	}

	// if we do any transformation, we do a call to FT_Set_Transform() here
	
	// First, rotate
	rmatrix.xx = (FT_Fixed)( rotation.Cosine()*0x10000); 
	rmatrix.xy = (FT_Fixed)( rotation.Sine()*0x10000); 
	rmatrix.yx = (FT_Fixed)(-rotation.Sine()*0x10000); 
	rmatrix.yy = (FT_Fixed)( rotation.Cosine()*0x10000); 
	
	// Next, shear
	smatrix.xx = (FT_Fixed)(0x10000); 
	smatrix.xy = (FT_Fixed)(-shear.Cosine()*0x10000); 
	smatrix.yx = (FT_Fixed)(0); 
	smatrix.yy = (FT_Fixed)(0x10000); 

	//FT_Matrix_Multiply(&rmatrix,&smatrix);
	FT_Matrix_Multiply(&smatrix,&rmatrix);
	
	// Set up the increment value for escapement padding
	space.x=int32(d->edelta.space * rotation.Cosine()*64);
	space.y=int32(d->edelta.space * rotation.Sine()*64);
	nonspace.x=int32(d->edelta.nonspace * rotation.Cosine()*64);
	nonspace.y=int32(d->edelta.nonspace * rotation.Sine()*64);
	
	// set the pen position in 26.6 cartesian space coordinates
	pen.x=(int32)pt.x * 64;
	pen.y=(int32)pt.y * 64;
	
	slot=face->glyph;

	
	strlength=strlen(string);
	if(length<strlength)
		strlength=length;

	for(i=0;i<strlength;i++)
	{
		//FT_Set_Transform(face,&smatrix,&pen);
		FT_Set_Transform(face,&rmatrix,&pen);

		// Handle escapement padding option
		if((uint8)string[i]<=0x20)
		{
			pen.x+=space.x;
			pen.y+=space.y;
		}
		else
		{
			pen.x+=nonspace.x;
			pen.y+=nonspace.y;
		}

	
		// get kerning and move pen
		if(use_kerning && previous && glyph_index)
		{
			FT_Get_Kerning(face, previous, glyph_index,ft_kerning_default, &delta);
			pen.x+=delta.x;
			pen.y+=delta.y;
		}

		error=FT_Load_Char(face,string[i],
			((antialias)?FT_LOAD_RENDER:FT_LOAD_RENDER | FT_LOAD_MONOCHROME) );

		if(!error)
		{
			if(antialias)
				BlitGray2RGB32(&slot->bitmap,
					BPoint(slot->bitmap_left,pt.y-(slot->bitmap_top-pt.y)), d);
			else
				BlitMono2RGB32(&slot->bitmap,
					BPoint(slot->bitmap_left,pt.y-(slot->bitmap_top-pt.y)), d);
		}
		else
			printf("Couldn't load character %c\n", string[i]);

		// increment pen position
		pen.x+=slot->advance.x;
		pen.y+=slot->advance.y;
		previous=glyph_index;
	}
	FT_Done_Face(face);
	Unlock();
}

void BitmapDriver::BlitMono2RGB32(FT_Bitmap *src, BPoint pt, LayerData *d)
{
	rgb_color color=d->highcolor.GetColor32();
	
	// pointers to the top left corner of the area to be copied in each bitmap
	uint8 *srcbuffer, *destbuffer;
	
	// index pointers which are incremented during the course of the blit
	uint8 *srcindex, *destindex, *rowptr, value;
	
	// increment values for the index pointers
	int32 srcinc=src->pitch, destinc=_target->BytesPerRow();
	
	int16 i,j,k, srcwidth=src->pitch, srcheight=src->rows;
	int32 x=(int32)pt.x,y=(int32)pt.y;
	
	// starting point in source bitmap
	srcbuffer=(uint8*)src->buffer;

	if(y<0)
	{
		if(y<pt.y)
			y++;
		srcbuffer+=srcinc * (0-y);
		srcheight-=srcinc;
		destbuffer+=destinc * (0-y);
	}

	if(y+srcheight>_target->Height())
	{
		if(y>pt.y)
			y--;
		srcheight-=(y+srcheight-1)-_target->Height();
	}

	if(x+srcwidth>_target->Width())
	{
		if(x>pt.x)
			x--;
		srcwidth-=(x+srcwidth-1)-_target->Width();
	}
	
	if(x<0)
	{
		if(x<pt.x)
			x++;
		srcbuffer+=(0-x)>>3;
		srcwidth-=0-x;
		destbuffer+=(0-x)*4;
	}
	
	// starting point in destination bitmap
	destbuffer=(uint8*)_target->Bits()+int32( (pt.y*_target->BytesPerRow())+(pt.x*4) );

	srcindex=srcbuffer;
	destindex=destbuffer;

	for(i=0; i<srcheight; i++)
	{
		rowptr=destindex;		

		for(j=0;j<srcwidth;j++)
		{
			for(k=0; k<8; k++)
			{
				value=*(srcindex+j) & (1 << (7-k));
				if(value)
				{
					rowptr[0]=color.blue;
					rowptr[1]=color.green;
					rowptr[2]=color.red;
					rowptr[3]=color.alpha;
				}

				rowptr+=4;
			}

		}
		
		srcindex+=srcinc;
		destindex+=destinc;
	}

}

void BitmapDriver::BlitGray2RGB32(FT_Bitmap *src, BPoint pt, LayerData *d)
{
	// pointers to the top left corner of the area to be copied in each bitmap
	uint8 *srcbuffer=NULL, *destbuffer=NULL;
	
	// index pointers which are incremented during the course of the blit
	uint8 *srcindex=NULL, *destindex=NULL, *rowptr=NULL;
	
	rgb_color highcolor=d->highcolor.GetColor32(), lowcolor=d->lowcolor.GetColor32();	float rstep,gstep,bstep,astep;

	rstep=float(highcolor.red-lowcolor.red)/255.0;
	gstep=float(highcolor.green-lowcolor.green)/255.0;
	bstep=float(highcolor.blue-lowcolor.blue)/255.0;
	astep=float(highcolor.alpha-lowcolor.alpha)/255.0;
	
	// increment values for the index pointers
	int32 x=(int32)pt.x,
		y=(int32)pt.y,
		srcinc=src->pitch,
//		destinc=dest->BytesPerRow(),
		destinc=_target->BytesPerRow(),
		srcwidth=src->width,
		srcheight=src->rows,
		incval=0;
	
	int16 i,j;
	
	// starting point in source bitmap
	srcbuffer=(uint8*)src->buffer;

	// starting point in destination bitmap
//	destbuffer=(uint8*)dest->Bits()+(y*dest->BytesPerRow()+(x*4));
	destbuffer=(uint8*)_target->Bits()+(y*_target->BytesPerRow()+(x*4));


	if(y<0)
	{
		if(y<pt.y)
			y++;
		
		incval=0-y;
		
		srcbuffer+=incval * srcinc;
		srcheight-=incval;
		destbuffer+=incval * destinc;
	}

	if(y+srcheight>_target->Height())
	{
		if(y>pt.y)
			y--;
		srcheight-=(y+srcheight-1)-_target->Height();
	}

	if(x+srcwidth>_target->Width())
	{
		if(x>pt.x)
			x--;
		srcwidth-=(x+srcwidth-1)-_target->Width();
	}
	
	if(x<0)
	{
		if(x<pt.x)
			x++;
		incval=0-x;
		srcbuffer+=incval;
		srcwidth-=incval;
		destbuffer+=incval*4;
	}

	int32 value;

	srcindex=srcbuffer;
	destindex=destbuffer;

	for(i=0; i<srcheight; i++)
	{
		rowptr=destindex;		

		for(j=0;j<srcwidth;j++)
		{
			value=*(srcindex+j) ^ 255;

			if(value!=255)
			{
				if(d->draw_mode==B_OP_COPY)
				{
					rowptr[0]=uint8(highcolor.blue-(value*bstep));
					rowptr[1]=uint8(highcolor.green-(value*gstep));
					rowptr[2]=uint8(highcolor.red-(value*rstep));
					rowptr[3]=255;
				}
				else
					if(d->draw_mode==B_OP_OVER)
					{
						if(highcolor.alpha>127)
						{
							rowptr[0]=uint8(highcolor.blue-(value*(float(highcolor.blue-rowptr[0])/255.0)));
							rowptr[1]=uint8(highcolor.green-(value*(float(highcolor.green-rowptr[1])/255.0)));
							rowptr[2]=uint8(highcolor.red-(value*(float(highcolor.red-rowptr[2])/255.0)));
							rowptr[3]=255;
						}
					}
			}
			rowptr+=4;

		}
		
		srcindex+=srcinc;
		destindex+=destinc;
	}
}

rgb_color BitmapDriver::GetBlitColor(rgb_color src, rgb_color dest, LayerData *d, bool use_high)
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
	int bytes_per_row = _target->BytesPerRow();
	left = x - fLineThickness/2;
	right = x + fLineThickness/2;
	top = y - fLineThickness/2;
	bottom = y + fLineThickness/2;
	switch(_target->BitsPerPixel())
	{
		case 8:
			{
				int x,y;
				uint8 *fb = (uint8 *)_target->Bits() + top*bytes_per_row;
				for (y=top; y<=bottom; y++)
				{
					for (x=left; x<=right; x++)
						fb[x] = fDrawPattern.GetColor(x,y).GetColor8();;
					fb += bytes_per_row;
				}
			} break;
		case 15:
			{
				int x,y;
				uint16 *fb = (uint16 *)((uint8 *)_target->Bits() + top*bytes_per_row);
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
				uint16 *fb = (uint16 *)((uint8 *)_target->Bits() + top*bytes_per_row);
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
				uint32 *fb = (uint32 *)((uint8 *)_target->Bits() + top*bytes_per_row);
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
	int bytes_per_row = _target->BytesPerRow();

	if ( x1 > x2 )
	{
		x = x2;
		x2 = x1;
		x1 = x;
	}
	y1 = y - fLineThickness/2;
	y2 = y + fLineThickness/2;
	switch(_target->BitsPerPixel())
	{
		case 8:
			{
				uint8 *fb = (uint8 *)_target->Bits() + y1*bytes_per_row;
				for (y=y1; y<=y2; y++)
				{
					for (x=x1; x<=x2; x++)
						fb[x] = fDrawPattern.GetColor(x,y).GetColor8();
					fb += bytes_per_row;
				}
			} break;
		case 15:
			{
				uint16 *fb = (uint16 *)((uint8 *)_target->Bits() + y1*bytes_per_row);
				for (y=y1; y<=y2; y++)
				{
					for (x=x1; x<=x2; x++)
						fb[x] = fDrawPattern.GetColor(x,y).GetColor15();
					fb = (uint16 *)((uint8 *)fb + bytes_per_row);
				}
			} break;
		case 16:
			{
				uint16 *fb = (uint16 *)((uint8 *)_target->Bits() + y1*bytes_per_row);
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
				uint32 *fb = (uint32 *)((uint8 *)_target->Bits() + y1*bytes_per_row);
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
	int bytes_per_row = _target->BytesPerRow();

	if ( y1 > y2 )
	{
		y = y2;
		y2 = y1;
		y1 = y;
	}
	x1 = x - fLineThickness/2;
	x2 = x + fLineThickness/2;
	switch(_target->BitsPerPixel())
	{
		case 8:
			{
				uint8 *fb = (uint8 *)_target->Bits() + y1*bytes_per_row;
				for (y=y1; y<=y2; y++)
				{
					for (x=x1; x<=x2; x++)
						fb[x] = fDrawPattern.GetColor(x,y).GetColor8();
					fb += bytes_per_row;
				}
			} break;
		case 15:
			{
				uint16 *fb = (uint16 *)((uint8 *)_target->Bits() + y1*bytes_per_row);
				for (y=y1; y<=y2; y++)
				{
					for (x=x1; x<=x2; x++)
						fb[x] = fDrawPattern.GetColor(x,y).GetColor15();
					fb = (uint16 *)((uint8 *)fb + bytes_per_row);
				}
			} break;
		case 16:
			{
				uint16 *fb = (uint16 *)((uint8 *)_target->Bits() + y1*bytes_per_row);
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
				uint32 *fb = (uint32 *)((uint8 *)_target->Bits() + y1*bytes_per_row);
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

void BitmapDriver::FillSolidRect(int32 left, int32 top, int32 right, int32 bottom)
{
	int bytes_per_row = _target->BytesPerRow();
	switch(_target->BitsPerPixel())
	{
		case 8:
			{
				uint8 *fb = (uint8 *)_target->Bits() + top*bytes_per_row;
				uint8 color8 = fDrawColor.GetColor8();
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
				uint16 *fb = (uint16 *)((uint8 *)_target->Bits() + top*bytes_per_row);
				uint16 color15 = fDrawColor.GetColor15();
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
				uint16 *fb = (uint16 *)((uint8 *)_target->Bits() + top*bytes_per_row);
				uint16 color16 = fDrawColor.GetColor16();
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
				uint32 *fb = (uint32 *)((uint8 *)_target->Bits() + top*bytes_per_row);
				rgb_color fill_color = fDrawColor.GetColor32();
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

void BitmapDriver::FillPatternRect(int32 left, int32 top, int32 right, int32 bottom)
{
	int bytes_per_row = _target->BytesPerRow();
	switch(_target->BitsPerPixel())
	{
		case 8:
			{
				uint8 *fb = (uint8 *)_target->Bits() + top*bytes_per_row;
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
				uint16 *fb = (uint16 *)((uint8 *)_target->Bits() + top*bytes_per_row);
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
				uint16 *fb = (uint16 *)((uint8 *)_target->Bits() + top*bytes_per_row);
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
				uint32 *fb = (uint32 *)((uint8 *)_target->Bits() + top*bytes_per_row);
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
