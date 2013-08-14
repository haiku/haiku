/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef TEXT_LAYOUT_H
#define TEXT_LAYOUT_H

#include <Font.h>
#include <Referenceable.h>
#include <String.h>

#include "GlyphInfo.h"
#include "LineInfo.h"


class BView;


class TextLayout : public BReferenceable {
public:
								TextLayout();
								TextLayout(const TextLayout& other);
	virtual						~TextLayout();

			void				SetText(const BString& text);
			const BString&		Text() const
									{ return fText; }

			void				SetFont(const BFont& font);
			const BFont&		Font() const
									{ return fDefaultFont; }

			void				SetGlyphSpacing(float glyphSpacing);
			float				GlyphSpacing() const
									{ return fGlyphSpacing; }

			void				SetFirstLineInset(float inset);
			float				FirstLineInset() const
									{ return fFirstLineInset; }

			void				SetLineInset(float inset);
			float				LineInset() const
									{ return fLineInset; }

			void				SetLineSpacing(float spacing);
			float				LineSpacing() const
									{ return fLineSpacing; }

			void				SetWidth(float width);
			float				Width() const
									{ return fWidth; }

			float				Height();
			void				Draw(BView* view, const BPoint& offset);

private:
			void				_ValidateLayout();
			void				_Layout();
			void				_GetGlyphAdvance(int offset,
									float& advanceX, float& advanceY,
									float escapementArray[]) const;
			
			void				_FinalizeLine(int lineStart, int lineEnd,
									int lineIndex, float y, float& lineHeight);
private:
			BString				fText;

			BFont				fDefaultFont;
			float				fAscent;
			float				fDescent;
			float				fWidth;
			float				fGlyphSpacing;
			float				fFirstLineInset;
			float				fLineInset;
			float				fLineSpacing;
			
			bool				fLayoutValid;

			GlyphInfo*			fGlyphInfoBuffer;
			int					fGlyphInfoCount;
			LineInfoList		fLineInfos;

};

#endif // FILTER_VIEW_H
