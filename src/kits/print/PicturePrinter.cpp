/*

PicturePrinter

Copyright (c) 2002 Haiku.

Author: 
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

#include <stdio.h>

#include "PicturePrinter.h"

PicturePrinter::PicturePrinter(int indent)
	: fIndent(indent)
 {
 }

void PicturePrinter::Print(const char* text) {
	printf("%s ", text);
}

void PicturePrinter::Print(BPoint* p) {
	printf("point (%f, %f) ", p->x, p->y);
}

void PicturePrinter::Print(BRect* r) {
	printf("rect [l: %f, t: %f, r: %f, b: %f] ", r->left, r->top, r->right, r->bottom);
}

void PicturePrinter::Print(int numPoints, BPoint* points) {
	for (int i = 0; i < numPoints; i ++) {
		Indent(1); printf("%d ", i); Print(&points[i]); Cr();
	}
}

void PicturePrinter::Print(int numRects, BRect* rects) {
	for (int i = 0; i < numRects; i ++) {
		Indent(1); printf("%d ", i); Print(&rects[i]); Cr();
	}
}

void PicturePrinter::Print(BShape* shape) {
	printf("Shape %p\n", shape);
	ShapePrinter printer(this);
	printer.Iterate(shape);
}

void PicturePrinter::Print(const char* text, float f) {
	printf("%s %f ", text, f); 
}

void PicturePrinter::Print(const char* text, BPoint* point) {
	Print(text); Print(point);
}

void PicturePrinter::Print(rgb_color color) {
	printf("color r: %d g: %d b: %d", color.red, color.green, color.blue);
}

void PicturePrinter::Print(float f) {
	printf("%f ", f);
}

void PicturePrinter::Cr() {
	printf("\n");
}

void PicturePrinter::Indent(int inc) {
	for (int i = fIndent + inc; i > 0; i --) printf("  ");
}

void PicturePrinter::IncIndent() {
	fIndent ++;
}

void PicturePrinter::DecIndent() {
	fIndent --;
}

void PicturePrinter::Op(int number) { 
	Indent(); printf("Unknown operator %d\n", number); Cr();
}


void PicturePrinter::MovePenBy(BPoint delta) { 
	Indent(); Print("MovePenBy"); Print(&delta); Cr();
}


void PicturePrinter::StrokeLine(BPoint start, BPoint end) { 
	Indent(); Print("StrokeLine"); Print(&start); Print(&end); Cr();
}


void PicturePrinter::StrokeRect(BRect rect) { 
	Indent(); Print("StrokeRect"); Print(&rect); Cr();
}


void PicturePrinter::FillRect(BRect rect) { 
	Indent(); Print("FillRect"); Print(&rect); Cr();
}


void PicturePrinter::StrokeRoundRect(BRect rect, BPoint radii) { 
	Indent(); Print("StrokeRoundRect"); Print(&rect); Print("radii", &radii); Cr();
}


void PicturePrinter::FillRoundRect(BRect rect, BPoint radii) { 
	Indent(); Print("FillRoundRect"); Print(&rect); Print("radii", &radii); Cr();
}


void PicturePrinter::StrokeBezier(BPoint *control) { 
	Indent(); Print("StrokeBezier"); Print(4, control); Cr();
}


void PicturePrinter::FillBezier(BPoint *control) { 
	Indent(); Print("FillBezier"); Print(4, control); Cr();
}


void PicturePrinter::StrokeArc(BPoint center, BPoint radii, float startTheta, float arcTheta) { 
	Indent(); Print("StrokeArc center="); Print(&center); Print("radii="); Print(&radii); Print("arcTheta=", arcTheta); Cr();
}


void PicturePrinter::FillArc(BPoint center, BPoint radii, float startTheta, float arcTheta) { 
	Indent(); Print("FillArc center="); Print(&center); Print("radii="); Print(&radii); Print("arcTheta=", arcTheta); Cr();
}


void PicturePrinter::StrokeEllipse(BPoint center, BPoint radii) { 
	Indent(); Print("StrokeEllipse center="); Print(&center); Print("radii="); Print(&radii); Cr();
}


void PicturePrinter::FillEllipse(BPoint center, BPoint radii) { 
	Indent(); Print("FillEllipse center="); Print(&center); Print("radii="); Print(&radii); Cr();
}


void PicturePrinter::StrokePolygon(int32 numPoints, BPoint *points, bool isClosed) { 
	Indent(); Print("StrokePolygon");
	printf("%s ", isClosed ? "closed" : "open"); Cr();
	Print(numPoints, points);
}


void PicturePrinter::FillPolygon(int32 numPoints, BPoint *points, bool isClosed) { 
	Indent(); Print("FillPolygon");
	printf("%s ", isClosed ? "closed" : "open"); Cr();
	Print(numPoints, points);
}


void PicturePrinter::StrokeShape(BShape *shape) { 
	Indent(); Print("StrokeShape"); Print(shape); Cr();
}


void PicturePrinter::FillShape(BShape *shape) { 
	Indent(); Print("FillShape"); Print(shape); Cr();
}


void PicturePrinter::DrawString(char *string, float escapement_nospace, float escapement_space) { 
	Indent(); Print("DrawString"); 
	Print("escapement_nospace", escapement_nospace);
	Print("escapement_space", escapement_space);
	Print("text:"); Print(string); Cr();
}


void PicturePrinter::DrawPixels(BRect src, BRect dest, int32 width, int32 height, int32 bytesPerRow, int32 pixelFormat, int32 flags, void *data) { 
	Indent(); Print("DrawPixels"); Cr();
}


void PicturePrinter::SetClippingRects(BRect *rects, uint32 numRects) { 
	Indent(); Print("SetClippingRects"); 
	if (numRects == 0) Print("none");
	Cr();
	Print(numRects, rects);
}


void PicturePrinter::ClipToPicture(BPicture *picture, BPoint point, bool clip_to_inverse_picture) { 
	Indent(); 
	Print(clip_to_inverse_picture ? "ClipToInversePicture" : "ClipToPicture");
	Print("point=", &point); Cr();
	PicturePrinter printer(fIndent+1);
	printer.Iterate(picture);
}


void PicturePrinter::PushState() { 
	Indent(); Print("PushState"); Cr();
	IncIndent();
}


void PicturePrinter::PopState() { 
	DecIndent();
	Indent(); Print("PopState"); Cr();
}


void PicturePrinter::EnterStateChange() { 
	Indent(); Print("EnterStateChange"); Cr();
}


void PicturePrinter::ExitStateChange() { 
	Indent(); Print("ExitStateChange"); Cr();
}


void PicturePrinter::EnterFontState() { 
	Indent(); Print("EnterFontState"); Cr();
}


void PicturePrinter::ExitFontState() { 
	Indent(); Print("ExitFontState"); Cr();
}


void PicturePrinter::SetOrigin(BPoint pt) { 
	Indent(); Print("SetOrigin"); Print(&pt); Cr();
}


void PicturePrinter::SetPenLocation(BPoint pt) { 
	Indent(); Print("SetPenLocation"); Print(&pt); Cr();
}


void PicturePrinter::SetDrawingMode(drawing_mode mode) { 
	Indent(); Print("SetDrawingMode"); 
	switch (mode) {
		case B_OP_COPY: Print("B_OP_COPY"); break;
		case B_OP_OVER: Print("B_OP_OVER"); break;
		case B_OP_ERASE: Print("B_OP_ERASE"); break;
		case B_OP_INVERT: Print("B_OP_INVERT"); break;
		case B_OP_SELECT: Print("B_OP_SELECT"); break;
		case B_OP_ALPHA: Print("B_OP_ALPHA"); break;
		case B_OP_MIN: Print("B_OP_MIN"); break;
		case B_OP_MAX: Print("B_OP_MAX"); break;
		case B_OP_ADD: Print("B_OP_ADD"); break;
		case B_OP_SUBTRACT: Print("B_OP_SUBTRACT"); break;
		case B_OP_BLEND: Print("B_OP_BLEND"); break;
		default: Print("Unknown mode: ", (float)mode);
	}
	Cr();
}


void PicturePrinter::SetLineMode(cap_mode capMode, join_mode joinMode, float miterLimit) { 
	Indent(); Print("SetLineMode"); 
	switch (capMode) {
		case B_BUTT_CAP:   Print("B_BUTT_CAP"); break;
		case B_ROUND_CAP:  Print("B_ROUND_CAP"); break;
		case B_SQUARE_CAP: Print("B_SQUARE_CAP"); break;
	}
	switch (joinMode) {
		case B_MITER_JOIN: Print("B_MITER_JOIN"); break;
		case B_ROUND_JOIN: Print("B_ROUND_JOIN"); break;
		case B_BUTT_JOIN: Print("B_BUTT_JOIN"); break;
		case B_SQUARE_JOIN: Print("B_SQUARE_JOIN"); break;
		case B_BEVEL_JOIN: Print("B_BEVEL_JOIN"); break;
	}
	Print("miterLimit", miterLimit);
	Cr();
}


void PicturePrinter::SetPenSize(float size) { 
	Indent(); Print("SetPenSize", size); Cr();
}


void PicturePrinter::SetForeColor(rgb_color color) { 
	Indent(); Print("SetForeColor"); Print(color); Cr();
}


void PicturePrinter::SetBackColor(rgb_color color) { 
	Indent(); Print("SetBackColor"); Print(color); Cr();
}

static bool compare(pattern a, pattern b) {
	for (int i = 0; i < 8; i ++) {
		if (a.data[i] != b.data[i]) return false;
	}
	return true;
}

void PicturePrinter::SetStipplePattern(pattern p) { 
	Indent(); Print("SetStipplePattern"); 
	if (compare(p, B_SOLID_HIGH)) Print("B_SOLID_HIGH");
	else if (compare(p, B_SOLID_LOW)) Print("B_SOLID_LOW");
	else if (compare(p, B_MIXED_COLORS)) Print("B_MIXED_COLORS");
	else {
		for (int i = 0; i < 8; i++) {
			printf("%2.2x ", (unsigned int)p.data[i]);
		}
	}
	Cr();
}


void PicturePrinter::SetScale(float scale) { 
	Indent(); Print("SetScale", scale); Cr();
}


void PicturePrinter::SetFontFamily(char *family) { 
	Indent(); Print("SetFontFamily"); Print(family); Cr();
}


void PicturePrinter::SetFontStyle(char *style) { 
	Indent(); Print("SetFontStyle"); Print(style); Cr();
}


void PicturePrinter::SetFontSpacing(int32 spacing) { 
	Indent(); Print("SetFontSpacing"); 
	switch(spacing) {
		case B_CHAR_SPACING: Print("B_CHAR_SPACING"); break;
		case B_STRING_SPACING: Print("B_STRING_SPACING"); break;
		case B_BITMAP_SPACING: Print("B_BITMAP_SPACING"); break;
		case B_FIXED_SPACING: Print("B_FIXED_SPACING"); break;
		default: Print("Unknown: ", (float)spacing);
	}
	Cr();
}


void PicturePrinter::SetFontSize(float size) { 
	Indent(); Print("SetFontSize", size); Cr();
}


void PicturePrinter::SetFontRotate(float rotation) { 
	Indent(); Print("SetFontRotation", rotation); Cr();
}


void PicturePrinter::SetFontEncoding(int32 encoding) { 
	Indent(); Print("SetFontEncoding");
	switch (encoding) {
		case B_UNICODE_UTF8: Print("B_UNICODE_UTF8"); break;
		case B_ISO_8859_1: Print("B_ISO_8859_1"); break;
		case B_ISO_8859_2: Print("B_ISO_8859_2"); break;
		case B_ISO_8859_3: Print("B_ISO_8859_3"); break;
		case B_ISO_8859_4: Print("B_ISO_8859_4"); break;
		case B_ISO_8859_5: Print("B_ISO_8859_5"); break;
		case B_ISO_8859_6: Print("B_ISO_8859_6"); break;
		case B_ISO_8859_7: Print("B_ISO_8859_7"); break;
		case B_ISO_8859_8: Print("B_ISO_8859_8"); break;
		case B_ISO_8859_9: Print("B_ISO_8859_9"); break;
		case B_ISO_8859_10: Print("B_ISO_8859_10"); break;
		case B_MACINTOSH_ROMAN: Print("B_MACINTOSH_ROMAN"); break;
		default: Print("Unknown:", (float)encoding);
	}
	Cr();
}

#define PRINT_FLAG(flag) \
  if (flags & flag) { f |= flag; Print(#flag); }

void PicturePrinter::SetFontFlags(int32 flags) { 
	Indent(); Print("SetFontFlags"); 
	int f = 0;
	if (flags == 0) Print("none set");
	PRINT_FLAG(B_DISABLE_ANTIALIASING);
	PRINT_FLAG(B_FORCE_ANTIALIASING);
	if (flags != f) printf("Unknown Additional Flags %" B_PRId32 "", flags & ~f);
	Cr();
}


void PicturePrinter::SetFontShear(float shear) { 
	Indent(); Print("SetFontShear", shear); Cr();
}


void PicturePrinter::SetFontFace(int32 flags) { 
	Indent(); Print("SetFontFace");
	int32 f = 0;
	if (flags == 0) Print("none set");
	PRINT_FLAG(B_REGULAR_FACE);
	PRINT_FLAG(B_BOLD_FACE);
	PRINT_FLAG(B_ITALIC_FACE);
	PRINT_FLAG(B_NEGATIVE_FACE);
	PRINT_FLAG(B_OUTLINED_FACE);
	PRINT_FLAG(B_UNDERSCORE_FACE);
	PRINT_FLAG(B_STRIKEOUT_FACE);
	if (flags != f) printf("Unknown Additional Flags %" B_PRId32 "", flags & ~f);
	Cr();
}


// Implementation of ShapePrinter
ShapePrinter::ShapePrinter(PicturePrinter* printer)
	: fPrinter(printer)
{
	fPrinter->IncIndent();
}

ShapePrinter::~ShapePrinter() {
	fPrinter->DecIndent();
}

status_t 
ShapePrinter::IterateBezierTo(int32 bezierCount, BPoint *control)
{
	fPrinter->Indent(); fPrinter->Print("BezierTo"); fPrinter->Cr();
	for (int32 i = 0; i < bezierCount; i++, control += 3) {
		fPrinter->Indent(1); 
		fPrinter->Print(i / 3.0);
		fPrinter->Print(&control[0]);
		fPrinter->Print(&control[1]);
		fPrinter->Print(&control[2]);
		fPrinter->Cr();
	}
	return B_OK;
}

status_t 
ShapePrinter::IterateClose(void)
{
	fPrinter->Indent(); fPrinter->Print("Close"); fPrinter->Cr();
	return B_OK;
}

status_t 
ShapePrinter::IterateLineTo(int32 lineCount, BPoint *linePoints)
{
	fPrinter->Indent(); fPrinter->Print("LineTo"); fPrinter->Cr();
	BPoint *p = linePoints;
	for (int32 i = 0; i < lineCount; i++) {
		fPrinter->Indent(1); fPrinter->Print(p); fPrinter->Cr();
		p++;
	}
	return B_OK;
}

status_t 
ShapePrinter::IterateMoveTo(BPoint *point)
{
	fPrinter->Indent(); fPrinter->Print("MoveTo", point); fPrinter->Cr();
	return B_OK;
}

