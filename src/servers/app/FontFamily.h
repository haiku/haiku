/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef FONT_FAMILY_H_
#define FONT_FAMILY_H_


#include <Font.h>
#include <Locker.h>
#include <Node.h>
#include <ObjectList.h>
#include <Rect.h>
#include <String.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include "ReferenceCounting.h"
#include "HashTable.h"


struct node_ref;
class FontFamily;
class ServerFont;

enum font_format {
	FONT_TRUETYPE = 0,
	FONT_TYPE_1,
	FONT_OPENTYPE,
	FONT_BDF,
	FONT_CFF,
	FONT_CID,
	FONT_PCF,
	FONT_PFR,
	FONT_SFNT,
	FONT_TYPE_42,
	FONT_WINFONT,
};


class FontKey : public Hashable {
	public:
		FontKey(uint16 familyID, uint16 styleID)
			: fHash(familyID | (styleID << 16UL))
		{
		}

		virtual uint32	Hash() const
							{ return fHash; }
		virtual bool	CompareTo(Hashable& other) const
							{ return fHash == other.Hash(); }

	private:
		uint32	fHash;
};


/*!
	\class FontStyle FontFamily.h
	\brief Object used to represent a font style
	
	FontStyle objects help abstract a lot of the font engine details while
	still offering plenty of information the style in question.
*/
class FontStyle : public ReferenceCounting, public Hashable/*, public BLocker*/ {
	public:
						FontStyle(node_ref& nodeRef, const char* path, FT_Face face);
		virtual			~FontStyle();

		virtual uint32	Hash() const;
		virtual bool	CompareTo(Hashable& other) const;

		const node_ref& NodeRef() const { return fNodeRef; }

		bool			Lock();
		void			Unlock();

/*!
	\fn bool FontStyle::IsFixedWidth(void)
	\brief Determines whether the font's character width is fixed
	\return true if fixed, false if not
*/
		bool			IsFixedWidth() const
							{ return fFreeTypeFace->face_flags & FT_FACE_FLAG_FIXED_WIDTH; }
/*!
	\fn bool FontStyle::IsScalable(void)
	\brief Determines whether the font can be scaled to any size
	\return true if scalable, false if not
*/
		bool			IsScalable() const
							{ return fFreeTypeFace->face_flags & FT_FACE_FLAG_SCALABLE; }
/*!
	\fn bool FontStyle::HasKerning(void)
	\brief Determines whether the font has kerning information
	\return true if kerning info is available, false if not
*/
		bool			HasKerning() const
							{ return fFreeTypeFace->face_flags & FT_FACE_FLAG_KERNING; }
/*!
	\fn bool FontStyle::HasTuned(void)
	\brief Determines whether the font contains strikes
	\return true if it has strikes included, false if not
*/
		bool			HasTuned() const
							{ return fFreeTypeFace->num_fixed_sizes > 0; }
/*!
	\fn bool FontStyle::TunedCount(void)
	\brief Returns the number of strikes the style contains
	\return The number of strikes the style contains
*/
		int32			TunedCount() const
							{ return fFreeTypeFace->num_fixed_sizes; }
/*!
	\fn bool FontStyle::GlyphCount(void)
	\brief Returns the number of glyphs in the style
	\return The number of glyphs the style contains
*/
		uint16			GlyphCount() const
							{ return fFreeTypeFace->num_glyphs; }
/*!
	\fn bool FontStyle::CharMapCount(void)
	\brief Returns the number of character maps the style contains
	\return The number of character maps the style contains
*/
		uint16			CharMapCount() const
							{ return fFreeTypeFace->num_charmaps; }

		const char*		Name() const
							{ return fName.String(); }
		FontFamily*		Family() const
							{ return fFamily; }
		uint16			ID() const
							{ return fID; }
		uint32			Flags() const;

		uint16			Face() const
							{ return fFace; }
		uint16			PreservedFace(uint16) const;

		const char*		Path() const;
		void			GetHeight(float size, font_height &heigth) const;
		font_direction	Direction() const
							{ return B_FONT_LEFT_TO_RIGHT; }
		font_file_format FileFormat() const
							{ return B_TRUETYPE_WINDOWS; }

		FT_Face			FreeTypeFace() const
							{ return fFreeTypeFace; }

		status_t		UpdateFace(FT_Face face);

// TODO: Re-enable when I understand how the FT2 Cache system changed from
// 2.1.4 to 2.1.8
//		int16			ConvertToUnicode(uint16 c);

	private:
		friend class FontFamily;
		uint16			_TranslateStyleToFace(const char *name) const;
		void			_SetFontFamily(FontFamily* family, uint16 id);

	private:
		FT_Face			fFreeTypeFace;
		BString			fName;
		BString			fPath;
		node_ref		fNodeRef;

		FontFamily*		fFamily;
		uint16			fID;

		BRect			fBounds;

		font_height		fHeight;
		uint16			fFace;
};

/*!
	\class FontFamily FontFamily.h
	\brief Class representing a collection of similar styles
	
	FontFamily objects bring together many styles of the same face, such as
	Arial Roman, Arial Italic, Arial Bold, etc.
*/
class FontFamily {
	public:
		FontFamily(const char* name, uint16 id);
		virtual ~FontFamily();

		const char*	Name() const;

		bool		AddStyle(FontStyle* style);
		bool		RemoveStyle(FontStyle* style);

		FontStyle*	GetStyle(const char* style) const;
		FontStyle*	GetStyleMatchingFace(uint16 face) const;
		FontStyle*	GetStyleByID(uint16 face) const;

		uint16		ID() const
						{ return fID; }
		uint32		Flags();

		bool		HasStyle(const char* style) const;
		int32		CountStyles() const;
		FontStyle*	StyleAt(int32 index) const;

	private:
		BString		fName;
		BObjectList<FontStyle> fStyles;
		uint16		fID;
		uint16		fNextID;
		uint32		fFlags;
};

#endif	/* FONT_FAMILY_H_ */
