/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PARAGRAPH_H
#define PARAGRAPH_H

#include "List.h"
#include "ParagraphStyle.h"
#include "TextSpan.h"


typedef List<TextSpan, false>	TextSpanList;


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

	inline	const TextSpanList&	TextSpans() const
									{ return fTextSpans; }

			bool				Prepend(const TextSpan& span);
			bool				Append(const TextSpan& span);
			bool				Insert(int32 offset, const TextSpan& span);
			bool				Remove(int32 offset, int32 length);
			void				Clear();

			int32				Length() const;
			bool				IsEmpty() const;

			BString				GetText(int32 start, int32 length) const;

private:
			ParagraphStyle		fStyle;
			TextSpanList		fTextSpans;
};


#endif // PARAGRAPH_H
