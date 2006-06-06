/*
 * Copyright 2006 Haiku, Inc. All Rights Reserved.
 * Copyright 1997, 1998 R3 Software Ltd. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Timothy Wayper <timmy@wunderbear.com>
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "CalcView.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include <Beep.h>
#include <Clipboard.h>
#include <Font.h>
#include <MenuItem.h>
#include <Message.h>
#include <PlaySound.h>
#include <Point.h>
#include <PopUpMenu.h>

#include "CalcApplication.h"
#include "CalcOptionsWindow.h"
#include "Parser.h"


const uint8 K_COLOR_OFFSET		= 32;
const float K_FONT_YPROP		= 0.6f;
const float K_DISPLAY_YPROP		= 0.2;

enum {
	K_OPTIONS_REQUESTED =	'opts'
};

// default calculator key pad layout
const char *kDefaultKeypadDescription =
	"7   8   9   (   )  \n"
	"4   5   6   *   /  \n"
	"1   2   3   +   -  \n"
	"0   .   BS  =   C  \n";


struct CalcView::CalcKey {
	char		label[8];
	char		code[8];
	char		keymap[4];
	uint32		flags;
	float		width;
};


CalcView *CalcView::Instantiate(BMessage *archive)
{
	if (!validate_instantiation(archive, "CalcView"))
		return NULL; 

	return new CalcView(archive);
}


CalcView::CalcView(BRect frame, rgb_color rgbBaseColor)
	: BView(frame, "calc-view", B_FOLLOW_ALL_SIDES,
			B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE | B_FRAME_EVENTS),
	  fColums(5),
	  fRows(4),

	  fBaseColor(rgbBaseColor),
	  fExpressionBGColor((rgb_color){ 0, 0, 0, 255 }),

	  fWidth(frame.Width()),
	  fHeight(frame.Height()),

	  fKeypadDescription(strdup(kDefaultKeypadDescription)),
	  fKeypad(NULL),

	  fExpression(""),

	  fAboutItem(NULL),
	  fOptionsItem(NULL),
	  fPopUpMenu(NULL),

	  fOptions(new CalcOptions()),
	  fOptionsWindow(NULL)
{
	// tell the app server not to erase our b/g
	SetViewColor(B_TRANSPARENT_32_BIT);

	// parse calculator description
	_ParseCalcDesc(fKeypadDescription);
	
	// colorize based on base color.
	_Colorize();
	
	// create pop-up menu system
	_CreatePopUpMenu();
}


CalcView::CalcView(BMessage* archive)
	: BView(archive),
	  fColums(5),
	  fRows(4),

	  fBaseColor((rgb_color){ 128, 128, 128, 255 }),
	  fExpressionBGColor((rgb_color){ 0, 0, 0, 255 }),

	  fKeypadDescription(strdup(kDefaultKeypadDescription)),
	  fKeypad(NULL),

	  fExpression(""),

	  fAboutItem(NULL),
	  fOptionsItem(NULL),
	  fPopUpMenu(NULL),

	  fOptions(new CalcOptions()),
	  fOptionsWindow(NULL)
{
	// read data from archive
	LoadSettings(archive);
	
	// load frame dimensions
	BRect frame = Frame();
	fWidth = frame.Width();
	fHeight = frame.Height();
	
	// create pop-up menu system
	_CreatePopUpMenu();
}


CalcView::~CalcView()
{
	delete fKeypad;
	delete fOptions;
	free(fKeypadDescription);
}


void
CalcView::MessageReceived(BMessage* message)
{
	//message->PrintToStream();
	
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
				if (be_clipboard->Lock()){ 
					BMessage *clipper = be_clipboard->Data();
					//clipper->PrintToStream();
					Paste(clipper);
					be_clipboard->Unlock(); 
					} // end if	
				break;
			
			// (replicant) about box requested
			case B_ABOUT_REQUESTED:
				AboutRequested();
				break;
			
			// calculator options window requested
			case K_OPTIONS_REQUESTED: {
				BRect frame(30.0f, 50.0f, 230.0, 200.0);
				fOptionsWindow = new CalcOptionsWindow(frame, fOptions);
				fOptionsWindow->Show();
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
	rgb_color rgbWhite = { 255, 255, 255, 0 };

	// calculate grid sizes
	float sizeDisp = fHeight * K_DISPLAY_YPROP;
	float sizeCol = fWidth / (float)fColums;
	float sizeRow = (fHeight - sizeDisp) / (float)fRows;
	
	// setup areas
	BRect displayRect(0.0, 0.0, fWidth, sizeDisp);
	BRect keypadRect(0.0, sizeDisp, fWidth, fHeight);
	
	SetFont(be_bold_font);
	
	// ****** DISPLAY Area
	if (updateRect.Intersects(displayRect)){
	
		// paint display b/g
		SetHighColor(fExpressionBGColor);
		FillRect(updateRect & displayRect);
		
		// render display text
		SetHighColor(rgbWhite);
		SetLowColor(fExpressionBGColor);
		SetDrawingMode(B_OP_COPY);
		SetFontSize(sizeDisp * K_FONT_YPROP);
		float baselineOffset = sizeDisp * (1.0 - K_FONT_YPROP) * 0.5;
		DrawString(fExpression.String(),
					BPoint(fWidth - StringWidth(fExpression.String()),
						sizeDisp - baselineOffset));
		
		} // end if
		
	
	// ****** KEYPAD Area
	if (updateRect.Intersects(keypadRect)){
	
		// paint keypad b/g
		SetHighColor(fBaseColor);
		FillRect(updateRect & keypadRect);
		
		// render key main grid
		BeginLineArray((fColums + fRows) << 1 + 1);
		
		// render cols
		AddLine(BPoint(0.0, sizeDisp),
			BPoint(0.0, fHeight),
			fLightColor);
		for (int col = 1; col < fColums; col++){
			AddLine(BPoint(col * sizeCol - 1.0, sizeDisp),
				BPoint(col * sizeCol - 1.0, fHeight),
				fDarkColor);
			AddLine(BPoint(col * sizeCol, sizeDisp),
				BPoint(col * sizeCol, fHeight),
				fLightColor);
			} // end for
		AddLine(BPoint(fColums * sizeCol, sizeDisp),
			BPoint(fColums * sizeCol, fHeight),
			fDarkColor);
			
		// render rows
		for (int row = 0; row < fRows; row++){
			AddLine(BPoint(0.0, sizeDisp + row * sizeRow - 1.0),
				BPoint(fWidth, sizeDisp + row * sizeRow - 1.0),
				fDarkColor);
			AddLine(BPoint(0.0, sizeDisp + row * sizeRow),
				BPoint(fWidth, sizeDisp + row * sizeRow),
				fLightColor);
			} // end for
		AddLine(BPoint(0.0, sizeDisp + fRows * sizeRow),
			BPoint(fWidth, sizeDisp + fRows * sizeRow),
			fDarkColor);
			
		// main grid complete
		EndLineArray();
		
		// render key symbols
		float halfSizeCol = sizeCol * 0.5f;
		SetHighColor(rgbWhite);
		SetLowColor(fBaseColor);
		SetDrawingMode(B_OP_COPY);
		SetFontSize(((fHeight - sizeDisp)/(float)fRows) * K_FONT_YPROP);
		float baselineOffset = ((fHeight - sizeDisp)/(float)fRows)
			* (1.0 - K_FONT_YPROP) * 0.5;
		CalcKey *key = fKeypad;
		for (int row = 0; row < fRows; row++){
			for (int col = 0; col < fColums; col++){
				float halfSymbolWidth = StringWidth(key->label) * 0.5f;
				DrawString(key->label,
					BPoint(col * sizeCol + halfSizeCol - halfSymbolWidth,
						sizeDisp + (row + 1) * sizeRow - baselineOffset));
				key++;
				} // end for
			} // end for

		} // end if
	
}


void
CalcView::MouseDown(BPoint point)
{
	// ensure this view is the current focus
	MakeFocus();

	// read mouse buttons state
	int32 buttons = 0;
	Window()->CurrentMessage()->FindInt32("buttons", &buttons);

	// display popup menu if not primary mouse button
	if ((B_PRIMARY_MOUSE_BUTTON & buttons) == 0) {
		BMenuItem* selected;
		if ((selected = fPopUpMenu->Go(ConvertToScreen(point))) != NULL)
			MessageReceived(selected->Message());
		return;
	}

	// calculate grid sizes
	float sizeDisp = fHeight * K_DISPLAY_YPROP;
	float sizeCol = fWidth / (float)fColums;
	float sizeRow = (fHeight - sizeDisp) / (float)fRows;
	
	// click on display, initiate drag if appropriate
	if ((point.y - sizeDisp) < 0.0) {
		// only drag if there's some text
		if (fExpression.Length() > 0){
			// assemble drag message
			BMessage dragmsg(B_MIME_DATA);
			dragmsg.AddData("text/plain",
							B_MIME_TYPE,
							fExpression.String(), 
							fExpression.Length());
				
			// initiate drag & drop
			SetFontSize(sizeDisp * K_FONT_YPROP);
			float left = fWidth;
			float textWidth = StringWidth(fExpression.String());
			if (textWidth < fWidth)
				left -= textWidth;
			else
				left = 0;
			BRect displayRect(left, 0.0, fWidth, sizeDisp);
			DragMessage(&dragmsg, displayRect);
		}
	} else {
		// click on keypad

		// calculate location within grid
		int gridCol = (int)(point.x / sizeCol);
		int gridRow = (int)((point.y - sizeDisp) / sizeRow);
		
		// check limits
		if ((gridCol >= 0) && (gridCol < fColums) &&
			(gridRow >= 0) && (gridRow < fRows)) {
		
			// process key press
			_PressKey(gridRow * fColums + gridCol);
		}
	}
}


void
CalcView::KeyDown(const char *bytes, int32 numBytes)
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
			case 'c':	// hack!
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
				int keys = fRows * fColums;
				for (int i=0; i<keys; i++) {
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
			set_keyboard_locks(B_NUM_LOCK |
							   (modifiers() & (B_CAPS_LOCK | B_SCROLL_LOCK)));
		}
	}

	// pass on request to parent
	BView::MakeFocus(focused);
}


void
CalcView::FrameResized(float width, float height)
{
	fWidth = width;
	fHeight = height;
}


void
CalcView::AboutRequested()
{
/*
	BRect frame(100.0, 100.0, 300.0, 280.0);
	new RAboutBox(frame, "DeskCalc v1.3.0", "code: timmy\nqa: rosalie\n\n"
		B_UTF8_COPYRIGHT"1997 R3 Software Ltd.\nAll Rights Reserved.",
		RAboutBox::R_SHOW_ON_OPEN);
*/
}


status_t
CalcView::Archive(BMessage* archive, bool deep) const
{
	// passed on request to parent
	status_t ret = BView::Archive(archive, deep);


	// save app signature for replicant add-on loading
    if (ret == B_OK)
    	ret = archive->AddString("add_on", kAppSig);

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
	fExpression.SetTo(""); // remove data

	_InvalidateExpression();
}


void
CalcView::Copy()
{
	// access system clipboard
	if (be_clipboard->Lock()) {
		be_clipboard->Clear();
		BMessage *clipper = be_clipboard->Data();
		clipper->what = B_MIME_DATA;
		// TODO: should check return for errors!
		clipper->AddData("text/plain",
						 B_MIME_TYPE,
						 fExpression.String(),
						 fExpression.Length());
		//clipper->PrintToStream();
		be_clipboard->Commit(); 
		be_clipboard->Unlock(); 
	}
}

 
void
CalcView::Paste(BMessage *message)
{
	// handle color drops first		
	// read incoming color
	const rgb_color* dropColor = NULL;
	ssize_t dataSize;
	if (message->FindData("RGBColor",
						  B_RGB_COLOR_TYPE,
						  (const void**)&dropColor,
						  &dataSize) == B_OK
		&& dataSize == sizeof(rgb_color)) {
			
		// calculate view relative drop point
		BPoint dropPoint = ConvertFromScreen(message->DropPoint());
		
		// calculate current keypad area
		float sizeDisp = fHeight * K_DISPLAY_YPROP;
		BRect keypadRect(0.0, sizeDisp, fWidth, fHeight);
		
		// check location of color drop
		if (keypadRect.Contains(dropPoint) && dropColor) {
			fBaseColor = *dropColor;
			_Colorize();
			// redraw keypad
			Invalidate(keypadRect);
		}

	} else {
		// look for text/plain MIME data
		const char* text;
		ssize_t numBytes;
		if (message->FindData("text/plain",
							  B_MIME_TYPE,
							  (const void**)&text,
							  &numBytes) == B_OK) {
			fExpression.Append(text, numBytes);
			_InvalidateExpression();
		}
	}
}


status_t
CalcView::LoadSettings(BMessage* archive)
{
	if (!archive)
		return B_BAD_VALUE;

	// record calculator description
	const char* calcDesc;
	if (archive->FindString("calcDesc", &calcDesc) < B_OK)
		calcDesc = kDefaultKeypadDescription;

	// save calculator description for reference
	free(fKeypadDescription);
	fKeypadDescription = strdup(calcDesc);

	// read grid dimensions
	if (archive->FindInt16("cols", &fColums) < B_OK)
		fColums = 5;
	if (archive->FindInt16("rows", &fRows) < B_OK)
		fRows = 4;
	
	// read color scheme
	const rgb_color* color;
	ssize_t size;
	if (archive->FindData("rgbBaseColor", B_RGB_COLOR_TYPE,
						  (const void**)&color, &size) < B_OK
						  || size != sizeof(rgb_color)) {
		fBaseColor = (rgb_color){ 128, 128, 128, 255 };
		puts("Missing rgbBaseColor from CalcView archive!\n");
	} else {
		fBaseColor = *color;
	}
		
	if (archive->FindData("rgbDisplay", B_RGB_COLOR_TYPE,
						  (const void**)&color, &size) < B_OK
						  || size != sizeof(rgb_color)) {
		fExpressionBGColor = (rgb_color){ 0, 0, 0, 255 };
		puts("Missing rgbBaseColor from CalcView archive!\n");
	} else {
		fExpressionBGColor = *color;
	}
	
	// load options
	const CalcOptions* options;
	if (archive->FindData("options", B_RAW_TYPE,
						  (const void**)&options, &size) == B_OK
		&& size == sizeof(CalcOptions)) {
		memcpy(fOptions, options, size);
	} else {
		puts("Missing options from CalcView archive.\n");
	}
	
	// load display text
	const char* display;
	if (archive->FindString("displayText", &display) < B_OK) {
		puts("Missing display text from CalcView archive.\n");
	} else {
		// init expression text
		fExpression = display;
	}

	// parse calculator description
	_ParseCalcDesc(fKeypadDescription);
	
	// colorize based on base color.
	_Colorize();

	return B_OK;
}


status_t
CalcView::SaveSettings(BMessage* archive) const
{
	status_t ret = archive ? B_OK : B_BAD_VALUE;

	// record grid dimensions
    if (ret == B_OK)
		ret = archive->AddInt16("cols", fColums);
    if (ret == B_OK)
		ret = archive->AddInt16("rows", fRows);
	
	// record color scheme
    if (ret == B_OK)
		ret = archive->AddData("rgbBaseColor", B_RGB_COLOR_TYPE,
							   &fBaseColor, sizeof(rgb_color));
	if (ret == B_OK)
		ret = archive->AddData("rgbDisplay", B_RGB_COLOR_TYPE,
							   &fExpressionBGColor, sizeof(rgb_color));

	// record current options
	if (ret == B_OK)
		ret = archive->AddData("options", B_RAW_TYPE,
							   fOptions, sizeof(CalcOptions));

	// record display text
	if (ret == B_OK)
		ret = archive->AddString("displayText", fExpression.String());

	// record calculator description
	if (ret == B_OK)
		ret = archive->AddString("calcDesc", fKeypadDescription);

	return ret;
}


// #pragma mark -


void
CalcView::_ParseCalcDesc(const char* keypadDescription)
{
	// TODO: should calculate dimensions from desc here!
	fKeypad = new CalcKey[fRows * fColums];

	// scan through calculator description and assemble keypad
	bool scanFlag = true;
	CalcKey *key = fKeypad;
	const char *p = keypadDescription;
	while (scanFlag) {
		// copy label
		char *l = key->label;
		while (!isspace(*p))
			*l++ = *p++;
		*l = '\0';
		
		// set code
		if (strcmp(key->label, "=") == 0)
			strcpy(key->code, "\n");
		else
			strcpy(key->code, key->label);
		
		// set keymap
		if (strlen(key->label)==1) {
			strcpy(key->keymap, key->label);
		} else {
			*key->keymap = '\0';
		} // end if
		
		// advance	
		while (isspace(*p))
			++p;
		key++;
		
		// check desc termination
		if (*p == '\0')
			scanFlag = false;
	}
}


void
CalcView::_PressKey(int key)
{
	assert(key < (fRows * fColums));
	assert(key >= 0);

	// check for backspace
	if (strcmp(fKeypad[key].label, "BS") == 0) {
		int32 length = fExpression.Length();
		if (length > 0) {
			fExpression.Remove(length - 1, 1);
		} else {
			beep();
			return; // no need to redraw
		}
	} else if (strcmp(fKeypad[key].label, "C") == 0) {
		// C means clear
		fExpression.SetTo("");
	} else {
		// append to display text
		fExpression.Append(fKeypad[key].code);
	
		// check for evaluation order
		if (fKeypad[key].code[0] == '\n') {
			_Evaluate();
		} else {
			// audio feedback
			if (fOptions->audio_feedback) {
				BEntry zimp("key.AIFF");
				entry_ref zimp_ref;
				zimp.GetRef(&zimp_ref);
				play_sound(&zimp_ref, true, false, true);
			}
		}
	}

	// redraw display
	_InvalidateExpression();
}


void
CalcView::_PressKey(char *label)
{
	int keys = fRows * fColums;
	for (int i = 0; i < keys; i++) {
		if (strcmp(fKeypad[i].label, label) == 0) {
			_PressKey(i);
			return;
		}
	}
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
}


void
CalcView::_Evaluate()
{
	const double EXP_SWITCH_HI = 1e12; // # digits to switch from std->exp form
	const double EXP_SWITCH_LO = 1e-12;

	if (fExpression.Length() == 0) {
		beep();
		return;
	}

	// audio feedback
	if (fOptions->audio_feedback) {
		BEntry zimp("zimp.AIFF");
		entry_ref zimp_ref;
		zimp.GetRef(&zimp_ref);
		play_sound(&zimp_ref, true, false, false);
	}

//printf("evaluate: %s\n", fExpression.String());

	// evaluate expression
	Expression parser;

	char* tmpstr = strdup(fExpression.String());
	char* start = tmpstr;
	char* end = start + fExpression.Length() - 1;
	double value = 0.0;

	try {
		value = parser.Eval(start, end, true);

	} catch (const char* error) {
		fExpression = error;
		_InvalidateExpression();
		return;
	}

	free(tmpstr);

//printf("  -> value: %f\n", value);

	// beautify the expression
	// TODO: see if this is necessary at all
	char buf[64];
	if (value == 0) {
		strcpy(buf, "0");
	} else if (((value <  EXP_SWITCH_HI) && (value >  EXP_SWITCH_LO)) ||
			   ((value > -EXP_SWITCH_HI) && (value < -EXP_SWITCH_LO))) {
		// print in std form
		sprintf(buf, "%9lf", value);

		// hack to remove surplus zeros!
		if (strchr(buf, '.')) {
			int32 i = strlen(buf) - 1;
			for (; i > 0; i--) {
				if (buf[i] == '0')
					buf[i] = '\0';
				else
					break;
			}
			if (buf[i] == '.')
				buf[i] = '\0';
		}
	} else {
		// print in exponential form
		sprintf(buf, "%le", value);
	}
		
	// render new result to display
	fExpression = buf;
	
	// redraw display
	_InvalidateExpression();
}


void
CalcView::_CreatePopUpMenu()
{
	// construct items
	fAboutItem = new BMenuItem("About Calculator...",
		new BMessage(B_ABOUT_REQUESTED));
	fOptionsItem = new BMenuItem("Options...",
		new BMessage(K_OPTIONS_REQUESTED));

	// construct menu
	fPopUpMenu = new BPopUpMenu("pop-up", false, false);
	fPopUpMenu->AddItem(fAboutItem);
	fPopUpMenu->AddItem(fOptionsItem);
}


void
CalcView::_InvalidateExpression()
{
	float sizeDisp = fHeight * K_DISPLAY_YPROP;
	BRect displayRect(0.0, 0.0, fWidth, sizeDisp);
	Invalidate(displayRect);
}

