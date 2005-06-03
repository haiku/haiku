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

#include <Font.h>
#include <Rect.h>

#include "FontFamily.h"

class BShape;
class BString;

class ServerFont {
 public:
								ServerFont();
								ServerFont(FontStyle* style,
										   float size = 12.0,
										   float fRotation = 0.0,
										   float fShear = 90.0,
										   uint16 flags = 0,
										   uint8 spacing = B_CHAR_SPACING);
								ServerFont(const ServerFont& font);
	virtual						~ServerFont();

	// TODO: make more advanced...
			status_t			InitCheck() const
									{ return fStyle ? B_OK : B_NO_INIT; }


			ServerFont			&operator=(const ServerFont& font);
	

			font_direction		Direction() const
									{ return fDirection; }
			uint32				Encoding() const
									{ return fEncoding; }
			edge_info			Edges() const
									{ return fEdges; }
			uint32				Flags() const
									{ return fFlags; }
			uint32				Spacing() const
									{ return fSpacing; }
			float				Shear() const
									{ return fShear; }
			float				Rotation() const
									{ return fRotation; }
			float				Size() const
									{ return fSize; }
			uint16				Face() const
									{ return fFace; }
			uint32				CountGlyphs()
									{ return fStyle->GlyphCount(); }
			int32				CountTuned();


			font_file_format	FileFormat();

			const char*			GetStyle() const;
			const char*			GetFamily() const;
			const char*			GetPath() const
									{ return fStyle->GetPath(); }
	
			status_t			SetFamilyAndStyle(uint16 familyID,
												  uint16 styleID);
			status_t			SetFamilyAndStyle(uint32 fontID);

			uint16				StyleID() const
									{ return fStyle->GetID(); }
			uint16				FamilyID() const
									{ return fStyle->Family()->GetID(); }
			uint32				GetFamilyAndStyle() const;


			void				SetDirection(const font_direction& dir)
									{ fDirection = dir; }
			void				SetEdges(const edge_info& info)
									{ fEdges = info; }
			void				SetEncoding(uint32 encoding)
									{ fEncoding = encoding; }
			void				SetFlags(const uint32& value)
									{ fFlags = value; }
			void				SetSpacing(const uint32& value)
									{ fSpacing = value; }
			void				SetShear(const float& value)
									{ fShear = value; }
			void				SetSize(const float& value)
									{ fSize = value; }
			void				SetRotation(const float& value)
									{ fRotation = value; }
			void				SetFace(const uint32& value)
									{ fFace = value; }
	
			bool				IsFixedWidth() const
									{ return fStyle->IsFixedWidth(); }
			bool				IsScalable() const
									{ return fStyle->IsScalable(); }
			bool				HasKerning() const
									{ return fStyle->HasKerning(); }
			bool				HasTuned() const
									{ return fStyle->HasTuned(); }
			int32				TunedCount() const
									{ return fStyle->TunedCount(); }
			uint16				GlyphCount() const
									{ return fStyle->GlyphCount(); }
			uint16				CharMapCount() const
									{ return fStyle->CharMapCount(); }

			BShape**			GetGlyphShapes(const char charArray[],
											   int32 numChars) const;

			BPoint*				GetEscapements(const char charArray[],
											   int32 numChars,
											   BPoint offsetArray[]) const;
			bool				GetEscapements(const char charArray[],
											   int32 numChars,
											   float widthArray[],
											   escapement_delta delta) const;

			float				StringWidth(const char* string, int32 numBytes) const;

			FT_Face				GetFTFace() const
									{ return fStyle->GetFTFace(); };
	
			BRect				BoundingBox();
			void				Height(font_height* fh) const;

			void				TruncateString(BString* inOut,
											   uint32 mode,
											   float width) const;

protected:
	friend class FontStyle;
			void				_SetStyle(FontStyle* style);


			FontStyle*			fStyle;
			edge_info			fEdges;
			float				fSize;
			float				fRotation;
			float				fShear;
			BRect				fBounds;
			uint32				fFlags;
			uint32				fSpacing;
			font_direction		fDirection;
			uint16				fFace;
			uint32				fEncoding;
};

#endif

