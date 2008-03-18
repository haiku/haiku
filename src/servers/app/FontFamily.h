/*
 * Copyright 2001-2008, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef FONT_FAMILY_H_
#define FONT_FAMILY_H_


#include <ObjectList.h>
#include <String.h>

#include "FontStyle.h"


/*!
	\class FontFamily FontFamily.h
	\brief Class representing a collection of similar styles

	FontFamily objects bring together many styles of the same face, such as
	Arial Roman, Arial Italic, Arial Bold, etc.
*/
class FontFamily {
public:
						FontFamily(const char* name, uint16 id);
	virtual				~FontFamily();

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
			FontStyle*	_FindStyle(const char* name) const;

			BString		fName;
			BObjectList<FontStyle> fStyles;
			uint16		fID;
			uint16		fNextID;
			uint32		fFlags;
};

#endif	// FONT_FAMILY_H_
