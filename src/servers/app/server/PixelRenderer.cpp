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
//	File Name:		PixelRenderer.h
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//
//	Description:	Base class for pixel plotting renderers
//					Based on concepts from the Anti-Grain Geometry vector gfx library
//------------------------------------------------------------------------------
#include "PixelRenderer.h"
#include <string.h>
#include "IPoint.h"

PixelRenderer::PixelRenderer(GraphicsBuffer &buffer)
{
	fBuffer=&buffer;
}

PixelRenderer::~PixelRenderer(void)
{
}

RGBColor PixelRenderer::GetPixel(const IPoint &pt)
{
	RGBColor c(0,0,0,0);
	return c;
}

void PixelRenderer::PixelRenderer::PutPixel(const IPoint &pt, RGBColor &c)
{
}

void PixelRenderer::PutHLine(const IPoint &pt, const uint32 length, RGBColor &c)
{
}

void PixelRenderer::PutVLine(const IPoint &pt, const uint32 length, RGBColor &c)
{
}

//----------------------------------------------------------------------------------------
//
//	PixelRendererRGBA32:	32-Bit color handler
//
//----------------------------------------------------------------------------------------

PixelRendererRGBA32::PixelRendererRGBA32(GraphicsBuffer &buffer)
 : PixelRenderer(buffer)
{
}

PixelRendererRGBA32::~PixelRendererRGBA32(void)
{
}

RGBColor PixelRendererRGBA32::GetPixel(const IPoint &pt)
{
	uint8 *pixel=GetBuffer()->RowAt(pt.y) + (pt.x << 2);
	return RGBColor(pixel[2],pixel[1],pixel[0],pixel[3]);
}

void PixelRendererRGBA32::PutPixel(const IPoint &pt, RGBColor &color)
{
	rgb_color c=color.GetColor32();

	uint8 *pixel=GetBuffer()->RowAt(pt.y) + (pt.x << 2);
	
	pixel[0]=c.blue;
	pixel[1]=c.green;
	pixel[2]=c.red;
	pixel[3]=c.alpha;
}

void PixelRendererRGBA32::PutHLine(const IPoint &pt, const uint32 length, RGBColor &color)
{
	rgb_color c=color.GetColor32();
	
	uint32 *pixel32=(uint32*)GetBuffer()->RowAt(pt.y);
	uint8 *pixel=(uint8*)&(pixel32[pt.x]);
	
	pixel[0]=c.blue;
	pixel[1]=c.green;
	pixel[2]=c.red;
	pixel[3]=c.alpha;
	int32 color32=*pixel32;
	
	uint32 end=pt.x+length;
	
	for(uint32 i=pt.x+1; i<end; i++)
		pixel32[i]=color32;
}

void PixelRendererRGBA32::PutVLine(const IPoint &pt, const uint32 length, RGBColor &color)
{
	rgb_color c=color.GetColor32();
	
	uint32 *pixel32=(uint32*)GetBuffer()->RowAt(pt.y);
	uint8 *pixel=(uint8*)&(pixel32[pt.x]);
	
	pixel[0]=c.blue;
	pixel[1]=c.green;
	pixel[2]=c.red;
	pixel[3]=c.alpha;
	int32 color32=*pixel32;
	
	uint32 end=pt.y+length;
	
	for(uint32 i=pt.y+1; i<end; i++)
	{
		pixel32=(uint32*)GetBuffer()->RowAt(i);
		pixel32[pt.x]=color32;
	}
}

//----------------------------------------------------------------------------------------
//
//	PixelRendererRGB16:	16-Bit color handler -- 565 space
//
//----------------------------------------------------------------------------------------

PixelRendererRGB16::PixelRendererRGB16(GraphicsBuffer &buffer)
 : PixelRenderer(buffer)
{
}

PixelRendererRGB16::~PixelRendererRGB16(void)
{
}

// TODO: Double check the values in RGB16 renderer to make sure they are correct for little endian

RGBColor PixelRendererRGB16::GetPixel(const IPoint &pt)
{
	uint16 pixel=((uint16*)GetBuffer()->RowAt(pt.y))[pt.x];
	return RGBColor( (pixel<<3) & 248, (pixel>>3) & 252, (pixel>>8) & 248);
}

void PixelRendererRGB16::PutPixel(const IPoint &pt, RGBColor &c)
{
	((uint16*)GetBuffer()->RowAt(pt.y))[pt.x]=c.GetColor16();
}

void PixelRendererRGB16::PutHLine(const IPoint &pt, const uint32 length, RGBColor &color)
{
	uint16 *pixel16=(uint16*)GetBuffer()->RowAt(pt.y);
	uint32 end=pt.x+length;
	
	for(uint32 i=pt.x; i<end; i++)
		pixel16[i]=color.GetColor16();
}

void PixelRendererRGB16::PutVLine(const IPoint &pt, const uint32 length, RGBColor &color)
{
	uint32 end=pt.y+length;
	uint16 *index;
	
	for(uint32 i=pt.y; i<end; i++)
	{
		index=(uint16*)GetBuffer()->RowAt(i);
		index[pt.x]=color.GetColor16();
	}
}

//----------------------------------------------------------------------------------------
//
//	PixelRendererRGBA15:	15-Bit color handler -- 5551 space
//
//----------------------------------------------------------------------------------------

PixelRendererRGBA15::PixelRendererRGBA15(GraphicsBuffer &buffer)
 : PixelRenderer(buffer)
{
}

PixelRendererRGBA15::~PixelRendererRGBA15(void)
{
}

// TODO: Double check the values in RGBA15 renderer to make sure they are correct for little endian

RGBColor PixelRendererRGBA15::GetPixel(const IPoint &pt)
{
	uint16 pixel=((uint16*)GetBuffer()->RowAt(pt.y))[pt.x];
	return RGBColor( 	(pixel<<3) & 248, 
						(pixel>>3) & 252,
						(pixel>>7) & 248,
						((pixel>>8) & 128)?255:0 );
}

void PixelRendererRGBA15::PutPixel(const IPoint &pt, RGBColor &color)
{
	((uint16*)GetBuffer()->RowAt(pt.y))[pt.x]=color.GetColor15();
}

void PixelRendererRGBA15::PutHLine(const IPoint &pt, const uint32 length, RGBColor &color)
{
	uint16 *pixel16=(uint16*)GetBuffer()->RowAt(pt.y);
	uint32 end=pt.x+length;
	
	for(uint32 i=pt.x; i<end; i++)
		pixel16[i]=color.GetColor15();
}

void PixelRendererRGBA15::PutVLine(const IPoint &pt, const uint32 length, RGBColor &color)
{
	uint32 end=pt.y+length;
	uint16 *index;
	
	for(uint32 i=pt.y; i<end; i++)
	{
		index=(uint16*)GetBuffer()->RowAt(i);
		index[pt.x]=color.GetColor15();
	}
}

//----------------------------------------------------------------------------------------
//
//	PixelRendererCMAP8:	8-Bit color handler
//
//----------------------------------------------------------------------------------------

PixelRendererCMAP8::PixelRendererCMAP8(GraphicsBuffer &buffer)
 : PixelRenderer(buffer)
{
}

PixelRendererCMAP8::~PixelRendererCMAP8(void)
{
}

// TODO: Double check the values in CMAP8 renderer to make sure they are correct for little endian

RGBColor PixelRendererCMAP8::GetPixel(const IPoint &pt)
{
	uint8 pixel=((uint8*)GetBuffer()->RowAt(pt.y))[pt.x];
	return RGBColor(pixel);
}

void PixelRendererCMAP8::PutPixel(const IPoint &pt, RGBColor &color)
{
	(GetBuffer()->RowAt(pt.y))[pt.x]=color.GetColor8();
}

void PixelRendererCMAP8::PutHLine(const IPoint &pt, const uint32 length, RGBColor &color)
{
	uint8 *index=GetBuffer()->RowAt(pt.y)+pt.x;
	memset(index,color.GetColor8(),length);
}

void PixelRendererCMAP8::PutVLine(const IPoint &pt, const uint32 length, RGBColor &color)
{
	uint8 *index=GetBuffer()->RowAt(pt.y)+pt.x;
	uint32 end=pt.y+length;
	
	for(uint32 i=pt.y; i<end; i++)
	{
		*index=color.GetColor8();
		index+=GetBuffer()->BytesPerRow();
	}
}
