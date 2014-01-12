/*
 * Copyright 2014, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "TextEditor.h"

#include <algorithm>


TextEditor::TextEditor()
	:
	fDocument(),
	fLayout(),
	fSelection(),
	fCaretAnchorX(0.0f),
	fStyleAtCaret()
{
}


TextEditor::TextEditor(const TextEditor& other)
	:
	fDocument(other.fDocument),
	fLayout(other.fLayout),
	fSelection(other.fSelection),
	fCaretAnchorX(other.fCaretAnchorX),
	fStyleAtCaret(other.fStyleAtCaret)
{
}


TextEditor&
TextEditor::operator=(const TextEditor& other)
{
	if (this == &other)
		return *this;

	fDocument = other.fDocument;
	fLayout = other.fLayout;
	fSelection = other.fSelection;
	fCaretAnchorX = other.fCaretAnchorX;
	fStyleAtCaret = other.fStyleAtCaret;
	return *this;
}


bool
TextEditor::operator==(const TextEditor& other) const
{
	if (this == &other)
		return true;

	return fDocument == other.fDocument
		&& fLayout == other.fLayout
		&& fSelection == other.fSelection
		&& fCaretAnchorX == other.fCaretAnchorX
		&& fStyleAtCaret == other.fStyleAtCaret;
}


bool
TextEditor::operator!=(const TextEditor& other) const
{
	return !(*this == other);
}


// #pragma mark -


void
TextEditor::SetDocument(const TextDocumentRef& ref)
{
	fDocument = ref;
	SetSelection(TextSelection());
}


void
TextEditor::SetLayout(const TextDocumentLayoutRef& ref)
{
	fLayout = ref;
	SetSelection(TextSelection());
}


void
TextEditor::SetSelection(TextSelection selection)
{
	_SetSelection(selection.Caret(), selection.Anchor(), true, true);
}


void
TextEditor::SetCharacterStyle(::CharacterStyle style)
{
	if (fStyleAtCaret == style)
		return;

	fStyleAtCaret = style;

	if (fSelection.Caret() != fSelection.Anchor()) {
		// TODO: Apply style to selection range
	}
}


void
TextEditor::KeyDown(KeyEvent event)
{
	if (fDocument.Get() == NULL)
		return;

	bool select = (event.modifiers & B_SHIFT_KEY) != 0;

	switch (event.key) {
		case B_UP_ARROW:
			_LineUp(select);
			break;

		case B_DOWN_ARROW:
			_LineDown(select);
			break;

		case B_LEFT_ARROW:
			if (_HasSelection() && !select) {
				_SetCaretOffset(
					std::min(fSelection.Caret(), fSelection.Anchor()),
					true, false, true
				);
			} else
				_SetCaretOffset(fSelection.Caret() - 1, true, select, true);
			break;

		case B_RIGHT_ARROW:
			if (_HasSelection() && !select) {
				_SetCaretOffset(
					std::max(fSelection.Caret(), fSelection.Anchor()),
					true, false, true
				);
			} else
				_SetCaretOffset(fSelection.Caret() + 1, true, select, true);
			break;

		case B_HOME:
			_LineStart(select);
			break;

		case B_END:
			_LineEnd(select);
			break;

		case B_ENTER:
			Insert(fSelection.Caret(), "\n");
			break;

		case B_TAB:
			// TODO: Tab support in TextLayout
			Insert(fSelection.Caret(), " ");
			break;

		case B_ESCAPE:
			break;

		case B_BACKSPACE:
			if (_HasSelection()) {
				Remove(_SelectionStart(), _SelectionLength());
			} else {
				if (fSelection.Caret() > 0)
					Remove(fSelection.Caret() - 1, 1);
			}
			break;

		case B_DELETE:
			if (_HasSelection()) {
				Remove(_SelectionStart(), _SelectionLength());
			} else {
				if (fSelection.Caret() < fDocument->Length())
					Remove(fSelection.Caret(), 1);
			}
			break;

		case B_INSERT:
			// TODO: Toggle insert mode (or maybe just don't support it)
			break;

		case B_PAGE_UP:
		case B_PAGE_DOWN:
		case B_SUBSTITUTE:
		case B_FUNCTION_KEY:
		case B_KATAKANA_HIRAGANA:
		case B_HANKAKU_ZENKAKU:
			break;

		default:
			if (event.bytes != NULL && event.length > 0) {
				// Handle null-termintating the string
				BString text(event.bytes, event.length);
				Insert(fSelection.Caret(), text);
			}
			break;
	}
}


void
TextEditor::Insert(int32 offset, const BString& string)
{
	// TODO: ...
}


void
TextEditor::Remove(int32 offset, int32 length)
{
	// TODO: ...
}


// #pragma mark - private


// _SetCaretOffset
void
TextEditor::_SetCaretOffset(int32 offset, bool updateAnchor,
	bool lockSelectionAnchor, bool updateSelectionStyle)
{
	if (fDocument.Get() == NULL)
		return;

	if (offset < 0)
		offset = 0;
	int32 textLength = fDocument->Length();
	if (offset > textLength)
		offset = textLength;

	int32 caret = offset;
	int32 anchor = lockSelectionAnchor ? fSelection.Anchor() : offset;
	_SetSelection(caret, anchor, updateAnchor, updateSelectionStyle);
}


// _SetSelection
void
TextEditor::_SetSelection(int32 caret, int32 anchor, bool updateAnchor,
	bool updateSelectionStyle)
{
	if (fLayout.Get() == NULL)
		return;
	
	if (caret == fSelection.Caret() && caret == fSelection.Anchor())
		return;

	fSelection.SetAnchor(anchor);
	fSelection.SetCaret(caret);

	if (updateAnchor) {
		float x1;
		float y1;
		float x2;
		float y2;

		fLayout->GetTextBounds(caret, x1, y1, x2, y2);
		fCaretAnchorX = x1;
	}

	if (updateSelectionStyle)
		_UpdateStyleAtCaret();
}


void
TextEditor::_UpdateStyleAtCaret()
{
	if (fDocument.Get() == NULL)
		return;

	int32 offset = fSelection.Caret() - 1;
	if (offset < 0)
		offset = 0;
	SetCharacterStyle(fDocument->CharacterStyleAt(offset));
}


// #pragma mark -


void
TextEditor::_LineUp(bool select)
{
	// TODO
}


void
TextEditor::_LineDown(bool select)
{
	// TODO
}


void
TextEditor::_LineStart(bool select)
{
	// TODO
}


void
TextEditor::_LineEnd(bool select)
{
	// TODO
}


// #pragma mark -


bool
TextEditor::_HasSelection() const
{
	return _SelectionLength() > 0;
}


int32
TextEditor::_SelectionStart() const
{
	return std::min(fSelection.Caret(), fSelection.Anchor());
}


int32
TextEditor::_SelectionEnd() const
{
	return std::max(fSelection.Caret(), fSelection.Anchor());
}


int32
TextEditor::_SelectionLength() const
{
	return _SelectionEnd() - _SelectionStart();
}
