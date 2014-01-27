/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef TEXT_DOCUMENT_H
#define TEXT_DOCUMENT_H

#include <Referenceable.h>

#include "CharacterStyle.h"
#include "List.h"
#include "Paragraph.h"
#include "TextListener.h"


typedef List<Paragraph, false>			ParagraphList;
typedef List<TextListenerRef, false>	TextListenerList;

class TextDocument;
typedef BReference<TextDocument> TextDocumentRef;


class TextDocument : public BReferenceable {
public:
								TextDocument();
								TextDocument(
									const CharacterStyle& characterStyle,
									const ParagraphStyle& paragraphStyle);
								TextDocument(const TextDocument& other);

			TextDocument&		operator=(const TextDocument& other);
			bool				operator==(const TextDocument& other) const;
			bool				operator!=(const TextDocument& other) const;

			// Text insertion and removing
			status_t			Insert(int32 textOffset, const BString& text);
			status_t			Insert(int32 textOffset, const BString& text,
									const CharacterStyle& style);
			status_t			Insert(int32 textOffset, const BString& text,
									const CharacterStyle& characterStyle,
									const ParagraphStyle& paragraphStyle);

			status_t			Remove(int32 textOffset, int32 length);

			status_t			Replace(int32 textOffset, int32 length,
									const BString& text);
			status_t			Replace(int32 textOffset, int32 length,
									const BString& text,
									const CharacterStyle& style);
			status_t			Replace(int32 textOffset, int32 length,
									const BString& text,
									const CharacterStyle& characterStyle,
									const ParagraphStyle& paragraphStyle);

			// Style access
			const CharacterStyle& CharacterStyleAt(int32 textOffset) const;
			const ParagraphStyle& ParagraphStyleAt(int32 textOffset) const;

			// Paragraph access
			const ParagraphList& Paragraphs() const
									{ return fParagraphs; }

			int32				ParagraphIndexFor(int32 textOffset,
									int32& paragraphOffset) const;

			const Paragraph&	ParagraphAt(int32 textOffset,
									int32& paragraphOffset) const;

			const Paragraph&	ParagraphAt(int32 index) const;

			bool				Append(const Paragraph& paragraph);

			// Query information
			int32				Length() const;

			BString				Text() const;
			BString				Text(int32 textOffset, int32 length) const;
			TextDocumentRef		SubDocument(int32 textOffset,
									int32 length) const;

			void				PrintToStream() const;

			// Listener support
			bool				AddListener(const TextListenerRef& listener);
			bool				RemoveListener(const TextListenerRef& listener);

private:
			void				_NotifyTextChanging(
									TextChangingEvent& event) const;
			void				_NotifyTextChanged(
									const TextChangedEvent& event) const;

private:
			ParagraphList		fParagraphs;
			Paragraph			fEmptyLastParagraph;
			CharacterStyle		fDefaultCharacterStyle;

			TextListenerList	fTextListeners;
};


#endif // TEXT_DOCUMENT_H
