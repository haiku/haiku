/*

Template

Copyright (c) 2002 OpenBeOS. 

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

#include "Template.h"

virtual void Template::Op(int number) { 
}


virtual void Template::MovePenBy(BPoint delta) { 
}


virtual void Template::StrokeLine(BPoint start, BPoint end) { 
}


virtual void Template::StrokeRect(BRect rect) { 
}


virtual void Template::FillRect(BRect rect) { 
}


virtual void Template::StrokeRoundRect(BRect rect, BPoint radii) { 
}


virtual void Template::FillRoundRect(BRect rect, BPoint radii) { 
}


virtual void Template::StrokeBezier(BPoint *control) { 
}


virtual void Template::FillBezier(BPoint *control) { 
}


virtual void Template::StrokeArc(BPoint center, BPoint radii, float startTheta, float arcTheta) { 
}


virtual void Template::FillArc(BPoint center, BPoint radii, float startTheta, float arcTheta) { 
}


virtual void Template::StrokeEllipse(BPoint center, BPoint radii) { 
}


virtual void Template::FillEllipse(BPoint center, BPoint radii) { 
}


virtual void Template::StrokePolygon(int32 numPoints, BPoint *points, bool isClosed) { 
}


virtual void Template::FillPolygon(int32 numPoints, BPoint *points, bool isClosed) { 
}


virtual void Template::StrokeShape(BShape *shape) { 
}


virtual void Template::FillShape(BShape *shape) { 
}


virtual void Template::DrawString(char *string, float escapement_nospace, float escapement_space) { 
}


virtual void Template::DrawPixels(BRect src, BRect dest, int32 width, int32 height, int32 bytesPerRow, int32 pixelFormat, int32 flags, void *data) { 
}


virtual void Template::SetClippingRects(BRect *rects, uint32 numRects) { 
}


virtual void Template::ClipToPicture(BPicture *picture, BPoint point, bool clip_to_inverse_picture) { 
}


virtual void Template::PushState() { 
}


virtual void Template::PopState() { 
}


virtual void Template::EnterStateChange() { 
}


virtual void Template::ExitStateChange() { 
}


virtual void Template::EnterFontState() { 
}


virtual void Template::ExitFontState() { 
}


virtual void Template::SetOrigin(BPoint pt) { 
}


virtual void Template::SetPenLocation(BPoint pt) { 
}


virtual void Template::SetDrawingMode(drawing_mode mode) { 
}


virtual void Template::SetLineMode(cap_mode capMode, join_mode joinMode, float miterLimit) { 
}


virtual void Template::SetPenSize(float size) { 
}


virtual void Template::SetForeColor(rgb_color color) { 
}


virtual void Template::SetBackColor(rgb_color color) { 
}


virtual void Template::SetStipplePattern(pattern p) { 
}


virtual void Template::SetScale(float scale) { 
}


virtual void Template::SetFontFamily(char *family) { 
}


virtual void Template::SetFontStyle(char *style) { 
}


virtual void Template::SetFontSpacing(int32 spacing) { 
}


virtual void Template::SetFontSize(float size) { 
}


virtual void Template::SetFontRotate(float rotation) { 
}


virtual void Template::SetFontEncoding(int32 encoding) { 
}


virtual void Template::SetFontFlags(int32 flags) { 
}


virtual void Template::SetFontShear(float shear) { 
}


virtual void Template::SetFontFace(int32 flags) { 
}



