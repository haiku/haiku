/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef TEXT_DOCUMENT_LAYOUT_H
#define TEXT_DOCUMENT_LAYOUT_H

#include <Referenceable.h>

#include "List.h"
#include "TextDocument.h"
#include "ParagraphLayout.h"


class BView;


class ParagraphLayoutInfo {
public:
	ParagraphLayoutInfo()
		:
		y(0.0f),
		layout()
	{
	}

	ParagraphLayoutInfo(float y, const ParagraphLayoutRef& layout)
		:
		y(y),
		layout(layout)
	{
	}

	ParagraphLayoutInfo(const ParagraphLayoutInfo& other)
		:
		y(other.y),
		layout(other.layout)
	{
	}


	ParagraphLayoutInfo& operator=(const ParagraphLayoutInfo& other)
	{
		y = other.y;
		layout = other.layout;
		return *this;
	}

	bool operator==(const ParagraphLayoutInfo& other) const
	{
		return y == other.y
			&& layout == other.layout;
	}

	bool operator!=(const ParagraphLayoutInfo& other) const
	{
		return !(*this == other);
	}

public:
	float				y;
	ParagraphLayoutRef	layout;
};


typedef List<ParagraphLayoutInfo, false> ParagraphLayoutList;


class TextDocumentLayout : public BReferenceable {
public:
								TextDocumentLayout();
								TextDocumentLayout(
									const TextDocumentRef& document);
								TextDocumentLayout(
									const TextDocumentLayout& other);
	virtual						~TextDocumentLayout();

			void				SetTextDocument(
									const TextDocumentRef& document);

			void				SetWidth(float width);
			float				Width() const
									{ return fWidth; }

			float				Height();
			void				Draw(BView* view, const BPoint& offset,
									const BRect& updateRect);

private:
			void				_Init();
			void				_ValidateLayout();
			void				_Layout();

			void				_DrawLayout(BView* view,
									const ParagraphLayoutInfo& layout) const;

private:
			float				fWidth;
			bool				fLayoutValid;

			TextDocumentRef		fDocument;
			ParagraphLayoutList	fParagraphLayouts;
};


typedef BReference<TextDocumentLayout> TextDocumentLayoutRef;

#endif // TEXT_DOCUMENT_LAYOUT_H
