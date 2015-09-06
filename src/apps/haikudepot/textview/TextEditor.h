/*
 * Copyright 2014, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef TEXT_EDITOR_H
#define TEXT_EDITOR_H


#include <Point.h>
#include <Referenceable.h>

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


class TextEditor : public BReferenceable {
public:
								TextEditor();
								TextEditor(const TextEditor& other);
	virtual						~TextEditor();

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

			void				SetEditingEnabled(bool enabled);
	inline	bool				IsEditingEnabled() const
									{ return fEditingEnabled; }

			void				SetCaret(BPoint location, bool extendSelection);
			void				SelectAll();
			void				SetSelection(TextSelection selection);
	inline	TextSelection		Selection() const
									{ return fSelection; }

			void				SetCharacterStyle(::CharacterStyle style);
			::CharacterStyle	CharacterStyle() const
									{ return fStyleAtCaret; }

	virtual	void				KeyDown(KeyEvent event);

	virtual	status_t			Insert(int32 offset, const BString& string);
	virtual	status_t			Remove(int32 offset, int32 length);
	virtual	status_t			Replace(int32 offset, int32 length,
									const BString& string);

			void				LineUp(bool select);
			void				LineDown(bool select);
			void				LineStart(bool select);
			void				LineEnd(bool select);

			bool				HasSelection() const;
			int32				SelectionStart() const;
			int32				SelectionEnd() const;
			int32				SelectionLength() const;
	inline	int32				CaretOffset() const
									{ return fSelection.Caret(); }

private:
			void				_MoveToLine(int32 lineIndex, bool select);
			void				_SetCaretOffset(int32 offset,
									bool updateAnchor,
									bool lockSelectionAnchor,
									bool updateSelectionStyle);
			void				_SetSelection(int32 caret, int32 anchor,
									bool updateAnchor,
									bool updateSelectionStyle);

			void				_UpdateStyleAtCaret();

private:
			TextDocumentRef		fDocument;
			TextDocumentLayoutRef fLayout;
			TextSelection		fSelection;
			float				fCaretAnchorX;
			::CharacterStyle	fStyleAtCaret;
			bool				fEditingEnabled;
};


typedef BReference<TextEditor> TextEditorRef;


#endif // TEXT_EDITOR_H
