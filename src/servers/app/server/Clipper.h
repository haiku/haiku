//------------------------------------------------------------------------------
//	Copyright (c) 2001-2003, OpenBeOS
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

	void FillArc(BRegion* clip_reg, BRect r, float angle, float span, LayerData *d, const Pattern &pat);
	void FillBezier(BRegion* clip_reg, BPoint *pts, LayerData *d, const Pattern &pat);
	void FillEllipse(BRegion* clip_reg, BRect r, LayerData *d, const Pattern &pat);
	void FillPolygon(BRegion* clip_reg, BPoint *ptlist, int32 numpts, BRect rect, LayerData *d, const Pattern &pat);
	void FillRect(BRegion* clip_reg, BRect r, LayerData *d, const Pattern &pat);
	void FillRegion(BRegion* clip_reg, BRegion* r, LayerData *d, const Pattern &pat);
	void FillRoundRect(BRegion* clip_reg, BRect r, float xrad, float yrad, LayerData *d, const Pattern &pat);
//	void FillShape(BRegion* clip_reg, SShape *sh, LayerData *d, const Pattern &pat);
	void FillTriangle(BRegion* clip_reg, BPoint *pts, BRect r, LayerData *d, const Pattern &pat);

	void StrokeArc(BRegion* clip_reg, BRect r, float angle, float span, LayerData *d, const Pattern &pat);
	void StrokeBezier(BRegion* clip_reg, BPoint *pts, LayerData *d, const Pattern &pat);
	void StrokeEllipse(BRegion* clip_reg, BRect r, LayerData *d, const Pattern &pat);
	void StrokeLine(BRegion* clip_reg, BPoint start, BPoint end, LayerData *d, const Pattern &pat);
	void StrokePolygon(BRegion* clip_reg, BPoint *ptlist, int32 numpts, BRect rect, LayerData *d, const Pattern &pat, bool is_closed=true);
	void StrokeRect(BRegion* clip_reg, BRect r, LayerData *d, const Pattern &pat);
	void StrokeRoundRect(BRegion* clip_reg, BRect r, float xrad, float yrad, LayerData *d, const Pattern &pat);
//	void StrokeShape(BRegion* clip_reg, SShape *sh, LayerData *d, const Pattern &pat);
	void StrokeTriangle(BRegion* clip_reg, BPoint *pts, BRect r, LayerData *d, const Pattern &pat);
	void StrokeLineArray(BRegion* clip_reg, BPoint *pts, int32 numlines, RGBColor *colors, LayerData *d);
private:
	DisplayDriver* fDriver;
};

#endif
