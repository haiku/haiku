/*
 * Copyright 2006-2013, Haiku, Inc. All rights reserved.
 * Copyright 1997, 1998 R3 Software Ltd. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus, superstippi@gmx.de
 *		Philippe Saint-Pierre, stpere@gmail.com
 *		John Scipione, jscipione@gmail.com
 *		Timothy Wayper, timmy@wunderbear.com
 */


#include "CalcView.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include <AboutWindow.h>
#include <Alert.h>
#include <Application.h>
#include <AppFileInfo.h>
#include <Beep.h>
#include <Bitmap.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <Clipboard.h>
#include <File.h>
#include <Font.h>
#include <MenuItem.h>
#include <Message.h>
#include <MessageRunner.h>
#include <PlaySound.h>
#include <Point.h>
#include <PopUpMenu.h>
#include <Region.h>
#include <Roster.h>

#include <ExpressionParser.h>

#include "CalcApplication.h"
#include "CalcOptions.h"
#include "ExpressionTextView.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "CalcView"


static const int32 kMsgCalculating = 'calc';
static const int32 kMsgAnimateDots = 'dots';
static const int32 kMsgDoneEvaluating = 'done';

//const uint8 K_COLOR_OFFSET				= 32;
const float kFontScaleY						= 0.4f;
const float kFontScaleX						= 0.4f;
const float kExpressionFontScaleY			= 0.6f;
const float kDisplayScaleY					= 0.2f;

static const bigtime_t kFlashOnOffInterval	= 100000;
static const bigtime_t kCalculatingInterval	= 1000000;
static const bigtime_t kAnimationInterval	= 333333;

static const float kMinimumWidthCompact		= 130.0f;
static const float kMaximumWidthCompact		= 400.0f;
static const float kMinimumHeightCompact	= 20.0f;
static const float kMaximumHeightCompact	= 60.0f;

// Basic mode size limits are defined in CalcView.h so
// that they can be used by the CalcWindow constructor.

static const float kMinimumWidthScientific	= 240.0f;
static const float kMaximumWidthScientific	= 400.0f;
static const float kMinimumHeightScientific	= 200.0f;
static const float kMaximumHeightScientific	= 400.0f;

// basic mode keypad layout (default)
const char *kKeypadDescriptionBasic = B_TRANSLATE_MARK(
	"7   8   9   (   )  \n"
	"4   5   6   *   /  \n"
	"1   2   3   +   -  \n"
	"0   .   BS  =   C  \n");

// scientific mode keypad layout
const char *kKeypadDescriptionScientific = B_TRANSLATE_MARK(
	"ln    sin   cos   tan   π    \n"
	"log   asin  acos  atan  sqrt \n"
	"exp   sinh  cosh  tanh  cbrt \n"
	"!     ceil  floor E     ^    \n"
	"7     8     9     (     )    \n"
	"4     5     6     *     /    \n"
	"1     2     3     +     -    \n"
	"0     .     BS    =     C    \n");


enum {
	FLAGS_FLASH_KEY							= 1 << 0,
	FLAGS_MOUSE_DOWN						= 1 << 1
};


struct CalcView::CalcKey {
	char		label[8];
	char		code[8];
	char		keymap[4];
	uint32		flags;
//	float		width;
};


CalcView*
CalcView::Instantiate(BMessage* archive)
{
	if (!validate_instantiation(archive, "CalcView"))
		return NULL;

	return new CalcView(archive);
}


CalcView::CalcView(BRect frame, rgb_color rgbBaseColor, BMessage* settings)
	:
	BView(frame, "DeskCalc", B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_FRAME_EVENTS),
	fColumns(5),
	fRows(4),

	fBaseColor(rgbBaseColor),
	fExpressionBGColor((rgb_color){ 0, 0, 0, 255 }),

	fHasCustomBaseColor(rgbBaseColor != ui_color(B_PANEL_BACKGROUND_COLOR)),

	fWidth(1),
	fHeight(1),

	fKeypadDescription(strdup(kKeypadDescriptionBasic)),
	fKeypad(NULL),

#ifdef __HAIKU__
	fCalcIcon(new BBitmap(BRect(0, 0, 15, 15), 0, B_RGBA32)),
#else
	fCalcIcon(new BBitmap(BRect(0, 0, 15, 15), 0, B_CMAP8)),
#endif

	fPopUpMenu(NULL),
	fAutoNumlockItem(NULL),
	fAudioFeedbackItem(NULL),
	fOptions(new CalcOptions()),
	fEvaluateThread(-1),
	fEvaluateMessageRunner(NULL),
	fEvaluateSemaphore(B_BAD_SEM_ID),
	fEnabled(true)
{
	// tell the app server not to erase our b/g
	SetViewColor(B_TRANSPARENT_32_BIT);

	_Init(settings);
}


CalcView::CalcView(BMessage* archive)
	:
	BView(archive),
	fColumns(5),
	fRows(4),

	fBaseColor(ui_color(B_PANEL_BACKGROUND_COLOR)),
	fExpressionBGColor((rgb_color){ 0, 0, 0, 255 }),

	fHasCustomBaseColor(false),

	fWidth(1),
	fHeight(1),

	fKeypadDescription(strdup(kKeypadDescriptionBasic)),
	fKeypad(NULL),

#ifdef __HAIKU__
	fCalcIcon(new BBitmap(BRect(0, 0, 15, 15), 0, B_RGBA32)),
#else
	fCalcIcon(new BBitmap(BRect(0, 0, 15, 15), 0, B_CMAP8)),
#endif

	fPopUpMenu(NULL),
	fAutoNumlockItem(NULL),
	fAudioFeedbackItem(NULL),
	fOptions(new CalcOptions()),
	fEvaluateThread(-1),
	fEvaluateMessageRunner(NULL),
	fEvaluateSemaphore(B_BAD_SEM_ID),
	fEnabled(true)
{
	// Do not restore the follow mode, in shelfs, we never follow.
	SetResizingMode(B_FOLLOW_NONE);

	_Init(archive);
}


CalcView::~CalcView()
{
	delete fKeypad;
	delete fOptions;
	free(fKeypadDescription);
	delete fEvaluateMessageRunner;
	delete_sem(fEvaluateSemaphore);
}


void
CalcView::AttachedToWindow()
{
	if (be_control_look == NULL)
		SetFont(be_bold_font);

	BRect frame(Frame());
	FrameResized(frame.Width(), frame.Height());

	bool addKeypadModeMenuItems = true;
	if (Parent() && (Parent()->Flags() & B_DRAW_ON_CHILDREN) != 0) {
		// don't add these items if we are a replicant on the desktop
		addKeypadModeMenuItems = false;
	}

	// create and attach the pop-up menu
	_CreatePopUpMenu(addKeypadModeMenuItems);

	if (addKeypadModeMenuItems)
		SetKeypadMode(fOptions->keypad_mode);
}


void
CalcView::MessageReceived(BMessage* message)
{
	if (message->what == B_COLORS_UPDATED && !fHasCustomBaseColor) {
		const char* panelBgColorName = ui_color_name(B_PANEL_BACKGROUND_COLOR);
		if (message->HasColor(panelBgColorName)) {
			fBaseColor = message->GetColor(panelBgColorName, fBaseColor);
			_Colorize();
		}

		return;
	}

	if (Parent() && (Parent()->Flags() & B_DRAW_ON_CHILDREN) != 0) {
		// if we are embedded in desktop we need to receive these
		// message here since we don't have a parent BWindow
		switch (message->what) {
			case MSG_OPTIONS_AUTO_NUM_LOCK:
				ToggleAutoNumlock();
				return;

			case MSG_OPTIONS_AUDIO_FEEDBACK:
				ToggleAudioFeedback();
				return;

			case MSG_OPTIONS_ANGLE_MODE_RADIAN:
				SetDegreeMode(false);
				return;

			case MSG_OPTIONS_ANGLE_MODE_DEGREE:
				SetDegreeMode(true);
				return;
		}
	}

	// check if message was dropped
	if (message->WasDropped()) {
		// pass message on to paste
		if (message->IsSourceRemote())
			Paste(message);
	} else {
		// act on posted message type
		switch (message->what) {

			// handle "cut"
			case B_CUT:
				Cut();
				break;

			// handle copy
			case B_COPY:
				Copy();
				break;

			// handle paste
			case B_PASTE:
				// access system clipboard
				if (be_clipboard->Lock()) {
					BMessage* clipper = be_clipboard->Data();
					//clipper->PrintToStream();
					Paste(clipper);
					be_clipboard->Unlock();
				}
				break;

			// (replicant) about box requested
			case B_ABOUT_REQUESTED:
			{
				BAboutWindow* window = new BAboutWindow(kAppName, kSignature);

				// create the about window
				const char* extraCopyrights[] = {
					"1997, 1998 R3 Software Ltd.",
					NULL
				};

				const char* authors[] = {
					"Stephan Aßmus",
					"John Scipione",
					"Timothy Wayper",
					"Ingo Weinhold",
					NULL
				};

				window->AddCopyright(2006, "Haiku, Inc.", extraCopyrights);
				window->AddAuthors(authors);

				window->Show();

				break;
			}

			case MSG_UNFLASH_KEY:
			{
				int32 key;
				if (message->FindInt32("key", &key) == B_OK)
					_FlashKey(key, 0);

				break;
			}

			case kMsgAnimateDots:
			{
				int32 end = fExpressionTextView->TextLength();
				int32 start = end - 3;
				if (fEnabled || strcmp(fExpressionTextView->Text() + start,
						"...") != 0) {
					// stop the message runner
					delete fEvaluateMessageRunner;
					fEvaluateMessageRunner = NULL;
					break;
				}

				uint8 dot = 0;
				if (message->FindUInt8("dot", &dot) == B_OK) {
					rgb_color fontColor = fExpressionTextView->HighColor();
					rgb_color backColor = fExpressionTextView->LowColor();
					fExpressionTextView->SetStylable(true);
					fExpressionTextView->SetFontAndColor(start, end, NULL, 0,
						&backColor);
					fExpressionTextView->SetFontAndColor(start + dot - 1,
						start + dot, NULL, 0, &fontColor);
					fExpressionTextView->SetStylable(false);
				}

				dot++;
				if (dot == 4)
					dot = 1;

				delete fEvaluateMessageRunner;
				BMessage animate(kMsgAnimateDots);
				animate.AddUInt8("dot", dot);
				fEvaluateMessageRunner = new (std::nothrow) BMessageRunner(
					BMessenger(this), &animate, kAnimationInterval, 1);
				break;
			}

			case kMsgCalculating:
			{
				// calculation has taken more than 3 seconds
				if (fEnabled) {
					// stop the message runner
					delete fEvaluateMessageRunner;
					fEvaluateMessageRunner = NULL;
					break;
				}

				BString calculating;
				calculating << B_TRANSLATE("Calculating") << "...";
				fExpressionTextView->SetText(calculating.String());

				delete fEvaluateMessageRunner;
				BMessage animate(kMsgAnimateDots);
				animate.AddUInt8("dot", 1U);
				fEvaluateMessageRunner = new (std::nothrow) BMessageRunner(
					BMessenger(this), &animate, kAnimationInterval, 1);
				break;
			}

			case kMsgDoneEvaluating:
			{
				_SetEnabled(true);
				rgb_color fontColor = fExpressionTextView->HighColor();
				fExpressionTextView->SetFontAndColor(NULL, 0, &fontColor);

				const char* result;
				if (message->FindString("error", &result) == B_OK)
					fExpressionTextView->SetText(result);
				else if (message->FindString("value", &result) == B_OK)
					fExpressionTextView->SetValue(result);

				// stop the message runner
				delete fEvaluateMessageRunner;
				fEvaluateMessageRunner = NULL;
				break;
			}

			default:
				BView::MessageReceived(message);
				break;
		}
	}
}


void
CalcView::Draw(BRect updateRect)
{
	bool drawBackground = !_IsEmbedded();

	SetHighColor(fBaseColor);
	BRect expressionRect(_ExpressionRect());
	if (updateRect.Intersects(expressionRect)) {
		if (fOptions->keypad_mode == KEYPAD_MODE_COMPACT
			&& expressionRect.Height() >= fCalcIcon->Bounds().Height()) {
			// render calc icon
			expressionRect.left = fExpressionTextView->Frame().right + 2;
			if (drawBackground) {
				SetHighColor(fBaseColor);
				FillRect(updateRect & expressionRect);
			}

			if (fCalcIcon->ColorSpace() == B_RGBA32) {
				SetDrawingMode(B_OP_ALPHA);
				SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
			} else {
				SetDrawingMode(B_OP_OVER);
			}

			BPoint iconPos;
			iconPos.x = expressionRect.right - (expressionRect.Width()
				+ fCalcIcon->Bounds().Width()) / 2.0;
			iconPos.y = expressionRect.top + (expressionRect.Height()
				- fCalcIcon->Bounds().Height()) / 2.0;
			DrawBitmap(fCalcIcon, iconPos);

			SetDrawingMode(B_OP_COPY);
		}

		// render border around expression text view
		expressionRect = fExpressionTextView->Frame();
		expressionRect.InsetBy(-2, -2);
		if (fOptions->keypad_mode != KEYPAD_MODE_COMPACT && drawBackground) {
			expressionRect.InsetBy(-2, -2);
			StrokeRect(expressionRect);
			expressionRect.InsetBy(1, 1);
			StrokeRect(expressionRect);
			expressionRect.InsetBy(1, 1);
		}

		uint32 flags = 0;
		if (!drawBackground)
			flags |= BControlLook::B_BLEND_FRAME;
		be_control_look->DrawTextControlBorder(this, expressionRect,
			updateRect, fBaseColor, flags);
	}

	if (fOptions->keypad_mode == KEYPAD_MODE_COMPACT)
		return;

	// calculate grid sizes
	BRect keypadRect(_KeypadRect());

	if (be_control_look != NULL) {
		if (drawBackground)
			StrokeRect(keypadRect);
		keypadRect.InsetBy(1, 1);
	}

	float sizeDisp = keypadRect.top;
	float sizeCol = (keypadRect.Width() + 1) / (float)fColumns;
	float sizeRow = (keypadRect.Height() + 1) / (float)fRows;

	if (!updateRect.Intersects(keypadRect))
		return;

	SetFontSize(min_c(sizeRow * kFontScaleY, sizeCol * kFontScaleX));

	CalcKey* key = fKeypad;
	for (int row = 0; row < fRows; row++) {
		for (int col = 0; col < fColumns; col++) {
			BRect frame;
			frame.left = keypadRect.left + col * sizeCol;
			frame.right = keypadRect.left + (col + 1) * sizeCol - 1;
			frame.top = sizeDisp + row * sizeRow;
			frame.bottom = sizeDisp + (row + 1) * sizeRow - 1;

			if (drawBackground) {
				SetHighColor(fBaseColor);
				StrokeRect(frame);
			}
			frame.InsetBy(1, 1);

			uint32 flags = 0;
			if (!drawBackground)
				flags |= BControlLook::B_BLEND_FRAME;
			if (key->flags != 0)
				flags |= BControlLook::B_ACTIVATED;
			flags |= BControlLook::B_IGNORE_OUTLINE;

			be_control_look->DrawButtonFrame(this, frame, updateRect,
				fBaseColor, fBaseColor, flags);

			be_control_look->DrawButtonBackground(this, frame, updateRect,
				fBaseColor, flags);

			be_control_look->DrawLabel(this, key->label, frame, updateRect,
				fBaseColor, flags, BAlignment(B_ALIGN_HORIZONTAL_CENTER,
					B_ALIGN_VERTICAL_CENTER), &fButtonTextColor);

			key++;
		}
	}
}


void
CalcView::MouseDown(BPoint point)
{
	// ensure this view is the current focus
	if (!fExpressionTextView->IsFocus()) {
		// Call our version of MakeFocus(), since that will also apply the
		// num_lock setting.
		MakeFocus();
	}

	// read mouse buttons state
	int32 buttons = 0;
	Window()->CurrentMessage()->FindInt32("buttons", &buttons);

	if ((B_PRIMARY_MOUSE_BUTTON & buttons) == 0) {
		// display popup menu if not primary mouse button
		BMenuItem* selected;
		if ((selected = fPopUpMenu->Go(ConvertToScreen(point))) != NULL
			&& selected->Message() != NULL) {
			Window()->PostMessage(selected->Message(), this);
		}
		return;
	}

	if (fOptions->keypad_mode == KEYPAD_MODE_COMPACT) {
		if (fCalcIcon != NULL) {
			BRect bounds(Bounds());
			bounds.left = bounds.right - fCalcIcon->Bounds().Width();
			if (bounds.Contains(point)) {
				// user clicked on calculator icon
				fExpressionTextView->Clear();
			}
		}
		return;
	}

	// calculate grid sizes
	float sizeDisp = fHeight * kDisplayScaleY;
	float sizeCol = fWidth / (float)fColumns;
	float sizeRow = (fHeight - sizeDisp) / (float)fRows;

	// calculate location within grid
	int gridCol = (int)floorf(point.x / sizeCol);
	int gridRow = (int)floorf((point.y - sizeDisp) / sizeRow);

	// check limits
	if ((gridCol >= 0) && (gridCol < fColumns)
		&& (gridRow >= 0) && (gridRow < fRows)) {

		// process key press
		int key = gridRow * fColumns + gridCol;
		_FlashKey(key, FLAGS_MOUSE_DOWN);
		_PressKey(key);

		// make sure we receive the mouse up!
		SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
	}
}


void
CalcView::MouseUp(BPoint point)
{
	if (fOptions->keypad_mode == KEYPAD_MODE_COMPACT)
		return;

	int keys = fRows * fColumns;
	for (int i = 0; i < keys; i++) {
		if (fKeypad[i].flags & FLAGS_MOUSE_DOWN) {
			_FlashKey(i, 0);
			break;
		}
	}
}


void
CalcView::KeyDown(const char* bytes, int32 numBytes)
{
	// if single byte character...
	if (numBytes == 1) {

		//printf("Key pressed: %c\n", bytes[0]);

		switch (bytes[0]) {

			case B_ENTER:
				// translate to evaluate key
				_PressKey("=");
				break;

			case B_LEFT_ARROW:
			case B_BACKSPACE:
				// translate to backspace key
				_PressKey("BS");
				break;

			case B_SPACE:
			case B_ESCAPE:
			case 'c':
				// translate to clear key
				_PressKey("C");
				break;

			// bracket translation
			case '[':
			case '{':
				_PressKey("(");
				break;

			case ']':
			case '}':
				_PressKey(")");
				break;

			default: {
				// scan the keymap array for match
				int keys = fRows * fColumns;
				for (int i = 0; i < keys; i++) {
					if (fKeypad[i].keymap[0] == bytes[0]) {
						_PressKey(i);
						return;
					}
				}
				break;
			}
		}
	}
}


void
CalcView::MakeFocus(bool focused)
{
	if (focused) {
		// set num lock
		if (fOptions->auto_num_lock) {
			set_keyboard_locks(B_NUM_LOCK
				| (modifiers() & (B_CAPS_LOCK | B_SCROLL_LOCK)));
		}
	}

	// pass on request to text view
	fExpressionTextView->MakeFocus(focused);
}


void
CalcView::FrameResized(float width, float height)
{
	fWidth = width;
	fHeight = height;

	// layout expression text view
	BRect expressionRect = _ExpressionRect();
	if (fOptions->keypad_mode == KEYPAD_MODE_COMPACT) {
		expressionRect.InsetBy(2, 2);
		expressionRect.right -= ceilf(fCalcIcon->Bounds().Width() * 1.5);
	} else
		expressionRect.InsetBy(4, 4);

	fExpressionTextView->MoveTo(expressionRect.LeftTop());
	fExpressionTextView->ResizeTo(expressionRect.Width(), expressionRect.Height());

	// configure expression text view font size and color
	float sizeDisp = fOptions->keypad_mode == KEYPAD_MODE_COMPACT
		? fHeight : fHeight * kDisplayScaleY;
	BFont font(be_bold_font);
	font.SetSize(sizeDisp * kExpressionFontScaleY);
	rgb_color fontColor = fExpressionTextView->HighColor();
	fExpressionTextView->SetFontAndColor(&font, B_FONT_ALL, &fontColor);

	expressionRect.OffsetTo(B_ORIGIN);
	float inset = (expressionRect.Height() - fExpressionTextView->LineHeight(0)) / 2;
	expressionRect.InsetBy(0, inset);
	fExpressionTextView->SetTextRect(expressionRect);
	Invalidate();
}


status_t
CalcView::Archive(BMessage* archive, bool deep) const
{
	fExpressionTextView->RemoveSelf();

	// passed on request to parent
	status_t ret = BView::Archive(archive, deep);

	const_cast<CalcView*>(this)->AddChild(fExpressionTextView);

	// save app signature for replicant add-on loading
	if (ret == B_OK)
		ret = archive->AddString("add_on", kSignature);

	// save all the options
	if (ret == B_OK)
		ret = SaveSettings(archive);

	// add class info last
	if (ret == B_OK)
		ret = archive->AddString("class", "CalcView");

	return ret;
}


void
CalcView::Cut()
{
	Copy();	// copy data to clipboard
	fExpressionTextView->Clear(); // remove data
}


void
CalcView::Copy()
{
	// access system clipboard
	if (be_clipboard->Lock()) {
		be_clipboard->Clear();
		BMessage* clipper = be_clipboard->Data();
		clipper->what = B_MIME_DATA;
		// TODO: should check return for errors!
		BString expression = fExpressionTextView->Text();
		clipper->AddData("text/plain", B_MIME_TYPE,
			expression.String(), expression.Length());
		//clipper->PrintToStream();
		be_clipboard->Commit();
		be_clipboard->Unlock();
	}
}


void
CalcView::Paste(BMessage* message)
{
	// handle files first
	int32 count;
	if (message->GetInfo("refs", NULL, &count) == B_OK) {
		entry_ref ref;
		ssize_t read;
		BFile file;
		char buffer[256];
		memset(buffer, 0, sizeof(buffer));
		for (int32 i = 0; i < count; i++) {
			if (message->FindRef("refs", i, &ref) == B_OK) {
				if (file.SetTo(&ref, B_READ_ONLY) == B_OK) {
					read = file.Read(buffer, sizeof(buffer) - 1);
					if (read <= 0)
						continue;
					BString expression(buffer);
					int32 j = expression.Length();
					while (j > 0 && expression[j - 1] == '\n')
						j--;
					expression.Truncate(j);
					if (expression.Length() > 0)
						fExpressionTextView->Insert(expression.String());
				}
			}
		}
		return;
	}
	// handle color drops
	// read incoming color
	const rgb_color* dropColor = NULL;
	ssize_t dataSize;
	if (message->FindData("RGBColor", B_RGB_COLOR_TYPE,
			(const void**)&dropColor, &dataSize) == B_OK
		&& dataSize == sizeof(rgb_color)) {

		// calculate view relative drop point
		BPoint dropPoint = ConvertFromScreen(message->DropPoint());

		// calculate current keypad area
		float sizeDisp = fHeight * kDisplayScaleY;
		BRect keypadRect(0.0, sizeDisp, fWidth, fHeight);

		// check location of color drop
		if (keypadRect.Contains(dropPoint) && dropColor != NULL) {
			fBaseColor = *dropColor;
			fHasCustomBaseColor =
				fBaseColor != ui_color(B_PANEL_BACKGROUND_COLOR);
			_Colorize();
			// redraw
			Invalidate();
		}

	} else {
		// look for text/plain MIME data
		const char* text;
		ssize_t numBytes;
		if (message->FindData("text/plain", B_MIME_TYPE,
				(const void**)&text, &numBytes) == B_OK) {
			BString temp;
			temp.Append(text, numBytes);
			fExpressionTextView->Insert(temp.String());
		}
	}
}


status_t
CalcView::SaveSettings(BMessage* archive) const
{
	status_t ret = archive ? B_OK : B_BAD_VALUE;

	// record grid dimensions
	if (ret == B_OK)
		ret = archive->AddInt16("cols", fColumns);

	if (ret == B_OK)
		ret = archive->AddInt16("rows", fRows);

	// record color scheme
	if (ret == B_OK) {
		ret = archive->AddData("rgbBaseColor", B_RGB_COLOR_TYPE,
			&fBaseColor, sizeof(rgb_color));
	}

	if (ret == B_OK) {
		ret = archive->AddData("rgbDisplay", B_RGB_COLOR_TYPE,
			&fExpressionBGColor, sizeof(rgb_color));
	}

	// record current options
	if (ret == B_OK)
		ret = fOptions->SaveSettings(archive);

	// record display text
	if (ret == B_OK)
		ret = archive->AddString("displayText", fExpressionTextView->Text());

	// record expression history
	if (ret == B_OK)
		ret = fExpressionTextView->SaveSettings(archive);

	// record calculator description
	if (ret == B_OK)
		ret = archive->AddString("calcDesc", fKeypadDescription);

	return ret;
}


void
CalcView::Evaluate()
{
	if (fExpressionTextView->TextLength() == 0) {
		beep();
		return;
	}

	fEvaluateThread = spawn_thread(_EvaluateThread, "Evaluate Thread",
		B_LOW_PRIORITY, this);
	if (fEvaluateThread < B_OK) {
		// failed to create evaluate thread, error out
		fExpressionTextView->SetText(strerror(fEvaluateThread));
		return;
	}

	_AudioFeedback(false);
	_SetEnabled(false);
		// Disable input while we evaluate

	status_t threadStatus = resume_thread(fEvaluateThread);
	if (threadStatus != B_OK) {
		// evaluate thread failed to start, error out
		fExpressionTextView->SetText(strerror(threadStatus));
		_SetEnabled(true);
		return;
	}

	if (fEvaluateMessageRunner == NULL) {
		BMessage message(kMsgCalculating);
		fEvaluateMessageRunner = new (std::nothrow) BMessageRunner(
			BMessenger(this), &message, kCalculatingInterval, 1);
		status_t runnerStatus = fEvaluateMessageRunner->InitCheck();
		if (runnerStatus != B_OK)
			printf("Evaluate Message Runner: %s\n", strerror(runnerStatus));
	}
}


void
CalcView::FlashKey(const char* bytes, int32 numBytes)
{
	BString temp;
	temp.Append(bytes, numBytes);
	int32 key = _KeyForLabel(temp.String());
	if (key >= 0)
		_FlashKey(key, FLAGS_FLASH_KEY);
}


void
CalcView::ToggleAutoNumlock(void)
{
	fOptions->auto_num_lock = !fOptions->auto_num_lock;
	fAutoNumlockItem->SetMarked(fOptions->auto_num_lock);
}


void
CalcView::ToggleAudioFeedback(void)
{
	fOptions->audio_feedback = !fOptions->audio_feedback;
	fAudioFeedbackItem->SetMarked(fOptions->audio_feedback);
}


void
CalcView::SetDegreeMode(bool degrees)
{
	fOptions->degree_mode = degrees;
	fAngleModeRadianItem->SetMarked(!degrees);
	fAngleModeDegreeItem->SetMarked(degrees);
}


void
CalcView::SetKeypadMode(uint8 mode)
{
	if (_IsEmbedded())
		return;

	BWindow* window = Window();
	if (window == NULL)
		return;

	if (fOptions->keypad_mode == mode)
		return;

	fOptions->keypad_mode = mode;
	_MarkKeypadItems(fOptions->keypad_mode);

	float width = fWidth;
	float height = fHeight;

	switch (fOptions->keypad_mode) {
		case KEYPAD_MODE_COMPACT:
		{
			if (window->Bounds() == Frame()) {
				window->SetSizeLimits(kMinimumWidthCompact,
					kMaximumWidthCompact, kMinimumHeightCompact,
					kMaximumHeightCompact);
				window->ResizeTo(width, height * kDisplayScaleY);
			} else
				ResizeTo(width, height * kDisplayScaleY);

			break;
		}

		case KEYPAD_MODE_SCIENTIFIC:
		{
			free(fKeypadDescription);
			fKeypadDescription = strdup(kKeypadDescriptionScientific);
			fRows = 8;
			_ParseCalcDesc(fKeypadDescription);

			window->SetSizeLimits(kMinimumWidthScientific,
				kMaximumWidthScientific, kMinimumHeightScientific,
				kMaximumHeightScientific);

			if (width < kMinimumWidthScientific)
				width = kMinimumWidthScientific;
			else if (width > kMaximumWidthScientific)
				width = kMaximumWidthScientific;

			if (height < kMinimumHeightScientific)
				height = kMinimumHeightScientific;
			else if (height > kMaximumHeightScientific)
				height = kMaximumHeightScientific;

			if (width != fWidth || height != fHeight)
				ResizeTo(width, height);
			else
				Invalidate();

			break;
		}

		case KEYPAD_MODE_BASIC:
		default:
		{
			free(fKeypadDescription);
			fKeypadDescription = strdup(kKeypadDescriptionBasic);
			fRows = 4;
			_ParseCalcDesc(fKeypadDescription);

			window->SetSizeLimits(kMinimumWidthBasic, kMaximumWidthBasic,
				kMinimumHeightBasic, kMaximumHeightBasic);

			if (width < kMinimumWidthBasic)
				width = kMinimumWidthBasic;
			else if (width > kMaximumWidthBasic)
				width = kMaximumWidthBasic;

			if (height < kMinimumHeightBasic)
				height = kMinimumHeightBasic;
			else if (height > kMaximumHeightBasic)
				height = kMaximumHeightBasic;

			if (width != fWidth || height != fHeight)
				ResizeTo(width, height);
			else
				Invalidate();
		}
	}
}


// #pragma mark -


/*static*/ status_t
CalcView::_EvaluateThread(void* data)
{
	CalcView* calcView = reinterpret_cast<CalcView*>(data);
	if (calcView == NULL)
		return B_BAD_TYPE;

	BMessenger messenger(calcView);
	if (!messenger.IsValid())
		return B_BAD_VALUE;

	BString result;
	status_t status = acquire_sem(calcView->fEvaluateSemaphore);
	if (status == B_OK) {
		ExpressionParser parser;
		parser.SetDegreeMode(calcView->fOptions->degree_mode);
		BString expression(calcView->fExpressionTextView->Text());
		try {
			result = parser.Evaluate(expression.String());
		} catch (ParseException e) {
			result << e.message.String() << " at " << (e.position + 1);
			status = B_ERROR;
		}
		release_sem(calcView->fEvaluateSemaphore);
	} else
		result = strerror(status);

	BMessage message(kMsgDoneEvaluating);
	message.AddString(status == B_OK ? "value" : "error", result.String());
	messenger.SendMessage(&message);

	return status;
}


void
CalcView::_Init(BMessage* settings)
{
	// create expression text view
	fExpressionTextView = new ExpressionTextView(_ExpressionRect(), this);
	AddChild(fExpressionTextView);

	// read data from archive
	_LoadSettings(settings);

	// fetch the calc icon for compact view
	_FetchAppIcon(fCalcIcon);

	fEvaluateSemaphore = create_sem(1, "Evaluate Semaphore");
}


status_t
CalcView::_LoadSettings(BMessage* archive)
{
	if (!archive)
		return B_BAD_VALUE;

	// record calculator description
	const char* calcDesc;
	if (archive->FindString("calcDesc", &calcDesc) < B_OK)
		calcDesc = kKeypadDescriptionBasic;

	// save calculator description for reference
	free(fKeypadDescription);
	fKeypadDescription = strdup(calcDesc);

	// read grid dimensions
	if (archive->FindInt16("cols", &fColumns) < B_OK)
		fColumns = 5;
	if (archive->FindInt16("rows", &fRows) < B_OK)
		fRows = 4;

	// read color scheme
	const rgb_color* color;
	ssize_t size;
	if (archive->FindData("rgbBaseColor", B_RGB_COLOR_TYPE,
			(const void**)&color, &size) < B_OK
		|| size != sizeof(rgb_color)) {
		fBaseColor = ui_color(B_PANEL_BACKGROUND_COLOR);
		puts("Missing rgbBaseColor from CalcView archive!\n");
	} else
		fBaseColor = *color;

	if (archive->FindData("rgbDisplay", B_RGB_COLOR_TYPE,
			(const void**)&color, &size) < B_OK
		|| size != sizeof(rgb_color)) {
		fExpressionBGColor = (rgb_color){ 0, 0, 0, 255 };
		puts("Missing rgbBaseColor from CalcView archive!\n");
	} else {
		fExpressionBGColor = *color;
	}

	fHasCustomBaseColor = fBaseColor != ui_color(B_PANEL_BACKGROUND_COLOR);

	// load options
	fOptions->LoadSettings(archive);

	// load display text
	const char* display;
	if (archive->FindString("displayText", &display) < B_OK) {
		puts("Missing expression text from CalcView archive.\n");
	} else {
		// init expression text
		fExpressionTextView->SetText(display);
	}

	// load expression history
	fExpressionTextView->LoadSettings(archive);

	// parse calculator description
	_ParseCalcDesc(fKeypadDescription);

	// colorize based on base color.
	_Colorize();

	return B_OK;
}


void
CalcView::_ParseCalcDesc(const char* keypadDescription)
{
	// TODO: should calculate dimensions from desc here!
	fKeypad = new CalcKey[fRows * fColumns];

	// scan through calculator description and assemble keypad
	CalcKey* key = fKeypad;
	const char* p = keypadDescription;

	while (*p != 0) {
		// copy label
		char* l = key->label;
		while (!isspace(*p))
			*l++ = *p++;
		*l = '\0';

		// set code
		if (strcmp(key->label, "=") == 0)
			strlcpy(key->code, "\n", sizeof(key->code));
		else
			strlcpy(key->code, key->label, sizeof(key->code));

		// set keymap
		if (strlen(key->label) == 1)
			strlcpy(key->keymap, key->label, sizeof(key->keymap));
		else
			*key->keymap = '\0';

		key->flags = 0;

		// add this to the expression text view, so that it
		// will forward the respective KeyDown event to us
		fExpressionTextView->AddKeypadLabel(key->label);

		// advance
		while (isspace(*p))
			++p;
		key++;
	}
}


void
CalcView::_PressKey(int key)
{
	if (!fEnabled)
		return;

	assert(key < (fRows * fColumns));
	assert(key >= 0);

	if (strcmp(fKeypad[key].label, B_TRANSLATE_COMMENT("BS",
		"Key label, 'BS' means backspace")) == 0) {
		// BS means backspace
		fExpressionTextView->BackSpace();
	} else if (strcmp(fKeypad[key].label, B_TRANSLATE_COMMENT("C",
		"Key label, 'C' means clear")) == 0) {
		// C means clear
		fExpressionTextView->Clear();
	} else if (strcmp(fKeypad[key].label, B_TRANSLATE("acos")) == 0
		|| strcmp(fKeypad[key].label, B_TRANSLATE("asin")) == 0
		|| strcmp(fKeypad[key].label, B_TRANSLATE("atan")) == 0
		|| strcmp(fKeypad[key].label, B_TRANSLATE("cbrt")) == 0
		|| strcmp(fKeypad[key].label, B_TRANSLATE("ceil")) == 0
		|| strcmp(fKeypad[key].label, B_TRANSLATE("cos")) == 0
		|| strcmp(fKeypad[key].label, B_TRANSLATE("cosh")) == 0
		|| strcmp(fKeypad[key].label, B_TRANSLATE("exp")) == 0
		|| strcmp(fKeypad[key].label, B_TRANSLATE("floor")) == 0
		|| strcmp(fKeypad[key].label, B_TRANSLATE("log")) == 0
		|| strcmp(fKeypad[key].label, B_TRANSLATE("ln")) == 0
		|| strcmp(fKeypad[key].label, B_TRANSLATE("sin")) == 0
		|| strcmp(fKeypad[key].label, B_TRANSLATE("sinh")) == 0
		|| strcmp(fKeypad[key].label, B_TRANSLATE("sqrt")) == 0
		|| strcmp(fKeypad[key].label, B_TRANSLATE("tan")) == 0
		|| strcmp(fKeypad[key].label, B_TRANSLATE("tanh")) == 0) {
		int32 labelLen = strlen(fKeypad[key].label);
		int32 startSelection = 0;
		int32 endSelection = 0;
		fExpressionTextView->GetSelection(&startSelection, &endSelection);
		if (endSelection > startSelection) {
			// There is selected text, put it inbetween the parens
			fExpressionTextView->Insert(startSelection, fKeypad[key].label,
				labelLen);
			fExpressionTextView->Insert(startSelection + labelLen, "(", 1);
			fExpressionTextView->Insert(endSelection + labelLen + 1, ")", 1);
			// Put the cursor after the ending paren
			// Need to cast to BTextView because Select() is protected
			// in the InputTextView class
			static_cast<BTextView*>(fExpressionTextView)->Select(
				endSelection + labelLen + 2, endSelection + labelLen + 2);
		} else {
			// There is no selected text, insert at the cursor location
			fExpressionTextView->Insert(fKeypad[key].label);
			fExpressionTextView->Insert("()");
			// Put the cursor inside the parens so you can enter an argument
			// Need to cast to BTextView because Select() is protected
			// in the InputTextView class
			static_cast<BTextView*>(fExpressionTextView)->Select(
				endSelection + labelLen + 1, endSelection + labelLen + 1);
		}
	} else {
		// check for evaluation order
		if (fKeypad[key].code[0] == '\n') {
			fExpressionTextView->ApplyChanges();
		} else {
			// insert into expression text
			fExpressionTextView->Insert(fKeypad[key].code);
		}
	}

	_AudioFeedback(true);
}


void
CalcView::_PressKey(const char* label)
{
	int32 key = _KeyForLabel(label);
	if (key >= 0)
		_PressKey(key);
}


int32
CalcView::_KeyForLabel(const char* label) const
{
	int keys = fRows * fColumns;
	for (int i = 0; i < keys; i++) {
		if (strcmp(fKeypad[i].label, label) == 0) {
			return i;
		}
	}
	return -1;
}


void
CalcView::_FlashKey(int32 key, uint32 flashFlags)
{
	if (fOptions->keypad_mode == KEYPAD_MODE_COMPACT)
		return;

	if (flashFlags != 0)
		fKeypad[key].flags |= flashFlags;
	else
		fKeypad[key].flags = 0;
	Invalidate();

	if (fKeypad[key].flags == FLAGS_FLASH_KEY) {
		BMessage message(MSG_UNFLASH_KEY);
		message.AddInt32("key", key);
		BMessageRunner::StartSending(BMessenger(this), &message,
			kFlashOnOffInterval, 1);
	}
}


void
CalcView::_AudioFeedback(bool inBackGround)
{
	// TODO: Use beep events... This interface is not implemented on Haiku
	// anyways...
#if 0
	if (fOptions->audio_feedback) {
		BEntry zimp("key.AIFF");
		entry_ref zimp_ref;
		zimp.GetRef(&zimp_ref);
		play_sound(&zimp_ref, true, false, inBackGround);
	}
#endif
}


void
CalcView::_Colorize()
{
	// calculate light and dark color from base color
	fLightColor.red		= (uint8)(fBaseColor.red * 1.25);
	fLightColor.green	= (uint8)(fBaseColor.green * 1.25);
	fLightColor.blue	= (uint8)(fBaseColor.blue * 1.25);
	fLightColor.alpha	= 255;

	fDarkColor.red		= (uint8)(fBaseColor.red * 0.75);
	fDarkColor.green	= (uint8)(fBaseColor.green * 0.75);
	fDarkColor.blue		= (uint8)(fBaseColor.blue * 0.75);
	fDarkColor.alpha	= 255;

	// keypad text color
	if (fBaseColor.Brightness() > 100)
		fButtonTextColor = (rgb_color){ 0, 0, 0, 255 };
	else
		fButtonTextColor = (rgb_color){ 255, 255, 255, 255 };

	// expression text color
	if (fExpressionBGColor.Brightness() > 100)
		fExpressionTextColor = (rgb_color){ 0, 0, 0, 255 };
	else
		fExpressionTextColor = (rgb_color){ 255, 255, 255, 255 };
}


void
CalcView::_CreatePopUpMenu(bool addKeypadModeMenuItems)
{
	// construct items
	fAutoNumlockItem = new BMenuItem(B_TRANSLATE("Enable Num Lock on startup"),
		new BMessage(MSG_OPTIONS_AUTO_NUM_LOCK));
	fAudioFeedbackItem = new BMenuItem(B_TRANSLATE("Audio Feedback"),
		new BMessage(MSG_OPTIONS_AUDIO_FEEDBACK));
	fAngleModeRadianItem = new BMenuItem(B_TRANSLATE("Radians"),
		new BMessage(MSG_OPTIONS_ANGLE_MODE_RADIAN));
	fAngleModeDegreeItem = new BMenuItem(B_TRANSLATE("Degrees"),
		new BMessage(MSG_OPTIONS_ANGLE_MODE_DEGREE));
	if (addKeypadModeMenuItems) {
		fKeypadModeCompactItem = new BMenuItem(B_TRANSLATE("Compact"),
			new BMessage(MSG_OPTIONS_KEYPAD_MODE_COMPACT), '0');
		fKeypadModeBasicItem = new BMenuItem(B_TRANSLATE("Basic"),
			new BMessage(MSG_OPTIONS_KEYPAD_MODE_BASIC), '1');
		fKeypadModeScientificItem = new BMenuItem(B_TRANSLATE("Scientific"),
			new BMessage(MSG_OPTIONS_KEYPAD_MODE_SCIENTIFIC), '2');
	}

	// apply current settings
	fAutoNumlockItem->SetMarked(fOptions->auto_num_lock);
	fAudioFeedbackItem->SetMarked(fOptions->audio_feedback);
	fAngleModeRadianItem->SetMarked(!fOptions->degree_mode);
	fAngleModeDegreeItem->SetMarked(fOptions->degree_mode);

	// construct menu
	fPopUpMenu = new BPopUpMenu("pop-up", false, false);

	fPopUpMenu->AddItem(fAutoNumlockItem);
	// TODO: Enable this when we use beep events which can be configured
	// in the Sounds preflet.
	//fPopUpMenu->AddItem(fAudioFeedbackItem);
	fPopUpMenu->AddSeparatorItem();
	fPopUpMenu->AddItem(fAngleModeRadianItem);
	fPopUpMenu->AddItem(fAngleModeDegreeItem);
	if (addKeypadModeMenuItems) {
		fPopUpMenu->AddSeparatorItem();
		fPopUpMenu->AddItem(fKeypadModeCompactItem);
		fPopUpMenu->AddItem(fKeypadModeBasicItem);
		fPopUpMenu->AddItem(fKeypadModeScientificItem);
		_MarkKeypadItems(fOptions->keypad_mode);
	}
}


BRect
CalcView::_ExpressionRect() const
{
	BRect r(0.0, 0.0, fWidth, fHeight);
	if (fOptions->keypad_mode != KEYPAD_MODE_COMPACT) {
		r.bottom = floorf(fHeight * kDisplayScaleY) + 1;
	}
	return r;
}


BRect
CalcView::_KeypadRect() const
{
	BRect r(0.0, 0.0, -1.0, -1.0);
	if (fOptions->keypad_mode != KEYPAD_MODE_COMPACT) {
		r.right = fWidth;
		r.bottom = fHeight;
		r.top = floorf(fHeight * kDisplayScaleY);
	}
	return r;
}


void
CalcView::_MarkKeypadItems(uint8 keypad_mode)
{
	switch (keypad_mode) {
		case KEYPAD_MODE_COMPACT:
			fKeypadModeCompactItem->SetMarked(true);
			fKeypadModeBasicItem->SetMarked(false);
			fKeypadModeScientificItem->SetMarked(false);
			break;

		case KEYPAD_MODE_SCIENTIFIC:
			fKeypadModeCompactItem->SetMarked(false);
			fKeypadModeBasicItem->SetMarked(false);
			fKeypadModeScientificItem->SetMarked(true);
			break;

		default: // KEYPAD_MODE_BASIC is the default
			fKeypadModeCompactItem->SetMarked(false);
			fKeypadModeBasicItem->SetMarked(true);
			fKeypadModeScientificItem->SetMarked(false);
	}
}


void
CalcView::_FetchAppIcon(BBitmap* into)
{
	entry_ref appRef;
	status_t status = be_roster->FindApp(kSignature, &appRef);
	if (status == B_OK) {
		BFile file(&appRef, B_READ_ONLY);
		BAppFileInfo appInfo(&file);
		status = appInfo.GetIcon(into, B_MINI_ICON);
	}
	if (status != B_OK)
		memset(into->Bits(), 0, into->BitsLength());
}


// Returns whether or not CalcView is embedded somewhere, most likely
// the Desktop
bool
CalcView::_IsEmbedded()
{
	return Parent() != NULL && (Parent()->Flags() & B_DRAW_ON_CHILDREN) != 0;
}


void
CalcView::_SetEnabled(bool enable)
{
	fEnabled = enable;
	fExpressionTextView->MakeSelectable(enable);
	fExpressionTextView->MakeEditable(enable);
}
