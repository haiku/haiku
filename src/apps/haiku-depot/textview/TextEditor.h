/*
 * Copyright 2014, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef TEXT_EDITOR_H
#define TEXT_EDITOR_H


#include "CharacterStyle.h"
#include "TextDocument.h"
#include "TextDocumentLayout.h"
#include "TextSelection.h"


class KeyEvent {
public:
			const char*			bytes;
			int32				length;
			int32				key;
			int32				modifiers;
};


class TextEditor {
public:
								TextEditor();
								TextEditor(const TextEditor& other);

			TextEditor&			operator=(const TextEditor& other);
			bool				operator==(const TextEditor& other) const;
			bool				operator!=(const TextEditor& other) const;

			void				SetDocument(const TextDocumentRef& ref);
			TextDocumentRef		Document() const
									{ return fDocument; }

			void				SetLayout(
									const TextDocumentLayoutRef& ref);
			TextDocumentLayoutRef Layout() const
									{ return fLayout; }

			void				SetSelection(TextSelection selection);
	inline	TextSelection		Selection() const
									{ return fSelection; }

			void				SetCharacterStyle(::CharacterStyle style);
			::CharacterStyle	CharacterStyle() const
									{ return fStyleAtCaret; }

			void				KeyDown(KeyEvent event);

			void				Insert(int32 offset, const BString& string);
			void				Remove(int32 offset, int32 length);

private:
			void				_SetCaretOffset(int32 offset,
									bool updateAnchor,
									bool lockSelectionAnchor,
									bool updateSelectionStyle);
			void				_SetSelection(int32 caret, int32 anchor,
									bool updateAnchor,
									bool updateSelectionStyle);

			void				_UpdateStyleAtCaret();

			void				_LineUp(bool select);
			void				_LineDown(bool select);
			void				_LineStart(bool select);
			void				_LineEnd(bool select);

			bool				_HasSelection() const;
			int32				_SelectionStart() const;
			int32				_SelectionEnd() const;
			int32				_SelectionLength() const;

private:
			TextDocumentRef		fDocument;
			TextDocumentLayoutRef fLayout;
			TextSelection		fSelection;
			float				fCaretAnchorX;
			::CharacterStyle	fStyleAtCaret;
};


#endif // TEXT_EDITOR_H
