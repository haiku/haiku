/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Jérôme Duval, jerome.duval@free.fr
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef SERVER_FONT_H
#define SERVER_FONT_H


#include <Font.h>
#include <Rect.h>

#include "FontFamily.h"

class BShape;
class BString;


class ServerFont {
 public:
								ServerFont();
								ServerFont(FontStyle& style,
										   float size = 12.0,
										   float fRotation = 0.0,
										   float fShear = 90.0,
										   uint16 flags = 0,
										   uint8 spacing = B_CHAR_SPACING);
								ServerFont(const ServerFont& font);
	virtual						~ServerFont();

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
			void				SetEdges(edge_info info)
									{ fEdges = info; }
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
			void				SetFace(uint32 face);

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


			FT_Face				GetTransformedFace(bool rotate, bool shear) const;
			void				PutTransformedFace(FT_Face face) const;

			status_t			GetGlyphShapes(const char charArray[],
									int32 numChars, BShape *shapeArray[]) const;

			status_t			GetHasGlyphs(const char charArray[], 
									int32 numChars, bool hasArray[]) const;
			
			status_t			GetEdges(const char charArray[], 
									int32 numChars, edge_info edgeArray[]) const;

			status_t			GetEscapements(const char charArray[],
									int32 numChars, escapement_delta delta,
									BPoint escapementArray[],
									BPoint offsetArray[]) const;

			status_t			GetEscapements(const char charArray[],
									int32 numChars, escapement_delta delta,
									float widthArray[]) const;

			status_t			GetBoundingBoxesAsString(const char charArray[],
									int32 numChars, BRect rectArray[],
									bool stringEscapement, font_metric_mode mode,
									escapement_delta delta);

			status_t			GetBoundingBoxesForStrings(char *charArray[],
									int32 lengthArray[], int32 numStrings,
									BRect rectArray[], font_metric_mode mode,
									escapement_delta deltaArray[]);

			float				StringWidth(const char *string, int32 numChars) const;

			bool				Lock() const { return fStyle->Lock(); }
			void				Unlock() const { fStyle->Unlock(); }

			FT_Face				GetFTFace() const
									{ return fStyle->FreeTypeFace(); };
	
			BRect				BoundingBox();
			void				GetHeight(font_height& height) const;

			void				TruncateString(BString* inOut,
											   uint32 mode,
											   float width) const;

protected:
	friend class FontStyle;

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

#endif	/* SERVER_FONT_H */
