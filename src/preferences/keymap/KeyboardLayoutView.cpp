/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "KeyboardLayoutView.h"

#include <Bitmap.h>
#include <ControlLook.h>
#include <LayoutUtils.h>
#include <Window.h>

#include "Keymap.h"


static const rgb_color kBrightColor = {230, 230, 230, 255};
static const rgb_color kDarkColor = {200, 200, 200, 255};
static const rgb_color kSecondDeadKeyColor = {240, 240, 150, 255};
static const rgb_color kDeadKeyColor = {152, 203, 255, 255};


KeyboardLayoutView::KeyboardLayoutView(const char* name)
	: BView(name, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE | B_FRAME_EVENTS),
	fKeymap(NULL),
	fModifiers(0),
	fDeadKey(0),
	fIsDragging(false),
	fDropTarget(NULL)
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
	_LayoutKeyboard();
	Invalidate();
}


void
KeyboardLayoutView::SetKeymap(Keymap* keymap)
{
	fKeymap = keymap;
	Invalidate();
}


void
KeyboardLayoutView::SetTarget(BMessenger target)
{
	fTarget = target;
}


void
KeyboardLayoutView::SetFont(const BFont& font)
{
	fFont = font;

	font_height fontHeight;
	fFont.GetHeight(&fontHeight);
	fBaseFontHeight = fontHeight.ascent + fontHeight.descent;
	fBaseFontSize = fFont.Size();

	Invalidate();
}


void
KeyboardLayoutView::AttachedToWindow()
{
	if (Parent())
		SetViewColor(Parent()->ViewColor());
	else
		SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	SetFont(*be_plain_font);
	fSpecialFont = *be_fixed_font;

	_LayoutKeyboard();
}


void
KeyboardLayoutView::FrameResized(float width, float height)
{
	_LayoutKeyboard();
}


BSize
KeyboardLayoutView::MinSize()
{
	return BLayoutUtils::ComposeSize(ExplicitMinSize(), BSize(100, 50));
}


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
	fClickPoint = point;
	fIsDragging = false;

	Key* key = _KeyAt(point);
	if (key != NULL) {
		fKeyState[key->code / 8] |= (1 << (7 - (key->code & 7)));
		_InvalidateKey(key);
	}
}


void
KeyboardLayoutView::MouseUp(BPoint point)
{
	fDropTarget = NULL;

	Key* key = _KeyAt(fClickPoint);
	if (key != NULL) {
		fKeyState[key->code / 8] &= ~(1 << (7 - (key->code & 7)));

		if (fKeymap != NULL && _HandleDeadKey(key->code, fModifiers)
			&& fDeadKey != 0)
			return;

		_InvalidateKey(key);

		if (!fIsDragging && fKeymap != NULL) {
			// Send fake key down message to target
			BMessage message(B_KEY_DOWN);
			message.AddInt64("when", system_time());
			message.AddData("states", B_UINT8_TYPE, &fKeyState,
				sizeof(fKeyState));
			message.AddInt32("key", key->code);
			message.AddInt32("modifiers", fModifiers);

			char* string;
			int32 numBytes;
			fKeymap->GetChars(key->code, fModifiers, fDeadKey, &string,
				&numBytes);
			if (string != NULL) {
				message.AddString("bytes", string);
				delete[] string;
			}

			fKeymap->GetChars(key->code, 0, 0, &string, &numBytes);
			if (string != NULL) {
				message.AddInt32("raw_char", string[0]);
				message.AddInt8("byte", string[0]);
				delete[] string;
			}

			fTarget.SendMessage(&message);
		}
	}
}


void
KeyboardLayoutView::MouseMoved(BPoint point, uint32 transit,
	const BMessage* dragMessage)
{
	if (fKeymap == NULL)
		return;

	if (dragMessage != NULL) {
		_InvalidateKey(fDropTarget);

		fDropTarget = _KeyAt(point);
		if (fDropTarget != NULL)
			_InvalidateKey(fDropTarget);
	} else if (!fIsDragging && (fabs(point.x - fClickPoint.x) > 4
		|| fabs(point.y - fClickPoint.y) > 4)) {
		// start dragging
		Key* key = _KeyAt(fClickPoint);
		if (key == NULL)
			return;

		BRect frame = _FrameFor(key);
		BPoint offset = fClickPoint - frame.LeftTop();
		frame.OffsetTo(B_ORIGIN);

		BRect rect = frame;
		rect.right--;
		rect.bottom--;
		BBitmap* bitmap = new BBitmap(rect, B_BITMAP_ACCEPTS_VIEWS, B_RGBA32);
		bitmap->Lock();

		BView* view = new BView(rect, "drag", 0, 0);
		bitmap->AddChild(view);

		_DrawKey(view, frame, key, frame, false);

		view->Sync();
		bitmap->RemoveChild(view);
		bitmap->Unlock();

		// Make it transparent
		// TODO: is there a better way to do this?
		uint8* bits = (uint8*)bitmap->Bits();
		for (int32 i = 0; i < bitmap->BitsLength(); i += 4) {
			bits[i + 3] = 144;
		}

		BMessage drag(B_MIME_DATA);
		drag.AddInt32("key", key->code);

		char* string;
		int32 numBytes;
		fKeymap->GetChars(key->code, fModifiers, fDeadKey, &string,
			&numBytes);
		if (string != NULL) {
			drag.AddData("text/plain", B_MIME_DATA, string, numBytes);
			delete[] string;
		}

		DragMessage(&drag, bitmap, B_OP_ALPHA, offset);
		fIsDragging = true;

		fKeyState[key->code / 8] &= ~(1 << (7 - (key->code & 7)));
		_InvalidateKey(key);
	}
}


void
KeyboardLayoutView::Draw(BRect updateRect)
{
	for (int32 i = 0; i < fLayout->CountKeys(); i++) {
		Key* key = fLayout->KeyAt(i);

		_DrawKey(this, updateRect, key, _FrameFor(key),
			_IsKeyPressed(key->code));
	}
}


void
KeyboardLayoutView::MessageReceived(BMessage* message)
{
	if (message->WasDropped()) {
		fDropTarget = NULL;

		Key* key = _KeyAt(ConvertFromScreen(message->DropPoint()));
		if (key != NULL && fKeymap != NULL) {
			const char* data;
			ssize_t size;
			if (message->FindData("text/plain", B_MIME_DATA,
					(const void**)&data, &size) == B_OK) {
				// ignore trailing null bytes
				if (data[size - 1] == '\0')
					size--;

				fKeymap->SetKey(key->code, fModifiers, fDeadKey,
					(const char*)data, size);
				_InvalidateKey(key);
			}
		}
	}

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


void
KeyboardLayoutView::_LayoutKeyboard()
{
	float factorX = Bounds().Width() / fLayout->Bounds().Width();
	float factorY = Bounds().Height() / fLayout->Bounds().Height();

	fFactor = min_c(factorX, factorY);
	fOffset = BPoint((Bounds().Width() - fLayout->Bounds().Width()
			* fFactor) / 2,
		(Bounds().Height() - fLayout->Bounds().Height() * fFactor) / 2);

	if (fLayout->DefaultKeySize().width < 11)
		fGap = 1;
	else
		fGap = 2;
}


void
KeyboardLayoutView::_DrawKey(BView* view, BRect updateRect, Key* key,
	BRect rect, bool pressed)
{
	rgb_color base = key->dark ? kDarkColor : kBrightColor;
	key_kind keyKind = kNormalKey;
	int32 deadKey = 0;
	bool secondDeadKey;

	char text[32];
	if (fKeymap != NULL) {
		_GetKeyLabel(key, text, sizeof(text), keyKind);
		deadKey = fKeymap->IsDeadKey(key->code, fModifiers);
		secondDeadKey = fKeymap->IsDeadSecondKey(key->code, fModifiers,
			fDeadKey);
	} else {
		// Show the key code if there is no keymap
		snprintf(text, sizeof(text), "%02lx", key->code);
	}

	_SetFontSize(view, keyKind);

	if (secondDeadKey)
		base = kSecondDeadKeyColor;
	else if (deadKey > 0)
		base = kDeadKeyColor;

	if (key->shape == kRectangleKeyShape) {
		be_control_look->DrawButtonFrame(view, rect, updateRect, base,
			pressed ? BControlLook::B_ACTIVATED : 0);
		be_control_look->DrawButtonBackground(view, rect, updateRect, 
			base, pressed ? BControlLook::B_ACTIVATED : 0);

		// TODO: make font size depend on key size!
		rect.InsetBy(1, 1);

		be_control_look->DrawLabel(view, text, rect, updateRect,
			base, 0, BAlignment(B_ALIGN_CENTER, B_ALIGN_MIDDLE));
	} else if (key->shape == kEnterKeyShape) {
		// TODO: make better!
		rect.bottom -= 20;
		be_control_look->DrawButtonBackground(view, rect, updateRect, 
			base, 0, BControlLook::B_LEFT_BORDER
				| BControlLook::B_RIGHT_BORDER 
				| BControlLook::B_TOP_BORDER);

		rect = _FrameFor(key);
		rect.top += 20;
		rect.left += 10;
		be_control_look->DrawButtonBackground(view, rect, updateRect, 
			base, 0, BControlLook::B_LEFT_BORDER
				| BControlLook::B_RIGHT_BORDER
				| BControlLook::B_BOTTOM_BORDER);
	}
}


const char*
KeyboardLayoutView::_SpecialKeyLabel(const key_map& map, uint32 code)
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
	if (code == B_PRINT_KEY)
		return "PRINT";
	if (code == B_PAUSE_KEY)
		return "PAUSE";

	return NULL;
}


const char*
KeyboardLayoutView::_SpecialMappedKeySymbol(const char* bytes, size_t numBytes)
{
	if (numBytes != 1)
		return NULL;

	if (bytes[0] == B_TAB)
		return "\xe2\x86\xb9";
	if (bytes[0] == B_ENTER)
		return "\xe2\x86\xb5";
	if (bytes[0] == B_BACKSPACE)
		return "back";//\xe2\x86\x92";

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


const char*
KeyboardLayoutView::_SpecialMappedKeyLabel(const char* bytes, size_t numBytes)
{
	if (numBytes != 1)
		return NULL;

	if (bytes[0] == B_ESCAPE)
		return "ESC";
	if (bytes[0] == B_BACKSPACE)
		return "BACKSPACE";

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

	return NULL;
}


bool
KeyboardLayoutView::_FunctionKeyLabel(uint32 code, char* text, size_t textSize)
{
	if (code >= B_F1_KEY && code <= B_F12_KEY) {
		snprintf(text, textSize, "F%ld", code + 1 - B_F1_KEY);
		return true;
	}

	return false;
}


void
KeyboardLayoutView::_GetKeyLabel(Key* key, char* text, size_t textSize,
	key_kind& keyKind)
{
	const key_map& map = fKeymap->Map();
	keyKind = kNormalKey;
	text[0] = '\0';

	const char* special = _SpecialKeyLabel(map, key->code);
	if (special != NULL) {
		strlcpy(text, special, textSize);
		keyKind = kSpecialKey;
		return;
	}

	if (_FunctionKeyLabel(key->code, text, textSize)) {
		keyKind = kSpecialKey;
		return;
	}

	char* bytes = NULL;
	int32 numBytes;
	fKeymap->GetChars(key->code, fModifiers, fDeadKey, &bytes,
		&numBytes);
	if (bytes != NULL) {
		special = _SpecialMappedKeyLabel(bytes, numBytes);
		if (special != NULL) {
			strlcpy(text, special, textSize);
			keyKind = kSpecialKey;
			return;
		}

		special = _SpecialMappedKeySymbol(bytes, numBytes);
		if (special != NULL) {
			strlcpy(text, special, textSize);
			keyKind = kSymbolKey;
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

	if (fDropTarget != NULL && fDropTarget->code == code)
		return true;

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
	_InvalidateKey(_KeyForCode(code));
}


void
KeyboardLayoutView::_InvalidateKey(Key* key)
{
	if (key != NULL)
		Invalidate(_FrameFor(key));
}


/*!	Updates the fDeadKey member, and invalidates the view if needed.
	fKeymap must be valid when calling this method.

	\return true if the view has been invalidated.
*/
bool
KeyboardLayoutView::_HandleDeadKey(int32 key, int32 modifiers)
{
	int32 deadKey = fKeymap->IsDeadKey(key, modifiers);
	if (fDeadKey != deadKey) {
		Invalidate();
		fDeadKey = deadKey;
		return true;
	} else if (fDeadKey != 0) {
		Invalidate();
		fDeadKey = 0;
		return true;
	}

	return false;
}


void
KeyboardLayoutView::_KeyChanged(BMessage* message)
{
	const uint8* state;
	ssize_t size;
	int32 key;
	if (message->FindData("states", B_UINT8_TYPE, (const void**)&state, &size)
			!= B_OK
		|| message->FindInt32("key", &key) != B_OK)
		return;

	// Update key state, and invalidate change keys

	bool checkSingle = true;

	if (message->what == B_KEY_DOWN || message->what == B_UNMAPPED_KEY_DOWN) {
		if (_HandleDeadKey(key, fModifiers))
			checkSingle = false;
	}

	for (int32 i = 0; i < 16; i++) {
		if (fKeyState[i] != state[i]) {
			uint8 diff = fKeyState[i] ^ state[i];
			fKeyState[i] = state[i];

			if (!checkSingle)
				continue;

			for (int32 j = 7; diff != 0; j--, diff >>= 1) {
				if (diff & 1) {
					_InvalidateKey(i * 8 + j);
				}
			}
		}
	}
}


Key*
KeyboardLayoutView::_KeyAt(BPoint point)
{
	// Find key candidate

	BPoint keyPoint = point;
	keyPoint -= fOffset;
	keyPoint.x /= fFactor;
	keyPoint.y /= fFactor;

	for (int32 i = 0; i < fLayout->CountKeys(); i++) {
		Key* key = fLayout->KeyAt(i);
		if (key->frame.Contains(keyPoint)) {
			BRect frame = _FrameFor(key);
			if (frame.Contains(point))
				return key;

			return NULL;
		}
	}

	return NULL;
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


void
KeyboardLayoutView::_SetFontSize(BView* view, key_kind keyKind)
{
	BSize size = fLayout->DefaultKeySize();
	float fontSize = fBaseFontSize;
	if (fBaseFontHeight >= size.height * fFactor * 0.5) {
		fontSize *= (size.height * fFactor * 0.5) / fBaseFontHeight;
		if (fontSize < 8)
			fontSize = 8;
	}

	switch (keyKind) {
		case kNormalKey:
			fFont.SetSize(fontSize);
			view->SetFont(&fFont);
			break;
		case kSpecialKey:
			fSpecialFont.SetSize(fontSize * 0.7);
			view->SetFont(&fSpecialFont);
			break;
		case kSymbolKey:
			fSpecialFont.SetSize(fontSize * 1.6);
			view->SetFont(&fSpecialFont);
			break;
	}
}
