/*
 * Copyright 2009-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "KeyboardLayoutView.h"

#include <stdio.h>
#include <stdlib.h>

#include <Beep.h>
#include <Bitmap.h>
#include <ControlLook.h>
#include <LayoutUtils.h>
#include <Region.h>
#include <Window.h>

#include "Keymap.h"


static const rgb_color kBrightColor = {230, 230, 230, 255};
static const rgb_color kDarkColor = {200, 200, 200, 255};
static const rgb_color kSecondDeadKeyColor = {240, 240, 150, 255};
static const rgb_color kDeadKeyColor = {152, 203, 255, 255};
static const rgb_color kLitIndicatorColor = {116, 212, 83, 255};


KeyboardLayoutView::KeyboardLayoutView(const char* name)
	:
	BView(name, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE | B_FRAME_EVENTS),
	fOffscreenBitmap(NULL),
	fKeymap(NULL),
	fEditable(true),
	fModifiers(0),
	fDeadKey(0),
	fButtons(0),
	fDragKey(NULL),
	fDropTarget(NULL),
	fOldSize(0, 0)
{
	fLayout = new KeyboardLayout;
	memset(fKeyState, 0, sizeof(fKeyState));

	SetEventMask(B_KEYBOARD_EVENTS);
}


KeyboardLayoutView::~KeyboardLayoutView()
{
	delete fOffscreenBitmap;
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
KeyboardLayoutView::SetBaseFont(const BFont& font)
{
	fBaseFont = font;

	font_height fontHeight;
	fBaseFont.GetHeight(&fontHeight);
	fBaseFontHeight = fontHeight.ascent + fontHeight.descent;
	fBaseFontSize = fBaseFont.Size();

	Invalidate();
}


void
KeyboardLayoutView::AttachedToWindow()
{
	SetViewColor(B_TRANSPARENT_COLOR);

	SetBaseFont(*be_plain_font);
	fSpecialFont = *be_fixed_font;
	fModifiers = modifiers();
}


void
KeyboardLayoutView::FrameResized(float width, float height)
{
	_InitOffscreen();
	_LayoutKeyboard();
}


void
KeyboardLayoutView::WindowActivated(bool active)
{
	if (active)
		Invalidate();
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
	fDragKey = NULL;
	fDropPoint.x = -1;

	Key* key = _KeyAt(point);
	if (key == NULL)
		return;

	int32 buttons = 0;
	if (Looper() != NULL && Looper()->CurrentMessage() != NULL)
		Looper()->CurrentMessage()->FindInt32("buttons", &buttons);

	if ((buttons & B_TERTIARY_MOUSE_BUTTON) != 0
		&& (fButtons & B_TERTIARY_MOUSE_BUTTON) == 0) {
		// toggle the "deadness" of dead keys via middle mouse button
		if (fKeymap != NULL) {
			bool isEnabled = false;
			uint8 deadKey
				= fKeymap->DeadKey(key->code, fModifiers, &isEnabled);
			if (deadKey > 0) {
				fKeymap->SetDeadKeyEnabled(key->code, fModifiers, !isEnabled);
				_InvalidateKey(key);
			}
		}
	} else {
		if (fKeymap != NULL && fKeymap->IsModifierKey(key->code)) {
			if (_KeyState(key->code)) {
				uint32 modifier = fKeymap->Modifier(key->code);
				if ((modifier & modifiers()) == 0) {
					_SetKeyState(key->code, false);
					fModifiers &= ~modifier;
					Invalidate();
				}
			} else {
				_SetKeyState(key->code, true);
				fModifiers |= fKeymap->Modifier(key->code);
				Invalidate();
			}

			// TODO: if possible, we could handle the lock keys for real
		} else {
			_SetKeyState(key->code, true);
			_InvalidateKey(key);
		}
	}

	fButtons = buttons;
}


void
KeyboardLayoutView::MouseUp(BPoint point)
{
	Key* key = _KeyAt(fClickPoint);

	int32 buttons = 0;
	if (Looper() != NULL && Looper()->CurrentMessage() != NULL)
		Looper()->CurrentMessage()->FindInt32("buttons", &buttons);

	if (key != NULL) {
		if ((fButtons & B_TERTIARY_MOUSE_BUTTON) != 0
			&& (buttons & B_TERTIARY_MOUSE_BUTTON) == 0) {
			_SetKeyState(key->code, false);
			_InvalidateKey(key);
			fButtons = buttons;
		} else {
			fButtons = buttons;

			// modifier keys are sticky when used with the mouse
			if (fKeymap != NULL && fKeymap->IsModifierKey(key->code))
				return;

			_SetKeyState(key->code, false);

			if (_HandleDeadKey(key->code, fModifiers) && fDeadKey != 0)
				return;

			_InvalidateKey(key);

			if (fDragKey == NULL && fKeymap != NULL) {
				// Send fake key down message to target
				_SendFakeKeyDown(key);
			}
		}
	}
	fDragKey = NULL;
}


void
KeyboardLayoutView::MouseMoved(BPoint point, uint32 transit,
	const BMessage* dragMessage)
{
	if (fKeymap == NULL)
		return;

	// prevent dragging for tertiary mouse button
	if ((fButtons & B_TERTIARY_MOUSE_BUTTON) != 0)
		return;

	if (dragMessage != NULL) {
		if (fEditable) {
			_InvalidateKey(fDropTarget);
			fDropPoint = point;

			_EvaluateDropTarget(point);
		}

		return;
	}

	int32 buttons;
	if (Window()->CurrentMessage() == NULL
		|| Window()->CurrentMessage()->FindInt32("buttons", &buttons) != B_OK
		|| buttons == 0)
		return;

	if (fDragKey == NULL && (fabs(point.x - fClickPoint.x) > 4
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
		BBitmap* bitmap = new BBitmap(rect, B_RGBA32, true);
		bitmap->Lock();

		BView* view = new BView(rect, "drag", B_FOLLOW_NONE, 0);
		bitmap->AddChild(view);

		view->SetHighColor(0, 0, 0, 0);
		view->FillRect(view->Bounds());
		view->SetDrawingMode(B_OP_ALPHA);
		view->SetHighColor(0, 0, 0, 128);
		// set the level of transparency by value
		view->SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_COMPOSITE);
		_DrawKey(view, frame, key, frame, false);

		view->Sync();
		bitmap->Unlock();

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
		fDragKey = key;
		fDragModifiers = fModifiers;

		fKeyState[key->code / 8] &= ~(1 << (7 - (key->code & 7)));
		_InvalidateKey(key);
	}
}


void
KeyboardLayoutView::Draw(BRect updateRect)
{
	if (fOldSize != BSize(Bounds().Width(), Bounds().Height())) {
		_InitOffscreen();
		_LayoutKeyboard();
	}

	BView* view;
	if (fOffscreenBitmap != NULL) {
		view = fOffscreenView;
		view->LockLooper();
	} else
		view = this;

	// Draw background

	if (Parent())
		view->SetLowColor(Parent()->ViewColor());
	else
		view->SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	view->FillRect(updateRect, B_SOLID_LOW);

	// Draw keys

	for (int32 i = 0; i < fLayout->CountKeys(); i++) {
		Key* key = fLayout->KeyAt(i);

		_DrawKey(view, updateRect, key, _FrameFor(key),
			_IsKeyPressed(key->code));
	}

	// Draw LED indicators

	for (int32 i = 0; i < fLayout->CountIndicators(); i++) {
		Indicator* indicator = fLayout->IndicatorAt(i);

		_DrawIndicator(view, updateRect, indicator, _FrameFor(indicator->frame),
			(fModifiers & indicator->modifier) != 0);
	}

	if (fOffscreenBitmap != NULL) {
		view->Sync();
		view->UnlockLooper();

		DrawBitmapAsync(fOffscreenBitmap, BPoint(0, 0));
	}
}


void
KeyboardLayoutView::MessageReceived(BMessage* message)
{
	if (message->WasDropped() && fEditable && fDropTarget != NULL
		&& fKeymap != NULL) {
		int32 keyCode;
		const char* data;
		ssize_t size;
		if (message->FindData("text/plain", B_MIME_DATA,
				(const void**)&data, &size) == B_OK) {
			// Automatically convert UTF-8 escaped strings (for example from
			// CharacterMap)
			int32 dataSize = 0;
			uint8 buffer[16];
			if (size > 3 && data[0] == '\\' && data[1] == 'x') {
				char tempBuffer[16];
				if (size > 15)
					size = 15;
				memcpy(tempBuffer, data, size);
				tempBuffer[size] = '\0';
				data = tempBuffer;

				while (size > 3 && data[0] == '\\' && data[1] == 'x') {
					buffer[dataSize++] = strtoul(&data[2], NULL, 16);
					if ((buffer[dataSize - 1] & 0x80) == 0)
						break;

					size -= 4;
					data += 4;
				}
				data = (const char*)buffer;
			} else if ((data[0] & 0xc0) != 0x80 && (data[0] & 0x80) != 0) {
				// only accept the first character UTF-8 character
				while (dataSize < size && (data[dataSize] & 0x80) != 0) {
					dataSize++;
				}
			} else if ((data[0] & 0x80) == 0) {
				// an ASCII character
				dataSize = 1;
			} else {
				// no valid character
				beep();
				return;
			}

			int32 buttons;
			if (!message->IsSourceRemote()
				&& message->FindInt32("buttons", &buttons) == B_OK
				&& (buttons & B_SECONDARY_MOUSE_BUTTON) != 0
				&& message->FindInt32("key", &keyCode) == B_OK) {
				// switch keys if the dropped object came from us
				Key* key = _KeyForCode(keyCode);
				if (key == NULL
					|| (key == fDropTarget && fDragModifiers == fModifiers))
					return;

				char* string;
				int32 numBytes;
				fKeymap->GetChars(fDropTarget->code, fModifiers, fDeadKey,
					&string, &numBytes);
				if (string != NULL) {
					// switch keys
					fKeymap->SetKey(fDropTarget->code, fModifiers, fDeadKey,
						(const char*)data, dataSize);
					fKeymap->SetKey(key->code, fDragModifiers, fDeadKey,
						string, numBytes);
					delete[] string;
				} else if (fKeymap->IsModifierKey(fDropTarget->code)) {
					// switch key with modifier
					fKeymap->SetModifier(key->code,
						fKeymap->Modifier(fDropTarget->code));
					fKeymap->SetKey(fDropTarget->code, fModifiers, fDeadKey,
						(const char*)data, dataSize);
				}
			} else {
				// Send the old key to the target, so it's not lost entirely
				_SendFakeKeyDown(fDropTarget);

				fKeymap->SetKey(fDropTarget->code, fModifiers, fDeadKey,
					(const char*)data, dataSize);
			}
		} else if (!message->IsSourceRemote()
			&& message->FindInt32("key", &keyCode) == B_OK) {
			// Switch an unmapped key

			Key* key = _KeyForCode(keyCode);
			if (key != NULL && key == fDropTarget)
				return;

			uint32 modifier = fKeymap->Modifier(keyCode);

			char* string;
			int32 numBytes;
			fKeymap->GetChars(fDropTarget->code, fModifiers, fDeadKey,
				&string, &numBytes);
			if (string != NULL) {
				// switch key with modifier
				fKeymap->SetModifier(fDropTarget->code, modifier);
				fKeymap->SetKey(keyCode, fDragModifiers, fDeadKey,
					string, numBytes);
				delete[] string;
			} else {
				// switch modifier keys
				fKeymap->SetModifier(keyCode,
					fKeymap->Modifier(fDropTarget->code));
				fKeymap->SetModifier(fDropTarget->code, modifier);
			}

			_InvalidateKey(fDragKey);
		}

		_InvalidateKey(fDropTarget);
		fDropTarget = NULL;
		fDropPoint.x = -1;
		return;
	}

	switch (message->what) {
		case B_UNMAPPED_KEY_DOWN:
		case B_UNMAPPED_KEY_UP:
			_KeyChanged(message);
			break;

		case B_MODIFIERS_CHANGED:
		{
			int32 newModifiers;
			if (message->FindInt32("modifiers", &newModifiers) == B_OK
				&& fModifiers != newModifiers) {
				fModifiers = newModifiers;
				_EvaluateDropTarget(fDropPoint);
				if (Window()->IsActive())
					Invalidate();
			}
			break;
		}

		default:
			BView::MessageReceived(message);
			break;
	}
}


void
KeyboardLayoutView::_InitOffscreen()
{
	delete fOffscreenBitmap;
	fOffscreenView = NULL;

	fOffscreenBitmap = new(std::nothrow) BBitmap(Bounds(),
		B_BITMAP_ACCEPTS_VIEWS, B_RGB32);
	if (fOffscreenBitmap != NULL && fOffscreenBitmap->IsValid()) {
		fOffscreenBitmap->Lock();
		fOffscreenView = new(std::nothrow) BView(Bounds(), "offscreen view",
			0, 0);
		if (fOffscreenView != NULL) {
			if (Parent() != NULL) {
				fOffscreenView->SetViewColor(Parent()->ViewColor());
			} else {
				fOffscreenView->SetViewColor(
					ui_color(B_PANEL_BACKGROUND_COLOR));
			}

			fOffscreenView->SetLowColor(fOffscreenView->ViewColor());
			fOffscreenBitmap->AddChild(fOffscreenView);
		}
		fOffscreenBitmap->Unlock();
	}

	if (fOffscreenView == NULL) {
		// something went wrong
		delete fOffscreenBitmap;
		fOffscreenBitmap = NULL;
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

	fOldSize.width = Bounds().Width();
	fOldSize.height = Bounds().Height();
}


void
KeyboardLayoutView::_DrawKeyButton(BView* view, BRect& rect, BRect updateRect,
	rgb_color base, rgb_color background, bool pressed)
{
	be_control_look->DrawButtonFrame(view, rect, updateRect, 4.0f, base,
		background, pressed ? BControlLook::B_ACTIVATED : 0);
	be_control_look->DrawButtonBackground(view, rect, updateRect, 4.0f,
		base, pressed ? BControlLook::B_ACTIVATED : 0);
}


void
KeyboardLayoutView::_DrawKey(BView* view, BRect updateRect, const Key* key,
	BRect rect, bool pressed)
{
	rgb_color base = key->dark ? kDarkColor : kBrightColor;
	rgb_color background = ui_color(B_PANEL_BACKGROUND_COLOR);
	key_kind keyKind = kNormalKey;
	int32 deadKey = 0;
	bool secondDeadKey = false;
	bool isDeadKeyEnabled = true;

	char text[32];
	if (fKeymap != NULL) {
		_GetKeyLabel(key, text, sizeof(text), keyKind);
		deadKey = fKeymap->DeadKey(key->code, fModifiers, &isDeadKeyEnabled);
		secondDeadKey = fKeymap->IsDeadSecondKey(key->code, fModifiers,
			fDeadKey);
	} else {
		// Show the key code if there is no keymap
		snprintf(text, sizeof(text), "%02lx", key->code);
	}

	_SetFontSize(view, keyKind);

	if (secondDeadKey)
		base = kSecondDeadKeyColor;
	else if (deadKey > 0 && isDeadKeyEnabled)
		base = kDeadKeyColor;

	if (key->shape == kRectangleKeyShape) {
		_DrawKeyButton(view, rect, updateRect, base, background, pressed);

		rect.InsetBy(1, 1);

		_GetAbbreviatedKeyLabelIfNeeded(view, rect, key, text, sizeof(text));
		be_control_look->DrawLabel(view, text, rect, updateRect,
			base, 0, BAlignment(B_ALIGN_CENTER, B_ALIGN_MIDDLE));
	} else if (key->shape == kEnterKeyShape) {
		BRegion region(rect);
		BRect originalRect = rect;
		BRect missingRect = rect;

		// TODO: for some reason, this does not always equal the bottom of
		// the other keys...
		missingRect.top = floorf(rect.top
			+ fLayout->DefaultKeySize().height * fFactor - fGap - 1);
		missingRect.right = floorf(missingRect.left
			+ (key->frame.Width() - key->second_row) * fFactor - fGap - 2);
		region.Exclude(missingRect);
		view->ConstrainClippingRegion(&region);

		be_control_look->DrawButtonFrame(view, rect, updateRect,
			4.0f, 4.0f, 0.0f, 0.0f, base, background,
			pressed ? BControlLook::B_ACTIVATED : 0);
		be_control_look->DrawButtonBackground(view, rect, updateRect,
			4.0f, 4.0f, 0.0f, 0.0f, base,
			pressed ? BControlLook::B_ACTIVATED : 0);

		rect.left = missingRect.right;
		_GetAbbreviatedKeyLabelIfNeeded(view, rect, key, text, sizeof(text));

		be_control_look->DrawLabel(view, text, rect, updateRect,
			base, 0, BAlignment(B_ALIGN_CENTER, B_ALIGN_MIDDLE));

		missingRect.right--;
		missingRect.top -= 2;
		region.Set(missingRect);
		view->ConstrainClippingRegion(&region);

		rect = originalRect;
		rect.bottom = missingRect.top + 2;

		be_control_look->DrawButtonFrame(view, rect, updateRect,
			0.0f, 0.0f, 4.0f, 0.0f, base, background,
			pressed ? BControlLook::B_ACTIVATED : 0,
			BControlLook::B_LEFT_BORDER | BControlLook::B_BOTTOM_BORDER);
		be_control_look->DrawButtonBackground(view, rect, updateRect,
			0.0f, 0.0f, 4.0f, 0.0f, base,
			pressed ? BControlLook::B_ACTIVATED : 0,
			BControlLook::B_LEFT_BORDER | BControlLook::B_BOTTOM_BORDER);

		missingRect.left = missingRect.right;
		missingRect.right++;
		missingRect.top += 2;
		region.Set(missingRect);
		view->ConstrainClippingRegion(&region);

		rect = originalRect;
		rect.left = missingRect.right - 2;
		rect.top = missingRect.top - 2;

		be_control_look->DrawButtonFrame(view, rect, updateRect,
			0.0f, 0.0f, 4.0f, 4.0f, base, background,
			pressed ? BControlLook::B_ACTIVATED : 0,
			BControlLook::B_LEFT_BORDER | BControlLook::B_RIGHT_BORDER
				| BControlLook::B_BOTTOM_BORDER);
		be_control_look->DrawButtonBackground(view, rect, updateRect,
			0.0f, 0.0f, 4.0f, 4.0f, base,
			pressed ? BControlLook::B_ACTIVATED : 0,
			BControlLook::B_LEFT_BORDER | BControlLook::B_RIGHT_BORDER
				| BControlLook::B_BOTTOM_BORDER);

		view->ConstrainClippingRegion(NULL);
	}
}


void
KeyboardLayoutView::_DrawIndicator(BView* view, BRect updateRect,
	const Indicator* indicator, BRect rect, bool lit)
{
	float rectTop = rect.top;
	rect.top += 2 * rect.Height() / 3;

	const char* label = NULL;
	if (indicator->modifier == B_CAPS_LOCK)
		label = "caps";
	else if (indicator->modifier == B_NUM_LOCK)
		label = "num";
	else if (indicator->modifier == B_SCROLL_LOCK)
		label = "scroll";
	if (label != NULL) {
		_SetFontSize(view, kIndicator);

		font_height fontHeight;
		GetFontHeight(&fontHeight);
		if (ceilf(rect.top - fontHeight.ascent + fontHeight.descent - 2)
				>= rectTop) {
			view->SetHighColor(0, 0, 0);
			view->SetLowColor(ViewColor());

			BString text(label);
			view->TruncateString(&text, B_TRUNCATE_END, rect.Width());
			view->DrawString(text.String(),
				BPoint(ceilf(rect.left + (rect.Width()
						- StringWidth(text.String())) / 2),
					ceilf(rect.top - fontHeight.descent - 2)));
		}
	}

	rect.left += rect.Width() / 4;
	rect.right -= rect.Width() / 3;

	rgb_color background = ui_color(B_PANEL_BACKGROUND_COLOR);
	rgb_color base = lit ? kLitIndicatorColor : kDarkColor;

	be_control_look->DrawButtonFrame(view, rect, updateRect, base,
		background, BControlLook::B_DISABLED);
	be_control_look->DrawButtonBackground(view, rect, updateRect,
		base, BControlLook::B_DISABLED);
}


const char*
KeyboardLayoutView::_SpecialKeyLabel(const key_map& map, uint32 code,
	bool abbreviated)
{
	if (code == map.caps_key)
		return abbreviated ? "CAPS" : "CAPS LOCK";
	if (code == map.scroll_key)
		return "SCROLL";
	if (code == map.num_key)
		return abbreviated ? "NUM" : "NUM LOCK";
	if (code == map.left_shift_key || code == map.right_shift_key)
		return "SHIFT";
	if (code == map.left_command_key || code == map.right_command_key)
		return abbreviated ? "CMD" : "COMMAND";
	if (code == map.left_control_key || code == map.right_control_key)
		return abbreviated ? "CTRL" : "CONTROL";
	if (code == map.left_option_key || code == map.right_option_key)
		return abbreviated ? "OPT" : "OPTION";
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
		return "\xe2\x8c\xab";

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
KeyboardLayoutView::_SpecialMappedKeyLabel(const char* bytes, size_t numBytes,
	bool abbreviated)
{
	if (numBytes != 1)
		return NULL;

	if (bytes[0] == B_ESCAPE)
		return "ESC";

	if (bytes[0] == B_INSERT)
		return "INS";
	if (bytes[0] == B_DELETE)
		return "DEL";
	if (bytes[0] == B_HOME)
		return "HOME";
	if (bytes[0] == B_END)
		return "END";
	if (bytes[0] == B_PAGE_UP)
		return abbreviated ? "PG \xe2\x86\x91" : "PAGE \xe2\x86\x91";
	if (bytes[0] == B_PAGE_DOWN)
		return abbreviated ? "PG \xe2\x86\x93" : "PAGE \xe2\x86\x93";

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
KeyboardLayoutView::_GetAbbreviatedKeyLabelIfNeeded(BView* view, BRect rect,
	const Key* key, char* text, size_t textSize)
{
	if (floorf(rect.Width()) > ceilf(view->StringWidth(text)))
		return;

	// Check if we have a shorter version of this key

	const key_map& map = fKeymap->Map();

	const char* special = _SpecialKeyLabel(map, key->code, true);
	if (special != NULL) {
		strlcpy(text, special, textSize);
		return;
	}

	char* bytes = NULL;
	int32 numBytes;
	fKeymap->GetChars(key->code, fModifiers, fDeadKey, &bytes, &numBytes);
	if (bytes != NULL) {
		special = _SpecialMappedKeyLabel(bytes, numBytes, true);
		if (special != NULL)
			strlcpy(text, special, textSize);

		delete[] bytes;
	}
}


void
KeyboardLayoutView::_GetKeyLabel(const Key* key, char* text, size_t textSize,
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
	fKeymap->GetChars(key->code, fModifiers, fDeadKey, &bytes, &numBytes);
	if (bytes != NULL) {
		special = _SpecialMappedKeyLabel(bytes, numBytes);
		if (special != NULL) {
			strlcpy(text, special, textSize);
			keyKind = kSpecialKey;
		} else {
			special = _SpecialMappedKeySymbol(bytes, numBytes);
			if (special != NULL) {
				strlcpy(text, special, textSize);
				keyKind = kSymbolKey;
			} else {
				bool hasGlyphs;
				fBaseFont.GetHasGlyphs(bytes, 1, &hasGlyphs);
				if (hasGlyphs)
					strlcpy(text, bytes, textSize);
			}
		}

		delete[] bytes;
	}
}


bool
KeyboardLayoutView::_IsKeyPressed(uint32 code)
{
	if (fDropTarget != NULL && fDropTarget->code == (int32)code)
		return true;

	return _KeyState(code);
}


bool
KeyboardLayoutView::_KeyState(uint32 code) const
{
	if (code >= 16 * 8)
		return false;

	return (fKeyState[code / 8] & (1 << (7 - (code & 7)))) != 0;
}


void
KeyboardLayoutView::_SetKeyState(uint32 code, bool pressed)
{
	if (code >= 16 * 8)
		return;

	if (pressed)
		fKeyState[code / 8] |= (1 << (7 - (code & 7)));
	else
		fKeyState[code / 8] &= ~(1 << (7 - (code & 7)));
}


Key*
KeyboardLayoutView::_KeyForCode(uint32 code)
{
	// TODO: have a lookup array

	for (int32 i = 0; i < fLayout->CountKeys(); i++) {
		Key* key = fLayout->KeyAt(i);
		if (key->code == (int32)code)
			return key;
	}

	return NULL;
}


void
KeyboardLayoutView::_InvalidateKey(uint32 code)
{
	_InvalidateKey(_KeyForCode(code));
}


void
KeyboardLayoutView::_InvalidateKey(const Key* key)
{
	if (key != NULL)
		Invalidate(_FrameFor(key));
}


/*!	Updates the fDeadKey member, and invalidates the view if needed.

	\return true if the view has been invalidated.
*/
bool
KeyboardLayoutView::_HandleDeadKey(uint32 key, int32 modifiers)
{
	if (fKeymap == NULL || fKeymap->IsModifierKey(key))
		return false;

	bool isEnabled = false;
	int32 deadKey = fKeymap->DeadKey(key, modifiers, &isEnabled);
	if (fDeadKey != deadKey) {
		if (isEnabled) {
			Invalidate();
			fDeadKey = deadKey;
			return true;
		}
	} else if (fDeadKey != 0) {
		Invalidate();
		fDeadKey = 0;
		return true;
	}

	return false;
}


void
KeyboardLayoutView::_KeyChanged(const BMessage* message)
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

		if (_KeyForCode(key) == NULL)
			printf("no key for code %ld\n", key);
	}

	for (int32 i = 0; i < 16; i++) {
		if (fKeyState[i] != state[i]) {
			uint8 diff = fKeyState[i] ^ state[i];
			fKeyState[i] = state[i];

			if (!checkSingle || !Window()->IsActive())
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
KeyboardLayoutView::_FrameFor(BRect keyFrame)
{
	BRect rect;
	rect.left   = ceilf(keyFrame.left * fFactor);
	rect.top    = ceilf(keyFrame.top * fFactor);
	rect.right  = floorf((keyFrame.Width()) * fFactor + rect.left - fGap - 1);
	rect.bottom = floorf((keyFrame.Height()) * fFactor + rect.top - fGap - 1);
	rect.OffsetBy(fOffset);

	return rect;
}


BRect
KeyboardLayoutView::_FrameFor(const Key* key)
{
	return _FrameFor(key->frame);
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
			fBaseFont.SetSize(fontSize);
			view->SetFont(&fBaseFont);
			break;
		case kSpecialKey:
			fSpecialFont.SetSize(fontSize * 0.7);
			view->SetFont(&fSpecialFont);
			break;
		case kSymbolKey:
			fSpecialFont.SetSize(fontSize * 1.6);
			view->SetFont(&fSpecialFont);
			break;

		case kIndicator:
		{
			BFont font;
			font.SetSize(fontSize * 0.8);
			view->SetFont(&font);
			break;
		}
	}
}


void
KeyboardLayoutView::_EvaluateDropTarget(BPoint point)
{
	fDropTarget = _KeyAt(point);
	if (fDropTarget != NULL) {
		if (fDropTarget == fDragKey && fModifiers == fDragModifiers)
			fDropTarget = NULL;
		else
			_InvalidateKey(fDropTarget);
	}
}


void
KeyboardLayoutView::_SendFakeKeyDown(const Key* key)
{
	BMessage message(B_KEY_DOWN);
	message.AddInt64("when", system_time());
	message.AddData("states", B_UINT8_TYPE, &fKeyState,
		sizeof(fKeyState));
	message.AddInt32("key", key->code);
	message.AddInt32("modifiers", fModifiers);
	message.AddPointer("keymap", fKeymap);

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
