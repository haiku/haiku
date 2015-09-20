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
#include "UndoableEditListener.h"


typedef List<Paragraph, false>					ParagraphList;
typedef List<TextListenerRef, false>			TextListenerList;
typedef List<UndoableEditListenerRef, false>	UndoListenerList;

class TextDocument;
typedef BReference<TextDocument> TextDocumentRef;


class TextDocument : public BReferenceable {
public:
								TextDocument();
								TextDocument(
									CharacterStyle characterStyle,
									ParagraphStyle paragraphStyle);
								TextDocument(const TextDocument& other);

			TextDocument&		operator=(const TextDocument& other);
			bool				operator==(const TextDocument& other) const;
			bool				operator!=(const TextDocument& other) const;

			// Text insertion and removing
			status_t			Insert(int32 textOffset, const BString& text);
			status_t			Insert(int32 textOffset, const BString& text,
									CharacterStyle style);
			status_t			Insert(int32 textOffset, const BString& text,
									CharacterStyle characterStyle,
									ParagraphStyle paragraphStyle);

			status_t			Remove(int32 textOffset, int32 length);

			status_t			Replace(int32 textOffset, int32 length,
									const BString& text);
			status_t			Replace(int32 textOffset, int32 length,
									const BString& text,
									CharacterStyle style);
			status_t			Replace(int32 textOffset, int32 length,
									const BString& text,
									CharacterStyle characterStyle,
									ParagraphStyle paragraphStyle);
			status_t			Replace(int32 textOffset, int32 length,
									TextDocumentRef document);

			// Style access
			const CharacterStyle& CharacterStyleAt(int32 textOffset) const;
			const ParagraphStyle& ParagraphStyleAt(int32 textOffset) const;

			// Paragraph access
			const ParagraphList& Paragraphs() const
									{ return fParagraphs; }

			int32				CountParagraphs() const;

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

			// Support
	static	TextDocumentRef		NormalizeText(const BString& text,
									CharacterStyle characterStyle,
									ParagraphStyle paragraphStyle);

			// Listener support
			bool				AddListener(TextListenerRef listener);
			bool				RemoveListener(TextListenerRef listener);
			bool				AddUndoListener(
									UndoableEditListenerRef listener);
			bool				RemoveUndoListener(
									UndoableEditListenerRef listener);

private:
			status_t			_Insert(int32 textOffset,
									TextDocumentRef document,
									int32& firstParagraph,
									int32& paragraphCount);
			status_t			_Remove(int32 textOffset, int32 length,
									int32& firstParagraph,
									int32& paragraphCount);

private:
			void				_NotifyTextChanging(
									TextChangingEvent& event) const;
			void				_NotifyTextChanged(
									const TextChangedEvent& event) const;
			void				_NotifyUndoableEditHappened(
									const UndoableEditRef& edit) const;

private:
			ParagraphList		fParagraphs;
			Paragraph			fEmptyLastParagraph;
			CharacterStyle		fDefaultCharacterStyle;

			TextListenerList	fTextListeners;
			UndoListenerList	fUndoListeners;
};


#endif // TEXT_DOCUMENT_H
