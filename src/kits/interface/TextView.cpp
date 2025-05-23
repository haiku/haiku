/*
 * Copyright 2001-2020 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus, superstippi@gmx.de
 *		Stefano Ceccherini, stefano.ceccherini@gmail.com
 *		Marc Flerackers, mflerackers@androme.be
 *		Hiroshi Lockheimer (BTextView is based on his STEEngine)
 *		John Scipione, jscipione@gmail.com
 *		Oliver Tappe, zooey@hirschkaefer.de
 */


// TODOs:
// - Consider using BObjectList instead of BList
// 	 for disallowed characters (it would remove a lot of reinterpret_casts)
// - Check for correctness and possible optimizations the calls to _Refresh(),
// 	 to refresh only changed parts of text (currently we often redraw the whole
//   text)

// Known Bugs:
// - Double buffering doesn't work well (disabled by default)


#include <TextView.h>

#include <algorithm>
#include <new>

#include <stdio.h>
#include <stdlib.h>

#include <Alignment.h>
#include <Application.h>
#include <Beep.h>
#include <Bitmap.h>
#include <Clipboard.h>
#include <ControlLook.h>
#include <Debug.h>
#include <Entry.h>
#include <Input.h>
#include <LayoutBuilder.h>
#include <LayoutUtils.h>
#include <MessageRunner.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <PropertyInfo.h>
#include <Region.h>
#include <ScrollBar.h>
#include <SystemCatalog.h>
#include <Window.h>

#include <binary_compatibility/Interface.h>

#include "InlineInput.h"
#include "LineBuffer.h"
#include "StyleBuffer.h"
#include "TextGapBuffer.h"
#include "UndoBuffer.h"
#include "WidthBuffer.h"


using namespace std;
using BPrivate::gSystemCatalog;


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "TextView"


#define TRANSLATE(str) \
	gSystemCatalog.GetString(B_TRANSLATE_MARK(str), "TextView")

#undef TRACE
#undef CALLED
//#define TRACE_TEXT_VIEW
#ifdef TRACE_TEXT_VIEW
#	include <FunctionTracer.h>
	static int32 sFunctionDepth = -1;
#	define CALLED(x...)	FunctionTracer _ft("BTextView", __FUNCTION__, \
							sFunctionDepth)
#	define TRACE(x...)	{ BString _to; \
							_to.Append(' ', (sFunctionDepth + 1) * 2); \
							printf("%s", _to.String()); printf(x); }
#else
#	define CALLED(x...)
#	define TRACE(x...)
#endif


#define USE_WIDTHBUFFER 1
#define USE_DOUBLEBUFFERING 0


struct flattened_text_run {
	int32	offset;
	font_family	family;
	font_style style;
	float	size;
	float	shear;		// typically 90.0
	uint16	face;		// typically 0
	uint8	red;
	uint8	green;
	uint8	blue;
	uint8	alpha;		// 255 == opaque
	uint16	_reserved_;	// 0
};

struct flattened_text_run_array {
	uint32	magic;
	uint32	version;
	int32	count;
	flattened_text_run styles[1];
};

static const uint32 kFlattenedTextRunArrayMagic = 'Ali!';
static const uint32 kFlattenedTextRunArrayVersion = 0;


enum {
	CHAR_CLASS_DEFAULT,
	CHAR_CLASS_WHITESPACE,
	CHAR_CLASS_GRAPHICAL,
	CHAR_CLASS_QUOTE,
	CHAR_CLASS_PUNCTUATION,
	CHAR_CLASS_PARENS_OPEN,
	CHAR_CLASS_PARENS_CLOSE,
	CHAR_CLASS_END_OF_TEXT
};


class BTextView::TextTrackState {
public:
	TextTrackState(BMessenger messenger);
	~TextTrackState();

	void SimulateMouseMovement(BTextView* view);

public:
	int32				clickOffset;
	bool				shiftDown;
	BRect				selectionRect;
	BPoint				where;

	int32				anchor;
	int32				selStart;
	int32				selEnd;

private:
	BMessageRunner*		fRunner;
};


struct BTextView::LayoutData {
	LayoutData()
		: leftInset(0),
		  topInset(0),
		  rightInset(0),
		  bottomInset(0),
		  valid(false),
		  overridden(false)
	{
	}

	float				leftInset;
	float				topInset;
	float				rightInset;
	float				bottomInset;

	BSize				min;
	BSize				preferred;
	bool				valid : 1;
	bool				overridden : 1;
};


static const rgb_color kBlueInputColor = { 152, 203, 255, 255 };
static const rgb_color kRedInputColor = { 255, 152, 152, 255 };

static const float kHorizontalScrollBarStep = 10.0;
static const float kVerticalScrollBarStep = 12.0;

static const int32 kMsgNavigateArrow = '_NvA';
static const int32 kMsgNavigatePage  = '_NvP';
static const int32 kMsgRemoveWord    = '_RmW';


static property_info sPropertyList[] = {
	{
		"selection",
		{ B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 },
		"Returns the current selection.", 0,
		{ B_INT32_TYPE, 0 }
	},
	{
		"selection",
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
		"Returns the style information for the text in the specified range in "
			"the BTextView.", 0,
		{ B_RAW_TYPE, 0 },
	},
	{
		"text_run_array",
		{ B_SET_PROPERTY, 0 },
		{ B_RANGE_SPECIFIER, B_REVERSE_RANGE_SPECIFIER, 0 },
		"Sets the style information for the text in the specified range in the "
			"BTextView.", 0,
		{ B_RAW_TYPE, 0 },
	},

	{ 0 }
};


BTextView::BTextView(BRect frame, const char* name, BRect textRect,
	uint32 resizeMask, uint32 flags)
	:
	BView(frame, name, resizeMask,
		flags | B_FRAME_EVENTS | B_PULSE_NEEDED | B_INPUT_METHOD_AWARE),
	fText(NULL),
	fLines(NULL),
	fStyles(NULL),
	fDisallowedChars(NULL),
	fUndo(NULL),
	fDragRunner(NULL),
	fClickRunner(NULL),
	fLayoutData(NULL)
{
	_InitObject(textRect, NULL, NULL);
	SetViewUIColor(B_DOCUMENT_BACKGROUND_COLOR);
}


BTextView::BTextView(BRect frame, const char* name, BRect textRect,
	const BFont* initialFont, const rgb_color* initialColor,
	uint32 resizeMask, uint32 flags)
	:
	BView(frame, name, resizeMask,
		flags | B_FRAME_EVENTS | B_PULSE_NEEDED | B_INPUT_METHOD_AWARE),
	fText(NULL),
	fLines(NULL),
	fStyles(NULL),
	fDisallowedChars(NULL),
	fUndo(NULL),
	fDragRunner(NULL),
	fClickRunner(NULL),
	fLayoutData(NULL)
{
	_InitObject(textRect, initialFont, initialColor);
	SetViewUIColor(B_DOCUMENT_BACKGROUND_COLOR);
}


BTextView::BTextView(const char* name, uint32 flags)
	:
	BView(name,
		flags | B_FRAME_EVENTS | B_PULSE_NEEDED | B_INPUT_METHOD_AWARE),
	fText(NULL),
	fLines(NULL),
	fStyles(NULL),
	fDisallowedChars(NULL),
	fUndo(NULL),
	fDragRunner(NULL),
	fClickRunner(NULL),
	fLayoutData(NULL)
{
	_InitObject(Bounds(), NULL, NULL);
	SetViewUIColor(B_DOCUMENT_BACKGROUND_COLOR);
}


BTextView::BTextView(const char* name, const BFont* initialFont,
	const rgb_color* initialColor, uint32 flags)
	:
	BView(name,
		flags | B_FRAME_EVENTS | B_PULSE_NEEDED | B_INPUT_METHOD_AWARE),
	fText(NULL),
	fLines(NULL),
	fStyles(NULL),
	fDisallowedChars(NULL),
	fUndo(NULL),
	fDragRunner(NULL),
	fClickRunner(NULL),
	fLayoutData(NULL)
{
	_InitObject(Bounds(), initialFont, initialColor);
	SetViewUIColor(B_DOCUMENT_BACKGROUND_COLOR);
}


BTextView::BTextView(BMessage* archive)
	:
	BView(archive),
	fText(NULL),
	fLines(NULL),
	fStyles(NULL),
	fDisallowedChars(NULL),
	fUndo(NULL),
	fDragRunner(NULL),
	fClickRunner(NULL),
	fLayoutData(NULL)
{
	CALLED();
	BRect rect;

	if (archive->FindRect("_trect", &rect) != B_OK)
		rect.Set(0, 0, 0, 0);

	_InitObject(rect, NULL, NULL);

	bool toggle;

	if (archive->FindBool("_password", &toggle) == B_OK)
		HideTyping(toggle);

	const char* text = NULL;
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
	const int32* disallowedChars = NULL;
	if (archive->FindData("_dis_ch", B_RAW_TYPE,
		(const void**)&disallowedChars, &disallowedCount) == B_OK) {

		fDisallowedChars = new BList;
		disallowedCount /= sizeof(int32);
		for (int32 x = 0; x < disallowedCount; x++) {
			fDisallowedChars->AddItem(
				reinterpret_cast<void*>(disallowedChars[x]));
		}
	}

	ssize_t runSize = 0;
	const void* flattenedRun = NULL;

	if (archive->FindData("_runs", B_RAW_TYPE, &flattenedRun, &runSize)
			== B_OK) {
		text_run_array* runArray = UnflattenRunArray(flattenedRun,
			(int32*)&runSize);
		if (runArray) {
			SetRunArray(0, fText->Length(), runArray);
			FreeRunArray(runArray);
		}
	}
}


BTextView::~BTextView()
{
	_CancelInputMethod();
	_StopMouseTracking();
	_DeleteOffscreen();

	delete fText;
	delete fLines;
	delete fStyles;
	delete fDisallowedChars;
	delete fUndo;
	delete fDragRunner;
	delete fClickRunner;
	delete fLayoutData;
}


BArchivable*
BTextView::Instantiate(BMessage* archive)
{
	CALLED();
	if (validate_instantiation(archive, "BTextView"))
		return new BTextView(archive);
	return NULL;
}


status_t
BTextView::Archive(BMessage* data, bool deep) const
{
	CALLED();
	status_t err = BView::Archive(data, deep);
	if (err == B_OK)
		err = data->AddString("_text", Text());
	if (err == B_OK)
		err = data->AddInt32("_align", fAlignment);
	if (err == B_OK)
		err = data->AddFloat("_tab", fTabWidth);
	if (err == B_OK)
		err = data->AddInt32("_col_sp", fColorSpace);
	if (err == B_OK)
		err = data->AddRect("_trect", fTextRect);
	if (err == B_OK)
		err = data->AddInt32("_max", fMaxBytes);
	if (err == B_OK)
		err = data->AddInt32("_sel", fSelStart);
	if (err == B_OK)
		err = data->AddInt32("_sel", fSelEnd);
	if (err == B_OK)
		err = data->AddBool("_stylable", fStylable);
	if (err == B_OK)
		err = data->AddBool("_auto_in", fAutoindent);
	if (err == B_OK)
		err = data->AddBool("_wrap", fWrap);
	if (err == B_OK)
		err = data->AddBool("_nsel", !fSelectable);
	if (err == B_OK)
		err = data->AddBool("_nedit", !fEditable);
	if (err == B_OK)
		err = data->AddBool("_password", IsTypingHidden());

	if (err == B_OK && fDisallowedChars != NULL && fDisallowedChars->CountItems() > 0) {
		err = data->AddData("_dis_ch", B_RAW_TYPE, fDisallowedChars->Items(),
			fDisallowedChars->CountItems() * sizeof(int32));
	}

	if (err == B_OK) {
		int32 runSize = 0;
		text_run_array* runArray = RunArray(0, fText->Length());

		void* flattened = FlattenRunArray(runArray, &runSize);
		if (flattened != NULL) {
			data->AddData("_runs", B_RAW_TYPE, flattened, runSize);
			free(flattened);
		} else
			err = B_NO_MEMORY;

		FreeRunArray(runArray);
	}

	return err;
}


void
BTextView::AttachedToWindow()
{
	BView::AttachedToWindow();

	SetDrawingMode(B_OP_COPY);

	Window()->SetPulseRate(500000);

	fCaretVisible = false;
	fCaretTime = 0;
	fClickCount = 0;
	fClickTime = 0;
	fDragOffset = -1;
	fActive = false;

	_ValidateTextRect();

	_AutoResize(true);

	_UpdateScrollbars();

	SetViewCursor(B_CURSOR_SYSTEM_DEFAULT);
}


void
BTextView::DetachedFromWindow()
{
	BView::DetachedFromWindow();
}


void
BTextView::Draw(BRect updateRect)
{
	// what lines need to be drawn?
	int32 startLine = _LineAt(BPoint(0.0, updateRect.top));
	int32 endLine = _LineAt(BPoint(0.0, updateRect.bottom));

	_DrawLines(startLine, endLine, -1, true);
}


void
BTextView::MouseDown(BPoint where)
{
	// should we even bother?
	if (!fEditable && !fSelectable)
		return;

	_CancelInputMethod();

	if (!IsFocus())
		MakeFocus();

	_HideCaret();

	_StopMouseTracking();

	int32 modifiers = 0;
	uint32 buttons = 0;
	BMessage* currentMessage = Window()->CurrentMessage();
	if (currentMessage != NULL) {
		currentMessage->FindInt32("modifiers", &modifiers);
		currentMessage->FindInt32("buttons", (int32*)&buttons);
	}

	if (buttons == B_SECONDARY_MOUSE_BUTTON) {
		_ShowContextMenu(where);
		return;
	}

	BMessenger messenger(this);
	fTrackingMouse = new (nothrow) TextTrackState(messenger);
	if (fTrackingMouse == NULL)
		return;

	fTrackingMouse->clickOffset = OffsetAt(where);
	fTrackingMouse->shiftDown = modifiers & B_SHIFT_KEY;
	fTrackingMouse->where = where;

	bigtime_t clickTime = system_time();
	bigtime_t clickSpeed = 0;
	get_click_speed(&clickSpeed);
	bool multipleClick
		= clickTime - fClickTime < clickSpeed
			&& fLastClickOffset == fTrackingMouse->clickOffset;

	fWhere = where;

	SetMouseEventMask(B_POINTER_EVENTS | B_KEYBOARD_EVENTS,
		B_LOCK_WINDOW_FOCUS | B_NO_POINTER_HISTORY);

	if (fSelStart != fSelEnd && !fTrackingMouse->shiftDown && !multipleClick) {
		BRegion region;
		GetTextRegion(fSelStart, fSelEnd, &region);
		if (region.Contains(where)) {
			// Setup things for dragging
			fTrackingMouse->selectionRect = region.Frame();
			fClickCount = 1;
			fClickTime = clickTime;
			fLastClickOffset = OffsetAt(where);
			return;
		}
	}

	if (multipleClick) {
		if (fClickCount > 3) {
			fClickCount = 0;
			fClickTime = 0;
		} else {
			fClickCount++;
			fClickTime = clickTime;
		}
	} else if (!fTrackingMouse->shiftDown) {
		// If no multiple click yet and shift is not pressed, this is an
		// independent first click somewhere into the textview - we initialize
		// the corresponding members for handling potential multiple clicks:
		fLastClickOffset = fCaretOffset = fTrackingMouse->clickOffset;
		fClickCount = 1;
		fClickTime = clickTime;

		// Deselect any previously selected text
		Select(fTrackingMouse->clickOffset, fTrackingMouse->clickOffset);
	}

	if (fClickTime == clickTime) {
		BMessage message(_PING_);
		message.AddInt64("clickTime", clickTime);
		delete fClickRunner;

		BMessenger messenger(this);
		fClickRunner = new (nothrow) BMessageRunner(messenger, &message,
			clickSpeed, 1);
	}

	if (!fSelectable) {
		_StopMouseTracking();
		return;
	}

	int32 offset = fSelStart;
	if (fTrackingMouse->clickOffset > fSelStart)
		offset = fSelEnd;

	fTrackingMouse->anchor = offset;

	MouseMoved(where, B_INSIDE_VIEW, NULL);
}


void
BTextView::MouseUp(BPoint where)
{
	BView::MouseUp(where);
	_PerformMouseUp(where);

	delete fDragRunner;
	fDragRunner = NULL;
}


void
BTextView::MouseMoved(BPoint where, uint32 code, const BMessage* dragMessage)
{
	// check if it's a "click'n'move"
	if (_PerformMouseMoved(where, code))
		return;

	switch (code) {
		case B_ENTERED_VIEW:
		case B_INSIDE_VIEW:
			_TrackMouse(where, dragMessage, true);
			break;

		case B_EXITED_VIEW:
			_DragCaret(-1);
			if (Window()->IsActive() && dragMessage == NULL)
				SetViewCursor(B_CURSOR_SYSTEM_DEFAULT);
			break;

		default:
			BView::MouseMoved(where, code, dragMessage);
	}
}


void
BTextView::WindowActivated(bool active)
{
	BView::WindowActivated(active);

	if (active && IsFocus()) {
		if (!fActive)
			_Activate();
	} else {
		if (fActive)
			_Deactivate();
	}

	BPoint where;
	uint32 buttons;
	GetMouse(&where, &buttons, false);

	if (Bounds().Contains(where))
		_TrackMouse(where, NULL);
}


void
BTextView::KeyDown(const char* bytes, int32 numBytes)
{
	const char keyPressed = bytes[0];

	if (!fEditable) {
		// only arrow and page keys are allowed
		// (no need to hide the cursor)
		switch (keyPressed) {
			case B_LEFT_ARROW:
			case B_RIGHT_ARROW:
			case B_UP_ARROW:
			case B_DOWN_ARROW:
				_HandleArrowKey(keyPressed);
				break;

			case B_HOME:
			case B_END:
			case B_PAGE_UP:
			case B_PAGE_DOWN:
				_HandlePageKey(keyPressed);
				break;

			default:
				BView::KeyDown(bytes, numBytes);
				break;
		}

		return;
	}

	// hide the cursor and caret
	if (IsFocus())
		be_app->ObscureCursor();
	_HideCaret();

	switch (keyPressed) {
		case B_BACKSPACE:
			_HandleBackspace();
			break;

		case B_LEFT_ARROW:
		case B_RIGHT_ARROW:
		case B_UP_ARROW:
		case B_DOWN_ARROW:
			_HandleArrowKey(keyPressed);
			break;

		case B_DELETE:
			_HandleDelete();
			break;

		case B_HOME:
		case B_END:
		case B_PAGE_UP:
		case B_PAGE_DOWN:
			_HandlePageKey(keyPressed);
			break;

		case B_ESCAPE:
		case B_INSERT:
		case B_FUNCTION_KEY:
			// ignore, pass it up to superclass
			BView::KeyDown(bytes, numBytes);
			break;

		default:
			// bail out if the character is not allowed
			if (fDisallowedChars
				&& fDisallowedChars->HasItem(
					reinterpret_cast<void*>((uint32)keyPressed))) {
				beep();
				return;
			}

			_HandleAlphaKey(bytes, numBytes);
			break;
	}

	// draw the caret
	if (fSelStart == fSelEnd)
		_ShowCaret();
}


void
BTextView::Pulse()
{
	if (fActive && (fEditable || fSelectable) && fSelStart == fSelEnd) {
		if (system_time() > (fCaretTime + 500000.0))
			_InvertCaret();
	}
}


void
BTextView::FrameResized(float newWidth, float newHeight)
{
	BView::FrameResized(newWidth, newHeight);

	// frame resized in _AutoResize() instead
	if (fResizable)
		return;

	if (fWrap) {
		// recalculate line breaks
		// will update scroll bars if text rect changes
		_ResetTextRect();
	} else {
		// don't recalculate line breaks,
		// move text rect into position and redraw.

		float dataWidth = _TextWidth();
		newWidth = std::max(dataWidth, newWidth);

		// align rect
		BRect rect(fLayoutData->leftInset, fLayoutData->topInset,
			newWidth - fLayoutData->rightInset,
			newHeight - fLayoutData->bottomInset);

		rect = BLayoutUtils::AlignOnRect(rect,
			BSize(fTextRect.Width(), fTextRect.Height()),
			BAlignment(fAlignment, B_ALIGN_TOP));
		fTextRect.OffsetTo(rect.left, rect.top);

		// must invalidate whole thing because of highlighting
		Invalidate();
		_UpdateScrollbars();
	}
}


void
BTextView::MakeFocus(bool focus)
{
	BView::MakeFocus(focus);

	if (focus && Window() != NULL && Window()->IsActive()) {
		if (!fActive)
			_Activate();
	} else {
		if (fActive)
			_Deactivate();
	}
}


void
BTextView::MessageReceived(BMessage* message)
{
	// ToDo: block input if not editable (Andrew)

	// was this message dropped?
	if (message->WasDropped()) {
		BPoint dropOffset;
		BPoint dropPoint = message->DropPoint(&dropOffset);
		ConvertFromScreen(&dropPoint);
		ConvertFromScreen(&dropOffset);
		if (!_MessageDropped(message, dropPoint, dropOffset))
			BView::MessageReceived(message);

		return;
	}

	switch (message->what) {
		case B_CUT:
			if (!IsTypingHidden())
				Cut(be_clipboard);
			else
				beep();
			break;

		case B_COPY:
			if (!IsTypingHidden())
				Copy(be_clipboard);
			else
				beep();
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

		case B_INPUT_METHOD_EVENT:
		{
			int32 opcode;
			if (message->FindInt32("be:opcode", &opcode) == B_OK) {
				switch (opcode) {
					case B_INPUT_METHOD_STARTED:
					{
						BMessenger messenger;
						if (message->FindMessenger("be:reply_to", &messenger)
								== B_OK) {
							ASSERT(fInline == NULL);
							fInline = new InlineInput(messenger);
						}
						break;
					}

					case B_INPUT_METHOD_STOPPED:
						delete fInline;
						fInline = NULL;
						break;

					case B_INPUT_METHOD_CHANGED:
						if (fInline != NULL)
							_HandleInputMethodChanged(message);
						break;

					case B_INPUT_METHOD_LOCATION_REQUEST:
						if (fInline != NULL)
							_HandleInputMethodLocationRequest();
						break;

					default:
						break;
				}
			}
			break;
		}

		case B_SET_PROPERTY:
		case B_GET_PROPERTY:
		case B_COUNT_PROPERTIES:
		{
			BPropertyInfo propInfo(sPropertyList);
			BMessage specifier;
			const char* property;

			if (message->GetCurrentSpecifier(NULL, &specifier) < B_OK
				|| specifier.FindString("property", &property) < B_OK) {
				BView::MessageReceived(message);
				return;
			}

			if (propInfo.FindMatch(message, 0, &specifier, specifier.what,
					property) < B_OK) {
				BView::MessageReceived(message);
				break;
			}

			BMessage reply;
			bool handled = false;
			switch(message->what) {
				case B_GET_PROPERTY:
					handled = _GetProperty(message, &specifier, property,
						&reply);
					break;

				case B_SET_PROPERTY:
					handled = _SetProperty(message, &specifier, property,
						&reply);
					break;

				case B_COUNT_PROPERTIES:
					handled = _CountProperties(message, &specifier,
						property, &reply);
					break;

				default:
					break;
			}
			if (handled)
				message->SendReply(&reply);
			else
				BView::MessageReceived(message);
			break;
		}

		case _PING_:
		{
			if (message->HasInt64("clickTime")) {
				bigtime_t clickTime;
				message->FindInt64("clickTime", &clickTime);
				if (clickTime == fClickTime) {
					if (fSelStart != fSelEnd && fSelectable) {
						BRegion region;
						GetTextRegion(fSelStart, fSelEnd, &region);
						if (region.Contains(fWhere))
							_TrackMouse(fWhere, NULL);
					}
					delete fClickRunner;
					fClickRunner = NULL;
				}
			} else if (fTrackingMouse) {
				fTrackingMouse->SimulateMouseMovement(this);
				_PerformAutoScrolling();
			}
			break;
		}

		case _DISPOSE_DRAG_:
			if (fEditable)
				_TrackDrag(fWhere);
			break;

		case kMsgNavigateArrow:
		{
			int32 key = message->GetInt32("key", 0);
			int32 modifiers = message->GetInt32("modifiers", 0);
			_HandleArrowKey(key, modifiers);
			break;
		}

		case kMsgNavigatePage:
		{
			int32 key = message->GetInt32("key", 0);
			int32 modifiers = message->GetInt32("modifiers", 0);
			_HandlePageKey(key, modifiers);
			break;
		}

		case kMsgRemoveWord:
		{
			int32 key = message->GetInt32("key", 0);
			int32 modifiers = message->GetInt32("modifiers", 0);
			if (key == B_DELETE)
				_HandleDelete(modifiers);
			else if (key == B_BACKSPACE)
				_HandleBackspace(modifiers);
			break;
		}

		default:
			BView::MessageReceived(message);
			break;
	}
}


BHandler*
BTextView::ResolveSpecifier(BMessage* message, int32 index, BMessage* specifier,
	int32 what, const char* property)
{
	BPropertyInfo propInfo(sPropertyList);
	BHandler* target = this;

	if (propInfo.FindMatch(message, index, specifier, what, property) < B_OK) {
		target = BView::ResolveSpecifier(message, index, specifier, what,
			property);
	}

	return target;
}


status_t
BTextView::GetSupportedSuites(BMessage* data)
{
	if (data == NULL)
		return B_BAD_VALUE;

	status_t err = data->AddString("suites", "suite/vnd.Be-text-view");
	if (err != B_OK)
		return err;

	BPropertyInfo prop_info(sPropertyList);
	err = data->AddFlat("messages", &prop_info);

	if (err != B_OK)
		return err;
	return BView::GetSupportedSuites(data);
}


status_t
BTextView::Perform(perform_code code, void* _data)
{
	switch (code) {
		case PERFORM_CODE_MIN_SIZE:
			((perform_data_min_size*)_data)->return_value
				= BTextView::MinSize();
			return B_OK;
		case PERFORM_CODE_MAX_SIZE:
			((perform_data_max_size*)_data)->return_value
				= BTextView::MaxSize();
			return B_OK;
		case PERFORM_CODE_PREFERRED_SIZE:
			((perform_data_preferred_size*)_data)->return_value
				= BTextView::PreferredSize();
			return B_OK;
		case PERFORM_CODE_LAYOUT_ALIGNMENT:
			((perform_data_layout_alignment*)_data)->return_value
				= BTextView::LayoutAlignment();
			return B_OK;
		case PERFORM_CODE_HAS_HEIGHT_FOR_WIDTH:
			((perform_data_has_height_for_width*)_data)->return_value
				= BTextView::HasHeightForWidth();
			return B_OK;
		case PERFORM_CODE_GET_HEIGHT_FOR_WIDTH:
		{
			perform_data_get_height_for_width* data
				= (perform_data_get_height_for_width*)_data;
			BTextView::GetHeightForWidth(data->width, &data->min, &data->max,
				&data->preferred);
			return B_OK;
		}
		case PERFORM_CODE_SET_LAYOUT:
		{
			perform_data_set_layout* data = (perform_data_set_layout*)_data;
			BTextView::SetLayout(data->layout);
			return B_OK;
		}
		case PERFORM_CODE_LAYOUT_INVALIDATED:
		{
			perform_data_layout_invalidated* data
				= (perform_data_layout_invalidated*)_data;
			BTextView::LayoutInvalidated(data->descendants);
			return B_OK;
		}
		case PERFORM_CODE_DO_LAYOUT:
		{
			BTextView::DoLayout();
			return B_OK;
		}
	}

	return BView::Perform(code, _data);
}


void
BTextView::SetText(const char* text, const text_run_array* runs)
{
	SetText(text, text ? strlen(text) : 0, runs);
}


void
BTextView::SetText(const char* text, int32 length, const text_run_array* runs)
{
	_CancelInputMethod();

	// hide the caret/unhighlight the selection
	if (fActive) {
		if (fSelStart != fSelEnd) {
			if (fSelectable)
				Highlight(fSelStart, fSelEnd);
		} else
			_HideCaret();
	}

	// remove data from buffer
	if (fText->Length() > 0)
		DeleteText(0, fText->Length());

	if (text != NULL && length > 0)
		InsertText(text, length, 0, runs);

	// bounds are invalid, set them based on text
	if (!Bounds().IsValid()) {
		ResizeTo(LineWidth(0) - 1, LineHeight(0));
		fTextRect = Bounds();
		_ValidateTextRect();
		_UpdateInsets(fTextRect);
	}

	// recalculate line breaks and draw the text
	_Refresh(0, length);
	fCaretOffset = fSelStart = fSelEnd = 0;

	// draw the caret
	_ShowCaret();
}


void
BTextView::SetText(BFile* file, int32 offset, int32 length,
	const text_run_array* runs)
{
	CALLED();

	_CancelInputMethod();

	if (file == NULL)
		return;

	if (fText->Length() > 0)
		DeleteText(0, fText->Length());

	if (!fText->InsertText(file, offset, length, 0))
		return;

	// update the start offsets of each line below offset
	fLines->BumpOffset(length, _LineAt(offset) + 1);

	// update the style runs
	fStyles->BumpOffset(length, fStyles->OffsetToRun(offset - 1) + 1);

	if (fStylable && runs != NULL)
		SetRunArray(offset, offset + length, runs);
	else {
		// apply null-style to inserted text
		_ApplyStyleRange(offset, offset + length);
	}

	// recalculate line breaks and draw the text
	_Refresh(0, length);
	fCaretOffset = fSelStart = fSelEnd = 0;
	ScrollToOffset(fSelStart);

	// draw the caret
	_ShowCaret();
}


void
BTextView::Insert(const char* text, const text_run_array* runs)
{
	if (text != NULL)
		_DoInsertText(text, strlen(text), fSelStart, runs);
}


void
BTextView::Insert(const char* text, int32 length, const text_run_array* runs)
{
	if (text != NULL && length > 0)
		_DoInsertText(text, strnlen(text, length), fSelStart, runs);
}


void
BTextView::Insert(int32 offset, const char* text, int32 length,
	const text_run_array* runs)
{
	// pin offset at reasonable values
	if (offset < 0)
		offset = 0;
	else if (offset > fText->Length())
		offset = fText->Length();

	if (text != NULL && length > 0)
		_DoInsertText(text, strnlen(text, length), offset, runs);
}


void
BTextView::Delete()
{
	Delete(fSelStart, fSelEnd);
}


void
BTextView::Delete(int32 startOffset, int32 endOffset)
{
	CALLED();

	// pin offsets at reasonable values
	if (startOffset < 0)
		startOffset = 0;
	else if (startOffset > fText->Length())
		startOffset = fText->Length();

	if (endOffset < 0)
		endOffset = 0;
	else if (endOffset > fText->Length())
		endOffset = fText->Length();

	// anything to delete?
	if (startOffset == endOffset)
		return;

	// hide the caret/unhighlight the selection
	if (fActive) {
		if (fSelStart != fSelEnd) {
			if (fSelectable)
				Highlight(fSelStart, fSelEnd);
		} else
			_HideCaret();
	}
	// remove data from buffer
	DeleteText(startOffset, endOffset);

	// check if the caret needs to be moved
	if (fCaretOffset >= endOffset)
		fCaretOffset -= (endOffset - startOffset);
	else if (fCaretOffset >= startOffset && fCaretOffset < endOffset)
		fCaretOffset = startOffset;

	fSelEnd = fSelStart = fCaretOffset;

	// recalculate line breaks and draw what's left
	_Refresh(startOffset, endOffset, fCaretOffset);

	// draw the caret
	_ShowCaret();
}


const char*
BTextView::Text() const
{
	return fText->RealText();
}


int32
BTextView::TextLength() const
{
	return fText->Length();
}


void
BTextView::GetText(int32 offset, int32 length, char* buffer) const
{
	if (buffer != NULL)
		fText->GetString(offset, length, buffer);
}


uchar
BTextView::ByteAt(int32 offset) const
{
	if (offset < 0 || offset >= fText->Length())
		return '\0';

	return fText->RealCharAt(offset);
}


int32
BTextView::CountLines() const
{
	return fLines->NumLines();
}


int32
BTextView::CurrentLine() const
{
	return LineAt(fSelStart);
}


void
BTextView::GoToLine(int32 index)
{
	_CancelInputMethod();
	_HideCaret();
	fSelStart = fSelEnd = fCaretOffset = OffsetAt(index);
	_ShowCaret();
}


void
BTextView::Cut(BClipboard* clipboard)
{
	_CancelInputMethod();
	if (!fEditable)
		return;
	if (fUndo) {
		delete fUndo;
		fUndo = new CutUndoBuffer(this);
	}
	Copy(clipboard);
	Delete();
}


void
BTextView::Copy(BClipboard* clipboard)
{
	_CancelInputMethod();

	if (clipboard->Lock()) {
		clipboard->Clear();

		BMessage* clip = clipboard->Data();
		if (clip != NULL) {
			int32 numBytes = fSelEnd - fSelStart;
			const char* text = fText->GetString(fSelStart, &numBytes);
			clip->AddData("text/plain", B_MIME_TYPE, text, numBytes);

			int32 size;
			if (fStylable) {
				text_run_array* runArray = RunArray(fSelStart, fSelEnd, &size);
				clip->AddData("application/x-vnd.Be-text_run_array",
					B_MIME_TYPE, runArray, size);
				FreeRunArray(runArray);
			}
			clipboard->Commit();
		}
		clipboard->Unlock();
	}
}


void
BTextView::Paste(BClipboard* clipboard)
{
	CALLED();
	_CancelInputMethod();

	if (!fEditable || !clipboard->Lock())
		return;

	BMessage* clip = clipboard->Data();
	if (clip != NULL) {
		const char* text = NULL;
		ssize_t length = 0;

		if (clip->FindData("text/plain", B_MIME_TYPE,
				(const void**)&text, &length) == B_OK) {
			text_run_array* runArray = NULL;
			ssize_t runLength = 0;

			if (fStylable) {
				clip->FindData("application/x-vnd.Be-text_run_array",
					B_MIME_TYPE, (const void**)&runArray, &runLength);
			}

			_FilterDisallowedChars((char*)text, length, runArray);

			if (length < 1) {
				beep();
				clipboard->Unlock();
				return;
			}

			if (fUndo) {
				delete fUndo;
				fUndo = new PasteUndoBuffer(this, text, length, runArray,
					runLength);
			}

			if (fSelStart != fSelEnd)
				Delete();

			Insert(text, length, runArray);
			ScrollToOffset(fSelEnd);
		}
	}

	clipboard->Unlock();
}


void
BTextView::Clear()
{
	// We always check for fUndo != NULL (not only here),
	// because when fUndo is NULL, undo is deactivated.
	if (fUndo) {
		delete fUndo;
		fUndo = new ClearUndoBuffer(this);
	}

	Delete();
}


bool
BTextView::AcceptsPaste(BClipboard* clipboard)
{
	bool result = false;

	if (fEditable && clipboard && clipboard->Lock()) {
		BMessage* data = clipboard->Data();
		result = data && data->HasData("text/plain", B_MIME_TYPE);
		clipboard->Unlock();
	}

	return result;
}


bool
BTextView::AcceptsDrop(const BMessage* message)
{
	return fEditable && message
		&& message->HasData("text/plain", B_MIME_TYPE);
}


void
BTextView::Select(int32 startOffset, int32 endOffset)
{
	CALLED();
	if (!fSelectable)
		return;

	_CancelInputMethod();

	// pin offsets at reasonable values
	if (startOffset < 0)
		startOffset = 0;
	else if (startOffset > fText->Length())
		startOffset = fText->Length();
	if (endOffset < 0)
		endOffset = 0;
	else if (endOffset > fText->Length())
		endOffset = fText->Length();

	// a negative selection?
	if (startOffset > endOffset)
		return;

	// is the new selection any different from the current selection?
	if (startOffset == fSelStart && endOffset == fSelEnd)
		return;

	fStyles->InvalidateNullStyle();

	_HideCaret();

	if (startOffset == endOffset) {
		if (fSelStart != fSelEnd) {
			// unhilite the selection
			if (fActive)
				Highlight(fSelStart, fSelEnd);
		}
		fSelStart = fSelEnd = fCaretOffset = startOffset;
		_ShowCaret();
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


void
BTextView::SelectAll()
{
	Select(0, fText->Length());
}


void
BTextView::GetSelection(int32* _start, int32* _end) const
{
	int32 start = 0;
	int32 end = 0;

	if (fSelectable) {
		start = fSelStart;
		end = fSelEnd;
	}

	if (_start)
		*_start = start;

	if (_end)
		*_end = end;
}


void
BTextView::AdoptSystemColors()
{
	if (IsEditable())
		SetViewUIColor(B_DOCUMENT_BACKGROUND_COLOR);
	else
		SetViewUIColor(B_DOCUMENT_BACKGROUND_COLOR, _UneditableTint());

	SetLowUIColor(ViewUIColor());
	SetHighUIColor(B_DOCUMENT_TEXT_COLOR);
}


bool
BTextView::HasSystemColors() const
{
	float tint = B_NO_TINT;
	float uneditableTint = _UneditableTint();

	return ViewUIColor(&tint) == B_DOCUMENT_BACKGROUND_COLOR
		&& (tint == B_NO_TINT || tint == uneditableTint)
		&& LowUIColor(&tint) == B_DOCUMENT_BACKGROUND_COLOR
		&& (tint == B_NO_TINT || tint == uneditableTint)
		&& HighUIColor(&tint) == B_DOCUMENT_TEXT_COLOR && tint == B_NO_TINT;
}


void
BTextView::SetFontAndColor(const BFont* font, uint32 mode, const rgb_color* color)
{
	SetFontAndColor(fSelStart, fSelEnd, font, mode, color);
}


void
BTextView::SetFontAndColor(int32 startOffset, int32 endOffset,
	const BFont* font, uint32 mode, const rgb_color* color)
{
	CALLED();

	_HideCaret();

	const int32 textLength = fText->Length();

	if (!fStylable) {
		// When the text view is not stylable, we always set the whole text's
		// style and ignore the offsets
		startOffset = 0;
		endOffset = textLength;
	} else {
		// pin offsets at reasonable values
		if (startOffset < 0)
			startOffset = 0;
		else if (startOffset > textLength)
			startOffset = textLength;

		if (endOffset < 0)
			endOffset = 0;
		else if (endOffset > textLength)
			endOffset = textLength;
	}

	// apply the style to the style buffer
	fStyles->InvalidateNullStyle();
	_ApplyStyleRange(startOffset, endOffset, mode, font, color);

	if ((mode & (B_FONT_FAMILY_AND_STYLE | B_FONT_SIZE)) != 0) {
		// ToDo: maybe only invalidate the layout (depending on
		// B_SUPPORTS_LAYOUT) and have it _Refresh() automatically?
		InvalidateLayout();
		// recalc the line breaks and redraw with new style
		_Refresh(startOffset, endOffset);
	} else {
		// the line breaks wont change, simply redraw
		_RequestDrawLines(_LineAt(startOffset), _LineAt(endOffset));
	}

	_ShowCaret();
}


void
BTextView::GetFontAndColor(int32 offset, BFont* _font,
	rgb_color* _color) const
{
	fStyles->GetStyle(offset, _font, _color);
}


void
BTextView::GetFontAndColor(BFont* _font, uint32* _mode,
	rgb_color* _color, bool* _sameColor) const
{
	fStyles->ContinuousGetStyle(_font, _mode, _color, _sameColor,
		fSelStart, fSelEnd);
}


void
BTextView::SetRunArray(int32 startOffset, int32 endOffset,
	const text_run_array* runs)
{
	CALLED();

	_CancelInputMethod();

	text_run_array oneRun;

	if (!fStylable) {
		// when the text view is not stylable, we always set the whole text's
		// style with the first run and ignore the offsets
		if (runs->count == 0)
			return;

		startOffset = 0;
		endOffset = fText->Length();
		oneRun.count = 1;
		oneRun.runs[0] = runs->runs[0];
		oneRun.runs[0].offset = 0;
		runs = &oneRun;
	} else {
		// pin offsets at reasonable values
		if (startOffset < 0)
			startOffset = 0;
		else if (startOffset > fText->Length())
			startOffset = fText->Length();

		if (endOffset < 0)
			endOffset = 0;
		else if (endOffset > fText->Length())
			endOffset = fText->Length();
	}

	_SetRunArray(startOffset, endOffset, runs);

	_Refresh(startOffset, endOffset);
}


text_run_array*
BTextView::RunArray(int32 startOffset, int32 endOffset, int32* _size) const
{
	// pin offsets at reasonable values
	if (startOffset < 0)
		startOffset = 0;
	else if (startOffset > fText->Length())
		startOffset = fText->Length();

	if (endOffset < 0)
		endOffset = 0;
	else if (endOffset > fText->Length())
		endOffset = fText->Length();

	STEStyleRange* styleRange
		= fStyles->GetStyleRange(startOffset, endOffset - 1);
	if (styleRange == NULL)
		return NULL;

	text_run_array* runArray = AllocRunArray(styleRange->count, _size);
	if (runArray != NULL) {
		for (int32 i = 0; i < runArray->count; i++) {
			runArray->runs[i].offset = styleRange->runs[i].offset;
			runArray->runs[i].font = styleRange->runs[i].style.font;
			runArray->runs[i].color = styleRange->runs[i].style.color;
		}
	}

	free(styleRange);

	return runArray;
}


int32
BTextView::LineAt(int32 offset) const
{
	// pin offset at reasonable values
	if (offset < 0)
		offset = 0;
	else if (offset > fText->Length())
		offset = fText->Length();

	int32 lineNum = _LineAt(offset);
	if (_IsOnEmptyLastLine(offset))
		lineNum++;
	return lineNum;
}


int32
BTextView::LineAt(BPoint point) const
{
	int32 lineNum = _LineAt(point);
	if ((*fLines)[lineNum + 1]->origin <= point.y - fTextRect.top)
		lineNum++;

	return lineNum;
}


BPoint
BTextView::PointAt(int32 offset, float* _height) const
{
	// pin offset at reasonable values
	if (offset < 0)
		offset = 0;
	else if (offset > fText->Length())
		offset = fText->Length();

	// ToDo: Cleanup.
	int32 lineNum = _LineAt(offset);
	STELine* line = (*fLines)[lineNum];
	float height = 0;

	BPoint result;
	result.x = 0.0;
	result.y = line->origin + fTextRect.top;

	bool onEmptyLastLine = _IsOnEmptyLastLine(offset);

	if (fStyles->NumRuns() == 0) {
		// Handle the case where there is only one line (no text inserted)
		fStyles->SyncNullStyle(0);
		height = _NullStyleHeight();
	} else {
		height = (line + 1)->origin - line->origin;

		if (onEmptyLastLine) {
			// special case: go down one line if offset is at the newline
			// at the end of the buffer ...
			result.y += height;
			// ... and return the height of that (empty) line
			fStyles->SyncNullStyle(offset);
			height = _NullStyleHeight();
		} else {
			int32 length = offset - line->offset;
			result.x += _TabExpandedStyledWidth(line->offset, length);
		}
	}

	if (fAlignment != B_ALIGN_LEFT) {
		float lineWidth = onEmptyLastLine ? 0.0 : LineWidth(lineNum);
		float alignmentOffset = fTextRect.Width() + 1 - lineWidth;
		if (fAlignment == B_ALIGN_CENTER)
			alignmentOffset = floorf(alignmentOffset / 2);
		result.x += alignmentOffset;
	}

	// convert from text rect coordinates
	result.x += fTextRect.left;

	// round up
	result.x = lroundf(result.x);
	result.y = lroundf(result.y);
	if (_height != NULL)
		*_height = height;

	return result;
}


int32
BTextView::OffsetAt(BPoint point) const
{
	const int32 textLength = fText->Length();

	// should we even bother?
	if (point.y >= fTextRect.bottom)
		return textLength;
	else if (point.y < fTextRect.top)
		return 0;

	int32 lineNum = _LineAt(point);
	STELine* line = (*fLines)[lineNum];

#define COMPILE_PROBABLY_BAD_CODE 1

#if COMPILE_PROBABLY_BAD_CODE
	// special case: if point is within the text rect and PixelToLine()
	// tells us that it's on the last line, but if point is actually
	// lower than the bottom of the last line, return the last offset
	// (can happen for newlines)
	if (lineNum == (fLines->NumLines() - 1)) {
		if (point.y >= ((line + 1)->origin + fTextRect.top))
			return textLength;
	}
#endif

	// convert to text rect coordinates
	if (fAlignment != B_ALIGN_LEFT) {
		float alignmentOffset = fTextRect.Width() + 1 - LineWidth(lineNum);
		if (fAlignment == B_ALIGN_CENTER)
			alignmentOffset = floorf(alignmentOffset / 2);
		point.x -= alignmentOffset;
	}

	point.x -= fTextRect.left;
	point.x = std::max(point.x, 0.0f);

	// ToDo: The following code isn't very efficient, because it always starts
	// from the left end, so when the point is near the right end it's very
	// slow.
	int32 offset = line->offset;
	const int32 limit = (line + 1)->offset;
	float location = 0;
	do {
		const int32 nextInitial = _NextInitialByte(offset);
		const int32 saveOffset = offset;
		float width = 0;
		if (ByteAt(offset) == B_TAB)
			width = _ActualTabWidth(location);
		else
			width = _StyledWidth(saveOffset, nextInitial - saveOffset);
		if (location + width > point.x) {
			if (fabs(location + width - point.x) < fabs(location - point.x))
				offset = nextInitial;
			break;
		}

		location += width;
		offset = nextInitial;
	} while (offset < limit);

	if (offset == (line + 1)->offset) {
		// special case: newlines aren't visible
		// return the offset of the character preceding the newline
		if (ByteAt(offset - 1) == B_ENTER)
			return --offset;

		// special case: return the offset preceding any spaces that
		// aren't at the end of the buffer
		if (offset != textLength && ByteAt(offset - 1) == B_SPACE)
			return --offset;
	}

	return offset;
}


int32
BTextView::OffsetAt(int32 line) const
{
	if (line < 0)
		return 0;

	if (line > fLines->NumLines())
		return fText->Length();

	return (*fLines)[line]->offset;
}


void
BTextView::FindWord(int32 offset, int32* _fromOffset, int32* _toOffset)
{
	if (offset < 0) {
		if (_fromOffset)
			*_fromOffset = 0;

		if (_toOffset)
			*_toOffset = 0;

		return;
	}

	if (offset > fText->Length()) {
		if (_fromOffset)
			*_fromOffset = fText->Length();

		if (_toOffset)
			*_toOffset = fText->Length();

		return;
	}

	if (_fromOffset)
		*_fromOffset = _PreviousWordBoundary(offset);

	if (_toOffset)
		*_toOffset = _NextWordBoundary(offset);
}


bool
BTextView::CanEndLine(int32 offset)
{
	if (offset < 0 || offset > fText->Length())
		return false;

	// TODO: This should be improved using the LocaleKit.
	uint32 classification = _CharClassification(offset);

	// wrapping is always allowed at end of text and at newlines
	if (classification == CHAR_CLASS_END_OF_TEXT || ByteAt(offset) == B_ENTER)
		return true;

	uint32 nextClassification = _CharClassification(offset + 1);
	if (nextClassification == CHAR_CLASS_END_OF_TEXT)
		return true;

	// never separate a punctuation char from its preceeding word
	if (classification == CHAR_CLASS_DEFAULT
		&& nextClassification == CHAR_CLASS_PUNCTUATION) {
		return false;
	}

	if ((classification == CHAR_CLASS_WHITESPACE
			&& nextClassification != CHAR_CLASS_WHITESPACE)
		|| (classification != CHAR_CLASS_WHITESPACE
			&& nextClassification == CHAR_CLASS_WHITESPACE)) {
		return true;
	}

	// allow wrapping after whitespace, unless more whitespace (except for
	// newline) follows
	if (classification == CHAR_CLASS_WHITESPACE
		&& (nextClassification != CHAR_CLASS_WHITESPACE
			|| ByteAt(offset + 1) == B_ENTER)) {
		return true;
	}

	// allow wrapping after punctuation chars, unless more punctuation, closing
	// parens or quotes follow
	if (classification == CHAR_CLASS_PUNCTUATION
		&& nextClassification != CHAR_CLASS_PUNCTUATION
		&& nextClassification != CHAR_CLASS_PARENS_CLOSE
		&& nextClassification != CHAR_CLASS_QUOTE) {
		return true;
	}

	// allow wrapping after quotes, graphical chars and closing parens only if
	// whitespace follows (not perfect, but seems to do the right thing most
	// of the time)
	if ((classification == CHAR_CLASS_QUOTE
			|| classification == CHAR_CLASS_GRAPHICAL
			|| classification == CHAR_CLASS_PARENS_CLOSE)
		&& nextClassification == CHAR_CLASS_WHITESPACE) {
		return true;
	}

	return false;
}


float
BTextView::LineWidth(int32 lineNumber) const
{
	if (lineNumber < 0 || lineNumber >= fLines->NumLines())
		return 0;

	STELine* line = (*fLines)[lineNumber];
	int32 length = (line + 1)->offset - line->offset;

	// skip newline at the end of the line, if any, as it does no contribute
	// to the width
	if (ByteAt((line + 1)->offset - 1) == B_ENTER)
		length--;

	return _TabExpandedStyledWidth(line->offset, length);
}


float
BTextView::LineHeight(int32 lineNumber) const
{
	float lineHeight = TextHeight(lineNumber, lineNumber);
	if (lineHeight == 0.0) {
		// We probably don't have text content yet. Take the initial
		// style's font height or fall back to the plain font.
		const BFont* font;
		fStyles->GetNullStyle(&font, NULL);
		if (font == NULL)
			font = be_plain_font;

		font_height fontHeight;
		font->GetHeight(&fontHeight);
		// This is how the height is calculated in _RecalculateLineBreaks().
		lineHeight = ceilf(fontHeight.ascent + fontHeight.descent) + 1;
	}

	return lineHeight;
}


float
BTextView::TextHeight(int32 startLine, int32 endLine) const
{
	const int32 numLines = fLines->NumLines();
	if (startLine < 0)
		startLine = 0;
	else if (startLine > numLines - 1)
		startLine = numLines - 1;

	if (endLine < 0)
		endLine = 0;
	else if (endLine > numLines - 1)
		endLine = numLines - 1;

	float height = (*fLines)[endLine + 1]->origin
		- (*fLines)[startLine]->origin;

	if (startLine != endLine && endLine == numLines - 1
		&& fText->RealCharAt(fText->Length() - 1) == B_ENTER) {
		height += (*fLines)[endLine + 1]->origin - (*fLines)[endLine]->origin;
	}

	return ceilf(height);
}


void
BTextView::GetTextRegion(int32 startOffset, int32 endOffset,
	BRegion* outRegion) const
{
	if (!outRegion)
		return;

	outRegion->MakeEmpty();

	// pin offsets at reasonable values
	if (startOffset < 0)
		startOffset = 0;
	else if (startOffset > fText->Length())
		startOffset = fText->Length();
	if (endOffset < 0)
		endOffset = 0;
	else if (endOffset > fText->Length())
		endOffset = fText->Length();

	// return an empty region if the range is invalid
	if (startOffset >= endOffset)
		return;

	float startLineHeight = 0.0;
	float endLineHeight = 0.0;
	BPoint startPt = PointAt(startOffset, &startLineHeight);
	BPoint endPt = PointAt(endOffset, &endLineHeight);

	startLineHeight = ceilf(startLineHeight);
	endLineHeight = ceilf(endLineHeight);

	BRect selRect;
	const BRect bounds(Bounds());

	if (startPt.y == endPt.y) {
		// this is a one-line region
		selRect.left = startPt.x;
		selRect.top = startPt.y;
		selRect.right = endPt.x - 1;
		selRect.bottom = endPt.y + endLineHeight - 1;
		outRegion->Include(selRect);
	} else {
		// more than one line in the specified offset range

		// include first line from start of selection to end of window
		selRect.left = startPt.x;
		selRect.top = startPt.y;
		selRect.right = std::max(fTextRect.right,
			bounds.right - fLayoutData->rightInset);
		selRect.bottom = startPt.y + startLineHeight - 1;
		outRegion->Include(selRect);

		if (startPt.y + startLineHeight < endPt.y) {
			// more than two lines in the range
			// include middle lines from start to end of window
			selRect.left = std::min(fTextRect.left,
				bounds.left + fLayoutData->leftInset);
			selRect.top = startPt.y + startLineHeight;
			selRect.right = std::max(fTextRect.right,
				bounds.right - fLayoutData->rightInset);
			selRect.bottom = endPt.y - 1;
			outRegion->Include(selRect);
		}

		// include last line start of window to end of selection
		selRect.left = std::min(fTextRect.left,
			bounds.left + fLayoutData->leftInset);
		selRect.top = endPt.y;
		selRect.right = endPt.x - 1;
		selRect.bottom = endPt.y + endLineHeight - 1;
		outRegion->Include(selRect);
	}
}


void
BTextView::ScrollToOffset(int32 offset)
{
	BRect bounds = Bounds();
	float lineHeight = 0.0;
	BPoint point = PointAt(offset, &lineHeight);
	BPoint scrollBy(B_ORIGIN);

	// horizontal
	if (point.x < bounds.left)
		scrollBy.x = point.x - bounds.right;
	else if (point.x > bounds.right)
		scrollBy.x = point.x - bounds.left;

	// prevent from scrolling out of view
	if (scrollBy.x != 0.0) {
		float rightMax = fTextRect.right + fLayoutData->rightInset;
		if (bounds.right + scrollBy.x > rightMax)
			scrollBy.x = rightMax - bounds.right;
		float leftMin = fTextRect.left - fLayoutData->leftInset;
		if (bounds.left + scrollBy.x < leftMin)
			scrollBy.x = leftMin - bounds.left;
	}

	// vertical
	if (CountLines() > 1) {
		// scroll in Y only if multiple lines!
		if (point.y < bounds.top - fLayoutData->topInset)
			scrollBy.y = point.y - bounds.top - fLayoutData->topInset;
		else if (point.y + lineHeight > bounds.bottom
				+ fLayoutData->bottomInset) {
			scrollBy.y = point.y + lineHeight - bounds.bottom
				+ fLayoutData->bottomInset;
		}
	}

	ScrollBy(scrollBy.x, scrollBy.y);

	// Update text rect position and scroll bars
	if (CountLines() > 1 && !fWrap)
		FrameResized(Bounds().Width(), Bounds().Height());
}


void
BTextView::ScrollToSelection()
{
	ScrollToOffset(fSelStart);
}


void
BTextView::Highlight(int32 startOffset, int32 endOffset)
{
	// pin offsets at reasonable values
	if (startOffset < 0)
		startOffset = 0;
	else if (startOffset > fText->Length())
		startOffset = fText->Length();
	if (endOffset < 0)
		endOffset = 0;
	else if (endOffset > fText->Length())
		endOffset = fText->Length();

	if (startOffset >= endOffset)
		return;

	BRegion selRegion;
	GetTextRegion(startOffset, endOffset, &selRegion);

	SetDrawingMode(B_OP_INVERT);
	FillRegion(&selRegion, B_SOLID_HIGH);
	SetDrawingMode(B_OP_COPY);
}


// #pragma mark - Configuration methods


void
BTextView::SetTextRect(BRect rect)
{
	if (rect == fTextRect)
		return;

	if (!fWrap) {
		rect.right = Bounds().right;
		rect.bottom = Bounds().bottom;
	}

	_UpdateInsets(rect);

	fTextRect = rect;

	_ResetTextRect();
}


BRect
BTextView::TextRect() const
{
	return fTextRect;
}


void
BTextView::_ResetTextRect()
{
	BRect oldTextRect(fTextRect);
	// reset text rect to bounds minus insets ...
	fTextRect = Bounds().OffsetToCopy(B_ORIGIN);
	fTextRect.left += fLayoutData->leftInset;
	fTextRect.top += fLayoutData->topInset;
	fTextRect.right -= fLayoutData->rightInset;
	fTextRect.bottom -= fLayoutData->bottomInset;

	// and rewrap (potentially adjusting the right and the bottom of the text
	// rect)
	_Refresh(0, fText->Length());

	// Make sure that the dirty area outside the text is redrawn too.
	BRegion invalid(oldTextRect | fTextRect);
	invalid.Exclude(fTextRect);
	Invalidate(&invalid);
}


void
BTextView::SetInsets(float left, float top, float right, float bottom)
{
	if (fLayoutData->leftInset == left
		&& fLayoutData->topInset == top
		&& fLayoutData->rightInset == right
		&& fLayoutData->bottomInset == bottom)
		return;

	fLayoutData->leftInset = left;
	fLayoutData->topInset = top;
	fLayoutData->rightInset = right;
	fLayoutData->bottomInset = bottom;

	fLayoutData->overridden = true;

	InvalidateLayout();
	Invalidate();
}


void
BTextView::GetInsets(float* _left, float* _top, float* _right,
	float* _bottom) const
{
	if (_left)
		*_left = fLayoutData->leftInset;
	if (_top)
		*_top = fLayoutData->topInset;
	if (_right)
		*_right = fLayoutData->rightInset;
	if (_bottom)
		*_bottom = fLayoutData->bottomInset;
}


void
BTextView::SetStylable(bool stylable)
{
	fStylable = stylable;
}


bool
BTextView::IsStylable() const
{
	return fStylable;
}


void
BTextView::SetTabWidth(float width)
{
	if (width == fTabWidth)
		return;

	fTabWidth = width;

	if (Window() != NULL)
		_Refresh(0, fText->Length());
}


float
BTextView::TabWidth() const
{
	return fTabWidth;
}


void
BTextView::MakeSelectable(bool selectable)
{
	if (selectable == fSelectable)
		return;

	fSelectable = selectable;

	if (fActive && fSelStart != fSelEnd && Window() != NULL)
		Highlight(fSelStart, fSelEnd);
}


bool
BTextView::IsSelectable() const
{
	return fSelectable;
}


void
BTextView::MakeEditable(bool editable)
{
	if (editable == fEditable)
		return;

	fEditable = editable;

	// apply uneditable colors or unapply them
	if (HasSystemColors())
		AdoptSystemColors();

	// TextControls change the color of the text when
	// they are made editable, so we need to invalidate
	// the NULL style here
	// TODO: it works well, but it could be caused by a bug somewhere else
	if (fEditable)
		fStyles->InvalidateNullStyle();
	if (Window() != NULL && fActive) {
		if (!fEditable) {
			if (!fSelectable)
				_HideCaret();
			_CancelInputMethod();
		}
	}
}


bool
BTextView::IsEditable() const
{
	return fEditable;
}


void
BTextView::SetWordWrap(bool wrap)
{
	if (wrap == fWrap)
		return;

	bool updateOnScreen = fActive && Window() != NULL;
	if (updateOnScreen) {
		// hide the caret, unhilite the selection
		if (fSelStart != fSelEnd) {
			if (fSelectable)
				Highlight(fSelStart, fSelEnd);
		} else
			_HideCaret();
	}

	BRect savedBounds = Bounds();

	fWrap = wrap;
	if (wrap)
		_ResetTextRect(); // calls _Refresh
	else
		_Refresh(0, fText->Length());

	if (fEditable || fSelectable)
		ScrollToOffset(fCaretOffset);

	// redraw text rect and update scroll bars if bounds have changed
	if (Bounds() != savedBounds)
		FrameResized(Bounds().Width(), Bounds().Height());

	if (updateOnScreen) {
		// show the caret, hilite the selection
		if (fSelStart != fSelEnd) {
			if (fSelectable)
				Highlight(fSelStart, fSelEnd);
		} else
			_ShowCaret();
	}
}


bool
BTextView::DoesWordWrap() const
{
	return fWrap;
}


void
BTextView::SetMaxBytes(int32 max)
{
	const int32 textLength = fText->Length();
	fMaxBytes = max;

	if (fMaxBytes < textLength) {
		int32 offset = fMaxBytes;
		// Delete the text after fMaxBytes, but
		// respect multibyte characters boundaries.
		const int32 previousInitial = _PreviousInitialByte(offset);
		if (_NextInitialByte(previousInitial) != offset)
			offset = previousInitial;

		Delete(offset, textLength);
	}
}


int32
BTextView::MaxBytes() const
{
	return fMaxBytes;
}


void
BTextView::DisallowChar(uint32 character)
{
	if (fDisallowedChars == NULL)
		fDisallowedChars = new BList;
	if (!fDisallowedChars->HasItem(reinterpret_cast<void*>(character)))
		fDisallowedChars->AddItem(reinterpret_cast<void*>(character));
}


void
BTextView::AllowChar(uint32 character)
{
	if (fDisallowedChars != NULL)
		fDisallowedChars->RemoveItem(reinterpret_cast<void*>(character));
}


void
BTextView::SetAlignment(alignment align)
{
	// Do a reality check
	if (fAlignment != align &&
			(align == B_ALIGN_LEFT ||
			 align == B_ALIGN_RIGHT ||
			 align == B_ALIGN_CENTER)) {
		fAlignment = align;

		// After setting new alignment, update the view/window
		if (Window() != NULL) {
			FrameResized(Bounds().Width(), Bounds().Height());
				// text rect position and scroll bars may change
			Invalidate();
		}
	}
}


alignment
BTextView::Alignment() const
{
	return fAlignment;
}


void
BTextView::SetAutoindent(bool state)
{
	fAutoindent = state;
}


bool
BTextView::DoesAutoindent() const
{
	return fAutoindent;
}


void
BTextView::SetColorSpace(color_space colors)
{
	if (colors != fColorSpace && fOffscreen) {
		fColorSpace = colors;
		_DeleteOffscreen();
		_NewOffscreen();
	}
}


color_space
BTextView::ColorSpace() const
{
	return fColorSpace;
}


void
BTextView::MakeResizable(bool resize, BView* resizeView)
{
	if (resize) {
		fResizable = true;
		fContainerView = resizeView;

		// Wrapping mode and resizable mode can't live together
		if (fWrap) {
			fWrap = false;

			if (fActive && Window() != NULL) {
				if (fSelStart != fSelEnd) {
					if (fSelectable)
						Highlight(fSelStart, fSelEnd);
				} else
					_HideCaret();
			}
		}
		// We need to reset the right inset, as otherwise the auto-resize would
		// get confused about just how wide the textview needs to be.
		// This seems to be an artifact of how Tracker creates the textview
		// during a rename action.
		fLayoutData->rightInset = fLayoutData->leftInset;
	} else {
		fResizable = false;
		fContainerView = NULL;
		if (fOffscreen)
			_DeleteOffscreen();
		_NewOffscreen();
	}

	_Refresh(0, fText->Length());
}


bool
BTextView::IsResizable() const
{
	return fResizable;
}


void
BTextView::SetDoesUndo(bool undo)
{
	if (undo && fUndo == NULL)
		fUndo = new UndoBuffer(this, B_UNDO_UNAVAILABLE);
	else if (!undo && fUndo != NULL) {
		delete fUndo;
		fUndo = NULL;
	}
}


bool
BTextView::DoesUndo() const
{
	return fUndo != NULL;
}


void
BTextView::HideTyping(bool enabled)
{
	if (enabled)
		Delete(0, fText->Length());

	fText->SetPasswordMode(enabled);
}


bool
BTextView::IsTypingHidden() const
{
	return fText->PasswordMode();
}


// #pragma mark - Size methods


void
BTextView::ResizeToPreferred()
{
	BView::ResizeToPreferred();
}


void
BTextView::GetPreferredSize(float* _width, float* _height)
{
	CALLED();

	_ValidateLayoutData();

	if (_width) {
		float width = Bounds().Width();
		if (width < fLayoutData->min.width
			|| (Flags() & B_SUPPORTS_LAYOUT) != 0) {
			width = fLayoutData->min.width;
		}
		*_width = width;
	}

	if (_height) {
		float height = Bounds().Height();
		if (height < fLayoutData->min.height
			|| (Flags() & B_SUPPORTS_LAYOUT) != 0) {
			height = fLayoutData->min.height;
		}
		*_height = height;
	}
}


BSize
BTextView::MinSize()
{
	CALLED();

	_ValidateLayoutData();
	return BLayoutUtils::ComposeSize(ExplicitMinSize(), fLayoutData->min);
}


BSize
BTextView::MaxSize()
{
	CALLED();

	return BLayoutUtils::ComposeSize(ExplicitMaxSize(),
		BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));
}


BSize
BTextView::PreferredSize()
{
	CALLED();

	_ValidateLayoutData();
	return BLayoutUtils::ComposeSize(ExplicitPreferredSize(),
		fLayoutData->preferred);
}


bool
BTextView::HasHeightForWidth()
{
	if (IsEditable())
		return BView::HasHeightForWidth();

	// When not editable, we assume that all text is supposed to be visible.
	return true;
}


void
BTextView::GetHeightForWidth(float width, float* min, float* max,
	float* preferred)
{
	if (IsEditable()) {
		BView::GetHeightForWidth(width, min, max, preferred);
		return;
	}

	BRect saveTextRect = fTextRect;

	fTextRect.right = fTextRect.left + width;

	// If specific insets were set, reduce the width accordingly (this may result in more
	// linebreaks being inserted)
	if (fLayoutData->overridden) {
		fTextRect.left += fLayoutData->leftInset;
		fTextRect.right -= fLayoutData->rightInset;
	}

	int32 fromLine = _LineAt(0);
	int32 toLine = _LineAt(fText->Length());
	_RecalculateLineBreaks(&fromLine, &toLine);

	// If specific insets were set, add the top and bottom margins to the returned preferred height
	if (fLayoutData->overridden) {
		fTextRect.top -= fLayoutData->topInset;
		fTextRect.bottom += fLayoutData->bottomInset;
	}

	if (min != NULL)
		*min = fTextRect.Height();
	if (max != NULL)
		*max = B_SIZE_UNLIMITED;
	if (preferred != NULL)
		*preferred = fTextRect.Height();

	// Restore the text rect since we were not supposed to change it in this method.
	// Unfortunately, we did change a few other things by calling _RecalculateLineBreaks, that are
	// not so easily undone. However, we are likely to soon get resized to the new width and height
	// computed here, and that will recompute the linebreaks and do a full _Refresh if needed.
	fTextRect = saveTextRect;
}


//	#pragma mark - Layout methods


void
BTextView::LayoutInvalidated(bool descendants)
{
	CALLED();

	fLayoutData->valid = false;
}


void
BTextView::DoLayout()
{
	// Bail out, if we shan't do layout.
	if (!(Flags() & B_SUPPORTS_LAYOUT))
		return;

	CALLED();

	// If the user set a layout, we let the base class version call its
	// hook.
	if (GetLayout()) {
		BView::DoLayout();
		return;
	}

	_ValidateLayoutData();

	// validate current size
	BSize size(Bounds().Size());
	if (size.width < fLayoutData->min.width)
		size.width = fLayoutData->min.width;
	if (size.height < fLayoutData->min.height)
		size.height = fLayoutData->min.height;

	_ResetTextRect();
}


void
BTextView::_ValidateLayoutData()
{
	if (fLayoutData->valid)
		return;

	CALLED();

	float lineHeight = ceilf(LineHeight(0));
	TRACE("line height: %.2f\n", lineHeight);

	// compute our minimal size
	BSize min(lineHeight * 3, lineHeight);
	min.width += fLayoutData->leftInset + fLayoutData->rightInset;
	min.height += fLayoutData->topInset + fLayoutData->bottomInset;

	fLayoutData->min = min;

	// compute our preferred size
	fLayoutData->preferred.height = _TextHeight();

	if (fWrap)
		fLayoutData->preferred.width = min.width + 5 * lineHeight;
	else {
		float maxWidth = fLines->MaxWidth() + fLayoutData->leftInset + fLayoutData->rightInset;
		if (maxWidth < min.width)
			maxWidth = min.width;

		fLayoutData->preferred.width = maxWidth;
		fLayoutData->min = fLayoutData->preferred;
	}

	fLayoutData->valid = true;
	ResetLayoutInvalidation();

	TRACE("width: %.2f, height: %.2f\n", min.width, min.height);
}


//	#pragma mark -


void
BTextView::AllAttached()
{
	BView::AllAttached();
}


void
BTextView::AllDetached()
{
	BView::AllDetached();
}


/* static */
text_run_array*
BTextView::AllocRunArray(int32 entryCount, int32* outSize)
{
	int32 size = sizeof(text_run_array) + (entryCount - 1) * sizeof(text_run);

	text_run_array* runArray = (text_run_array*)calloc(size, 1);
	if (runArray == NULL) {
		if (outSize != NULL)
			*outSize = 0;
		return NULL;
	}

	runArray->count = entryCount;

	// Call constructors explicitly as the text_run_array
	// was allocated with malloc (and has to, for backwards
	// compatibility)
	for (int32 i = 0; i < runArray->count; i++)
		new (&runArray->runs[i].font) BFont;

	if (outSize != NULL)
		*outSize = size;

	return runArray;
}


/* static */
text_run_array*
BTextView::CopyRunArray(const text_run_array* orig, int32 countDelta)
{
	text_run_array* copy = AllocRunArray(countDelta, NULL);
	if (copy != NULL) {
		for (int32 i = 0; i < countDelta; i++) {
			copy->runs[i].offset = orig->runs[i].offset;
			copy->runs[i].font = orig->runs[i].font;
			copy->runs[i].color = orig->runs[i].color;
		}
	}
	return copy;
}


/* static */
void
BTextView::FreeRunArray(text_run_array* array)
{
	if (array == NULL)
		return;

	// Call destructors explicitly
	for (int32 i = 0; i < array->count; i++)
		array->runs[i].font.~BFont();

	free(array);
}


/* static */
void*
BTextView::FlattenRunArray(const text_run_array* runArray, int32* _size)
{
	CALLED();
	int32 size = sizeof(flattened_text_run_array) + (runArray->count - 1)
		* sizeof(flattened_text_run);

	flattened_text_run_array* array = (flattened_text_run_array*)malloc(size);
	if (array == NULL) {
		if (_size)
			*_size = 0;
		return NULL;
	}

	array->magic = B_HOST_TO_BENDIAN_INT32(kFlattenedTextRunArrayMagic);
	array->version = B_HOST_TO_BENDIAN_INT32(kFlattenedTextRunArrayVersion);
	array->count = B_HOST_TO_BENDIAN_INT32(runArray->count);

	for (int32 i = 0; i < runArray->count; i++) {
		array->styles[i].offset = B_HOST_TO_BENDIAN_INT32(
			runArray->runs[i].offset);
		runArray->runs[i].font.GetFamilyAndStyle(&array->styles[i].family,
			&array->styles[i].style);
		array->styles[i].size = B_HOST_TO_BENDIAN_FLOAT(
			runArray->runs[i].font.Size());
		array->styles[i].shear = B_HOST_TO_BENDIAN_FLOAT(
			runArray->runs[i].font.Shear());
		array->styles[i].face = B_HOST_TO_BENDIAN_INT16(
			runArray->runs[i].font.Face());
		array->styles[i].red = runArray->runs[i].color.red;
		array->styles[i].green = runArray->runs[i].color.green;
		array->styles[i].blue = runArray->runs[i].color.blue;
		array->styles[i].alpha = 255;
		array->styles[i]._reserved_ = 0;
	}

	if (_size)
		*_size = size;

	return array;
}


/* static */
text_run_array*
BTextView::UnflattenRunArray(const void* data, int32* _size)
{
	CALLED();
	flattened_text_run_array* array = (flattened_text_run_array*)data;

	if (B_BENDIAN_TO_HOST_INT32(array->magic) != kFlattenedTextRunArrayMagic
		|| B_BENDIAN_TO_HOST_INT32(array->version)
			!= kFlattenedTextRunArrayVersion) {
		if (_size)
			*_size = 0;

		return NULL;
	}

	int32 count = B_BENDIAN_TO_HOST_INT32(array->count);

	text_run_array* runArray = AllocRunArray(count, _size);
	if (runArray == NULL)
		return NULL;

	for (int32 i = 0; i < count; i++) {
		runArray->runs[i].offset = B_BENDIAN_TO_HOST_INT32(
			array->styles[i].offset);

		// Set family and style independently from each other, so that
		// even if the family doesn't exist, we try to preserve the style
		runArray->runs[i].font.SetFamilyAndStyle(array->styles[i].family, NULL);
		runArray->runs[i].font.SetFamilyAndStyle(NULL, array->styles[i].style);

		runArray->runs[i].font.SetSize(B_BENDIAN_TO_HOST_FLOAT(
			array->styles[i].size));
		runArray->runs[i].font.SetShear(B_BENDIAN_TO_HOST_FLOAT(
			array->styles[i].shear));

		uint16 face = B_BENDIAN_TO_HOST_INT16(array->styles[i].face);
		if (face != B_REGULAR_FACE) {
			// Be's version doesn't seem to set this correctly
			runArray->runs[i].font.SetFace(face);
		}

		runArray->runs[i].color.red = array->styles[i].red;
		runArray->runs[i].color.green = array->styles[i].green;
		runArray->runs[i].color.blue = array->styles[i].blue;
		runArray->runs[i].color.alpha = array->styles[i].alpha;
	}

	return runArray;
}


void
BTextView::InsertText(const char* text, int32 length, int32 offset,
	const text_run_array* runs)
{
	CALLED();

	if (length < 0)
		length = 0;

	if (offset < 0)
		offset = 0;
	else if (offset > fText->Length())
		offset = fText->Length();

	if (length > 0) {
		// add the text to the buffer
		fText->InsertText(text, length, offset);

		// update the start offsets of each line below offset
		fLines->BumpOffset(length, _LineAt(offset) + 1);

		// update the style runs
		fStyles->BumpOffset(length, fStyles->OffsetToRun(offset - 1) + 1);

		// offset the caret/selection, if the text was inserted before it
		if (offset <= fSelEnd) {
			fSelStart += length;
			fCaretOffset = fSelEnd = fSelStart;
		}
	}

	if (fStylable && runs != NULL)
		_SetRunArray(offset, offset + length, runs);
	else {
		// apply null-style to inserted text
		_ApplyStyleRange(offset, offset + length);
	}
}


void
BTextView::DeleteText(int32 fromOffset, int32 toOffset)
{
	CALLED();

	if (fromOffset < 0)
		fromOffset = 0;
	else if (fromOffset > fText->Length())
		fromOffset = fText->Length();

	if (toOffset < 0)
		toOffset = 0;
	else if (toOffset > fText->Length())
		toOffset = fText->Length();

	if (fromOffset >= toOffset)
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

	// adjust the selection accordingly, assumes fSelEnd >= fSelStart!
	int32 range = toOffset - fromOffset;
	if (fSelStart >= toOffset) {
		// selection is behind the range that was removed
		fSelStart -= range;
		fSelEnd -= range;
	} else if (fSelStart >= fromOffset && fSelEnd <= toOffset) {
		// the selection is within the range that was removed
		fSelStart = fSelEnd = fromOffset;
	} else if (fSelStart >= fromOffset && fSelEnd > toOffset) {
		// the selection starts within and ends after the range
		// the remaining part is the part that was after the range
		fSelStart = fromOffset;
		fSelEnd = fromOffset + fSelEnd - toOffset;
	} else if (fSelStart < fromOffset && fSelEnd < toOffset) {
		// the selection starts before, but ends within the range
		fSelEnd = fromOffset;
	} else if (fSelStart < fromOffset && fSelEnd >= toOffset) {
		// the selection starts before and ends after the range
		fSelEnd -= range;
	}
}


/*!	Undoes the last changes.

	\param clipboard A \a clipboard to use for the undo operation.
*/
void
BTextView::Undo(BClipboard* clipboard)
{
	if (fUndo)
		fUndo->Undo(clipboard);
}


undo_state
BTextView::UndoState(bool* isRedo) const
{
	return fUndo == NULL ? B_UNDO_UNAVAILABLE : fUndo->State(isRedo);
}


//	#pragma mark - GetDragParameters() is protected


void
BTextView::GetDragParameters(BMessage* drag, BBitmap** bitmap, BPoint* point,
	BHandler** handler)
{
	CALLED();
	if (drag == NULL)
		return;

	// Add originator and action
	drag->AddPointer("be:originator", this);
	drag->AddInt32("be_actions", B_TRASH_TARGET);

	// add the text
	int32 numBytes = fSelEnd - fSelStart;
	const char* text = fText->GetString(fSelStart, &numBytes);
	drag->AddData("text/plain", B_MIME_TYPE, text, numBytes);

	// add the corresponding styles
	int32 size = 0;
	text_run_array* styles = RunArray(fSelStart, fSelEnd, &size);

	if (styles != NULL) {
		drag->AddData("application/x-vnd.Be-text_run_array", B_MIME_TYPE,
			styles, size);

		FreeRunArray(styles);
	}

	if (bitmap != NULL)
		*bitmap = NULL;

	if (handler != NULL)
		*handler = NULL;
}


//	#pragma mark - FBC padding and forbidden methods


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


// #pragma mark - Private methods


/*!	Inits the BTextView object.

	\param textRect The BTextView's text rect.
	\param initialFont The font which the BTextView will use.
	\param initialColor The initial color of the text.
*/
void
BTextView::_InitObject(BRect textRect, const BFont* initialFont,
	const rgb_color* initialColor)
{
	BFont font;
	if (initialFont == NULL)
		GetFont(&font);
	else
		font = *initialFont;

	_NormalizeFont(&font);

	rgb_color documentTextColor = ui_color(B_DOCUMENT_TEXT_COLOR);

	if (initialColor == NULL)
		initialColor = &documentTextColor;

	fText = new BPrivate::TextGapBuffer;
	fLines = new LineBuffer;
	fStyles = new StyleBuffer(&font, initialColor);

	fInstalledNavigateCommandWordwiseShortcuts = false;
	fInstalledNavigateOptionWordwiseShortcuts = false;
	fInstalledNavigateOptionLinewiseShortcuts = false;
	fInstalledNavigateHomeEndDocwiseShortcuts = false;

	fInstalledSelectCommandWordwiseShortcuts = false;
	fInstalledSelectOptionWordwiseShortcuts = false;
	fInstalledSelectOptionLinewiseShortcuts = false;
	fInstalledSelectHomeEndDocwiseShortcuts = false;

	fInstalledRemoveCommandWordwiseShortcuts = false;
	fInstalledRemoveOptionWordwiseShortcuts = false;

	// We put these here instead of in the constructor initializer list
	// to have less code duplication, and a single place where to do changes
	// if needed.
	fTextRect = textRect;
		// NOTE: The only places where text rect is changed:
		// * width and height are adjusted in _RecalculateLineBreaks(),
		// text rect maintains constant insets, use SetInsets() to change.
	fMinTextRectWidth = fTextRect.Width();
		// see SetTextRect()
	fSelStart = fSelEnd = 0;
	fCaretVisible = false;
	fCaretTime = 0;
	fCaretOffset = 0;
	fClickCount = 0;
	fClickTime = 0;
	fDragOffset = -1;
	fCursor = 0;
	fActive = false;
	fStylable = false;
	fTabWidth = 28.0;
	fSelectable = true;
	fEditable = true;
	fWrap = true;
	fMaxBytes = INT32_MAX;
	fDisallowedChars = NULL;
	fAlignment = B_ALIGN_LEFT;
	fAutoindent = false;
	fOffscreen = NULL;
	fColorSpace = B_CMAP8;
	fResizable = false;
	fContainerView = NULL;
	fUndo = NULL;
	fInline = NULL;
	fDragRunner = NULL;
	fClickRunner = NULL;
	fTrackingMouse = NULL;

	fLayoutData = new LayoutData;
	_UpdateInsets(textRect);

	fLastClickOffset = -1;

	SetDoesUndo(true);
}


//!	Handles when Backspace key is pressed.
void
BTextView::_HandleBackspace(int32 modifiers)
{
	if (!fEditable)
		return;

	if (modifiers < 0) {
		BMessage* currentMessage = Window()->CurrentMessage();
		if (currentMessage == NULL
			|| currentMessage->FindInt32("modifiers", &modifiers) != B_OK) {
			modifiers = 0;
		}
	}

	bool controlKeyDown = (modifiers & B_CONTROL_KEY) != 0;
	bool optionKeyDown  = (modifiers & B_OPTION_KEY)  != 0;
	bool commandKeyDown = (modifiers & B_COMMAND_KEY) != 0;

	if ((commandKeyDown || optionKeyDown) && !controlKeyDown) {
		fSelStart = _PreviousWordStart(fCaretOffset - 1);
		fSelEnd = fCaretOffset;
	}

	if (fUndo) {
		TypingUndoBuffer* undoBuffer = dynamic_cast<TypingUndoBuffer*>(fUndo);
		if (!undoBuffer) {
			delete fUndo;
			fUndo = undoBuffer = new TypingUndoBuffer(this);
		}
		undoBuffer->BackwardErase();
	}

	// we may draw twice, so turn updates off for now
	if (Window() != NULL)
		Window()->DisableUpdates();

	if (fSelStart == fSelEnd) {
		if (fSelStart != 0)
			fSelStart = _PreviousInitialByte(fSelStart);
	} else
		Highlight(fSelStart, fSelEnd);

	DeleteText(fSelStart, fSelEnd);
	fCaretOffset = fSelEnd = fSelStart;

	_Refresh(fSelStart, fSelEnd, fCaretOffset);

	// turn updates back on
	if (Window() != NULL)
		Window()->EnableUpdates();
}


//!	Handles when an arrow key is pressed.
void
BTextView::_HandleArrowKey(uint32 arrowKey, int32 modifiers)
{
	// return if there's nowhere to go
	if (fText->Length() == 0)
		return;

	int32 selStart = fSelStart;
	int32 selEnd = fSelEnd;

	if (modifiers < 0) {
		BMessage* currentMessage = Window()->CurrentMessage();
		if (currentMessage == NULL
			|| currentMessage->FindInt32("modifiers", &modifiers) != B_OK) {
			modifiers = 0;
		}
	}

	bool shiftKeyDown   = (modifiers & B_SHIFT_KEY)   != 0;
	bool controlKeyDown = (modifiers & B_CONTROL_KEY) != 0;
	bool optionKeyDown  = (modifiers & B_OPTION_KEY)  != 0;
	bool commandKeyDown = (modifiers & B_COMMAND_KEY) != 0;

	int32 lastClickOffset = fCaretOffset;

	switch (arrowKey) {
		case B_LEFT_ARROW:
			if (!fEditable && !fSelectable)
				_ScrollBy(-kHorizontalScrollBarStep, 0);
			else if (fSelStart != fSelEnd && !shiftKeyDown)
				fCaretOffset = fSelStart;
			else {
				if ((commandKeyDown || optionKeyDown) && !controlKeyDown)
					fCaretOffset = _PreviousWordStart(fCaretOffset - 1);
				else
					fCaretOffset = _PreviousInitialByte(fCaretOffset);

				if (shiftKeyDown && fCaretOffset != lastClickOffset) {
					if (fCaretOffset < fSelStart) {
						// extend selection to the left
						selStart = fCaretOffset;
						if (lastClickOffset > fSelStart) {
							// caret has jumped across "anchor"
							selEnd = fSelStart;
						}
					} else {
						// shrink selection from the right
						selEnd = fCaretOffset;
					}
				}
			}
			break;

		case B_RIGHT_ARROW:
			if (!fEditable && !fSelectable)
				_ScrollBy(kHorizontalScrollBarStep, 0);
			else if (fSelStart != fSelEnd && !shiftKeyDown)
				fCaretOffset = fSelEnd;
			else {
				if ((commandKeyDown || optionKeyDown) && !controlKeyDown)
					fCaretOffset = _NextWordEnd(fCaretOffset);
				else
					fCaretOffset = _NextInitialByte(fCaretOffset);

				if (shiftKeyDown && fCaretOffset != lastClickOffset) {
					if (fCaretOffset > fSelEnd) {
						// extend selection to the right
						selEnd = fCaretOffset;
						if (lastClickOffset < fSelEnd) {
							// caret has jumped across "anchor"
							selStart = fSelEnd;
						}
					} else {
						// shrink selection from the left
						selStart = fCaretOffset;
					}
				}
			}
			break;

		case B_UP_ARROW:
		{
			if (!fEditable && !fSelectable)
				_ScrollBy(0, -kVerticalScrollBarStep);
			else if (fSelStart != fSelEnd && !shiftKeyDown)
				fCaretOffset = fSelStart;
			else {
				if (optionKeyDown && !commandKeyDown && !controlKeyDown)
					fCaretOffset = _PreviousLineStart(fCaretOffset);
				else if (commandKeyDown && !optionKeyDown && !controlKeyDown) {
					_ScrollTo(0, 0);
					fCaretOffset = 0;
				} else {
					float height;
					BPoint point = PointAt(fCaretOffset, &height);
					// find the caret position on the previous
					// line by gently stepping onto this line
					for (int i = 1; i <= height; i++) {
						point.y--;
						int32 offset = OffsetAt(point);
						if (offset < fCaretOffset || i == height) {
							fCaretOffset = offset;
							break;
						}
					}
				}

				if (shiftKeyDown && fCaretOffset != lastClickOffset) {
					if (fCaretOffset < fSelStart) {
						// extend selection to the top
						selStart = fCaretOffset;
						if (lastClickOffset > fSelStart) {
							// caret has jumped across "anchor"
							selEnd = fSelStart;
						}
					} else {
						// shrink selection from the bottom
						selEnd = fCaretOffset;
					}
				}
			}
			break;
		}

		case B_DOWN_ARROW:
		{
			if (!fEditable && !fSelectable)
				_ScrollBy(0, kVerticalScrollBarStep);
			else if (fSelStart != fSelEnd && !shiftKeyDown)
				fCaretOffset = fSelEnd;
			else {
				if (optionKeyDown && !commandKeyDown && !controlKeyDown)
					fCaretOffset = _NextLineEnd(fCaretOffset);
				else if (commandKeyDown && !optionKeyDown && !controlKeyDown) {
					_ScrollTo(0, fTextRect.bottom + fLayoutData->bottomInset);
					fCaretOffset = fText->Length();
				} else {
					float height;
					BPoint point = PointAt(fCaretOffset, &height);
					point.y += height;
					fCaretOffset = OffsetAt(point);
				}

				if (shiftKeyDown && fCaretOffset != lastClickOffset) {
					if (fCaretOffset > fSelEnd) {
						// extend selection to the bottom
						selEnd = fCaretOffset;
						if (lastClickOffset < fSelEnd) {
							// caret has jumped across "anchor"
							selStart = fSelEnd;
						}
					} else {
						// shrink selection from the top
						selStart = fCaretOffset;
					}
				}
			}
			break;
		}
	}

	fStyles->InvalidateNullStyle();

	if (fEditable || fSelectable) {
		if (shiftKeyDown)
			Select(selStart, selEnd);
		else
			Select(fCaretOffset, fCaretOffset);

		// scroll if needed
		ScrollToOffset(fCaretOffset);
	}
}


//!	Handles when the Delete key is pressed.
void
BTextView::_HandleDelete(int32 modifiers)
{
	if (!fEditable)
		return;

	if (modifiers < 0) {
		BMessage* currentMessage = Window()->CurrentMessage();
		if (currentMessage == NULL
			|| currentMessage->FindInt32("modifiers", &modifiers) != B_OK) {
			modifiers = 0;
		}
	}

	bool controlKeyDown = (modifiers & B_CONTROL_KEY) != 0;
	bool optionKeyDown  = (modifiers & B_OPTION_KEY)  != 0;
	bool commandKeyDown = (modifiers & B_COMMAND_KEY) != 0;

	if ((commandKeyDown || optionKeyDown) && !controlKeyDown) {
		fSelStart = fCaretOffset;
		fSelEnd = _NextWordEnd(fCaretOffset) + 1;
	}

	if (fUndo) {
		TypingUndoBuffer* undoBuffer = dynamic_cast<TypingUndoBuffer*>(fUndo);
		if (!undoBuffer) {
			delete fUndo;
			fUndo = undoBuffer = new TypingUndoBuffer(this);
		}
		undoBuffer->ForwardErase();
	}

	// we may draw twice, so turn updates off for now
	if (Window() != NULL)
		Window()->DisableUpdates();

	if (fSelStart == fSelEnd) {
		if (fSelEnd != fText->Length())
			fSelEnd = _NextInitialByte(fSelEnd);
	} else
		Highlight(fSelStart, fSelEnd);

	DeleteText(fSelStart, fSelEnd);
	fCaretOffset = fSelEnd = fSelStart;

	_Refresh(fSelStart, fSelEnd, fCaretOffset);

	// turn updates back on
	if (Window() != NULL)
		Window()->EnableUpdates();
}


//!	Handles when the Page Up, Page Down, Home, or End key is pressed.
void
BTextView::_HandlePageKey(uint32 pageKey, int32 modifiers)
{
	if (modifiers < 0) {
		BMessage* currentMessage = Window()->CurrentMessage();
		if (currentMessage == NULL
			|| currentMessage->FindInt32("modifiers", &modifiers) != B_OK) {
			modifiers = 0;
		}
	}

	bool shiftKeyDown   = (modifiers & B_SHIFT_KEY)   != 0;
	bool controlKeyDown = (modifiers & B_CONTROL_KEY) != 0;
	bool optionKeyDown  = (modifiers & B_OPTION_KEY)  != 0;
	bool commandKeyDown = (modifiers & B_COMMAND_KEY) != 0;

	STELine* line = NULL;
	int32 selStart = fSelStart;
	int32 selEnd = fSelEnd;

	int32 lastClickOffset = fCaretOffset;
	switch (pageKey) {
		case B_HOME:
			if (!fEditable && !fSelectable) {
				fCaretOffset = 0;
				_ScrollTo(0, 0);
				break;
			} else {
				if (commandKeyDown && !optionKeyDown && !controlKeyDown) {
					_ScrollTo(0, 0);
					fCaretOffset = 0;
				} else {
					// get the start of the last line if caret is on it
					line = (*fLines)[_LineAt(lastClickOffset)];
					fCaretOffset = line->offset;
				}

				if (!shiftKeyDown)
					selStart = selEnd = fCaretOffset;
				else if (fCaretOffset != lastClickOffset) {
					if (fCaretOffset < fSelStart) {
						// extend selection to the left
						selStart = fCaretOffset;
						if (lastClickOffset > fSelStart) {
							// caret has jumped across "anchor"
							selEnd = fSelStart;
						}
					} else {
						// shrink selection from the right
						selEnd = fCaretOffset;
					}
				}
			}
			break;

		case B_END:
			if (!fEditable && !fSelectable) {
				fCaretOffset = fText->Length();
				_ScrollTo(0, fTextRect.bottom + fLayoutData->bottomInset);
				break;
			} else {
				if (commandKeyDown && !optionKeyDown && !controlKeyDown) {
					_ScrollTo(0, fTextRect.bottom + fLayoutData->bottomInset);
					fCaretOffset = fText->Length();
				} else {
					// If we are on the last line, just go to the last
					// character in the buffer, otherwise get the starting
					// offset of the next line, and go to the previous character
					int32 currentLine = _LineAt(lastClickOffset);
					if (currentLine + 1 < fLines->NumLines()) {
						line = (*fLines)[currentLine + 1];
						fCaretOffset = _PreviousInitialByte(line->offset);
					} else {
						// This check is needed to avoid moving the cursor
						// when the cursor is on the last line, and that line
						// is empty
						if (fCaretOffset != fText->Length()) {
							fCaretOffset = fText->Length();
							if (ByteAt(fCaretOffset - 1) == B_ENTER)
								fCaretOffset--;
						}
					}
				}

				if (!shiftKeyDown)
					selStart = selEnd = fCaretOffset;
				else if (fCaretOffset != lastClickOffset) {
					if (fCaretOffset > fSelEnd) {
						// extend selection to the right
						selEnd = fCaretOffset;
						if (lastClickOffset < fSelEnd) {
							// caret has jumped across "anchor"
							selStart = fSelEnd;
						}
					} else {
						// shrink selection from the left
						selStart = fCaretOffset;
					}
				}
			}
			break;

		case B_PAGE_UP:
		{
			float lineHeight;
			BPoint currentPos = PointAt(fCaretOffset, &lineHeight);
			BPoint nextPos(currentPos.x,
				currentPos.y + lineHeight - Bounds().Height());
			fCaretOffset = OffsetAt(nextPos);
			nextPos = PointAt(fCaretOffset);
			_ScrollBy(0, nextPos.y - currentPos.y);

			if (!fEditable && !fSelectable)
				break;

			if (!shiftKeyDown)
				selStart = selEnd = fCaretOffset;
			else if (fCaretOffset != lastClickOffset) {
				if (fCaretOffset < fSelStart) {
					// extend selection to the top
					selStart = fCaretOffset;
					if (lastClickOffset > fSelStart) {
						// caret has jumped across "anchor"
						selEnd = fSelStart;
					}
				} else {
					// shrink selection from the bottom
					selEnd = fCaretOffset;
				}
			}

			break;
		}

		case B_PAGE_DOWN:
		{
			BPoint currentPos = PointAt(fCaretOffset);
			BPoint nextPos(currentPos.x, currentPos.y + Bounds().Height());
			fCaretOffset = OffsetAt(nextPos);
			nextPos = PointAt(fCaretOffset);
			_ScrollBy(0, nextPos.y - currentPos.y);

			if (!fEditable && !fSelectable)
				break;

			if (!shiftKeyDown)
				selStart = selEnd = fCaretOffset;
			else if (fCaretOffset != lastClickOffset) {
				if (fCaretOffset > fSelEnd) {
					// extend selection to the bottom
					selEnd = fCaretOffset;
					if (lastClickOffset < fSelEnd) {
						// caret has jumped across "anchor"
						selStart = fSelEnd;
					}
				} else {
					// shrink selection from the top
					selStart = fCaretOffset;
				}
			}

			break;
		}
	}

	if (fEditable || fSelectable) {
		if (shiftKeyDown)
			Select(selStart, selEnd);
		else
			Select(fCaretOffset, fCaretOffset);

		ScrollToOffset(fCaretOffset);
	}
}


/*!	Handles when an alpha-numeric key is pressed.

	\param bytes The string or character associated with the key.
	\param numBytes The amount of bytes containes in "bytes".
*/
void
BTextView::_HandleAlphaKey(const char* bytes, int32 numBytes)
{
	if (!fEditable)
		return;

	if (fUndo) {
		TypingUndoBuffer* undoBuffer = dynamic_cast<TypingUndoBuffer*>(fUndo);
		if (!undoBuffer) {
			delete fUndo;
			fUndo = undoBuffer = new TypingUndoBuffer(this);
		}
		undoBuffer->InputCharacter(numBytes);
	}

	if (fSelStart != fSelEnd) {
		Highlight(fSelStart, fSelEnd);
		DeleteText(fSelStart, fSelEnd);
	}

	// we may draw twice, so turn updates off for now
	if (Window() != NULL)
		Window()->DisableUpdates();

	if (fAutoindent && numBytes == 1 && *bytes == B_ENTER) {
		int32 start, offset;
		start = offset = OffsetAt(_LineAt(fSelStart));

		while (ByteAt(offset) != '\0' &&
				(ByteAt(offset) == B_TAB || ByteAt(offset) == B_SPACE)
				&& offset < fSelStart)
			offset++;

		_DoInsertText(bytes, numBytes, fSelStart, NULL);
		if (start != offset)
			_DoInsertText(Text() + start, offset - start, fSelStart, NULL);
	} else
		_DoInsertText(bytes, numBytes, fSelStart, NULL);

	fCaretOffset = fSelEnd;
	ScrollToOffset(fCaretOffset);

	// turn updates back on
	if (Window() != NULL)
		Window()->EnableUpdates();
}


/*!	Redraw the text between the two given offsets, recalculating line-breaks
	if needed.

	\param fromOffset The offset from where to refresh.
	\param toOffset The offset where to refresh to.
	\param scrollTo Scroll the view to \a scrollTo offset if not \c INT32_MIN.
*/
void
BTextView::_Refresh(int32 fromOffset, int32 toOffset, int32 scrollTo)
{
	// TODO: Cleanup
	float saveHeight = fTextRect.Height();
	float saveWidth = fTextRect.Width();
	int32 fromLine = _LineAt(fromOffset);
	int32 toLine = _LineAt(toOffset);
	int32 saveFromLine = fromLine;
	int32 saveToLine = toLine;

	_RecalculateLineBreaks(&fromLine, &toLine);

	// TODO: Maybe there is still something we can do without a window...
	if (!Window())
		return;

	BRect bounds = Bounds();
	float newHeight = fTextRect.Height();

	// if the line breaks have changed, force an erase
	if (fromLine != saveFromLine || toLine != saveToLine
			|| newHeight != saveHeight) {
		fromOffset = -1;
	}

	if (newHeight != saveHeight) {
		// the text area has changed
		if (newHeight < saveHeight)
			toLine = _LineAt(BPoint(0.0f, saveHeight + fTextRect.top));
		else
			toLine = _LineAt(BPoint(0.0f, newHeight + fTextRect.top));
	}

	// draw only those lines that are visible
	int32 fromVisible = _LineAt(BPoint(0.0f, bounds.top));
	int32 toVisible = _LineAt(BPoint(0.0f, bounds.bottom));
	fromLine = std::max(fromVisible, fromLine);
	toLine = std::min(toLine, toVisible);

	_AutoResize(false);

	_RequestDrawLines(fromLine, toLine);

	// erase the area below the text
	BRect eraseRect = bounds;
	eraseRect.top = fTextRect.top + (*fLines)[fLines->NumLines()]->origin;
	eraseRect.bottom = fTextRect.top + saveHeight;
	if (eraseRect.bottom > eraseRect.top && eraseRect.Intersects(bounds)) {
		SetLowColor(ViewColor());
		FillRect(eraseRect, B_SOLID_LOW);
	}

	// update the scroll bars if the text area has changed
	if (newHeight != saveHeight || fMinTextRectWidth != saveWidth)
		_UpdateScrollbars();

	if (scrollTo != INT32_MIN)
		ScrollToOffset(scrollTo);

	Flush();
}


/*!	Recalculate line breaks between two lines.

	\param startLine The line number to start recalculating line breaks.
	\param endLine The line number to stop recalculating line breaks.
*/
void
BTextView::_RecalculateLineBreaks(int32* startLine, int32* endLine)
{
	CALLED();

	float width = fTextRect.Width();

	// don't try to compute anything with word wrapping if the text rect is not set
	if (fWrap && (!fTextRect.IsValid() || width == 0))
		return;

	// sanity check
	*startLine = (*startLine < 0) ? 0 : *startLine;
	*endLine = (*endLine > fLines->NumLines() - 1) ? fLines->NumLines() - 1
		: *endLine;

	int32 textLength = fText->Length();
	int32 lineIndex = (*startLine > 0) ? *startLine - 1 : 0;
	int32 recalThreshold = (*fLines)[*endLine + 1]->offset;
	STELine* curLine = (*fLines)[lineIndex];
	STELine* nextLine = curLine + 1;

	do {
		float ascent, descent;
		int32 fromOffset = curLine->offset;
		int32 toOffset = _FindLineBreak(fromOffset, &ascent, &descent, &width);

		curLine->ascent = ascent;
		curLine->width = width;

		// we want to advance at least by one character
		int32 nextOffset = _NextInitialByte(fromOffset);
		if (toOffset < nextOffset && fromOffset < textLength)
			toOffset = nextOffset;

		lineIndex++;
		STELine saveLine = *nextLine;
		if (lineIndex > fLines->NumLines() || toOffset < nextLine->offset) {
			// the new line comes before the old line start, add a line
			STELine newLine;
			newLine.offset = toOffset;
			newLine.origin = ceilf(curLine->origin + ascent + descent) + 1;
			newLine.ascent = 0;
			fLines->InsertLine(&newLine, lineIndex);
		} else {
			// update the existing line
			nextLine->offset = toOffset;
			nextLine->origin = ceilf(curLine->origin + ascent + descent) + 1;

			// remove any lines that start before the current line
			while (lineIndex < fLines->NumLines()
				&& toOffset >= ((*fLines)[lineIndex] + 1)->offset) {
				fLines->RemoveLines(lineIndex + 1);
			}

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

	// make sure that the sentinel line (which starts at the end of the buffer)
	// has always a width of 0
	(*fLines)[fLines->NumLines()]->width = 0;

	// update text rect
	fTextRect.left = Bounds().left + fLayoutData->leftInset;
	fTextRect.right = Bounds().right - fLayoutData->rightInset;

	// always set text rect bottom
	float newHeight = TextHeight(0, fLines->NumLines() - 1);
	fTextRect.bottom = fTextRect.top + newHeight;

	if (!fWrap) {
		fMinTextRectWidth = fLines->MaxWidth() - 1;

		// expand width if needed
		switch (fAlignment) {
			default:
			case B_ALIGN_LEFT:
				// move right edge
				fTextRect.right = fTextRect.left + fMinTextRectWidth;
				break;

			case B_ALIGN_RIGHT:
				// move left edge
				fTextRect.left = fTextRect.right - fMinTextRectWidth;
				break;

			case B_ALIGN_CENTER:
				// move both edges
				fTextRect.InsetBy(roundf((fTextRect.Width()
					- fMinTextRectWidth) / 2), 0);
				break;
		}

		_ValidateTextRect();
	}

	*endLine = lineIndex - 1;
	*startLine = std::min(*startLine, *endLine);
}


void
BTextView::_ValidateTextRect()
{
	// text rect right must be greater than left
	if (fTextRect.right <= fTextRect.left)
		fTextRect.right = fTextRect.left + 1;
	// text rect bottom must be greater than top
	if (fTextRect.bottom <= fTextRect.top)
		fTextRect.bottom = fTextRect.top + 1;
}


int32
BTextView::_FindLineBreak(int32 fromOffset, float* _ascent, float* _descent,
	float* inOutWidth)
{
	*_ascent = 0.0;
	*_descent = 0.0;

	const int32 limit = fText->Length();

	// is fromOffset at the end?
	if (fromOffset >= limit) {
		// try to return valid height info anyway
		if (fStyles->NumRuns() > 0) {
			fStyles->Iterate(fromOffset, 1, fInline, NULL, NULL, _ascent,
				_descent);
		} else {
			if (fStyles->IsValidNullStyle()) {
				const BFont* font = NULL;
				fStyles->GetNullStyle(&font, NULL);

				font_height fh;
				font->GetHeight(&fh);
				*_ascent = fh.ascent;
				*_descent = fh.descent + fh.leading;
			}
		}
		*inOutWidth = 0;

		return limit;
	}

	int32 offset = fromOffset;

	if (!fWrap) {
		// Text wrapping is turned off.
		// Just find the offset of the first \n character
		offset = limit - fromOffset;
		fText->FindChar(B_ENTER, fromOffset, &offset);
		offset += fromOffset;
		int32 toOffset = (offset < limit) ? offset : limit;

		*inOutWidth = _TabExpandedStyledWidth(fromOffset, toOffset - fromOffset,
			_ascent, _descent);

		return offset < limit ? offset + 1 : limit;
	}

	bool done = false;
	float ascent = 0.0;
	float descent = 0.0;
	int32 delta = 0;
	float deltaWidth = 0.0;
	float strWidth = 0.0;
	uchar theChar;

	// wrap the text
	while (offset < limit && !done) {
		// find the next line break candidate
		for (; (offset + delta) < limit; delta++) {
			if (CanEndLine(offset + delta)) {
				theChar = fText->RealCharAt(offset + delta);
				if (theChar != B_SPACE && theChar != B_TAB
					&& theChar != B_ENTER) {
					// we are scanning for trailing whitespace below, so we
					// have to skip non-whitespace characters, that can end
					// the line, here
					delta++;
				}
				break;
			}
		}

		int32 deltaBeforeWhitespace = delta;
		// now skip over trailing whitespace, if any
		for (; (offset + delta) < limit; delta++) {
			theChar = fText->RealCharAt(offset + delta);
			if (theChar == B_ENTER) {
				// found a newline, we're done!
				done = true;
				delta++;
				break;
			} else if (theChar != B_SPACE && theChar != B_TAB) {
				// stop at anything else than trailing whitespace
				break;
			}
		}

		delta = std::max(delta, (int32)1);

		// do not include B_ENTER-terminator into width & height calculations
		deltaWidth = _TabExpandedStyledWidth(offset,
								done ? delta - 1 : delta, &ascent, &descent);
		strWidth += deltaWidth;

		if (strWidth >= *inOutWidth) {
			// we've found where the line will wrap
			done = true;

			// we have included trailing whitespace in the width computation
			// above, but that is not being shown anyway, so we try again
			// without the trailing whitespace
			if (delta == deltaBeforeWhitespace) {
				// there is no trailing whitespace, no point in trying
				break;
			}

			// reset string width to start of current run ...
			strWidth -= deltaWidth;

			// ... and compute the resulting width (of visible characters)
			strWidth += _StyledWidth(offset, deltaBeforeWhitespace, NULL, NULL);
			if (strWidth >= *inOutWidth) {
				// width of visible characters exceeds line, we need to wrap
				// before the current "word"
				break;
			}
		}

		*_ascent = std::max(ascent, *_ascent);
		*_descent = std::max(descent, *_descent);

		offset += delta;
		delta = 0;
	}

	if (offset - fromOffset < 1) {
		// there weren't any words that fit entirely in this line
		// force a break in the middle of a word
		*_ascent = 0.0;
		*_descent = 0.0;
		strWidth = 0.0;

		int32 current = fromOffset;
		for (offset = _NextInitialByte(current); current < limit;
				current = offset, offset = _NextInitialByte(offset)) {
			strWidth += _StyledWidth(current, offset - current, &ascent,
				&descent);
			if (strWidth >= *inOutWidth) {
				offset = _PreviousInitialByte(offset);
				break;
			}

			*_ascent = std::max(ascent, *_ascent);
			*_descent = std::max(descent, *_descent);
		}
	}

	return std::min(offset, limit);
}


int32
BTextView::_PreviousLineStart(int32 offset)
{
	if (offset <= 0)
		return 0;

	while (offset > 0) {
		offset = _PreviousInitialByte(offset);
		if (_CharClassification(offset) == CHAR_CLASS_WHITESPACE
			&& ByteAt(offset) == B_ENTER) {
			return offset + 1;
		}
	}

	return offset;
}


int32
BTextView::_NextLineEnd(int32 offset)
{
	int32 textLen = fText->Length();
	if (offset >= textLen)
		return textLen;

	while (offset < textLen) {
		if (_CharClassification(offset) == CHAR_CLASS_WHITESPACE
			&& ByteAt(offset) == B_ENTER) {
			break;
		}
		offset = _NextInitialByte(offset);
	}

	return offset;
}


int32
BTextView::_PreviousWordBoundary(int32 offset)
{
	uint32 charType = _CharClassification(offset);
	int32 previous;
	while (offset > 0) {
		previous = _PreviousInitialByte(offset);
		if (_CharClassification(previous) != charType)
			break;
		offset = previous;
	}

	return offset;
}


int32
BTextView::_NextWordBoundary(int32 offset)
{
	int32 textLen = fText->Length();
	uint32 charType = _CharClassification(offset);
	while (offset < textLen) {
		offset = _NextInitialByte(offset);
		if (_CharClassification(offset) != charType)
			break;
	}

	return offset;
}


int32
BTextView::_PreviousWordStart(int32 offset)
{
	if (offset <= 1)
		return 0;

	--offset;
		// need to look at previous char
	if (_CharClassification(offset) != CHAR_CLASS_DEFAULT) {
		// skip non-word characters
		while (offset > 0) {
			offset = _PreviousInitialByte(offset);
			if (_CharClassification(offset) == CHAR_CLASS_DEFAULT)
				break;
		}
	}
	while (offset > 0) {
		// skip to start of word
		int32 previous = _PreviousInitialByte(offset);
		if (_CharClassification(previous) != CHAR_CLASS_DEFAULT)
			break;
		offset = previous;
	}

	return offset;
}


int32
BTextView::_NextWordEnd(int32 offset)
{
	int32 textLen = fText->Length();
	if (_CharClassification(offset) != CHAR_CLASS_DEFAULT) {
		// skip non-word characters
		while (offset < textLen) {
			offset = _NextInitialByte(offset);
			if (_CharClassification(offset) == CHAR_CLASS_DEFAULT)
				break;
		}
	}
	while (offset < textLen) {
		// skip to end of word
		offset = _NextInitialByte(offset);
		if (_CharClassification(offset) != CHAR_CLASS_DEFAULT)
			break;
	}

	return offset;
}


/*!	Returns the width used by the characters starting at the given
	offset with the given length, expanding all tab characters as needed.
*/
float
BTextView::_TabExpandedStyledWidth(int32 offset, int32 length, float* _ascent,
	float* _descent) const
{
	float ascent = 0.0;
	float descent = 0.0;
	float maxAscent = 0.0;
	float maxDescent = 0.0;

	float width = 0.0;
	int32 numBytes = length;
	bool foundTab = false;
	do {
		foundTab = fText->FindChar(B_TAB, offset, &numBytes);
		width += _StyledWidth(offset, numBytes, &ascent, &descent);

		if (maxAscent < ascent)
			maxAscent = ascent;
		if (maxDescent < descent)
			maxDescent = descent;

		if (foundTab) {
			width += _ActualTabWidth(width);
			numBytes++;
		}

		offset += numBytes;
		length -= numBytes;
		numBytes = length;
	} while (foundTab && length > 0);

	if (_ascent != NULL)
		*_ascent = maxAscent;
	if (_descent != NULL)
		*_descent = maxDescent;

	return width;
}


/*!	Calculate the width of the text within the given limits.

	\param fromOffset The offset where to start.
	\param length The length of the text to examine.
	\param _ascent A pointer to a float which will contain the maximum ascent.
	\param _descent A pointer to a float which will contain the maximum descent.

	\return The width for the text within the given limits.
*/
float
BTextView::_StyledWidth(int32 fromOffset, int32 length, float* _ascent,
	float* _descent) const
{
	if (length == 0) {
		// determine height of char at given offset, but return empty width
		fStyles->Iterate(fromOffset, 1, fInline, NULL, NULL, _ascent,
			_descent);
		return 0.0;
	}

	float result = 0.0;
	float ascent = 0.0;
	float descent = 0.0;
	float maxAscent = 0.0;
	float maxDescent = 0.0;

	// iterate through the style runs
	const BFont* font = NULL;
	int32 numBytes;
	while ((numBytes = fStyles->Iterate(fromOffset, length, fInline, &font,
			NULL, &ascent, &descent)) != 0) {
		maxAscent = std::max(ascent, maxAscent);
		maxDescent = std::max(descent, maxDescent);

#if USE_WIDTHBUFFER
		// Use _BWidthBuffer_ if possible
		if (BPrivate::gWidthBuffer != NULL) {
			result += BPrivate::gWidthBuffer->StringWidth(*fText, fromOffset,
				numBytes, font);
		} else {
#endif
			const char* text = fText->GetString(fromOffset, &numBytes);
			result += font->StringWidth(text, numBytes);

#if USE_WIDTHBUFFER
		}
#endif

		fromOffset += numBytes;
		length -= numBytes;
	}

	if (_ascent != NULL)
		*_ascent = maxAscent;
	if (_descent != NULL)
		*_descent = maxDescent;

	return result;
}


//!	Calculate the actual tab width for the given location.
float
BTextView::_ActualTabWidth(float location) const
{
	float tabWidth = fTabWidth - fmod(location, fTabWidth);
	if (round(tabWidth) == 0)
		tabWidth = fTabWidth;

	return tabWidth;
}


void
BTextView::_DoInsertText(const char* text, int32 length, int32 offset,
	const text_run_array* runs)
{
	_CancelInputMethod();

	if (fText->Length() + length > MaxBytes())
		return;

	if (fSelStart != fSelEnd)
		Select(fSelStart, fSelStart);

	const int32 textLength = fText->Length();
	if (offset > textLength)
		offset = textLength;

	// copy data into buffer
	InsertText(text, length, offset, runs);

	// recalc line breaks and draw the text
	_Refresh(offset, offset + length);
}


void
BTextView::_DoDeleteText(int32 fromOffset, int32 toOffset)
{
	CALLED();
}


void
BTextView::_DrawLine(BView* view, const int32 &lineNum,
	const int32 &startOffset, const bool &erase, BRect &eraseRect,
	BRegion &inputRegion)
{
	STELine* line = (*fLines)[lineNum];
	float startLeft = fTextRect.left;
	if (startOffset != -1) {
		if (ByteAt(startOffset) == B_ENTER) {
			// StartOffset is a newline
			startLeft = PointAt(line->offset).x;
		} else
			startLeft = PointAt(startOffset).x;
	} else if (fAlignment != B_ALIGN_LEFT) {
		float alignmentOffset = fTextRect.Width() + 1 - LineWidth(lineNum);
		if (fAlignment == B_ALIGN_CENTER)
			alignmentOffset = floorf(alignmentOffset / 2);
		startLeft += alignmentOffset;
	}

	int32 length = (line + 1)->offset;
	if (startOffset != -1)
		length -= startOffset;
	else
		length -= line->offset;

	// DrawString() chokes if you draw a newline
	if (ByteAt((line + 1)->offset - 1) == B_ENTER)
		length--;

	view->MovePenTo(startLeft,
		line->origin + line->ascent + fTextRect.top + 1);

	if (erase) {
		eraseRect.top = line->origin + fTextRect.top;
		eraseRect.bottom = (line + 1)->origin + fTextRect.top;
		view->FillRect(eraseRect, B_SOLID_LOW);
	}

	// do we have any text to draw?
	if (length <= 0)
		return;

	bool foundTab = false;
	int32 tabChars = 0;
	int32 numTabs = 0;
	int32 offset = startOffset != -1 ? startOffset : line->offset;
	const BFont* font = NULL;
	const rgb_color* color = NULL;
	int32 numBytes;
	drawing_mode defaultTextRenderingMode = DrawingMode();
	// iterate through each style on this line
	while ((numBytes = fStyles->Iterate(offset, length, fInline, &font,
			&color)) != 0) {
		view->SetFont(font);
		view->SetHighColor(*color);

		tabChars = std::min(numBytes, length);
		do {
			foundTab = fText->FindChar(B_TAB, offset, &tabChars);
			if (foundTab) {
				do {
					numTabs++;
					if (ByteAt(offset + tabChars + numTabs) != B_TAB)
						break;
				} while ((tabChars + numTabs) < numBytes);
			}

			drawing_mode textRenderingMode = defaultTextRenderingMode;

			if (inputRegion.CountRects() > 0
				&& ((offset <= fInline->Offset()
					&& fInline->Offset() < offset + tabChars)
				|| (fInline->Offset() <= offset
					&& offset < fInline->Offset() + fInline->Length()))) {

				textRenderingMode = B_OP_OVER;

				BRegion textRegion;
				GetTextRegion(offset, offset + length, &textRegion);

				textRegion.IntersectWith(&inputRegion);
				view->PushState();

				// Highlight in blue the inputted text
				view->SetHighColor(kBlueInputColor);
				view->FillRect(textRegion.Frame());

				// Highlight in red the selected part
				if (fInline->SelectionLength() > 0) {
					BRegion selectedRegion;
					GetTextRegion(fInline->Offset()
						+ fInline->SelectionOffset(), fInline->Offset()
						+ fInline->SelectionOffset()
						+ fInline->SelectionLength(), &selectedRegion);

					textRegion.IntersectWith(&selectedRegion);

					view->SetHighColor(kRedInputColor);
					view->FillRect(textRegion.Frame());
				}

				view->PopState();
			}

			int32 size = tabChars;
			const char* stringToDraw = fText->GetString(offset, &size);
			view->SetDrawingMode(textRenderingMode);
			view->DrawString(stringToDraw, size);

			if (foundTab) {
				float penPos = PenLocation().x - fTextRect.left;
				switch (fAlignment) {
					default:
					case B_ALIGN_LEFT:
						// nothing more to do
						break;

					case B_ALIGN_RIGHT:
						// subtract distance from left to line
						penPos -= fTextRect.Width() - LineWidth(lineNum);
						break;

					case B_ALIGN_CENTER:
						// subtract half distance from left to line
						penPos -= floorf((fTextRect.Width() + 1
							- LineWidth(lineNum)) / 2);
						break;
				}
				float tabWidth = _ActualTabWidth(penPos);

				// add in the rest of the tabs (if there are any)
				tabWidth += ((numTabs - 1) * fTabWidth);

				// move pen by tab(s) width
				view->MovePenBy(tabWidth, 0.0);
				tabChars += numTabs;
			}

			offset += tabChars;
			length -= tabChars;
			numBytes -= tabChars;
			tabChars = std::min(numBytes, length);
			numTabs = 0;
		} while (foundTab && tabChars > 0);
	}
}


void
BTextView::_DrawLines(int32 startLine, int32 endLine, int32 startOffset,
	bool erase)
{
	if (!Window())
		return;

	const BRect bounds(Bounds());

	// clip the text extending to end of selection
	BRect clipRect(fTextRect);
	clipRect.left = std::min(fTextRect.left,
		bounds.left + fLayoutData->leftInset);
	clipRect.right = std::max(fTextRect.right,
		bounds.right - fLayoutData->rightInset);
	clipRect = bounds & clipRect;

	BRegion newClip;
	newClip.Set(clipRect);
	ConstrainClippingRegion(&newClip);

	// set the low color to the view color so that
	// drawing to a non-white background will work
	SetLowColor(ViewColor());

	BView* view = NULL;
	if (fOffscreen == NULL)
		view = this;
	else {
		fOffscreen->Lock();
		view = fOffscreen->ChildAt(0);
		view->SetLowColor(ViewColor());
		view->FillRect(view->Bounds(), B_SOLID_LOW);
	}

	long maxLine = fLines->NumLines() - 1;
	if (startLine < 0)
		startLine = 0;
	if (endLine > maxLine)
		endLine = maxLine;

	// TODO: See if we can avoid this
	if (fAlignment != B_ALIGN_LEFT)
		erase = true;

	BRect eraseRect = clipRect;
	int32 startEraseLine = startLine;
	STELine* line = (*fLines)[startLine];

	if (erase && startOffset != -1 && fAlignment == B_ALIGN_LEFT) {
		// erase only to the right of startOffset
		startEraseLine++;
		int32 startErase = startOffset;

		BPoint erasePoint = PointAt(startErase);
		eraseRect.left = erasePoint.x;
		eraseRect.top = erasePoint.y;
		eraseRect.bottom = (line + 1)->origin + fTextRect.top;

		view->FillRect(eraseRect, B_SOLID_LOW);

		eraseRect = clipRect;
	}

	BRegion inputRegion;
	if (fInline != NULL && fInline->IsActive()) {
		GetTextRegion(fInline->Offset(), fInline->Offset() + fInline->Length(),
			&inputRegion);
	}

	//BPoint leftTop(startLeft, line->origin);
	for (int32 lineNum = startLine; lineNum <= endLine; lineNum++) {
		const bool eraseThisLine = erase && lineNum >= startEraseLine;
		_DrawLine(view, lineNum, startOffset, eraseThisLine, eraseRect,
			inputRegion);
		startOffset = -1;
			// Set this to -1 so the next iteration will use the line offset
	}

	// draw the caret/hilite the selection
	if (fActive) {
		if (fSelStart != fSelEnd) {
			if (fSelectable)
				Highlight(fSelStart, fSelEnd);
		} else {
			if (fCaretVisible)
				_DrawCaret(fSelStart, true);
		}
	}

	if (fOffscreen != NULL) {
		view->Sync();
		/*BPoint penLocation = view->PenLocation();
		BRect drawRect(leftTop.x, leftTop.y, penLocation.x, penLocation.y);
		DrawBitmap(fOffscreen, drawRect, drawRect);*/
		fOffscreen->Unlock();
	}

	ConstrainClippingRegion(NULL);
}


void
BTextView::_RequestDrawLines(int32 startLine, int32 endLine)
{
	if (!Window())
		return;

	long maxLine = fLines->NumLines() - 1;

	STELine* from = (*fLines)[startLine];
	STELine* to = endLine == maxLine ? NULL : (*fLines)[endLine + 1];
	BRect invalidRect(Bounds().left, from->origin + fTextRect.top,
		Bounds().right,
		to != NULL ? to->origin + fTextRect.top : fTextRect.bottom);
	Invalidate(invalidRect);
}


void
BTextView::_DrawCaret(int32 offset, bool visible)
{
	float lineHeight;
	BPoint caretPoint = PointAt(offset, &lineHeight);
	caretPoint.x = std::min(caretPoint.x, fTextRect.right);

	BRect caretRect;
	caretRect.left = caretRect.right = caretPoint.x;
	caretRect.top = caretPoint.y;
	caretRect.bottom = caretPoint.y + lineHeight - 1;

	if (visible)
		InvertRect(caretRect);
	else
		Invalidate(caretRect);
}


inline void
BTextView::_ShowCaret()
{
	if (fActive && !fCaretVisible && fEditable && fSelStart == fSelEnd)
		_InvertCaret();
}


inline void
BTextView::_HideCaret()
{
	if (fCaretVisible && fSelStart == fSelEnd)
		_InvertCaret();
}


//!	Hides the caret if it is being shown, and if it's hidden, shows it.
void
BTextView::_InvertCaret()
{
	fCaretVisible = !fCaretVisible;
	_DrawCaret(fSelStart, fCaretVisible);
	fCaretTime = system_time();
}


/*!	Place the dragging caret at the given offset.

	\param offset The offset (zero based within the object's text) where to
	       place the dragging caret. If it's -1, hide the caret.
*/
void
BTextView::_DragCaret(int32 offset)
{
	// does the caret need to move?
	if (offset == fDragOffset)
		return;

	// hide the previous drag caret
	if (fDragOffset != -1)
		_DrawCaret(fDragOffset, false);

	// do we have a new location?
	if (offset != -1) {
		if (fActive) {
			// ignore if offset is within active selection
			if (offset >= fSelStart && offset <= fSelEnd) {
				fDragOffset = -1;
				return;
			}
		}

		_DrawCaret(offset, true);
	}

	fDragOffset = offset;
}


void
BTextView::_StopMouseTracking()
{
	delete fTrackingMouse;
	fTrackingMouse = NULL;
}


bool
BTextView::_PerformMouseUp(BPoint where)
{
	if (fTrackingMouse == NULL)
		return false;

	if (fTrackingMouse->selectionRect.IsValid())
		Select(fTrackingMouse->clickOffset, fTrackingMouse->clickOffset);

	_StopMouseTracking();
	// adjust cursor if necessary
	_TrackMouse(where, NULL, true);

	return true;
}


bool
BTextView::_PerformMouseMoved(BPoint where, uint32 code)
{
	fWhere = where;

	if (fTrackingMouse == NULL)
		return false;

	int32 currentOffset = OffsetAt(where);
	if (fTrackingMouse->selectionRect.IsValid()) {
		// we are tracking the mouse for drag action, if the mouse has moved
		// to another index or more than three pixels from where it was clicked,
		// we initiate a drag now:
		if (currentOffset != fTrackingMouse->clickOffset
			|| fabs(fTrackingMouse->where.x - where.x) > 3
			|| fabs(fTrackingMouse->where.y - where.y) > 3) {
			_StopMouseTracking();
			_InitiateDrag();
			return true;
		}
		return false;
	}

	switch (fClickCount) {
		case 3:
			// triple click, extend selection linewise
			if (currentOffset <= fTrackingMouse->anchor) {
				fTrackingMouse->selStart
					= (*fLines)[_LineAt(currentOffset)]->offset;
				fTrackingMouse->selEnd = fTrackingMouse->shiftDown
					? fSelEnd
					: (*fLines)[_LineAt(fTrackingMouse->anchor) + 1]->offset;
			} else {
				fTrackingMouse->selStart
					= fTrackingMouse->shiftDown
						? fSelStart
						: (*fLines)[_LineAt(fTrackingMouse->anchor)]->offset;
				fTrackingMouse->selEnd
					= (*fLines)[_LineAt(currentOffset) + 1]->offset;
			}
			break;

		case 2:
			// double click, extend selection wordwise
			if (currentOffset <= fTrackingMouse->anchor) {
				fTrackingMouse->selStart = _PreviousWordBoundary(currentOffset);
				fTrackingMouse->selEnd
					= fTrackingMouse->shiftDown
						? fSelEnd
						: _NextWordBoundary(fTrackingMouse->anchor);
			} else {
				fTrackingMouse->selStart
					= fTrackingMouse->shiftDown
						? fSelStart
						: _PreviousWordBoundary(fTrackingMouse->anchor);
				fTrackingMouse->selEnd = _NextWordBoundary(currentOffset);
			}
			break;

		default:
			// new click, extend selection char by char
			if (currentOffset <= fTrackingMouse->anchor) {
				fTrackingMouse->selStart = currentOffset;
				fTrackingMouse->selEnd
					= fTrackingMouse->shiftDown
						? fSelEnd : fTrackingMouse->anchor;
			} else {
				fTrackingMouse->selStart
					= fTrackingMouse->shiftDown
						? fSelStart : fTrackingMouse->anchor;
				fTrackingMouse->selEnd = currentOffset;
			}
			break;
	}

	// position caret to follow the direction of the selection
	if (fTrackingMouse->selEnd != fSelEnd)
		fCaretOffset = fTrackingMouse->selEnd;
	else if (fTrackingMouse->selStart != fSelStart)
		fCaretOffset = fTrackingMouse->selStart;

	Select(fTrackingMouse->selStart, fTrackingMouse->selEnd);
	_TrackMouse(where, NULL);

	return true;
}


/*!	Tracks the mouse position, doing special actions like changing the
	view cursor.

	\param where The point where the mouse has moved.
	\param message The dragging message, if there is any.
	\param force Passed as second parameter of SetViewCursor()
*/
void
BTextView::_TrackMouse(BPoint where, const BMessage* message, bool force)
{
	BRegion textRegion;
	GetTextRegion(fSelStart, fSelEnd, &textRegion);

	if (message && AcceptsDrop(message))
		_TrackDrag(where);
	else if ((fSelectable || fEditable)
		&& (fTrackingMouse != NULL || !textRegion.Contains(where))) {
		SetViewCursor(B_CURSOR_I_BEAM, force);
	} else
		SetViewCursor(B_CURSOR_SYSTEM_DEFAULT, force);
}


//!	Tracks the mouse position when the user is dragging some data.
void
BTextView::_TrackDrag(BPoint where)
{
	CALLED();
	if (Bounds().Contains(where))
		_DragCaret(OffsetAt(where));
}


//!	Initiates a drag operation.
void
BTextView::_InitiateDrag()
{
	BMessage dragMessage(B_MIME_DATA);
	BBitmap* dragBitmap = NULL;
	BPoint bitmapPoint;
	BHandler* dragHandler = NULL;

	GetDragParameters(&dragMessage, &dragBitmap, &bitmapPoint, &dragHandler);
	SetViewCursor(B_CURSOR_SYSTEM_DEFAULT);

	if (dragBitmap != NULL)
		DragMessage(&dragMessage, dragBitmap, bitmapPoint, dragHandler);
	else {
		BRegion region;
		GetTextRegion(fSelStart, fSelEnd, &region);
		BRect bounds = Bounds();
		BRect dragRect = region.Frame();
		if (!bounds.Contains(dragRect))
			dragRect = bounds & dragRect;

		DragMessage(&dragMessage, dragRect, dragHandler);
	}

	BMessenger messenger(this);
	BMessage message(_DISPOSE_DRAG_);
	fDragRunner = new (nothrow) BMessageRunner(messenger, &message, 100000);
}


//!	Handles when some data is dropped on the view.
bool
BTextView::_MessageDropped(BMessage* message, BPoint where, BPoint offset)
{
	ASSERT(message);

	void* from = NULL;
	bool internalDrop = false;
	if (message->FindPointer("be:originator", &from) == B_OK
			&& from == this && fSelEnd != fSelStart)
		internalDrop = true;

	_DragCaret(-1);

	delete fDragRunner;
	fDragRunner = NULL;

	_TrackMouse(where, NULL);

	// are we sure we like this message?
	if (!AcceptsDrop(message))
		return false;

	int32 dropOffset = OffsetAt(where);
	if (dropOffset > fText->Length())
		dropOffset = fText->Length();

	// if this view initiated the drag, move instead of copy
	if (internalDrop) {
		// dropping onto itself?
		if (dropOffset >= fSelStart && dropOffset <= fSelEnd)
			return true;
	}

	ssize_t dataLength = 0;
	const char* text = NULL;
	entry_ref ref;
	if (message->FindData("text/plain", B_MIME_TYPE, (const void**)&text,
			&dataLength) == B_OK) {
		text_run_array* runArray = NULL;
		ssize_t runLength = 0;
		if (fStylable) {
			message->FindData("application/x-vnd.Be-text_run_array",
				B_MIME_TYPE, (const void**)&runArray, &runLength);
		}

		_FilterDisallowedChars((char*)text, dataLength, runArray);

		if (dataLength < 1) {
			beep();
			return true;
		}

		if (fUndo) {
			delete fUndo;
			fUndo = new DropUndoBuffer(this, text, dataLength, runArray,
				runLength, dropOffset, internalDrop);
		}

		if (internalDrop) {
			if (dropOffset > fSelEnd)
				dropOffset -= dataLength;
			Delete();
		}

		Insert(dropOffset, text, dataLength, runArray);
		if (IsFocus())
			Select(dropOffset, dropOffset + dataLength);
	}

	return true;
}


void
BTextView::_PerformAutoScrolling()
{
	// Scroll the view a bit if mouse is outside the view bounds
	BRect bounds = Bounds();
	BPoint scrollBy(B_ORIGIN);

	// R5 does a pretty soft auto-scroll, we try to do the same by
	// simply scrolling the distance between cursor and border
	if (fWhere.x > bounds.right)
		scrollBy.x = fWhere.x - bounds.right;
	else if (fWhere.x < bounds.left)
		scrollBy.x = fWhere.x - bounds.left; // negative value

	// prevent horizontal scrolling if text rect is inside view rect
	if (fTextRect.left > bounds.left && fTextRect.right < bounds.right)
		scrollBy.x = 0;

	if (CountLines() > 1) {
		// scroll in Y only if multiple lines!
		if (fWhere.y > bounds.bottom)
			scrollBy.y = fWhere.y - bounds.bottom;
		else if (fWhere.y < bounds.top)
			scrollBy.y = fWhere.y - bounds.top; // negative value
	}

	// prevent vertical scrolling if text rect is inside view rect
	if (fTextRect.top > bounds.top && fTextRect.bottom < bounds.bottom)
		scrollBy.y = 0;

	if (scrollBy != B_ORIGIN)
		_ScrollBy(scrollBy.x, scrollBy.y);
}


//!	Updates the scrollbars associated with the object (if any).
void
BTextView::_UpdateScrollbars()
{
	BRect bounds(Bounds());
	BScrollBar* horizontalScrollBar = ScrollBar(B_HORIZONTAL);
 	BScrollBar* verticalScrollBar = ScrollBar(B_VERTICAL);

	// do we have a horizontal scroll bar?
	if (horizontalScrollBar != NULL) {
		long viewWidth = bounds.IntegerWidth();
		long dataWidth = (long)ceilf(_TextWidth());

		long maxRange = dataWidth - viewWidth;
		maxRange = std::max(maxRange, 0l);

		horizontalScrollBar->SetRange(0, (float)maxRange);
		horizontalScrollBar->SetProportion((float)viewWidth
			/ (float)dataWidth);
		horizontalScrollBar->SetSteps(kHorizontalScrollBarStep,
			dataWidth / 10);
	}

	// how about a vertical scroll bar?
	if (verticalScrollBar != NULL) {
		long viewHeight = bounds.IntegerHeight();
		long dataHeight = (long)ceilf(_TextHeight());

		long maxRange = dataHeight - viewHeight;
		maxRange = std::max(maxRange, 0l);

		verticalScrollBar->SetRange(0, maxRange);
		verticalScrollBar->SetProportion((float)viewHeight
			/ (float)dataHeight);
		verticalScrollBar->SetSteps(kVerticalScrollBarStep,
			viewHeight);
	}
}


//!	Scrolls by the given offsets
void
BTextView::_ScrollBy(float horizontal, float vertical)
{
	BRect bounds = Bounds();
	_ScrollTo(bounds.left + horizontal, bounds.top + vertical);
}


//!	Scrolls to the given position, making sure not to scroll out of bounds.
void
BTextView::_ScrollTo(float x, float y)
{
	BRect bounds = Bounds();
	long viewWidth = bounds.IntegerWidth();
	long viewHeight = bounds.IntegerHeight();

	float minWidth = fTextRect.left - fLayoutData->leftInset;
	float maxWidth = fTextRect.right + fLayoutData->rightInset - viewWidth;
	float minHeight = fTextRect.top - fLayoutData->topInset;
	float maxHeight = fTextRect.bottom + fLayoutData->bottomInset - viewHeight;

	// set horizontal scroll limits
	if (x > maxWidth)
		x = maxWidth;
	if (x < minWidth)
		x = minWidth;

	// set vertical scroll limits
	if (y > maxHeight)
		y = maxHeight;
	if (y < minHeight)
		y = minHeight;

	ScrollTo(x, y);
}


//!	Autoresizes the view to fit the contained text.
void
BTextView::_AutoResize(bool redraw)
{
	if (!fResizable || fContainerView == NULL)
		return;

	// NOTE: This container view thing is only used by Tracker.
	// move container view if not left aligned
	float oldWidth = Bounds().Width();
	float newWidth = _TextWidth();
	float right = oldWidth - newWidth;

	if (fAlignment == B_ALIGN_CENTER)
		fContainerView->MoveBy(roundf(right / 2), 0);
	else if (fAlignment == B_ALIGN_RIGHT)
		fContainerView->MoveBy(right, 0);

	// resize container view
	float grow = newWidth - oldWidth;
	fContainerView->ResizeBy(grow, 0);

	// reposition text view
	fTextRect.OffsetTo(fLayoutData->leftInset, fLayoutData->topInset);

	// scroll rect to start, there is room for full text
	ScrollToOffset(0);

	if (redraw)
		_RequestDrawLines(0, 0);
}


//!	Creates a new offscreen BBitmap with an associated BView.
void
BTextView::_NewOffscreen(float padding)
{
	if (fOffscreen != NULL)
		_DeleteOffscreen();

#if USE_DOUBLEBUFFERING
	BRect bitmapRect(0, 0, fTextRect.Width() + padding, fTextRect.Height());
	fOffscreen = new BBitmap(bitmapRect, fColorSpace, true, false);
	if (fOffscreen != NULL && fOffscreen->Lock()) {
		BView* bufferView = new BView(bitmapRect, "drawing view", 0, 0);
		fOffscreen->AddChild(bufferView);
		fOffscreen->Unlock();
	}
#endif
}


//!	Deletes the textview's offscreen bitmap, if any.
void
BTextView::_DeleteOffscreen()
{
	if (fOffscreen != NULL && fOffscreen->Lock()) {
		delete fOffscreen;
		fOffscreen = NULL;
	}
}


/*!	Creates a new offscreen bitmap, highlight the selection, and set the
	cursor to \c B_CURSOR_I_BEAM.
*/
void
BTextView::_Activate()
{
	fActive = true;

	// Create a new offscreen BBitmap
	_NewOffscreen();

	if (fSelStart != fSelEnd) {
		if (fSelectable)
			Highlight(fSelStart, fSelEnd);
	} else
		_ShowCaret();

	BPoint where;
	uint32 buttons;
	GetMouse(&where, &buttons, false);
	if (Bounds().Contains(where))
		_TrackMouse(where, NULL);

	if (Window() != NULL) {
		BMessage* message;

		if (!Window()->HasShortcut(B_LEFT_ARROW, B_COMMAND_KEY)
			&& !Window()->HasShortcut(B_RIGHT_ARROW, B_COMMAND_KEY)) {
			message = new BMessage(kMsgNavigateArrow);
			message->AddInt32("key", B_LEFT_ARROW);
			message->AddInt32("modifiers", B_COMMAND_KEY);
			Window()->AddShortcut(B_LEFT_ARROW, B_COMMAND_KEY, message, this);

			message = new BMessage(kMsgNavigateArrow);
			message->AddInt32("key", B_RIGHT_ARROW);
			message->AddInt32("modifiers", B_COMMAND_KEY);
			Window()->AddShortcut(B_RIGHT_ARROW, B_COMMAND_KEY, message, this);

			fInstalledNavigateCommandWordwiseShortcuts = true;
		}
		if (!Window()->HasShortcut(B_LEFT_ARROW, B_COMMAND_KEY | B_SHIFT_KEY)
			&& !Window()->HasShortcut(B_RIGHT_ARROW,
				B_COMMAND_KEY | B_SHIFT_KEY)) {
			message = new BMessage(kMsgNavigateArrow);
			message->AddInt32("key", B_LEFT_ARROW);
			message->AddInt32("modifiers", B_COMMAND_KEY | B_SHIFT_KEY);
			Window()->AddShortcut(B_LEFT_ARROW, B_COMMAND_KEY | B_SHIFT_KEY,
				message, this);

			message = new BMessage(kMsgNavigateArrow);
			message->AddInt32("key", B_RIGHT_ARROW);
			message->AddInt32("modifiers", B_COMMAND_KEY | B_SHIFT_KEY);
			Window()->AddShortcut(B_RIGHT_ARROW, B_COMMAND_KEY | B_SHIFT_KEY,
				message, this);

			fInstalledSelectCommandWordwiseShortcuts = true;
		}
		if (!Window()->HasShortcut(B_DELETE, B_COMMAND_KEY)
			&& !Window()->HasShortcut(B_BACKSPACE, B_COMMAND_KEY)) {
			message = new BMessage(kMsgRemoveWord);
			message->AddInt32("key", B_DELETE);
			message->AddInt32("modifiers", B_COMMAND_KEY);
			Window()->AddShortcut(B_DELETE, B_COMMAND_KEY, message, this);

			message = new BMessage(kMsgRemoveWord);
			message->AddInt32("key", B_BACKSPACE);
			message->AddInt32("modifiers", B_COMMAND_KEY);
			Window()->AddShortcut(B_BACKSPACE, B_COMMAND_KEY, message, this);

			fInstalledRemoveCommandWordwiseShortcuts = true;
		}

		if (!Window()->HasShortcut(B_LEFT_ARROW, B_OPTION_KEY)
			&& !Window()->HasShortcut(B_RIGHT_ARROW, B_OPTION_KEY)) {
			message = new BMessage(kMsgNavigateArrow);
			message->AddInt32("key", B_LEFT_ARROW);
			message->AddInt32("modifiers", B_OPTION_KEY);
			Window()->AddShortcut(B_LEFT_ARROW, B_OPTION_KEY, message, this);

			message = new BMessage(kMsgNavigateArrow);
			message->AddInt32("key", B_RIGHT_ARROW);
			message->AddInt32("modifiers", B_OPTION_KEY);
			Window()->AddShortcut(B_RIGHT_ARROW, B_OPTION_KEY, message, this);

			fInstalledNavigateOptionWordwiseShortcuts = true;
		}
		if (!Window()->HasShortcut(B_LEFT_ARROW, B_OPTION_KEY | B_SHIFT_KEY)
			&& !Window()->HasShortcut(B_RIGHT_ARROW,
				B_OPTION_KEY | B_SHIFT_KEY)) {
			message = new BMessage(kMsgNavigateArrow);
			message->AddInt32("key", B_LEFT_ARROW);
			message->AddInt32("modifiers", B_OPTION_KEY | B_SHIFT_KEY);
			Window()->AddShortcut(B_LEFT_ARROW, B_OPTION_KEY | B_SHIFT_KEY,
				message, this);

			message = new BMessage(kMsgNavigateArrow);
			message->AddInt32("key", B_RIGHT_ARROW);
			message->AddInt32("modifiers", B_OPTION_KEY | B_SHIFT_KEY);
			Window()->AddShortcut(B_RIGHT_ARROW, B_OPTION_KEY | B_SHIFT_KEY,
				message, this);

			fInstalledSelectOptionWordwiseShortcuts = true;
		}
		if (!Window()->HasShortcut(B_DELETE, B_OPTION_KEY)
			&& !Window()->HasShortcut(B_BACKSPACE, B_OPTION_KEY)) {
			message = new BMessage(kMsgRemoveWord);
			message->AddInt32("key", B_DELETE);
			message->AddInt32("modifiers", B_OPTION_KEY);
			Window()->AddShortcut(B_DELETE, B_OPTION_KEY, message, this);

			message = new BMessage(kMsgRemoveWord);
			message->AddInt32("key", B_BACKSPACE);
			message->AddInt32("modifiers", B_OPTION_KEY);
			Window()->AddShortcut(B_BACKSPACE, B_OPTION_KEY, message, this);

			fInstalledRemoveOptionWordwiseShortcuts = true;
		}

		if (!Window()->HasShortcut(B_UP_ARROW, B_OPTION_KEY)
			&& !Window()->HasShortcut(B_DOWN_ARROW, B_OPTION_KEY)) {
			message = new BMessage(kMsgNavigateArrow);
			message->AddInt32("key", B_UP_ARROW);
			message->AddInt32("modifiers", B_OPTION_KEY);
			Window()->AddShortcut(B_UP_ARROW, B_OPTION_KEY, message, this);

			message = new BMessage(kMsgNavigateArrow);
			message->AddInt32("key", B_DOWN_ARROW);
			message->AddInt32("modifiers", B_OPTION_KEY);
			Window()->AddShortcut(B_DOWN_ARROW, B_OPTION_KEY, message, this);

			fInstalledNavigateOptionLinewiseShortcuts = true;
		}
		if (!Window()->HasShortcut(B_UP_ARROW, B_OPTION_KEY | B_SHIFT_KEY)
			&& !Window()->HasShortcut(B_DOWN_ARROW,
				B_OPTION_KEY | B_SHIFT_KEY)) {
			message = new BMessage(kMsgNavigateArrow);
			message->AddInt32("key", B_UP_ARROW);
			message->AddInt32("modifiers", B_OPTION_KEY | B_SHIFT_KEY);
			Window()->AddShortcut(B_UP_ARROW, B_OPTION_KEY | B_SHIFT_KEY,
				message, this);

			message = new BMessage(kMsgNavigateArrow);
			message->AddInt32("key", B_DOWN_ARROW);
			message->AddInt32("modifiers", B_OPTION_KEY | B_SHIFT_KEY);
			Window()->AddShortcut(B_DOWN_ARROW, B_OPTION_KEY | B_SHIFT_KEY,
				message, this);

			fInstalledSelectOptionLinewiseShortcuts = true;
		}

		if (!Window()->HasShortcut(B_HOME, B_COMMAND_KEY)
			&& !Window()->HasShortcut(B_END, B_COMMAND_KEY)) {
			message = new BMessage(kMsgNavigatePage);
			message->AddInt32("key", B_HOME);
			message->AddInt32("modifiers", B_COMMAND_KEY);
			Window()->AddShortcut(B_HOME, B_COMMAND_KEY, message, this);

			message = new BMessage(kMsgNavigatePage);
			message->AddInt32("key", B_END);
			message->AddInt32("modifiers", B_COMMAND_KEY);
			Window()->AddShortcut(B_END, B_COMMAND_KEY, message, this);

			fInstalledNavigateHomeEndDocwiseShortcuts = true;
		}
		if (!Window()->HasShortcut(B_HOME, B_COMMAND_KEY | B_SHIFT_KEY)
			&& !Window()->HasShortcut(B_END, B_COMMAND_KEY | B_SHIFT_KEY)) {
			message = new BMessage(kMsgNavigatePage);
			message->AddInt32("key", B_HOME);
			message->AddInt32("modifiers", B_COMMAND_KEY | B_SHIFT_KEY);
			Window()->AddShortcut(B_HOME, B_COMMAND_KEY | B_SHIFT_KEY,
				message, this);

			message = new BMessage(kMsgNavigatePage);
			message->AddInt32("key", B_END);
			message->AddInt32("modifiers", B_COMMAND_KEY | B_SHIFT_KEY);
			Window()->AddShortcut(B_END, B_COMMAND_KEY | B_SHIFT_KEY,
				message, this);

			fInstalledSelectHomeEndDocwiseShortcuts = true;
		}
	}
}


//!	Unhilights the selection, set the cursor to \c B_CURSOR_SYSTEM_DEFAULT.
void
BTextView::_Deactivate()
{
	fActive = false;

	_CancelInputMethod();
	_DeleteOffscreen();

	if (fSelStart != fSelEnd) {
		if (fSelectable)
			Highlight(fSelStart, fSelEnd);
	} else
		_HideCaret();

	if (Window() != NULL) {
		if (fInstalledNavigateCommandWordwiseShortcuts) {
			Window()->RemoveShortcut(B_LEFT_ARROW, B_COMMAND_KEY);
			Window()->RemoveShortcut(B_RIGHT_ARROW, B_COMMAND_KEY);
			fInstalledNavigateCommandWordwiseShortcuts = false;
		}
		if (fInstalledSelectCommandWordwiseShortcuts) {
			Window()->RemoveShortcut(B_LEFT_ARROW, B_COMMAND_KEY | B_SHIFT_KEY);
			Window()->RemoveShortcut(B_RIGHT_ARROW,
				B_COMMAND_KEY | B_SHIFT_KEY);
			fInstalledSelectCommandWordwiseShortcuts = false;
		}
		if (fInstalledRemoveCommandWordwiseShortcuts) {
			Window()->RemoveShortcut(B_DELETE, B_COMMAND_KEY);
			Window()->RemoveShortcut(B_BACKSPACE, B_COMMAND_KEY);
			fInstalledRemoveCommandWordwiseShortcuts = false;
		}

		if (fInstalledNavigateOptionWordwiseShortcuts) {
			Window()->RemoveShortcut(B_LEFT_ARROW, B_OPTION_KEY);
			Window()->RemoveShortcut(B_RIGHT_ARROW, B_OPTION_KEY);
			fInstalledNavigateOptionWordwiseShortcuts = false;
		}
		if (fInstalledSelectOptionWordwiseShortcuts) {
			Window()->RemoveShortcut(B_LEFT_ARROW, B_OPTION_KEY | B_SHIFT_KEY);
			Window()->RemoveShortcut(B_RIGHT_ARROW, B_OPTION_KEY | B_SHIFT_KEY);
			fInstalledSelectOptionWordwiseShortcuts = false;
		}
		if (fInstalledRemoveOptionWordwiseShortcuts) {
			Window()->RemoveShortcut(B_DELETE, B_OPTION_KEY);
			Window()->RemoveShortcut(B_BACKSPACE, B_OPTION_KEY);
			fInstalledRemoveOptionWordwiseShortcuts = false;
		}

		if (fInstalledNavigateOptionLinewiseShortcuts) {
			Window()->RemoveShortcut(B_UP_ARROW, B_OPTION_KEY);
			Window()->RemoveShortcut(B_DOWN_ARROW, B_OPTION_KEY);
			fInstalledNavigateOptionLinewiseShortcuts = false;
		}
		if (fInstalledSelectOptionLinewiseShortcuts) {
			Window()->RemoveShortcut(B_UP_ARROW, B_OPTION_KEY | B_SHIFT_KEY);
			Window()->RemoveShortcut(B_DOWN_ARROW, B_OPTION_KEY | B_SHIFT_KEY);
			fInstalledSelectOptionLinewiseShortcuts = false;
		}

		if (fInstalledNavigateHomeEndDocwiseShortcuts) {
			Window()->RemoveShortcut(B_HOME, B_COMMAND_KEY);
			Window()->RemoveShortcut(B_END, B_COMMAND_KEY);
			fInstalledNavigateHomeEndDocwiseShortcuts = false;
		}
		if (fInstalledSelectHomeEndDocwiseShortcuts) {
			Window()->RemoveShortcut(B_HOME, B_COMMAND_KEY | B_SHIFT_KEY);
			Window()->RemoveShortcut(B_END, B_COMMAND_KEY | B_SHIFT_KEY);
			fInstalledSelectHomeEndDocwiseShortcuts = false;
		}
	}
}


/*!	Changes the passed in font to be displayable by the object.

	Set font rotation to 0, removes any font flag, set font spacing
	to \c B_BITMAP_SPACING and font encoding to \c B_UNICODE_UTF8.
*/
void
BTextView::_NormalizeFont(BFont* font)
{
	if (font) {
		font->SetRotation(0.0f);
		font->SetFlags(0);
		font->SetSpacing(B_BITMAP_SPACING);
		font->SetEncoding(B_UNICODE_UTF8);
	}
}


void
BTextView::_SetRunArray(int32 startOffset, int32 endOffset,
	const text_run_array* runs)
{
	const int32 numStyles = runs->count;
	if (numStyles > 0) {
		const text_run* theRun = &runs->runs[0];
		for (int32 index = 0; index < numStyles; index++) {
			int32 fromOffset = theRun->offset + startOffset;
			int32 toOffset = endOffset;
			if (index + 1 < numStyles) {
				toOffset = (theRun + 1)->offset + startOffset;
				toOffset = (toOffset > endOffset) ? endOffset : toOffset;
			}

			_ApplyStyleRange(fromOffset, toOffset, B_FONT_ALL, &theRun->font,
				&theRun->color, false);

			theRun++;
		}
		fStyles->InvalidateNullStyle();
	}
}


/*!	Returns the character class of the character at the given offset.

	\param offset The offset where the wanted character can be found.

	\return A value which represents the character's classification.
*/
uint32
BTextView::_CharClassification(int32 offset) const
{
	// TODO: Should check against a list of characters containing also
	// japanese word breakers.
	// And what about other languages ? Isn't there a better way to check
	// for separator characters ?
	// Andrew suggested to have a look at UnicodeBlockObject.h
	switch (fText->RealCharAt(offset)) {
		case '\0':
			return CHAR_CLASS_END_OF_TEXT;

		case B_SPACE:
		case B_TAB:
		case B_ENTER:
			return CHAR_CLASS_WHITESPACE;

		case '=':
		case '+':
		case '@':
		case '#':
		case '$':
		case '%':
		case '^':
		case '&':
		case '*':
		case '\\':
		case '|':
		case '<':
		case '>':
		case '/':
		case '~':
			return CHAR_CLASS_GRAPHICAL;

		case '\'':
		case '"':
			return CHAR_CLASS_QUOTE;

		case ',':
		case '.':
		case '?':
		case '!':
		case ';':
		case ':':
		case '-':
			return CHAR_CLASS_PUNCTUATION;

		case '(':
		case '[':
		case '{':
			return CHAR_CLASS_PARENS_OPEN;

		case ')':
		case ']':
		case '}':
			return CHAR_CLASS_PARENS_CLOSE;

		default:
			return CHAR_CLASS_DEFAULT;
	}
}


/*!	Returns the offset of the next UTF-8 character.

	\param offset The offset where to start looking.

	\return The offset of the next UTF-8 character.
*/
int32
BTextView::_NextInitialByte(int32 offset) const
{
	if (offset >= fText->Length())
		return offset;

	for (++offset; (ByteAt(offset) & 0xC0) == 0x80; ++offset)
		;

	return offset;
}


/*!	Returns the offset of the previous UTF-8 character.

	\param offset The offset where to start looking.

	\return The offset of the previous UTF-8 character.
*/
int32
BTextView::_PreviousInitialByte(int32 offset) const
{
	if (offset <= 0)
		return 0;

	int32 count = 6;

	for (--offset; offset > 0 && count; --offset, --count) {
		if ((ByteAt(offset) & 0xC0) != 0x80)
			break;
	}

	return count ? offset : 0;
}


bool
BTextView::_GetProperty(BMessage* message, BMessage* specifier,
	const char* property, BMessage* reply)
{
	CALLED();
	if (strcmp(property, "selection") == 0) {
		reply->what = B_REPLY;
		reply->AddInt32("result", fSelStart);
		reply->AddInt32("result", fSelEnd);
		reply->AddInt32("error", B_OK);

		return true;
	} else if (strcmp(property, "Text") == 0) {
		if (IsTypingHidden()) {
			// Do not allow stealing passwords via scripting
			beep();
			return false;
		}

		int32 index, range;
		specifier->FindInt32("index", &index);
		specifier->FindInt32("range", &range);

		char* buffer = new char[range + 1];
		GetText(index, range, buffer);

		reply->what = B_REPLY;
		reply->AddString("result", buffer);
		reply->AddInt32("error", B_OK);

		delete[] buffer;

		return true;
	} else if (strcmp(property, "text_run_array") == 0)
		return false;

	return false;
}


bool
BTextView::_SetProperty(BMessage* message, BMessage* specifier,
	const char* property, BMessage* reply)
{
	CALLED();
	if (strcmp(property, "selection") == 0) {
		int32 index, range;

		specifier->FindInt32("index", &index);
		specifier->FindInt32("range", &range);

		Select(index, index + range);

		reply->what = B_REPLY;
		reply->AddInt32("error", B_OK);

		return true;
	} else if (strcmp(property, "Text") == 0) {
		int32 index, range;
		specifier->FindInt32("index", &index);
		specifier->FindInt32("range", &range);

		const char* buffer = NULL;
		if (message->FindString("data", &buffer) == B_OK) {
			InsertText(buffer, range, index, NULL);
			_Refresh(index, index + range);
		} else {
			DeleteText(index, index + range);
			_Refresh(index, index);
		}

		reply->what = B_REPLY;
		reply->AddInt32("error", B_OK);

		return true;
	} else if (strcmp(property, "text_run_array") == 0)
		return false;

	return false;
}


bool
BTextView::_CountProperties(BMessage* message, BMessage* specifier,
	const char* property, BMessage* reply)
{
	CALLED();
	if (strcmp(property, "Text") == 0) {
		reply->what = B_REPLY;
		reply->AddInt32("result", fText->Length());
		reply->AddInt32("error", B_OK);
		return true;
	}

	return false;
}


//!	Called when the object receives a \c B_INPUT_METHOD_CHANGED message.
void
BTextView::_HandleInputMethodChanged(BMessage* message)
{
	// TODO: block input if not editable (Andrew)
	ASSERT(fInline != NULL);

	const char* string = NULL;
	if (message->FindString("be:string", &string) < B_OK || string == NULL)
		return;

	_HideCaret();

	if (IsFocus())
		be_app->ObscureCursor();

	// If we find the "be:confirmed" boolean (and the boolean is true),
	// it means it's over for now, so the current InlineInput object
	// should become inactive. We will probably receive a
	// B_INPUT_METHOD_STOPPED message after this one.
	bool confirmed;
	if (message->FindBool("be:confirmed", &confirmed) != B_OK)
		confirmed = false;

	// Delete the previously inserted text (if any)
	if (fInline->IsActive()) {
		const int32 oldOffset = fInline->Offset();
		DeleteText(oldOffset, oldOffset + fInline->Length());
		if (confirmed)
			fInline->SetActive(false);
		fCaretOffset = fSelStart = fSelEnd = oldOffset;
	}

	const int32 stringLen = strlen(string);

	fInline->SetOffset(fSelStart);
	fInline->SetLength(stringLen);
	fInline->ResetClauses();

	if (!confirmed && !fInline->IsActive())
		fInline->SetActive(true);

	// Get the clauses, and pass them to the InlineInput object
	// TODO: Find out if what we did it's ok, currently we don't consider
	// clauses at all, while the bebook says we should; though the visual
	// effect we obtained seems correct. Weird.
	int32 clauseCount = 0;
	int32 clauseStart;
	int32 clauseEnd;
	while (message->FindInt32("be:clause_start", clauseCount, &clauseStart)
			== B_OK
		&& message->FindInt32("be:clause_end", clauseCount, &clauseEnd)
			== B_OK) {
		if (!fInline->AddClause(clauseStart, clauseEnd))
			break;
		clauseCount++;
	}

	if (confirmed) {
		_Refresh(fSelStart, fSelEnd, fSelEnd);
		_ShowCaret();

		// now we need to feed ourselves the individual characters as if the
		// user would have pressed them now - this lets KeyDown() pick out all
		// the special characters like B_BACKSPACE, cursor keys and the like:
		const char* currPos = string;
		const char* prevPos = currPos;
		while (*currPos != '\0') {
			if ((*currPos & 0xC0) == 0xC0) {
				// found the start of an UTF-8 char, we collect while it lasts
				++currPos;
				while ((*currPos & 0xC0) == 0x80)
					++currPos;
			} else if ((*currPos & 0xC0) == 0x80) {
				// illegal: character starts with utf-8 intermediate byte,
				// skip it
				prevPos = ++currPos;
			} else {
				// single byte character/code, just feed that
				++currPos;
			}
			KeyDown(prevPos, currPos - prevPos);
			prevPos = currPos;
		}

		_Refresh(fSelStart, fSelEnd, fSelEnd);
	} else {
		// temporarily show transient state of inline input
		int32 selectionStart = 0;
		int32 selectionEnd = 0;
		message->FindInt32("be:selection", 0, &selectionStart);
		message->FindInt32("be:selection", 1, &selectionEnd);

		fInline->SetSelectionOffset(selectionStart);
		fInline->SetSelectionLength(selectionEnd - selectionStart);

		const int32 inlineOffset = fInline->Offset();
		InsertText(string, stringLen, fSelStart, NULL);

		_Refresh(inlineOffset, fSelEnd, fSelEnd);
		_ShowCaret();
	}
}


/*!	Called when the object receives a \c B_INPUT_METHOD_LOCATION_REQUEST
	message.
*/
void
BTextView::_HandleInputMethodLocationRequest()
{
	ASSERT(fInline != NULL);

	int32 offset = fInline->Offset();
	const int32 limit = offset + fInline->Length();

	BMessage message(B_INPUT_METHOD_EVENT);
	message.AddInt32("be:opcode", B_INPUT_METHOD_LOCATION_REQUEST);

	// Add the location of the UTF8 characters
	while (offset < limit) {
		float height;
		BPoint where = PointAt(offset, &height);
		ConvertToScreen(&where);

		message.AddPoint("be:location_reply", where);
		message.AddFloat("be:height_reply", height);

		offset = _NextInitialByte(offset);
	}

	fInline->Method()->SendMessage(&message);
}


//!	Tells the Input Server method add-on to stop the current transaction.
void
BTextView::_CancelInputMethod()
{
	if (!fInline)
		return;

	InlineInput* inlineInput = fInline;
	fInline = NULL;

	if (inlineInput->IsActive() && Window()) {
		_Refresh(inlineInput->Offset(), fText->Length()
			- inlineInput->Offset());

		BMessage message(B_INPUT_METHOD_EVENT);
		message.AddInt32("be:opcode", B_INPUT_METHOD_STOPPED);
		inlineInput->Method()->SendMessage(&message);
	}

	delete inlineInput;
}


/*!	Returns the line number of the character at the given \a offset.

	\note This will never return the last line (use LineAt() if you
	      need to be correct about that.) N.B.

	\param offset The offset of the wanted character.

	\return The line number of the character at the given \a offset as an int32.
*/
int32
BTextView::_LineAt(int32 offset) const
{
	return fLines->OffsetToLine(offset);
}


/*!	Returns the line number that the given \a point is on.

	\note This will never return the last line (use LineAt() if you
	      need to be correct about that.) N.B.

	\param point The \a point the get the line number of.

	\return The line number of the given \a point as an int32.
*/
int32
BTextView::_LineAt(const BPoint& point) const
{
	return fLines->PixelToLine(point.y - fTextRect.top);
}


/*!	Returns whether or not the given \a offset is on the empty line at the end
	of the buffer.
*/
bool
BTextView::_IsOnEmptyLastLine(int32 offset) const
{
	return (offset == fText->Length() && offset > 0
		&& fText->RealCharAt(offset - 1) == B_ENTER);
}


void
BTextView::_ApplyStyleRange(int32 fromOffset, int32 toOffset, uint32 mode,
	const BFont* font, const rgb_color* color, bool syncNullStyle)
{
	BFont normalized;
		// Declared before the if so it stays allocated until the call to
		// SetStyleRange
	if (font != NULL) {
		// if a font has been given, normalize it
		normalized = *font;
		_NormalizeFont(&normalized);
		font = &normalized;
	}

	if (!fStylable) {
		// always apply font and color to full range for non-stylable textviews
		fromOffset = 0;
		toOffset = fText->Length();
	}

	if (syncNullStyle)
		fStyles->SyncNullStyle(fromOffset);

	fStyles->SetStyleRange(fromOffset, toOffset, fText->Length(), mode,
		font, color);
}


float
BTextView::_NullStyleHeight() const
{
	const BFont* font = NULL;
	fStyles->GetNullStyle(&font, NULL);

	font_height fontHeight;
	font->GetHeight(&fontHeight);
	return ceilf(fontHeight.ascent + fontHeight.descent + 1);
}


void
BTextView::_ShowContextMenu(BPoint where)
{
	bool isRedo;
	undo_state state = UndoState(&isRedo);
	bool isUndo = state != B_UNDO_UNAVAILABLE && !isRedo;

	int32 start;
	int32 finish;
	GetSelection(&start, &finish);

	bool canEdit = IsEditable();
	int32 length = fText->Length();

	BPopUpMenu* menu = new BPopUpMenu(B_EMPTY_STRING, false, false);

	BLayoutBuilder::Menu<>(menu)
		.AddItem(TRANSLATE("Undo"), B_UNDO/*, 'Z'*/)
			.SetEnabled(canEdit && isUndo)
		.AddItem(TRANSLATE("Redo"), B_UNDO/*, 'Z', B_SHIFT_KEY*/)
			.SetEnabled(canEdit && isRedo)
		.AddSeparator()
		.AddItem(TRANSLATE("Cut"), B_CUT, 'X')
			.SetEnabled(canEdit && start != finish)
		.AddItem(TRANSLATE("Copy"), B_COPY, 'C')
			.SetEnabled(start != finish)
		.AddItem(TRANSLATE("Paste"), B_PASTE, 'V')
			.SetEnabled(canEdit && be_clipboard->SystemCount() > 0)
		.AddSeparator()
		.AddItem(TRANSLATE("Select all"), B_SELECT_ALL, 'A')
			.SetEnabled(!(start == 0 && finish == length))
	;

	menu->SetTargetForItems(this);
	ConvertToScreen(&where);
	menu->Go(where, true, true,	true);
}


void
BTextView::_FilterDisallowedChars(char* text, ssize_t& length,
	text_run_array* runArray)
{
	if (!fDisallowedChars)
		return;

	if (fDisallowedChars->IsEmpty() || !text)
		return;

	ssize_t stringIndex = 0;
	if (runArray) {
		ssize_t remNext = 0;

		for (int i = 0; i < runArray->count; i++) {
			runArray->runs[i].offset -= remNext;
			while (stringIndex < runArray->runs[i].offset
				&& stringIndex < length) {
				if (fDisallowedChars->HasItem(
					reinterpret_cast<void*>(text[stringIndex]))) {
					memmove(text + stringIndex, text + stringIndex + 1,
						length - stringIndex - 1);
					length--;
					runArray->runs[i].offset--;
					remNext++;
				} else
					stringIndex++;
			}
		}
	}

	while (stringIndex < length) {
		if (fDisallowedChars->HasItem(
			reinterpret_cast<void*>(text[stringIndex]))) {
			memmove(text + stringIndex, text + stringIndex + 1,
				length - stringIndex - 1);
			length--;
		} else
			stringIndex++;
	}
}


void
BTextView::_UpdateInsets(const BRect& rect)
{
	// do not update insets if SetInsets() was called
	if (fLayoutData->overridden)
		return;

	const BRect& bounds = Bounds();

	// we disallow negative insets, as they would cause parts of the
	// text to be hidden
	fLayoutData->leftInset = rect.left >= bounds.left
		? rect.left - bounds.left : 0;
	fLayoutData->topInset = rect.top >= bounds.top
		? rect.top - bounds.top : 0;
	fLayoutData->rightInset = bounds.right >= rect.right
		? bounds.right - rect.right : 0;
	fLayoutData->bottomInset = bounds.bottom >= rect.bottom
		? bounds.bottom - rect.bottom : 0;

	// only add default insets if text rect is set to bounds
	if (rect == bounds && (fEditable || fSelectable)) {
		float hPadding = be_control_look->DefaultLabelSpacing();
		float hInset = floorf(hPadding / 2.0f);
		float vInset = 1;
		fLayoutData->leftInset += hInset;
		fLayoutData->topInset += vInset;
		fLayoutData->rightInset += hInset;
		fLayoutData->bottomInset += vInset;
	}
}


float
BTextView::_ViewWidth()
{
	return Bounds().Width()
		- fLayoutData->leftInset
		- fLayoutData->rightInset;
}


float
BTextView::_ViewHeight()
{
	return Bounds().Height()
		- fLayoutData->topInset
		- fLayoutData->bottomInset;
}


BRect
BTextView::_ViewRect()
{
	BRect rect(Bounds());
	rect.left += fLayoutData->leftInset;
	rect.top += fLayoutData->topInset;
	rect.right -= fLayoutData->rightInset;
	rect.bottom -= fLayoutData->bottomInset;

	return rect;
}


float
BTextView::_TextWidth()
{
	return fTextRect.Width()
		+ fLayoutData->leftInset
		+ fLayoutData->rightInset;
}


float
BTextView::_TextHeight()
{
	return fTextRect.Height()
		+ fLayoutData->topInset
		+ fLayoutData->bottomInset;
}


BRect
BTextView::_TextRect()
{
	BRect rect(fTextRect);
	rect.left -= fLayoutData->leftInset;
	rect.top -= fLayoutData->topInset;
	rect.right += fLayoutData->rightInset;
	rect.bottom += fLayoutData->bottomInset;

	return rect;
}


float
BTextView::_UneditableTint() const
{
	return ui_color(B_DOCUMENT_BACKGROUND_COLOR).IsLight() ? B_DARKEN_1_TINT : 0.853;
}


// #pragma mark - BTextView::TextTrackState


BTextView::TextTrackState::TextTrackState(BMessenger messenger)
	:
	clickOffset(0),
	shiftDown(false),
	anchor(0),
	selStart(0),
	selEnd(0),
	fRunner(NULL)
{
	BMessage message(_PING_);
	const bigtime_t scrollSpeed = 25 * 1000;	// 40 scroll steps per second
	fRunner = new (nothrow) BMessageRunner(messenger, &message, scrollSpeed);
}


BTextView::TextTrackState::~TextTrackState()
{
	delete fRunner;
}


void
BTextView::TextTrackState::SimulateMouseMovement(BTextView* textView)
{
	BPoint where;
	uint32 buttons;
	// When the mouse cursor is still and outside the textview,
	// no B_MOUSE_MOVED message are sent, obviously. But scrolling
	// has to work neverthless, so we "fake" a MouseMoved() call here.
	textView->GetMouse(&where, &buttons);
	textView->_PerformMouseMoved(where, B_INSIDE_VIEW);
}


// #pragma mark - Binary ABI compat


extern "C" void
B_IF_GCC_2(InvalidateLayout__9BTextViewb,  _ZN9BTextView16InvalidateLayoutEb)(
	BTextView* view, bool descendants)
{
	perform_data_layout_invalidated data;
	data.descendants = descendants;

	view->Perform(PERFORM_CODE_LAYOUT_INVALIDATED, &data);
}
