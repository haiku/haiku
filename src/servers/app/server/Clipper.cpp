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
//	File Name:		Clipper.cpp
//	Author:			Gabe Yoder <gyoder@stny.rr.com>
//	Description:		A wrapper which translates drawing commands
//				into appropriately clipped drawing commands.
//  
//------------------------------------------------------------------------------
#include "Clipper.h"

Clipper::Clipper(DisplayDriver* driver)
{
	fDriver = driver;
}

Clipper::~Clipper()
{
}

void Clipper::DrawBitmap(BRegion* clip_reg, ServerBitmap *bmp, BRect src, BRect dest, LayerData *d)
{
	fDriver->DrawBitmap(bmp,src,dest,d);
}

void Clipper::DrawString(BRegion* clip_reg, const char *string, int32 length, BPoint pt, LayerData *d, escapement_delta *edelta)
{
	fDriver->DrawString(string,length,pt,d,edelta);
}

void Clipper::FillArc(BRegion* clip_reg, BRect r, float angle, float span, LayerData *d, const Pattern &pat)
{
	fDriver->FillArc(r,angle,span,d,pat);
}

void Clipper::FillBezier(BRegion* clip_reg, BPoint *pts, LayerData *d, const Pattern &pat)
{
	fDriver->FillBezier(pts,d,pat);
}

void Clipper::FillEllipse(BRegion* clip_reg, BRect r, LayerData *d, const Pattern &pat)
{
	fDriver->FillEllipse(r,d,pat);
}

void Clipper::FillPolygon(BRegion* clip_reg, BPoint *ptlist, int32 numpts, BRect rect, LayerData *d, const Pattern &pat)
{
	fDriver->FillPolygon(ptlist,numpts,rect,d,pat);
}

void Clipper::FillRect(BRegion* clip_reg, BRect r, LayerData *d, const Pattern &pat)
{
	BRegion reg;

	reg.Set(r);
	if ( clip_reg )
		reg.IntersectWith(clip_reg);
	fDriver->FillRegion(&reg,d,pat);
}

void Clipper::FillRegion(BRegion* clip_reg, BRegion *r, LayerData *d, const Pattern &pat)
{
	BRegion reg;

	reg = *r;
	if ( clip_reg )
		reg.IntersectWith(clip_reg);
	fDriver->FillRegion(&reg,d,pat);
}

void Clipper::FillRoundRect(BRegion* clip_reg, BRect r, float xrad, float yrad, LayerData *d, const Pattern &pat)
{
	if ( !clip_reg )
		fDriver->FillRoundRect(r,xrad,yrad,d,pat);
	else
	{
		BRegion reg(r);
		BRect corner_rect(0,0,xrad,yrad);
		BRect arc_rect(0,0,2*xrad,2*yrad);
		corner_rect.OffsetTo(r.left,r.top);
		reg.Exclude(corner_rect);
		corner_rect.OffsetTo(r.right-xrad,r.top);
		reg.Exclude(corner_rect);
		corner_rect.OffsetTo(r.left,r.bottom-yrad);
		reg.Exclude(corner_rect);
		corner_rect.OffsetTo(r.right-xrad,r.bottom-yrad);
		reg.Exclude(corner_rect);
		FillRegion(clip_reg,&reg,d,pat);
		arc_rect.OffsetTo(r.left,r.top);
		FillArc(clip_reg,arc_rect,90,90,d,pat);
		arc_rect.OffsetTo(r.left,r.bottom-2*yrad);
		FillArc(clip_reg,arc_rect,180,90,d,pat);
		arc_rect.OffsetTo(r.right-2*xrad,r.bottom-2*yrad);
		FillArc(clip_reg,arc_rect,270,90,d,pat);
		arc_rect.OffsetTo(r.right-2*xrad,r.top);
		FillArc(clip_reg,arc_rect,0,90,d,pat);
	}
}

void Clipper::FillTriangle(BRegion* clip_reg, BPoint *pts, BRect r, LayerData *d, const Pattern &pat)
{
	fDriver->FillTriangle(pts,r,d,pat);
}

void Clipper::StrokeArc(BRegion* clip_reg, BRect r, float angle, float span, LayerData *d, const Pattern &pat)
{
	fDriver->StrokeArc(r,angle,span,d,pat);
}

void Clipper::StrokeBezier(BRegion* clip_reg, BPoint *pts, LayerData *d, const Pattern &pat)
{
	fDriver->StrokeBezier(pts,d,pat);
}

void Clipper::StrokeEllipse(BRegion* clip_reg, BRect r, LayerData *d, const Pattern &pat)
{
	fDriver->StrokeEllipse(r,d,pat);
}

void Clipper::StrokeLine(BRegion* clip_reg, BPoint start, BPoint end, LayerData *d, const Pattern &pat)
{
	fDriver->StrokeLine(start,end,d,pat);
}

void Clipper::StrokePolygon(BRegion* clip_reg, BPoint *ptlist, int32 numpts, BRect rect, LayerData *d, const Pattern &pat, bool is_closed)
{
	fDriver->StrokePolygon(ptlist,numpts,rect,d,pat,is_closed);
}

void Clipper::StrokeRect(BRegion* clip_reg, BRect r, LayerData *d, const Pattern &pat)
{
	fDriver->StrokeRect(r,d,pat);
}

void Clipper::StrokeRoundRect(BRegion* clip_reg, BRect r, float xrad, float yrad, LayerData *d, const Pattern &pat)
{
	fDriver->StrokeRoundRect(r,xrad,yrad,d,pat);
}

void Clipper::StrokeTriangle(BRegion* clip_reg, BPoint *pts, BRect r, LayerData *d, const Pattern &pat)
{
	fDriver->StrokeTriangle(pts,r,d,pat);
}

void Clipper::StrokeLineArray(BRegion* clip_reg, BPoint *pts, int32 numlines, RGBColor *colors, LayerData *d)
{
	fDriver->StrokeLineArray(pts,numlines,colors,d);
}
