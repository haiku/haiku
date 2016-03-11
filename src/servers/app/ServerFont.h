/*
 * Copyright 2001-2008, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Jérôme Duval, jerome.duval@free.fr
 *		Axel Dörfler, axeld@pinc-software.de
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef SERVER_FONT_H
#define SERVER_FONT_H


#include <Font.h>
#include <Rect.h>

#include "FontFamily.h"
#include "GlobalSubpixelSettings.h"
#include "Transformable.h"

class BShape;
class BString;


class ServerFont {
 public:
								ServerFont();
								ServerFont(FontStyle& style,
										   float size = 12.0,
										   float rotation = 0.0,
										   float shear = 90.0,
										   float falseBoldWidth = 0.0,
										   uint16 flags = 0,
										   uint8 spacing = B_BITMAP_SPACING);
								ServerFont(const ServerFont& font);
	virtual						~ServerFont();

			ServerFont			&operator=(const ServerFont& font);
			bool				operator==(const ServerFont& other) const;

			font_direction		Direction() const
									{ return fDirection; }
			uint32				Encoding() const
									{ return fEncoding; }
			uint32				Flags() const
									{ return fFlags; }
			uint32				Spacing() const
									{ return fSpacing; }
			float				Shear() const
									{ return fShear; }
			float				Rotation() const
									{ return fRotation; }
			float				FalseBoldWidth() const
									{ return fFalseBoldWidth; }
			float				Size() const
									{ return fSize; }
			uint16				Face() const
									{ return fFace; }
			uint32				CountGlyphs()
									{ return fStyle->GlyphCount(); }
			int32				CountTuned();

			font_file_format	FileFormat();

			const char*			Style() const;
			const char*			Family() const;
			const char*			Path() const
									{ return fStyle->Path(); }

			void				SetStyle(FontStyle* style);
			status_t			SetFamilyAndStyle(uint16 familyID,
												  uint16 styleID);
			status_t			SetFamilyAndStyle(uint32 fontID);

			uint16				StyleID() const
									{ return fStyle->ID(); }
			uint16				FamilyID() const
									{ return fStyle->Family()->ID(); }
			uint32				GetFamilyAndStyle() const;

			void				SetDirection(font_direction dir)
									{ fDirection = dir; }
			void				SetEncoding(uint32 encoding)
									{ fEncoding = encoding; }
			void				SetFlags(uint32 value)
									{ fFlags = value; }
			void				SetSpacing(uint32 value)
									{ fSpacing = value; }
			void				SetShear(float value)
									{ fShear = value; }
			void				SetSize(float value)
									{ fSize = value; }
			void				SetRotation(float value)
									{ fRotation = value; }
			void				SetFalseBoldWidth(float value)
									{ fFalseBoldWidth = value; }
			status_t			SetFace(uint16 face);

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
	inline	bool				Hinting() const;

			status_t			GetGlyphShapes(const char charArray[],
									int32 numChars, BShape *shapeArray[]) const;

			status_t			GetHasGlyphs(const char charArray[],
									int32 numBytes, bool hasArray[]) const;

			status_t			GetEdges(const char charArray[], int32 numBytes,
									edge_info edgeArray[]) const;

			status_t			GetEscapements(const char charArray[],
									int32 numBytes, int32 numChars,
									escapement_delta delta,
									BPoint escapementArray[],
									BPoint offsetArray[]) const;

			status_t			GetEscapements(const char charArray[],
									int32 numBytes, int32 numChars,
									escapement_delta delta,
									float widthArray[]) const;

			status_t			GetBoundingBoxes(const char charArray[],
									int32 numBytes, BRect rectArray[],
									bool stringEscapement,
									font_metric_mode mode,
									escapement_delta delta,
									bool asString);

			status_t			GetBoundingBoxesForStrings(char *charArray[],
									int32 lengthArray[], int32 numStrings,
									BRect rectArray[], font_metric_mode mode,
									escapement_delta deltaArray[]);

			float				StringWidth(const char *string,
									int32 numBytes,
									const escapement_delta* delta = NULL) const;

			bool				Lock() const { return fStyle->Lock(); }
			void				Unlock() const { fStyle->Unlock(); }

//			FT_Face				GetFTFace() const
//									{ return fStyle->FreeTypeFace(); };

			BRect				BoundingBox();
			void				GetHeight(font_height& height) const;

			void				TruncateString(BString* inOut,
											   uint32 mode,
											   float width) const;

			Transformable		EmbeddedTransformation() const;
			status_t 			GetUnicodeBlocks(unicode_block &blocksForFont);
			status_t			IncludesUnicodeBlock(uint32 start, uint32 end, bool &hasBlock);

protected:
	friend class FontStyle;
			FT_Face				GetTransformedFace(bool rotate,
									bool shear) const;
			void				PutTransformedFace(FT_Face face) const;
			
			FontStyle*			fStyle;
			float				fSize;
			float				fRotation;
			float				fShear;
			float				fFalseBoldWidth;
			BRect				fBounds;
			uint32				fFlags;
			uint32				fSpacing;
			font_direction		fDirection;
			uint16				fFace;
			uint32				fEncoding;
};

inline bool ServerFont::Hinting() const
{
	switch (gDefaultHintingMode) {
		case HINTING_MODE_OFF:
			return false;
		default:
		case HINTING_MODE_ON:
			return true;
		case HINTING_MODE_MONOSPACED_ONLY:
			return IsFixedWidth();
	}
}

#endif	/* SERVER_FONT_H */
