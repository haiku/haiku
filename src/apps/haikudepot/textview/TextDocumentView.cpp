/*
 * Copyright 2013-2015, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "TextDocumentView.h"

#include <algorithm>
#include <stdio.h>

#include <Clipboard.h>
#include <Cursor.h>
#include <MessageRunner.h>
#include <ScrollBar.h>
#include <Shape.h>
#include <Window.h>


enum {
	MSG_BLINK_CARET		= 'blnk',
};


TextDocumentView::TextDocumentView(const char* name)
	:
	BView(name, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE | B_FRAME_EVENTS),
	fInsetLeft(0.0f),
	fInsetTop(0.0f),
	fInsetRight(0.0f),
	fInsetBottom(0.0f),

	fCaretBounds(),
	fCaretBlinker(NULL),
	fCaretBlinkToken(0),
	fSelectionEnabled(true),
	fShowCaret(false),
	fMouseDown(false)
{
	fTextDocumentLayout.SetWidth(_TextLayoutWidth(Bounds().Width()));

	// Set default TextEditor
	SetTextEditor(TextEditorRef(new(std::nothrow) TextEditor(), true));

	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	SetLowUIColor(ViewUIColor());
}


TextDocumentView::~TextDocumentView()
{
	// Don't forget to remove listeners
	SetTextEditor(TextEditorRef());
	delete fCaretBlinker;
}


void
TextDocumentView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_COPY:
			Copy(be_clipboard);
			break;
		case B_SELECT_ALL:
			SelectAll();
			break;

		case MSG_BLINK_CARET:
		{
			int32 token;
			if (message->FindInt32("token", &token) == B_OK
				&& token == fCaretBlinkToken) {
				_BlinkCaret();
			}
			break;
		}

		default:
			BView::MessageReceived(message);
	}
}


void
TextDocumentView::Draw(BRect updateRect)
{
	FillRect(updateRect, B_SOLID_LOW);

	fTextDocumentLayout.SetWidth(_TextLayoutWidth(Bounds().Width()));
	fTextDocumentLayout.Draw(this, BPoint(fInsetLeft, fInsetTop), updateRect);

	if (!fSelectionEnabled || fTextEditor.Get() == NULL)
		return;

	bool isCaret = fTextEditor->SelectionLength() == 0;

	if (isCaret) {
		if (fShowCaret && fTextEditor->IsEditingEnabled())
			_DrawCaret(fTextEditor->CaretOffset());
	} else {
		_DrawSelection();
	}
}


void
TextDocumentView::AttachedToWindow()
{
	_UpdateScrollBars();
}


void
TextDocumentView::FrameResized(float width, float height)
{
	fTextDocumentLayout.SetWidth(width);
	_UpdateScrollBars();
}


void
TextDocumentView::WindowActivated(bool active)
{
	Invalidate();
}


void
TextDocumentView::MakeFocus(bool focus)
{
	if (focus != IsFocus())
		Invalidate();
	BView::MakeFocus(focus);
}


void
TextDocumentView::MouseDown(BPoint where)
{
	if (!fSelectionEnabled)
		return;

	MakeFocus();

	int32 modifiers = 0;
	if (Window() != NULL && Window()->CurrentMessage() != NULL)
		Window()->CurrentMessage()->FindInt32("modifiers", &modifiers);

	fMouseDown = true;
	SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);

	bool extendSelection = (modifiers & B_SHIFT_KEY) != 0;
	SetCaret(where, extendSelection);
}


void
TextDocumentView::MouseUp(BPoint where)
{
	fMouseDown = false;
}


void
TextDocumentView::MouseMoved(BPoint where, uint32 transit,
	const BMessage* dragMessage)
{
	if (!fSelectionEnabled)
		return;

	BCursor iBeamCursor(B_CURSOR_ID_I_BEAM);
	SetViewCursor(&iBeamCursor);

	if (fMouseDown)
		SetCaret(where, true);
}


void
TextDocumentView::KeyDown(const char* bytes, int32 numBytes)
{
	if (fTextEditor.Get() == NULL)
		return;

	KeyEvent event;
	event.bytes = bytes;
	event.length = numBytes;
	event.key = 0;
	event.modifiers = modifiers();

	if (Window() != NULL && Window()->CurrentMessage() != NULL) {
		BMessage* message = Window()->CurrentMessage();
		message->FindInt32("raw_char", &event.key);
		message->FindInt32("modifiers", &event.modifiers);
	}

	fTextEditor->KeyDown(event);
	_ShowCaret(true);
	// TODO: It is necessary to invalidate all, since neither the caret bounds
	// are updated in a way that would work here, nor is the text updated 
	// correcty which has been edited.
	Invalidate();
}


void
TextDocumentView::KeyUp(const char* bytes, int32 numBytes)
{
}


BSize
TextDocumentView::MinSize()
{
	return BSize(fInsetLeft + fInsetRight + 50.0f, fInsetTop + fInsetBottom);
}


BSize
TextDocumentView::MaxSize()
{
	return BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED);
}


BSize
TextDocumentView::PreferredSize()
{
	return BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED);
}


bool
TextDocumentView::HasHeightForWidth()
{
	return true;
}


void
TextDocumentView::GetHeightForWidth(float width, float* min, float* max,
	float* preferred)
{
	TextDocumentLayout layout(fTextDocumentLayout);
	layout.SetWidth(_TextLayoutWidth(width));

	float height = layout.Height() + 1 + fInsetTop + fInsetBottom;

	if (min != NULL)
		*min = height;
	if (max != NULL)
		*max = height;
	if (preferred != NULL)
		*preferred = height;
}


// #pragma mark -


void
TextDocumentView::SetTextDocument(const TextDocumentRef& document)
{
	fTextDocument = document;
	fTextDocumentLayout.SetTextDocument(fTextDocument);
	if (fTextEditor.Get() != NULL)
		fTextEditor->SetDocument(document);

	InvalidateLayout();
	Invalidate();
	_UpdateScrollBars();
}


void
TextDocumentView::SetEditingEnabled(bool enabled)
{
	if (fTextEditor.Get() != NULL)
		fTextEditor->SetEditingEnabled(enabled);
}


void
TextDocumentView::SetTextEditor(const TextEditorRef& editor)
{
	if (fTextEditor == editor)
		return;

	if (fTextEditor.Get() != NULL) {
		fTextEditor->SetDocument(TextDocumentRef());
		fTextEditor->SetLayout(TextDocumentLayoutRef());
		// TODO: Probably has to remove listeners
	}

	fTextEditor = editor;

	if (fTextEditor.Get() != NULL) {
		fTextEditor->SetDocument(fTextDocument);
		fTextEditor->SetLayout(TextDocumentLayoutRef(
			&fTextDocumentLayout));
		// TODO: Probably has to add listeners
	}
}


void
TextDocumentView::SetInsets(float inset)
{
	SetInsets(inset, inset, inset, inset);
}


void
TextDocumentView::SetInsets(float horizontal, float vertical)
{
	SetInsets(horizontal, vertical, horizontal, vertical);
}


void
TextDocumentView::SetInsets(float left, float top, float right, float bottom)
{
	if (fInsetLeft == left && fInsetTop == top
		&& fInsetRight == right && fInsetBottom == bottom) {
		return;
	}

	fInsetLeft = left;
	fInsetTop = top;
	fInsetRight = right;
	fInsetBottom = bottom;

	InvalidateLayout();
	Invalidate();
}


void
TextDocumentView::SetSelectionEnabled(bool enabled)
{
	if (fSelectionEnabled == enabled)
		return;
	fSelectionEnabled = enabled;
	Invalidate();
	// TODO: Deselect
}


void
TextDocumentView::SetCaret(BPoint location, bool extendSelection)
{
	if (!fSelectionEnabled || fTextEditor.Get() == NULL)
		return;

	location.x -= fInsetLeft;
	location.y -= fInsetTop;

	fTextEditor->SetCaret(location, extendSelection);
	_ShowCaret(!extendSelection);
	Invalidate();
}


void
TextDocumentView::SelectAll()
{
	if (!fSelectionEnabled || fTextEditor.Get() == NULL)
		return;

	fTextEditor->SelectAll();
	_ShowCaret(false);
	Invalidate();
}


bool
TextDocumentView::HasSelection() const
{
	return fTextEditor.Get() != NULL && fTextEditor->HasSelection();
}


void
TextDocumentView::GetSelection(int32& start, int32& end) const
{
	if (fTextEditor.Get() != NULL) {
		start = fTextEditor->SelectionStart();
		end = fTextEditor->SelectionEnd();
	}
}


void
TextDocumentView::Copy(BClipboard* clipboard)
{
	if (!HasSelection() || fTextDocument.Get() == NULL) {
		// Nothing to copy, don't clear clipboard contents for now reason.
		return;
	}

	if (clipboard == NULL || !clipboard->Lock())
		return;

	clipboard->Clear();

	BMessage* clip = clipboard->Data();
	if (clip != NULL) {
		int32 start;
		int32 end;
		GetSelection(start, end);

		BString text = fTextDocument->Text(start, end - start);
		clip->AddData("text/plain", B_MIME_TYPE, text.String(),
			text.Length());

		// TODO: Support for "application/x-vnd.Be-text_run_array"

		clipboard->Commit();
	}

	clipboard->Unlock();
}


// #pragma mark - private


float
TextDocumentView::_TextLayoutWidth(float viewWidth) const
{
	return viewWidth - (fInsetLeft + fInsetRight);
}


static const float kHorizontalScrollBarStep = 10.0f;
static const float kVerticalScrollBarStep = 12.0f;


void
TextDocumentView::_UpdateScrollBars()
{
	BRect bounds(Bounds());

	BScrollBar* horizontalScrollBar = ScrollBar(B_HORIZONTAL);
	if (horizontalScrollBar != NULL) {
		long viewWidth = bounds.IntegerWidth();
		long dataWidth = (long)ceilf(
			fTextDocumentLayout.Width() + fInsetLeft + fInsetRight);

		long maxRange = dataWidth - viewWidth;
		maxRange = std::max(maxRange, 0L);

		horizontalScrollBar->SetRange(0, (float)maxRange);
		horizontalScrollBar->SetProportion((float)viewWidth / dataWidth);
		horizontalScrollBar->SetSteps(kHorizontalScrollBarStep, dataWidth / 10);
	}

 	BScrollBar* verticalScrollBar = ScrollBar(B_VERTICAL);
	if (verticalScrollBar != NULL) {
		long viewHeight = bounds.IntegerHeight();
		long dataHeight = (long)ceilf(
			fTextDocumentLayout.Height() + fInsetTop + fInsetBottom);

		long maxRange = dataHeight - viewHeight;
		maxRange = std::max(maxRange, 0L);

		verticalScrollBar->SetRange(0, maxRange);
		verticalScrollBar->SetProportion((float)viewHeight / dataHeight);
		verticalScrollBar->SetSteps(kVerticalScrollBarStep, viewHeight);
	}
}


void
TextDocumentView::_ShowCaret(bool show)
{
	fShowCaret = show;
	if (fCaretBounds.IsValid())
		Invalidate(fCaretBounds);
	else
		Invalidate();
	// Cancel previous blinker, increment blink token so we only accept
	// the message from the blinker we just created
	fCaretBlinkToken++;
	BMessage message(MSG_BLINK_CARET);
	message.AddInt32("token", fCaretBlinkToken);
	delete fCaretBlinker;
	fCaretBlinker = new BMessageRunner(BMessenger(this), &message,
		500000, 1);
}	


void
TextDocumentView::_BlinkCaret()
{
	if (!fSelectionEnabled || fTextEditor.Get() == NULL)
		return;

	_ShowCaret(!fShowCaret);
}


void
TextDocumentView::_DrawCaret(int32 textOffset)
{
	if (!IsFocus() || Window() == NULL || !Window()->IsActive())
		return;

	float x1;
	float y1;
	float x2;
	float y2;

	fTextDocumentLayout.GetTextBounds(textOffset, x1, y1, x2, y2);
	x2 = x1 + 1;

	fCaretBounds = BRect(x1, y1, x2, y2);
	fCaretBounds.OffsetBy(fInsetLeft, fInsetTop);

	SetDrawingMode(B_OP_INVERT);
	FillRect(fCaretBounds);
}


void
TextDocumentView::_DrawSelection()
{
	int32 start;
	int32 end;
	GetSelection(start, end);

	BShape shape;
	_GetSelectionShape(shape, start, end);

	SetDrawingMode(B_OP_SUBTRACT);

	SetLineMode(B_ROUND_CAP, B_ROUND_JOIN);
	MovePenTo(fInsetLeft - 0.5f, fInsetTop - 0.5f);

	if (IsFocus() && Window() != NULL && Window()->IsActive()) {
		SetHighColor(30, 30, 30);
		FillShape(&shape);
	}

	SetHighColor(40, 40, 40);
	StrokeShape(&shape);
}


void
TextDocumentView::_GetSelectionShape(BShape& shape, int32 start, int32 end)
{
	float startX1;
	float startY1;
	float startX2;
	float startY2;
	fTextDocumentLayout.GetTextBounds(start, startX1, startY1, startX2,
		startY2);

	startX1 = floorf(startX1);
	startY1 = floorf(startY1);
	startX2 = ceilf(startX2);
	startY2 = ceilf(startY2);

	float endX1;
	float endY1;
	float endX2;
	float endY2;
	fTextDocumentLayout.GetTextBounds(end, endX1, endY1, endX2, endY2);

	endX1 = floorf(endX1);
	endY1 = floorf(endY1);
	endX2 = ceilf(endX2);
	endY2 = ceilf(endY2);

	int32 startLineIndex = fTextDocumentLayout.LineIndexForOffset(start);
	int32 endLineIndex = fTextDocumentLayout.LineIndexForOffset(end);

	if (startLineIndex == endLineIndex) {
		// Selection on one line
		BPoint lt(startX1, startY1);
		BPoint rt(endX1, endY1);
		BPoint rb(endX1, endY2);
		BPoint lb(startX1, startY2);

		shape.MoveTo(lt);
		shape.LineTo(rt);
		shape.LineTo(rb);
		shape.LineTo(lb);
		shape.Close();
	} else if (startLineIndex == endLineIndex - 1 && endX1 <= startX1) {
		// Selection on two lines, with gap:
		// ---------
		// ------###
		// ##-------
		// ---------
		float width = ceilf(fTextDocumentLayout.Width());

		BPoint lt(startX1, startY1);
		BPoint rt(width, startY1);
		BPoint rb(width, startY2);
		BPoint lb(startX1, startY2);

		shape.MoveTo(lt);
		shape.LineTo(rt);
		shape.LineTo(rb);
		shape.LineTo(lb);
		shape.Close();

		lt = BPoint(0, endY1);
		rt = BPoint(endX1, endY1);
		rb = BPoint(endX1, endY2);
		lb = BPoint(0, endY2);

		shape.MoveTo(lt);
		shape.LineTo(rt);
		shape.LineTo(rb);
		shape.LineTo(lb);
		shape.Close();
	} else {
		// Selection over multiple lines
		float width = ceilf(fTextDocumentLayout.Width());

		shape.MoveTo(BPoint(startX1, startY1));
		shape.LineTo(BPoint(width, startY1));
		shape.LineTo(BPoint(width, endY1));
		shape.LineTo(BPoint(endX1, endY1));
		shape.LineTo(BPoint(endX1, endY2));
		shape.LineTo(BPoint(0, endY2));
		shape.LineTo(BPoint(0, startY2));
		shape.LineTo(BPoint(startX1, startY2));
		shape.Close();
	}
}


