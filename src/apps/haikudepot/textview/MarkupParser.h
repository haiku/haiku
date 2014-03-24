/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MARKUP_PARSER_H
#define MARKUP_PARSER_H

#include "TextDocument.h"


class MarkupParser {
public:
								MarkupParser();
								MarkupParser(
									const CharacterStyle& characterStyle,
									const ParagraphStyle& paragraphStyle);

			void				SetStyles(
									const CharacterStyle& characterStyle,
									const ParagraphStyle& paragraphStyle);

			const CharacterStyle& HeadingCharacterStyle() const
									{ return fHeadingStyle; }
			const ParagraphStyle& HeadingParagraphStyle() const
									{ return fHeadingParagraphStyle; }

			const CharacterStyle& NormalCharacterStyle() const
									{ return fNormalStyle; }
			const ParagraphStyle& NormalParagraphStyle() const
									{ return fParagraphStyle; }

			TextDocumentRef		CreateDocumentFromMarkup(const BString& text);
			void				AppendMarkup(const TextDocumentRef& document,
									const BString& text);

private:
			void				_InitStyles();

			void				_ParseText(const BString& text);
			void				_CopySpan(const BString& text,
									int32& start, int32 end);
			void				_ToggleStyle(const CharacterStyle& style);
			void				_FinishParagraph(bool isLast);

private:
			CharacterStyle		fNormalStyle;
			CharacterStyle		fBoldStyle;
			CharacterStyle		fItalicStyle;
			CharacterStyle		fBoldItalicStyle;
			CharacterStyle		fHeadingStyle;

			ParagraphStyle		fParagraphStyle;
			ParagraphStyle		fHeadingParagraphStyle;
			ParagraphStyle		fBulletStyle;

			const CharacterStyle* fCurrentCharacterStyle;
			const ParagraphStyle* fCurrentParagraphStyle;

			// while parsing:
			TextDocumentRef		fTextDocument;
			Paragraph			fCurrentParagraph;
			int32				fSpanStartOffset;
};


#endif // MARKUP_PARSER_H
