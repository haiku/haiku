/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef TEXT_STYLE_H
#define TEXT_STYLE_H

#include <Font.h>
#include <GraphicsDefs.h>
#include <Referenceable.h>


class TextStyle : public BReferenceable {
public:
	TextStyle();
	TextStyle(const BFont& font, float ascent, float descent, float width,
		float glyphSpacing,
		rgb_color fgColor, rgb_color bgColor, bool strikeOut,
		rgb_color strikeOutColor, bool underline,
		uint32 underlineStyle, rgb_color underlineColor);

	TextStyle(const TextStyle& other);

	bool operator==(const TextStyle& other) const;

public:
	BFont					font;

	// The following three values override glyph metrics unless -1
	float					ascent;
	float					descent;
	float					width;
	float					glyphSpacing;

	rgb_color				fgColor;
	rgb_color				bgColor;

	bool					strikeOut;
	rgb_color				strikeOutColor;

	bool					underline;
	uint32					underlineStyle;
	rgb_color				underlineColor;
};


typedef BReference<TextStyle> TextStyleRef;


#endif // TEXT_STYLE_H
