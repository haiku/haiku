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
//	File Name:		ServerFont.h
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Shadow BFont class
//  
//------------------------------------------------------------------------------
#ifndef SERVERFONT_H_
#define SERVERFONT_H_

#include <Rect.h>
#include <Font.h>
#include "FontFamily.h"

class ServerFont
{
public:
	ServerFont(void);
	ServerFont(FontStyle *style, float size=12.0, float frotation=0.0, float fshear=90.0,
			uint16 flags=0, uint8 spacing=B_CHAR_SPACING);
	ServerFont(const ServerFont &font);
	~ServerFont(void);
	font_direction Direction(void) const { return fDirection; }
	uint32 Encoding(void) const { return fEncoding; }
	edge_info Edges(void) const { return fEdges; }
	uint32 Flags(void) const { return fFlags; }
	uint32 Spacing(void) const { return fSpacing; }
	float Shear(void) const { return fshear; }
	float Rotation(void) const { return frotation; }
	float Size(void) const { return fSize; }
	uint16 Face(void) const { return fFace; }
	uint32 CountGlyphs(void) { return fStyle->GlyphCount(); }
	int32 CountTuned(void);
	font_file_format FileFormat(void);
	
	status_t SetFamilyAndStyle(const uint16 &familyID,const uint16 &styleID);
	status_t SetFamilyAndStyle(const uint32 &fontID);
	void SetDirection(const font_direction &dir) { fDirection=dir; }
	void SetEdges(const edge_info &info) { fEdges=info; }
	void SetEncoding(uint32 encoding) { fEncoding=encoding; }
	void SetFlags(const uint32 &value) { fFlags=value; }
	void SetSpacing(const uint32 &value) { fSpacing=value; }
	void SetShear(const float &value) { fshear=value; }
	void SetSize(const float &value) { fSize=value; }
	void SetRotation(const float &value) { frotation=value; }
	void SetFace(const uint32 &value) { fFace=value; }
	
	bool IsFixedWidth(void) { return fStyle->IsFixedWidth(); }
	bool IsScalable(void) { return fStyle->IsScalable(); }
	bool HasKerning(void) { return fStyle->HasKerning(); }
	bool HasTuned(void) { return fStyle->HasTuned(); }
	int32 TunedCount(void) { return fStyle->TunedCount(); }
	uint16 GlyphCount(void) { return fStyle->GlyphCount(); }
	uint16 CharMapCount(void) { return fStyle->CharMapCount(); }
	
	const char *GetStyle(void) const;
	const char *GetFamily(void) const;
	const char *GetPath(void) const { return fStyle->GetPath(); }
	uint16 StyleID(void) const { return fStyle->GetID(); }
	uint16 FamilyID(void) const { return fStyle->Family()->GetID(); }
	uint32 GetFamilyAndStyle(void);

	BRect BoundingBox(void);
	void Height(font_height *fh);

	ServerFont &operator=(const ServerFont& font);
	
protected:
	friend class FontStyle;
	FontStyle *fStyle;
	edge_info fEdges;
	float fSize, frotation, fshear;
	BRect fBounds;
	uint32 fFlags;
	uint32 fSpacing;
	uint16 fFace;
	font_direction fDirection;
	uint32 fEncoding;
};

#endif

