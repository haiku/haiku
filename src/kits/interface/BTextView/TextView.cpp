//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		TextView.cpp
//	Author:			Marc Flerackers (mflerackers@androme.be)
//	Description:	BTextView displays and manages styled text.
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <algorithm>

// System Includes -------------------------------------------------------------
#include <Application.h>
#include <TextView.h>
#include <ScrollBar.h>
#include <Clipboard.h>
#include <Window.h>
#include <Message.h>
#include <PropertyInfo.h>
#include <Errors.h>
#include <SupportDefs.h>
#include <UTF8.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "TextGapBuffer.h"
#include "LineBuffer.h"
#include "StyleBuffer.h"
#include "moreUTF8.h"
// Local Defines ---------------------------------------------------------------

using std::min;
using std::max;
// _BTextTrackState_ class -----------------------------------------------------
class _BTextTrackState_ {

public:
	_BTextTrackState_(bool inSelection)
		:	fMoved(false),
			fInSelection(inSelection)
	{}

	bool	fMoved;
	bool	fInSelection;
};
//------------------------------------------------------------------------------

// Globals ---------------------------------------------------------------------
static property_info prop_list[] =
{
	{
		"Selection",
		{ B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 },
		"Returns the current selection.", 0,
		{ B_INT32_TYPE, 0 }
	},
	{
		"Selection",
		{ B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 },
		"Sets the current selection.", 0,
		{ B_INT32_TYPE, 0 }
	},
	{
		"Text",
		{ B_COUNT_PROPERTIES, 0 },
		{ B_DIRECT_SPECIFIER, 0 },
		"Returns the length of the text in bytes.", 0,
		{ B_INT32_TYPE, 0 }
	},
	{
		"Text",
		{ B_GET_PROPERTY, 0 },
		{ B_RANGE_SPECIFIER, B_REVERSE_RANGE_SPECIFIER, 0 },
		"Returns the text in the specified range in the BTextView.", 0,
		{ B_STRING_TYPE, 0 }
	},
	{
		"Text",
		{ B_SET_PROPERTY, 0 },
		{ B_RANGE_SPECIFIER, B_REVERSE_RANGE_SPECIFIER, 0 },
		"Removes or inserts text into the specified range in the BTextView.", 0,
		{ B_STRING_TYPE, 0 }
	},
	{
		"text_run_array",
		{ B_GET_PROPERTY, 0 },
		{ B_RANGE_SPECIFIER, B_REVERSE_RANGE_SPECIFIER, 0 },
		"Returns the style information for the text in the specified range in the BTextView.", 0,
		{ B_RAW_TYPE, 0 },
	},
	{
		"text_run_array",
		{ B_SET_PROPERTY, 0 },
		{ B_RANGE_SPECIFIER, B_REVERSE_RANGE_SPECIFIER, 0 },
		"Sets the style information for the text in the specified range in the BTextView.", 0,
		{ B_RAW_TYPE, 0 },
	},
	{ 0 }
};

//------------------------------------------------------------------------------
BTextView::BTextView(BRect frame, const char *name, BRect textRect,
					 uint32 resizeMask, uint32 flags)
	:	BView(frame, name, resizeMask, flags | B_FRAME_EVENTS | B_PULSE_NEEDED),
		fText(NULL),
		fLines(NULL),
		fStyles(NULL),
		//fTextRect
		fSelStart(0),
		fSelEnd(0),
		fCaretVisible(true),
		//fCaretTime
		//fClickOffset
		//fClickCount
		//fClickTime
		//fDragOffset
		//fCursor
		//fActive
		fStylable(false),
		fTabWidth(28.0f),
		fSelectable(true),
		fEditable(true),
		fWrap(true),
		fMaxBytes(0),
		fDisallowedChars(NULL),
		fAlignment(B_ALIGN_LEFT),
		fAutoindent(false),
		fOffscreen(NULL),
		fColorSpace(B_CMAP8),
		fResizable(false),
		fContainerView(NULL),
		fUndo(NULL),
		fInline(NULL),
		fDragRunner(NULL),
		fClickRunner(NULL),
		//fWhere
		fTrackingMouse(NULL),
		fTextChange(NULL)
{
	InitObject(textRect, NULL, NULL);
}
//------------------------------------------------------------------------------
BTextView::BTextView(BRect frame, const char *name, BRect textRect,
					 const BFont *initialFont, const rgb_color *initialColor,
					 uint32 resizeMask, uint32 flags)
	:	BView(frame, name, resizeMask, flags | B_FRAME_EVENTS),
		fText(NULL),
		fLines(NULL),
		fStyles(NULL),
		//fTextRect
		fSelStart(0),
		fSelEnd(0),
		fCaretVisible(true),
		//fCaretTime
		//fClickOffset
		//fClickCount
		//fClickTime
		//fDragOffset
		//fCursor
		//fActive
		fStylable(false),
		fTabWidth(28.0f),
		fSelectable(true),
		fEditable(true),
		fWrap(true),
		fMaxBytes(0),
		fDisallowedChars(NULL),
		fAlignment(B_ALIGN_LEFT),
		fAutoindent(false),
		fOffscreen(NULL),
		fColorSpace(B_CMAP8),
		fResizable(false),
		fContainerView(NULL),
		fUndo(NULL),
		fInline(NULL),
		fDragRunner(NULL),
		fClickRunner(NULL),
		//fWhere
		fTrackingMouse(NULL),
		fTextChange(NULL)
{
	InitObject(textRect, initialFont, initialColor);
}
//------------------------------------------------------------------------------
BTextView::BTextView(BMessage *archive)
	:	BView(archive)
{
	archive->AddString("_text", Text()); 
	archive->AddInt32("_align", fAlignment); 
	archive->AddFloat("_tab", fTabWidth);
	archive->AddInt32("_col_sp", fColorSpace);
	archive->AddRect("_trect", fTextRect);
	archive->AddInt32("_max", fMaxBytes);
	archive->AddInt32("_sel", fSelStart);
	archive->AddInt32("_sel", fSelEnd);
	//"_dis_ch" (array) B_RAW_TYPE Disallowed characters. 
	//"_runs" B_RAW_TYPE Flattened run array. 
	archive->AddBool("_stylable", fStylable);
	archive->AddBool("_auto_in", fAutoindent);
	archive->AddBool("_wrap", fWrap);
	archive->AddBool("_nsel", !fSelectable);
	archive->AddBool("_nedit", !fEditable);
}
//------------------------------------------------------------------------------
BTextView::~BTextView()
{
	delete fText;
	delete fLines;
}
//------------------------------------------------------------------------------
BArchivable *BTextView::Instantiate(BMessage *archive)
{
	if (validate_instantiation(archive, "BTextView"))
		return new BTextView(archive);
	else
		return NULL;
}
//------------------------------------------------------------------------------
status_t BTextView::Archive(BMessage *data, bool deep) const
{
	// TODO: write archive
	return BView::Archive(data, deep);
}
//------------------------------------------------------------------------------
void BTextView::AttachedToWindow()
{
	// TODO: set drawing mode to B_OP_COPY, adjust scrollbars, enable and set
	// pulse
}
//------------------------------------------------------------------------------
void BTextView::DetachedFromWindow()
{
	//TODO: if cursor in bounds, reset it to B_HAND_CURSOR
}
//------------------------------------------------------------------------------
void BTextView::Draw(BRect updateRect)
{
	int32 from = 0, to = 0;

	while (fLines->fObjectList[from].fHeight + fLines->fObjectList[from].fLineHeight
			< updateRect.top && from < CountLines())
		from++;

	while (to < fLines->fItemCount - 2 &&
		fLines->fObjectList[to].fHeight + fLines->fObjectList[to].fLineHeight
		< updateRect.bottom && to < CountLines())
		to++;

	if (from > 0)
		from--;

	const BFont *font;
	const rgb_color *color;

	fStyles->GetNullStyle(&font, &color);

	SetFont(font);
	SetHighColor(*color);

	DrawLines(from, to, 0, false);

	if (IsFocus())
	{
		if (fSelStart == fSelEnd)
		{
			/*float caretHeight;
			BPoint caret = PointAt(fSelStart, &caretHeight);
			StrokeLine(BPoint(caret.x, caret.y),
				BPoint(caret.x, caret.y + caretHeight));*/
			if (fCaretVisible)
				DrawCaret(fSelStart);
		}
		else
			Highlight(fSelStart, fSelEnd);
	}
}
//------------------------------------------------------------------------------
void BTextView::MouseDown(BPoint where)
{
	MakeFocus();
	SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);

	fClickOffset = OffsetAt(where);
	fTrackingMouse = new _BTextTrackState_(fSelStart < fClickOffset &&
		fClickOffset < fSelEnd);

	if (!fTrackingMouse->fInSelection)
		Select (fClickOffset, fClickOffset);

	SetMouseEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY | B_SUSPEND_VIEW_FOCUS);
}
//------------------------------------------------------------------------------
void BTextView::MouseUp(BPoint where)
{	
	if (fTrackingMouse)
	{
		if (!fTrackingMouse->fMoved)
			Select(fClickOffset, fClickOffset);
	
		delete fTrackingMouse;
		fTrackingMouse = NULL;
	}
}
//------------------------------------------------------------------------------
void BTextView::MouseMoved(BPoint where, uint32 code, const BMessage *message)
{
	// TODO: cursor changing, and message tracking
	switch (code)
	{
		case B_EXITED_VIEW:
			SetViewCursor(B_CURSOR_SYSTEM_DEFAULT);
			break;
		default:
			SetViewCursor(B_CURSOR_I_BEAM);
			break;
	}
		
	if (message == NULL)
	{
		if (fTrackingMouse)
		{
			if (fTrackingMouse->fInSelection)
			{
				BMessage msg(B_SIMPLE_DATA);

				msg.AddData("text/plain", B_MIME_TYPE, fText->Text() + fSelStart,
					fSelEnd - fSelStart);

				delete fTrackingMouse;
				fTrackingMouse = NULL;

				BBitmap *bitmap = NULL;
				BPoint point;
				BHandler *handler = this;

				GetDragParameters(&msg, &bitmap, &point, &handler);

				DragMessage(&msg, bitmap, point, handler);
			}
			else
			{
				int32 offset = OffsetAt(where);

				if (offset < fClickOffset)
					Select(offset, fClickOffset);
				else
					Select(fClickOffset, offset);

				fTrackingMouse->fMoved = true;
			}
		}
	}
	else
	{
		int32 offset = OffsetAt(where);

		Select(offset, offset);
	}
}
//------------------------------------------------------------------------------
void BTextView::WindowActivated(bool state)
{
	if (!state)
		fCaretVisible = false;
	
	Invalidate();	
	// TODO: handle highlighting when state changes
}
//------------------------------------------------------------------------------
void BTextView::KeyDown(const char *bytes, int32 numBytes)
{
	char key = bytes[0];
	
	switch (key)
	{
		case B_BACKSPACE:
			HandleBackspace();
			break;
		case B_LEFT_ARROW:
		case B_RIGHT_ARROW:
			HandleArrowKey(key);
			break;
		case B_DELETE:
			HandleDelete();
			break;
		case B_PAGE_UP:
		case B_PAGE_DOWN:
			HandlePageKey(key);
			break;
		default:
			HandleAlphaKey(bytes, numBytes);
	}
}
//------------------------------------------------------------------------------
void BTextView::Pulse()
{
	// TODO: blink caret
	if (Window()->IsActive() && IsFocus())
	{
		fCaretVisible = !fCaretVisible;
		
		if (fSelStart == fSelEnd)
			InvertCaret();
	}
}
//------------------------------------------------------------------------------
void BTextView::FrameResized(float width, float height)
{
	// TODO: update scrollbars
}
//------------------------------------------------------------------------------
void BTextView::MakeFocus(bool focusState)
{
	// TODO: change highlight according to focusState
	BView::MakeFocus(focusState);

	Invalidate();
}
//------------------------------------------------------------------------------
void BTextView::MessageReceived(BMessage *message)
{
	// TODO: Handle scripting
	switch (message->what)
	{
		case B_CUT:
			Cut(be_clipboard);
			break;
		case B_COPY:
			Copy(be_clipboard);
			break;
		case B_PASTE:
			Paste(be_clipboard);
			break;
		case B_SELECT_ALL:
			SelectAll ();
			break;
		case B_MIME_DATA:
		{
			if (!AcceptsDrop(message))
				break;

			const char *text;
			ssize_t len;

			if (message->FindData("text/plain", B_MIME_TYPE, (const void**)&text, &len) == B_OK)
			{
				InsertText(text, len, fSelStart, NULL);
				Select(fSelStart, fSelStart + len);
			}

			break;
		}
		default:
			BView::MessageReceived(message);
	}
}
//------------------------------------------------------------------------------
BHandler *BTextView::ResolveSpecifier(BMessage *message, int32 index,
									  BMessage *specifier, int32 form,
									  const char *property)
{
	BPropertyInfo propInfo(prop_list);
	BHandler *target = NULL;

	switch (propInfo.FindMatch(message, 0, specifier, form, property))
	{
		case B_ERROR:
			break;

		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
			target = this;
			break;
	}

	if (!target)
		target = BView::ResolveSpecifier(message, index, specifier, form,
			property);

	return target;
}
//------------------------------------------------------------------------------
status_t BTextView::GetSupportedSuites(BMessage *data)
{
	// TODO: check if the suite name is ok, according to the documentation it
	// is unnamed
	status_t err;

	if (data == NULL)
		return B_BAD_VALUE;

	err = data->AddString("suites", "");

	if (err != B_OK)
		return err;
	
	BPropertyInfo prop_info(prop_list);
	err = data->AddFlat("messages", &prop_info);

	if (err != B_OK)
		return err;
	
	return BView::GetSupportedSuites(data);
}
//------------------------------------------------------------------------------
status_t BTextView::Perform(perform_code d, void *arg)
{
	return B_ERROR;
}
//------------------------------------------------------------------------------
void BTextView::SetText(const char *inText, const text_run_array *inRuns)
{
	DeleteText(0, fText->fLogicalBytes);

	if (inText)
		InsertText(inText, strlen(inText), 0, NULL);
}
//------------------------------------------------------------------------------
void BTextView::SetText(const char *inText, int32 inLength,
						const text_run_array *inRuns)
{
	DeleteText(0, fText->fLogicalBytes);

	if (inText)
		InsertText(inText, inLength, 0, NULL);
}
//------------------------------------------------------------------------------
void BTextView::SetText (BFile *inFile, int32 startOffset, int32 inLength,
						 const text_run_array *inRuns)
{
	// TODO:
}
//------------------------------------------------------------------------------
void BTextView::Insert(const char *inText, const text_run_array *inRuns)
{
	Insert(fSelStart, inText, strlen(inText), inRuns);
}
//------------------------------------------------------------------------------
void BTextView::Insert(const char *inText, int32 inLength,
					   const text_run_array *inRuns)
{
	Insert(fSelStart, inText, inLength, inRuns);
}
//------------------------------------------------------------------------------
void BTextView::Insert(int32 startOffset, const char *inText, int32 inLength,
					   const text_run_array *inRuns)
{
	InsertText(inText, inLength, startOffset, inRuns);
}
//------------------------------------------------------------------------------
void BTextView::Delete()
{
	if (fSelStart != fSelEnd)
		Delete(fSelStart, fSelEnd);
}
//------------------------------------------------------------------------------
void BTextView::Delete(int32 startOffset, int32 endOffset)
{
	if (startOffset != endOffset)
		DeleteText(startOffset, endOffset);
}
//------------------------------------------------------------------------------
const char *BTextView::Text() const
{
	return fText->Text();
}
//------------------------------------------------------------------------------
int32 BTextView::TextLength() const
{
	return fText->fLogicalBytes;
}
//------------------------------------------------------------------------------
void BTextView::GetText(int32 offset, int32 length, char *buffer) const
{
	memcpy(buffer, fText->RealText() + offset, length);
	buffer[length] = '\0';
}
//------------------------------------------------------------------------------
uchar BTextView::ByteAt(int32 offset) const
{
	return fText->RealCharAt(offset);
}
//------------------------------------------------------------------------------
int32 BTextView::CountLines() const
{
	return fLines->fItemCount - 1;
}
//------------------------------------------------------------------------------
int32 BTextView::CurrentLine() const
{
	return LineAt(fSelStart);
}
//------------------------------------------------------------------------------
void BTextView::GoToLine(int32 index)
{
	fSelStart = fSelEnd = OffsetAt(index);
}
//------------------------------------------------------------------------------
void BTextView::Cut(BClipboard *clipboard)
{
	Copy(clipboard);
	DeleteText(fSelStart, fSelEnd);
}
//------------------------------------------------------------------------------
void BTextView::Copy(BClipboard *clipboard)
{
	BMessage *clip = NULL;

	if (clipboard->Lock())
	{
		clipboard->Clear(); 
   
   		clip = clipboard->Data();
		
		if (clip != NULL)
		{
			clip->AddData("text/plain", B_MIME_TYPE, Text() + fSelStart,
				fSelEnd - fSelStart); 
			clipboard->Commit();
		}
   }

   clipboard->Unlock(); 
}
//------------------------------------------------------------------------------
void BTextView::Paste(BClipboard *clipboard)
{
	BMessage *clip = NULL;

	if (clipboard->Lock())
	{
		clip = clipboard->Data();
		if (clip != NULL)
		{
			const char *text;
			ssize_t len;

			clip->FindData("text/plain", B_MIME_TYPE, (const void **)&text, &len);

			DeleteText(fSelStart, fSelEnd);
			InsertText(text, len, fSelStart, NULL);
		}

		clipboard->Unlock();
	}
}
//------------------------------------------------------------------------------
void BTextView::Clear()
{
	if (fSelStart != fSelEnd)
		DeleteText(fSelStart, fSelEnd);
}
//------------------------------------------------------------------------------
bool BTextView::AcceptsPaste(BClipboard *clipboard)
{
	// TODO: check if mime on clipboard is text/plain
	return (fEditable);
}
//------------------------------------------------------------------------------
bool BTextView::AcceptsDrop(const BMessage *inMessage)
{
	const void *data;

	return (fEditable &&
		inMessage->FindData("text/plain", B_MIME_TYPE, &data, NULL) == B_OK);
}
//------------------------------------------------------------------------------
void BTextView::Select(int32 startOffset, int32 endOffset)
{
	fSelStart = min(max((int32)0, startOffset), fText->fLogicalBytes);
	fSelEnd = min(max(fSelStart, endOffset), fText->fLogicalBytes);

	Invalidate();
}
//------------------------------------------------------------------------------
void BTextView::SelectAll()
{
	Select(0, fText->fLogicalBytes);
}
//------------------------------------------------------------------------------
void BTextView::GetSelection(int32 *outStart, int32 *outEnd) const
{
	*outStart = fSelStart;
	*outEnd = fSelEnd;
}
//------------------------------------------------------------------------------
void BTextView::SetFontAndColor(const BFont *inFont, uint32 inMode,
								const rgb_color *inColor)
{
	fStyles->SetNullStyle(inMode, inFont, inColor, 0);
}
//------------------------------------------------------------------------------
void BTextView::SetFontAndColor(int32 startOffset, int32 endOffset,
								const BFont *inFont, uint32 inMode,
								const rgb_color	*inColor)
{
	fStyles->SetNullStyle(inMode, inFont, inColor, 0);
}
//------------------------------------------------------------------------------
void BTextView::GetFontAndColor(int32 inOffset, BFont*outFont,
								rgb_color *outColor) const
{
	const BFont *font;
	const rgb_color *color;

	fStyles->GetNullStyle(&font, &color);

	if (outFont)
		*outFont = *font;

	if (outColor)
		*outColor = *color;
}
//------------------------------------------------------------------------------
void BTextView::GetFontAndColor(BFont *outFont, uint32 *outMode,
								rgb_color *outColor, bool *outEqColor) const
{
	const BFont *font;
	const rgb_color *color;

	fStyles->GetNullStyle(&font, &color);

	if (outFont)
		*outFont = *font;

	if (outColor)
		*outColor = *color;
}
//------------------------------------------------------------------------------
void BTextView::SetRunArray(int32 startOffset, int32 endOffset,
							const text_run_array *inRuns)
{
}
//------------------------------------------------------------------------------
text_run_array *BTextView::RunArray(int32 startOffset, int32 endOffset,
									int32 *outSize) const
{
/*	text_run_array *textRunArray = new text_run_array;

	textRunArray->count = 1;
	textRunArray->runs[0].offset = 0;
	textRunArray->runs[0].font = NULL;
	//textRunArray->runs[0].color;

	return textRunArray;*/

	return NULL;
}
//------------------------------------------------------------------------------
int32 BTextView::LineAt(int32 offset) const
{
	return fLines->OffsetToLine(offset);
}
//------------------------------------------------------------------------------
int32 BTextView::LineAt(BPoint point) const
{
	return fLines->PixelToLine(point.y - fTextRect.top);
}
//------------------------------------------------------------------------------
BPoint BTextView::PointAt(int32 inOffset, float *outHeight) const
{
	int32 line = LineAt(inOffset);

	if (outHeight)
		*outHeight = LineHeight(line);

	return BPoint(fTextRect.left +
		StringWidth(fText->RealText() + fLines->fObjectList[line].fOffset,
		inOffset - fLines->fObjectList[line].fOffset),
		fTextRect.top + fLines->fObjectList[line].fHeight);
}
//------------------------------------------------------------------------------
int32 BTextView::OffsetAt(BPoint point) const
{
	// Find the line we clicked on
	int32 line = LineAt(point);
	int32 offset = fLines->fObjectList[line].fOffset;

	point.x -= fTextRect.left;

	// If we clicked past the end of the line return last index
	if (point.x > fLines->fObjectList[line].fWidth)
	{
		if (fText->RealCharAt(fLines->fObjectList[line + 1].fOffset - 1) == '\n')
			return fLines->fObjectList[line + 1].fOffset - 1;
		else
			return fLines->fObjectList[line + 1].fOffset;
	}

	// Search offset
	float width = 0.0f, nextWidth;
	int32 startOffset = offset, nextOffset;

	while (point.x > width && offset < fLines->fObjectList[line + 1].fOffset - 1)
	{
		nextOffset = next_utf8(fText->RealText() + offset) - fText->RealText();
		nextWidth = StringWidth(fText->RealText(), nextOffset - startOffset);

		if (point.x < width + (nextWidth - width) * 0.5f)
			return offset;

		width = nextWidth;
		offset = nextOffset;
	}

	return offset;
}
//------------------------------------------------------------------------------
int32 BTextView::OffsetAt(int32 line) const
{
	if (line > fLines->fItemCount)
		return fText->fLogicalBytes;

	return fLines->fObjectList[line].fOffset;
}
//------------------------------------------------------------------------------
void BTextView::FindWord(int32 inOffset, int32 *outFromOffset,
						 int32 *outToOffset)
{
}
//------------------------------------------------------------------------------
bool BTextView::CanEndLine(int32 offset)
{
	switch(fText->RealCharAt(offset))
	{
		case B_SPACE:
		case B_TAB:
		case B_ENTER:
		case '=':
		case '+':
		case '-':
		case '<':
		case '>':
		case '^':
		case '/':
		case '|':
		case '&':
		case '*':
		case '\0':
			return true;
	}

	return false;
}
//------------------------------------------------------------------------------
float BTextView::LineWidth(int32 lineNum) const
{
	if (lineNum < 0)
		return fLines->fObjectList[0].fWidth;
	else if (lineNum > fLines->fItemCount - 2)
		return fLines->fObjectList[fLines->fItemCount - 2].fWidth;
	else
		return fLines->fObjectList[lineNum].fWidth;
}
//------------------------------------------------------------------------------
float BTextView::LineHeight(int32 lineNum) const
{
	if (lineNum < 0)
		return fLines->fObjectList[0].fLineHeight;
	else if (lineNum > fLines->fItemCount - 2)
		return fLines->fObjectList[fLines->fItemCount - 2].fLineHeight;
	else
		return fLines->fObjectList[lineNum].fLineHeight;
}
//------------------------------------------------------------------------------
float BTextView::TextHeight(int32 startLine, int32 endLine) const
{
	if (startLine < 0)
		startLine = 0;

	if (endLine > fLines->fItemCount - 1)
		endLine = fLines->fItemCount - 1;

	float height = 0.0f;

	for (int32 i = startLine; i < endLine + 1; i ++)
		height += fLines->fObjectList[i].fLineHeight + 3.0f;

	return height;
}
//------------------------------------------------------------------------------
void BTextView::GetTextRegion(int32 startOffset, int32 endOffset,
							  BRegion *outRegion) const
{
}
//------------------------------------------------------------------------------
void BTextView::ScrollToOffset(int32 inOffset)
{
	int32 line = LineAt(inOffset);
	BRect rect;

	rect.left = 0;
	rect.top = fLines->fObjectList[line].fHeight;
	rect.right = LineWidth(line);
	rect.bottom = rect.top + LineHeight(line);

	if (Bounds().Intersects(rect.InsetByCopy(0, 2)))
		return;

	if (rect.top < Bounds().top )
		ScrollTo(0, rect.top);
	else
		ScrollTo(0, rect.bottom - Bounds().Height());

	UpdateScrollbars();
}
//------------------------------------------------------------------------------
void BTextView::ScrollToSelection()
{
	ScrollToOffset(fSelStart);
}
//------------------------------------------------------------------------------
void BTextView::Highlight(int32 startOffset, int32 endOffset)
{
	rgb_color color = HighColor();
	SetHighColor(0, 0, 0, 128);

	int32 lineStart = LineAt(startOffset);
	int32 lineEnd = LineAt(endOffset);

	float selHeight;
	BPoint selStart, selEnd;

	for (int32 line = lineStart; line <= lineEnd; line++)
	{
		if (line == lineStart)
			selStart = PointAt(startOffset);
		else
			selStart = PointAt(OffsetAt(line));

		if (line == lineEnd)
			selEnd = PointAt(endOffset, &selHeight);
		else
		{
			selEnd = PointAt(OffsetAt(line), &selHeight);
			selEnd.x = LineWidth(line);
		}

		InvertRect(BRect(selStart.x, selStart.y, selEnd.x, selEnd.y + selHeight));
	}
	
	SetHighColor(color);
}
//------------------------------------------------------------------------------
void BTextView::SetTextRect(BRect rect)
{
}
//------------------------------------------------------------------------------
BRect BTextView::TextRect() const
{
	return BRect();
}
//------------------------------------------------------------------------------
void BTextView::SetStylable(bool stylable)
{
	fStylable = stylable;
}
//------------------------------------------------------------------------------
bool BTextView::IsStylable() const
{
	return fStylable;
}
//------------------------------------------------------------------------------
void BTextView::SetTabWidth(float width)
{
	fTabWidth = width;
}
//------------------------------------------------------------------------------
float BTextView::TabWidth() const
{
	return fTabWidth;
}
//------------------------------------------------------------------------------
void BTextView::MakeSelectable(bool selectable)
{
	fSelectable = selectable;
}
//------------------------------------------------------------------------------
bool BTextView::IsSelectable() const
{
	return fSelectable;
}
//------------------------------------------------------------------------------
void BTextView::MakeEditable(bool editable)
{
	fEditable = editable;
}
//------------------------------------------------------------------------------
bool BTextView::IsEditable() const
{
	return fEditable;
}
//------------------------------------------------------------------------------
void BTextView::SetWordWrap(bool wrap)
{
	fWrap = wrap;
}
//------------------------------------------------------------------------------
bool BTextView::DoesWordWrap() const
{
	return fWrap;
}
//------------------------------------------------------------------------------
void BTextView::SetMaxBytes(int32 max)
{
	fMaxBytes = max;
}
//------------------------------------------------------------------------------
int32 BTextView::MaxBytes() const
{
	return fMaxBytes;
}
//------------------------------------------------------------------------------
void BTextView::DisallowChar(uint32 aChar)
{
}
//------------------------------------------------------------------------------
void BTextView::AllowChar(uint32 aChar)
{
}
//------------------------------------------------------------------------------
void BTextView::SetAlignment(alignment flag)
{
	fAlignment = flag;
}
//------------------------------------------------------------------------------
alignment BTextView::Alignment() const
{
	return fAlignment;
}
//------------------------------------------------------------------------------
void BTextView::SetAutoindent(bool state)
{
	fAutoindent = state;
}
//------------------------------------------------------------------------------
bool BTextView::DoesAutoindent() const
{
	return fAutoindent;
}
//------------------------------------------------------------------------------
void BTextView::SetColorSpace(color_space colors)
{
	fColorSpace = colors;
}
//------------------------------------------------------------------------------
color_space BTextView::ColorSpace() const
{
	return fColorSpace;
}
//------------------------------------------------------------------------------
void BTextView::MakeResizable(bool resize, BView *resizeView)
{
	fResizable = resize;
	fContainerView = resizeView;
}
//------------------------------------------------------------------------------
bool BTextView::IsResizable () const
{
	return fResizable;
}
//------------------------------------------------------------------------------
void BTextView::SetDoesUndo(bool undo)
{
	
}
//------------------------------------------------------------------------------
bool BTextView::DoesUndo() const
{
	return false;
}
//------------------------------------------------------------------------------
void BTextView::HideTyping(bool enabled)
{
	
}
//------------------------------------------------------------------------------
bool BTextView::IsTypingHidden() const
{
	return false;
}
//------------------------------------------------------------------------------
void BTextView::ResizeToPreferred()
{
	float widht, height;
	GetPreferredSize(&widht, &height);
	BView::ResizeTo(widht,height);
}
//------------------------------------------------------------------------------
void BTextView::GetPreferredSize(float *width, float *height)
{
}
//------------------------------------------------------------------------------
void BTextView::AllAttached()
{
}
//------------------------------------------------------------------------------
void BTextView::AllDetached()
{
}
//------------------------------------------------------------------------------
void *BTextView::FlattenRunArray(const text_run_array *inArray,
								 int32 *outSize)
{
	// TODO: see TranslatorFormats for info
	return NULL;
}
//------------------------------------------------------------------------------
text_run_array *BTextView::UnflattenRunArray(const void	*data, int32 *outSize)
{
	// TODO: see TranslatorFormats for info
	return NULL;
}
//------------------------------------------------------------------------------
void BTextView::InsertText(const char *inText, int32 inLength, int32 inOffset,
						   const text_run_array *inRuns)
{
	//_ASSERT ( _CrtCheckMemory () );

	fText->InsertText(inText, inLength, inOffset);

	int32 line = fLines->OffsetToLine(inOffset);
	fLines->BumpOffset(line + 1, inLength);

	// Move selection if needed
	if (inOffset <= fSelStart)
	{
		fSelStart += inLength;
		fSelEnd += inLength;
	}

	//_ASSERT ( _CrtCheckMemory () );

	// Update line buffer
	Refresh(inOffset, inOffset + inLength, true, false);

	//_ASSERT ( _CrtCheckMemory () );
}
//------------------------------------------------------------------------------
void BTextView::DeleteText(int32 fromOffset, int32 toOffset)
{
	//_ASSERT ( _CrtCheckMemory () );

	fText->RemoveRange(fromOffset, toOffset);
	fLines->RemoveLineRange(fromOffset, toOffset);

	// Move selection if needed
	if (fromOffset <= fSelStart)
	{
		if (toOffset <= fSelStart)
		{
			fSelStart -= toOffset - fromOffset;
			fSelEnd -= toOffset - fromOffset;
		}
		else
		{
			fSelStart = fromOffset;
			fSelEnd = fromOffset;
		}
	}

	//_ASSERT ( _CrtCheckMemory () );

	// Update line buffer
	Refresh(fromOffset, fromOffset, true, false);

	//_ASSERT ( _CrtCheckMemory () );
}
//------------------------------------------------------------------------------
void BTextView::Undo(BClipboard *clipboard)
{
}
//------------------------------------------------------------------------------
undo_state BTextView::UndoState(bool *isRedo) const
{
	return B_UNDO_UNAVAILABLE;
}
//------------------------------------------------------------------------------
void BTextView::GetDragParameters(BMessage *drag, BBitmap **bitmap,
								  BPoint *point, BHandler **handler)
{
}
//------------------------------------------------------------------------------

//Private or Reserved
//------------------------------------------------------------------------------
void BTextView::InitObject(BRect textRect, const BFont *initialFont,
						   const rgb_color *initialColor)
{
	fTextRect = textRect;

	fText = new _BTextGapBuffer_;
	fLines = new _BLineBuffer_;
	fStyles = new _BStyleBuffer_(initialFont, initialColor);

	font_height fh;
	GetFontHeight(&fh);
	fLines->fObjectList[0].fLineHeight = (float)ceil(fh.ascent + fh.descent);
}
//------------------------------------------------------------------------------
void BTextView::HandleBackspace()
{
	if (!fEditable)
		return;
		
	if (fSelStart < fSelEnd)
	{
		DeleteText(fSelStart, fSelEnd);
	}
	else if ( fSelStart > 0 )
	{
		int32 selStart = previous_utf8(fText->RealText(),
			fText->RealText() + fSelStart) - fText->RealText();
		DeleteText(selStart, fSelStart);
	}
}
//------------------------------------------------------------------------------
void BTextView::HandleArrowKey(uint32 inArrowKey)
{
	switch(inArrowKey)
	{
		case B_LEFT_ARROW:
			if (fSelStart > 0)
			{
				int32 sel = previous_utf8(fText->RealText(),
					fText->RealText() + fSelStart) - fText->RealText();
				
				if (modifiers() & B_SHIFT_KEY)
					Select(sel, fSelEnd);
				else
					Select(sel, sel);
			}
			break;
		case B_RIGHT_ARROW:
		{
			int32 sel;

			if (fSelEnd < fText->fLogicalBytes - 1)
				sel= next_utf8(fText->RealText() + fSelEnd) - fText->RealText();
			else
				sel = fText->fLogicalBytes;
			
			if (modifiers() & B_SHIFT_KEY)
				Select(fSelStart, sel);
			else
				Select(sel, sel);

			break;
		}
		case B_DOWN_ARROW:
		{
			//TODO: Implement
			break;
		}
		case B_UP_ARROW:
		{
			//TODO: Implement
			break;
		}
	}
}
//------------------------------------------------------------------------------
void BTextView::HandleDelete()
{
	if (!fEditable)
		return;
		
	if (fSelStart < fSelEnd)
	{
		DeleteText(fSelStart, fSelEnd);
	}
	else if (fSelStart < fText->fLogicalBytes)
	{
		int32 selEnd = next_utf8(fText->RealText() + fSelStart) -
			fText->RealText();
		DeleteText(fSelStart, selEnd);
	}
}
//------------------------------------------------------------------------------
void BTextView::HandlePageKey(uint32 inPageKey)
{
	if (inPageKey == B_PAGE_DOWN)
	{
		GoToLine(CountLines() - 1); 	
	}
	else if (inPageKey == B_PAGE_UP)
	{
		GoToLine(0);
	}
	Invalidate();
}
//------------------------------------------------------------------------------
void BTextView::HandleAlphaKey(const char *bytes, int32 numBytes)
{
	if (!fEditable)
		return;
		
	if (fSelStart != fSelEnd)
		DeleteText(fSelStart, fSelEnd);

	InsertText(bytes, numBytes, fSelStart, NULL);
}
//------------------------------------------------------------------------------
void BTextView::Refresh(int32 fromOffset, int32 toOffset, bool erase,
						bool scroll)
{
	float ascent, descent, width;

	// Find the first line that changed
	int32 line = LineAt(fromOffset);
	int32 offset = FindLineBreak(OffsetAt(line), &ascent, &descent, &width);

	// Update the lineheight and width of this line
	fLines->fObjectList[line].fLineHeight = (float)ceil(ascent + descent);
	fLines->fObjectList[line].fWidth = width;
	line++;

	// Check if we have to insert new lines
	while (offset < toOffset)
	{
		STELine l;

		l.fOffset = offset + 1;
		l.fHeight = fLines->fObjectList[line - 1].fHeight +
			fLines->fObjectList[line - 1].fLineHeight + 3;
		l.fLineHeight = (float)ceil(ascent + descent);
		l.fWidth = width;

		fLines->InsertLine(&l, line);

		offset++;
		line++;
		offset = FindLineBreak(offset, &ascent, &descent, &width);
	}

	// Update the height of the remaining lines
	for (int i = line; i < fLines->fItemCount; i++)
		fLines->fObjectList[i].fHeight = fLines->fObjectList[i - 1].fHeight +
			fLines->fObjectList[i - 1].fLineHeight + 3;

	// Update the screen
	UpdateScrollbars();

	/*Invalidate(BRect(0.0f, PointAt(fromOffset).y, Bounds().right,
		PointAt(toOffset).y + LineHeight(LineAt(toOffset))));*/
	Invalidate();
}
//------------------------------------------------------------------------------
void BTextView::RecalLineBreaks(int32 *startLine, int32 *endLine)
{
}
//------------------------------------------------------------------------------
int32 BTextView::FindLineBreak(int32 fromOffset, float *outAscent,
							   float *outDescent, float	*ioWidth)
{
	if (fromOffset >= fText->fLogicalBytes)
	{
		// Fill in font height if needed
		if (outAscent && outDescent)
		{
			*outAscent = 0.0f;
			*outDescent = 0.0f;
		}

		return fromOffset;
	}

	// Find the actual linebreak
	int32 offset = fromOffset;
	char *ptr = fText->RealText();

	while(offset < fText->fLogicalBytes)
	{
		if (ptr[offset] == '\n' && ptr[offset + 1] != '\0')
			break;

		if (ptr[offset] == '\0')
			break;

		offset++;
	}

	// Fill in font height if needed
	if (outAscent && outDescent)
	{
		font_height fh;
		GetFontHeight(&fh);

		*outAscent = fh.ascent;
		*outDescent = fh.descent;
	}

	// Calculate width if needed
	if (ioWidth)
		*ioWidth = StringWidth(ptr + fromOffset, offset - fromOffset);

	return offset;
}
//------------------------------------------------------------------------------
float BTextView::StyledWidth(int32 fromOffset, int32 length, float *outAscent,
							 float *outDescent) const
{
	return 0.0f;
}
//------------------------------------------------------------------------------
float BTextView::ActualTabWidth(float location) const
{
	return 0.0f;
}
//------------------------------------------------------------------------------
void BTextView::DoInsertText(const char *inText, int32 inLength, int32 inOffset,
							 const text_run_array *inRuns,
							 _BTextChangeResult_ *outResult)
{
}
//------------------------------------------------------------------------------
void BTextView::DoDeleteText(int32 fromOffset, int32 toOffset,
							 _BTextChangeResult_ *outResult)
{
}
//------------------------------------------------------------------------------		
void BTextView::DrawLines(int32 startLine, int32 endLine, int32 startOffset,
						  bool erase)
{
	char *string;
	int32 length;

	font_height fh;
	GetFontHeight(&fh);

	for (int32 i = startLine; i < endLine + 1; i++)
	{
		string = fText->RealText() + fLines->fObjectList[i].fOffset;
		length = fLines->fObjectList[i + 1].fOffset -
			fLines->fObjectList[i].fOffset;

		if (length > 0)
		{
			if (fAlignment == B_ALIGN_LEFT)
				DrawString(string, length, BPoint(fTextRect.left,
					fTextRect.top + fLines->fObjectList[i].fHeight +
					fh.ascent));
			else if (fAlignment == B_ALIGN_CENTER)
				DrawString(string, length, BPoint(fTextRect.left +
					fTextRect.Width() / 2 - fLines->fObjectList[i].fWidth / 2,
					fTextRect.top + fLines->fObjectList[i].fHeight + fh.ascent));
			else
				DrawString(string, length, BPoint(fTextRect.right -
					fLines->fObjectList[i].fWidth, fTextRect.top +
					fLines->fObjectList[i].fHeight + fh.ascent));
		}
	}
}
//------------------------------------------------------------------------------
void BTextView::DrawCaret(int32 offset)
{
	Highlight(offset, offset);
}
//------------------------------------------------------------------------------
void BTextView::InvertCaret()
{
	Highlight(fSelStart, fSelStart);
}
//------------------------------------------------------------------------------
void BTextView::DragCaret(int32 offset)
{
}
//------------------------------------------------------------------------------
void BTextView::StopMouseTracking()
{
}
//------------------------------------------------------------------------------
bool BTextView::PerformMouseUp(BPoint where)
{
	return false;
}
//------------------------------------------------------------------------------
bool BTextView::PerformMouseMoved(BPoint where, uint32 code)
{
	return false;
}
//------------------------------------------------------------------------------
void BTextView::TrackMouse(BPoint where, const BMessage *message, bool force)
{
}
//------------------------------------------------------------------------------
void BTextView::TrackDrag(BPoint where)
{
}
//------------------------------------------------------------------------------
void BTextView::InitiateDrag()
{
}
//------------------------------------------------------------------------------
bool BTextView::MessageDropped(BMessage *inMessage, BPoint where, BPoint offset)
{
	return false;
}
//------------------------------------------------------------------------------				
void BTextView::UpdateScrollbars()
{
	BRect bounds(Bounds());
	BScrollBar *vertScroller = ScrollBar(B_VERTICAL);

	if (vertScroller)
	{
		float height = TextHeight(0, CountLines());

		if (bounds.Height() > height)
			vertScroller->SetRange(0.0f, 0.0f);
		else
		{
			vertScroller->SetRange(0.0f, height - bounds.Height());
			vertScroller->SetProportion(bounds.Height() / height);
		}
	}
}
//------------------------------------------------------------------------------
void BTextView::AutoResize(bool doredraw)
{
}
//------------------------------------------------------------------------------		
void BTextView::NewOffscreen(float padding)
{
}
//------------------------------------------------------------------------------
void BTextView::DeleteOffscreen()
{
}
//------------------------------------------------------------------------------
void BTextView::Activate()
{
}
//------------------------------------------------------------------------------
void BTextView::Deactivate()
{
}
//------------------------------------------------------------------------------
void BTextView::NormalizeFont(BFont *font)
{
}
//------------------------------------------------------------------------------
uint32 BTextView::CharClassification(int32 offset) const
{
	return 0;
}
//------------------------------------------------------------------------------
int32 BTextView::NextInitialByte(int32 offset) const
{
	return 0;
}
//------------------------------------------------------------------------------
int32 BTextView::PreviousInitialByte(int32 offset) const
{
	return 0;
}
//------------------------------------------------------------------------------
bool BTextView::GetProperty(BMessage *specifier, int32 form,
							const char *property, BMessage *reply)
{
	return false;
}
//------------------------------------------------------------------------------
bool BTextView::SetProperty(BMessage *specifier, int32 form,
							const char *property, BMessage *reply)
{
	return false;
}
//------------------------------------------------------------------------------
bool BTextView::CountProperties(BMessage *specifier, int32 form,
								const char *property, BMessage *reply)
{
	return false;
}
//------------------------------------------------------------------------------
void BTextView::HandleInputMethodChanged(BMessage *message)
{
}
//------------------------------------------------------------------------------
void BTextView::HandleInputMethodLocationRequest()
{
}
//------------------------------------------------------------------------------
void BTextView::CancelInputMethod()
{
}
//------------------------------------------------------------------------------
void BTextView::LockWidthBuffer()
{
}
//------------------------------------------------------------------------------
void BTextView::UnlockWidthBuffer()
{
}
//------------------------------------------------------------------------------

//FBC
void BTextView::_ReservedTextView3() {}
void BTextView::_ReservedTextView4() {}
void BTextView::_ReservedTextView5() {}
void BTextView::_ReservedTextView6() {}
void BTextView::_ReservedTextView7() {}
void BTextView::_ReservedTextView8() {}
void BTextView::_ReservedTextView9() {}
void BTextView::_ReservedTextView10() {}
void BTextView::_ReservedTextView11() {}
void BTextView::_ReservedTextView12() {}
/*
 * $Log $
 *
 * $Id  $
 *
 */
