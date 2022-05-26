/*

PictureIterator

Copyright (c) 2001, 2002 Haiku.

Authors: 
	Philippe Houdoin
	Simon Gauvin	
	Michael Pfeiffer
	
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

#include "PictureIterator.h"

// BPicture playback handlers class instance redirectors
static void	_MovePenBy(void *p, BPoint delta) 														{ return ((PictureIterator *) p)->MovePenBy(delta); }
static void	_StrokeLine(void *p, BPoint start, BPoint end) 											{ return ((PictureIterator *) p)->StrokeLine(start, end); }
static void	_StrokeRect(void *p, BRect rect) 														{ return ((PictureIterator *) p)->StrokeRect(rect); }
static void	_FillRect(void *p, BRect rect) 															{ return ((PictureIterator *) p)->FillRect(rect); }
static void	_StrokeRoundRect(void *p, BRect rect, BPoint radii) 									{ return ((PictureIterator *) p)->StrokeRoundRect(rect, radii); }
static void	_FillRoundRect(void *p, BRect rect, BPoint radii)  										{ return ((PictureIterator *) p)->FillRoundRect(rect, radii); }
static void	_StrokeBezier(void *p, BPoint *control)  												{ return ((PictureIterator *) p)->StrokeBezier(control); }
static void	_FillBezier(void *p, BPoint *control)  													{ return ((PictureIterator *) p)->FillBezier(control); }
static void	_StrokeArc(void *p, BPoint center, BPoint radii, float startTheta, float arcTheta)		{ return ((PictureIterator *) p)->StrokeArc(center, radii, startTheta, arcTheta); }
static void	_FillArc(void *p, BPoint center, BPoint radii, float startTheta, float arcTheta)		{ return ((PictureIterator *) p)->FillArc(center, radii, startTheta, arcTheta); }
static void	_StrokeEllipse(void *p, BPoint center, BPoint radii)									{ return ((PictureIterator *) p)->StrokeEllipse(center, radii); }
static void	_FillEllipse(void *p, BPoint center, BPoint radii)										{ return ((PictureIterator *) p)->FillEllipse(center, radii); }
static void	_StrokePolygon(void *p, int32 numPoints, BPoint *points, bool isClosed) 				{ return ((PictureIterator *) p)->StrokePolygon(numPoints, points, isClosed); }
static void	_FillPolygon(void *p, int32 numPoints, BPoint *points, bool isClosed)					{ return ((PictureIterator *) p)->FillPolygon(numPoints, points, isClosed); }
static void	_StrokeShape(void * p, BShape *shape)													{ return ((PictureIterator *) p)->StrokeShape(shape); }
static void	_FillShape(void * p, BShape *shape)														{ return ((PictureIterator *) p)->FillShape(shape); }
static void	_DrawString(void *p, char *string, float deltax, float deltay)							{ return ((PictureIterator *) p)->DrawString(string, deltax, deltay); }
static void	_DrawPixels(void *p, BRect src, BRect dest, int32 width, int32 height, int32 bytesPerRow, int32 pixelFormat, int32 flags, void *data)
						{ return ((PictureIterator *) p)->DrawPixels(src, dest, width, height, bytesPerRow, pixelFormat, flags, data); }
static void	_SetClippingRects(void *p, BRect *rects, uint32 numRects)								{ return ((PictureIterator *) p)->SetClippingRects(rects, numRects); }
static void	_ClipToPicture(void * p, BPicture *picture, BPoint point, bool clip_to_inverse_picture)	{ return ((PictureIterator *) p)->ClipToPicture(picture, point, clip_to_inverse_picture); }
static void	_PushState(void *p)  																	{ return ((PictureIterator *) p)->PushState(); }
static void	_PopState(void *p)  																	{ return ((PictureIterator *) p)->PopState(); }
static void	_EnterStateChange(void *p) 																{ return ((PictureIterator *) p)->EnterStateChange(); }
static void	_ExitStateChange(void *p) 																{ return ((PictureIterator *) p)->ExitStateChange(); }
static void	_EnterFontState(void *p) 																{ return ((PictureIterator *) p)->EnterFontState(); }
static void	_ExitFontState(void *p) 																{ return ((PictureIterator *) p)->ExitFontState(); }
static void	_SetOrigin(void *p, BPoint pt)															{ return ((PictureIterator *) p)->SetOrigin(pt); }
static void	_SetPenLocation(void *p, BPoint pt)														{ return ((PictureIterator *) p)->SetPenLocation(pt); }
static void	_SetDrawingMode(void *p, drawing_mode mode)												{ return ((PictureIterator *) p)->SetDrawingMode(mode); }
static void	_SetLineMode(void *p, cap_mode capMode, join_mode joinMode, float miterLimit)			{ return ((PictureIterator *) p)->SetLineMode(capMode, joinMode, miterLimit); }
static void	_SetPenSize(void *p, float size)														{ return ((PictureIterator *) p)->SetPenSize(size); }
static void	_SetForeColor(void *p, rgb_color color)													{ return ((PictureIterator *) p)->SetForeColor(color); }
static void	_SetBackColor(void *p, rgb_color color)													{ return ((PictureIterator *) p)->SetBackColor(color); }
static void	_SetStipplePattern(void *p, pattern pat)												{ return ((PictureIterator *) p)->SetStipplePattern(pat); }
static void	_SetScale(void *p, float scale)															{ return ((PictureIterator *) p)->SetScale(scale); }
static void	_SetFontFamily(void *p, char *family)													{ return ((PictureIterator *) p)->SetFontFamily(family); }
static void	_SetFontStyle(void *p, char *style)														{ return ((PictureIterator *) p)->SetFontStyle(style); }
static void	_SetFontSpacing(void *p, int32 spacing)													{ return ((PictureIterator *) p)->SetFontSpacing(spacing); }
static void	_SetFontSize(void *p, float size)														{ return ((PictureIterator *) p)->SetFontSize(size); }
static void	_SetFontRotate(void *p, float rotation)													{ return ((PictureIterator *) p)->SetFontRotate(rotation); }
static void	_SetFontEncoding(void *p, int32 encoding)												{ return ((PictureIterator *) p)->SetFontEncoding(encoding); }
static void	_SetFontFlags(void *p, int32 flags)														{ return ((PictureIterator *) p)->SetFontFlags(flags); }
static void	_SetFontShear(void *p, float shear)														{ return ((PictureIterator *) p)->SetFontShear(shear); }
static void	_SetFontFace(void * p, int32 flags)														{ return ((PictureIterator *) p)->SetFontFace(flags); }

// undefined or undocumented operation handlers...
static void	_op0(void * p)	{ return ((PictureIterator *) p)->Op(0); }
static void	_op19(void * p)	{ return ((PictureIterator *) p)->Op(19); }
static void	_op45(void * p)	{ return ((PictureIterator *) p)->Op(45); }
static void	_op47(void * p)	{ return ((PictureIterator *) p)->Op(47); }
static void	_op48(void * p)	{ return ((PictureIterator *) p)->Op(48); }
static void	_op49(void * p)	{ return ((PictureIterator *) p)->Op(49); }

// Private Variables
// -----------------

static void *
playbackHandlers[] = {
		(void *)_op0,					// 0	no operation
		(void *)_MovePenBy,				// 1	MovePenBy(void *user, BPoint delta)
		(void *)_StrokeLine,			// 2	StrokeLine(void *user, BPoint start, BPoint end)
		(void *)_StrokeRect,			// 3	StrokeRect(void *user, BRect rect)
		(void *)_FillRect,				// 4	FillRect(void *user, BRect rect)
		(void *)_StrokeRoundRect,		// 5	StrokeRoundRect(void *user, BRect rect, BPoint radii)
		(void *)_FillRoundRect,			// 6	FillRoundRect(void *user, BRect rect, BPoint radii)
		(void *)_StrokeBezier,			// 7	StrokeBezier(void *user, BPoint *control)
		(void *)_FillBezier,			// 8	FillBezier(void *user, BPoint *control)
		(void *)_StrokeArc,				// 9	StrokeArc(void *user, BPoint center, BPoint radii, float startTheta, float arcTheta)
		(void *)_FillArc,				// 10	FillArc(void *user, BPoint center, BPoint radii, float startTheta, float arcTheta)
		(void *)_StrokeEllipse,			// 11	StrokeEllipse(void *user, BPoint center, BPoint radii)
		(void *)_FillEllipse,			// 12	FillEllipse(void *user, BPoint center, BPoint radii)
		(void *)_StrokePolygon,			// 13	StrokePolygon(void *user, int32 numPoints, BPoint *points, bool isClosed)
		(void *)_FillPolygon,			// 14	FillPolygon(void *user, int32 numPoints, BPoint *points, bool isClosed)
		(void *)_StrokeShape,			// 15	StrokeShape(void *user, BShape *shape)
		(void *)_FillShape,				// 16	FillShape(void *user, BShape *shape)
		(void *)_DrawString,			// 17	DrawString(void *user, char *string, float deltax, float deltay)
		(void *)_DrawPixels,			// 18	DrawPixels(void *user, BRect src, BRect dest, int32 width, int32 height, int32 bytesPerRow, int32 pixelFormat, int32 flags, void *data)
		(void *)_op19,					// 19	*reserved*
		(void *)_SetClippingRects,		// 20	SetClippingRects(void *user, BRect *rects, uint32 numRects)
		(void *)_ClipToPicture,			// 21	ClipToPicture(void *user, BPicture *picture, BPoint pt, bool clip_to_inverse_picture)
		(void *)_PushState,				// 22	PushState(void *user)
		(void *)_PopState,				// 23	PopState(void *user)
		(void *)_EnterStateChange,		// 24	EnterStateChange(void *user)
		(void *)_ExitStateChange,		// 25	ExitStateChange(void *user)
		(void *)_EnterFontState,		// 26	EnterFontState(void *user)
		(void *)_ExitFontState,			// 27	ExitFontState(void *user)
		(void *)_SetOrigin,				// 28	SetOrigin(void *user, BPoint pt)
		(void *)_SetPenLocation,		// 29	SetPenLocation(void *user, BPoint pt)
		(void *)_SetDrawingMode,		// 30	SetDrawingMode(void *user, drawing_mode mode)
		(void *)_SetLineMode,			// 31	SetLineMode(void *user, cap_mode capMode, join_mode joinMode, float miterLimit)
		(void *)_SetPenSize,			// 32	SetPenSize(void *user, float size)
		(void *)_SetForeColor,			// 33	SetForeColor(void *user, rgb_color color)
		(void *)_SetBackColor,			// 34	SetBackColor(void *user, rgb_color color)
		(void *)_SetStipplePattern,		// 35	SetStipplePattern(void *user, pattern p)
		(void *)_SetScale,				// 36	SetScale(void *user, float scale)
		(void *)_SetFontFamily,			// 37	SetFontFamily(void *user, char *family)
		(void *)_SetFontStyle,			// 38	SetFontStyle(void *user, char *style)
		(void *)_SetFontSpacing,		// 39	SetFontSpacing(void *user, int32 spacing)
		(void *)_SetFontSize,			// 40	SetFontSize(void *user, float size)
		(void *)_SetFontRotate,			// 41	SetFontRotate(void *user, float rotation)
		(void *)_SetFontEncoding,		// 42	SetFontEncoding(void *user, int32 encoding)
		(void *)_SetFontFlags,			// 43	SetFontFlags(void *user, int32 flags)
		(void *)_SetFontShear,			// 44	SetFontShear(void *user, float shear)
		(void *)_op45,					// 45	*reserved*
		(void *)_SetFontFace,			// 46	SetFontFace(void *user, int32 flags)
		(void *)_op47,
		(void *)_op48,
		(void *)_op49,

		NULL
	}; 

void
PictureIterator::Iterate(BPicture* picture) {
	picture->Play(playbackHandlers, 50, this);
}
