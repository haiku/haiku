/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PARAGRAPH_LAYOUT_H
#define PARAGRAPH_LAYOUT_H

#include <Font.h>
#include <Referenceable.h>
#include <String.h>

#include "GlyphInfo.h"
#include "Paragraph.h"


class BView;


class LayoutLine {
public:
	LayoutLine()
		:
		textOffset(0),
		y(0.0f),
		height(0.0f),
		maxAscent(0.0f),
		maxDescent(0.0f)
	{
	}
	
	LayoutLine(const LayoutLine& other)
		:
		textOffset(other.textOffset),
		y(other.y),
		height(other.height),
		maxAscent(other.maxAscent),
		maxDescent(other.maxDescent),
		layoutedSpans(other.layoutedSpans)
	{
	}

	LayoutLine& operator=(const LayoutLine& other)
	{
		textOffset = other.textOffset;
		y = other.y;
		height = other.height;
		maxAscent = other.maxAscent;
		maxDescent = other.maxDescent;
		layoutedSpans = other.layoutedSpans;
		return *this;
	}

	bool operator==(const LayoutLine& other) const
	{
		return textOffset == other.textOffset
			&& y == other.y
			&& height == other.height
			&& maxAscent == other.maxAscent
			&& maxDescent == other.maxDescent
			&& layoutedSpans == otner.layoutedSpans;
	}

	bool operator!=(const LayoutLine& other) const
	{
		return !(*this == other);
	}

public:
	int				textOffset;
	float			y;
	float			height;
	float			maxAscent;
	float			maxDescent;

	TextSpanList	layoutedSpans;
};


typedef List<LayoutLine, false> LayoutLineList;


class ParagraphLayout : public BReferenceable {
public:
								ParagraphLayout();
								ParagraphLayout(const Paragraph& paragraph);
								ParagraphLayout(const ParagraphLayout& other);
	virtual						~ParagraphLayout();

			void				SetParagraph(const ::Paragraph& paragraph);
			const ::Paragraph&	Paragraph() const
									{ return fParagraph; }

			void				SetWidth(float width);
			float				Width() const
									{ return fWidth; }

			float				Height();
			void				Draw(BView* view, const BPoint& offset);

private:
			void				_ValidateLayout();
			void				_Layout();

			void				_Init();
			bool				_AppendGlyphInfos(const TextSpan& span);
			bool				_AppendGlyphInfo(uint32 charCode,
									const TextStyleRef& span);

			void				_GetGlyphAdvance(int offset,
									float& advanceX, float& advanceY,
									float escapementArray[]) const;
			
			void				_FinalizeLine(int lineStart, int lineEnd,
									int lineIndex, float y, float& lineHeight);

			void				_DrawLine(BView* view,
									const LayoutLine& line) const;
			void				_DrawSpan(BView* view, const TextSpan& span,
									int textOffset) const;

private:
			TextSpanList		fTextSpans;

			float				fWidth;
			
			bool				fLayoutValid;

			GlyphInfo*			fGlyphInfoBuffer;
			int					fGlpyhInfoBufferSize;
			int					fGlyphInfoCount;

			LayoutLineList		fLayoutLines;
};

#endif // PARAGRAPH_LAYOUT_H
