//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
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

class FontStyle;

class ServerFont
{
public:
	ServerFont(void);
	ServerFont(FontStyle *fstyle, float fsize=12.0, float frotation=0.0, float fshear=90.0,
			uint16 flags=0, uint8 spacing=B_CHAR_SPACING);
	ServerFont(const ServerFont &font);
	~ServerFont(void);
	font_direction Direction(void) { return fdirection; }
	uint8 Encoding(void) { return fencoding; }
	edge_info Edges(void) { return fedges; }
	uint32 Flags(void) { return fflags; }
	uint8 Spacing(void) { return fspacing; }
	float Shear(void) { return fshear; }
	float Rotation(void) { return frotation; }
	float Size(void) { return fsize; }
	uint16 Face(void) { return fface; }
	uint32 CountGlyphs(void);
	int32 CountTuned(void);
	font_file_format FileFormat(void);
	FontStyle *Style(void) { return fstyle; }

	void SetDirection(const font_direction &dir) { fdirection=dir; }
	void SetEdges(const edge_info &info) { fedges=info; }
	void SetEncoding(uint8 encoding) { fencoding=encoding; }
	void SetFlags(const uint32 &value) { fflags=value; }
	void SetSpacing(const uint8 &value) { fspacing=value; }
	void SetShear(const float &value) { fshear=value; }
	void SetSize(const float &value) { fsize=value; }
	void SetRotation(const float &value) { frotation=value; }
	void SetFace(const uint16 &value) { fface=value; }
	
	BRect BoundingBox(void);
	void Height(font_height *fh);

		ServerFont&			operator=(const ServerFont& font);
protected:
	friend class FontStyle;
	FontStyle *fstyle;
	font_height fheight;
	edge_info fedges;
	float fsize, frotation, fshear;
	BRect fbounds;
	uint32 fflags;
	uint8 fspacing;
	uint16 fface;
	font_direction fdirection;
	uint8 ftruncate;
	uint8 fencoding;
};

#endif
/*
 @log
 	* added '=' operator
*/
