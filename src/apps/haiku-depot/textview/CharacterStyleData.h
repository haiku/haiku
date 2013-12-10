/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef CHARACTER_STYLE_DATA_H
#define CHARACTER_STYLE_DATA_H

#include <Font.h>
#include <Referenceable.h>


enum {
	STRIKE_OUT_NONE		= 0,
	STRIKE_OUT_SINGLE	= 1,
};


enum {
	UNDERLINE_NONE		= 0,
	UNDERLINE_SINGLE	= 1,
	UNDERLINE_DOUBLE	= 2,
	UNDERLINE_WIGGLE	= 3,
	UNDERLINE_DOTTED	= 4,
};


class CharacterStyleData;
typedef BReference<CharacterStyleData>	CharacterStyleDataRef;


// You cannot modify a CharacterStyleData object once it has been
// created. Instead, the setter methods return a new object that equals
// the object on which the method has been called, except for the changed
// property. If the property already has the requested value, then a
// reference to the same object is returned.
class CharacterStyleData : public BReferenceable {
public:
								CharacterStyleData();
								CharacterStyleData(
									const CharacterStyleData& other);

			bool				operator==(
									const CharacterStyleData& other) const;
			bool				operator!=(
									const CharacterStyleData& other) const;

			CharacterStyleDataRef SetFont(const BFont& font);
	inline	const BFont&		Font() const
									{ return fFont; }

			CharacterStyleDataRef SetAscent(float ascent);
			
			// Returns the ascent of the configured font, unless the ascent
			// has been overridden by a fixed value with SetAscent().
			float				Ascent() const;

			CharacterStyleDataRef SetDescent(float descent);

			// Returns the descent of the configured font, unless the descent
			// has been overridden by a fixed value with SetDescent().
			float				Descent() const;

			CharacterStyleDataRef SetWidth(float width);
	inline	float				Width() const
									{ return fWidth; }

			CharacterStyleDataRef SetGlyphSpacing(float glyphSpacing);
	inline	float				GlyphSpacing() const
									{ return fGlyphSpacing; }

			CharacterStyleDataRef SetForegroundColor(rgb_color color);
	inline	rgb_color			ForegroundColor() const
									{ return fFgColor; }

			CharacterStyleDataRef SetBackgroundColor(rgb_color color);
	inline	rgb_color			BackgroundColor() const
									{ return fBgColor; }

			CharacterStyleDataRef SetStrikeOutColor(rgb_color color);
	inline	rgb_color			StrikeOutColor() const
									{ return fStrikeOutColor; }

			CharacterStyleDataRef SetUnderlineColor(rgb_color color);
	inline	rgb_color			UnderlineColor() const
									{ return fUnderlineColor; }

			CharacterStyleDataRef SetStrikeOut(uint8 strikeOut);
	inline	uint8				StrikeOut() const
									{ return fStrikeOutStyle; }

			CharacterStyleDataRef SetUnderline(uint8 underline);
	inline	uint8				Underline() const
									{ return fUnderlineStyle; }

private:
			CharacterStyleData&	operator=(const CharacterStyleData& other);

private:
			BFont				fFont;

			// The following three values override glyph metrics unless -1
			// This is useful when you want to have a place-holder character
			// in the text. You would assign this character a CharacterStyle
			// which defines ascent, descent and width to fixed values, thereby
			// controlling the exact dimensions of the character, regardless of
			// font size. Instead of the character, one could render an icon
			// that is layouted within the text.
			float				fAscent;
			float				fDescent;
			float				fWidth;

			// Additional spacing to be applied between glyphs.
			float				fGlyphSpacing;

			rgb_color			fFgColor;
			rgb_color			fBgColor;

			rgb_color			fStrikeOutColor;
			rgb_color			fUnderlineColor;

			uint8				fStrikeOutStyle;
			uint8				fUnderlineStyle;
};



#endif // CHARACTER_STYLE_DATA_H
