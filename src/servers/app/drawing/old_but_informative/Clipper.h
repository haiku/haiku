//------------------------------------------------------------------------------
//	Copyright (c) 2001-2003, Haiku, Inc.
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
//	File Name:		Clipper.h
//	Author:			Gabe Yoder <gyoder@stny.rr.com>
//	Description:		A wrapper which translates drawing commands
//				into appropriately clipped drawing commands.
//  
//------------------------------------------------------------------------------
#ifndef _CLIPPER_H_
#define _CLIPPER_H_

#include "DisplayDriver.h"
#include "Region.h"
#include "Picture.h"

class Clipper
{
public:
	Clipper(DisplayDriver* driver);
	~Clipper();

	void DrawBitmap(BRegion* clip_reg, ServerBitmap *bmp, BRect src, BRect dest, LayerData *d);
	void DrawString(BRegion* clip_reg, const char *string, int32 length, BPoint pt, LayerData *d, escapement_delta *edelta=NULL);


	void FillArc(const BRect r, float angle, float span, RGBColor& color, BRegion* clip_reg=NULL);
	void FillArc(const BRect r, float angle, float span, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color, BRegion* clip_reg=NULL);
	void FillBezier(BPoint *pts, RGBColor& color, BRegion* clip_reg=NULL);
	void FillBezier(BPoint *pts, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color, BRegion* clip_reg=NULL);
	void FillEllipse(BRect r, RGBColor& color, BRegion* clip_reg=NULL);
	void FillEllipse(BRect r, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color, BRegion* clip_reg=NULL);
	void FillPolygon(BPoint *ptlist, int32 numpts, RGBColor& color, BRegion* clip_reg=NULL);
	void FillPolygon(BPoint *ptlist, int32 numpts, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color, BRegion* clip_reg=NULL);
	void FillRect(const BRect r, RGBColor& color, BRegion* clip_reg=NULL);
	void FillRect(const BRect r, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color, BRegion* clip_reg=NULL);
	void FillRegion(BRegion& r, RGBColor& color, BRegion* clip_reg=NULL);
	void FillRegion(BRegion& r, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color, BRegion* clip_reg=NULL);
	void FillRoundRect(BRect r, float xrad, float yrad, RGBColor& color, BRegion* clip_reg=NULL);
	void FillRoundRect(BRect r, float xrad, float yrad, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color, BRegion* clip_reg=NULL);
	void FillTriangle(BPoint *pts, RGBColor& color, BRegion* clip_reg=NULL);
	void FillTriangle(BPoint *pts, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color, BRegion* clip_reg=NULL);

	void StrokeArc(BRect r, float angle, float span, float pensize, RGBColor& color, BRegion* clip_reg=NULL);
	void StrokeArc(BRect r, float angle, float span, float pensize, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color, BRegion* clip_reg=NULL);
	void StrokeBezier(BPoint *pts, float pensize, RGBColor& color, BRegion* clip_reg=NULL);
	void StrokeBezier(BPoint *pts, float pensize, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color, BRegion* clip_reg=NULL);
	void StrokeEllipse(BRect r, float pensize, RGBColor& color, BRegion* clip_reg=NULL);
	void StrokeEllipse(BRect r, float pensize, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color, BRegion* clip_reg=NULL);
	void StrokeLine(BPoint start, BPoint end, float pensize, RGBColor& color, BRegion* clip_reg=NULL);
	void StrokeLine(BPoint start, BPoint end, float pensize, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color, BRegion* clip_reg=NULL);
	void StrokePolygon(BPoint *ptlist, int32 numpts, float pensize, RGBColor& color, bool is_closed=true, BRegion* clip_reg=NULL);
	void StrokePolygon(BPoint *ptlist, int32 numpts, float pensize, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color, bool is_closed=true, BRegion* clip_reg=NULL);
	void StrokeRect(BRect r, float pensize, RGBColor& color, BRegion* clip_reg=NULL);
	void StrokeRect(BRect r, float pensize, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color, BRegion* clip_reg=NULL);
	void StrokeRegion(BRegion& r, float pensize, RGBColor& color, BRegion* clip_reg=NULL);
	void StrokeRegion(BRegion& r, float pensize, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color, BRegion* clip_reg=NULL);
	void StrokeRoundRect(BRect r, float xrad, float yrad, float pensize, RGBColor& color, BRegion* clip_reg=NULL);
	void StrokeRoundRect(BRect r, float xrad, float yrad, float pensize, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color, BRegion* clip_reg=NULL);
	void StrokeTriangle(BPoint *pts, float pensize, RGBColor& color, BRegion* clip_reg=NULL);
	void StrokeTriangle(BPoint *pts, float pensize, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color, BRegion* clip_reg=NULL);

	void StrokeLineArray(BPoint *pts, int32 numlines, float pensize, RGBColor *colors, BRegion* clip_reg=NULL);
private:
	DisplayDriver* fDriver;
};

#endif
