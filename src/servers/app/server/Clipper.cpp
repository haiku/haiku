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
#include "LayerData.h"

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








void Clipper::FillArc(const BRect r, float angle, float span, RGBColor& color, BRegion* clip_reg)
{
	fDriver->FillArc(r,angle,span,color);
}

void Clipper::FillArc(const BRect r, float angle, float span, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color, BRegion* clip_reg)
{
	fDriver->FillArc(r,angle,span,pattern,high_color,low_color);
}

void Clipper::FillBezier(BPoint *pts, RGBColor& color, BRegion* clip_reg)
{
	fDriver->FillBezier(pts,color);
}

void Clipper::FillBezier(BPoint *pts, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color, BRegion* clip_reg)
{
	fDriver->FillBezier(pts,pattern,high_color,low_color);
}

void Clipper::FillEllipse(BRect r, RGBColor& color, BRegion* clip_reg)
{
	fDriver->FillEllipse(r,color);
}

void Clipper::FillEllipse(BRect r, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color, BRegion* clip_reg)
{
	fDriver->FillEllipse(r,pattern,high_color,low_color);
}

void Clipper::FillPolygon(BPoint *ptlist, int32 numpts, RGBColor& color, BRegion* clip_reg)
{
	fDriver->FillPolygon(ptlist,numpts,color);
}

void Clipper::FillPolygon(BPoint *ptlist, int32 numpts, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color, BRegion* clip_reg)
{
	fDriver->FillPolygon(ptlist,numpts,pattern,high_color,low_color);
}

void Clipper::FillRect(const BRect r, RGBColor& color, BRegion* clip_reg)
{
	if ( clip_reg )
	{
		BRegion reg;
		reg.Set(r);
		reg.IntersectWith(clip_reg);
		fDriver->FillRegion(reg,color);
	}
	else
		fDriver->FillRect(r,color);
}

void Clipper::FillRect(const BRect r, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color, BRegion* clip_reg)
{
	if ( clip_reg )
	{
		BRegion reg;
		reg.Set(r);
		reg.IntersectWith(clip_reg);
		fDriver->FillRegion(reg,pattern,high_color,low_color);
	}
	else
		fDriver->FillRect(r,pattern,high_color,low_color);
}

void Clipper::FillRegion(BRegion& r, RGBColor& color, BRegion* clip_reg)
{
	if ( clip_reg )
	{
		BRegion reg;
		reg = r;
		reg.IntersectWith(clip_reg);
		fDriver->FillRegion(reg,color);
	}
	else
		fDriver->FillRegion(r,color);
}

void Clipper::FillRegion(BRegion& r, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color, BRegion* clip_reg)
{
	if ( clip_reg )
	{
		BRegion reg;
		reg = r;
		reg.IntersectWith(clip_reg);
		fDriver->FillRegion(reg,pattern,high_color,low_color);
	}
	else
		fDriver->FillRegion(r,pattern,high_color,low_color);
}

void Clipper::FillRoundRect(BRect r, float xrad, float yrad, RGBColor& color, BRegion* clip_reg)
{
	if ( !clip_reg )
		fDriver->FillRoundRect(r,xrad,yrad,color);
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
		FillRegion(reg,color,clip_reg);
		arc_rect.OffsetTo(r.left,r.top);
		FillArc(arc_rect,90,90,color,clip_reg);
		arc_rect.OffsetTo(r.left,r.bottom-2*yrad);
		FillArc(arc_rect,180,90,color,clip_reg);
		arc_rect.OffsetTo(r.right-2*xrad,r.bottom-2*yrad);
		FillArc(arc_rect,270,90,color,clip_reg);
		arc_rect.OffsetTo(r.right-2*xrad,r.top);
		FillArc(arc_rect,0,90,color,clip_reg);
	}
}

void Clipper::FillRoundRect(BRect r, float xrad, float yrad, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color, BRegion* clip_reg)
{
	if ( !clip_reg )
		fDriver->FillRoundRect(r,xrad,yrad,pattern,high_color,low_color);
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
		FillRegion(reg,pattern,high_color,low_color,clip_reg);
		arc_rect.OffsetTo(r.left,r.top);
		FillArc(arc_rect,90,90,pattern,high_color,low_color,clip_reg);
		arc_rect.OffsetTo(r.left,r.bottom-2*yrad);
		FillArc(arc_rect,180,90,pattern,high_color,low_color,clip_reg);
		arc_rect.OffsetTo(r.right-2*xrad,r.bottom-2*yrad);
		FillArc(arc_rect,270,90,pattern,high_color,low_color,clip_reg);
		arc_rect.OffsetTo(r.right-2*xrad,r.top);
		FillArc(arc_rect,0,90,pattern,high_color,low_color,clip_reg);
	}
}

void Clipper::FillTriangle(BPoint *pts, RGBColor& color, BRegion* clip_reg)
{
	fDriver->FillTriangle(pts,color);
}

void Clipper::FillTriangle(BPoint *pts, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color, BRegion* clip_reg)
{
	fDriver->FillTriangle(pts,pattern,high_color,low_color);
}

void Clipper::StrokeArc(BRect r, float angle, float span, float pensize, RGBColor& color, BRegion* clip_reg)
{
	fDriver->StrokeArc(r,angle,span,pensize,color);
}

void Clipper::StrokeArc(BRect r, float angle, float span, float pensize, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color, BRegion* clip_reg)
{
	fDriver->StrokeArc(r,angle,span,pensize,pattern,high_color,low_color);
}

void Clipper::StrokeBezier(BPoint *pts, float pensize, RGBColor& color, BRegion* clip_reg)
{
	fDriver->StrokeBezier(pts,pensize,color);
}

void Clipper::StrokeBezier(BPoint *pts, float pensize, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color, BRegion* clip_reg)
{
	fDriver->StrokeBezier(pts,pensize,pattern,high_color,low_color);
}

void Clipper::StrokeEllipse(BRect r, float pensize, RGBColor& color, BRegion* clip_reg)
{
	fDriver->StrokeEllipse(r,pensize,color);
}

void Clipper::StrokeEllipse(BRect r, float pensize, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color, BRegion* clip_reg)
{
	fDriver->StrokeEllipse(r,pensize,pattern,high_color,low_color);
}

void Clipper::StrokeLine(BPoint start, BPoint end, float pensize, RGBColor& color, BRegion* clip_reg)
{
	if ( clip_reg )
	{
		int i;
		BRect currentRect;
		LineCalc line(start,end);
		BRect lineBounds(line.MinX(),line.MinY(),line.MaxX(),line.MaxY());
		for (i=0; i<clip_reg->CountRects(); i++)
		{
			currentRect = clip_reg->RectAt(i);
			if ( currentRect.Intersects(lineBounds) ) 
			{
				if ( currentRect.Contains(start) && currentRect.Contains(end) )
				{
					fDriver->StrokeLine(start,end,pensize,color);
					return;
				}
				if ( line.ClipToRect(currentRect) )
				{
					fDriver->StrokeLine(line.GetStart(),line.GetEnd(),pensize,color);
					line.SetPoints(start,end);
				}
			}
		}
	}
	else
		fDriver->StrokeLine(start,end,pensize,color);
}

void Clipper::StrokeLine(BPoint start, BPoint end, float pensize, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color, BRegion* clip_reg)
{
	if ( clip_reg )
	{
		int i;
		BRect currentRect;
		LineCalc line(start,end);
		BRect lineBounds(line.MinX(),line.MinY(),line.MaxX(),line.MaxY());
		for (i=0; i<clip_reg->CountRects(); i++)
		{
			currentRect = clip_reg->RectAt(i);
			if ( currentRect.Intersects(lineBounds) ) 
			{
				if ( currentRect.Contains(start) && currentRect.Contains(end) )
				{
					fDriver->StrokeLine(start,end,pensize,pattern,high_color,low_color);
					return;
				}
				if ( line.ClipToRect(currentRect) )
				{
					fDriver->StrokeLine(line.GetStart(),line.GetEnd(),pensize,pattern,high_color,low_color);
					line.SetPoints(start,end);
				}
			}
		}
	}
	else
		fDriver->StrokeLine(start,end,pensize,pattern,high_color,low_color);
}

void Clipper::StrokePolygon(BPoint *ptlist, int32 numpts, float pensize, RGBColor& color, bool is_closed, BRegion* clip_reg)
{
	fDriver->StrokePolygon(ptlist,numpts,pensize,color,is_closed);
}

void Clipper::StrokePolygon(BPoint *ptlist, int32 numpts, float pensize, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color, bool is_closed, BRegion* clip_reg)
{
	fDriver->StrokePolygon(ptlist,numpts,pensize,pattern,high_color,low_color,is_closed);
}

void Clipper::StrokeRect(BRect r, float pensize, RGBColor& color, BRegion* clip_reg)
{
	if ( clip_reg )
	{
		StrokeLine(r.LeftTop(), r.RightTop(), pensize, color, clip_reg);
		StrokeLine(r.LeftTop(), r.LeftBottom(), pensize, color, clip_reg);
		StrokeLine(r.RightTop(), r.RightBottom(), pensize, color, clip_reg);
		StrokeLine(r.LeftBottom(), r.RightBottom(), pensize, color, clip_reg);
	}
	else
		fDriver->StrokeRect(r,pensize,color);
}

void Clipper::StrokeRect(BRect r, float pensize, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color, BRegion* clip_reg)
{
	if ( clip_reg )
	{
		StrokeLine(r.LeftTop(), r.RightTop(), pensize, pattern, high_color, low_color, clip_reg);
		StrokeLine(r.LeftTop(), r.LeftBottom(), pensize, pattern, high_color, low_color, clip_reg);
		StrokeLine(r.RightTop(), r.RightBottom(), pensize, pattern, high_color, low_color, clip_reg);
		StrokeLine(r.LeftBottom(), r.RightBottom(), pensize, pattern, high_color, low_color, clip_reg);
	}
	else
		fDriver->StrokeRect(r,pensize,pattern,high_color,low_color);
}

void Clipper::StrokeRegion(BRegion& r, float pensize, RGBColor& color, BRegion* clip_reg)
{
	fDriver->StrokeRegion(r,pensize,color);
}

void Clipper::StrokeRegion(BRegion& r, float pensize, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color, BRegion* clip_reg)
{
	fDriver->StrokeRegion(r,pensize,pattern,high_color,low_color);
}

void Clipper::StrokeRoundRect(BRect r, float xrad, float yrad, float pensize, RGBColor& color, BRegion* clip_reg)
{
	fDriver->StrokeRoundRect(r,xrad,yrad,pensize,color);
}

void Clipper::StrokeRoundRect(BRect r, float xrad, float yrad, float pensize, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color, BRegion* clip_reg)
{
	fDriver->StrokeRoundRect(r,xrad,yrad,pensize,pattern,high_color,low_color);
}

void Clipper::StrokeTriangle(BPoint *pts, float pensize, RGBColor& color, BRegion* clip_reg)
{
	fDriver->StrokeTriangle(pts,pensize,color);
}

void Clipper::StrokeTriangle(BPoint *pts, float pensize, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color, BRegion* clip_reg)
{
	fDriver->StrokeTriangle(pts,pensize,pattern,high_color,low_color);
}

void Clipper::StrokeLineArray(BPoint *pts, int32 numlines, float pensize, RGBColor *colors, BRegion* clip_reg)
{
	fDriver->StrokeLineArray(pts,numlines,pensize,colors);
}
