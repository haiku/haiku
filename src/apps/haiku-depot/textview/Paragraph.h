/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PARAGRAPH_H
#define PARAGRAPH_H

#include "List.h"
#include "ParagraphStyle.h"


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

private:
			ParagraphStyle		fStyle;
};


#endif // PARAGRAPH_H
