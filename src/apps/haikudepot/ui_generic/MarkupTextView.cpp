/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "MarkupTextView.h"


static const rgb_color kLightBlack = (rgb_color){ 60, 60, 60, 255 };


MarkupTextView::MarkupTextView(const char* name)
	:
	TextDocumentView(name)
{
	SetEditingEnabled(false);
	CharacterStyle regularStyle;

	float fontSize = regularStyle.Font().Size();

	ParagraphStyle paragraphStyle;
	paragraphStyle.SetJustify(true);
	paragraphStyle.SetSpacingTop(ceilf(fontSize * 0.3f));
	paragraphStyle.SetLineSpacing(ceilf(fontSize * 0.2f));

	fMarkupParser.SetStyles(regularStyle, paragraphStyle);
}


void
MarkupTextView::SetText(const BString& markupText)
{
	SetTextDocument(fMarkupParser.CreateDocumentFromMarkup(markupText));
}


void
MarkupTextView::SetText(const BString heading, const BString& markupText)
{
	TextDocumentRef document(new(std::nothrow) TextDocument(), true);

	Paragraph paragraph(fMarkupParser.HeadingParagraphStyle());
	paragraph.Append(TextSpan(heading,
		fMarkupParser.HeadingCharacterStyle()));
	document->Append(paragraph);

	fMarkupParser.AppendMarkup(document, markupText);

	SetTextDocument(document);
}


void
MarkupTextView::SetDisabledText(const BString& text)
{
	TextDocumentRef document(new(std::nothrow) TextDocument(), true);

	ParagraphStyle paragraphStyle;
	paragraphStyle.SetAlignment(ALIGN_CENTER);

	CharacterStyle disabledStyle(fMarkupParser.NormalCharacterStyle());
	disabledStyle.SetForegroundColor(kLightBlack);

	Paragraph paragraph(paragraphStyle);
	paragraph.Append(TextSpan(text, disabledStyle));
	document->Append(paragraph);

	SetTextDocument(document);
}
