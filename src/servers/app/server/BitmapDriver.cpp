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
//					
//	Description:	Driver to draw on ServerBitmaps
//  
//------------------------------------------------------------------------------
#include "Angle.h"
#include "AccelerantDriver.h"
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

#ifndef CLIP_X

#define CLIP_X(a) ( (a < 0) ? 0 : ((a > _target->Width()-1) ? \
			_target->Width()-1 : a) )
#define CLIP_Y(a) ( (a < 0) ? 0 : ((a > _target->Height()-1) ? \
			_target->Height()-1 : a) )
#define CHECK_X(a) ( (a >= 0) || (a <= _target->Width()-1) )
#define CHECK_Y(a) ( (a >= 0) || (a <= _target->Height()-1) )

#endif

extern RGBColor workspace_default_color;	// defined in AppServer.cpp

void HLine_32Bit(graphics_card_info i, uint16 x, uint16 y, uint16 length, rgb_color col);
void HLine_16Bit(graphics_card_info i, uint16 x, uint16 y, uint16 length, uint16 col);
void HLine_16Bit(graphics_card_info i, uint16 x, uint16 y, uint16 length, uint8 col);

class BitmapLineCalc
{
public:
	BitmapLineCalc(const BPoint &pta, const BPoint &ptb);
	float GetX(float y);
	float GetY(float x);
	float Slope(void) { return slope; }
	float Offset(void) { return offset; }
private:
	float slope;
	float offset;
	BPoint start, end;
};

BitmapLineCalc::BitmapLineCalc(const BPoint &pta, const BPoint &ptb)
{
	start=pta;
	end=ptb;
	slope=(start.y-end.y)/(start.x-end.x);
	offset=start.y-(slope * start.x);
}

float BitmapLineCalc::GetX(float y)
{
	return ( (y-offset)/slope );
}

float BitmapLineCalc::GetY(float x)
{
	return ( (slope * x) + offset );
}

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

/*!
	\brief Called for all BView::FillArc calls
	\param r Rectangle enclosing the entire arc
	\param angle Starting angle for the arc in degrees
	\param span Span of the arc in degrees. Ending angle = angle+span.
	\param d Data structure containing any other data necessary for the call. Always non-NULL.
	\param pat 8-byte array containing the pattern to use. Always non-NULL.

	Bounds checking must be done in this call because only part of the arc may end up
	being clipped.
*/
void BitmapDriver::FillArc(BRect r, float angle, float span, LayerData *d, const Pattern &pat)
{
	float xc = (r.left+r.right)/2;
	float yc = (r.top+r.bottom)/2;
	float rx = r.Width()/2;
	float ry = r.Height()/2;
	int Rx2 = ROUND(rx*rx);
	int Ry2 = ROUND(ry*ry);
	int twoRx2 = 2*Rx2;
	int twoRy2 = 2*Ry2;
	int p;
	int x=0;
	int y = (int)ry;
	int px = 0;
	int py = twoRx2 * y;
	int startx, endx;
	int starty, endy;
	int xclip, startclip, endclip;
	int startQuad, endQuad;
	bool useQuad1, useQuad2, useQuad3, useQuad4;
	bool shortspan = false;
	float oldpensize;
	BPoint center(xc,yc);

	// Watch out for bozos giving us whacko spans
	if ( (span >= 360) || (span <= -360) )
	{
	  FillEllipse(r,d,pat);
	  return;
	}

	Lock();
	PatternHandler pattern(pat);
	pattern.SetColors(d->highcolor, d->lowcolor);
	oldpensize = d->pensize;
	d->pensize = 1;

	if ( span > 0 )
	{
		startQuad = (int)(angle/90)%4+1;
		endQuad = (int)((angle+span)/90)%4+1;
		startx = ROUND(.5*r.Width()*fabs(cos(angle*M_PI/180)));
		endx = ROUND(.5*r.Width()*fabs(cos((angle+span)*M_PI/180)));
	}
	else
	{
		endQuad = (int)(angle/90)%4+1;
		startQuad = (int)((angle+span)/90)%4+1;
		endx = ROUND(.5*r.Width()*fabs(cos(angle*M_PI/180)));
		startx = ROUND(.5*r.Width()*fabs(cos((angle+span)*M_PI/180)));
	}

	starty = ROUND(ry*sqrt(1-(double)startx*startx/(rx*rx)));
	endy = ROUND(ry*sqrt(1-(double)endx*endx/(rx*rx)));

	if ( startQuad != endQuad )
	{
		useQuad1 = (endQuad > 1) && (startQuad > endQuad);
		useQuad2 = ((startQuad == 1) && (endQuad > 2)) || ((startQuad > endQuad) && (endQuad > 2));
		useQuad3 = ((startQuad < 3) && (endQuad == 4)) || ((startQuad < 3) && (endQuad < startQuad));
		useQuad4 = (startQuad < 4) && (startQuad > endQuad);
	}
	else
	{
		if ( (span < 90) && (span > -90) )
		{
			useQuad1 = false;
			useQuad2 = false;
			useQuad3 = false;
			useQuad4 = false;
			shortspan = true;
		}
		else
		{
			useQuad1 = (startQuad != 1);
			useQuad2 = (startQuad != 2);
			useQuad3 = (startQuad != 3);
			useQuad4 = (startQuad != 4);
		}
	}

	if ( useQuad1 && CHECK_Y(yc-y) && (CHECK_X(xc) || CHECK_X(xc+x)) )
		HLine(ROUND(CLIP_X(xc)),ROUND(CLIP_X(xc+x)),ROUND(yc-y),&pattern);
	if ( useQuad2 && CHECK_Y(yc-y) && (CHECK_X(xc) || CHECK_X(xc-x)) )
		HLine(ROUND(CLIP_X(xc)),ROUND(CLIP_X(xc-x)),ROUND(yc-y),&pattern);
	if ( useQuad3 && CHECK_Y(yc+y) && (CHECK_X(xc) || CHECK_X(xc-x)) )
		HLine(ROUND(CLIP_X(xc)),ROUND(CLIP_X(xc-x)),ROUND(yc+y),&pattern);
	if ( useQuad4 && CHECK_Y(yc+y) && (CHECK_X(xc) || CHECK_X(xc+x)) )
		HLine(ROUND(CLIP_X(xc)),ROUND(CLIP_X(xc+x)),ROUND(yc+y),&pattern);
	if ( (!shortspan && (((startQuad == 1) && (x <= startx)) || ((endQuad == 1) && (x >= endx)))) || 
	     (shortspan && (startQuad == 1) && (x <= startx) && (x >= endx)) ) 
		StrokeLine(BPoint(xc+x,yc-y),center,d,pat);
	if ( (!shortspan && (((startQuad == 2) && (x >= startx)) || ((endQuad == 2) && (x <= endx)))) || 
	     (shortspan && (startQuad == 2) && (x >= startx) && (x <= endx)) ) 
		StrokeLine(BPoint(xc-x,yc-y),center,d,pat);
	if ( (!shortspan && (((startQuad == 3) && (x <= startx)) || ((endQuad == 3) && (x >= endx)))) || 
	     (shortspan && (startQuad == 3) && (x <= startx) && (x >= endx)) ) 
		StrokeLine(BPoint(xc-x,yc+y),center,d,pat);
	if ( (!shortspan && (((startQuad == 4) && (x >= startx)) || ((endQuad == 4) && (x <= endx)))) || 
	     (shortspan && (startQuad == 4) && (x >= startx) && (x <= endx)) ) 
		StrokeLine(BPoint(xc+x,yc+y),center,d,pat);

	p = ROUND (Ry2 - (Rx2 * ry) + (.25 * Rx2));
	while (px < py)
	{
		x++;
		px += twoRy2;
		if ( p < 0 )
			p += Ry2 + px;
		else
		{
			y--;
			py -= twoRx2;
			p += Ry2 + px - py;
		}

		if ( useQuad1 && CHECK_Y(yc-y) && (CHECK_X(xc) || CHECK_X(xc+x)) )
			HLine(ROUND(CLIP_X(xc)),ROUND(CLIP_X(xc+x)),ROUND(yc-y),&pattern);
		if ( useQuad2 && CHECK_Y(yc-y) && (CHECK_X(xc) || CHECK_X(xc-x)) )
			HLine(ROUND(CLIP_X(xc)),ROUND(CLIP_X(xc-x)),ROUND(yc-y),&pattern);
		if ( useQuad3 && CHECK_Y(yc+y) && (CHECK_X(xc) || CHECK_X(xc-x)) )
			HLine(ROUND(CLIP_X(xc)),ROUND(CLIP_X(xc-x)),ROUND(yc+y),&pattern);
		if ( useQuad4 && CHECK_Y(yc+y) && (CHECK_X(xc) || CHECK_X(xc+x)) )
			HLine(ROUND(CLIP_X(xc)),ROUND(CLIP_X(xc+x)),ROUND(yc+y),&pattern);
		if ( !shortspan )
		{
			if ( startQuad == 1 )
			{
				if ( CHECK_Y(yc-y) )
				{
					if ( x <= startx )
					{
						if ( CHECK_X(xc) || CHECK_X(xc+x) )
							HLine(ROUND(CLIP_X(xc)),ROUND(CLIP_X(xc+x)),ROUND(yc-y),&pattern);
					}
					else
					{
						xclip = ROUND(y*startx/(double)starty);
						if ( CHECK_X(xc) || CHECK_X(xc+xclip) )
							HLine(ROUND(CLIP_X(xc)),ROUND(CLIP_X(xc+xclip)),ROUND(yc-y),&pattern);
					}
				}
			}
			else if ( startQuad == 2 )
			{
				if ( CHECK_Y(yc-y) )
				{
					if ( x >= startx )
					{
						xclip = ROUND(y*startx/(double)starty);
						if ( CHECK_X(xc-x) || CHECK_X(xc-xclip) )
							HLine(ROUND(CLIP_X(xc-x)),ROUND(CLIP_X(xc-xclip)),ROUND(yc-y),&pattern);
					}
				}
			}
			else if ( startQuad == 3 )
			{
				if ( CHECK_Y(yc+y) )
				{
					if ( x <= startx )
					{
						if ( CHECK_X(xc-x) || CHECK_X(xc) )
							HLine(ROUND(CLIP_X(xc-x)),ROUND(CLIP_X(xc)),ROUND(yc+y),&pattern);
					}
					else
					{
						xclip = ROUND(y*startx/(double)starty);
						if ( CHECK_X(xc-xclip) || CHECK_X(xc) )
							HLine(ROUND(CLIP_X(xc-xclip)),ROUND(CLIP_X(xc)),ROUND(yc+y),&pattern);
					}
				}
			}
			else if ( startQuad == 4 )
			{
				if ( CHECK_Y(yc+y) )
				{
					if ( x >= startx )
					{
						xclip = ROUND(y*startx/(double)starty);
						if ( CHECK_X(xc+xclip) || CHECK_X(xc+x) )
							HLine(ROUND(CLIP_X(xc+xclip)),ROUND(CLIP_X(xc+x)),ROUND(yc+y),&pattern);
					}
				}
			}

			if ( endQuad == 1 )
			{
				if ( CHECK_Y(yc-y) )
				{
					if ( x >= endx )
					{
						xclip = ROUND(y*endx/(double)endy);
						if ( CHECK_X(xc+xclip) || CHECK_X(xc+x) )
							HLine(ROUND(CLIP_X(xc+xclip)),ROUND(CLIP_X(xc+x)),ROUND(yc-y),&pattern);
					}
				}
			}
			else if ( endQuad == 2 )
			{
				if ( CHECK_Y(yc-y) )
				{
					if ( x <= endx )
					{
						if ( CHECK_X(xc-x) || CHECK_X(xc) )
							HLine(ROUND(CLIP_X(xc-x)),ROUND(CLIP_X(xc)),ROUND(yc-y),&pattern);
					}
					else
					{
						xclip = ROUND(y*endx/(double)endy);
						if ( CHECK_X(xc-xclip) || CHECK_X(xc) )
							HLine(ROUND(CLIP_X(xc-xclip)),ROUND(CLIP_X(xc)),ROUND(yc-y),&pattern);
					}
				}
			}
			else if ( endQuad == 3 )
			{
				if ( CHECK_Y(yc+y) )
				{
					if ( x >= endx )
					{
						xclip = ROUND(y*endx/(double)endy);
						if ( CHECK_X(xc-x) || CHECK_X(xc-xclip) )
							HLine(ROUND(CLIP_X(xc-x)),ROUND(CLIP_X(xc-xclip)),ROUND(yc+y),&pattern);
					}
				}
			}
			else if ( endQuad == 4 )
			{
				if ( CHECK_Y(yc+y) )
				{
					if ( x <= endx )
					{
						if ( CHECK_X(xc) || CHECK_X(xc+x) )
							HLine(ROUND(CLIP_X(xc)),ROUND(CLIP_X(xc+x)),ROUND(yc+y),&pattern);
					}
					else
					{
						xclip = ROUND(y*endx/(double)endy);
						if ( CHECK_X(xc) || CHECK_X(xc+xclip) )
							HLine(ROUND(CLIP_X(xc)),ROUND(CLIP_X(xc+xclip)),ROUND(yc+y),&pattern);
					}
				}
			}
		}
		else
		{
			startclip = ROUND(y*startx/(double)starty);
			endclip = ROUND(y*endx/(double)endy);
			if ( startQuad == 1 )
			{
				if ( CHECK_Y(yc-y) )
				{
					if ( (x <= startx) && (x >= endx) )
					{
						if ( CHECK_X(xc+endclip) || CHECK_X(xc+x) )
							HLine(ROUND(CLIP_X(xc+endclip)),ROUND(CLIP_X(xc+x)),ROUND(yc-y),&pattern);
					}
					else
					{
						if ( CHECK_X(xc+endclip) || CHECK_X(xc+startclip) )
							HLine(ROUND(CLIP_X(xc+endclip)),ROUND(CLIP_X(xc+startclip)),ROUND(yc-y),&pattern);
					}
				}
			}
			else if ( startQuad == 2 )
			{
				if ( CHECK_Y(yc-y) )
				{
					if ( (x <= startx) && (x >= endx) )
					{
						if ( CHECK_X(xc-x) || CHECK_X(xc-startclip) )
							HLine(ROUND(CLIP_X(xc-x)),ROUND(CLIP_X(xc-startclip)),ROUND(yc-y),&pattern);
					}
					else
					{
						if ( CHECK_X(xc-endclip) || CHECK_X(xc-startclip) )
							HLine(ROUND(CLIP_X(xc-endclip)),ROUND(CLIP_X(xc-startclip)),ROUND(yc-y),&pattern);
					}
				}
			}
			else if ( startQuad == 3 )
			{
				if ( CHECK_Y(yc+y) )
				{
					if ( (x <= startx) && (x >= endx) )
					{
						if ( CHECK_X(xc-x) || CHECK_X(xc-endclip) )
							HLine(ROUND(CLIP_X(xc-x)),ROUND(CLIP_X(xc-endclip)),ROUND(yc+y),&pattern);
					}
					else
					{
						if ( CHECK_X(xc-startclip) || CHECK_X(xc-endclip) )
							HLine(ROUND(CLIP_X(xc-startclip)),ROUND(CLIP_X(xc-endclip)),ROUND(yc+y),&pattern);
					}
				}
			}
			else if ( startQuad == 4 )
			{
				if ( CHECK_Y(yc+y) )
				{
					if ( (x <= startx) && (x >= endx) )
					{
						if ( CHECK_X(xc+startclip) || CHECK_X(xc+x) )
							HLine(ROUND(CLIP_X(xc+startclip)),ROUND(CLIP_X(xc+x)),ROUND(yc+y),&pattern);
					}
					else
					{
						if ( CHECK_X(xc+startclip) || CHECK_X(xc+endclip) )
							HLine(ROUND(CLIP_X(xc+startclip)),ROUND(CLIP_X(xc+endclip)),ROUND(yc+y),&pattern);
					}
				}
			}
		}
	}

	p = ROUND(Ry2*(x+.5)*(x+.5) + Rx2*(y-1)*(y-1) - Rx2*Ry2);
	while (y>0)
	{
		y--;
		py -= twoRx2;
		if (p>0)
			p += Rx2 - py;
		else
		{
			x++;
			px += twoRy2;
			p += Rx2 - py +px;
		}

		if ( useQuad1 && CHECK_Y(yc-y) && (CHECK_X(xc) || CHECK_X(xc+x)) )
			HLine(ROUND(CLIP_X(xc)),ROUND(CLIP_X(xc+x)),ROUND(yc-y),&pattern);
		if ( useQuad2 && CHECK_Y(yc-y) && (CHECK_X(xc) || CHECK_X(xc-x)) )
			HLine(ROUND(CLIP_X(xc)),ROUND(CLIP_X(xc-x)),ROUND(yc-y),&pattern);
		if ( useQuad3 && CHECK_Y(yc+y) && (CHECK_X(xc) || CHECK_X(xc-x)) )
			HLine(ROUND(CLIP_X(xc)),ROUND(CLIP_X(xc-x)),ROUND(yc+y),&pattern);
		if ( useQuad4 && CHECK_Y(yc+y) && (CHECK_X(xc) || CHECK_X(xc+x)) )
			HLine(ROUND(CLIP_X(xc)),ROUND(CLIP_X(xc+x)),ROUND(yc+y),&pattern);
		if ( !shortspan )
		{
			if ( startQuad == 1 )
			{
				if ( CHECK_Y(yc-y) )
				{
					if ( x <= startx )
					{
						if ( CHECK_X(xc) || CHECK_X(xc+x) )
							HLine(ROUND(CLIP_X(xc)),ROUND(CLIP_X(xc+x)),ROUND(yc-y),&pattern);
					}
					else
					{
						xclip = ROUND(y*startx/(double)starty);
						if ( CHECK_X(xc) || CHECK_X(xc+xclip) )
							HLine(ROUND(CLIP_X(xc)),ROUND(CLIP_X(xc+xclip)),ROUND(yc-y),&pattern);
					}
				}
			}
			else if ( startQuad == 2 )
			{
				if ( CHECK_Y(yc-y) )
				{
					if ( x >= startx )
					{
						xclip = ROUND(y*startx/(double)starty);
						if ( CHECK_X(xc-x) || CHECK_X(xc-xclip) )
							HLine(ROUND(CLIP_X(xc-x)),ROUND(CLIP_X(xc-xclip)),ROUND(yc-y),&pattern);
					}
				}
			}
			else if ( startQuad == 3 )
			{
				if ( CHECK_Y(yc+y) )
				{
					if ( x <= startx )
					{
						if ( CHECK_X(xc-x) || CHECK_X(xc) )
							HLine(ROUND(CLIP_X(xc-x)),ROUND(CLIP_X(xc)),ROUND(yc+y),&pattern);
					}
					else
					{
						xclip = ROUND(y*startx/(double)starty);
						if ( CHECK_X(xc-xclip) || CHECK_X(xc) )
							HLine(ROUND(CLIP_X(xc-xclip)),ROUND(CLIP_X(xc)),ROUND(yc+y),&pattern);
					}
				}
			}
			else if ( startQuad == 4 )
			{
				if ( CHECK_Y(yc+y) )
				{
					if ( x >= startx )
					{
						xclip = ROUND(y*startx/(double)starty);
						if ( CHECK_X(xc+xclip) || CHECK_X(xc+x) )
							HLine(ROUND(CLIP_X(xc+xclip)),ROUND(CLIP_X(xc+x)),ROUND(yc+y),&pattern);
					}
				}
			}

			if ( endQuad == 1 )
			{
				if ( CHECK_Y(yc-y) )
				{
					if ( x >= endx )
					{
						xclip = ROUND(y*endx/(double)endy);
						if ( CHECK_X(xc+xclip) || CHECK_X(xc+x) )
							HLine(ROUND(CLIP_X(xc+xclip)),ROUND(CLIP_X(xc+x)),ROUND(yc-y),&pattern);
					}
				}
			}
			else if ( endQuad == 2 )
			{
				if ( CHECK_Y(yc-y) )
				{
					if ( x <= endx )
					{
						if ( CHECK_X(xc-x) || CHECK_X(xc) )
							HLine(ROUND(CLIP_X(xc-x)),ROUND(CLIP_X(xc)),ROUND(yc-y),&pattern);
					}
					else
					{
						xclip = ROUND(y*endx/(double)endy);
						if ( CHECK_X(xc-xclip) || CHECK_X(xc) )
							HLine(ROUND(CLIP_X(xc-xclip)),ROUND(CLIP_X(xc)),ROUND(yc-y),&pattern);
					}
				}
			}
			else if ( endQuad == 3 )
			{
				if ( CHECK_Y(yc+y) )
				{
					if ( x >= endx )
					{
						xclip = ROUND(y*endx/(double)endy);
						if ( CHECK_X(xc-x) || CHECK_X(xc-xclip) )
							HLine(ROUND(CLIP_X(xc-x)),ROUND(CLIP_X(xc-xclip)),ROUND(yc+y),&pattern);
					}
				}
			}
			else if ( endQuad == 4 )
			{
				if ( CHECK_Y(yc+y) )
				{
					if ( x <= endx )
					{
						if ( CHECK_X(xc) || CHECK_X(xc+x) )
							HLine(ROUND(CLIP_X(xc)),ROUND(CLIP_X(xc+x)),ROUND(yc+y),&pattern);
					}
					else
					{
						xclip = ROUND(y*endx/(double)endy);
						if ( CHECK_X(xc) || CHECK_X(xc+xclip) )
							HLine(ROUND(CLIP_X(xc)),ROUND(CLIP_X(xc+xclip)),ROUND(yc+y),&pattern);
					}
				}
			}
		}
		else
		{
			startclip = ROUND(y*startx/(double)starty);
			endclip = ROUND(y*endx/(double)endy);
			if ( startQuad == 1 )
			{
				if ( CHECK_Y(yc-y) )
				{
					if ( (x <= startx) && (x >= endx) )
					{
						if ( CHECK_X(xc+endclip) || CHECK_X(xc+x) )
							HLine(ROUND(CLIP_X(xc+endclip)),ROUND(CLIP_X(xc+x)),ROUND(yc-y),&pattern);
					}
					else
					{
						if ( CHECK_X(xc+endclip) || CHECK_X(xc+startclip) )
							HLine(ROUND(CLIP_X(xc+endclip)),ROUND(CLIP_X(xc+startclip)),ROUND(yc-y),&pattern);
					}
				}
			}
			else if ( startQuad == 2 )
			{
				if ( CHECK_Y(yc-y) )
				{
					if ( (x <= startx) && (x >= endx) )
					{
						if ( CHECK_X(xc-x) || CHECK_X(xc-startclip) )
							HLine(ROUND(CLIP_X(xc-x)),ROUND(CLIP_X(xc-startclip)),ROUND(yc-y),&pattern);
					}
					else
					{
						if ( CHECK_X(xc-endclip) || CHECK_X(xc-startclip) )
							HLine(ROUND(CLIP_X(xc-endclip)),ROUND(CLIP_X(xc-startclip)),ROUND(yc-y),&pattern);
					}
				}
			}
			else if ( startQuad == 3 )
			{
				if ( CHECK_Y(yc+y) )
				{
					if ( (x <= startx) && (x >= endx) )
					{
						if ( CHECK_X(xc-x) || CHECK_X(xc-endclip) )
							HLine(ROUND(CLIP_X(xc-x)),ROUND(CLIP_X(xc-endclip)),ROUND(yc+y),&pattern);
					}
					else
					{
						if ( CHECK_X(xc-startclip) || CHECK_X(xc-endclip) )
							HLine(ROUND(CLIP_X(xc-startclip)),ROUND(CLIP_X(xc-endclip)),ROUND(yc+y),&pattern);
					}
				}
			}
			else if ( startQuad == 4 )
			{
				if ( CHECK_Y(yc+y) )
				{
					if ( (x <= startx) && (x >= endx) )
					{
						if ( CHECK_X(xc+startclip) || CHECK_X(xc+x) )
							HLine(ROUND(CLIP_X(xc+startclip)),ROUND(CLIP_X(xc+x)),ROUND(yc+y),&pattern);
					}
					else
					{
						if ( CHECK_X(xc+startclip) || CHECK_X(xc+endclip) )
							HLine(ROUND(CLIP_X(xc+startclip)),ROUND(CLIP_X(xc+endclip)),ROUND(yc+y),&pattern);
					}
				}
			}
		}
	}
	d->pensize = oldpensize;
	Unlock();
}

/*!
	\brief Called for all BView::FillBezier calls.
	\param pts 4-element array of BPoints in the order of start, end, and then the two control
	points. 
	\param d Data structure containing any other data necessary for the call. Always non-NULL.
	\param pat 8-byte array containing the pattern to use. Always non-NULL.

	Bounds checking must be done in this call.
*/
void BitmapDriver::FillBezier(BPoint *pts, LayerData *d, const Pattern &pat)
{
	double Ax, Bx, Cx, Dx;
	double Ay, By, Cy, Dy;
	int x, y;
	int lastx=-1, lasty=-1;
	double t;
	double dt = .0002;
	double dt2, dt3;
	double X, Y, dx, ddx, dddx, dy, ddy, dddy;
	float oldpensize;
	bool steep = false;

	Lock();
	if ( fabs(pts[3].y-pts[0].y) > fabs(pts[3].x-pts[0].x) )
		steep = true;
	
	AccLineCalc line(pts[0], pts[3]);
	oldpensize = d->pensize;
	d->pensize = 1;

	Ax = -pts[0].x + 3*pts[1].x - 3*pts[2].x + pts[3].x;
	Bx = 3*pts[0].x - 6*pts[1].x + 3*pts[2].x;
	Cx = -3*pts[0].x + 3*pts[1].x;
	Dx = pts[0].x;

	Ay = -pts[0].y + 3*pts[1].y - 3*pts[2].y + pts[3].y;
	By = 3*pts[0].y - 6*pts[1].y + 3*pts[2].y;
	Cy = -3*pts[0].y + 3*pts[1].y;
	Dy = pts[0].y;
	
	dt2 = dt * dt;
	dt3 = dt2 * dt;
	X = Dx;
	dx = Ax*dt3 + Bx*dt2 + Cx*dt;
	ddx = 6*Ax*dt3 + 2*Bx*dt2;
	dddx = 6*Ax*dt3;
	Y = Dy;
	dy = Ay*dt3 + By*dt2 + Cy*dt;
	ddy = 6*Ay*dt3 + 2*By*dt2;
	dddy = 6*Ay*dt3;

	lastx = -1;
	lasty = -1;

	for (t=0; t<=1; t+=dt)
	{
		x = ROUND(X);
		y = ROUND(Y);
		if ( (x!=lastx) || (y!=lasty) )
		{
			if ( steep )
				StrokeLine(BPoint(x,y),BPoint(line.GetX(y),y),d,pat);
			else
				StrokeLine(BPoint(x,y),BPoint(x,line.GetY(x)),d,pat);
		}
		lastx = x;
		lasty = y;

		X += dx;
		dx += ddx;
		ddx += dddx;
		Y += dy;
		dy += ddy;
		ddy += dddy;
	}
	d->pensize = oldpensize;
	Unlock();
}

/*!
	\brief Called for all BView::FillEllipse calls
	\param r BRect enclosing the ellipse to be drawn.
	\param d Data structure containing any other data necessary for the call. Always non-NULL.
	\param pat 8-byte array containing the pattern to use. Always non-NULL.

	Bounds checking must be done in this call because only part of the ellipse may end up
	being clipped.
*/
void BitmapDriver::FillEllipse(BRect r, LayerData *ldata, const Pattern &pat)
{
// Ellipse code shamelessly stolen from the graphics library gd v2.0.1 and bolted on
// to support our API
	long d, b_sq, b_sq_4, b_sq_6;
	long a_sq, a_sq_4, a_sq_6;
	int x, y, switchem;
	long lsqrt (long);
	int pix, half, pstart;
	int32 thick = (int32)ldata->pensize;
	
	half = thick / 2;
	int32 w=int32(r.Width()/2),
		h=int32(r.Height()/2);
	int32 cx=(int32)r.left+w;
	int32 cy=(int32)r.top+h;
	
	d = 2 * (long) h *h + (long) w *w - 2 * (long) w *w * h;
	b_sq = (long) h *h;
	b_sq_4 = 4 * (long) h *h;
	b_sq_6 = 6 * (long) h *h;
	a_sq = (long) w *w;
	a_sq_4 = 4 * (long) w *w;
	a_sq_6 = 6 * (long) w *w;
	
	x = 0;
	y = -h;
//	switchem = a_sq / lsqrt (a_sq + b_sq);
	switchem = a_sq / (int32)sqrt(a_sq + b_sq);

	while (x <= switchem)
	{
		pstart = y - half;
		for (pix = pstart; pix < pstart + thick; pix++)
		{
			Line( BPoint(cx - x, cy + pix), BPoint(cx + x, cy + pix), ldata, pat_solidhigh);
			Line( BPoint(cx - x, cy - pix), BPoint(cx + x, cy - pix), ldata, pat_solidhigh);
		}
		if (d < 0)
			d += b_sq_4 * x++ + b_sq_6;
		else
			d += b_sq_4 * x++ + b_sq_6 + a_sq_4 * (++y);
	}
	
	/* Middlesplat!
	** Go a little further if the thickness is not nominal...
	*/
	if (thick > 1)
	{
		int xp = x;
		int yp = y;
		int dp = d;
		int thick2 = thick + 2;
		int half2 = half + 1;
		
		while (xp <= switchem + half)
		{
			pstart = yp - half2;
			for (pix = pstart; pix < pstart + thick2; pix++)
			{
				Line( BPoint(cx - xp, cy + pix), BPoint(cx + xp, cy + pix), ldata, pat_solidhigh);
				Line( BPoint(cx - xp, cy - pix), BPoint(cx + xp, cy - pix), ldata, pat_solidhigh);
			}
			if (dp < 0)
				dp += b_sq_4 * xp++ + b_sq_6;
			else
				dp += b_sq_4 * xp++ + b_sq_6 + a_sq_4 * (++yp);
		}
	}
	
	d += -2 * (long) b_sq + 2 * (long) a_sq - 2 * (long) b_sq *(x - 1) + 2 * (long) a_sq *(y - 1);
	
	while (y <= 0)
	{
		pstart = x - half;
		for (pix = pstart; pix < pstart + thick; pix++)
		{
			Line( BPoint(cx - pix, cy + y), BPoint(cx + pix, cy + y), ldata, pat_solidhigh);
			Line( BPoint(cx - pix, cy - y), BPoint(cx + pix, cy - y), ldata, pat_solidhigh);
		}
		
		if (d < 0)
			d += a_sq_4 * y++ + a_sq_6 + b_sq_4 * (++x);
		else
			d += a_sq_4 * y++ + a_sq_6;
	}
}

/*!
	\brief Called for all BView::FillPolygon calls
	\param ptlist Array of BPoints defining the polygon.
	\param numpts Number of points in the BPoint array.
	\param rect Rectangle which contains the polygon
	\param d Data structure containing any other data necessary for the call. Always non-NULL.
	\param pat 8-byte array containing the pattern to use. Always non-NULL.

	The points in the array are not guaranteed to be within the framebuffer's 
	coordinate range.
*/
void BitmapDriver::FillPolygon(BPoint *ptlist, int32 numpts, BRect rect, LayerData *d, const Pattern &pat)
{
}


/*!
	\brief Called for all BView::FillRect calls
	\param r BRect to be filled. Guaranteed to be in the frame buffer's coordinate space
	\param d Data structure containing any other data necessary for the call. Always non-NULL.
	\param pat 8-byte array containing the pattern to use. Always non-NULL.

*/
void BitmapDriver::FillRect(BRect r, LayerData *d, const Pattern &pat)
{
	Lock();
	if(_target)
	{
	//	int32 width=rect.IntegerWidth();
		for(int32 i=(int32)r.top;i<=r.bottom;i++)
	//		HLine(fbuffer->gcinfo,(int32)rect.left,i,width,col);
			Line(BPoint(r.left,i),BPoint(r.right,i),d,pat);
	}
	Unlock();
}

/*!
	\brief Called for all BView::FillRoundRect calls
	\param r The rectangle itself
	\param xrad X radius of the corner arcs
	\param yrad Y radius of the corner arcs
	\param d Data structure containing any other data necessary for the call. Always non-NULL.
	\param pat 8-byte array containing the pattern to use. Always non-NULL.

	Bounds checking must be done in this call because only part of the roundrect may end 
	up being clipped.
*/
void BitmapDriver::FillRoundRect(BRect r, float xrad, float yrad, LayerData *d, const Pattern &pat)
{
	float arc_x;
	float yrad2 = yrad*yrad;
	int i;

	Lock();
	PatternHandler pattern(pat);
	pattern.SetColors(d->highcolor, d->lowcolor);
	
	for (i=0; i<=(int)yrad; i++)
	{
		arc_x = xrad*sqrt(1-i*i/yrad2);
		if ( CHECK_Y(r.top+yrad-i) )
			HLine(ROUND(CLIP_X(r.left+xrad-arc_x)), ROUND(CLIP_X(r.right-xrad+arc_x)),
				ROUND(r.top+yrad-i), &pattern);
		if ( CHECK_Y(r.bottom-yrad+i) )
			HLine(ROUND(CLIP_X(r.left+xrad-arc_x)), ROUND(CLIP_X(r.right-xrad+arc_x)),
				ROUND(r.bottom-yrad+i), &pattern);
	}
	FillRect(BRect(CLIP_X(r.left),CLIP_Y(r.top+yrad),
			CLIP_X(r.right),CLIP_Y(r.bottom-yrad)), d, pat);

	Unlock();
}

/*!
	\brief Called for all BView::FillTriangle calls
	\param pts Array of 3 BPoints. Always non-NULL.
	\param r BRect enclosing the triangle. While it will definitely enclose the triangle,
	it may not be within the frame buffer's bounds.
	\param d Data structure containing any other data necessary for the call. Always non-NULL.
	\param pat 8-byte array containing the pattern to use. Always non-NULL.

	Bounds checking must be done in this call because only part of the triangle may end 
	up being clipped.
*/
void BitmapDriver::FillTriangle(BPoint *pts, BRect r, LayerData *d, const Pattern &pat)
{
	if(!pts || !d)
		return;

	Lock();
	if(_target)
	{
		BPoint first, second, third;

		// Sort points according to their y values
		if(pts[0].y < pts[1].y)
		{
			first=pts[0];
			second=pts[1];
		}
		else
		{
			first=pts[1];
			second=pts[0];
		}
		
		if(second.y<pts[2].y)
		{
			third=pts[2];
		}
		else
		{
			// second is lower than "third", so we must ensure that this third point
			// isn't higher than our first point
			third=second;
			if(first.y<pts[2].y)
				second=pts[2];
			else
			{
				second=first;
				first=pts[2];
			}
		}
		
		// Now that the points are sorted, check to see if they all have the same
		// y value
		if(first.y==second.y && second.y==third.y)
		{
			BPoint start,end;
			start.y=first.y;
			end.y=first.y;
			start.x=MIN(first.x,MIN(second.x,third.x));
			end.x=MAX(first.x,MAX(second.x,third.x));
			Line(start,end, d, pat);
			Unlock();
			return;
		}

		int32 i;

		// Special case #1: first and second in the same row
		if(first.y==second.y)
		{
			BitmapLineCalc lineA(first, third);
			BitmapLineCalc lineB(second, third);
			
			Line(first, second,d,pat);
			for(i=int32(first.y+1);i<third.y;i++)
				Line( BPoint(lineA.GetX(i),i), BPoint(lineB.GetX(i),i),d,pat);
			Unlock();
			return;
		}
		
		// Special case #2: second and third in the same row
		if(second.y==third.y)
		{
			BitmapLineCalc lineA(first, second);
			BitmapLineCalc lineB(first, third);
			
			Line(second, third,d,pat);
			for(i=int32(first.y+1);i<third.y;i++)
				Line( BPoint(lineA.GetX(i),i), BPoint(lineB.GetX(i),i),d,pat);
			Unlock();
			return;
		}
		
		// Normal case.
		// Calculate the y deltas for the two lines and we set the maximum for the
		// first loop to the lesser of the two so that we can change lines.
		int32 dy1=int32(second.y-first.y),
			dy2=int32(third.y-first.y),
			max;
		max=int32(first.y+MIN(dy1,dy2));
		
		BitmapLineCalc lineA(first, second);
		BitmapLineCalc lineB(first, third);
		BitmapLineCalc lineC(second, third);
		
		for(i=int32(first.y+1);i<max;i++)
			Line( BPoint(lineA.GetX(i),i), BPoint(lineB.GetX(i),i),d,pat);

		for(i=max; i<third.y; i++)
			Line( BPoint(lineC.GetX(i),i), BPoint(lineB.GetX(i),i),d,pat);
		
	}
	Unlock();
}

void BitmapDriver::SetThickPixel(int x, int y, int thick, RGBColor col)
{
	switch(_target->BitsPerPixel())
	{
		case 32:
		case 24:
			SetThickPixel32(x,y,thick,col.GetColor32());
			break;
		case 16:
		case 15:
			SetThickPixel16(x,y,thick,col.GetColor16());
			break;
		case 8:
			SetThickPixel8(x,y,thick,col.GetColor8());
			break;
		default:
			printf("Unknown pixel depth %d in SetThickPixel\n",_target->BitsPerPixel());
			break;
	}
}

void BitmapDriver::SetThickPixel32(int x, int y, int thick, rgb_color col)
{
	// Code courtesy of YNOP's SecondDriver
	union
	{
		uint8 bytes[4];
		uint32 word;
	}c1;
	
	c1.bytes[0]=col.blue; 
	c1.bytes[1]=col.green; 
	c1.bytes[2]=col.red; 
	c1.bytes[3]=col.alpha; 

	/*
	uint32 *bits=(uint32*)_target->Bits();
	*(bits + x + (y*_target->Width()))=c1.word;
	*/
	uint32 *bits=(uint32*)_target->Bits()+(x-thick/2)+(y-thick/2)*_target->BytesPerRow();
	int i,j;
	for (i=0; i<thick; i++)
	{
		for (j=0; j<thick; j++)
		{
			*(bits+j)=c1.word;
		}
		bits += _target->BytesPerRow();
	}
}

void BitmapDriver::SetThickPixel16(int x, int y, int thick, uint16 col)
{
// TODO: Implement
printf("SetThickPixel16 unimplemented\n");

}

void BitmapDriver::SetThickPixel8(int x, int y, int thick, uint8 col)
{
	// When the DisplayDriver API changes, we'll use the uint8 highcolor. Until then,
	// we'll use *pattern
  /*
	uint8 *bits=(uint8*)_target->Bits();
	*(bits + x + (y*_target->BytesPerRow()))=col;
	*/

	uint8 *bits=(uint8*)_target->Bits()+(x-thick/2)+(y-thick/2)*_target->BytesPerRow();
	int i,j;
	for (i=0; i<thick; i++)
	{
		for (j=0; j<thick; j++)
		{
			*(bits+j)=col;
		}
		bits += _target->BytesPerRow();
	}
}

void BitmapDriver::SetPixel(int x, int y, RGBColor col)
{
	switch(_target->BitsPerPixel())
	{
		case 32:
		case 24:
			SetPixel32(x,y,col.GetColor32());
			break;
		case 16:
		case 15:
			SetPixel16(x,y,col.GetColor16());
			break;
		case 8:
			SetPixel8(x,y,col.GetColor8());
			break;
		default:
			break;
	}
}

void BitmapDriver::SetPixel32(int x, int y, rgb_color col)
{
	// Code courtesy of YNOP's SecondDriver
	union
	{
		uint8 bytes[4];
		uint32 word;
	}c1;
	
	c1.bytes[0]=col.blue; 
	c1.bytes[1]=col.green; 
	c1.bytes[2]=col.red; 
	c1.bytes[3]=col.alpha; 

	uint32 *bits=(uint32*)_target->Bits();
	*(bits + x + (y*_target->BytesPerRow()))=c1.word;
}

void BitmapDriver::SetPixel16(int x, int y, uint16 col)
{
// TODO: Implement
printf("SetPixel16 unimplemented\n");

}

void BitmapDriver::SetPixel8(int x, int y, uint8 col)
{
	// When the DisplayDriver API changes, we'll use the uint8 highcolor. Until then,
	// we'll use *pattern
	uint8 *bits=(uint8*)_target->Bits();
	*(bits + x + (y*_target->BytesPerRow()))=col;

}

//! Empty
void BitmapDriver::SetMode(int32 space)
{
	// No need to reset a bitmap's color space
}


/*!
	\brief Called for all BView::StrokeArc calls
	\param r Rectangle enclosing the entire arc
	\param angle Starting angle for the arc in degrees
	\param span Span of the arc in degrees. Ending angle = angle+span.
	\param d Data structure containing any other data necessary for the call. Always non-NULL.
	\param pat 8-byte array containing the pattern to use. Always non-NULL.

	Bounds checking must be done in this call because only part of the arc may end up
	being clipped.
*/
void BitmapDriver::StrokeArc(BRect r, float angle, float span, LayerData *d, const Pattern &pat)
{
// TODO: Add pattern support
	float xc = (r.left+r.right)/2;
	float yc = (r.top+r.bottom)/2;
	float rx = r.Width()/2;
	float ry = r.Height()/2;
	int Rx2 = ROUND(rx*rx);
	int Ry2 = ROUND(ry*ry);
	int twoRx2 = 2*Rx2;
	int twoRy2 = 2*Ry2;
	int p;
	int x=0;
	int y = (int)ry;
	int px = 0;
	int py = twoRx2 * y;
	int startx, endx;
	int startQuad, endQuad;
	bool useQuad1, useQuad2, useQuad3, useQuad4;
	bool shortspan = false;
	int thick = (int)d->pensize;

	// Watch out for bozos giving us whacko spans
	if ( (span >= 360) || (span <= -360) )
	{
	  StrokeEllipse(r,d,pat);
	  return;
	}

	if ( span > 0 )
	{
		startQuad = (int)(angle/90)%4+1;
		endQuad = (int)((angle+span)/90)%4+1;
		startx = ROUND(.5*r.Width()*fabs(cos(angle*M_PI/180)));
		endx = ROUND(.5*r.Width()*fabs(cos((angle+span)*M_PI/180)));
	}
	else
	{
		endQuad = (int)(angle/90)%4+1;
		startQuad = (int)((angle+span)/90)%4+1;
		endx = ROUND(.5*r.Width()*fabs(cos(angle*M_PI/180)));
		startx = ROUND(.5*r.Width()*fabs(cos((angle+span)*M_PI/180)));
	}

	if ( startQuad != endQuad )
	{
		useQuad1 = (endQuad > 1) && (startQuad > endQuad);
		useQuad2 = ((startQuad == 1) && (endQuad > 2)) || ((startQuad > endQuad) && (endQuad > 2));
		useQuad3 = ((startQuad < 3) && (endQuad == 4)) || ((startQuad < 3) && (endQuad < startQuad));
		useQuad4 = (startQuad < 4) && (startQuad > endQuad);
	}
	else
	{
		if ( (span < 90) && (span > -90) )
		{
			useQuad1 = false;
			useQuad2 = false;
			useQuad3 = false;
			useQuad4 = false;
			shortspan = true;
		}
		else
		{
			useQuad1 = (startQuad != 1);
			useQuad2 = (startQuad != 2);
			useQuad3 = (startQuad != 3);
			useQuad4 = (startQuad != 4);
		}
	}

	if ( useQuad1 || 
	     (!shortspan && (((startQuad == 1) && (x <= startx)) || ((endQuad == 1) && (x >= endx)))) || 
	     (shortspan && (startQuad == 1) && (x <= startx) && (x >= endx)) ) 
		SetThickPixel(ROUND(xc+x),ROUND(yc-y),thick,d->highcolor);
	if ( useQuad2 || 
	     (!shortspan && (((startQuad == 2) && (x >= startx)) || ((endQuad == 2) && (x <= endx)))) || 
	     (shortspan && (startQuad == 2) && (x >= startx) && (x <= endx)) ) 
		SetThickPixel(ROUND(xc-x),ROUND(yc-y),thick,d->highcolor);
	if ( useQuad3 || 
	     (!shortspan && (((startQuad == 3) && (x <= startx)) || ((endQuad == 3) && (x >= endx)))) || 
	     (shortspan && (startQuad == 3) && (x <= startx) && (x >= endx)) ) 
		SetThickPixel(ROUND(xc-x),ROUND(yc+y),thick,d->highcolor);
	if ( useQuad4 || 
	     (!shortspan && (((startQuad == 4) && (x >= startx)) || ((endQuad == 4) && (x <= endx)))) || 
	     (shortspan && (startQuad == 4) && (x >= startx) && (x <= endx)) ) 
		SetThickPixel(ROUND(xc+x),ROUND(yc+y),thick,d->highcolor);

	p = ROUND (Ry2 - (Rx2 * ry) + (.25 * Rx2));
	while (px < py)
	{
		x++;
		px += twoRy2;
		if ( p < 0 )
			p += Ry2 + px;
		else
		{
			y--;
			py -= twoRx2;
			p += Ry2 + px - py;
		}

		if ( useQuad1 || 
		     (!shortspan && (((startQuad == 1) && (x <= startx)) || ((endQuad == 1) && (x >= endx)))) || 
		     (shortspan && (startQuad == 1) && (x <= startx) && (x >= endx)) ) 
			SetThickPixel(ROUND(xc+x),ROUND(yc-y),thick,d->highcolor);
		if ( useQuad2 || 
		     (!shortspan && (((startQuad == 2) && (x >= startx)) || ((endQuad == 2) && (x <= endx)))) || 
		     (shortspan && (startQuad == 2) && (x >= startx) && (x <= endx)) ) 
			SetThickPixel(ROUND(xc-x),ROUND(yc-y),thick,d->highcolor);
		if ( useQuad3 || 
		     (!shortspan && (((startQuad == 3) && (x <= startx)) || ((endQuad == 3) && (x >= endx)))) || 
		     (shortspan && (startQuad == 3) && (x <= startx) && (x >= endx)) ) 
			SetThickPixel(ROUND(xc-x),ROUND(yc+y),thick,d->highcolor);
		if ( useQuad4 || 
		     (!shortspan && (((startQuad == 4) && (x >= startx)) || ((endQuad == 4) && (x <= endx)))) || 
		     (shortspan && (startQuad == 4) && (x >= startx) && (x <= endx)) ) 
			SetThickPixel(ROUND(xc+x),ROUND(yc+y),thick,d->highcolor);
	}

	p = ROUND(Ry2*(x+.5)*(x+.5) + Rx2*(y-1)*(y-1) - Rx2*Ry2);
	while (y>0)
	{
		y--;
		py -= twoRx2;
		if (p>0)
			p += Rx2 - py;
		else
		{
			x++;
			px += twoRy2;
			p += Rx2 - py +px;
		}

		if ( useQuad1 || 
		     (!shortspan && (((startQuad == 1) && (x <= startx)) || ((endQuad == 1) && (x >= endx)))) || 
		     (shortspan && (startQuad == 1) && (x <= startx) && (x >= endx)) ) 
			SetThickPixel(ROUND(xc+x),ROUND(yc-y),thick,d->highcolor);
		if ( useQuad2 || 
		     (!shortspan && (((startQuad == 2) && (x >= startx)) || ((endQuad == 2) && (x <= endx)))) || 
		     (shortspan && (startQuad == 2) && (x >= startx) && (x <= endx)) ) 
			SetThickPixel(ROUND(xc-x),ROUND(yc-y),thick,d->highcolor);
		if ( useQuad3 || 
		     (!shortspan && (((startQuad == 3) && (x <= startx)) || ((endQuad == 3) && (x >= endx)))) || 
		     (shortspan && (startQuad == 3) && (x <= startx) && (x >= endx)) ) 
			SetThickPixel(ROUND(xc-x),ROUND(yc+y),thick,d->highcolor);
		if ( useQuad4 || 
		     (!shortspan && (((startQuad == 4) && (x >= startx)) || ((endQuad == 4) && (x <= endx)))) || 
		     (shortspan && (startQuad == 4) && (x >= startx) && (x <= endx)) ) 
			SetThickPixel(ROUND(xc+x),ROUND(yc+y),thick,d->highcolor);
	}
}

/*!
	\brief Called for all BView::StrokeBezier calls.
	\param pts 4-element array of BPoints in the order of start, end, and then the two control
	points. 
	\param d Data structure containing any other data necessary for the call. Always non-NULL.
	\param pat 8-byte array containing the pattern to use. Always non-NULL.

	Bounds checking must be done in this call.
*/
void BitmapDriver::StrokeBezier(BPoint *pts, LayerData *d, const Pattern &pat)
{
// TODO: Add pattern support
	double Ax, Bx, Cx, Dx;
	double Ay, By, Cy, Dy;
	int x, y;
	int lastx=-1, lasty=-1;
	double t;
	double dt = .001;
	double dt2, dt3;
	double X, Y, dx, ddx, dddx, dy, ddy, dddy;

	Ax = -pts[0].x + 3*pts[1].x - 3*pts[2].x + pts[3].x;
	Bx = 3*pts[0].x - 6*pts[1].x + 3*pts[2].x;
	Cx = -3*pts[0].x + 3*pts[1].x;
	Dx = pts[0].x;

	Ay = -pts[0].y + 3*pts[1].y - 3*pts[2].y + pts[3].y;
	By = 3*pts[0].y - 6*pts[1].y + 3*pts[2].y;
	Cy = -3*pts[0].y + 3*pts[1].y;
	Dy = pts[0].y;
	
	dt2 = dt * dt;
	dt3 = dt2 * dt;
	X = Dx;
	dx = Ax*dt3 + Bx*dt2 + Cx*dt;
	ddx = 6*Ax*dt3 + 2*Bx*dt2;
	dddx = 6*Ax*dt3;
	Y = Dy;
	dy = Ay*dt3 + By*dt2 + Cy*dt;
	ddy = 6*Ay*dt3 + 2*By*dt2;
	dddy = 6*Ay*dt3;

	lastx = -1;
	lasty = -1;

	for (t=0; t<=1; t+=dt)
	{
		x = ROUND(X);
		y = ROUND(Y);
		if ( (x!=lastx) || (y!=lasty) )
			SetThickPixel(x,y,ROUND(d->pensize),d->highcolor);
		lastx = x;
		lasty = y;

		X += dx;
		dx += ddx;
		ddx += dddx;
		Y += dy;
		dy += ddy;
		ddy += dddy;
	}
}

/*!
	\brief Called for all BView::StrokeEllipse calls
	\param r BRect enclosing the ellipse to be drawn.
	\param d Data structure containing any other data necessary for the call. Always non-NULL.
	\param pat 8-byte array containing the pattern to use. Always non-NULL.

	Bounds checking must be done in this call because only part of the ellipse may end up
	being clipped.
*/
void BitmapDriver::StrokeEllipse(BRect r, LayerData *ldata, const Pattern &pat)
{
// Ellipse code shamelessly stolen from the graphics library gd v2.0.1 and bolted on
// to support our API
	long d, b_sq, b_sq_4, b_sq_6;
	long a_sq, a_sq_4, a_sq_6;
	int x, y, switchem;
	long lsqrt (long);
	int pix, half, pstart;
	int32 thick = (int32)ldata->pensize;
	
	half = thick / 2;
	int32 w=int32(r.Width()/2),
		h=int32(r.Height()/2);
	int32 cx=(int32)r.left+w;
	int32 cy=(int32)r.top+h;
	
	d = 2 * (long) h *h + (long) w *w - 2 * (long) w *w * h;
	b_sq = (long) h *h;
	b_sq_4 = 4 * (long) h *h;
	b_sq_6 = 6 * (long) h *h;
	a_sq = (long) w *w;
	a_sq_4 = 4 * (long) w *w;
	a_sq_6 = 6 * (long) w *w;
	
	x = 0;
	y = -h;
//	switchem = a_sq / lsqrt (a_sq + b_sq);
	switchem = a_sq / (int32)sqrt(a_sq + b_sq);

	while (x <= switchem)
	{
		pstart = y - half;
		for (pix = pstart; pix < pstart + thick; pix++)
		{
			SetPixel(cx + x, cy + pix, ldata->highcolor);
			SetPixel(cx - x, cy + pix, ldata->highcolor);
			SetPixel(cx + x, cy - pix, ldata->highcolor);
			SetPixel(cx - x, cy - pix, ldata->highcolor);
		}
		if (d < 0)
			d += b_sq_4 * x++ + b_sq_6;
		else
			d += b_sq_4 * x++ + b_sq_6 + a_sq_4 * (++y);
	}
	
	/* Middlesplat!
	** Go a little further if the thickness is not nominal...
	*/
	if (thick > 1)
	{
		int xp = x;
		int yp = y;
		int dp = d;
		int thick2 = thick + 2;
		int half2 = half + 1;
		
		while (xp <= switchem + half)
		{
			pstart = yp - half2;
			for (pix = pstart; pix < pstart + thick2; pix++)
			{
				SetPixel(cx + xp, cy + pix, ldata->highcolor);
				SetPixel(cx - xp, cy + pix, ldata->highcolor);
				SetPixel(cx + xp, cy - pix, ldata->highcolor);
				SetPixel(cx - xp, cy - pix, ldata->highcolor);
			}
			if (dp < 0)
				dp += b_sq_4 * xp++ + b_sq_6;
			else
				dp += b_sq_4 * xp++ + b_sq_6 + a_sq_4 * (++yp);
		}
	}
	
	d += -2 * (long) b_sq + 2 * (long) a_sq - 2 * (long) b_sq *(x - 1) + 2 * (long) a_sq *(y - 1);
	
	while (y <= 0)
	{
		pstart = x - half;
		for (pix = pstart; pix < pstart + thick; pix++)
		{
			SetPixel (cx + pix, cy + y, ldata->highcolor);
			SetPixel (cx - pix, cy + y, ldata->highcolor);
			SetPixel (cx + pix, cy - y, ldata->highcolor);
			SetPixel (cx - pix, cy - y, ldata->highcolor);
		}
		
		if (d < 0)
			d += a_sq_4 * y++ + a_sq_6 + b_sq_4 * (++x);
		else
			d += a_sq_4 * y++ + a_sq_6;
	}

}

/*!
	\brief Draws a line. Really.
	\param start Starting point
	\param end Ending point
	\param d Data structure containing any other data necessary for the call. Always non-NULL.
	\param pat 8-byte array containing the pattern to use. Always non-NULL.
	
	The endpoints themselves are guaranteed to be in bounds, but clipping for lines with
	a thickness greater than 1 will need to be done.
*/
void BitmapDriver::StrokeLine(BPoint start, BPoint end, LayerData *d, const Pattern &pat)
{
	Lock();
	if(_target)
	{
		// Courtesy YNOP's SecondDriver with minor changes by DW
		int oct=0;
		int xoff=(int32)end.x;
		int yoff=(int32)end.y; 
		int32 x2=(int32)start.x-xoff;
		int32 y2=(int32)start.y-yoff; 
		int32 x1=0;
		int32 y1=0;
		if(y2<0){ y2=-y2; oct+=4; }//bit2=1
		if(x2<0){ x2=-x2; oct+=2;}//bit1=1
		if(x2<y2){ int t=x2; x2=y2; y2=t; oct+=1;}//bit0=1
		int x=x1,
			y=y1,
			sum=x2-x1,
			Dx=2*(x2-x1),
			Dy=2*(y2-y1);
		for(int i=0; i <= x2-x1; i++)
		{
			switch(oct)
			{
				case 0:SetPixel(( x)+xoff,( y)+yoff,d->highcolor);break;
				case 1:SetPixel(( y)+xoff,( x)+yoff,d->highcolor);break;
	 			case 3:SetPixel((-y)+xoff,( x)+yoff,d->highcolor);break;
				case 2:SetPixel((-x)+xoff,( y)+yoff,d->highcolor);break;
				case 6:SetPixel((-x)+xoff,(-y)+yoff,d->highcolor);break;
				case 7:SetPixel((-y)+xoff,(-x)+yoff,d->highcolor);break;
				case 5:SetPixel(( y)+xoff,(-x)+yoff,d->highcolor);break;
				case 4:SetPixel(( x)+xoff,(-y)+yoff,d->highcolor);break;
			}
			x++;
			sum-=Dy;
			if(sum < 0)
			{
				y++;
				sum += Dx;
			}
		}
	}
	Unlock();
}

/*!
	\brief Called for all BView::StrokePolygon calls
	\param ptlist Array of BPoints defining the polygon.
	\param numpts Number of points in the BPoint array.
	\param rect Rectangle which contains the polygon
	\param d Data structure containing any other data necessary for the call. Always non-NULL.
	\param pat 8-byte array containing the pattern to use. Always non-NULL.

	The points in the array are not guaranteed to be within the framebuffer's 
	coordinate range.
*/
void BitmapDriver::StrokePolygon(BPoint *ptlist, int32 numpts, BRect rect, LayerData *d, const Pattern &pat, bool is_closed)
{
	Lock();
	if(_target)
	{
		for(int32 i=0; i<(numpts-1); i++)
			Line(ptlist[i],ptlist[i+1],d,pat);
	
		if(is_closed)
			Line(ptlist[numpts-1],ptlist[0],d,pat);
	}
	Unlock();
}

/*!
	\brief Called for all BView::StrokeRect calls
	\param r BRect to be filled. Guaranteed to be in the frame buffer's coordinate space
	\param d Data structure containing any other data necessary for the call. Always non-NULL.
	\param pat 8-byte array containing the pattern to use. Always non-NULL.

*/
void BitmapDriver::StrokeRect(BRect r, LayerData *d, const Pattern &pat)
{
	Lock();
	if(_target)
	{
		Line(r.LeftTop(),r.RightTop(),d,pat);
		Line(r.RightTop(),r.RightBottom(),d,pat);
		Line(r.RightBottom(),r.LeftBottom(),d,pat);
		Line(r.LeftTop(),r.LeftBottom(),d,pat);
	}
	Unlock();
}

/*!
	\brief Called for all BView::StrokeRoundRect calls
	\param r The rect itself
	\param xrad X radius of the corner arcs
	\param yrad Y radius of the corner arcs
	\param d Data structure containing any other data necessary for the call. Always non-NULL.
	\param pat 8-byte array containing the pattern to use. Always non-NULL.

	Bounds checking must be done in this call because only part of the roundrect may end 
	up being clipped.
*/
void BitmapDriver::StrokeRoundRect(BRect r, float xrad, float yrad, LayerData *d, const Pattern &pat)
{
	float hLeft, hRight;
	float vTop, vBottom;
	float bLeft, bRight, bTop, bBottom;
	Lock();
	PatternHandler pattern(pat);
	pattern.SetColors(d->highcolor, d->lowcolor);

	hLeft = r.left + xrad;
	hRight = r.right - xrad;
	vTop = r.top + yrad;
	vBottom = r.bottom - yrad;
	bLeft = hLeft + xrad;
	bRight = hRight -xrad;
	bTop = vTop + yrad;
	bBottom = vBottom - yrad;
	StrokeArc(BRect(bRight, r.top, r.right, bTop), 0, 90, d, pat);
	Line(BPoint(hLeft,r.top), BPoint(hRight, r.top), d, pat);
	
	StrokeArc(BRect(r.left,r.top,bLeft,bTop), 90, 90, d, pat);
	Line(BPoint(r.left,vTop),BPoint(r.left,vBottom),d,pat);

	StrokeArc(BRect(r.left,bBottom,bLeft,r.bottom), 180, 90, d, pat);
	Line(BPoint(hLeft,r.bottom), BPoint(hRight, r.bottom), d, pat);

	StrokeArc(BRect(bRight,bBottom,r.right,r.bottom), 270, 90, d, pat);
	StrokeLine(BPoint(r.right,vBottom),BPoint(r.right,vTop),d,pat);
	Unlock();
}

/*!
	\brief Called for all BView::StrokeTriangle calls
	\param pts Array of 3 BPoints. Always non-NULL.
	\param r BRect enclosing the triangle. While it will definitely enclose the triangle,
	it may not be within the frame buffer's bounds.
	\param d Data structure containing any other data necessary for the call. Always non-NULL.
	\param pat 8-byte array containing the pattern to use. Always non-NULL.

	Bounds checking must be done in this call because only part of the triangle may end 
	up being clipped.
*/
void BitmapDriver::StrokeTriangle(BPoint *pts, BRect r, LayerData *d, const Pattern &pat)
{
	Lock();
	if(_target)
	{
		Line(pts[0],pts[1],d,pat);
		Line(pts[1],pts[2],d,pat);
		Line(pts[2],pts[0],d,pat);
	}
	Unlock();
}

void BitmapDriver::SetPixelPattern(int x, int y, uint8 *pattern, uint8 patternindex)
{
	// This function is designed to add pattern support to this thing. Should be
	// inlined later to add speed lost in the multiple function calls.
	if(patternindex>32)
		return;

	if(_target)
	{
//		uint64 *p64=(uint64*)pattern;

		// check for transparency in mask. If transparent, we can quit here
		
//		bool transparent_bit=
//			( *p64 & ~((uint64)2 << (32-patternindex)))?true:false;

//		bool highcolor_bit=
//			( *p64 & ~((uint64)2 << (64-patternindex)))?true:false;
			
		switch(_target->BitsPerPixel())
		{	
			case 32:
			case 24:
			{
				
				break;
			}
			case 16:
			case 15:
			{
				break;
			}
			case 8:
			{
				break;
			}
			default:
			{
				break;
			}
		}
	}
}

void BitmapDriver::Line(BPoint start, BPoint end, LayerData *d, const Pattern &pat)
{
	// Internal function which is called from within other functions
	
	// Courtesy YNOP's SecondDriver with minor changes by DW
	int oct=0;
	int xoff=(int32)end.x;
	int yoff=(int32)end.y; 
	int32 x2=(int32)start.x-xoff;
	int32 y2=(int32)start.y-yoff; 
	int32 x1=0;
	int32 y1=0;
	if(y2<0){ y2=-y2; oct+=4; }//bit2=1
	if(x2<0){ x2=-x2; oct+=2;}//bit1=1
	if(x2<y2){ int t=x2; x2=y2; y2=t; oct+=1;}//bit0=1
	int x=x1,
		y=y1,
		sum=x2-x1,
		Dx=2*(x2-x1),
		Dy=2*(y2-y1);
	for(int i=0; i <= x2-x1; i++)
	{
		switch(oct)
		{
			case 0:SetPixel(( x)+xoff,( y)+yoff,d->highcolor);break;
			case 1:SetPixel(( y)+xoff,( x)+yoff,d->highcolor);break;
 			case 3:SetPixel((-y)+xoff,( x)+yoff,d->highcolor);break;
			case 2:SetPixel((-x)+xoff,( y)+yoff,d->highcolor);break;
			case 6:SetPixel((-x)+xoff,(-y)+yoff,d->highcolor);break;
			case 7:SetPixel((-y)+xoff,(-x)+yoff,d->highcolor);break;
			case 5:SetPixel(( y)+xoff,(-x)+yoff,d->highcolor);break;
			case 4:SetPixel(( x)+xoff,(-y)+yoff,d->highcolor);break;
		}
		x++;
		sum-=Dy;
		if(sum < 0)
		{
			y++;
			sum += Dx;
		}
	}
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
	\brief Draws a horizontal line
	\param x1 The first x coordinate (guaranteed to be in bounds)
	\param x2 The second x coordinate (guaranteed to be in bounds)
	\param y The y coordinate (guaranteed to be in bounds)
	\param pat The PatternHandler which detemines pixel colors
*/
void BitmapDriver::HLine(int32 x1, int32 x2, int32 y, PatternHandler *pat)
{
/*	// TODO: Finish porting over from AccelerantDriver
	int x;
	if ( x1 > x2 )
	{
		x = x2;
		x2 = x1;
		x1 = x;
	}
	switch (mDisplayMode.space)
	{
		case B_CMAP8:
		case B_GRAY8:
			{
				uint8 *fb = (uint8 *)mFrameBufferConfig.frame_buffer + y*mFrameBufferConfig.bytes_per_row;
				for (x=x1; x<=x2; x++)
					fb[x] = pat->GetColor(x,y).GetColor8();
			} break;
		case B_RGB16_BIG:
		case B_RGB16_LITTLE:
		case B_RGB15_BIG:
		case B_RGBA15_BIG:
		case B_RGB15_LITTLE:
		case B_RGBA15_LITTLE:
			{
				uint16 *fb = (uint16 *)((uint8 *)mFrameBufferConfig.frame_buffer + y*mFrameBufferConfig.bytes_per_row);
				for (x=x1; x<=x2; x++)
					fb[x] = pat->GetColor(x,y).GetColor16();
			} break;
		case B_RGB32_BIG:
		case B_RGBA32_BIG:
		case B_RGB32_LITTLE:
		case B_RGBA32_LITTLE:
			{
				uint32 *fb = (uint32 *)((uint8 *)mFrameBufferConfig.frame_buffer + y*mFrameBufferConfig.bytes_per_row);
				rgb_color color;
				for (x=x1; x<=x2; x++)
				{
					color = pat->GetColor(x,y).GetColor32();
					fb[x] = (color.alpha << 24) | (color.red << 16) | (color.green << 8) | (color.blue);
				}
			} break;
		default:
			printf("Error: Unknown color space\n");
	}
*/
}
