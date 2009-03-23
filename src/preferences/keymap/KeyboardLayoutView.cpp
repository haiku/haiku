/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "KeyboardLayoutView.h"

#include <ControlLook.h>
#include <Window.h>

#include "Keymap.h"


KeyboardLayoutView::KeyboardLayoutView(const char* name)
	: BView(name, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE | B_FRAME_EVENTS),
	fKeymap(NULL),
	fModifiers(0),
	fDeadKey(0)
{
	fLayout = new KeyboardLayout;
	memset(fKeyState, 0, sizeof(fKeyState));

	SetEventMask(B_KEYBOARD_EVENTS);
}


KeyboardLayoutView::~KeyboardLayoutView()
{
}


void
KeyboardLayoutView::SetKeyboardLayout(KeyboardLayout* layout)
{
	fLayout = layout;
	Invalidate();
}


void
KeyboardLayoutView::SetKeymap(Keymap* keymap)
{
	fKeymap = keymap;
	Invalidate();
}


void
KeyboardLayoutView::AttachedToWindow()
{
	if (Parent())
		SetViewColor(Parent()->ViewColor());
	else
		SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fFont = *be_plain_font;
	fSpecialFont = *be_fixed_font;

	font_height fontHeight;
	fFont.GetHeight(&fontHeight);
	fFontHeight = fontHeight.ascent + fontHeight.descent;

	FrameResized(0, 0);
}


void
KeyboardLayoutView::FrameResized(float width, float height)
{
	float factorX = Bounds().Width() / fLayout->Bounds().Width();
	float factorY = Bounds().Height() / fLayout->Bounds().Height();

	fFactor = min_c(factorX, factorY);
	fOffset = BPoint((Bounds().Width() - fLayout->Bounds().Width()
			* fFactor) / 2,
		(Bounds().Height() - fLayout->Bounds().Height() * fFactor) / 2);
	fMaxFontSize = 14;
	fGap = 2;
}


#if 0
BSize
KeyboardLayoutView::MinSize()
{
	// TODO!
	BSize size(100, 100);
	return size;
}
#endif

void
KeyboardLayoutView::KeyDown(const char* bytes, int32 numBytes)
{
	_KeyChanged(Window()->CurrentMessage());
}


void
KeyboardLayoutView::KeyUp(const char* bytes, int32 numBytes)
{
	_KeyChanged(Window()->CurrentMessage());
}


void
KeyboardLayoutView::MouseDown(BPoint point)
{
}


void
KeyboardLayoutView::MouseUp(BPoint point)
{
}


void
KeyboardLayoutView::MouseMoved(BPoint point, uint32 transit,
	const BMessage* dragMessage)
{
}


void
KeyboardLayoutView::Draw(BRect updateRect)
{
	rgb_color brightColor = {230, 230, 230};
	rgb_color darkColor = {200, 200, 200};

	for (int32 i = 0; i < fLayout->CountKeys(); i++) {
		Key* key = fLayout->KeyAt(i);

		BRect rect = _FrameFor(key);
		rgb_color base = key->dark ? darkColor : brightColor;
		bool pressed = _IsKeyPressed(key->code);
		bool special = false;

		char text[32];
		if (fKeymap != NULL) {
			_GetKeyLabel(key, text, sizeof(text), special);
		} else {
			// Show the key code if there is no keymap
			snprintf(text, sizeof(text), "%02lx", key->code);
		}

		if (special) {
			fSpecialFont.SetSize(_FontSizeFor(rect, text) * 0.8);
			SetFont(&fSpecialFont);
		} else {
			fFont.SetSize(_FontSizeFor(rect, text));
			SetFont(&fFont);
		}

		if (key->shape == kRectangleKeyShape) {
			be_control_look->DrawButtonFrame(this, rect, updateRect, base,
				pressed ? BControlLook::B_ACTIVATED : 0);
			be_control_look->DrawButtonBackground(this, rect, updateRect, 
				base, pressed ? BControlLook::B_ACTIVATED : 0);

			// TODO: make font size depend on key size!
			rect.InsetBy(1, 1);

			be_control_look->DrawLabel(this, text, rect, updateRect,
				base, 0, BAlignment(B_ALIGN_CENTER, B_ALIGN_MIDDLE));
		} else if (key->shape == kEnterKeyShape) {
			// TODO: make better!
			rect.bottom -= 20;
			be_control_look->DrawButtonBackground(this, rect, updateRect, 
				key->dark ? darkColor : brightColor, 0,
				BControlLook::B_LEFT_BORDER | BControlLook::B_RIGHT_BORDER 
					| BControlLook::B_TOP_BORDER);

			rect = _FrameFor(key);
			rect.top += 20;
			rect.left += 10;
			be_control_look->DrawButtonBackground(this, rect, updateRect, 
				key->dark ? darkColor : brightColor, 0,
				BControlLook::B_LEFT_BORDER | BControlLook::B_RIGHT_BORDER
					| BControlLook::B_BOTTOM_BORDER);
		}
	}
}


void
KeyboardLayoutView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_UNMAPPED_KEY_DOWN:
		case B_UNMAPPED_KEY_UP:
			_KeyChanged(message);
			break;

		case B_MODIFIERS_CHANGED:
			if (message->FindInt32("modifiers", &fModifiers) == B_OK)
				Invalidate();
			break;

		default:
			BView::MessageReceived(message);
			break;
	}
}


const char*
KeyboardLayoutView::_ModifierKeyLabel(const key_map& map, uint32 code)
{
	if (code == map.caps_key)
		return "CAPS LOCK";
	if (code == map.scroll_key)
		return "SCROLL";
	if (code == map.num_key)
		return "NUM LOCK";
	if (code == map.left_shift_key || code == map.right_shift_key)
		return "SHIFT";
	if (code == map.left_command_key || code == map.right_command_key)
		return "COMMAND";
	if (code == map.left_control_key || code == map.right_control_key)
		return "CONTROL";
	if (code == map.left_option_key || code == map.right_option_key)
		return "OPTION";
	if (code == map.menu_key)
		return "MENU";

	return NULL;
}


const char*
KeyboardLayoutView::_SpecialKeyLabel(const char* bytes, size_t numBytes)
{
	if (numBytes != 1)
		return NULL;

	if (bytes[0] == B_ESCAPE)
		return "ESC";
	if (bytes[0] == B_TAB)
		return "\xe2\x86\xb9";
	if (bytes[0] == B_ENTER)
		return "\xe2\x86\xb5";
	if (bytes[0] == B_BACKSPACE)
		return "back";//\xe2\x86\x92";

	if (bytes[0] == B_INSERT)
		return "INS";
	if (bytes[0] == B_DELETE)
		return "DEL";
	if (bytes[0] == B_HOME)
		return "HOME";
	if (bytes[0] == B_END)
		return "END";
	if (bytes[0] == B_PAGE_UP)
		return "PAGE \xe2\x86\x91";
	if (bytes[0] == B_PAGE_DOWN)
		return "PAGE \xe2\x86\x93";

	if (bytes[0] == B_UP_ARROW)
		return "\xe2\x86\x91";
	if (bytes[0] == B_LEFT_ARROW)
		return "\xe2\x86\x90";
	if (bytes[0] == B_DOWN_ARROW)
		return "\xe2\x86\x93";
	if (bytes[0] == B_RIGHT_ARROW)
		return "\xe2\x86\x92";

	return NULL;
}


bool
KeyboardLayoutView::_FunctionKeyLabel(const char* bytes, size_t numBytes,
	char* text, size_t textSize)
{
	if (bytes[0] != B_FUNCTION_KEY)
		return false;

	if (bytes[1] >= B_F1_KEY && bytes[1] <= B_F12_KEY) {
		snprintf(text, textSize, "F%d", bytes[1] + 1 - B_F1_KEY);
		return true;
	}

	return false;
}


void
KeyboardLayoutView::_GetKeyLabel(Key* key, char* text, size_t textSize,
	bool& specialKey)
{
	const key_map& map = fKeymap->Map();
	specialKey = false;
	text[0] = '\0';

	const char* modifier = _ModifierKeyLabel(map, key->code);
	if (modifier != NULL) {
		strlcpy(text, modifier, textSize);
		specialKey = true;
		return;
	}

	char* bytes = NULL;
	int32 numBytes;
	fKeymap->GetChars(key->code, fModifiers, fDeadKey, &bytes,
		&numBytes);
	if (bytes != NULL) {
		const char* special = _SpecialKeyLabel(bytes, numBytes);
		if (special != NULL) {
			strlcpy(text, special, textSize);
			specialKey = true;
			return;
		}
		if (_FunctionKeyLabel(bytes, numBytes, text, textSize)) {
			specialKey = true;
			return;
		}

		bool hasGlyphs;
		fFont.GetHasGlyphs(bytes, 1, &hasGlyphs);
		if (hasGlyphs)
			strlcpy(text, bytes, sizeof(text));

		delete[] bytes;
	}
}


bool
KeyboardLayoutView::_IsKeyPressed(int32 code)
{
	if (code >= 16 * 8)
		return false;

	return (fKeyState[code / 8] & (1 << (7 - (code & 7)))) != 0;
}


Key*
KeyboardLayoutView::_KeyForCode(int32 code)
{
	// TODO: have a lookup array
	
	for (int32 i = 0; i < fLayout->CountKeys(); i++) {
		Key* key = fLayout->KeyAt(i);
		if (key->code == code)
			return key;
	}

	return NULL;
}


void
KeyboardLayoutView::_InvalidateKey(int32 code)
{
	Key* key = _KeyForCode(code);
	if (key != NULL)
		Invalidate(_FrameFor(key));
}


void
KeyboardLayoutView::_KeyChanged(BMessage* message)
{
	const uint8* state;
	ssize_t size;
	if (message->FindData("states", B_UINT8_TYPE, (const void**)&state, &size)
			!= B_OK)
		return;

	// Update key state, and invalidate change keys

	for (int32 i = 0; i < 16; i++) {
		if (fKeyState[i] != state[i]) {
			uint8 diff = fKeyState[i] ^ state[i];
			fKeyState[i] = state[i];

			for (int32 j = 7; diff != 0; j--, diff >>= 1) {
				if (diff & 1) {
					_InvalidateKey(i * 8 + j);
				}
			}
		}
	}
}


BRect
KeyboardLayoutView::_FrameFor(Key* key)
{
	BRect rect;
	rect.left = key->frame.left * fFactor;
	rect.right = (key->frame.right - key->frame.left) * fFactor + rect.left
		- fGap - 1;
	rect.top = key->frame.top * fFactor;
	rect.bottom = (key->frame.bottom - key->frame.top) * fFactor + rect.top
		- fGap - 1;
	rect.OffsetBy(fOffset);

	return rect;
}


float
KeyboardLayoutView::_FontSizeFor(BRect frame, const char* text)
{
#if 0
	//float stringWidth = fFont.StringWidth(text);
	// TODO: take width into account!

	float size = fFont.Size() * (frame.Height() - 6) / fFontHeight;
	if (size > fMaxFontSize)
		return fMaxFontSize;
#endif

	return 10.0f;
}
