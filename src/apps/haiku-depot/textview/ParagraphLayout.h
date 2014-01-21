/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PARAGRAPH_LAYOUT_H
#define PARAGRAPH_LAYOUT_H

#include <Font.h>
#include <Referenceable.h>
#include <String.h>

#include "Paragraph.h"
#include "CharacterStyle.h"


class BView;


class GlyphInfo {
public:
	GlyphInfo()
		:
		charCode(0),
		x(0.0f),
		width(0.0f),
		lineIndex(0)
	{
	}

	GlyphInfo(uint32 charCode, float x, float width, int32 lineIndex)
		:
		charCode(charCode),
		x(x),
		width(width),
		lineIndex(lineIndex)
	{
	}

	GlyphInfo(const GlyphInfo& other)
		:
		charCode(other.charCode),
		x(other.x),
		width(other.width),
		lineIndex(other.lineIndex)
	{
	}

	GlyphInfo& operator=(const GlyphInfo& other)
	{
		charCode = other.charCode;
		x = other.x;
		width = other.width;
		lineIndex = other.lineIndex;
		return *this;
	}

	bool operator==(const GlyphInfo& other) const
	{
		return charCode == other.charCode
			&& x == other.x
			&& width == other.width
			&& lineIndex == other.lineIndex;
	}

	bool operator!=(const GlyphInfo& other) const
	{
		return !(*this == other);
	}

public:
	uint32					charCode;

	float					x;
	float					width;

	int32					lineIndex;
};


typedef List<GlyphInfo, false> GlyphInfoList;


class LineInfo {
public:
	LineInfo()
		:
		textOffset(0),
		y(0.0f),
		height(0.0f),
		maxAscent(0.0f),
		maxDescent(0.0f),
		extraGlyphSpacing(0.0f),
		extraWhiteSpacing(0.0f),
		layoutedSpans()
	{
	}

	LineInfo(int32 textOffset, float y, float height, float maxAscent,
		float maxDescent)
		:
		textOffset(textOffset),
		y(y),
		height(height),
		maxAscent(maxAscent),
		maxDescent(maxDescent),
		extraGlyphSpacing(0.0f),
		extraWhiteSpacing(0.0f),
		layoutedSpans()
	{
	}

	LineInfo(const LineInfo& other)
		:
		textOffset(other.textOffset),
		y(other.y),
		height(other.height),
		maxAscent(other.maxAscent),
		maxDescent(other.maxDescent),
		extraGlyphSpacing(other.extraGlyphSpacing),
		extraWhiteSpacing(other.extraWhiteSpacing),
		layoutedSpans(other.layoutedSpans)
	{
	}

	LineInfo& operator=(const LineInfo& other)
	{
		textOffset = other.textOffset;
		y = other.y;
		height = other.height;
		maxAscent = other.maxAscent;
		maxDescent = other.maxDescent;
		extraGlyphSpacing = other.extraGlyphSpacing;
		extraWhiteSpacing = other.extraWhiteSpacing;
		layoutedSpans = other.layoutedSpans;
		return *this;
	}

	bool operator==(const LineInfo& other) const
	{
		return textOffset == other.textOffset
			&& y == other.y
			&& height == other.height
			&& maxAscent == other.maxAscent
			&& maxDescent == other.maxDescent
			&& extraGlyphSpacing == other.extraGlyphSpacing
			&& extraWhiteSpacing == other.extraWhiteSpacing
			&& layoutedSpans == other.layoutedSpans;
	}

	bool operator!=(const LineInfo& other) const
	{
		return !(*this == other);
	}

public:
	int32			textOffset;

	float			y;
	float			height;

	float			maxAscent;
	float			maxDescent;

	float			extraGlyphSpacing;
	float			extraWhiteSpacing;

	TextSpanList	layoutedSpans;
};


typedef List<LineInfo, false> LineInfoList;


class ParagraphLayout : public BReferenceable {
public:
								ParagraphLayout();
								ParagraphLayout(const Paragraph& paragraph);
								ParagraphLayout(const ParagraphLayout& other);
	virtual						~ParagraphLayout();

			void				SetParagraph(const Paragraph& paragraph);
			const ParagraphStyle& Style() const
									{ return fParagraphStyle; }

			void				SetWidth(float width);
			float				Width() const
									{ return fWidth; }

			float				Height();
			void				Draw(BView* view, const BPoint& offset);

			int32				CountGlyphs() const;
			int32				CountLines();

			int32				LineIndexForOffset(int32 textOffset);
			int32				FirstOffsetOnLine(int32 lineIndex);
			int32				LastOffsetOnLine(int32 lineIndex);

			void				GetLineBounds(int32 lineIndex,
									float& x1, float& y1,
									float& x2, float& y2);

			void				GetTextBounds(int32 textOffset,
									float& x1, float& y1,
									float& x2, float& y2);

			int32				TextOffsetAt(float x, float y,
									bool& rightOfCenter);

private:
			void				_Init();

			void				_ValidateLayout();
			void				_Layout();
			void				_ApplyAlignment();

			bool				_AppendGlyphInfos(const TextSpan& span);
			bool				_AppendGlyphInfo(uint32 charCode,
									float advanceX,
									const CharacterStyle& style);

			bool				_FinalizeLine(int lineStart, int lineEnd,
									int lineIndex, float y, float& lineHeight);

			void				_IncludeStyleInLine(LineInfo& line,
									const CharacterStyle& style);

			void				_DrawLine(BView* view, const BPoint& offset,
									const LineInfo& line) const;
			void				_DrawSpan(BView* view, BPoint offset,
									const TextSpan& span,
									int32 textOffset) const;

			void				_GetEmptyLayoutBounds(float& x1, float& y1,
									float& x2, float& y2) const;

private:
			TextSpanList		fTextSpans;
			ParagraphStyle		fParagraphStyle;

			float				fWidth;
			bool				fLayoutValid;

			GlyphInfoList		fGlyphInfos;
			LineInfoList		fLineInfos;
};


typedef BReference<ParagraphLayout> ParagraphLayoutRef;


#endif // PARAGRAPH_LAYOUT_H
