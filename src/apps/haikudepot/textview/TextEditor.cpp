/*
 * Copyright 2014, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "TextEditor.h"

#include <algorithm>
#include <stdio.h>


TextEditor::TextEditor()
	:
	fDocument(),
	fLayout(),
	fSelection(),
	fCaretAnchorX(0.0f),
	fStyleAtCaret(),
	fEditingEnabled(true)
{
}


TextEditor::TextEditor(const TextEditor& other)
	:
	fDocument(other.fDocument),
	fLayout(other.fLayout),
	fSelection(other.fSelection),
	fCaretAnchorX(other.fCaretAnchorX),
	fStyleAtCaret(other.fStyleAtCaret),
	fEditingEnabled(other.fEditingEnabled)
{
}


TextEditor::~TextEditor()
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
	fEditingEnabled = other.fEditingEnabled;
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
		&& fStyleAtCaret == other.fStyleAtCaret
		&& fEditingEnabled == other.fEditingEnabled;
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
TextEditor::SetEditingEnabled(bool enabled)
{
	fEditingEnabled = enabled;
}


void
TextEditor::SetCaret(BPoint location, bool extendSelection)
{
	if (fDocument.Get() == NULL || fLayout.Get() == NULL)
		return;

	bool rightOfChar = false;
	int32 caretOffset = fLayout->TextOffsetAt(location.x, location.y,
		rightOfChar);

	if (rightOfChar)
		caretOffset++;

	_SetCaretOffset(caretOffset, true, extendSelection, true);
}


void
TextEditor::SelectAll()
{
	if (fDocument.Get() == NULL)
		return;

	SetSelection(TextSelection(0, fDocument->Length()));
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

	if (HasSelection()) {
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
			LineUp(select);
			break;

		case B_DOWN_ARROW:
			LineDown(select);
			break;

		case B_LEFT_ARROW:
			if (HasSelection() && !select) {
				_SetCaretOffset(
					std::min(fSelection.Caret(), fSelection.Anchor()),
					true, false, true);
			} else
				_SetCaretOffset(fSelection.Caret() - 1, true, select, true);
			break;

		case B_RIGHT_ARROW:
			if (HasSelection() && !select) {
				_SetCaretOffset(
					std::max(fSelection.Caret(), fSelection.Anchor()),
					true, false, true);
			} else
				_SetCaretOffset(fSelection.Caret() + 1, true, select, true);
			break;

		case B_HOME:
			LineStart(select);
			break;

		case B_END:
			LineEnd(select);
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
			if (HasSelection()) {
				Remove(SelectionStart(), SelectionLength());
			} else {
				if (fSelection.Caret() > 0)
					Remove(fSelection.Caret() - 1, 1);
			}
			break;

		case B_DELETE:
			if (HasSelection()) {
				Remove(SelectionStart(), SelectionLength());
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

				Replace(SelectionStart(), SelectionLength(), text);
			}
			break;
	}
}


status_t
TextEditor::Insert(int32 offset, const BString& string)
{
	if (!fEditingEnabled || fDocument.Get() == NULL)
		return B_ERROR;

	status_t ret = fDocument->Insert(offset, string, fStyleAtCaret);

	if (ret == B_OK) {
		_SetCaretOffset(offset + string.CountChars(), true, false, true);

		fDocument->PrintToStream();
	}

	return ret;
}


status_t
TextEditor::Remove(int32 offset, int32 length)
{
	if (!fEditingEnabled || fDocument.Get() == NULL)
		return B_ERROR;

	status_t ret = fDocument->Remove(offset, length);

	if (ret == B_OK) {
		_SetCaretOffset(offset, true, false, true);

		fDocument->PrintToStream();
	}

	return ret;
}


status_t
TextEditor::Replace(int32 offset, int32 length, const BString& string)
{
	if (!fEditingEnabled || fDocument.Get() == NULL)
		return B_ERROR;

	status_t ret = fDocument->Replace(offset, length, string);

	if (ret == B_OK) {
		_SetCaretOffset(offset + string.CountChars(), true, false, true);

		fDocument->PrintToStream();
	}

	return ret;
}


// #pragma mark -


void
TextEditor::LineUp(bool select)
{
	if (fLayout.Get() == NULL)
		return;

	int32 lineIndex = fLayout->LineIndexForOffset(fSelection.Caret());
	_MoveToLine(lineIndex - 1, select);
}


void
TextEditor::LineDown(bool select)
{
	if (fLayout.Get() == NULL)
		return;

	int32 lineIndex = fLayout->LineIndexForOffset(fSelection.Caret());
	_MoveToLine(lineIndex + 1, select);
}


void
TextEditor::LineStart(bool select)
{
	if (fLayout.Get() == NULL)
		return;

	int32 lineIndex = fLayout->LineIndexForOffset(fSelection.Caret());
	_SetCaretOffset(fLayout->FirstOffsetOnLine(lineIndex), true, select,
		true);
}


void
TextEditor::LineEnd(bool select)
{
	if (fLayout.Get() == NULL)
		return;

	int32 lineIndex = fLayout->LineIndexForOffset(fSelection.Caret());
	_SetCaretOffset(fLayout->LastOffsetOnLine(lineIndex), true, select,
		true);
}


// #pragma mark -


bool
TextEditor::HasSelection() const
{
	return SelectionLength() > 0;
}


int32
TextEditor::SelectionStart() const
{
	return std::min(fSelection.Caret(), fSelection.Anchor());
}


int32
TextEditor::SelectionEnd() const
{
	return std::max(fSelection.Caret(), fSelection.Anchor());
}


int32
TextEditor::SelectionLength() const
{
	return SelectionEnd() - SelectionStart();
}


// #pragma mark - private


// _MoveToLine
void
TextEditor::_MoveToLine(int32 lineIndex, bool select)
{
	if (lineIndex < 0) {
		// Move to beginning of line instead. Most editors do. Some only when
		// selecting. Note that we are not updating the horizontal anchor here,
		// even though the horizontal caret position changes. Most editors
		// return to the previous horizonal offset when moving back down from
		// the beginning of the line.
		_SetCaretOffset(0, false, select, true);
		return;
	}
	if (lineIndex >= fLayout->CountLines()) {
		// Move to end of line instead, see above for why we do not update the
		// horizontal anchor.
		_SetCaretOffset(fDocument->Length(), false, select, true);
		return;
	}

	float x1;
	float y1;
	float x2;
	float y2;
	fLayout->GetLineBounds(lineIndex , x1, y1, x2, y2);

	bool rightOfCenter;
	int32 textOffset = fLayout->TextOffsetAt(fCaretAnchorX, (y1 + y2) / 2,
		rightOfCenter);

	if (rightOfCenter)
		textOffset++;

	_SetCaretOffset(textOffset, false, select, true);
}

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


void
TextEditor::_SetSelection(int32 caret, int32 anchor, bool updateAnchor,
	bool updateSelectionStyle)
{
	if (fLayout.Get() == NULL)
		return;

	if (caret == fSelection.Caret() && anchor == fSelection.Anchor())
		return;

	fSelection.SetCaret(caret);
	fSelection.SetAnchor(anchor);

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


