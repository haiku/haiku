//------------------------------------------------------------------------------
//	Copyright (c) 2001-2003, OpenBeOS
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
//	Authors:		Hiroshi Lockheimer (TextView is based on his STEEngine)
//					Marc Flerackers (mflerackers@androme.be)
//					Stefano Ceccherini (burton666@libero.it)
//	Description:	BTextView displays and manages styled text.
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <cstdlib>
#include <cstdio>

// System Includes -------------------------------------------------------------
#include <Application.h>
#include <Bitmap.h>
#include <Clipboard.h>
#include <Errors.h>
#include <Message.h>
#include <PropertyInfo.h>
#include <Region.h>
#include <ScrollBar.h>
#include <TextView.h>
#include <Window.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "LineBuffer.h"
#include "StyleBuffer.h"
#include "TextGapBuffer.h"
#include "UndoBuffer.h"
#include "WidthBuffer.h"

// Local Defines ---------------------------------------------------------------
//#define TRACE_TEXTVIEW
#ifdef TRACE_TEXTVIEW
	#define CALLED() printf("%s\n", __PRETTY_FUNCTION__)
#else
	#define CALLED()
#endif

struct flattened_text_run {
	int32	offset;
	char	family[64];
	char	style[64];
	float	size;
	float	shear;		/* typically 90.0 */
	uint16	face;		/* typically 0 */
	uint8	red;
	uint8	green;
	uint8	blue;
	uint8	alpha;		/* 255 == opaque */
	uint16	_reserved_; /* 0 */
};


struct flattened_text_run_array {
	uchar				magic[4];	/* 41 6c 69 21 */
	uchar				version[4];	/* 00 00 00 00 */
	int32				count;
	flattened_text_run	styles[1];
};


enum {
	B_SEPARATOR_CHARACTER,
	B_OTHER_CHARACTER
} separatorCharacters;

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
static property_info
prop_list[] = {
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


// Initialized/finalized by init/fini_interface_kit ?
_BWidthBuffer_* BTextView::sWidths = NULL;
sem_id BTextView::sWidthSem = B_BAD_SEM_ID; 
int32 BTextView::sWidthAtom = 0;


//------------------------------------------------------------------------------
BTextView::BTextView(BRect frame, const char *name, BRect textRect,
					 uint32 resizeMask, uint32 flags)
	:	BView(frame, name, resizeMask, flags | B_FRAME_EVENTS | B_PULSE_NEEDED),
		fText(NULL),
		fLines(NULL),
		fStyles(NULL),
		fTextRect(textRect),
		fSelStart(0),
		fSelEnd(0),
		fCaretVisible(false),
		fCaretTime(0),
		fClickOffset(-1),
		fClickCount(0),
		fClickTime(0),
		fDragOffset(-1),
		//fCursor
		fActive(false),
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
	CALLED();
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
		fTextRect(textRect),
		fSelStart(0),
		fSelEnd(0),
		fCaretVisible(false),
		fCaretTime(0),
		fClickOffset(-1),
		fClickCount(0),
		fClickTime(0),
		fDragOffset(-1),
		//fCursor
		fActive(false),
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
	CALLED();
	InitObject(textRect, initialFont, initialColor);
}
//------------------------------------------------------------------------------
BTextView::BTextView(BMessage *archive)
	:	BView(archive)
{
	CALLED();
	BRect rect;

	if (archive->FindRect("_trect", &rect) != B_OK)
		rect.Set(0, 0, 0, 0);

	InitObject(rect, NULL, NULL); 
	
	const char *text = NULL;

	if (archive->FindString("_text", &text) == B_OK)
		SetText(text);

	int32 flag, flag2;

	if (archive->FindInt32("_align", &flag) == B_OK)
		SetAlignment((alignment)flag);

	float value;

	if (archive->FindFloat("_tab", &value) == B_OK)
		SetTabWidth(value);
	
	if (archive->FindInt32("_col_sp", &flag) == B_OK)
		SetColorSpace((color_space)flag);

	if (archive->FindInt32("_max", &flag) == B_OK)
		SetMaxBytes(flag);

	if (archive->FindInt32("_sel", &flag) == B_OK &&
		archive->FindInt32("_sel", &flag2) == B_OK)
		Select(flag, flag2);
	
	bool toggle;

	if (archive->FindBool("_stylable", &toggle) == B_OK)
		SetStylable(toggle);

	if (archive->FindBool("_auto_in", &toggle) == B_OK)
		SetAutoindent(toggle);

	if (archive->FindBool("_wrap", &toggle) == B_OK)
		SetWordWrap(toggle);

	if (archive->FindBool("_nsel", &toggle) == B_OK)
		MakeSelectable(!toggle);

	if (archive->FindBool("_nedit", &toggle) == B_OK)
		MakeEditable(!toggle);

	ssize_t disallowedCount = 0;
	const int32 *disallowedChars = NULL;
	if (archive->FindData("_dis_ch", B_RAW_TYPE,
		(const void **)&disallowedChars, &disallowedCount) == B_OK) {
		
		fDisallowedChars = new BList;
		disallowedCount /= sizeof(int32);
		for (int32 x = 0; x < disallowedCount; x++)
			fDisallowedChars->AddItem(reinterpret_cast<void *>(disallowedChars[x]));
	}
	
	ssize_t runSize = 0;
	const void *flattenedRunArray = NULL;
	if (archive->FindData("_runs", B_RAW_TYPE, &flattenedRunArray, &runSize) == B_OK) {
		
		text_run_array *runArray = UnflattenRunArray(runArray, (int32 *)&runSize);
		SetRunArray(0, TextLength(), runArray);
		free(runArray);
	}
	
}
//------------------------------------------------------------------------------
BTextView::~BTextView()
{
	CALLED();
	delete fText;
	delete fLines;
	delete fStyles;
	delete fDisallowedChars;
	delete fUndo;
	delete fOffscreen;
}
//------------------------------------------------------------------------------
BArchivable *
BTextView::Instantiate(BMessage *archive)
{
	CALLED();
	if (validate_instantiation(archive, "BTextView"))
		return new BTextView(archive);
	else
		return NULL;
}
//------------------------------------------------------------------------------
status_t 
BTextView::Archive(BMessage *data, bool deep) const
{
	CALLED();
	status_t err = BView::Archive(data, deep);
		
	data->AddString("_text", Text());
	data->AddInt32("_align", fAlignment);
	data->AddFloat("_tab", fTabWidth);
	data->AddInt32("_col_sp", fColorSpace);
	data->AddRect("_trect", fTextRect);
	data->AddInt32("_max", fMaxBytes);
	data->AddInt32("_sel", fSelStart);
	data->AddInt32("_sel", fSelEnd);	
	data->AddBool("_stylable", fStylable);
	data->AddBool("_auto_in", fAutoindent);
	data->AddBool("_wrap", fWrap);
	data->AddBool("_nsel", !fSelectable);
	data->AddBool("_nedit", !fEditable);
	
	if (fDisallowedChars != NULL)
		data->AddData("_dis_ch", B_RAW_TYPE, fDisallowedChars->Items(),
			fDisallowedChars->CountItems() * sizeof(int32));

	ssize_t runSize = 0;
	text_run_array *runArray = RunArray(0, TextLength());
	
	void *flattened = FlattenRunArray(runArray, (int32*)&runSize);	
	if (flattened != NULL) {
		data->AddData("_runs", B_RAW_TYPE, flattened, runSize);	
		free(flattened);
	}
	free(runArray);
	
	return err;
}
//------------------------------------------------------------------------------
void
BTextView::AttachedToWindow()
{
	CALLED();
	BView::AttachedToWindow();
	
	Window()->SetPulseRate(50000);
	
	fCaretVisible = false;
	fCaretTime = 0;
	fClickCount = 0;
	fClickTime = 0;
	fDragOffset = -1;
	fActive = false;
	
	if (fResizable)
		AutoResize(true);
	
	UpdateScrollbars();
}
//------------------------------------------------------------------------------
void
BTextView::DetachedFromWindow()
{
	CALLED();
	BView::DetachedFromWindow();
}
//------------------------------------------------------------------------------
void
BTextView::Draw(BRect updateRect)
{
	CALLED();
	// what lines need to be drawn?
	long startLine = LineAt(BPoint(0.0f, updateRect.top));
	long endLine = LineAt(BPoint(0.0f, updateRect.bottom));

	DrawLines(startLine, endLine);
	
	// draw the caret/hilite the selection
	if (fActive) {
		if (fSelStart != fSelEnd)
			Highlight(fSelStart, fSelEnd);
		else {
			if (fCaretVisible)
				DrawCaret(fSelStart);
		}
	}
}
//------------------------------------------------------------------------------
void
BTextView::MouseDown(BPoint where)
{
	CALLED();
	// should we even bother?
	if (!fEditable && !fSelectable)
		return;
	
	if (!IsFocus()) {
		MakeFocus();
		return;
	}
	
	// hide the caret if it's visible	
	if (fCaretVisible)
		InvertCaret();
	
	int32 mouseOffset = OffsetAt(where);	
	bool shiftDown = modifiers() & B_SHIFT_KEY;

	// should we initiate a drag?
	if (fSelStart != fSelEnd && !shiftDown) {
		BPoint loc;
		ulong buttons;
		GetMouse(&loc, &buttons);
		// TODO: here we shouldn't check for B_SECONDARY_MOUSE_BUTTON,
		// since dragging works also with the primary button.
		if (buttons == B_SECONDARY_MOUSE_BUTTON) {
			// was the click within the selection range?
			if (mouseOffset >= fSelStart && mouseOffset <= fSelEnd) {
				InitiateDrag();
				return;
			}
		}
	}
	
	// get the system-wide click speed
	bigtime_t clickSpeed = 0;
	get_click_speed(&clickSpeed);
	
	// is this a double/triple click, or is it a new click?
	if (clickSpeed > (system_time() - fClickTime) &&
		 mouseOffset == fClickOffset ) {
		if (fClickCount > 1) {
			// triple click
			fClickCount = 0;
			fClickTime = 0;
		} else {
			// double click
			fClickCount = 2;
			fClickTime = system_time();
		}
	} else {
		// new click
		fClickOffset = mouseOffset;
		fClickCount = 1;
		fClickTime = system_time();
	
		if (!shiftDown) {
			Select(mouseOffset, mouseOffset);
			if (fEditable)
				InvertCaret();
		}
	}
	
	// no need to track the mouse if we can't select
	if (!fSelectable)
		return;
		
	// track the mouse while it's down
	long start = 0;
	long end = 0;
	long anchor = (mouseOffset > fSelStart) ? fSelStart : fSelEnd;
	BPoint curMouse = where;
	ulong buttons = 0;
	do {
		if (mouseOffset > anchor) {
			start = anchor;
			end = mouseOffset;
		} else {
			start = mouseOffset;
			end = anchor;
		}
		
		switch (fClickCount) {
			case 0:
				// triple click, select line by line
				start = (*fLines)[LineAt(start)]->offset;
				end = (*fLines)[LineAt(end) + 1]->offset;
				break;
												
			case 2:
			{
				// double click, select word by word
				FindWord(mouseOffset, &start, &end);			
				break;
			}
				
			default:
				// new click, select char by char
				break;			
		}
		if (shiftDown) {
			if (mouseOffset > anchor)
				start = anchor;
			else
				end = anchor;
		}
		Select(start, end);
		
		// Should we scroll the view?
		if (!Bounds().Contains(curMouse)) {	
			float hDelta = 0;
			float vDelta = 0;
			if (ScrollBar(B_HORIZONTAL) != NULL) {
				if (curMouse.x < Bounds().left)
					hDelta = curMouse.x - Bounds().left;
				else {
					if (curMouse.x > Bounds().right)
						hDelta = curMouse.x - Bounds().right;
				}
				if (hDelta != 0)
					ScrollBar(B_HORIZONTAL)->SetValue(ScrollBar(B_HORIZONTAL)->Value() + hDelta);
			}
			if (ScrollBar(B_VERTICAL) != NULL) {
				if (curMouse.y < Bounds().top)
					vDelta = curMouse.y - Bounds().top;
				else if (curMouse.y > Bounds().bottom)
						vDelta = curMouse.y - Bounds().bottom;
				if (vDelta != 0)
					ScrollBar(B_VERTICAL)->SetValue(ScrollBar(B_VERTICAL)->Value() + vDelta);
			}
			if (hDelta != 0 || vDelta != 0)
				Window()->UpdateIfNeeded();
		}
		
		// Zzzz...
		snooze(30000);
		
		GetMouse(&curMouse, &buttons);
		mouseOffset = OffsetAt(curMouse);		
	} while (buttons != 0);
}
//------------------------------------------------------------------------------
void
BTextView::MouseUp(BPoint where)
{
	CALLED();
	PerformMouseUp(where);
}
//------------------------------------------------------------------------------
void
BTextView::MouseMoved(BPoint where, uint32 code, const BMessage *message)
{
	CALLED();
	switch (code) {
		case B_ENTERED_VIEW:
			if (fActive && message == NULL)
				SetViewCursor(B_CURSOR_I_BEAM);
			break;
			
		case B_INSIDE_VIEW:
			if (message != NULL) {
				if (AcceptsDrop(message))
					DragCaret(OffsetAt(where));
			}
			break;
			
		case B_EXITED_VIEW:
			DragCaret(-1);
			if (fActive)
				SetViewCursor(B_CURSOR_SYSTEM_DEFAULT);
			break;
			
		default:
			BView::MouseMoved(where, code, message);
			break;
	}
}
//------------------------------------------------------------------------------
void
BTextView::WindowActivated(bool state)
{
	CALLED();
	BView::WindowActivated(state);
	
	if (state && IsFocus()) {
		if (!fActive)
			Activate();
	} else {
		if (fActive)
			Deactivate();
	} 
}
//------------------------------------------------------------------------------
void
BTextView::KeyDown(const char *bytes, int32 numBytes)
{
	CALLED();
	// TODO: Remove this and move it to specific key handlers ?
	// moreover, we should check if ARROW keys works in case the object
	// isn't editable
	if (!fEditable)
		return;
	
	const char keyPressed = bytes[0];

	// hide the cursor and caret
	be_app->ObscureCursor();
	if (fCaretVisible)
		InvertCaret();
	
	switch (keyPressed) {
		case B_BACKSPACE:
			HandleBackspace();
			break;
			
		case B_LEFT_ARROW:
		case B_RIGHT_ARROW:
		case B_UP_ARROW:
		case B_DOWN_ARROW:
			HandleArrowKey(keyPressed);
			break;
		
		case B_DELETE:
			HandleDelete();
			break;
			
		case B_HOME:
		case B_END:
		case B_PAGE_UP:
		case B_PAGE_DOWN:
			HandlePageKey(keyPressed);
			break;
			
		case B_ESCAPE:
		case B_INSERT:
		case B_FUNCTION_KEY:
			// ignore, pass it up to superclass
			BView::KeyDown(bytes, numBytes);
			break;
			
		default:
			HandleAlphaKey(bytes, numBytes);
			break;
	}
	
	// draw the caret
	if (fSelStart == fSelEnd) {
		if (!fCaretVisible)
			InvertCaret();
	}
}
//------------------------------------------------------------------------------
void
BTextView::Pulse()
{
	CALLED();
	if (fActive && fEditable && fSelStart == fSelEnd) {
		if (system_time() > (fCaretTime + 500000.0))
			InvertCaret();
	}
}
//------------------------------------------------------------------------------
void
BTextView::FrameResized(float width, float height)
{
	CALLED();
	BView::FrameResized(width, height);

	UpdateScrollbars();
}
//------------------------------------------------------------------------------
void
BTextView::MakeFocus(bool focusState)
{
	CALLED();
	BView::MakeFocus(focusState);
	
	if (focusState && Window()->IsActive()) {
		if (!fActive)
			Activate();
	} else {
		if (fActive)
			Deactivate();
	} 
}
//------------------------------------------------------------------------------
void
BTextView::MessageReceived(BMessage *message)
{
	CALLED();
	// was this message dropped?
	if (message->WasDropped()) {
		BPoint dropLoc;
		BPoint offset;
		
		dropLoc = message->DropPoint(&offset);
		ConvertFromScreen(&dropLoc);
		ConvertFromScreen(&offset);
		if (!MessageDropped(message, dropLoc, offset))
			BView::MessageReceived(message);
		
		return;
	}

	switch (message->what) {
		case B_CUT:
			Cut(be_clipboard);
			break;
			
		case B_COPY:
			Copy(be_clipboard);
			break;
			
		case B_PASTE:
			Paste(be_clipboard);
			break;
		
		case B_UNDO:
			Undo(be_clipboard);
			break;
			
		case B_SELECT_ALL:
			SelectAll();
			break;
			
		case B_SET_PROPERTY:
		case B_GET_PROPERTY:
		case B_COUNT_PROPERTIES:
		{
			BPropertyInfo propInfo(prop_list);
			BMessage specifier;
			const char *property;

			if (message->GetCurrentSpecifier(NULL, &specifier) != B_OK ||
				specifier.FindString("property", &property) != B_OK)
				return;

			if (propInfo.FindMatch(message, 0, &specifier, specifier.what,
				property) == B_ERROR) {
				BView::MessageReceived(message);
				break;
			}

			switch(message->what) {

				case B_GET_PROPERTY:
				{
					BMessage reply;

					GetProperty(&specifier, specifier.what, property, &reply);

					message->SendReply(&reply);

					break;
				}
				case B_SET_PROPERTY:
				{
					BMessage reply;

					SetProperty(&specifier, specifier.what, property, &reply);

					message->SendReply(&reply);

					break;
				}
				case B_COUNT_PROPERTIES:
				{
					BMessage reply;

					CountProperties(&specifier, specifier.what, property, &reply);
					
					message->SendReply(&reply);

					break;
				}
				default:
					break;
			}

			break;
		}
		default:
			BView::MessageReceived(message);
			break;
	}
}
//------------------------------------------------------------------------------
BHandler *
BTextView::ResolveSpecifier(BMessage *message, int32 index,
									  BMessage *specifier, int32 form,
									  const char *property)
{
	CALLED();
	BPropertyInfo propInfo(prop_list);
	BHandler *target = NULL;

	switch (propInfo.FindMatch(message, 0, specifier, form, property)) {
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
			target = this;
			break;
		case B_ERROR:
		default:
			break;
	}

	if (!target)
		target = BView::ResolveSpecifier(message, index, specifier, form,
			property);

	return target;
}
//------------------------------------------------------------------------------
status_t
BTextView::GetSupportedSuites(BMessage *data)
{
	CALLED();
	status_t err;

	if (data == NULL)
		return B_BAD_VALUE;

	err = data->AddString("suites", "suite/vnd.Be-text-view");

	if (err != B_OK)
		return err;
	
	BPropertyInfo prop_info(prop_list);
	err = data->AddFlat("messages", &prop_info);

	if (err != B_OK)
		return err;
	
	return BView::GetSupportedSuites(data);
}
//------------------------------------------------------------------------------
status_t
BTextView::Perform(perform_code d, void *arg)
{
	CALLED();
	return BView::Perform(d, arg);
}
//------------------------------------------------------------------------------
void
BTextView::SetText(const char *inText, const text_run_array *inRuns)
{
	CALLED();
	// hide the caret/unhilite the selection
	if (fActive) {
		if (fSelStart != fSelEnd)
			Highlight(fSelStart, fSelEnd);
		else {
			if (fCaretVisible)
				InvertCaret();
		}
	}
	
	// remove data from buffer
	if (fText->Length() > 0)
		DeleteText(0, fText->Length()); // TODO: was fText->Length() - 1

	int len = (inText) ? strlen(inText) : 0;
	
	if (inText != NULL && len > 0)
		InsertText(inText, len, 0, inRuns);
	
	fSelStart = fSelEnd = 0;	

	// recalc line breaks and draw the text
	Refresh(0, len, true, true);

	// draw the caret
	if (fActive) {
		if (!fCaretVisible)
			InvertCaret();
	}
}
//------------------------------------------------------------------------------
void
BTextView::SetText(const char *inText, int32 inLength,
						const text_run_array *inRuns)
{
	CALLED();
	// hide the caret/unhilite the selection
	if (fActive) {
		if (fSelStart != fSelEnd)
			Highlight(fSelStart, fSelEnd);
		else {
			if (fCaretVisible)
				InvertCaret();
		}
	}
	
	// remove data from buffer
	if (fText->Length() > 0)
		DeleteText(0, fText->Length()); // TODO: was fText->Length() - 1
		
	if (inText != NULL && inLength > 0)
		InsertText(inText, inLength, 0, inRuns);
	
	fSelStart = fSelEnd = 0;	

	// recalc line breaks and draw the text
	Refresh(0, inLength, true, true);

	// draw the caret
	if (fActive) {
		if (!fCaretVisible)
			InvertCaret();
	}
}
//------------------------------------------------------------------------------
void
BTextView::SetText(BFile *inFile, int32 startOffset, int32 inLength,
						 const text_run_array *inRuns)
{
	CALLED();
	// TODO:
}
//------------------------------------------------------------------------------
void
BTextView::Insert(const char *inText, const text_run_array *inRuns)
{
	CALLED();
	Insert(fSelStart, inText, strlen(inText), inRuns);
}
//------------------------------------------------------------------------------
void
BTextView::Insert(const char *inText, int32 inLength,
					   const text_run_array *inRuns)
{
	CALLED();
	Insert(fSelStart, inText, inLength, inRuns);
}
//------------------------------------------------------------------------------
void
BTextView::Insert(int32 startOffset, const char *inText, int32 inLength,
					   const text_run_array *inRuns)
{
	CALLED();
	// do we really need to do anything?
	if (inLength < 1)
		return;
	
	// hide the caret/unhilite the selection
	if (fActive) {
		if (fSelStart != fSelEnd)
			Highlight(fSelStart, fSelEnd);
		else {
			if (fCaretVisible)
				InvertCaret();
		}
	}

	if (startOffset > TextLength())
		startOffset = TextLength();

	// copy data into buffer
	InsertText(inText, inLength, startOffset, inRuns);
	
	// offset the caret/selection
	long saveStart = fSelStart;
	fSelStart += inLength;
	fSelEnd += inLength;

	// recalc line breaks and draw the text
	Refresh(saveStart, startOffset, true, true);

	// draw the caret/hilite the selection
	if (fActive) {
		if (fSelStart != fSelEnd)
			Highlight(fSelStart, fSelEnd);
		else {
			if (!fCaretVisible)
				InvertCaret();
		}
	}
}
//------------------------------------------------------------------------------
void
BTextView::Delete()
{
	CALLED();
	// anything to delete?
	if (fSelStart == fSelEnd)
		return;
		
	// hide the caret/unhilite the selection
	if (fActive) {
		if (fSelStart != fSelEnd)
			Highlight(fSelStart, fSelEnd);
		else {
			if (fCaretVisible)
				InvertCaret();
		}
	}
	
	// remove data from buffer
	DeleteText(fSelStart, fSelEnd);
	
	// collapse the selection
	fSelEnd = fSelStart;
	
	// recalc line breaks and draw what's left
	Refresh(fSelStart, fSelEnd, true, true);
	
	// draw the caret
	if (fActive) {
		if (!fCaretVisible)
			InvertCaret();
	}
}
//------------------------------------------------------------------------------
void
BTextView::Delete(int32 startOffset, int32 endOffset)
{
	CALLED();
	// anything to delete?
	if (startOffset == endOffset)
		return;
		
	// hide the caret/unhilite the selection
	if (fActive) {
		if (startOffset != endOffset)
			Highlight(startOffset, endOffset);
		else {
			if (fCaretVisible)
				InvertCaret();
		}
	}
	
	// remove data from buffer
	DeleteText(startOffset, endOffset);
	
	// recalc line breaks and draw what's left
	Refresh(startOffset, endOffset, true, true);
	
	// draw the caret
	if (fActive) {
		if (!fCaretVisible)
			InvertCaret();
	}
}
//------------------------------------------------------------------------------
const char *
BTextView::Text() const
{
	CALLED();
	return fText->Text();
}
//------------------------------------------------------------------------------
int32
BTextView::TextLength() const
{
	CALLED();
	return fText->Length();
}
//------------------------------------------------------------------------------
void
BTextView::GetText(int32 offset, int32 length, char *buffer) const
{
	CALLED();
	int32 textLen = fText->Length();
	if (offset < 0 || offset > (textLen - 1)) {
		buffer[0] = '\0';
		return;
	}
		
	length = ((offset + length) > textLen) ? textLen - offset : length;
	for (int32 i = 0; i < length; i++)
		buffer[i] = (*fText)[i + offset];
	buffer[length] = '\0';
}
//------------------------------------------------------------------------------
uchar
BTextView::ByteAt(int32 offset) const
{
	CALLED();
	if (offset < 0 || offset > (fText->Length() - 1))
		return '\0';
		
	return (*fText)[offset];
}
//------------------------------------------------------------------------------
int32
BTextView::CountLines() const
{
	CALLED();
	return fLines->NumLines();
}
//------------------------------------------------------------------------------
int32
BTextView::CurrentLine() const
{
	CALLED();
	return LineAt(fSelStart);
}
//------------------------------------------------------------------------------
void
BTextView::GoToLine(int32 index)
{
	CALLED();
	fSelStart = fSelEnd = OffsetAt(index);
}
//------------------------------------------------------------------------------
void
BTextView::Cut(BClipboard *clipboard)
{
	CALLED();
	if (fUndo) {
		delete fUndo;
		fUndo = new _BCutUndoBuffer_(this);
	}
	Copy(clipboard);
	Delete();
}
//------------------------------------------------------------------------------
void
BTextView::Copy(BClipboard *clipboard)
{
	CALLED();
	BMessage *clip = NULL;

	if (clipboard->Lock()) {
		clipboard->Clear(); 
   
		if ((clip = clipboard->Data()) != NULL) {
			clip->AddData("text/plain", B_MIME_TYPE, Text() + fSelStart,
				fSelEnd - fSelStart);
			
			int32 size;
			if (fStylable) {
				text_run_array *runArray = RunArray(fSelStart, fSelEnd, &size);
				clip->AddData("application/x-vnd.Be-text_run_array", B_MIME_TYPE,
					runArray, size);
				free(runArray);
			}
			clipboard->Commit();
		}
		clipboard->Unlock(); 
	}	
}
//------------------------------------------------------------------------------
void
BTextView::Paste(BClipboard *clipboard)
{
	CALLED();
	BMessage *clip = NULL;

	if (clipboard->Lock()) { 
		clip = clipboard->Data();
		if (clip != NULL) {
			const char *text = NULL;
			ssize_t len = 0;
					
			if (clip->FindData("text/plain", B_MIME_TYPE,
					(const void **)&text, &len) == B_OK) {
				text_run_array *runArray = NULL;
				ssize_t runLen = 0;
				
				if (fStylable)
					clip->FindData("application/x-vnd.Be-text_run_array", B_MIME_TYPE,
						(const void **)&runArray, &runLen);

				if (fUndo) {
					delete fUndo;
					fUndo = new _BPasteUndoBuffer_(this, text, len, runArray, runLen);
				}
				if (fSelStart != fSelEnd)
					Delete();
					
				Insert(text, len, runArray);
			}		
		}

		clipboard->Unlock();
	}
}
//------------------------------------------------------------------------------
void
BTextView::Clear()
{
	CALLED();
	Delete();
}
//------------------------------------------------------------------------------
bool
BTextView::AcceptsPaste(BClipboard *clipboard)
{
	CALLED();
	if (!fEditable)
		return false;
		
	bool result = false;
	
	if (clipboard->Lock()) {
		BMessage *data = clipboard->Data();
		result = data && data->HasData("text/plain", B_MIME_TYPE);
		clipboard->Unlock();
	}

	return result;
}
//------------------------------------------------------------------------------
bool
BTextView::AcceptsDrop(const BMessage *inMessage)
{
	CALLED();
	if (fEditable && inMessage->HasData("text/plain", B_MIME_TYPE))
		return true;
			
	return false;
}
//------------------------------------------------------------------------------
void
BTextView::Select(int32 startOffset, int32 endOffset)
{
	CALLED();
	if (!fSelectable)
		return;
		
	// a negative selection?
	if (startOffset > endOffset)
		return;
		
	// is the new selection any different from the current selection?
	if (startOffset == fSelStart && endOffset == fSelEnd)
		return;
	
	fStyles->InvalidateNullStyle();
	
	// pin offsets at reasonable values
	startOffset = (startOffset < 0) ? 0 : startOffset;
	endOffset = (endOffset < 0) ? 0 : endOffset;
	endOffset = (endOffset > fText->Length()) ? fText->Length() : endOffset;

	// hide the caret
	if (fCaretVisible)
		InvertCaret();
	
	if (startOffset == endOffset) {
		if (fSelStart != fSelEnd) {
			// unhilite the selection
			if (fActive)
				Highlight(fSelStart, fSelEnd); 
		}
		fSelStart = fSelEnd = startOffset;
	} else {
		if (fActive) {
			// draw only those ranges that are different
			long start, end;
			if (startOffset != fSelStart) {
				// start of selection has changed
				if (startOffset > fSelStart) {
					start = fSelStart;
					end = startOffset;
				} else {
					start = startOffset;
					end = fSelStart;
				}
				Highlight(start, end);
			}
			
			if (endOffset != fSelEnd) {
				// end of selection has changed
				if (endOffset > fSelEnd) {
					start = fSelEnd;
					end = endOffset;
				} else {
					start = endOffset;
					end = fSelEnd;
				}
				Highlight(start, end);
			}
		}
		fSelStart = startOffset;
		fSelEnd = endOffset;
	}
}
//------------------------------------------------------------------------------
void
BTextView::SelectAll()
{
	CALLED();
	Select(0, fText->Length());
}
//------------------------------------------------------------------------------
void
BTextView::GetSelection(int32 *outStart, int32 *outEnd) const
{
	CALLED();
	if (fSelectable) {
		*outStart = fSelStart;
		*outEnd = fSelEnd;
	} else {
		*outStart = 0;
		*outEnd = 0;
	}
}
//------------------------------------------------------------------------------
void
BTextView::SetFontAndColor(const BFont *inFont, uint32 inMode,
								const rgb_color *inColor)
{
	CALLED();
	// hide the caret/unhilite the selection
	if (fActive) {
		if (fSelStart != fSelEnd)
			Highlight(fSelStart, fSelEnd);
		else {
			if (fCaretVisible)
				InvertCaret();
		}
	}
	
	BFont newFont = *inFont;
	NormalizeFont(&newFont);
	
	// add the style to the style buffer
	fStyles->SetStyleRange(fSelStart, fSelEnd, fText->Length(),
						  inMode, &newFont, inColor);
						
	if (inMode & doFont || inMode & doSize)
		// recalc the line breaks and redraw with new style
		Refresh(fSelStart, fSelEnd, fSelStart != fSelEnd, false);
	else
		// the line breaks wont change, simply redraw
		DrawLines(LineAt(fSelStart), LineAt(fSelEnd), fSelStart, true);
	
	// draw the caret/hilite the selection
	if (fActive) {
		if (fSelStart != fSelEnd)
			Highlight(fSelStart, fSelEnd);
		else {
			if (!fCaretVisible)
				InvertCaret();
		}
	}
}
//------------------------------------------------------------------------------
void
BTextView::SetFontAndColor(int32 startOffset, int32 endOffset,
								const BFont *inFont, uint32 inMode,
								const rgb_color	*inColor)
{
	CALLED();
	// hide the caret/unhilite the selection
	if (fActive) {
		if (startOffset != endOffset)
			Highlight(startOffset, endOffset);
		else {
			if (fCaretVisible)
				InvertCaret();
		}
	}
	
	BFont newFont = *inFont;
	NormalizeFont(&newFont);
	
	// add the style to the style buffer
	fStyles->SetStyleRange(startOffset, endOffset, fText->Length(),
						  inMode, &newFont, inColor);
						
	if (inMode & doFont || inMode & doSize)
		// recalc the line breaks and redraw with new style
		Refresh(startOffset, endOffset, startOffset != endOffset, false);
	else
		// the line breaks wont change, simply redraw
		DrawLines(LineAt(startOffset), LineAt(endOffset), startOffset, true);
	
	// draw the caret/hilite the selection
	if (fActive) {
		if (startOffset != endOffset)
			Highlight(startOffset, endOffset);
		else {
			if (!fCaretVisible)
				InvertCaret();
		}
	}
}
//------------------------------------------------------------------------------
void
BTextView::GetFontAndColor(int32 inOffset, BFont *outFont,
								rgb_color *outColor) const
{
	CALLED();
	fStyles->GetStyle(inOffset, outFont, outColor);
}
//------------------------------------------------------------------------------
void
BTextView::GetFontAndColor(BFont *outFont, uint32 *outMode,
								rgb_color *outColor, bool *outEqColor) const
{
	CALLED();
	// TODO fill in outMode and outEqColor
	fStyles->GetStyle(fSelStart, outFont, outColor);
	
	// XXX: This is a hack to make beshare work. 
	// We should use _BStyleBuffer_::ContinuousGetStyle() here.
	*outMode = doSize;
}
//------------------------------------------------------------------------------
void
BTextView::SetRunArray(int32 startOffset, int32 endOffset,
							const text_run_array *inRuns)
{
	CALLED();
	// pin offsets at reasonable values
	startOffset = (startOffset < 0) ? 0 : startOffset;
	endOffset = (endOffset < 0) ? 0 : endOffset;
	endOffset = (endOffset > fText->Length()) ? fText->Length() : endOffset;
	
	long numStyles = inRuns->count;
	if (numStyles > 0) {	
		int32 textLength = fText->Length();
		const text_run *theRun = &inRuns->runs[0];
		for (long index = 0; index < numStyles; index++) {
			long fromOffset = theRun->offset + startOffset;
			long toOffset = endOffset;
			if ((index + 1) < numStyles) {
				toOffset = (theRun + 1)->offset + startOffset;
				toOffset = (toOffset > endOffset) ? endOffset : toOffset;
			}

			fStyles->SetStyleRange(fromOffset, toOffset, textLength,
							  	  doAll, &theRun->font, &theRun->color);
							
			theRun++;
		}
		fStyles->InvalidateNullStyle();
		
		Refresh(startOffset, endOffset, true, false);
	}
}
//------------------------------------------------------------------------------
text_run_array *
BTextView::RunArray(int32 startOffset, int32 endOffset,
									int32 *outSize) const
{
	CALLED();
	STEStyleRangePtr result = fStyles->GetStyleRange(startOffset, endOffset - 1);

	if (result == NULL)
		return NULL;

	text_run_array *res = (text_run_array *)malloc(sizeof(int32) +
		(sizeof(text_run) * result->count));

	res->count = result->count;

	for (int32 i = 0; i < res->count; i++) {
		res->runs[i].offset = result->runs[i].offset;
		res->runs[i].font = &result->runs[i].style.font;
		res->runs[i].color = result->runs[i].style.color;
	}

	if (outSize != NULL)
		*outSize = sizeof(int32) + (sizeof(text_run) * res->count);

	return res;
}
//------------------------------------------------------------------------------
int32
BTextView::LineAt(int32 offset) const
{
	CALLED();
	return fLines->OffsetToLine(offset);
}
//------------------------------------------------------------------------------
int32
BTextView::LineAt(BPoint point) const
{
	CALLED();
	return fLines->PixelToLine(point.y - fTextRect.top);
}
//------------------------------------------------------------------------------
BPoint
BTextView::PointAt(int32 inOffset, float *outHeight) const
{
	CALLED();
	BPoint result;
	int32 textLength = fText->Length();
	int32 lineNum = LineAt(inOffset);
	STELinePtr line = (*fLines)[lineNum];
	float height = (line + 1)->origin - line->origin;
	
	result.x = 0.0;
	result.y = line->origin + fTextRect.top;
	
	// special case: go down one line if inOffset is a newline
	if (inOffset == textLength && (*fText)[textLength - 1] == '\n') {
		float ascent, descent;
		StyledWidth(inOffset, 1, &ascent, &descent);
		
		result.y += height;
		height = ascent + descent;
	} else {
		int32 offset = line->offset;
		int32 length = inOffset - line->offset;
		int32 numChars = length;
		bool foundTab = false;		
		do {
			foundTab = fText->FindChar('\t', offset, &numChars);
		
			result.x += StyledWidth(offset, numChars);
	
			if (foundTab) {
				result.x += fTabWidth - fmod(result.x, fTabWidth);
				numChars++;
			}
			
			offset += numChars;
			length -= numChars;
			numChars = length;
		} while (foundTab && length > 0);
	} 		

	// convert from text rect coordinates
	result.x += fTextRect.left - 1.0;

	// round up
	result.x = ceil(result.x);
	result.y = ceil(result.y);
	if (outHeight != NULL)
		*outHeight = height;
		
	return result;
}
//------------------------------------------------------------------------------
int32
BTextView::OffsetAt(BPoint point) const
{
	CALLED();
	// should we even bother?
	if (point.y >= fTextRect.bottom)
		return fText->Length();
	else if (point.y < fTextRect.top)
		return 0;

	int32 lineNum = LineAt(point);
	STELinePtr line = (*fLines)[lineNum];
	
	// special case: if point is within the text rect and PixelToLine()
	// tells us that it's on the last line, but if point is actually  
	// lower than the bottom of the last line, return the last offset 
	// (can happen for newlines)
	if (lineNum == (fLines->NumLines() - 1)) {
		if (point.y >= ((line + 1)->origin + fTextRect.top))
			return (fText->Length());
	}
	
	// convert to text rect coordinates
	point.x -= fTextRect.left;
	point.x = (point.x < 0.0) ? 0.0 : point.x;
	
	// do a pseudo-binary search of the character widths on the line
	// that PixelToLine() gave us
	// note: the right half of a character returns its offset + 1
	int32 offset = line->offset;
	int32 saveOffset = offset;
	int32 delta = 0;
	int32 limit = (line + 1)->offset;
	int32 length = limit - line->offset;
	float sigmaWidth = 0.0;
	float tabWidth = 0.0;
	int32 numChars = length;
	bool done = false;
	bool foundTab = false;
	do {
		saveOffset = offset;
		
		if (foundTab) {
			// is the point in the right-half of the tab?
			if (point.x >= (sigmaWidth - (tabWidth / 2)) &&
				point.x < sigmaWidth)
				break;
		}
		
		// any more tabs?
		foundTab = fText->FindChar('\t', offset, &numChars);
		
		delta = numChars / 2;
		delta = (delta < 1) ? 1 : delta;
		
		if (numChars > 1) {
			do {
				float deltaWidth = StyledWidth(offset, delta);
				float leftWidth = StyledWidth(offset + delta - 1, 1);
				sigmaWidth += deltaWidth;
	
				if (point.x >= (sigmaWidth - (leftWidth / 2))) {
					// we're to the left of the point
					float rightWidth = StyledWidth(offset + delta, 1);
					if (point.x < (sigmaWidth + (rightWidth / 2))) {
						// the next character is to the right, we're done!
						offset += delta;
						done = true;
						break;
					} else {
						// still too far to the left, measure some more
						offset += delta;
						delta /= 2;
						delta = (delta < 1) ? 1 : delta;
					}
				} else {
					// oops, we overshot the point, go back some 
					sigmaWidth -= deltaWidth;
					
					if (delta == 1) {
						done = true;
						break;
					}
						
					delta /= 2;
					delta = (delta < 1) ? 1 : delta;
	
				}
			} while (offset < (numChars + saveOffset));
		}
		
		if (done || offset >= limit)
			break;
			
		if (foundTab) {
			tabWidth = fTabWidth - fmod(sigmaWidth, fTabWidth);
			
			// is the point in the left-half of the tab?
			if (point.x < (sigmaWidth + (tabWidth / 2)))
				break;
				
			sigmaWidth += tabWidth;
			numChars++;
		}

		offset = saveOffset + numChars;
		length -= numChars;
		numChars = length;
	} while (foundTab && length > 0);
	
	if (offset == (line + 1)->offset) {
		// special case: newlines aren't visible
		// return the offset of the character preceding the newline
		if ((*fText)[offset - 1] == '\n')
			return --offset;

		// special case: return the offset preceding any spaces that 
		// aren't at the end of the buffer
		if (offset != fText->Length() && (*fText)[offset - 1] == ' ')
			return --offset;
	}
	
	return offset;
}
//------------------------------------------------------------------------------
int32
BTextView::OffsetAt(int32 line) const
{
	if (line > fLines->NumLines())
		return fText->Length();

	return (*fLines)[line]->offset;
}
//------------------------------------------------------------------------------
void 
BTextView::FindWord(int32 inOffset, int32 *outFromOffset,
						 int32 *outToOffset)
{
	int32 offset;

	// check to the left
	for (offset = inOffset; offset > 0; offset--) {
		if (CharClassification(offset - 1) == B_SEPARATOR_CHARACTER)
			break;
	}

	*outFromOffset = offset;

	// check to the right
	int32 textLen = TextLength();
	for (offset = inOffset; offset < textLen; offset++) {
		if (CharClassification(offset) == B_SEPARATOR_CHARACTER)
			break;
	}
	
	*outToOffset = offset;
}

//------------------------------------------------------------------------------
bool
BTextView::CanEndLine(int32 offset)
{
	CALLED();
	return (CharClassification(offset) == B_SEPARATOR_CHARACTER);
}
//------------------------------------------------------------------------------
float
BTextView::LineWidth(int32 lineNum) const
{
	CALLED();
	if (lineNum < 0)
		return (*fLines)[0]->width;
	else if (lineNum > fLines->NumLines() - 1)
		return (*fLines)[fLines->NumLines() - 1]->width;
	else
		return (*fLines)[lineNum]->width;
}
//------------------------------------------------------------------------------
float
BTextView::LineHeight(int32 lineNum) const
{
	CALLED();
	if (lineNum < 0)
		return (*fLines)[0]->ascent;
	else if (lineNum > fLines->NumLines() - 1)
		return (*fLines)[fLines->NumLines() - 1]->ascent;
	else
		return (*fLines)[lineNum]->ascent;
}
//------------------------------------------------------------------------------
float
BTextView::TextHeight(int32 startLine, int32 endLine) const
{
	CALLED();
	int32 numLines = fLines->NumLines();
	startLine = (startLine < 0) ? 0 : startLine;
	endLine = (endLine > numLines - 1) ? numLines - 1 : endLine;
	
	float height = (*fLines)[endLine + 1]->origin - 
				   (*fLines)[startLine]->origin;
				
	if (endLine == numLines - 1 && (*fText)[fText->Length() - 1] == '\n')
		height += (*fLines)[endLine + 1]->origin - (*fLines)[endLine]->origin;
	
	return height;
}
//------------------------------------------------------------------------------
void
BTextView::GetTextRegion(int32 startOffset, int32 endOffset,
							  BRegion *outRegion) const
{
	CALLED();
	outRegion->MakeEmpty();

	// return an empty region if the range is invalid
	if (startOffset >= endOffset)
		return;

	float startLineHeight = 0.0;
	float endLineHeight = 0.0;
	BPoint startPt = PointAt(startOffset, &startLineHeight);
	BPoint endPt = PointAt(endOffset, &endLineHeight);
	
	BRect selRect;

	if (startPt.y == endPt.y) {
		// this is a one-line region
		selRect.left = (startPt.x < fTextRect.left) ? fTextRect.left : startPt.x;
		selRect.top = startPt.y;
		selRect.right = endPt.x - 1.0;
		selRect.bottom = endPt.y + endLineHeight - 1.0;
		outRegion->Include(selRect);
	} else {
		// more than one line in the specified offset range
		selRect.left = (startPt.x < fTextRect.left) ? fTextRect.left : startPt.x;
		selRect.top = startPt.y;
		selRect.right = fTextRect.right;
		selRect.bottom = startPt.y + startLineHeight - 1.0;
		outRegion->Include(selRect);
		
		if (startPt.y + startLineHeight < endPt.y) {
			// more than two lines in the range
			selRect.left = fTextRect.left;
			selRect.top = startPt.y + startLineHeight + 1.0;
			selRect.right = fTextRect.right;
			selRect.bottom = endPt.y - 1.0;
			outRegion->Include(selRect);
		}
		
		selRect.left = fTextRect.left;
		selRect.top = endPt.y;
		selRect.right = endPt.x - 1.0;
		selRect.bottom = endPt.y + endLineHeight - 1.0;
		outRegion->Include(selRect);
	}
}
//------------------------------------------------------------------------------
void
BTextView::ScrollToOffset(int32 inOffset)
{
	CALLED();
	BRect bounds = Bounds();
	//STELinePtr	line = (*fLines)[LineAt(inOffset)];
	float lineHeight = 0.0;
	BPoint point = PointAt(inOffset, &lineHeight);
	
	if (ScrollBar(B_HORIZONTAL) != NULL) {
		if (point.x < bounds.left || point.x >= bounds.right)
			ScrollBar(B_HORIZONTAL)->SetValue(point.x - (bounds.IntegerWidth() / 2));
	}
	
	if (ScrollBar(B_VERTICAL) != NULL) {
		if (point.y < bounds.top || (point.y + lineHeight) >= bounds.bottom)
			ScrollBar(B_VERTICAL)->SetValue(point.y - (bounds.IntegerHeight() / 2));
	}
}
//------------------------------------------------------------------------------
void
BTextView::ScrollToSelection()
{
	CALLED();
	ScrollToOffset(fSelStart);
}
//------------------------------------------------------------------------------
void
BTextView::Highlight(int32 startOffset, int32 endOffset)
{
	CALLED();
	// get real
	if (startOffset >= endOffset)
		return;
		
	BRegion selRegion;
	GetTextRegion(startOffset, endOffset, &selRegion);
	
	SetDrawingMode(B_OP_INVERT);	
	FillRegion(&selRegion, B_SOLID_HIGH);
	SetDrawingMode(B_OP_COPY);
}
//------------------------------------------------------------------------------
void
BTextView::SetTextRect(BRect rect)
{
	CALLED();
	if (rect == fTextRect)
		return;
		
	fTextRect = rect;
	
	if (Window() != NULL)
		Refresh(0, LONG_MAX, true, false);
}
//------------------------------------------------------------------------------
BRect
BTextView::TextRect() const
{
	CALLED();
	return fTextRect;
}
//------------------------------------------------------------------------------
void
BTextView::SetStylable(bool stylable)
{
	CALLED();
	fStylable = stylable;
}
//------------------------------------------------------------------------------
bool
BTextView::IsStylable() const
{
	CALLED();
	return fStylable;
}
//------------------------------------------------------------------------------
void
BTextView::SetTabWidth(float width)
{
	CALLED();
	if (width == fTabWidth)
		return;
		
	fTabWidth = width;
	
	if (Window() != NULL)
		Refresh(0, LONG_MAX, true, false);
}
//------------------------------------------------------------------------------
float
BTextView::TabWidth() const
{
	CALLED();
	return fTabWidth;
}
//------------------------------------------------------------------------------
void
BTextView::MakeSelectable(bool selectable)
{
	CALLED();
	if (selectable == fSelectable)
		return;
		
	fSelectable = selectable;
	
	if (Window() != NULL) {
		if (fActive) {
			// show/hide the caret, hilite/unhilite the selection
			if (fSelStart != fSelEnd)
				Highlight(fSelStart, fSelEnd);
			else
				InvertCaret();
		}
	}
}
//------------------------------------------------------------------------------
bool
BTextView::IsSelectable() const
{
	CALLED();
	return fSelectable;
}
//------------------------------------------------------------------------------
void
BTextView::MakeEditable(bool editable)
{
	CALLED();
	if (editable == fEditable)
		return;
		
	fEditable = editable;
	
	if (Window() != NULL) {
		if (fActive) {
			if (!fEditable && fCaretVisible)
				InvertCaret();
		}
	}
}
//------------------------------------------------------------------------------
bool
BTextView::IsEditable() const
{
	CALLED();
	return fEditable;
}
//------------------------------------------------------------------------------
void
BTextView::SetWordWrap(bool wrap)
{
	CALLED();
	if (wrap == fWrap)
		return;
		
	fWrap = wrap;
	
	if (Window() != NULL) {
		if (fActive) {
			// hide the caret, unhilite the selection
			if (fSelStart != fSelEnd)
				Highlight(fSelStart, fSelEnd);
			else {
				if (fCaretVisible)
					InvertCaret();
			}
		}
		
		Refresh(0, LONG_MAX, true, false);
		
		if (fActive) {
			// show the caret, hilite the selection
			if (fSelStart != fSelEnd)
				Highlight(fSelStart, fSelEnd);
			else {
				if (!fCaretVisible)
					InvertCaret();
			}
		}
	}
}
//------------------------------------------------------------------------------
bool
BTextView::DoesWordWrap() const
{
	CALLED();
	return fWrap;
}
//------------------------------------------------------------------------------
void
BTextView::SetMaxBytes(int32 max)
{
	CALLED();
	fMaxBytes = max;
}
//------------------------------------------------------------------------------
int32
BTextView::MaxBytes() const
{
	CALLED();
	return fMaxBytes;
}
//------------------------------------------------------------------------------
void
BTextView::DisallowChar(uint32 aChar)
{
	CALLED();
	if (fDisallowedChars == NULL)
		fDisallowedChars = new BList;
	if (!fDisallowedChars->HasItem(reinterpret_cast<void *>(aChar)))
		fDisallowedChars->AddItem(reinterpret_cast<void *>(aChar));
}
//------------------------------------------------------------------------------
void
BTextView::AllowChar(uint32 aChar)
{
	CALLED();
	if (fDisallowedChars != NULL)
		fDisallowedChars->RemoveItem(reinterpret_cast<void *>(aChar));
}
//------------------------------------------------------------------------------
void
BTextView::SetAlignment(alignment flag)
{
	CALLED();
	fAlignment = flag;
}
//------------------------------------------------------------------------------
alignment
BTextView::Alignment() const
{
	CALLED();
	return fAlignment;
}
//------------------------------------------------------------------------------
void
BTextView::SetAutoindent(bool state)
{
	CALLED();
	fAutoindent = state;
}
//------------------------------------------------------------------------------
bool
BTextView::DoesAutoindent() const
{
	CALLED();
	return fAutoindent;
}
//------------------------------------------------------------------------------
void
BTextView::SetColorSpace(color_space colors)
{
	CALLED();
	fColorSpace = colors;
}
//------------------------------------------------------------------------------
color_space
BTextView::ColorSpace() const
{
	CALLED();
	return fColorSpace;
}
//------------------------------------------------------------------------------
void
BTextView::MakeResizable(bool resize, BView *resizeView)
{
	CALLED();
	fResizable = resize;
	fContainerView = resizeView;
}
//------------------------------------------------------------------------------
bool
BTextView::IsResizable () const
{
	CALLED();
	return fResizable;
}
//------------------------------------------------------------------------------
void
BTextView::SetDoesUndo(bool undo)
{
	CALLED();
	if (undo && fUndo == NULL)
		fUndo = new _BUndoBuffer_(this, B_UNDO_UNAVAILABLE);
	else if (!undo && fUndo != NULL) {
		delete fUndo;
		fUndo = NULL;
	}
}
//------------------------------------------------------------------------------
bool
BTextView::DoesUndo() const
{
	CALLED();
	return fUndo != NULL;
}
//------------------------------------------------------------------------------
void
BTextView::HideTyping(bool enabled)
{
	CALLED();
	//TODO: Implement ?
	//fText->SetPasswordMode(enabled);
}
//------------------------------------------------------------------------------
bool
BTextView::IsTypingHidden() const
{
	CALLED();
	return fText->PasswordMode();
}
//------------------------------------------------------------------------------
void
BTextView::ResizeToPreferred()
{
	CALLED();
	float widht, height;
	GetPreferredSize(&widht, &height);
	BView::ResizeTo(widht, height);
}
//------------------------------------------------------------------------------
void
BTextView::GetPreferredSize(float *width, float *height)
{
	CALLED();
	BView::GetPreferredSize(width, height);
}
//------------------------------------------------------------------------------
void
BTextView::AllAttached()
{
	CALLED();
	BView::AllAttached();
	printf("selstart %ld, selend %ld\n", fSelStart, fSelEnd);
}
//------------------------------------------------------------------------------
void
BTextView::AllDetached()
{
	CALLED();
	BView::AllDetached();
}
//------------------------------------------------------------------------------
void *
BTextView::FlattenRunArray(const text_run_array *inArray, int32 *outSize)
{
	CALLED();
	int32 size = sizeof(flattened_text_run_array) + (inArray->count - 1) *
		sizeof(flattened_text_run_array);

	flattened_text_run_array *array = (flattened_text_run_array *)malloc(size);

	array->magic[0] = 0x41;
	array->magic[1] = 0x6c;
	array->magic[2] = 0x69;
	array->magic[3] = 0x21;
	array->version[0] = 0x00;
	array->version[1] = 0x00;
	array->version[2] = 0x00;
	array->version[3] = 0x00;
	array->count = inArray->count;

	for (int32 i = 0; i < inArray->count; i++) {
		array->styles[i].offset = inArray->runs[i].offset;
		inArray->runs[i].font.GetFamilyAndStyle(&array->styles[i].family,
			&array->styles[i].style);
		array->styles[i].size = inArray->runs[i].font.Size();
		array->styles[i].shear = inArray->runs[i].font.Shear();
		array->styles[i].face = inArray->runs[i].font.Face();
		array->styles[i].red = inArray->runs[i].color.red;
		array->styles[i].green = inArray->runs[i].color.green;
		array->styles[i].blue = inArray->runs[i].color.blue;
		array->styles[i].alpha = 255;
		array->styles[i]._reserved_ = 0;
	}

	if (outSize)
		*outSize = size;

	return array;
}
//------------------------------------------------------------------------------
text_run_array *
BTextView::UnflattenRunArray(const void	*data, int32 *outSize)
{
	CALLED();
	flattened_text_run_array *array = (flattened_text_run_array *)data;

	if (array->magic[0] != 0x41 || array->magic[1] != 0x6c ||
		array->magic[2] != 0x69 || array->magic[3] != 0x21 ||
		array->version[0] != 0x00 || array->version[1] != 0x00 ||
		array->version[2] != 0x00 || array->version[3] != 0x00) {
		
		if (outSize)
			*outSize = 0;

		return NULL;
	}

	int32 size = sizeof(text_run_array) + (array->count - 1) * sizeof(text_run);

	text_run_array *run_array = (text_run_array *)malloc(size);

	run_array->count = array->count;

	for (int32 i = 0; i < array->count; i++) {
		run_array->runs[i].font = new BFont;
		run_array->runs[i].font.SetFamilyAndStyle(array->styles[i].family,
			array->styles[i].style);
		run_array->runs[i].font.SetSize(array->styles[i].size);
		run_array->runs[i].font.SetShear(array->styles[i].shear);
		run_array->runs[i].font.SetFace(array->styles[i].face);
		run_array->runs[i].color.red = array->styles[i].red;
		run_array->runs[i].color.green = array->styles[i].green;
		run_array->runs[i].color.blue = array->styles[i].blue;
		run_array->runs[i].color.alpha = array->styles[i].alpha;
	}

	if (outSize)
		*outSize = size;

	return run_array;
}
//------------------------------------------------------------------------------
void
BTextView::InsertText(const char *inText, int32 inLength, int32 inOffset,
						   const text_run_array *inRuns)
{
	CALLED();
	// why add nothing?
	if (inLength < 1)
		return;
	
	// add the text to the buffer
	fText->InsertText(inText, inLength, inOffset);
	
	// update the start offsets of each line below offset
	fLines->BumpOffset(inLength, LineAt(inOffset) + 1);
	
	// update the style runs
	fStyles->BumpOffset(inLength, fStyles->OffsetToRun(inOffset - 1) + 1);
	
	if (inRuns != NULL)
		//SetStyleRange(inOffset, inOffset + inLength, inStyles, false);
		SetRunArray(inOffset, inOffset + inLength, inRuns);
	else {
		// apply nullStyle to inserted text
		fStyles->SyncNullStyle(inOffset);
		fStyles->SetStyleRange(inOffset, inOffset + inLength, 
							  fText->Length(), doAll, NULL, NULL);
	}
}
//------------------------------------------------------------------------------
void
BTextView::DeleteText(int32 fromOffset, int32 toOffset)
{
	CALLED();
	// sanity checking
	if (fromOffset >= toOffset || fromOffset < 0 || toOffset < 0)
		return;
		
	// set nullStyle to style at beginning of range
	fStyles->InvalidateNullStyle();
	fStyles->SyncNullStyle(fromOffset);	
	
	// remove from the text buffer
	fText->RemoveRange(fromOffset, toOffset);
	
	// remove any lines that have been obliterated
	fLines->RemoveLineRange(fromOffset, toOffset);
	
	// remove any style runs that have been obliterated
	fStyles->RemoveStyleRange(fromOffset, toOffset);
}
//------------------------------------------------------------------------------
void
BTextView::Undo(BClipboard *clipboard)
{
	CALLED();
	if (fUndo)
		fUndo->Undo(clipboard);
}
//------------------------------------------------------------------------------
undo_state
BTextView::UndoState(bool *isRedo) const
{
	CALLED();
	return fUndo == NULL ? B_UNDO_UNAVAILABLE : fUndo->State(isRedo);
}
//------------------------------------------------------------------------------
void
BTextView::GetDragParameters(BMessage *drag, BBitmap **bitmap,
								  BPoint *point, BHandler **handler)
{
	CALLED();
	if (drag == NULL)
		return;

	// What is this for ? 
	drag->AddInt32("be_actions", B_TRASH_TARGET);
	
	// add the text
	drag->AddData("text/plain", B_MIME_TYPE, fText->Text() + fSelStart, 
			  	  fSelEnd - fSelStart);
	
	// add the corresponding styles
	int32 size = 0;
	text_run_array *styles = RunArray(fSelStart, fSelEnd, &size);
	
	drag->AddData("application/x-vnd.Be-text_run_array", B_MIME_TYPE,
		styles, size);
	
	// add the message originator	
	drag->AddPointer("be:originator", this);
	
	free(styles);

	if (bitmap != NULL)
		*bitmap = NULL;
	if (handler != NULL)
		*handler = NULL;
}
//------------------------------------------------------------------------------
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
//------------------------------------------------------------------------------
void
BTextView::InitObject(BRect textRect, const BFont *initialFont,
						   const rgb_color *initialColor)
{
	CALLED();
	fTextRect = textRect;

	BFont font;
	if (initialFont == NULL)
		GetFont(&font);
	else
		font = *initialFont;
		
	NormalizeFont(&font);
	
	rgb_color black = {0, 0, 0, 255};
	if (initialColor == NULL)
		initialColor = &black;

	fText = new _BTextGapBuffer_;
	fLines = new _BLineBuffer_;
	fStyles = new _BStyleBuffer_(&font, initialColor);
}
//------------------------------------------------------------------------------
void
BTextView::HandleBackspace()
{
	CALLED();
	if (fUndo) {
		_BTypingUndoBuffer_ *undoBuffer = dynamic_cast<_BTypingUndoBuffer_ *>(fUndo);
		if (!undoBuffer) {
			delete fUndo;
			fUndo = undoBuffer = new _BTypingUndoBuffer_(this);
		}
		undoBuffer->BackwardErase();
	}
	
	if (fSelStart == fSelEnd) {
		if (fSelStart == 0)
			return;
		else 
			fSelStart = PreviousInitialByte(fSelStart);
	} else
		Highlight(fSelStart, fSelEnd);
	
	DeleteText(fSelStart, fSelEnd);
	fSelEnd = fSelStart;
	
	Refresh(fSelStart, fSelEnd, true, false);
}
//------------------------------------------------------------------------------
void
BTextView::HandleArrowKey(uint32 inArrowKey)
{
	CALLED();
	// return if there's nowhere to go
	if (fText->Length() == 0)
		return;
			
	int32 selStart = fSelStart;
	int32 selEnd = fSelEnd;
	//int32	delta = 0;
	int32 scrollToOffset = 0;
	bool shiftDown = modifiers() & B_SHIFT_KEY;

	switch (inArrowKey) {
		case B_LEFT_ARROW:
			if (shiftDown) {
				if (selStart > 0)
					selStart = PreviousInitialByte(selStart);
			} else {
				if (selStart == selEnd) {
					if (selStart > 0) 
						selEnd = selStart = PreviousInitialByte(selStart);
				} else
					selEnd = selStart;
			}
			scrollToOffset = selStart;
			break;
			
		case B_RIGHT_ARROW:
			if (shiftDown) {
				if (selEnd < fText->Length())
					selEnd = NextInitialByte(selEnd);
			} else {
				if (selStart == selEnd) {
					if (selStart < fText->Length())
						selStart = selEnd = NextInitialByte(selEnd);
				}
				else
					selStart = selEnd;
			}
			scrollToOffset = selEnd;
			break;
			
		case B_UP_ARROW:
		{
			BPoint point = PointAt(selStart);
			point.y--;
			selStart = OffsetAt(point);
			if (!shiftDown)
				selEnd = selStart;
			scrollToOffset = selStart;
			break;
		}
		
		case B_DOWN_ARROW:
		{
			float height;
			BPoint point = PointAt(selEnd, &height);
			point.y += height;
			selEnd = OffsetAt(point);
			if (!shiftDown)
				selStart = selEnd;
			scrollToOffset = selEnd;
			break;
		}
	}
	
	// invalidate the null style
	fStyles->InvalidateNullStyle();
	
	Select(selStart, selEnd);
	
	// scroll if needed
	ScrollToOffset(scrollToOffset);
}
//------------------------------------------------------------------------------
void
BTextView::HandleDelete()
{
	CALLED();
	if (fUndo) {
		_BTypingUndoBuffer_ *undoBuffer = dynamic_cast<_BTypingUndoBuffer_ *>(fUndo);
		if (!undoBuffer) {
			delete fUndo;
			fUndo = undoBuffer = new _BTypingUndoBuffer_(this);
		}
		undoBuffer->ForwardErase();
	}	
	
	if (fSelStart == fSelEnd) {
		if (fSelEnd == fText->Length())
			return;
		else 
			fSelEnd = NextInitialByte(fSelEnd);
	} else
		Highlight(fSelStart, fSelEnd);
	
	DeleteText(fSelStart, fSelEnd);
	
	fSelEnd = fSelStart;
	
	Refresh(fSelStart, fSelEnd, true, true);
}
//------------------------------------------------------------------------------
void
BTextView::HandlePageKey(uint32 inPageKey)
{
	CALLED();
	switch (inPageKey) {
		case B_HOME:
		case B_END:
			ScrollToOffset((inPageKey == B_HOME) ? 0 : fText->Length());
			break;
			
		case B_PAGE_UP: 
		case B_PAGE_DOWN:
		{
			if (ScrollBar(B_VERTICAL) != NULL) {
				float delta = Bounds().Height();
				delta = (inPageKey == B_PAGE_UP) ? -delta : delta;
				ScrollBar(B_VERTICAL)->SetValue(ScrollBar(B_VERTICAL)->Value() + delta);
			}
			break;
		}
	}
}
//------------------------------------------------------------------------------
void
BTextView::HandleAlphaKey(const char *bytes, int32 numBytes)
{
	CALLED();
	if (fUndo) {
		_BTypingUndoBuffer_ *undoBuffer = dynamic_cast<_BTypingUndoBuffer_ *>(fUndo);
		if (!undoBuffer) {
			delete fUndo;
			fUndo = undoBuffer = new _BTypingUndoBuffer_(this);
		}
		undoBuffer->InputCharacter(numBytes);
	}

	bool refresh = fSelStart != fText->Length();

	if (fSelStart != fSelEnd) {
		Highlight(fSelStart, fSelEnd);
		DeleteText(fSelStart, fSelEnd);
		refresh = true;
	}
	
/*	if (fAutoindent && numBytes == 1 && *bytes == B_ENTER)
	{
		int32 start, offset;
		start = offset = OffsetAt(LineAt(fSelStart));
		const char *text = Text();
		
		while (*(text + offset) != '\0' &&
			*(text + offset) == '\t' || *(text + offset) == ' ')
			offset++;

		if (start != offset)
			InsertText(text + start, offset - start, fSelStart, NULL);

		InsertText(bytes, numBytes, fSelStart, NULL);
		numBytes += offset - start;
	}	
	else*/
		InsertText(bytes, numBytes, fSelStart, NULL);
	
	fSelEnd = fSelStart = fSelStart + numBytes;

	Refresh(fSelStart, fSelEnd, refresh, true);
}
//------------------------------------------------------------------------------
void
BTextView::Refresh(int32 fromOffset, int32 toOffset, bool erase,
						bool scroll)
{
	CALLED();
	float saveHeight = fTextRect.Height();
	int32 fromLine = LineAt(fromOffset);
	int32 toLine = LineAt(toOffset);
	int32 saveFromLine = fromLine;
	int32 saveToLine = toLine;
	float saveLineHeight = LineHeight(fromLine);
	BRect bounds = Bounds();
	
	RecalLineBreaks(&fromLine, &toLine);

	float newHeight = fTextRect.Height();
	
	// if the line breaks have changed, force an erase
	if ( fromLine != saveFromLine || toLine != saveToLine || 
		 newHeight != saveHeight )
		erase = true;
	
	if (newHeight != saveHeight) {
		// the text area has changed
		if (newHeight < saveHeight)
			toLine = LineAt(BPoint(0.0f, saveHeight + fTextRect.top));
		else
			toLine = LineAt(BPoint(0.0f, newHeight + fTextRect.top));
	}
	
	// draw only those lines that are visible
	int32 fromVisible = LineAt(BPoint(0.0f, bounds.top));
	int32 toVisible = LineAt(BPoint(0.0f, bounds.bottom));
	fromLine = (fromVisible > fromLine) ? fromVisible : fromLine;
	toLine = (toLine > toVisible) ? toVisible : toLine;

	int32 drawOffset = fromOffset;
	if ( LineHeight(fromLine) != saveLineHeight || 
		 newHeight < saveHeight || fromLine < saveFromLine )
		drawOffset = (*fLines)[fromLine]->offset;
			
	DrawLines(fromLine, toLine, drawOffset, erase);
	
	// erase the area below the text
	BRect eraseRect = bounds;
	eraseRect.top = fTextRect.top + (*fLines)[fLines->NumLines()]->origin;
	eraseRect.bottom = fTextRect.top + saveHeight;
	if (eraseRect.bottom > eraseRect.top && eraseRect.Intersects(bounds)) {
		SetLowColor(ViewColor());
		FillRect(eraseRect, B_SOLID_LOW);
	}
	
	// update the scroll bars if the text area has changed
	if (newHeight != saveHeight)
		UpdateScrollbars();
		
	if (scroll)
		ScrollToOffset(fSelEnd);

	Flush(); ////
}
//------------------------------------------------------------------------------
void
BTextView::RecalLineBreaks(int32 *startLine, int32 *endLine)
{
	CALLED();
	// are we insane?
	*startLine = (*startLine < 0) ? 0 : *startLine;
	*endLine = (*endLine > fLines->NumLines() - 1) ? fLines->NumLines() - 1 : *endLine;
	
	int32 textLength = fText->Length();
	int32 lineIndex = (*startLine > 0) ? *startLine - 1 : 0;
	int32 recalThreshold = (*fLines)[*endLine + 1]->offset;
	float width = fTextRect.Width();
	STELinePtr curLine = (*fLines)[lineIndex];
	STELinePtr nextLine = curLine + 1;

	do {
		float ascent, descent;
		int32 fromOffset = curLine->offset;
		int32 toOffset = FindLineBreak(fromOffset, &ascent, 
										 &descent, &width);

		// we want to advance at least by one character
		if (toOffset == fromOffset && fromOffset < textLength)
			toOffset = NextInitialByte(toOffset);
		
		// set the ascent of this line
		curLine->ascent = ascent;
		
		lineIndex++;
		STELine saveLine = *nextLine;		
		if ( lineIndex > fLines->NumLines() || 
			 toOffset < nextLine->offset ) {
			// the new line comes before the old line start, add a line
			STELine newLine;
			newLine.offset = toOffset;
			newLine.origin = curLine->origin + ascent + descent;
			newLine.ascent = 0;
			fLines->InsertLine(&newLine, lineIndex);
		} else {
			// update the exising line
			nextLine->offset = toOffset;
			nextLine->origin = curLine->origin + ascent + descent;
			
			// remove any lines that start before the current line
			while ( lineIndex < fLines->NumLines() &&
					toOffset >= ((*fLines)[lineIndex] + 1)->offset )
				fLines->RemoveLines(lineIndex + 1);
			
			nextLine = (*fLines)[lineIndex];
			if (nextLine->offset == saveLine.offset) {
				if (nextLine->offset >= recalThreshold) {
					if (nextLine->origin != saveLine.origin)
						fLines->BumpOrigin(nextLine->origin - saveLine.origin, 
										  lineIndex + 1);
					break;
				}
			} else {
				if (lineIndex > 0 && lineIndex == *startLine)
					*startLine = lineIndex - 1;
			}
		}

		curLine = (*fLines)[lineIndex];
		nextLine = curLine + 1;
	} while (curLine->offset < textLength);

	// update the text rect
	float newHeight = TextHeight(0, fLines->NumLines() - 1);
	fTextRect.bottom = fTextRect.top + newHeight;

	*endLine = lineIndex - 1;
	*startLine = (*startLine > *endLine) ? *endLine : *startLine;
}
//------------------------------------------------------------------------------
int32
BTextView::FindLineBreak(int32 fromOffset, float *outAscent,
							   float *outDescent, float	*ioWidth)
{
	CALLED();
	*outAscent = 0.0;
	*outDescent = 0.0;
	
	const int32 limit = fText->Length();

	// is fromOffset at the end?
	if (fromOffset >= limit) {
		// try to return valid height info anyway			
		if (fStyles->NumRuns() > 0)
			fStyles->Iterate(fromOffset, 1, NULL, NULL, outAscent, outDescent);
		else {
			if (fStyles->IsValidNullStyle()) {
				const BFont *font = NULL;
				fStyles->GetNullStyle(&font, NULL);
				
				font_height fh;
				font->GetHeight(&fh);
				*outAscent = fh.ascent;
				*outDescent = fh.descent + fh.leading;
			}
		}
		
		return limit;
	}
	
	bool done = false;
	float ascent = 0.0;
	float descent = 0.0;
	int32 offset = fromOffset;
	int32 delta = 0;
	float deltaWidth = 0.0;
	float tabWidth = 0.0;
	float strWidth = 0.0;
	
	// do we need to wrap the text?
	if (!fWrap) {
		offset = limit - fromOffset;
		fText->FindChar('\n', fromOffset, &offset);
		offset += fromOffset;
		offset = (offset < limit) ? offset + 1 : limit;
		
		// iterate through the style runs
		int32 length = offset;
		int32 startOffset = fromOffset;
		int32 numChars;
		while ((numChars = fStyles->Iterate(startOffset, length, NULL, NULL, &ascent, &descent)) != 0) {
			*outAscent = (ascent > *outAscent) ? ascent : *outAscent;
			*outDescent = (descent > *outDescent) ? descent : *outDescent;
			
			startOffset += numChars;
			length -= numChars;
		}
		
		return offset;
	}
	
	// wrap the text
	do {
		bool foundTab = false;
		
		// find the next line break candidate
		for ( ; (offset + delta) < limit ; delta++) {
			if (CanEndLine(offset + delta))
				break;
		}
		for ( ; (offset + delta) < limit; delta++) {
			uchar theChar = (*fText)[offset + delta];
			if (!CanEndLine(offset + delta))
				break;
			
			if (theChar == '\n') {
				// found a newline, we're done!
				done = true;
				delta++;
				break;
			} else {
				// include all trailing spaces and tabs,
				// but not spaces after tabs
				if (theChar != ' ' && theChar != '\t')
					break;
				else {
					if (theChar == ' ' && foundTab)
						break;
					else {
						if (theChar == '\t')
							foundTab = true;
					}
				}
			}
		}
		delta = (delta < 1) ? 1 : delta;

		deltaWidth = StyledWidth(offset, delta, &ascent, &descent);
		strWidth += deltaWidth;

		if (!foundTab)
			tabWidth = 0.0;
		else {
			int32 tabCount = 0;
			for (int32 i = delta - 1; (*fText)[offset + i] == '\t'; i--)
				tabCount++;
				
			tabWidth = fTabWidth - fmod(strWidth, fTabWidth);
			if (tabCount > 1)
				tabWidth += ((tabCount - 1) * fTabWidth);
			strWidth += tabWidth;
		}
			
		if (strWidth >= *ioWidth) {
			// we've found where the line will wrap
			bool foundNewline = done;
			done = true;
			int32 pos = delta - 1;
			if ((*fText)[offset + pos] != ' ' &&
				(*fText)[offset + pos] != '\t' &&
				(*fText)[offset + pos] != '\n')
				break;
			
			strWidth -= (deltaWidth + tabWidth);
			
			for ( ; ((offset + pos) > offset); pos--) {
				uchar theChar = (*fText)[offset + pos];
				if (theChar != ' ' &&
					theChar != '\t' &&
					theChar != '\n')
					break;
			}

			strWidth += StyledWidth(offset, pos + 1, &ascent, &descent);
			if (strWidth >= *ioWidth)
				break;

			if (!foundNewline) {
				for ( ; (offset + delta) < limit; delta++) {
					if ((*fText)[offset + delta] != ' ' &&
						(*fText)[offset + delta] != '\t')
						break;
				}
				if ( (offset + delta) < limit && 
					 (*fText)[offset + delta] == '\n' )
					delta++;
			}
			// get the ascent and descent of the spaces/tabs
			StyledWidth(offset, delta, &ascent, &descent);
		}
		
		*outAscent = (ascent > *outAscent) ? ascent : *outAscent;
		*outDescent = (descent > *outDescent) ? descent : *outDescent;
		
		offset += delta;
		delta = 0;
	} while (offset < limit && !done);
	
	if ((offset - fromOffset) < 1) {
		// there weren't any words that fit entirely in this line
		// force a break in the middle of a word
		*outAscent = 0.0;
		*outDescent = 0.0;
		strWidth = 0.0;
		
		for (offset = fromOffset; offset < limit; offset++) {
			strWidth += StyledWidth(offset, 1, &ascent, &descent);
			
			if (strWidth >= *ioWidth)
				break;
				
			*outAscent = (ascent > *outAscent) ? ascent : *outAscent;
			*outDescent = (descent > *outDescent) ? descent : *outDescent;
		}
	}
	
	offset = (offset < limit) ? offset : limit;
	
	return offset;
}
//------------------------------------------------------------------------------
float
BTextView::StyledWidth(int32 fromOffset, int32 length, float *outAscent,
							 float *outDescent) const
{
	CALLED();
	float result = 0.0;
	float ascent = 0.0;
	float descent = 0.0;
	float maxAscent = 0.0;
	float maxDescent = 0.0;
	
	// iterate through the style runs
	const BFont *font = NULL;
	int32 numChars;
	while ((numChars = fStyles->Iterate(fromOffset, length, &font, NULL, &ascent, &descent)) != 0) {		
		maxAscent = (ascent > maxAscent) ? ascent : maxAscent;
		maxDescent = (descent > maxDescent) ? descent : maxDescent;
		
		result += font->StringWidth(fText->Text() + fromOffset, numChars);
		
		fromOffset += numChars;
		length -= numChars;
	}

	if (outAscent != NULL)
		*outAscent = maxAscent;
	if (outDescent != NULL)
		*outDescent = maxDescent;

	return result;
}
//------------------------------------------------------------------------------
float
BTextView::ActualTabWidth(float location) const
{
	CALLED();
	return 0.0f;
}
//------------------------------------------------------------------------------
void
BTextView::DoInsertText(const char *inText, int32 inLength, int32 inOffset,
							 const text_run_array *inRuns,
							 _BTextChangeResult_ *outResult)
{
	CALLED();
}
//------------------------------------------------------------------------------
void
BTextView::DoDeleteText(int32 fromOffset, int32 toOffset,
							 _BTextChangeResult_ *outResult)
{
	CALLED();
}
//------------------------------------------------------------------------------		
void
BTextView::DrawLines(int32 startLine, int32 endLine, int32 startOffset,
						  bool erase)
{
	CALLED();	
	// clip the text
	BRect clipRect = Bounds() & fTextRect;
	clipRect.InsetBy(-1, -1);
	BRegion newClip;
	newClip.Set(clipRect);
	ConstrainClippingRegion(&newClip);

	// set the low color to the view color so that 
	// drawing to a non-white background will work	
	SetLowColor(ViewColor());

	long maxLine = fLines->NumLines() - 1;
	startLine = (startLine < 0) ? 0 : startLine;
	endLine = (endLine > maxLine) ? maxLine : endLine;

	BRect eraseRect = clipRect;
	long startEraseLine = startLine;
	STELinePtr line = (*fLines)[startLine];
	if (erase && startOffset != -1) {
		// erase only to the right of startOffset
		startEraseLine++;
		long startErase = startOffset;
		if (startErase > line->offset) {
			for ( ; ((*fText)[startErase] != ' ') && ((*fText)[startErase] != '\t'); startErase--) {
				if (startErase <= line->offset)
					break;	
			}
			if (startErase > line->offset)
				startErase--;
		}
		
		eraseRect.left = PointAt(startErase).x;
		eraseRect.top = line->origin + fTextRect.top;
		eraseRect.bottom = (line + 1)->origin + fTextRect.top;
		
		FillRect(eraseRect, B_SOLID_LOW);

		eraseRect = clipRect;
	}
	for (long i = startLine; i <= endLine; i++) {
		long length = (line + 1)->offset - line->offset;
		// DrawString() chokes if you draw a newline
		if ((*fText)[(line + 1)->offset - 1] == '\n')
			length--;	
		
		MovePenTo(fTextRect.left, line->origin + line->ascent + fTextRect.top);

		if (erase && i >= startEraseLine) {
			eraseRect.top = line->origin + fTextRect.top;
			eraseRect.bottom = (line + 1)->origin + fTextRect.top;
			
			FillRect(eraseRect, B_SOLID_LOW);
		}
		
		// do we have any text to draw?
		if (length > 0) {
			// iterate through each style on this line
			//BPoint		startPenLoc;
			bool foundTab = false;
			long tabChars = 0;
			long numTabs = 0;
			long offset = line->offset;
			const BFont *font = NULL;
			const rgb_color *color = NULL;
			int32 numChars;
			while ((numChars = fStyles->Iterate(offset, length, &font, &color)) != 0) {
				SetFont(font);
				SetHighColor(*color);
				
				tabChars = numChars;
				do {
					//if (style->underline)
					//	startPenLoc = PenLocation();
					
					foundTab = fText->FindChar('\t', offset, &tabChars);
					if (foundTab) {
						for (numTabs = 0; (tabChars + numTabs) < numChars; numTabs++) {
							if ((*fText)[offset + tabChars + numTabs] != '\t')
								break;
						}
					}

					DrawString(fText->GetString(offset, tabChars), tabChars);
					
					if (foundTab) {
						float penPos = PenLocation().x - fTextRect.left;
						float tabWidth = fTabWidth - fmod(penPos, fTabWidth);
						if (numTabs > 1)
							tabWidth += ((numTabs - 1) * fTabWidth);
							
						MovePenBy(tabWidth, 0.0);

						tabChars += numTabs;
					}
					
					/*if (style->underline) {
						BPoint savePenLoc = PenLocation();
						BPoint curPenLoc = savePenLoc;
						startPenLoc.y += 1.0;
						curPenLoc.y += 1.0;

						StrokeLine(startPenLoc, curPenLoc);
						
						MovePenTo(savePenLoc);
					}*/

					offset += tabChars;
					length -= tabChars;
					numChars -= tabChars;
					tabChars = numChars;
					numTabs = 0;
				} while (foundTab && tabChars > 0);
			}
		}
		line++;
	}

	ConstrainClippingRegion(NULL);
}
//------------------------------------------------------------------------------
void
BTextView::DrawCaret(int32 offset)
{
	CALLED();
	//long 		lineNum = LineAt(offset);
	//STELinePtr	line = (*fLines)[lineNum];
	float lineHeight;
	BPoint caretPoint = PointAt(offset, &lineHeight);
	caretPoint.x = (caretPoint.x > fTextRect.right) ? fTextRect.right : caretPoint.x;
	
	BRect caretRect;
	caretRect.left = caretRect.right = caretPoint.x;
	caretRect.top = caretPoint.y;
	caretRect.bottom = caretPoint.y + lineHeight;
	
	InvertRect(caretRect);

	Flush(); ////
}
//------------------------------------------------------------------------------
void
BTextView::InvertCaret()
{
	CALLED();
	DrawCaret(fSelStart);
	fCaretVisible = !fCaretVisible;
	fCaretTime = system_time();
}
//------------------------------------------------------------------------------
void
BTextView::DragCaret(int32 offset)
{
	CALLED();
	// does the caret need to move?
	if (offset == fDragOffset)
		return;
	
	// hide the previous drag caret
	if (fDragOffset != -1)
		DrawCaret(fDragOffset);
		
	// do we have a new location?
	if (offset != -1) {
		if (fActive) {
			// ignore if offset is within active selection
			if (offset >= fSelStart && offset <= fSelEnd) {
				fDragOffset = -1;
				return;
			}
		}
		
		DrawCaret(offset);
	}
	
	fDragOffset = offset;
}
//------------------------------------------------------------------------------
void
BTextView::StopMouseTracking()
{
	CALLED();
}
//------------------------------------------------------------------------------
bool
BTextView::PerformMouseUp(BPoint where)
{
	CALLED();
	return false;
}
//------------------------------------------------------------------------------
bool BTextView::PerformMouseMoved(BPoint where, uint32 code)
{
	CALLED();
	return false;
}
//------------------------------------------------------------------------------
void
BTextView::TrackMouse(BPoint where, const BMessage *message, bool force)
{
	CALLED();
}
//------------------------------------------------------------------------------
void
BTextView::TrackDrag(BPoint where)
{
	CALLED();
}
//------------------------------------------------------------------------------
void
BTextView::InitiateDrag()
{
	CALLED();
	BMessage *drag = new BMessage();
	BBitmap *dragBitmap = NULL;
	BPoint bitmapPoint;
	BHandler *dragHandler = NULL;
	
	GetDragParameters(drag, &dragBitmap, &bitmapPoint, &dragHandler);
	
	SetViewCursor(B_CURSOR_SYSTEM_DEFAULT);
	
	if (dragBitmap != NULL)
		DragMessage(drag, dragBitmap, bitmapPoint, dragHandler);
	else {
		BRegion hiliteRgn;
		GetTextRegion(fSelStart, fSelEnd, &hiliteRgn);
		BRect bounds = Bounds();
		BRect dragRect = hiliteRgn.Frame();
		if (!bounds.Contains(dragRect))
			dragRect = bounds & dragRect;
			
		DragMessage(drag, dragRect, dragHandler);
	}
}
//------------------------------------------------------------------------------
bool
BTextView::MessageDropped(BMessage *inMessage, BPoint where, BPoint offset)
{
	CALLED();
	if (fActive)
		SetViewCursor(B_CURSOR_I_BEAM);
		
	// are we sure we like this message?
	if (!AcceptsDrop(inMessage))
		return false;
		
	long dropOffset = OffsetAt(where);
	if (dropOffset > TextLength())
		dropOffset = TextLength();
	
	void *from = NULL;
	inMessage->FindPointer("be:originator", &from);
	
	bool internalDrop = (from == this && fSelEnd != fSelStart);
	
	// if this view initiated the drag, move instead of copy
	if (internalDrop) {
		// dropping onto itself?
		if (dropOffset >= fSelStart && dropOffset <= fSelEnd)
			return true; 
	}
	
	ssize_t dataLen = 0;
	const char *text = NULL;
	if (inMessage->FindData("text/plain", B_MIME_TYPE, (const void **)&text, &dataLen) == B_OK) {
		
		text_run_array *runArray = NULL;
		ssize_t runLen = 0;
		if (fStylable)
			inMessage->FindData("application/x-vnd.Be-text_run_array", B_MIME_TYPE,
					(const void **)&runArray, &runLen);
		
		if (fUndo) {
			delete fUndo;
			fUndo = new _BDropUndoBuffer_(this, text, dataLen, runArray, runLen, dropOffset, internalDrop);
		}
		
		if (internalDrop)
			Delete();
				
		Insert(dropOffset, text, dataLen, runArray);
	}
	
	return true;
}
//------------------------------------------------------------------------------				
void
BTextView::UpdateScrollbars()
{
	CALLED();
	//BRect bounds(Bounds());
	BScrollBar *hsb = ScrollBar(B_HORIZONTAL);
 	BScrollBar *vsb = ScrollBar(B_VERTICAL);

	// do we have a horizontal scroll bar?
	if (hsb != NULL) {
		long viewWidth = Bounds().IntegerWidth();
		long dataWidth = fTextRect.IntegerWidth();
		dataWidth += (long)ceil(fTextRect.left) + 1;
		
		long maxRange = dataWidth - viewWidth;
		maxRange = (maxRange < 0) ? 0 : maxRange;
		
		hsb->SetRange(0, (float)maxRange);
		hsb->SetProportion((float)viewWidth / (float)dataWidth);
		hsb->SetSteps(10, dataWidth / 10);
	}
	
	// how about a vertical scroll bar?
	if (vsb != NULL) {
		long viewHeight = Bounds().IntegerHeight();
		long dataHeight = fTextRect.IntegerHeight();
		dataHeight += (long)ceil(fTextRect.top) + 1;
		
		long maxRange = dataHeight - viewHeight;
		maxRange = (maxRange < 0) ? 0 : maxRange;
		
		vsb->SetRange(0, maxRange);
		vsb->SetProportion((float)viewHeight / (float)dataHeight);
		vsb->SetSteps(12, viewHeight);
	}
}
//------------------------------------------------------------------------------
void
BTextView::AutoResize(bool doredraw)
{
	CALLED();
}
//------------------------------------------------------------------------------		
void
BTextView::NewOffscreen(float padding)
{
	CALLED();
	if (fOffscreen != NULL)
		DeleteOffscreen();
	/*
	BRect bitmapRect(0, 0, fTextRect.Width() + padding, fTextRect.Height());
	fOffscreen = new BBitmap(bitmapRect, fColorSpace, true, false);
	if (fOffscreen != NULL && fOffscreen->Lock()) {
		BView *bufferView = new BView(bitmapRect, "buffer view", 0, 0);
		fOffscreen->AddChild(bufferView);
		fOffscreen->Unlock();
	}*/
}
//------------------------------------------------------------------------------
void
BTextView::DeleteOffscreen()
{
	CALLED();
	if (fOffscreen != NULL && fOffscreen->Lock()) {
		delete fOffscreen;
		fOffscreen = NULL;
	}	
}
//------------------------------------------------------------------------------
void
BTextView::Activate()
{
	CALLED();
	fActive = true;
	
	if (fSelStart != fSelEnd) {
		if (fSelectable)
			Highlight(fSelStart, fSelEnd);
	} else {
		if (fEditable)
			InvertCaret();
	}
	
	BPoint where;
	ulong buttons;
	GetMouse(&where, &buttons);
	if (Bounds().Contains(where))
		SetViewCursor(B_CURSOR_I_BEAM);
}
//------------------------------------------------------------------------------
void
BTextView::Deactivate()
{
	CALLED();
	fActive = false;
	
	if (fSelStart != fSelEnd) {
		if (fSelectable)
			Highlight(fSelStart, fSelEnd);
	} else {
		if (fCaretVisible)
			InvertCaret();
	}
	
	BPoint where;
	ulong buttons;
	GetMouse(&where, &buttons);
	if (Bounds().Contains(where))
		SetViewCursor(B_CURSOR_SYSTEM_DEFAULT);
}
//------------------------------------------------------------------------------
void
BTextView::NormalizeFont(BFont *font)
{
	CALLED();
	font->SetRotation(0.0f);
	font->SetFlags(font->Flags() & ~B_DISABLE_ANTIALIASING);
	font->SetSpacing(B_BITMAP_SPACING);
	font->SetEncoding(B_UNICODE_UTF8);
}
//------------------------------------------------------------------------------
uint32
BTextView::CharClassification(int32 offset) const
{
	CALLED();
	// Should check against a list of character containing also
	// japanese word breakers
	switch (fText->RealCharAt(offset)) {
		case B_SPACE:
		case '_':
		case '.':
		case '\0':
		case B_TAB:
		case B_ENTER:
		case '&':
		case '*':
		case '+':
		case '-':
		case '/':
		case '<':
		case '=':
		case '>':
		case '\\':
		case '^':
		case '|':		
			return B_SEPARATOR_CHARACTER;
		default:
			return B_OTHER_CHARACTER;
	}
}
//------------------------------------------------------------------------------
int32
BTextView::NextInitialByte(int32 offset) const
{
	CALLED();
	const char *text = Text();

	if (text == NULL)
		return 0;

	for (++offset; (*(text + offset) & 0xc0) == 0x80; ++offset)
		;

	return offset;
}
//------------------------------------------------------------------------------
int32
BTextView::PreviousInitialByte(int32 offset) const
{
	CALLED();
	const char *text = Text();
	int count = 6;
	
	for (--offset; (text + offset) > text && count; --offset, --count) {
		if ((*(text + offset) & 0xc0 ) != 0x80)
			break;
	}

	return count ? offset : 0;
}
//------------------------------------------------------------------------------
bool
BTextView::GetProperty(BMessage *specifier, int32 form,
							const char *property, BMessage *reply)
{
	CALLED();
	if (strcmp(property, "Selection") == 0) {
		reply->what = B_REPLY;
		reply->AddInt32("result", fSelStart);
		reply->AddInt32("result", fSelEnd);
		reply->AddInt32("error", B_OK);

		return true;

	} else if (strcmp(property, "Text") == 0) {
		int32 index, range;
		char *buffer;

		specifier->FindInt32("index", &index);
		specifier->FindInt32("range", &range);

		buffer = new char[range + 1];
		GetText(index, range, buffer);

		reply->what = B_REPLY;
		reply->AddString("result", buffer);
		delete buffer;
		reply->AddInt32("error", B_OK);

		return true;

	} else if (strcmp(property, "text_run_array") == 0)
		return false;
	else
		return false;
}
//------------------------------------------------------------------------------
bool
BTextView::SetProperty(BMessage *specifier, int32 form,
							const char *property, BMessage *reply)
{
	CALLED();
	if (strcmp(property, "Selection") == 0) {
		int32 index, range;

		specifier->FindInt32("index", &index);
		specifier->FindInt32("range", &range);

		Select(index, index + range);

		reply->what = B_REPLY;
		reply->AddInt32("error", B_OK);

		return true;

	} else if (strcmp(property, "Text") == 0) {
		int32 index, range;
		const char *buffer;

		specifier->FindInt32("index", &index);
		specifier->FindInt32("range", &range);
		
		if (specifier->FindString("data", &buffer) == B_OK)
			InsertText(buffer, range, index, NULL);
		else
			DeleteText(index, range);

		reply->what = B_REPLY;
		reply->AddInt32("error", B_OK);

		return true;

	} else if (strcmp(property, "text_run_array") == 0)
		return false;

	else
		return false;
}
//------------------------------------------------------------------------------
bool
BTextView::CountProperties(BMessage *specifier, int32 form,
								const char *property, BMessage *reply)
{
	CALLED();
	if (strcmp(property, "Text") == 0) {
		reply->what = B_REPLY;
		reply->AddInt32("result", TextLength());
		reply->AddInt32("error", B_OK);
		return true;

	} else
		return false;
}
//------------------------------------------------------------------------------
void
BTextView::HandleInputMethodChanged(BMessage *message)
{
	CALLED();
}
//------------------------------------------------------------------------------
void
BTextView::HandleInputMethodLocationRequest()
{
	CALLED();
}
//------------------------------------------------------------------------------
void
BTextView::CancelInputMethod()
{
	CALLED();
}
//------------------------------------------------------------------------------
void
BTextView::LockWidthBuffer()
{
	CALLED();
	if (atomic_add(&sWidthAtom, 1) > 0)
		acquire_sem(sWidthSem);
}
//------------------------------------------------------------------------------
void
BTextView::UnlockWidthBuffer()
{
	CALLED();
	if (atomic_add(&sWidthAtom, -1) > 1)
		release_sem(sWidthSem);
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */
