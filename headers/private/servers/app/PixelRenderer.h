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
//	File Name:		PixelRenderer.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//
//	Description:	Base class for pixel plotting renderers
//					Based on concepts from the Anti-Grain Geometry vector gfx library
//------------------------------------------------------------------------------
#ifndef PIXEL_RENDERER
#define PIXEL_RENDERER

#include "GraphicsBuffer.h"
#include "GraphicsDefs.h"
#include "RGBColor.h"

class IPoint;

class PixelRenderer
{
public:
	PixelRenderer(GraphicsBuffer &buffer);
	virtual ~PixelRenderer(void);
	
	virtual color_space ColorSpace(void) { return B_NO_COLOR_SPACE; }
	
	virtual RGBColor GetPixel(const IPoint &pt);
	virtual void PutPixel(const IPoint &pt, RGBColor &c);
	virtual void PutHLine(const IPoint &pt, const uint32 length, RGBColor &c);
	virtual void PutVLine(const IPoint &pt, const uint32 length, RGBColor &c);
	
	virtual void SetBuffer(GraphicsBuffer &buffer) { fBuffer=&buffer; }
	GraphicsBuffer *GetBuffer(void) const { return fBuffer; }

private:
	GraphicsBuffer *fBuffer;
};

class PixelRendererRGBA32 : public PixelRenderer
{
public:
	PixelRendererRGBA32(GraphicsBuffer &buffer);
	virtual ~PixelRendererRGBA32(void);
	
	virtual color_space ColorSpace(void) { return B_RGBA32; }
	
	virtual RGBColor GetPixel(const IPoint &pt);
	virtual void PutPixel(const IPoint &pt, RGBColor &c);
	virtual void PutHLine(const IPoint &pt, const uint32 length, RGBColor &c);
	virtual void PutVLine(const IPoint &pt, const uint32 length, RGBColor &c);
};

class PixelRendererRGB16 : public PixelRenderer
{
public:
	PixelRendererRGB16(GraphicsBuffer &buffer);
	virtual ~PixelRendererRGB16(void);
	
	virtual color_space ColorSpace(void) { return B_RGB16; }
	
	virtual RGBColor GetPixel(const IPoint &pt);
	virtual void PutPixel(const IPoint &pt, RGBColor &c);
	virtual void PutHLine(const IPoint &pt, const uint32 length, RGBColor &c);
	virtual void PutVLine(const IPoint &pt, const uint32 length, RGBColor &c);
};

class PixelRendererRGBA15 : public PixelRenderer
{
public:
	PixelRendererRGBA15(GraphicsBuffer &buffer);
	virtual ~PixelRendererRGBA15(void);
	
	virtual color_space ColorSpace(void) { return B_RGBA15; }
	
	virtual RGBColor GetPixel(const IPoint &pt);
	virtual void PutPixel(const IPoint &pt, RGBColor &c);
	virtual void PutHLine(const IPoint &pt, const uint32 length, RGBColor &c);
	virtual void PutVLine(const IPoint &pt, const uint32 length, RGBColor &c);
};

class PixelRendererCMAP8 : public PixelRenderer
{
public:
	PixelRendererCMAP8(GraphicsBuffer &buffer);
	virtual ~PixelRendererCMAP8(void);
	
	virtual color_space ColorSpace(void) { return B_CMAP8; }
	
	virtual RGBColor GetPixel(const IPoint &pt);
	virtual void PutPixel(const IPoint &pt, RGBColor &c);
	virtual void PutHLine(const IPoint &pt, const uint32 length, RGBColor &c);
	virtual void PutVLine(const IPoint &pt, const uint32 length, RGBColor &c);
};

// TODO: These inlines probably should go in ColorUtils
inline uint16 MakeRGB16Color(uint8 r, uint8 g, uint8 b)
{
	return (uint16)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

// TODO: Doublecheck to make sure the shifting is correct in MakeRGBA15Color
inline uint16 MakeRGBA15Color(uint8 r, uint8 g, uint8 b, bool visible=true)
{
	return (uint16)(((r & 0xF8) << 7) | ((g & 0xF8) << 2) | (b >> 3));
}

#endif
