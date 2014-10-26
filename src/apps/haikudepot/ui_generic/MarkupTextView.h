/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MARKUP_TEXT_VIEW_H
#define MARKUP_TEXT_VIEW_H


#include "MarkupParser.h"
#include "TextDocumentView.h"


class MarkupTextView : public TextDocumentView {
public:
								MarkupTextView(const char* name);

			void				SetText(const BString& markupText);
			void				SetText(const BString heading,
									const BString& markupText);

			void				SetDisabledText(const BString& text);

private:
			MarkupParser		fMarkupParser;
};


#endif // MARKUP_TEXT_VIEW_H
