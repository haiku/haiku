/*

Copyright (c) 2001, 2002 Haiku.

Author: 
	<YOUR NAME>
	
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/

#ifndef _TEMPLATE_H
#define _TEMPLATE_H

#include "PictureIterator.h"

class Template : public PictureIterator
{
public:
		
		// BPicture playback handlers
		virtual void		Op(int number);
		virtual void		MovePenBy(BPoint delta);
		virtual void		StrokeLine(BPoint start, BPoint end);
		virtual void		StrokeRect(BRect rect);
		virtual void		FillRect(BRect rect);
		virtual void		StrokeRoundRect(BRect rect, BPoint radii);
		virtual void		FillRoundRect(BRect rect, BPoint radii);
		virtual void		StrokeBezier(BPoint *control);
		virtual void		FillBezier(BPoint *control);
		virtual void		StrokeArc(BPoint center, BPoint radii, float startTheta, float arcTheta);
		virtual void		FillArc(BPoint center, BPoint radii, float startTheta, float arcTheta);
		virtual void		StrokeEllipse(BPoint center, BPoint radii);
		virtual void		FillEllipse(BPoint center, BPoint radii);
		virtual void		StrokePolygon(int32 numPoints, BPoint *points, bool isClosed);
		virtual void		FillPolygon(int32 numPoints, BPoint *points, bool isClosed);
		virtual void        StrokeShape(BShape *shape);
		virtual void        FillShape(BShape *shape);
		virtual void		DrawString(char *string, float escapement_nospace, float escapement_space);
		virtual void		DrawPixels(BRect src, BRect dest, int32 width, int32 height, int32 bytesPerRow, int32 pixelFormat, int32 flags, void *data);
		virtual void		SetClippingRects(BRect *rects, uint32 numRects);
		virtual void    	ClipToPicture(BPicture *picture, BPoint point, bool clip_to_inverse_picture);
		virtual void		PushState();
		virtual void		PopState();
		virtual void		EnterStateChange();
		virtual void		ExitStateChange();
		virtual void		EnterFontState();
		virtual void		ExitFontState();
		virtual void		SetOrigin(BPoint pt);
		virtual void		SetPenLocation(BPoint pt);
		virtual void		SetDrawingMode(drawing_mode mode);
		virtual void		SetLineMode(cap_mode capMode, join_mode joinMode, float miterLimit);
		virtual void		SetPenSize(float size);
		virtual void		SetForeColor(rgb_color color);
		virtual void		SetBackColor(rgb_color color);
		virtual void		SetStipplePattern(pattern p);
		virtual void		SetScale(float scale);
		virtual void		SetFontFamily(char *family);
		virtual void		SetFontStyle(char *style);
		virtual void		SetFontSpacing(int32 spacing);
		virtual void		SetFontSize(float size);
		virtual void		SetFontRotate(float rotation);
		virtual void		SetFontEncoding(int32 encoding);
		virtual void		SetFontFlags(int32 flags);
		virtual void		SetFontShear(float shear);
		virtual void		SetFontFace(int32 flags);
};

#endif // _TEMPLATE_H
