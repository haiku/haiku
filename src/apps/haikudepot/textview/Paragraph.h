/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2021, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PARAGRAPH_H
#define PARAGRAPH_H

#include <vector>

#include "ParagraphStyle.h"
#include "TextSpan.h"


class Paragraph {
public:
								Paragraph();
								Paragraph(const ParagraphStyle& style);
								Paragraph(const Paragraph& other);

			Paragraph&			operator=(const Paragraph& other);
			bool				operator==(const Paragraph& other) const;
			bool				operator!=(const Paragraph& other) const;

			void				SetStyle(const ParagraphStyle& style);
	inline	const ParagraphStyle& Style() const
									{ return fStyle; }

			int32				CountTextSpans() const;
			const TextSpan&		TextSpanAtIndex(int32 index) const;

			bool				Prepend(const TextSpan& span);
			bool				Append(const TextSpan& span);
			bool				Insert(int32 offset, const TextSpan& span);
			bool				Remove(int32 offset, int32 length);
			void				Clear();

			int32				Length() const;
			bool				IsEmpty() const;
			bool				EndsWith(BString string) const;

			BString				Text() const;
			BString				Text(int32 start, int32 length) const;
			Paragraph			SubParagraph(int32 start, int32 length) const;

			void				PrintToStream() const;

private:
			void				_InvalidateCachedLength();

private:
			ParagraphStyle		fStyle;
			std::vector<TextSpan>
								fTextSpans;
	mutable	int32				fCachedLength;
};


#endif // PARAGRAPH_H
