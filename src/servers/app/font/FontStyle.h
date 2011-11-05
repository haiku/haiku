/*
 * Copyright 2001-2008, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef FONT_STYLE_H_
#define FONT_STYLE_H_


#include <Font.h>
#include <Locker.h>
#include <Node.h>
#include <ObjectList.h>
#include <Path.h>
#include <Rect.h>
#include <String.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include "ReferenceCounting.h"
#include "HashTable.h"


struct node_ref;
class FontFamily;
class ServerFont;


class FontKey : public Hashable {
	public:
		FontKey(uint16 familyID, uint16 styleID)
			: fHash(familyID | (styleID << 16UL))
		{
		}
		virtual ~FontKey() {};

		virtual uint32	Hash() const
							{ return fHash; }
		virtual bool	CompareTo(Hashable& other) const
							{ return fHash == other.Hash(); }

	private:
		uint32	fHash;
};


/*!
	\class FontStyle FontStyle.h
	\brief Object used to represent a font style

	FontStyle objects help abstract a lot of the font engine details while
	still offering plenty of information the style in question.
*/
class FontStyle : public ReferenceCounting, public Hashable {
	public:
						FontStyle(node_ref& nodeRef, const char* path,
							FT_Face face);
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
							{ return FT_IS_FIXED_WIDTH(fFreeTypeFace); }


/*	\fn bool FontStyle::IsFullAndHalfFixed()
	\brief Determines whether the font has 2 different, fixed, widths.
	\return false (for now)
*/
		bool			IsFullAndHalfFixed() const
							{ return false; };

/*!
	\fn bool FontStyle::IsScalable(void)
	\brief Determines whether the font can be scaled to any size
	\return true if scalable, false if not
*/
		bool			IsScalable() const
							{ return FT_IS_SCALABLE(fFreeTypeFace); }
/*!
	\fn bool FontStyle::HasKerning(void)
	\brief Determines whether the font has kerning information
	\return true if kerning info is available, false if not
*/
		bool			HasKerning() const
							{ return FT_HAS_KERNING(fFreeTypeFace); }
/*!
	\fn bool FontStyle::HasTuned(void)
	\brief Determines whether the font contains strikes
	\return true if it has strikes included, false if not
*/
		bool			HasTuned() const
							{ return FT_HAS_FIXED_SIZES(fFreeTypeFace); }
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
		void			UpdatePath(const node_ref& parentNodeRef);

		void			GetHeight(float size, font_height &heigth) const;
		font_direction	Direction() const
							{ return B_FONT_LEFT_TO_RIGHT; }
		font_file_format FileFormat() const
							{ return B_TRUETYPE_WINDOWS; }

		FT_Face			FreeTypeFace() const
							{ return fFreeTypeFace; }

		status_t		UpdateFace(FT_Face face);

	private:
		friend class FontFamily;
		uint16			_TranslateStyleToFace(const char *name) const;
		void			_SetFontFamily(FontFamily* family, uint16 id);

	private:
		FT_Face			fFreeTypeFace;
		BString			fName;
		BPath			fPath;
		node_ref		fNodeRef;

		FontFamily*		fFamily;
		uint16			fID;

		BRect			fBounds;

		font_height		fHeight;
		uint16			fFace;
};

#endif	// FONT_STYLE_H_
