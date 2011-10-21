/*
 * Copyright 2001-2009, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Hiroshi Lockheimer (BTextView is based on his STEEngine)
 *		Marc Flerackers (mflerackers@androme.be)
 *		Stefano Ceccherini (stefano.ceccherini@gmail.com)
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Oliver Tappe <zooey@hirschkaefer.de>
 */

/*!	BTextView displays and manages styled text. */

// TODOs:
// - Finish documenting this class
// - Consider using BObjectList instead of BList
// 	 for disallowed characters (it would remove a lot of reinterpret_casts)
// - Check for correctness and possible optimizations the calls to _Refresh(),
// 	 to refresh only changed parts of text (currently we often redraw the whole
//   text)

// Known Bugs:
// - Double buffering doesn't work well (disabled by default)

#include <stdio.h>
#include <stdlib.h>
#include <new>

#include <Application.h>
#include <Beep.h>
#include <Bitmap.h>
#include <Clipboard.h>
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
#include <TextView.h>
#include <Window.h>

#include <binary_compatibility/Interface.h>

#include "InlineInput.h"
#include "LineBuffer.h"
#include "StyleBuffer.h"
#include "TextGapBuffer.h"
#include "UndoBuffer.h"
#include "WidthBuffer.h"

#include <Catalog.h>
#include <LocaleBackend.h>


using namespace std;
using BPrivate::gLocaleBackend;
using BPrivate::LocaleBackend;


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "TextView"


#define TRANSLATE(str) gLocaleBackend->GetString(B_TRANSLATE_MARK(str), \
		"TextView")

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
	float	shear;		/* typically 90.0 */
	uint16	face;		/* typically 0 */
	uint8	red;
	uint8	green;
	uint8	blue;
	uint8	alpha;		/* 255 == opaque */
	uint16	_reserved_;	/* 0 */
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
		  valid(false)
	{
	}

	void UpdateInsets(const BRect& bounds, const BRect& textRect)
	{
		// we disallow negative insets, as they would cause parts of the
		// text to be hidden
		leftInset = textRect.left >= bounds.left
			? textRect.left - bounds.left
			: 0;
		topInset = textRect.top >= bounds.top
			? textRect.top - bounds.top
			: 0;
		rightInset = bounds.right >= textRect.right
			? bounds.right - textRect.right
			: leftInset;
		bottomInset = bounds.bottom >= textRect.bottom
			? bounds.bottom - textRect.bottom
			: topInset;
	}

	float				leftInset;
	float				topInset;
	float				rightInset;
	float				bottomInset;

	BSize				min;
	BSize				preferred;
	bool				valid;
};


static const rgb_color kBlackColor = { 0, 0, 0, 255 };
static const rgb_color kBlueInputColor = { 152, 203, 255, 255 };
static const rgb_color kRedInputColor = { 255, 152, 152, 255 };

static const float kHorizontalScrollBarStep = 10.0;
static const float kVerticalScrollBarStep = 12.0;


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


/*! \brief Creates a BTextView object with the given attributes.
	\param frame The rect which will enclose the BTextView object.
	\param name The name of the object.
	\param textRect Determines the area of the text within the BTextView object.
	\param resizeMask The resizing mask for the BTextView, passed to the BView
		constructor.
	\param flags The flags for the BTextView, passed to the BView constructor.
*/
BTextView::BTextView(BRect frame, const char *name, BRect textRect,
		uint32 resizeMask, uint32 flags)
	: BView(frame, name, resizeMask,
		flags | B_FRAME_EVENTS | B_PULSE_NEEDED | B_INPUT_METHOD_AWARE)
{
	_InitObject(textRect, NULL, NULL);
}


/*! \brief Creates a BTextView object with the given attributes.
	\param frame The rect which will enclose the BTextView object.
	\param name The name of the object.
	\param textRect Determines the area of the text within the BTextView object.
	\param initialFont The BTextView will display its text using this font,
		unless otherwise specified.
	\param initialColor The BTextView will display its text using this color,
		unless otherwise specified.
	\param resizeMask The resizing mask for the BTextView, passed to the BView
		constructor.
	\param flags The flags for the BTextView, passed to the BView constructor.
*/
BTextView::BTextView(BRect frame, const char *name, BRect textRect,
		const BFont *initialFont, const rgb_color *initialColor,
		uint32 resizeMask, uint32 flags)
	: BView(frame, name, resizeMask,
		flags | B_FRAME_EVENTS | B_PULSE_NEEDED | B_INPUT_METHOD_AWARE)
{
	_InitObject(textRect, initialFont, initialColor);
}


/*! \brief Creates a BTextView object with the given attributes.
	\param name The name of the object.
	\param flags The flags for the BTextView, passed to the BView constructor.
*/
BTextView::BTextView(const char* name, uint32 flags)
	: BView(name, flags | B_FRAME_EVENTS | B_PULSE_NEEDED
		| B_INPUT_METHOD_AWARE)
{
	_InitObject(Bounds(), NULL, NULL);
}


/*! \brief Creates a BTextView object with the given attributes.
	\param name The name of the object.
	\param initialFont The BTextView will display its text using this font,
		unless otherwise specified.
	\param initialColor The BTextView will display its text using this color,
		unless otherwise specified.
	\param flags The flags for the BTextView, passed to the BView constructor.
*/
BTextView::BTextView(const char* name, const BFont* initialFont,
	const rgb_color* initialColor, uint32 flags)
	: BView(name, flags | B_FRAME_EVENTS | B_PULSE_NEEDED
		| B_INPUT_METHOD_AWARE)
{
	_InitObject(Bounds(), initialFont, initialColor);
}


/*! \brief Creates a BTextView object from the passed BMessage.
	\param archive The BMessage from which the object shall be created.
*/
BTextView::BTextView(BMessage *archive)
	: BView(archive)
{
	CALLED();
	BRect rect;

	if (archive->FindRect("_trect", &rect) != B_OK)
		rect.Set(0, 0, 0, 0);

	_InitObject(rect, NULL, NULL);

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
		for (int32 x = 0; x < disallowedCount; x++) {
			fDisallowedChars->AddItem(
				reinterpret_cast<void *>(disallowedChars[x]));
		}
	}

	ssize_t runSize = 0;
	const void *flattenedRun = NULL;

	if (archive->FindData("_runs", B_RAW_TYPE, &flattenedRun, &runSize)
			== B_OK) {
		text_run_array *runArray = UnflattenRunArray(flattenedRun,
			(int32*)&runSize);
		if (runArray) {
			SetRunArray(0, TextLength(), runArray);
			FreeRunArray(runArray);
		}
	}

}


/*! \brief Frees the memory allocated and destroy the object created on
	construction.
*/
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
	delete fClickRunner;
	delete fDragRunner;
	delete fLayoutData;
}


/*! \brief Static function used to istantiate a BTextView object from the given
		BMessage.
	\param archive The BMessage from which the object shall be created.
	\return A constructed BTextView as a BArchivable object.
*/
BArchivable *
BTextView::Instantiate(BMessage *archive)
{
	CALLED();
	if (validate_instantiation(archive, "BTextView"))
		return new BTextView(archive);
	return NULL;
}


/*! \brief Archives the object into the passed message.
	\param data A pointer to the message where to archive the object.
	\param deep ?
	\return \c B_OK if everything went well, an error code if not.
*/
status_t
BTextView::Archive(BMessage *data, bool deep) const
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

	if (err == B_OK && fDisallowedChars != NULL) {
		err = data->AddData("_dis_ch", B_RAW_TYPE, fDisallowedChars->Items(),
			fDisallowedChars->CountItems() * sizeof(int32));
	}

	if (err == B_OK) {
		int32 runSize = 0;
		text_run_array *runArray = RunArray(0, TextLength());

		void *flattened = FlattenRunArray(runArray, &runSize);
		if (flattened != NULL) {
			data->AddData("_runs", B_RAW_TYPE, flattened, runSize);
			free(flattened);
		} else
			err = B_NO_MEMORY;

		FreeRunArray(runArray);
	}

	return err;
}


/*! \brief Hook function called when the BTextView is added to the
	window's view hierarchy.

	Set the window's pulse rate to 2 per second and adjust scrollbars if needed
*/
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

	_AutoResize(true);

	_UpdateScrollbars();

	SetViewCursor(B_CURSOR_SYSTEM_DEFAULT);
}


/*! \brief Hook function called when the BTextView is removed from the
	window's view hierarchy.
*/
void
BTextView::DetachedFromWindow()
{
	BView::DetachedFromWindow();
}


/*! \brief Hook function called whenever
	the contents of the BTextView need to be (re)drawn.
	\param updateRect The rect which needs to be redrawn
*/
void
BTextView::Draw(BRect updateRect)
{
	// what lines need to be drawn?
	int32 startLine = _LineAt(BPoint(0.0, updateRect.top));
	int32 endLine = _LineAt(BPoint(0.0, updateRect.bottom));

	_DrawLines(startLine, endLine, -1, true);
}


/*! \brief Hook function called when a mouse button is clicked while
	the cursor is in the view.
	\param where The location where the mouse button has been clicked.
*/
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
	uint32 buttons;
	BMessage *currentMessage = Window()->CurrentMessage();
	if (currentMessage != NULL) {
		currentMessage->FindInt32("modifiers", &modifiers);
		currentMessage->FindInt32("buttons", (int32 *)&buttons);
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


/*! \brief Hook function called when a mouse button is released while
	the cursor is in the view.
	\param where The point where the mouse button has been released.

	Stops asynchronous mouse tracking
*/
void
BTextView::MouseUp(BPoint where)
{
	BView::MouseUp(where);
	_PerformMouseUp(where);

	delete fDragRunner;
	fDragRunner = NULL;
}


/*! \brief Hook function called whenever the mouse cursor enters, exits
	or moves inside the view.
	\param where The point where the mouse cursor has moved to.
	\param code A code which tells if the mouse entered or exited the view
	\param message The message containing dragged information, if any.
*/
void
BTextView::MouseMoved(BPoint where, uint32 code, const BMessage *message)
{
	// Check if it's a "click'n'move"
	if (_PerformMouseMoved(where, code))
		return;

	switch (code) {
		case B_ENTERED_VIEW:
		case B_INSIDE_VIEW:
			_TrackMouse(where, message, true);
			break;

		case B_EXITED_VIEW:
			_DragCaret(-1);
			if (Window()->IsActive() && message == NULL)
				SetViewCursor(B_CURSOR_SYSTEM_DEFAULT);
			break;

		default:
			BView::MouseMoved(where, code, message);
			break;
	}
}


/*! \brief Hook function called when the window becomes the active window
	or gives up that status.
	\param state If true, window has just become active, if false, window has
	just become inactive.
*/
void
BTextView::WindowActivated(bool state)
{
	BView::WindowActivated(state);

	if (state && IsFocus()) {
		if (!fActive)
			_Activate();
	} else {
		if (fActive)
			_Deactivate();
	}

	BPoint where;
	ulong buttons;
	GetMouse(&where, &buttons, false);

	if (Bounds().Contains(where))
		_TrackMouse(where, NULL);
}


/*! \brief Hook function called whenever a key is pressed while the view is
	the focus view of the active window.
*/
void
BTextView::KeyDown(const char *bytes, int32 numBytes)
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
			// if the character is not allowed, bail out.
			if (fDisallowedChars
				&& fDisallowedChars->HasItem(
					reinterpret_cast<void *>((uint32)keyPressed))) {
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


/*! \brief Hook function called every x microseconds.
	It's the function which makes the caret blink.
*/
void
BTextView::Pulse()
{
	if (fActive && fEditable && fSelStart == fSelEnd) {
		if (system_time() > (fCaretTime + 500000.0))
			_InvertCaret();
	}
}


/*! \brief Hook function called when the view's frame is resized.
	\param width The new view's width.
	\param height The new view's height.

	Updates the associated scrollbars.
*/
void
BTextView::FrameResized(float width, float height)
{
	BView::FrameResized(width, height);
	_UpdateScrollbars();
}


/*! \brief Highlight/unhighlight the selection when the view gets
		or looses the focus.
	\param focusState The focus state: true, if the view is getting the focus,
		false otherwise.
*/
void
BTextView::MakeFocus(bool focusState)
{
	BView::MakeFocus(focusState);

	if (focusState && Window() && Window()->IsActive()) {
		if (!fActive)
			_Activate();
	} else {
		if (fActive)
			_Deactivate();
	}
}


/*! \brief Hook function executed every time the BTextView gets a message.
	\param message The received message
*/
void
BTextView::MessageReceived(BMessage *message)
{
	// TODO: block input if not editable (Andrew)

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
			const char *property;

			if (message->GetCurrentSpecifier(NULL, &specifier) < B_OK
				|| specifier.FindString("property", &property) < B_OK)
				return;

			if (propInfo.FindMatch(message, 0, &specifier, specifier.what,
					property) < B_OK) {
				BView::MessageReceived(message);
				break;
			}

			BMessage reply;
			bool handled = false;
			switch(message->what) {
				case B_GET_PROPERTY:
					handled = _GetProperty(&specifier, specifier.what, property,
						&reply);
					break;

				case B_SET_PROPERTY:
					handled = _SetProperty(&specifier, specifier.what, property,
						&reply);
					break;

				case B_COUNT_PROPERTIES:
					handled = _CountProperties(&specifier, specifier.what,
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

		default:
			BView::MessageReceived(message);
			break;
	}
}


/*! \brief Returns the proper handler for the given scripting message.
	\param message The scripting message which needs to be examined.
	\param index The index of the specifier
	\param specifier The message which contains the specifier
	\param what The 'what' field of the specifier message.
	\param property The name of the targetted property
	\return The proper BHandler for the given scripting message.
*/
BHandler *
BTextView::ResolveSpecifier(BMessage *message, int32 index, BMessage *specifier,
	int32 what, const char *property)
{
	BPropertyInfo propInfo(sPropertyList);
	BHandler *target = this;

	if (propInfo.FindMatch(message, index, specifier, what, property) < B_OK) {
		target = BView::ResolveSpecifier(message, index, specifier, what,
			property);
	}
	return target;
}


status_t
BTextView::GetSupportedSuites(BMessage *data)
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
BTextView::SetText(const char *inText, const text_run_array *inRuns)
{
	SetText(inText, inText ? strlen(inText) : 0, inRuns);
}


void
BTextView::SetText(const char *inText, int32 inLength,
	const text_run_array *inRuns)
{
	_CancelInputMethod();

	// hide the caret/unhilite the selection
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

	if (inText != NULL && inLength > 0)
		InsertText(inText, inLength, 0, inRuns);

	// recalc line breaks and draw the text
	_Refresh(0, inLength, false);
	fCaretOffset = fSelStart = fSelEnd = 0;
	ScrollTo(B_ORIGIN);

	// draw the caret
	_ShowCaret();
}


void
BTextView::SetText(BFile *inFile, int32 inOffset, int32 inLength,
	const text_run_array *inRuns)
{
	CALLED();

	_CancelInputMethod();

	if (!inFile)
		return;

	if (fText->Length() > 0)
		DeleteText(0, fText->Length());

	fText->InsertText(inFile, inOffset, inLength, 0);

	// update the start offsets of each line below offset
	fLines->BumpOffset(inLength, _LineAt(inOffset) + 1);

	// update the style runs
	fStyles->BumpOffset(inLength, fStyles->OffsetToRun(inOffset - 1) + 1);

	if (fStylable && inRuns != NULL)
		SetRunArray(inOffset, inOffset + inLength, inRuns);
	else {
		// apply null-style to inserted text
		_ApplyStyleRange(inOffset, inOffset + inLength);
	}

	// recalc line breaks and draw the text
	_Refresh(0, inLength, false);
	fCaretOffset = fSelStart = fSelEnd = 0;
	ScrollToOffset(fSelStart);

	// draw the caret
	_ShowCaret();
}


void
BTextView::Insert(const char *inText, const text_run_array *inRuns)
{
	if (inText != NULL)
		_DoInsertText(inText, strlen(inText), fSelStart, inRuns);
}


void
BTextView::Insert(const char *inText, int32 inLength,
	const text_run_array *inRuns)
{
	if (inText != NULL && inLength > 0)
		_DoInsertText(inText, strnlen(inText, inLength), fSelStart, inRuns);
}


void
BTextView::Insert(int32 startOffset, const char *inText, int32 inLength,
					   const text_run_array *inRuns)
{
	if (inText != NULL && inLength > 0)
		_DoInsertText(inText, strnlen(inText, inLength), startOffset, inRuns);
}


/*! \brief Deletes the text within the current selection.
*/
void
BTextView::Delete()
{
	Delete(fSelStart, fSelEnd);
}


/*! \brief Delets the text comprised within the given offsets.
	\param startOffset The offset of the text to delete.
	\param endOffset The offset where the text to delete ends.
*/
void
BTextView::Delete(int32 startOffset, int32 endOffset)
{
	CALLED();
	// anything to delete?
	if (startOffset == endOffset)
		return;

	// hide the caret/unhilite the selection
	if (fActive) {
		if (fSelStart != fSelEnd) {
			if (fSelectable)
				Highlight(fSelStart, fSelEnd);
		} else
			_HideCaret();
	}
	// remove data from buffer
	DeleteText(startOffset, endOffset);

	// Check if the caret needs to be moved
	if (fCaretOffset >= endOffset)
		fCaretOffset -= (endOffset - startOffset);
	else if (fCaretOffset >= startOffset && fCaretOffset < endOffset)
		fCaretOffset = startOffset;

	fSelEnd = fSelStart = fCaretOffset;

	// recalc line breaks and draw what's left
	_Refresh(startOffset, endOffset, false);

	// draw the caret
	_ShowCaret();
}


/*! \brief Returns the BTextView text as a C string.
	\return A pointer to the text.

	It is possible that the BTextView object had to do some operations
	on the text, to be able to return it as a C string.
	If you need to call Text() repeatedly, you'd better use GetText().
*/
const char *
BTextView::Text() const
{
	return fText->RealText();
}


/*! \brief Returns the length of the BTextView's text.
	\return The length of the text.
*/
int32
BTextView::TextLength() const
{
	return fText->Length();
}


void
BTextView::GetText(int32 offset, int32 length, char *buffer) const
{
	if (buffer != NULL)
		fText->GetString(offset, length, buffer);
}


/*! \brief Returns the character at the given offset.
	\param offset The offset of the wanted character.
	\return The character at the given offset.
*/
uchar
BTextView::ByteAt(int32 offset) const
{
	if (offset < 0 || offset >= fText->Length())
		return '\0';

	return fText->RealCharAt(offset);
}


/*! \brief Returns the number of lines that the object contains.
	\return The number of lines contained in the BTextView object.
*/
int32
BTextView::CountLines() const
{
	return fLines->NumLines();
}


/*! \brief Returns the index of the current line.
	\return The index of the current line.
*/
int32
BTextView::CurrentLine() const
{
	return LineAt(fSelStart);
}


/*! \brief Move the caret to the specified line.
	\param index The index of the line.
*/
void
BTextView::GoToLine(int32 index)
{
	_CancelInputMethod();
	_HideCaret();
	fSelStart = fSelEnd = fCaretOffset = OffsetAt(index);
	_ShowCaret();
}


/*! \brief Cuts the current selection to the clipboard.
	\param clipboard The clipboard where to copy the cutted text.
*/
void
BTextView::Cut(BClipboard *clipboard)
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


/*! \brief Copies the current selection to the clipboard.
	\param clipboard The clipboard where to copy the selected text.
*/
void
BTextView::Copy(BClipboard *clipboard)
{
	_CancelInputMethod();

	if (clipboard->Lock()) {
		clipboard->Clear();

		BMessage *clip = clipboard->Data();
		if (clip != NULL) {
			int32 numBytes = fSelEnd - fSelStart;
			const char* text = fText->GetString(fSelStart, &numBytes);
			clip->AddData("text/plain", B_MIME_TYPE, text, numBytes);

			int32 size;
			if (fStylable) {
				text_run_array *runArray = RunArray(fSelStart, fSelEnd, &size);
				clip->AddData("application/x-vnd.Be-text_run_array",
					B_MIME_TYPE, runArray, size);
				FreeRunArray(runArray);
			}
			clipboard->Commit();
		}
		clipboard->Unlock();
	}
}


/*! \brief Paste the text contained in the clipboard to the BTextView.
	\param clipboard A pointer to the clipboard.
*/
void
BTextView::Paste(BClipboard *clipboard)
{
	CALLED();
	_CancelInputMethod();

	if (!fEditable || !clipboard->Lock())
		return;

	BMessage *clip = clipboard->Data();
	if (clip != NULL) {
		const char *text = NULL;
		ssize_t len = 0;

		if (clip->FindData("text/plain", B_MIME_TYPE,
				(const void **)&text, &len) == B_OK) {
			text_run_array *runArray = NULL;
			ssize_t runLen = 0;

			if (fStylable) {
				clip->FindData("application/x-vnd.Be-text_run_array",
					B_MIME_TYPE, (const void **)&runArray, &runLen);
			}

			if (fUndo) {
				delete fUndo;
				fUndo = new PasteUndoBuffer(this, text, len, runArray,
					runLen);
			}

			if (fSelStart != fSelEnd)
				Delete();

			Insert(text, len, runArray);
			ScrollToOffset(fSelEnd);
		}
	}

	clipboard->Unlock();
}


/*! \brief Deletes the currently selected text.
*/
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
BTextView::AcceptsPaste(BClipboard *clipboard)
{
	bool result = false;

	if (fEditable && clipboard && clipboard->Lock()) {
		BMessage *data = clipboard->Data();
		result = data && data->HasData("text/plain", B_MIME_TYPE);
		clipboard->Unlock();
	}

	return result;
}


bool
BTextView::AcceptsDrop(const BMessage *inMessage)
{
	return fEditable && inMessage
		&& inMessage->HasData("text/plain", B_MIME_TYPE);
}


/*! \brief Selects the text within the given offsets.
	\param startOffset The offset of the text to select.
	\param endOffset The offset where the text ends.
*/
void
BTextView::Select(int32 startOffset, int32 endOffset)
{
	CALLED();
	if (!fSelectable)
		return;

	_CancelInputMethod();

	// a negative selection?
	if (startOffset > endOffset)
		return;

	// pin offsets at reasonable values
	if (startOffset < 0)
		startOffset = 0;
	if (endOffset < 0)
		endOffset = 0;
	else if (endOffset > fText->Length())
		endOffset = fText->Length();

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


/*! \brief Selects all the text within the BTextView.
*/
void
BTextView::SelectAll()
{
	Select(0, fText->Length());
}


/*! \brief Gets the current selection.
	\param outStart A pointer to an int32 which will contain the selection
		start's offset.
	\param outEnd A pointer to an int32 which will contain the selection
		end's offset.
*/
void
BTextView::GetSelection(int32 *outStart, int32 *outEnd) const
{
	int32 start = 0, end = 0;

	if (fSelectable) {
		start = fSelStart;
		end = fSelEnd;
	}

	if (outStart)
		*outStart = start;
	if (outEnd)
		*outEnd = end;
}


void
BTextView::SetFontAndColor(const BFont *inFont, uint32 inMode,
	const rgb_color *inColor)
{
	SetFontAndColor(fSelStart, fSelEnd, inFont, inMode, inColor);
}


void
BTextView::SetFontAndColor(int32 startOffset, int32 endOffset,
	const BFont* font, uint32 fontMode, const rgb_color* color)
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
	_ApplyStyleRange(startOffset, endOffset, fontMode, font, color);

	if ((fontMode & (B_FONT_FAMILY_AND_STYLE | B_FONT_SIZE)) != 0) {
		// TODO: maybe only invalidate the layout (depending on
		// B_SUPPORTS_LAYOUT) and have it _Refresh() automatically?
		InvalidateLayout();
		// recalc the line breaks and redraw with new style
		_Refresh(startOffset, endOffset, false);
	} else {
		// the line breaks wont change, simply redraw
		_RequestDrawLines(_LineAt(startOffset), _LineAt(endOffset));
	}

	_ShowCaret();
}


void
BTextView::GetFontAndColor(int32 inOffset, BFont *outFont,
	rgb_color *outColor) const
{
	fStyles->GetStyle(inOffset, outFont, outColor);
}


void
BTextView::GetFontAndColor(BFont *outFont, uint32 *outMode, rgb_color
	*outColor, bool *outEqColor) const
{
	fStyles->ContinuousGetStyle(outFont, outMode, outColor, outEqColor,
		fSelStart, fSelEnd);
}


void
BTextView::SetRunArray(int32 startOffset, int32 endOffset,
	const text_run_array *inRuns)
{
	CALLED();

	_CancelInputMethod();

	const text_run_array *runs = inRuns;

	text_run_array oneRun;

	if (!fStylable) {
		// When the text view is not stylable, we always set the whole text's
		// style with the first run and ignore the offsets
		if (inRuns->count == 0)
			return;
		startOffset = 0;
		endOffset = fText->Length();
		oneRun.count = 1;
		oneRun.runs[0] = inRuns->runs[0];
		oneRun.runs[0].offset = 0;
		runs = &oneRun;
	}

	_SetRunArray(startOffset, endOffset, runs);

	_Refresh(startOffset, endOffset, false);
}


/*! \brief Returns a RunArray for the text within the given offsets.
	\param startOffset The offset where to start.
	\param endOffset The offset where the wanted text ends.
	\param outSize A pointer to an int32 which will contain the size
		of the run array.
	\return A text_run_array for the text in the given offsets.

	The returned text_run_array belongs to the caller, so you better
	free it as soon as you don't need it.
*/
text_run_array *
BTextView::RunArray(int32 startOffset, int32 endOffset, int32 *outSize) const
{
	STEStyleRange* styleRange = fStyles->GetStyleRange(startOffset,
		endOffset - 1);
	if (styleRange == NULL)
		return NULL;

	text_run_array *runArray = AllocRunArray(styleRange->count, outSize);
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


/*! \brief Returns the line number for the character at the given offset.
	\param offset The offset of the wanted character.
	\return A line number.
*/
int32
BTextView::LineAt(int32 offset) const
{
	int32 lineNum = _LineAt(offset);
	if (_IsOnEmptyLastLine(offset))
		lineNum++;
	return lineNum;
}


/*! \brief Returns the line number for the given point.
	\param point A point.
	\return A line number.
*/
int32
BTextView::LineAt(BPoint point) const
{
	int32 lineNum = _LineAt(point);
	if ((*fLines)[lineNum + 1]->origin <= point.y - fTextRect.top)
		lineNum++;
	return lineNum;
}


/*! \brief Returns the location of the character at the given offset.
	\param inOffset The offset of the character.
	\param outHeight Here the function will put the height of the character
		at the given offset.
	\return A BPoint which is the location of the character.
*/
BPoint
BTextView::PointAt(int32 inOffset, float *outHeight) const
{
	// TODO: Cleanup.
	int32 lineNum = _LineAt(inOffset);
	STELine* line = (*fLines)[lineNum];
	float height = 0;

	BPoint result;
	result.x = 0.0;
	result.y = line->origin + fTextRect.top;

	bool onEmptyLastLine = _IsOnEmptyLastLine(inOffset);

	if (fStyles->NumRuns() == 0) {
		// Handle the case where there is only one line (no text inserted)
		fStyles->SyncNullStyle(0);
		height = _NullStyleHeight();
	} else {
		height = (line + 1)->origin - line->origin;

		if (onEmptyLastLine) {
			// special case: go down one line if inOffset is at the newline
			// at the end of the buffer ...
			result.y += height;
			// ... and return the height of that (empty) line
			fStyles->SyncNullStyle(inOffset);
			height = _NullStyleHeight();
		} else {
			int32 length = inOffset - line->offset;
			result.x += _TabExpandedStyledWidth(line->offset, length);
		}
	}

	if (fAlignment != B_ALIGN_LEFT) {
		float lineWidth = onEmptyLastLine ? 0.0 : LineWidth(lineNum);
		float alignmentOffset = fTextRect.Width() - lineWidth;
		if (fAlignment == B_ALIGN_CENTER)
			alignmentOffset /= 2;
		result.x += alignmentOffset;
	}

	// convert from text rect coordinates
	result.x += fTextRect.left;

	// round up
	result.x = lroundf(result.x);
	result.y = lroundf(result.y);
	if (outHeight != NULL)
		*outHeight = height;

	return result;
}


/*! \brief Returns the offset for the given location.
	\param point A BPoint which specify the wanted location.
	\return The offset for the given point.
*/
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
		float alignmentOffset = fTextRect.Width() - LineWidth(lineNum);
		if (fAlignment == B_ALIGN_CENTER)
			alignmentOffset /= 2;
		point.x -= alignmentOffset;
	}

	point.x -= fTextRect.left;
	point.x = max_c(point.x, 0.0);

	// TODO: The following code isn't very efficient, because it always starts
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


/*! \brief Returns the offset of the given line.
	\param line A line number.
	\return The offset of the passed line.
*/
int32
BTextView::OffsetAt(int32 line) const
{
	if (line < 0)
		return 0;
	if (line > fLines->NumLines())
		return fText->Length();

	return (*fLines)[line]->offset;
}


/*! \brief Looks for a sequence of character that qualifies as a word.
	\param inOffset The offset where to start looking.
	\param outFromOffset A pointer to an integer which will contain the starting
		offset of the word.
	\param outToOffset A pointer to an integer which will contain the ending
		offset of the word.
*/
void
BTextView::FindWord(int32 inOffset, int32 *outFromOffset, int32 *outToOffset)
{
	if (outFromOffset)
		*outFromOffset = _PreviousWordBoundary(inOffset);

	if (outToOffset)
		*outToOffset = _NextWordBoundary(inOffset);
}


/*! \brief Returns true if the character at the given offset can be the last
		character in a line.
	\param offset The offset of the character.
	\return true if the character can be the last of a line, false if not.
*/
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


/*! \brief Returns the width of the line at the given index.
	\param lineNum A line index.
*/
float
BTextView::LineWidth(int32 lineNum) const
{
	if (lineNum < 0 || lineNum >= fLines->NumLines())
		return 0;

	STELine* line = (*fLines)[lineNum];
	int32 length = (line + 1)->offset - line->offset;

	// skip newline at the end of the line, if any, as it does no contribute
	// to the width
	if (ByteAt((line + 1)->offset - 1) == B_ENTER)
		length--;

	return _TabExpandedStyledWidth(line->offset, length);
}


/*! \brief Returns the height of the line at the given index.
	\param lineNum A line index.
*/
float
BTextView::LineHeight(int32 lineNum) const
{
	float lineHeight = TextHeight(lineNum, lineNum);
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


/*! \brief Returns the height of the text comprised between the two given lines.
	\param startLine The index of the starting line.
	\param endLine The index of the ending line.
*/
float
BTextView::TextHeight(int32 startLine, int32 endLine) const
{
	const int32 numLines = fLines->NumLines();
	if (startLine < 0)
		startLine = 0;
	if (endLine > numLines - 1)
		endLine = numLines - 1;

	float height = (*fLines)[endLine + 1]->origin
		- (*fLines)[startLine]->origin;

	if (startLine != endLine && endLine == numLines - 1
		&& fText->RealCharAt(fText->Length() - 1) == B_ENTER)
		height += (*fLines)[endLine + 1]->origin - (*fLines)[endLine]->origin;

	return ceilf(height);
}


void
BTextView::GetTextRegion(int32 startOffset, int32 endOffset,
	BRegion *outRegion) const
{
	if (!outRegion)
		return;

	outRegion->MakeEmpty();

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

	if (startPt.y == endPt.y) {
		// this is a one-line region
		selRect.left = max_c(startPt.x, fTextRect.left);
		selRect.top = startPt.y;
		selRect.right = endPt.x - 1.0;
		selRect.bottom = endPt.y + endLineHeight - 1.0;
		outRegion->Include(selRect);
	} else {
		// more than one line in the specified offset range
		selRect.left = max_c(startPt.x, fTextRect.left);
		selRect.top = startPt.y;
		selRect.right = fTextRect.right;
		selRect.bottom = startPt.y + startLineHeight - 1.0;
		outRegion->Include(selRect);

		if (startPt.y + startLineHeight < endPt.y) {
			// more than two lines in the range
			selRect.left = fTextRect.left;
			selRect.top = startPt.y + startLineHeight;
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


/*! \brief Scrolls the text so that the character at "inOffset" is within the
		visible range.
	\param inOffset The offset of the character.
*/
void
BTextView::ScrollToOffset(int32 inOffset)
{
	BRect bounds = Bounds();
	float lineHeight = 0.0;
	float xDiff = 0.0;
	float yDiff = 0.0;
	BPoint point = PointAt(inOffset, &lineHeight);

	// horizontal
	float extraSpace = fAlignment == B_ALIGN_LEFT ?
		ceilf(bounds.IntegerWidth() / 2) : 0.0;

	if (point.x < bounds.left)
		xDiff = point.x - bounds.left - extraSpace;
	else if (point.x > bounds.right)
		xDiff = point.x - bounds.right + extraSpace;

	// vertical
	if (point.y < bounds.top)
		yDiff = point.y - bounds.top;
	else if (point.y + lineHeight > bounds.bottom
		&& point.y - lineHeight > bounds.top) {
		yDiff = point.y + lineHeight - bounds.bottom;
	}

	// prevent negative scroll offset
	if (bounds.left + xDiff < 0.0)
		xDiff = -bounds.left;
	if (bounds.top + yDiff < 0.0)
		yDiff = -bounds.top;

	ScrollBy(xDiff, yDiff);
}


/*! \brief Scrolls the text so that the character which begins the current
		selection is within the visible range.
	\param inOffset The offset of the character.
*/
void
BTextView::ScrollToSelection()
{
	ScrollToOffset(fSelStart);
}


/*! \brief Highlight the text comprised between the given offset.
	\param startOffset The offset of the text to highlight.
	\param endOffset The offset where the text to highlight ends.
*/
void
BTextView::Highlight(int32 startOffset, int32 endOffset)
{
	// get real
	if (startOffset >= endOffset)
		return;

	BRegion selRegion;
	GetTextRegion(startOffset, endOffset, &selRegion);

	SetDrawingMode(B_OP_INVERT);
	FillRegion(&selRegion, B_SOLID_HIGH);
	SetDrawingMode(B_OP_COPY);
}


// #pragma mark - configuration


/*! \brief Sets the BTextView's text rectangle to be the same as the passed
		rect.
	\param rect A BRect.
*/
void
BTextView::SetTextRect(BRect rect)
{
	if (rect == fTextRect)
		return;

	if (!fWrap) {
		rect.right = Bounds().right - fLayoutData->rightInset;
		rect.bottom = Bounds().bottom - fLayoutData->bottomInset;
	}

	fLayoutData->UpdateInsets(Bounds().OffsetToCopy(B_ORIGIN), rect);

	_ResetTextRect();
}


/*! \brief Returns the current BTextView's text rectangle.
	\return The current text rectangle.
*/
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
	_Refresh(0, TextLength(), false);

	// Make sure that the dirty area outside the text is redrawn too.
	BRegion invalid(oldTextRect | fTextRect);
	invalid.Exclude(fTextRect);
	Invalidate(&invalid);
}


/*! \brief Sets the insets from the bounds for the BTextView's text rectangle.
*/
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

	InvalidateLayout();
	Invalidate();
}


/*! \brief Returns the insets from the bounds for the BTextView's text
	rectangle.
*/
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


/*! \brief Sets whether the BTextView accepts multiple character styles.
*/
void
BTextView::SetStylable(bool stylable)
{
	fStylable = stylable;
}


/*! \brief Tells if the object is stylable.
	\return true if the object is stylable, false otherwise.
	If the object is stylable, it can show multiple fonts at the same time.
*/
bool
BTextView::IsStylable() const
{
	return fStylable;
}


/*! \brief Sets the distance between tab stops (in pixel).
	\param width The distance (in pixel) between tab stops.
*/
void
BTextView::SetTabWidth(float width)
{
	if (width == fTabWidth)
		return;

	fTabWidth = width;

	if (Window() != NULL)
		_Refresh(0, fText->Length(), false);
}


/*! \brief Returns the BTextView's tab width.
	\return The BTextView's tab width.
*/
float
BTextView::TabWidth() const
{
	return fTabWidth;
}


/*! \brief Makes the object selectable, or not selectable.
	\param selectable If true, the object will be selectable from now on.
	 if false, it won't be selectable anymore.
*/
void
BTextView::MakeSelectable(bool selectable)
{
	if (selectable == fSelectable)
		return;

	fSelectable = selectable;

	if (fActive && fSelStart != fSelEnd && Window() != NULL)
		Highlight(fSelStart, fSelEnd);
}


/*! \brief Tells if the object is selectable
	\return \c true if the object is selectable,
			\c false if not.
*/
bool
BTextView::IsSelectable() const
{
	return fSelectable;
}


/*! \brief Set (or remove) the editable property for the object.
	\param editable If true, will make the object editable,
		if false, will make it not editable.
*/
void
BTextView::MakeEditable(bool editable)
{
	if (editable == fEditable)
		return;

	fEditable = editable;
	// TextControls change the color of the text when
	// they are made editable, so we need to invalidate
	// the NULL style here
	// TODO: it works well, but it could be caused by a bug somewhere else
	if (fEditable)
		fStyles->InvalidateNullStyle();
	if (Window() != NULL && fActive) {
		if (!fEditable) {
			_HideCaret();
			_CancelInputMethod();
		}
	}
}


/*! \brief Tells if the object is editable.
	\return \c true if the object is editable,
			\c false if not.
*/
bool
BTextView::IsEditable() const
{
	return fEditable;
}


/*! \brief Set (or unset) word wrapping mode.
	\param wrap Specifies if you want word wrapping active or not.
*/
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

	fWrap = wrap;
	if (wrap)
		_ResetTextRect();
	_Refresh(0, fText->Length(), false);

	if (updateOnScreen) {
		// show the caret, hilite the selection
		if (fSelStart != fSelEnd) {
			if (fSelectable)
				Highlight(fSelStart, fSelEnd);
		} else
			_ShowCaret();
	}
}


/*! \brief Tells if word wrapping is activated.
	\return true if word wrapping is active, false otherwise.
*/
bool
BTextView::DoesWordWrap() const
{
	return fWrap;
}


/*! \brief Sets the maximun number of bytes that the BTextView can contain.
	\param max The new max number of bytes.
*/
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


/*! \brief Returns the maximum number of bytes that the BTextView can contain.
	\return the maximum number of bytes that the BTextView can contain.
*/
int32
BTextView::MaxBytes() const
{
	return fMaxBytes;
}


/*! \brief Adds the given char to the disallowed chars list.
	\param aChar The character to add to the list.

	After this function returns, the given character won't be accepted
	by the textview anymore.
*/
void
BTextView::DisallowChar(uint32 aChar)
{
	if (fDisallowedChars == NULL)
		fDisallowedChars = new BList;
	if (!fDisallowedChars->HasItem(reinterpret_cast<void *>(aChar)))
		fDisallowedChars->AddItem(reinterpret_cast<void *>(aChar));
}


/*! \brief Removes the given character from the disallowed list.
	\param aChar The character to remove from the list.
*/
void
BTextView::AllowChar(uint32 aChar)
{
	if (fDisallowedChars != NULL)
		fDisallowedChars->RemoveItem(reinterpret_cast<void *>(aChar));
}


/*! \brief Sets the way text is aligned within the text rectangle.
	\param flag The new alignment.
*/
void
BTextView::SetAlignment(alignment flag)
{
	// Do a reality check
	if (fAlignment != flag &&
			(flag == B_ALIGN_LEFT ||
			 flag == B_ALIGN_RIGHT ||
			 flag == B_ALIGN_CENTER)) {
		fAlignment = flag;

		// After setting new alignment, update the view/window
		if (Window() != NULL)
			Invalidate();
	}
}


/*! \brief Returns the current alignment of the text.
	\return The current alignment.
*/
alignment
BTextView::Alignment() const
{
	return fAlignment;
}


/*! \brief Sets wheter a new line of text is automatically indented.
	\param state The new autoindent state
*/
void
BTextView::SetAutoindent(bool state)
{
	fAutoindent = state;
}


/*! \brief Returns the current autoindent state.
	\return The current autoindent state.
*/
bool
BTextView::DoesAutoindent() const
{
	return fAutoindent;
}


/*! \brief Set the color space for the offscreen BBitmap.
	\param colors The new colorspace for the offscreen BBitmap.
*/
void
BTextView::SetColorSpace(color_space colors)
{
	if (colors != fColorSpace && fOffscreen) {
		fColorSpace = colors;
		_DeleteOffscreen();
		_NewOffscreen();
	}
}


/*! \brief Returns the colorspace of the offscreen BBitmap, if any.
	\return The colorspace of the BTextView's offscreen BBitmap.
*/
color_space
BTextView::ColorSpace() const
{
	return fColorSpace;
}


/*! \brief Gives to the BTextView the ability to automatically resize itself
		when needed.
	\param resize If true, the BTextView will automatically resize itself.
	\param resizeView The BTextView's parent view, it's the view which resizes
		itself.
	The resizing mechanism is alternative to the BView resizing. The container
	view (the one passed to this function) should not automatically resize
	itself when the parent is resized.
*/
void
BTextView::MakeResizable(bool resize, BView *resizeView)
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
		// This seems to be an artefact of how Tracker creates the textview
		// during a rename action.
		fLayoutData->rightInset = fLayoutData->leftInset;
	} else {
		fResizable = false;
		fContainerView = NULL;
		if (fOffscreen)
			_DeleteOffscreen();
		_NewOffscreen();
	}

	_Refresh(0, fText->Length(), false);
}


/*! \brief Returns whether the BTextView is currently resizable.
	\returns whether the BTextView is currently resizable.
*/
bool
BTextView::IsResizable() const
{
	return fResizable;
}


/*! \brief Enables or disables the undo mechanism.
	\param undo If true enables the undo mechanism, if false, disables it.
*/
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


/*! \brief Tells if the object is undoable.
	\return Whether the object is undoable.
*/
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


// #pragma mark -


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
	// TODO: When not editable and not embedded in a scroll view, we should
	// assume that all text is supposed to be visible.
	return BView::HasHeightForWidth();
}


void
BTextView::GetHeightForWidth(float width, float* min, float* max,
	float* preferred)
{
	// TODO: See above and implement.
	BView::GetHeightForWidth(width, min, max, preferred);
}


void
BTextView::InvalidateLayout(bool descendants)
{
	CALLED();

	fLayoutData->valid = false;

	BView::InvalidateLayout(descendants);
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
	fLayoutData->preferred.height = fTextRect.Height()
		+ fLayoutData->topInset + fLayoutData->bottomInset;

	if (fWrap)
		fLayoutData->preferred.width = min.width + 5 * lineHeight;
	else {
		float maxWidth = fLines->MaxWidth();
		if (maxWidth < min.width)
			maxWidth = min.width;

		fLayoutData->preferred.width
			= maxWidth + fLayoutData->leftInset + fLayoutData->rightInset;
	}

	fLayoutData->valid = true;
	ResetLayoutInvalidation();

	TRACE("width: %.2f, height: %.2f\n", min.width, min.height);
}


// #pragma mark -


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
text_run_array *
BTextView::AllocRunArray(int32 entryCount, int32 *outSize)
{
	int32 size = sizeof(text_run_array) + (entryCount - 1) * sizeof(text_run);

	text_run_array *runArray = (text_run_array *)malloc(size);
	if (runArray == NULL) {
		if (outSize != NULL)
			*outSize = 0;
		return NULL;
	}

	memset(runArray, 0, sizeof(size));

	runArray->count = entryCount;

	// Call constructors explicitly as the text_run_array
	// was allocated with malloc (and has to, for backwards
	// compatibility)
	for (int32 i = 0; i < runArray->count; i++) {
		new (&runArray->runs[i].font) BFont;
	}

	if (outSize != NULL)
		*outSize = size;

	return runArray;
}


/* static */
text_run_array *
BTextView::CopyRunArray(const text_run_array *orig, int32 countDelta)
{
	text_run_array *copy = AllocRunArray(countDelta, NULL);
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
BTextView::FreeRunArray(text_run_array *array)
{
	if (array == NULL)
		return;

	// Call destructors explicitly
	for (int32 i = 0; i < array->count; i++)
		array->runs[i].font.~BFont();

	free(array);
}


/* static */
void *
BTextView::FlattenRunArray(const text_run_array* runArray, int32* _size)
{
	CALLED();
	int32 size = sizeof(flattened_text_run_array) + (runArray->count - 1)
		* sizeof(flattened_text_run);

	flattened_text_run_array *array = (flattened_text_run_array *)malloc(size);
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
text_run_array *
BTextView::UnflattenRunArray(const void* data, int32* _size)
{
	CALLED();
	flattened_text_run_array *array = (flattened_text_run_array *)data;

	if (B_BENDIAN_TO_HOST_INT32(array->magic) != kFlattenedTextRunArrayMagic
		|| B_BENDIAN_TO_HOST_INT32(array->version)
			!= kFlattenedTextRunArrayVersion) {
		if (_size)
			*_size = 0;

		return NULL;
	}

	int32 count = B_BENDIAN_TO_HOST_INT32(array->count);

	text_run_array *runArray = AllocRunArray(count, _size);
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
BTextView::InsertText(const char *inText, int32 inLength, int32 inOffset,
	const text_run_array *inRuns)
{
	CALLED();

	if (inLength < 0)
		inLength = 0;

	if (inOffset < 0)
		inOffset = 0;
	else if (inOffset > fText->Length())
		inOffset = fText->Length();

	if (inLength > 0) {
		// add the text to the buffer
		fText->InsertText(inText, inLength, inOffset);

		// update the start offsets of each line below offset
		fLines->BumpOffset(inLength, _LineAt(inOffset) + 1);

		// update the style runs
		fStyles->BumpOffset(inLength, fStyles->OffsetToRun(inOffset - 1) + 1);

		// offset the caret/selection, if the text was inserted before it
		if (inOffset <= fSelEnd) {
			fSelStart += inLength;
			fCaretOffset = fSelEnd = fSelStart;
		}
	}

	if (fStylable && inRuns != NULL) {
		_SetRunArray(inOffset, inOffset + inLength, inRuns);
	} else {
		// apply null-style to inserted text
		_ApplyStyleRange(inOffset, inOffset + inLength);
	}
}


void
BTextView::DeleteText(int32 fromOffset, int32 toOffset)
{
	CALLED();
	// sanity checking
	if (fromOffset >= toOffset || fromOffset < 0 || toOffset > fText->Length())
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


/*! \brief Undoes the last changes.
	\param clipboard A clipboard to use for the undo operation.
*/
void
BTextView::Undo(BClipboard *clipboard)
{
	if (fUndo)
		fUndo->Undo(clipboard);
}


undo_state
BTextView::UndoState(bool *isRedo) const
{
	return fUndo == NULL ? B_UNDO_UNAVAILABLE : fUndo->State(isRedo);
}


void
BTextView::GetDragParameters(BMessage *drag, BBitmap **bitmap, BPoint *point,
	BHandler **handler)
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
	text_run_array *styles = RunArray(fSelStart, fSelEnd, &size);

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


// #pragma mark -


/*! \brief Inits the BTextView object.
	\param textRect The BTextView's text rect.
	\param initialFont The font which the BTextView will use.
	\param initialColor The initial color of the text.
*/
void
BTextView::_InitObject(BRect textRect, const BFont *initialFont,
	const rgb_color *initialColor)
{
	BFont font;
	if (initialFont == NULL)
		GetFont(&font);
	else
		font = *initialFont;

	_NormalizeFont(&font);

	if (initialColor == NULL)
		initialColor = &kBlackColor;

	fText = new BPrivate::TextGapBuffer;
	fLines = new LineBuffer;
	fStyles = new StyleBuffer(&font, initialColor);

	// We put these here instead of in the constructor initializer list
	// to have less code duplication, and a single place where to do changes
	// if needed.
	fTextRect = textRect;
		// NOTE: The only places where text rect is changed:
		// * width is possibly adjusted in _AutoResize(),
		// * height is adjusted in _RecalculateLineBreaks().
		// When used within the layout management framework, the
		// text rect is changed to maintain constant insets.
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
	fMaxBytes = LONG_MAX;
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
	fLayoutData->UpdateInsets(Bounds().OffsetToCopy(B_ORIGIN), fTextRect);

	fLastClickOffset = -1;

	SetDoesUndo(true);

	// We need to translate some strings, and in order to do so, we need
	// to use the LocaleBackend to reach liblocale.so
	if (gLocaleBackend == NULL)
		LocaleBackend::LoadBackend();
}


/*! \brief Called when Backspace key is pressed.
*/
void
BTextView::_HandleBackspace()
{
	if (fUndo) {
		TypingUndoBuffer *undoBuffer = dynamic_cast<TypingUndoBuffer*>(
			fUndo);
		if (!undoBuffer) {
			delete fUndo;
			fUndo = undoBuffer = new TypingUndoBuffer(this);
		}
		undoBuffer->BackwardErase();
	}

	if (fSelStart == fSelEnd) {
		if (fSelStart == 0)
			return;
		else
			fSelStart = _PreviousInitialByte(fSelStart);
	} else
		Highlight(fSelStart, fSelEnd);

	DeleteText(fSelStart, fSelEnd);
	fCaretOffset = fSelEnd = fSelStart;

	_Refresh(fSelStart, fSelEnd, true);
}


/*! \brief Called when any arrow key is pressed.
	\param inArrowKey The code for the pressed key.
*/
void
BTextView::_HandleArrowKey(uint32 inArrowKey)
{
	// return if there's nowhere to go
	if (fText->Length() == 0)
		return;

	int32 selStart = fSelStart;
	int32 selEnd = fSelEnd;

	int32 modifiers = 0;
	BMessage *message = Window()->CurrentMessage();
	if (message != NULL)
		message->FindInt32("modifiers", &modifiers);

	bool shiftDown = modifiers & B_SHIFT_KEY;
	bool ctrlDown = modifiers & B_CONTROL_KEY;

	int32 lastClickOffset = fCaretOffset;
	switch (inArrowKey) {
		case B_LEFT_ARROW:
			if (!fEditable)
				_ScrollBy(-1 * kHorizontalScrollBarStep, 0);
			else if (fSelStart != fSelEnd && !shiftDown)
				fCaretOffset = fSelStart;
			else {
				fCaretOffset
					= ctrlDown
						? _PreviousWordStart(fCaretOffset - 1)
						: _PreviousInitialByte(fCaretOffset);
				if (shiftDown && fCaretOffset != lastClickOffset) {
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
			if (!fEditable)
				_ScrollBy(kHorizontalScrollBarStep, 0);
			else if (fSelStart != fSelEnd && !shiftDown)
				fCaretOffset = fSelEnd;
			else {
				fCaretOffset
					= ctrlDown
						? _NextWordEnd(fCaretOffset)
						: _NextInitialByte(fCaretOffset);
				if (shiftDown && fCaretOffset != lastClickOffset) {
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
			if (!fEditable)
				_ScrollBy(0, -1 * kVerticalScrollBarStep);
			else if (fSelStart != fSelEnd && !shiftDown)
				fCaretOffset = fSelStart;
			else {
				float height;
				BPoint point = PointAt(fCaretOffset, &height);
				point.y -= height;
				fCaretOffset = OffsetAt(point);
				if (shiftDown && fCaretOffset != lastClickOffset) {
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
			if (!fEditable)
				_ScrollBy(0, kVerticalScrollBarStep);
			else if (fSelStart != fSelEnd && !shiftDown)
				fCaretOffset = fSelEnd;
			else {
				float height;
				BPoint point = PointAt(fCaretOffset, &height);
				point.y += height;
				fCaretOffset = OffsetAt(point);
				if (shiftDown && fCaretOffset != lastClickOffset) {
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

	// invalidate the null style
	fStyles->InvalidateNullStyle();

	if (fEditable) {
		if (shiftDown)
			Select(selStart, selEnd);
		else
			Select(fCaretOffset, fCaretOffset);

		// scroll if needed
		ScrollToOffset(fCaretOffset);
	}
}


/*! \brief Called when the Delete key is pressed.
*/
void
BTextView::_HandleDelete()
{
	if (fUndo) {
		TypingUndoBuffer *undoBuffer = dynamic_cast<TypingUndoBuffer*>(
			fUndo);
		if (!undoBuffer) {
			delete fUndo;
			fUndo = undoBuffer = new TypingUndoBuffer(this);
		}
		undoBuffer->ForwardErase();
	}

	if (fSelStart == fSelEnd) {
		if (fSelEnd == fText->Length())
			return;
		else
			fSelEnd = _NextInitialByte(fSelEnd);
	} else
		Highlight(fSelStart, fSelEnd);

	DeleteText(fSelStart, fSelEnd);
	fCaretOffset = fSelEnd = fSelStart;

	_Refresh(fSelStart, fSelEnd, true);
}


/*! \brief Called when a "Page key" is pressed.
	\param inPageKey The page key which has been pressed.
*/
void
BTextView::_HandlePageKey(uint32 inPageKey)
{
	int32 mods = 0;
	BMessage *currentMessage = Window()->CurrentMessage();
	if (currentMessage)
		currentMessage->FindInt32("modifiers", &mods);

	bool shiftDown = mods & B_SHIFT_KEY;
	bool ctrlDown = mods & B_CONTROL_KEY;
	STELine* line = NULL;
	int32 selStart = fSelStart;
	int32 selEnd = fSelEnd;

	int32 lastClickOffset = fCaretOffset;
	switch (inPageKey) {
		case B_HOME:
			if (!fEditable) {
				_ScrollTo(0, 0);
				break;
			}

			if (ctrlDown) {
				_ScrollTo(0, 0);
				fCaretOffset = 0;
			} else {
				// get the start of the last line if caret is on it
				line = (*fLines)[_LineAt(lastClickOffset)];
				fCaretOffset = line->offset;
			}

			if (!shiftDown)
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

			break;

		case B_END:
			if (!fEditable) {
				_ScrollTo(0, fTextRect.bottom + fLayoutData->bottomInset);
				break;
			}

			if (ctrlDown) {
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

			if (!shiftDown)
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

			if (!fEditable)
				break;

			if (!shiftDown)
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

			if (!fEditable)
				break;

			if (!shiftDown)
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

	if (fEditable) {
		if (shiftDown)
			Select(selStart, selEnd);
		else
			Select(fCaretOffset, fCaretOffset);

		ScrollToOffset(fCaretOffset);
	}
}


/*! \brief Called when an alphanumeric key is pressed.
	\param bytes The string or character associated with the key.
	\param numBytes The amount of bytes containes in "bytes".
*/
void
BTextView::_HandleAlphaKey(const char *bytes, int32 numBytes)
{
	// TODO: block input if not editable (Andrew)
	if (fUndo) {
		TypingUndoBuffer *undoBuffer = dynamic_cast<TypingUndoBuffer*>(fUndo);
		if (!undoBuffer) {
			delete fUndo;
			fUndo = undoBuffer = new TypingUndoBuffer(this);
		}
		undoBuffer->InputCharacter(numBytes);
	}

	bool erase = fSelStart != fText->Length();

	if (fSelStart != fSelEnd) {
		Highlight(fSelStart, fSelEnd);
		DeleteText(fSelStart, fSelEnd);
		erase = true;
	}

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
}


/*! \brief Redraw the text comprised between the two given offsets,
	recalculating linebreaks if needed.
	\param fromOffset The offset from where to refresh.
	\param toOffset The offset where to refresh to.
	\param scroll If true, function will scroll the view to the end offset.
*/
void
BTextView::_Refresh(int32 fromOffset, int32 toOffset, bool scroll)
{
	// TODO: Cleanup
	float saveHeight = fTextRect.Height();
	float saveWidth = fTextRect.Width();
	int32 fromLine = _LineAt(fromOffset);
	int32 toLine = _LineAt(toOffset);
	int32 saveFromLine = fromLine;
	int32 saveToLine = toLine;
	float saveLineHeight = LineHeight(fromLine);

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
	fromLine = max_c(fromVisible, fromLine);
	toLine = min_c(toLine, toVisible);

	int32 drawOffset = fromOffset;
	if (LineHeight(fromLine) != saveLineHeight
		|| newHeight < saveHeight || fromLine < saveFromLine
		|| fAlignment != B_ALIGN_LEFT)
		drawOffset = (*fLines)[fromLine]->offset;

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

	if (scroll)
		ScrollToOffset(fSelEnd);

	Flush();
}


void
BTextView::_RecalculateLineBreaks(int32 *startLine, int32 *endLine)
{
	CALLED();

	// are we insane?
	*startLine = (*startLine < 0) ? 0 : *startLine;
	*endLine = (*endLine > fLines->NumLines() - 1) ? fLines->NumLines() - 1
		: *endLine;

	int32 textLength = fText->Length();
	int32 lineIndex = (*startLine > 0) ? *startLine - 1 : 0;
	int32 recalThreshold = (*fLines)[*endLine + 1]->offset;
	float width = max_c(fTextRect.Width(), 10);
		// TODO: The minimum width of 10 is a work around for the following
		// problem: If the text rect is too small, we are not calculating any
		// line heights, not even for the first line. Maybe this is a bug
		// in the algorithm, but other places in the class rely on at least
		// the first line to return a valid height. Maybe "10" should really
		// be the width of the very first glyph instead.
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

	// update the text rect
	float newHeight = TextHeight(0, fLines->NumLines() - 1);
	fTextRect.bottom = fTextRect.top + newHeight;
	if (!fWrap) {
		fMinTextRectWidth = fLines->MaxWidth();
		fTextRect.right = ceilf(fTextRect.left + fMinTextRectWidth);
	}

	*endLine = lineIndex - 1;
	*startLine = min_c(*startLine, *endLine);
}


int32
BTextView::_FindLineBreak(int32 fromOffset, float *outAscent, float *outDescent,
	float *inOutWidth)
{
	*outAscent = 0.0;
	*outDescent = 0.0;

	const int32 limit = fText->Length();

	// is fromOffset at the end?
	if (fromOffset >= limit) {
		// try to return valid height info anyway
		if (fStyles->NumRuns() > 0) {
			fStyles->Iterate(fromOffset, 1, fInline, NULL, NULL, outAscent,
				outDescent);
		} else {
			if (fStyles->IsValidNullStyle()) {
				const BFont *font = NULL;
				fStyles->GetNullStyle(&font, NULL);

				font_height fh;
				font->GetHeight(&fh);
				*outAscent = fh.ascent;
				*outDescent = fh.descent + fh.leading;
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
			outAscent, outDescent);

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

		delta = max_c(delta, 1);

		deltaWidth = _TabExpandedStyledWidth(offset, delta, &ascent, &descent);
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

		*outAscent = max_c(ascent, *outAscent);
		*outDescent = max_c(descent, *outDescent);

		offset += delta;
		delta = 0;
	}

	if (offset - fromOffset < 1) {
		// there weren't any words that fit entirely in this line
		// force a break in the middle of a word
		*outAscent = 0.0;
		*outDescent = 0.0;
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

			*outAscent = max_c(ascent, *outAscent);
			*outDescent = max_c(descent, *outDescent);
		}
	}

	return min_c(offset, limit);
}


int32
BTextView::_PreviousWordBoundary(int32 offset)
{
	if (offset <= 0)
		return 0;

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
	int32 textLen = TextLength();
	if (offset >= textLen)
		return textLen;

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

	--offset;	// need to look at previous char
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
	int32 textLen = TextLength();
	if (offset >= textLen)
		return textLen;

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


/*! \brief Returns the width used by the characters starting at the given
		offset with the given length, expanding all tab characters as needed.
*/
float
BTextView::_TabExpandedStyledWidth(int32 offset, int32 length, float* outAscent,
	float* outDescent) const
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

	if (outAscent != NULL)
		*outAscent = maxAscent;
	if (outDescent != NULL)
		*outDescent = maxDescent;

	return width;
}


/*! \brief Calculate the width of the text within the given limits.
	\param fromOffset The offset where to start.
	\param length The length of the text to examine.
	\param outAscent A pointer to a float which will contain the maximum
		ascent.
	\param outDescent A pointer to a float which will contain the maximum
		descent.
	\return The width for the text within the given limits.
*/
float
BTextView::_StyledWidth(int32 fromOffset, int32 length, float* outAscent,
	float* outDescent) const
{
	if (length == 0) {
		// determine height of char at given offset, but return empty width
		fStyles->Iterate(fromOffset, 1, fInline, NULL, NULL, outAscent,
			outDescent);
		return 0.0;
	}

	float result = 0.0;
	float ascent = 0.0;
	float descent = 0.0;
	float maxAscent = 0.0;
	float maxDescent = 0.0;

	// iterate through the style runs
	const BFont *font = NULL;
	int32 numChars;
	while ((numChars = fStyles->Iterate(fromOffset, length, fInline, &font,
			NULL, &ascent, &descent)) != 0) {
		maxAscent = max_c(ascent, maxAscent);
		maxDescent = max_c(descent, maxDescent);

#if USE_WIDTHBUFFER
		// Use _BWidthBuffer_ if possible
		if (BPrivate::gWidthBuffer != NULL) {
			result += BPrivate::gWidthBuffer->StringWidth(*fText, fromOffset,
				numChars, font);
		} else {
#endif
			const char* text = fText->GetString(fromOffset, &numChars);
			result += font->StringWidth(text, numChars);

#if USE_WIDTHBUFFER
		}
#endif

		fromOffset += numChars;
		length -= numChars;
	}

	if (outAscent != NULL)
		*outAscent = maxAscent;
	if (outDescent != NULL)
		*outDescent = maxDescent;

	return result;
}


/*! \brief Calculate the actual tab width for the given location.
	\param location The location to calculate the tab width of.
	\return The actual tab width for the given location
*/
float
BTextView::_ActualTabWidth(float location) const
{
	return fTabWidth - fmod(location, fTabWidth);
}


void
BTextView::_DoInsertText(const char *inText, int32 inLength, int32 inOffset,
	const text_run_array *inRuns)
{
	_CancelInputMethod();

	if (TextLength() + inLength > MaxBytes())
		return;

	if (fSelStart != fSelEnd)
		Select(fSelStart, fSelStart);

	const int32 textLength = TextLength();
	if (inOffset > textLength)
		inOffset = textLength;

	// copy data into buffer
	InsertText(inText, inLength, inOffset, inRuns);

	// recalc line breaks and draw the text
	_Refresh(inOffset, inOffset + inLength, false);
}


void
BTextView::_DoDeleteText(int32 fromOffset, int32 toOffset)
{
	CALLED();
}


void
BTextView::_DrawLine(BView *view, const int32 &lineNum,
	const int32 &startOffset, const bool &erase, BRect &eraseRect,
	BRegion &inputRegion)
{
	STELine *line = (*fLines)[lineNum];
	float startLeft = fTextRect.left;
	if (startOffset != -1) {
		if (ByteAt(startOffset) == B_ENTER) {
			// StartOffset is a newline
			startLeft = PointAt(line->offset).x;
		} else
			startLeft = PointAt(startOffset).x;
	}
	else if (fAlignment != B_ALIGN_LEFT) {
		float alignmentOffset = fTextRect.Width() - LineWidth(lineNum);
		if (fAlignment == B_ALIGN_CENTER)
			alignmentOffset /= 2;
		startLeft = fTextRect.left + alignmentOffset;
	}

	int32 length = (line + 1)->offset;
	if (startOffset != -1)
		length -= startOffset;
	else
		length -= line->offset;

	// DrawString() chokes if you draw a newline
	if (ByteAt((line + 1)->offset - 1) == B_ENTER)
		length--;

	view->MovePenTo(startLeft, line->origin + line->ascent + fTextRect.top + 1);

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
	const BFont *font = NULL;
	const rgb_color *color = NULL;
	int32 numBytes;
	drawing_mode defaultTextRenderingMode = DrawingMode();
	// iterate through each style on this line
	while ((numBytes = fStyles->Iterate(offset, length, fInline, &font,
			&color)) != 0) {
		view->SetFont(font);
		view->SetHighColor(*color);

		tabChars = min_c(numBytes, length);
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

			int32 returnedBytes = tabChars;
			const char *stringToDraw = fText->GetString(offset, &returnedBytes);
			view->SetDrawingMode(textRenderingMode);
			view->DrawString(stringToDraw, returnedBytes);
			if (foundTab) {
				float penPos = PenLocation().x - fTextRect.left;
				float tabWidth = _ActualTabWidth(penPos);
				if (numTabs > 1)
					tabWidth += ((numTabs - 1) * fTabWidth);

				view->MovePenBy(tabWidth, 0.0);
				tabChars += numTabs;
			}

			offset += tabChars;
			length -= tabChars;
			numBytes -= tabChars;
			tabChars = min_c(numBytes, length);
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

	// clip the text
	BRect textRect(fTextRect);
	float minWidth
		= Bounds().Width() - fLayoutData->leftInset - fLayoutData->rightInset;
	if (textRect.Width() < minWidth)
		textRect.right = textRect.left + minWidth;
	BRect clipRect = Bounds() & textRect;
	clipRect.InsetBy(-1, -1);

	BRegion newClip;
	newClip.Set(clipRect);
	ConstrainClippingRegion(&newClip);

	// set the low color to the view color so that
	// drawing to a non-white background will work
	SetLowColor(ViewColor());

	BView *view = NULL;
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
	if (startLine < 0)
		startLine = 0;
	if (endLine > maxLine)
		endLine = maxLine;

	STELine *from = (*fLines)[startLine];
	STELine *to = endLine == maxLine ? NULL : (*fLines)[endLine + 1];
	BRect invalidRect(Bounds().left, from->origin + fTextRect.top,
		Bounds().right,
		to != NULL ? to->origin + fTextRect.top : fTextRect.bottom);
	Invalidate(invalidRect);
	Window()->UpdateIfNeeded();
}


void
BTextView::_DrawCaret(int32 offset, bool visible)
{
	float lineHeight;
	BPoint caretPoint = PointAt(offset, &lineHeight);
	caretPoint.x = min_c(caretPoint.x, fTextRect.right);

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


/*! \brief Inverts the blinking caret status.
	Hides the caret if it is being shown, and if it's hidden, shows it.
*/
void
BTextView::_InvertCaret()
{
	fCaretVisible = !fCaretVisible;
	_DrawCaret(fSelStart, fCaretVisible);
	fCaretTime = system_time();
}


/*! \brief Place the dragging caret at the given offset.
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


/*! \brief Tracks the mouse position, doing special actions like changing the
		view cursor.
	\param where The point where the mouse has moved.
	\param message The dragging message, if there is any.
	\param force Passed as second parameter of SetViewCursor()
*/
void
BTextView::_TrackMouse(BPoint where, const BMessage *message, bool force)
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


/*! \brief Tracks the mouse position when the user is dragging some data.
	\param where The point where the mouse has moved.
*/
void
BTextView::_TrackDrag(BPoint where)
{
	CALLED();
	if (Bounds().Contains(where))
		_DragCaret(OffsetAt(where));
}


/*! \brief Function called to initiate a drag operation.
*/
void
BTextView::_InitiateDrag()
{
	BMessage dragMessage(B_MIME_DATA);
	BBitmap *dragBitmap = NULL;
	BPoint bitmapPoint;
	BHandler *dragHandler = NULL;

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


/*! \brief Called when some data is dropped on the view.
	\param inMessage The message which has been dropped.
	\param where The location where the message has been dropped.
	\param offset ?
	\return \c true if the message was handled, \c false if not.
*/
bool
BTextView::_MessageDropped(BMessage *inMessage, BPoint where, BPoint offset)
{
	ASSERT(inMessage);

	void *from = NULL;
	bool internalDrop = false;
	if (inMessage->FindPointer("be:originator", &from) == B_OK
			&& from == this && fSelEnd != fSelStart)
		internalDrop = true;

	_DragCaret(-1);

	delete fDragRunner;
	fDragRunner = NULL;

	_TrackMouse(where, NULL);

	// are we sure we like this message?
	if (!AcceptsDrop(inMessage))
		return false;

	int32 dropOffset = OffsetAt(where);
	if (dropOffset > TextLength())
		dropOffset = TextLength();

	// if this view initiated the drag, move instead of copy
	if (internalDrop) {
		// dropping onto itself?
		if (dropOffset >= fSelStart && dropOffset <= fSelEnd)
			return true;
	}

	ssize_t dataLen = 0;
	const char *text = NULL;
	entry_ref ref;
	if (inMessage->FindData("text/plain", B_MIME_TYPE, (const void **)&text,
			&dataLen) == B_OK) {
		text_run_array *runArray = NULL;
		ssize_t runLen = 0;
		if (fStylable)
			inMessage->FindData("application/x-vnd.Be-text_run_array",
				B_MIME_TYPE, (const void **)&runArray, &runLen);

		if (fUndo) {
			delete fUndo;
			fUndo = new DropUndoBuffer(this, text, dataLen, runArray,
				runLen, dropOffset, internalDrop);
		}

		if (internalDrop) {
			if (dropOffset > fSelEnd)
				dropOffset -= dataLen;
			Delete();
		}

		Insert(dropOffset, text, dataLen, runArray);
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
	if (fWhere.x > bounds.right) {
		scrollBy.x = fWhere.x - bounds.right;
	} else if (fWhere.x < bounds.left) {
		scrollBy.x = fWhere.x - bounds.left; // negative value
	}

	// prevent from scrolling out of view
	if (scrollBy.x != 0.0) {
		float rightMax = floorf(fTextRect.right + fLayoutData->rightInset);
		if (bounds.right + scrollBy.x > rightMax)
			scrollBy.x = rightMax - bounds.right;
		if (bounds.left + scrollBy.x < 0)
			scrollBy.x = -bounds.left;
	}

	if (CountLines() > 1) {
		// scroll in Y only if multiple lines!
		if (fWhere.y > bounds.bottom) {
			scrollBy.y = fWhere.y - bounds.bottom;
		} else if (fWhere.y < bounds.top) {
			scrollBy.y = fWhere.y - bounds.top; // negative value
		}

		// prevent from scrolling out of view
		if (scrollBy.y != 0.0) {
			float bottomMax = floorf(fTextRect.bottom
				+ fLayoutData->bottomInset);
			if (bounds.bottom + scrollBy.y > bottomMax)
				scrollBy.y = bottomMax - bounds.bottom;
			if (bounds.top + scrollBy.y < 0)
				scrollBy.y = -bounds.top;
		}
	}

	if (scrollBy != B_ORIGIN)
		ScrollBy(scrollBy.x, scrollBy.y);
}


/*! \brief Updates the scrollbars associated with the object (if any).
*/
void
BTextView::_UpdateScrollbars()
{
	BRect bounds(Bounds());
	BScrollBar* horizontalScrollBar = ScrollBar(B_HORIZONTAL);
 	BScrollBar* verticalScrollBar = ScrollBar(B_VERTICAL);

	// do we have a horizontal scroll bar?
	if (horizontalScrollBar != NULL) {
		long viewWidth = bounds.IntegerWidth();
		long dataWidth = (long)ceilf(fTextRect.IntegerWidth()
			+ fLayoutData->leftInset + fLayoutData->rightInset);

		long maxRange = dataWidth - viewWidth;
		maxRange = max_c(maxRange, 0);

		horizontalScrollBar->SetRange(0, (float)maxRange);
		horizontalScrollBar->SetProportion((float)viewWidth / (float)dataWidth);
		horizontalScrollBar->SetSteps(kHorizontalScrollBarStep, dataWidth / 10);
	}

	// how about a vertical scroll bar?
	if (verticalScrollBar != NULL) {
		long viewHeight = bounds.IntegerHeight();
		long dataHeight = (long)ceilf(fTextRect.IntegerHeight()
			+ fLayoutData->topInset + fLayoutData->bottomInset);

		long maxRange = dataHeight - viewHeight;
		maxRange = max_c(maxRange, 0);

		verticalScrollBar->SetRange(0, maxRange);
		verticalScrollBar->SetProportion((float)viewHeight / (float)dataHeight);
		verticalScrollBar->SetSteps(kVerticalScrollBarStep, viewHeight);
	}
}


/*! \brief Scrolls by the given offsets
*/
void
BTextView::_ScrollBy(float horizontal, float vertical)
{
	BRect bounds = Bounds();
	_ScrollTo(bounds.left + horizontal, bounds.top + vertical);
}


/*! \brief Scrolls to the given position, making sure not to scroll out of
	bounds
*/
void
BTextView::_ScrollTo(float x, float y)
{
	BRect bounds = Bounds();
	long viewWidth = bounds.IntegerWidth();
	long viewHeight = bounds.IntegerHeight();

	if (x > fTextRect.right - viewWidth)
		x = fTextRect.right - viewWidth;
	if (x < 0.0)
		x = 0.0;

	if (y > fTextRect.bottom + fLayoutData->bottomInset - viewHeight)
		y = fTextRect.bottom + fLayoutData->bottomInset - viewHeight;
	if (y < 0.0)
		y = 0.0;

	ScrollTo(x, y);
}


/*!	\brief Autoresizes the view to fit the contained text.
*/
void
BTextView::_AutoResize(bool redraw)
{
	if (!fResizable)
		return;

	BRect bounds = Bounds();
	float oldWidth = bounds.Width();
	float newWidth = ceilf(fLayoutData->leftInset + fTextRect.Width()
		+ fLayoutData->rightInset);

	if (fContainerView != NULL) {
		// NOTE: This container view thing is only used by Tracker.
		// move container view if not left aligned
		if (fAlignment == B_ALIGN_CENTER) {
			if (fmod(ceilf(newWidth - oldWidth), 2.0) != 0.0)
				newWidth += 1;
			fContainerView->MoveBy(ceilf(oldWidth - newWidth) / 2, 0);
		} else if (fAlignment == B_ALIGN_RIGHT) {
			fContainerView->MoveBy(ceilf(oldWidth - newWidth), 0);
		}
		// resize container view
		fContainerView->ResizeBy(ceilf(newWidth - oldWidth), 0);
	}


	if (redraw)
		_RequestDrawLines(0, 0);

	// erase any potential left over outside the text rect
	// (can only be on right hand side)
	BRect dirty(fTextRect.right + 1, fTextRect.top, bounds.right,
		fTextRect.bottom);
	if (dirty.IsValid()) {
		SetLowColor(ViewColor());
		FillRect(dirty, B_SOLID_LOW);
	}
}


/*! \brief Creates a new offscreen BBitmap with an associated BView.
	param padding Padding (?)

	Creates an offscreen BBitmap which will be used to draw.
*/
void
BTextView::_NewOffscreen(float padding)
{
	if (fOffscreen != NULL)
		_DeleteOffscreen();

#if USE_DOUBLEBUFFERING
	BRect bitmapRect(0, 0, fTextRect.Width() + padding, fTextRect.Height());
	fOffscreen = new BBitmap(bitmapRect, fColorSpace, true, false);
	if (fOffscreen != NULL && fOffscreen->Lock()) {
		BView *bufferView = new BView(bitmapRect, "drawing view", 0, 0);
		fOffscreen->AddChild(bufferView);
		fOffscreen->Unlock();
	}
#endif
}


/*! \brief Deletes the textview's offscreen bitmap, if any.
*/
void
BTextView::_DeleteOffscreen()
{
	if (fOffscreen != NULL && fOffscreen->Lock()) {
		delete fOffscreen;
		fOffscreen = NULL;
	}
}


/*!	\brief Creates a new offscreen bitmap, highlight the selection, and set the
	cursor to B_CURSOR_I_BEAM.
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
	ulong buttons;
	GetMouse(&where, &buttons, false);
	if (Bounds().Contains(where))
		_TrackMouse(where, NULL);
}


/*! \brief Unhilights the selection, set the cursor to B_CURSOR_SYSTEM_DEFAULT.
*/
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
}


/*! \brief Changes the passed font to be displayable by the object.
	\param font A pointer to the font to normalize.

	Set font rotation to 0, removes any font flag, set font spacing
	to \c B_BITMAP_SPACING and font encoding to \c B_UNICODE_UTF8
*/
void
BTextView::_NormalizeFont(BFont *font)
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
	const text_run_array *inRuns)
{
	if (startOffset > endOffset)
		return;

	const int32 textLength = fText->Length();

	// pin offsets at reasonable values
	if (startOffset < 0)
		startOffset = 0;
	else if (startOffset > textLength)
		startOffset = textLength;

	if (endOffset < 0)
		endOffset = 0;
	else if (endOffset > textLength)
		endOffset = textLength;

	const int32 numStyles = inRuns->count;
	if (numStyles > 0) {
		const text_run *theRun = &inRuns->runs[0];
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


/*! \brief Returns a value which tells if the given character is a separator
	character or not.
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


/*! \brief Returns the offset of the next UTF8 character within the BTextView's
		text.
	\param offset The offset where to start looking.
	\return The offset of the next UTF8 character.
*/
int32
BTextView::_NextInitialByte(int32 offset) const
{
	int32 textLength = TextLength();
	if (offset >= textLength)
		return textLength;

	for (++offset; (ByteAt(offset) & 0xC0) == 0x80; ++offset)
		;

	return offset;
}


/*! \brief Returns the offset of the previous UTF8 character within the
		BTextView's text.
	\param offset The offset where to start looking.
	\return The offset of the previous UTF8 character.
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
BTextView::_GetProperty(BMessage *specifier, int32 form, const char *property,
	BMessage *reply)
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

		char *buffer = new char[range + 1];
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
BTextView::_SetProperty(BMessage *specifier, int32 form, const char *property,
	BMessage *reply)
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

		const char *buffer = NULL;
		if (specifier->FindString("data", &buffer) == B_OK)
			InsertText(buffer, range, index, NULL);
		else
			DeleteText(index, range);

		reply->what = B_REPLY;
		reply->AddInt32("error", B_OK);

		return true;
	} else if (strcmp(property, "text_run_array") == 0)
		return false;

	return false;
}


bool
BTextView::_CountProperties(BMessage *specifier, int32 form,
	const char *property, BMessage *reply)
{
	CALLED();
	if (strcmp(property, "Text") == 0) {
		reply->what = B_REPLY;
		reply->AddInt32("result", TextLength());
		reply->AddInt32("error", B_OK);
		return true;
	}

	return false;
}


/*! \brief Called when the object receives a B_INPUT_METHOD_CHANGED message.
	\param message A B_INPUT_METHOD_CHANGED message.
*/
void
BTextView::_HandleInputMethodChanged(BMessage *message)
{
	// TODO: block input if not editable (Andrew)
	ASSERT(fInline != NULL);

	const char *string = NULL;
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
		_Refresh(fSelStart, fSelEnd, true);
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
				// illegal: character starts with utf-8 intermediate byte, skip it
				prevPos = ++currPos;
			} else {
				// single byte character/code, just feed that
				++currPos;
			}
			KeyDown(prevPos, currPos - prevPos);
			prevPos = currPos;
		}

		_Refresh(fSelStart, fSelEnd, true);
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

		_Refresh(inlineOffset, fSelEnd, true);
		_ShowCaret();
	}

}


/*! \brief Called when the object receives a B_INPUT_METHOD_LOCATION_REQUEST
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


/*! \brief Tells the input server method addon to stop the current transaction.
*/
void
BTextView::_CancelInputMethod()
{
	if (!fInline)
		return;

	InlineInput *inlineInput = fInline;
	fInline = NULL;

	if (inlineInput->IsActive() && Window()) {
		_Refresh(inlineInput->Offset(), fText->Length() - inlineInput->Offset(),
			false);

		BMessage message(B_INPUT_METHOD_EVENT);
		message.AddInt32("be:opcode", B_INPUT_METHOD_STOPPED);
		inlineInput->Method()->SendMessage(&message);
	}

	delete inlineInput;
}


/*! \brief Returns the line number for the character at the given offset.
		N.B.: this will never return the last line (use LineAt() if you
		need to be correct about that)
	\param offset The offset of the wanted character.
	\return A line number.
*/
int32
BTextView::_LineAt(int32 offset) const
{
	return fLines->OffsetToLine(offset);
}


/*! \brief Returns the line number for the given point.
		N.B.: this will never return the last line (use LineAt() if you
		need to be correct about that)
	\param point A point.
	\return A line number.
*/
int32
BTextView::_LineAt(const BPoint& point) const
{
	return fLines->PixelToLine(point.y - fTextRect.top);
}


/*! \brief Determines if the given offset is on the empty line at the end of
		the buffer.
	\param offset The offset that shall be checked.
*/
bool
BTextView::_IsOnEmptyLastLine(int32 offset) const
{
	return (offset == TextLength() && offset > 0
		&& fText->RealCharAt(offset - 1) == B_ENTER);
}


void
BTextView::_ApplyStyleRange(int32 fromOffset, int32 toOffset, uint32 inMode,
	const BFont *inFont, const rgb_color *inColor, bool syncNullStyle)
{
	if (inFont != NULL) {
		// if a font has been given, normalize it
		BFont font = *inFont;
		_NormalizeFont(&font);
		inFont = &font;
	}

	if (!fStylable) {
		// always apply font and color to full range for non-stylable textviews
		fromOffset = 0;
		toOffset = fText->Length();
	}

	if (syncNullStyle)
		fStyles->SyncNullStyle(fromOffset);

	fStyles->SetStyleRange(fromOffset, toOffset, fText->Length(), inMode,
		inFont, inColor);
}


float
BTextView::_NullStyleHeight() const
{
	const BFont *font = NULL;
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
	int32 length = TextLength();

	BPopUpMenu *menu = new BPopUpMenu(B_EMPTY_STRING, false, false);

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
		.AddItem(TRANSLATE("Select All"), B_SELECT_ALL, 'A')
			.SetEnabled(!(start == 0 && finish == length))
	;

	menu->SetTargetForItems(this);
	ConvertToScreen(&where);
	menu->Go(where, true, true,	true);
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
BTextView::TextTrackState::SimulateMouseMovement(BTextView *textView)
{
	BPoint where;
	ulong buttons;
	// When the mouse cursor is still and outside the textview,
	// no B_MOUSE_MOVED message are sent, obviously. But scrolling
	// has to work neverthless, so we "fake" a MouseMoved() call here.
	textView->GetMouse(&where, &buttons);
	textView->_PerformMouseMoved(where, B_INSIDE_VIEW);
}
