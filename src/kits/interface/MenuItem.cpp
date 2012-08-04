/*
 * Copyright 2001-2008, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Bill Hayden (haydentech@users.sourceforge.net)
 *		Stefano Ceccherini (stefano.ceccherini@gmail.com)
 *		Olivier Milla
 */

//!	Display item for BMenu class

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <Bitmap.h>
#include <ControlLook.h>
#include <MenuItem.h>
#include <Shape.h>
#include <String.h>
#include <Window.h>

#include <MenuPrivate.h>

#include "utf8_functions.h"


const float kLightBGTint = (B_LIGHTEN_1_TINT + B_LIGHTEN_1_TINT + B_NO_TINT) / 3.0;

// map control key shortcuts to drawable Unicode characters
// cf. http://unicode.org/charts/PDF/U2190.pdf
const char *kUTF8ControlMap[] = {
	NULL,
	"\xe2\x86\xb8", /* B_HOME U+21B8 */
	NULL, NULL,
	NULL, /* B_END */
	NULL, /* B_INSERT */
	NULL, NULL,
	NULL, /* B_BACKSPACE */
	"\xe2\x86\xb9", /* B_TAB U+21B9 */
	"\xe2\x86\xb5", /* B_ENTER, U+21B5 */
	//"\xe2\x8f\x8e", /* B_ENTER, U+23CE it's the official one */
	NULL, /* B_PAGE_UP */
	NULL, /* B_PAGE_DOWN */
	NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	"\xe2\x86\x90", /* B_LEFT_ARROW */
	"\xe2\x86\x92", /* B_RIGHT_ARROW */
	"\xe2\x86\x91", /* B_UP_ARROW */
	"\xe2\x86\x93", /* B_DOWN_ARROW */
};

using BPrivate::MenuPrivate;

BMenuItem::BMenuItem(const char *label, BMessage *message, char shortcut,
					 uint32 modifiers)
{
	_InitData();
	if (label != NULL)
		fLabel = strdup(label);

	SetMessage(message);

	fShortcutChar = shortcut;

	if (shortcut != 0)
		fModifiers = modifiers | B_COMMAND_KEY;
	else
		fModifiers = 0;
}


BMenuItem::BMenuItem(BMenu *menu, BMessage *message)
{
	_InitData();
	SetMessage(message);
	_InitMenuData(menu);
}


BMenuItem::BMenuItem(BMessage *data)
{
	_InitData();

	if (data->HasString("_label")) {
		const char *string;

		data->FindString("_label", &string);
		SetLabel(string);
	}

	bool disable;
	if (data->FindBool("_disable", &disable) == B_OK)
		SetEnabled(!disable);

	bool marked;
	if (data->FindBool("_marked", &marked) == B_OK)
		SetMarked(marked);

	int32 userTrigger;
	if (data->FindInt32("_user_trig", &userTrigger) == B_OK)
		SetTrigger(userTrigger);

	if (data->HasInt32("_shortcut")) {
		int32 shortcut, mods;

		data->FindInt32("_shortcut", &shortcut);
		data->FindInt32("_mods", &mods);

		SetShortcut(shortcut, mods);
	}

	if (data->HasMessage("_msg")) {
		BMessage *msg = new BMessage;
		data->FindMessage("_msg", msg);
		SetMessage(msg);
	}

	BMessage subMessage;
	if (data->FindMessage("_submenu", &subMessage) == B_OK) {
		BArchivable *object = instantiate_object(&subMessage);
		if (object != NULL) {
			BMenu *menu = dynamic_cast<BMenu *>(object);
			if (menu != NULL)
				_InitMenuData(menu);
		}
	}
}


BArchivable *
BMenuItem::Instantiate(BMessage *data)
{
	if (validate_instantiation(data, "BMenuItem"))
		return new BMenuItem(data);

	return NULL;
}


status_t
BMenuItem::Archive(BMessage *data, bool deep) const
{
	status_t ret = BArchivable::Archive(data, deep);

	if (ret == B_OK && fLabel)
		ret = data->AddString("_label", Label());

	if (ret == B_OK && !IsEnabled())
		ret = data->AddBool("_disable", true);

	if (ret == B_OK && IsMarked())
		ret = data->AddBool("_marked", true);

	if (ret == B_OK && fUserTrigger)
		ret = data->AddInt32("_user_trig", fUserTrigger);

	if (ret == B_OK && fShortcutChar) {
		ret = data->AddInt32("_shortcut", fShortcutChar);
		if (ret == B_OK)
			ret = data->AddInt32("_mods", fModifiers);
	}

	if (ret == B_OK && Message())
		ret = data->AddMessage("_msg", Message());

	if (ret == B_OK && deep && fSubmenu) {
		BMessage submenu;
		if (fSubmenu->Archive(&submenu, true) == B_OK)
			ret = data->AddMessage("_submenu", &submenu);
	}

	return ret;
}


BMenuItem::~BMenuItem()
{
	free(fLabel);
	delete fSubmenu;
}


void
BMenuItem::SetLabel(const char *string)
{
	if (fLabel != NULL) {
		free(fLabel);
		fLabel = NULL;
	}

	if (string != NULL)
		fLabel = strdup(string);

	if (fSuper != NULL) {
		fSuper->InvalidateLayout();

		if (fSuper->LockLooper()) {
			fSuper->Invalidate();
			fSuper->UnlockLooper();
		}
	}
}


void
BMenuItem::SetEnabled(bool state)
{
	if (fEnabled == state)
		return;

	fEnabled = state;

	if (fSubmenu != NULL)
		fSubmenu->SetEnabled(state);

	BMenu *menu = Menu();
	if (menu != NULL && menu->LockLooper()) {
		menu->Invalidate(fBounds);
		menu->UnlockLooper();
	}
}


void
BMenuItem::SetMarked(bool state)
{
	fMark = state;

	if (state && Menu() != NULL) {
		MenuPrivate priv(Menu());
		priv.ItemMarked(this);
	}
}


void
BMenuItem::SetTrigger(char trigger)
{
	fUserTrigger = trigger;

	// try uppercase letters first

	const char* pos = strchr(Label(), toupper(trigger));
	trigger = tolower(trigger);

	if (pos == NULL) {
		// take lowercase, too
		pos = strchr(Label(), trigger);
	}

	if (pos != NULL) {
		fTriggerIndex = UTF8CountChars(Label(), pos - Label());
		fTrigger = trigger;
	} else {
		fTrigger = 0;
		fTriggerIndex = -1;
	}

	if (fSuper != NULL)
		fSuper->InvalidateLayout();
}


void
BMenuItem::SetShortcut(char ch, uint32 modifiers)
{
	if (fShortcutChar != 0 && (fModifiers & B_COMMAND_KEY) && fWindow)
		fWindow->RemoveShortcut(fShortcutChar, fModifiers);

	fShortcutChar = ch;

	if (ch != 0)
		fModifiers = modifiers | B_COMMAND_KEY;
	else
		fModifiers = 0;

	if (fShortcutChar != 0 && (fModifiers & B_COMMAND_KEY) && fWindow)
		fWindow->AddShortcut(fShortcutChar, fModifiers, this);

	if (fSuper) {
		fSuper->InvalidateLayout();

		if (fSuper->LockLooper()) {
			fSuper->Invalidate();
			fSuper->UnlockLooper();
		}
	}
}


const char *
BMenuItem::Label() const
{
	return fLabel;
}


bool
BMenuItem::IsEnabled() const
{
	if (fSubmenu)
		return fSubmenu->IsEnabled();

	if (!fEnabled)
		return false;

	return fSuper ? fSuper->IsEnabled() : true;
}


bool
BMenuItem::IsMarked() const
{
	return fMark;
}


char
BMenuItem::Trigger() const
{
	return fUserTrigger;
}


char
BMenuItem::Shortcut(uint32 *modifiers) const
{
	if (modifiers)
		*modifiers = fModifiers;

	return fShortcutChar;
}


BMenu *
BMenuItem::Submenu() const
{
	return fSubmenu;
}


BMenu *
BMenuItem::Menu() const
{
	return fSuper;
}


BRect
BMenuItem::Frame() const
{
	return fBounds;
}


void
BMenuItem::GetContentSize(float *width, float *height)
{
	// TODO: Get rid of this. BMenu should handle this
	// automatically. Maybe it's not even needed, since our
	// BFont::Height() caches the value locally
	MenuPrivate(fSuper).CacheFontInfo();

	fCachedWidth = fSuper->StringWidth(fLabel);

	if (width)
		*width = (float)ceil(fCachedWidth);
	if (height) {
		*height = MenuPrivate(fSuper).FontHeight();
	}
}


void
BMenuItem::TruncateLabel(float maxWidth, char *newLabel)
{
	BFont font;
	fSuper->GetFont(&font);

	BString string(fLabel);

	font.TruncateString(&string, B_TRUNCATE_MIDDLE, maxWidth);

	string.CopyInto(newLabel, 0, string.Length());
	newLabel[string.Length()] = '\0';
}


void
BMenuItem::DrawContent()
{
	MenuPrivate menuPrivate(fSuper);
	menuPrivate.CacheFontInfo();

	fSuper->MovePenBy(0, menuPrivate.Ascent());
	BPoint lineStart = fSuper->PenLocation();

	float labelWidth, labelHeight;
	GetContentSize(&labelWidth, &labelHeight);

	fSuper->SetDrawingMode(B_OP_OVER);

	float frameWidth = fBounds.Width();
	if (menuPrivate.State() == MENU_STATE_CLOSED) {
		float rightMargin, leftMargin;
		menuPrivate.GetItemMargins(&leftMargin, NULL, &rightMargin, NULL);
		frameWidth = fSuper->Frame().Width() - (rightMargin + leftMargin);
	}

	// truncate if needed
	if (frameWidth >= labelWidth)
		fSuper->DrawString(fLabel);
	else {
		char *truncatedLabel = new char[strlen(fLabel) + 4];
		TruncateLabel(frameWidth, truncatedLabel);
		fSuper->DrawString(truncatedLabel);
		delete[] truncatedLabel;
	}

	if (fSuper->AreTriggersEnabled() && fTriggerIndex != -1) {
		float escapements[fTriggerIndex + 1];
		BFont font;
		fSuper->GetFont(&font);

		font.GetEscapements(fLabel, fTriggerIndex + 1, escapements);

		for (int32 i = 0; i < fTriggerIndex; i++)
			lineStart.x += escapements[i] * font.Size();

		lineStart.x--;
		lineStart.y++;

		BPoint lineEnd(lineStart);
		lineEnd.x += escapements[fTriggerIndex] * font.Size();

		fSuper->StrokeLine(lineStart, lineEnd);
	}
}


void
BMenuItem::Draw()
{
	rgb_color lowColor = fSuper->LowColor();

	bool enabled = IsEnabled();
	bool selected = IsSelected();

	// set low color and fill background if selected
	bool activated = selected && (enabled || Submenu())
		/*&& fSuper->fRedrawAfterSticky*/;
	if (activated) {
		if (be_control_look != NULL) {
			BRect rect = Frame();
			be_control_look->DrawMenuItemBackground(fSuper, rect, rect,
				ui_color(B_MENU_SELECTED_BACKGROUND_COLOR),
				BControlLook::B_ACTIVATED);
		} else {
			fSuper->SetLowColor(ui_color(B_MENU_SELECTED_BACKGROUND_COLOR));
			fSuper->FillRect(Frame(), B_SOLID_LOW);
		}
	}

	// set high color
	if (activated)
		fSuper->SetHighColor(ui_color(B_MENU_SELECTED_ITEM_TEXT_COLOR));
	else if (enabled)
		fSuper->SetHighColor(ui_color(B_MENU_ITEM_TEXT_COLOR));
	else {
		// TODO: Use a lighten tint if the menu uses a dark background
		fSuper->SetHighColor(tint_color(lowColor, B_DISABLED_LABEL_TINT));
	}

	// draw content
	fSuper->MovePenTo(ContentLocation());
	DrawContent();

	// draw extra symbols
	const menu_layout layout = MenuPrivate(fSuper).Layout();
	if (layout == B_ITEMS_IN_COLUMN) {
		if (IsMarked())
			_DrawMarkSymbol();

		if (fShortcutChar)
			_DrawShortcutSymbol();

		if (Submenu())
			_DrawSubmenuSymbol();
	}

	fSuper->SetLowColor(lowColor);
}


void
BMenuItem::Highlight(bool flag)
{
	Menu()->Invalidate(Frame());
}


bool
BMenuItem::IsSelected() const
{
	return fSelected;
}


BPoint
BMenuItem::ContentLocation() const
{
	const BRect &padding = MenuPrivate(fSuper).Padding();

	return BPoint(fBounds.left + padding.left,
		fBounds.top + padding.top);
}


void BMenuItem::_ReservedMenuItem1() {}
void BMenuItem::_ReservedMenuItem2() {}
void BMenuItem::_ReservedMenuItem3() {}
void BMenuItem::_ReservedMenuItem4() {}


BMenuItem::BMenuItem(const BMenuItem &)
{
}


BMenuItem &
BMenuItem::operator=(const BMenuItem &)
{
	return *this;
}


void
BMenuItem::_InitData()
{
	fLabel = NULL;
	fSubmenu = NULL;
	fWindow = NULL;
	fSuper = NULL;
	fModifiers = 0;
	fCachedWidth = 0;
	fTriggerIndex = -1;
	fUserTrigger = 0;
	fTrigger = 0;
	fShortcutChar = 0;
	fMark = false;
	fEnabled = true;
	fSelected = false;
}


void
BMenuItem::_InitMenuData(BMenu *menu)
{
	fSubmenu = menu;

	MenuPrivate(fSubmenu).SetSuperItem(this);

	BMenuItem *item = menu->FindMarked();

	if (menu->IsRadioMode() && menu->IsLabelFromMarked() && item != NULL)
		SetLabel(item->Label());
	else
		SetLabel(menu->Name());
}


void
BMenuItem::Install(BWindow *window)
{
	if (fSubmenu) {
		MenuPrivate(fSubmenu).Install(window);
	}

	fWindow = window;

	if (fShortcutChar != 0 && (fModifiers & B_COMMAND_KEY) && fWindow)
		window->AddShortcut(fShortcutChar, fModifiers, this);

	if (!Messenger().IsValid())
		SetTarget(window);
}


status_t
BMenuItem::Invoke(BMessage *message)
{
	if (!IsEnabled())
		return B_ERROR;

	if (fSuper->IsRadioMode())
		SetMarked(true);

	bool notify = false;
	uint32 kind = InvokeKind(&notify);

	BMessage clone(kind);
	status_t err = B_BAD_VALUE;

	if (!message && !notify)
		message = Message();

	if (!message) {
		if (!fSuper->IsWatched())
			return err;
	} else
		clone = *message;

	clone.AddInt32("index", Menu()->IndexOf(this));
	clone.AddInt64("when", (int64)system_time());
	clone.AddPointer("source", this);
	clone.AddMessenger("be:sender", BMessenger(fSuper));

	if (message)
		err = BInvoker::Invoke(&clone);

//	TODO: assynchronous messaging
//	SendNotices(kind, &clone);

	return err;
}


void
BMenuItem::Uninstall()
{
	if (fSubmenu != NULL) {
		MenuPrivate(fSubmenu).Uninstall();
	}

	if (Target() == fWindow)
		SetTarget(BMessenger());

	if (fShortcutChar != 0 && (fModifiers & B_COMMAND_KEY) != 0
		&& fWindow != NULL)
		fWindow->RemoveShortcut(fShortcutChar, fModifiers);

	fWindow = NULL;
}


void
BMenuItem::SetSuper(BMenu *super)
{
	if (fSuper != NULL && super != NULL)
		debugger("Error - can't add menu or menu item to more than 1 container (either menu or menubar).");

	if (fSubmenu != NULL) {
		MenuPrivate(fSubmenu).SetSuper(super);
	}

	fSuper = super;
}


void
BMenuItem::Select(bool selected)
{
	if (fSelected == selected)
		return;

	if (Submenu() || IsEnabled()) {
		fSelected = selected;
		Highlight(selected);
	}
}


void
BMenuItem::_DrawMarkSymbol()
{
	fSuper->PushState();

	BRect r(fBounds);
	float leftMargin;
	MenuPrivate(fSuper).GetItemMargins(&leftMargin, NULL, NULL, NULL);
	r.right = r.left + leftMargin - 3;
	r.left += 1;

	BPoint center(floorf((r.left + r.right) / 2.0),
		floorf((r.top + r.bottom) / 2.0));

	float size = min_c(r.Height() - 2, r.Width());
	r.top = floorf(center.y - size / 2 + 0.5);
	r.bottom = floorf(center.y + size / 2 + 0.5);
	r.left = floorf(center.x - size / 2 + 0.5);
	r.right = floorf(center.x + size / 2 + 0.5);

	BShape arrowShape;
	center.x += 0.5;
	center.y += 0.5;
	size *= 0.3;
	arrowShape.MoveTo(BPoint(center.x - size, center.y - size * 0.25));
	arrowShape.LineTo(BPoint(center.x - size * 0.25, center.y + size));
	arrowShape.LineTo(BPoint(center.x + size, center.y - size));

	fSuper->SetDrawingMode(B_OP_OVER);
	fSuper->SetPenSize(2.0);
	// NOTE: StrokeShape() offsets the shape by the current pen position,
	// it is not documented in the BeBook, but it is true!
	fSuper->MovePenTo(B_ORIGIN);
	fSuper->StrokeShape(&arrowShape);

	fSuper->PopState();
}


void
BMenuItem::_DrawShortcutSymbol()
{
	BMenu *menu = Menu();
	BFont font;
	menu->GetFont(&font);
	BPoint where = ContentLocation();
	where.x = fBounds.right - font.Size();

	if (fSubmenu)
		where.x -= fBounds.Height() - 3;

	const float ascent = MenuPrivate(fSuper).Ascent();
	if (fShortcutChar < B_SPACE && kUTF8ControlMap[(int)fShortcutChar])
		_DrawControlChar(fShortcutChar, where + BPoint(0, ascent));
	else
		fSuper->DrawChar(fShortcutChar, where + BPoint(0, ascent));

	where.y += (fBounds.Height() - 11) / 2 - 1;
	where.x -= 4;

	// TODO: It would be nice to draw these taking into account the text (low)
	// color.
	if (fModifiers & B_COMMAND_KEY) {
		const BBitmap *command = MenuPrivate::MenuItemCommand();
		const BRect &rect = command->Bounds();
		where.x -= rect.Width() + 1;
		fSuper->DrawBitmap(command, where);
	}

	if (fModifiers & B_CONTROL_KEY) {
		const BBitmap *control = MenuPrivate::MenuItemControl();
		const BRect &rect = control->Bounds();
		where.x -= rect.Width() + 1;
		fSuper->DrawBitmap(control, where);
	}

	if (fModifiers & B_OPTION_KEY) {
		const BBitmap *option = MenuPrivate::MenuItemOption();
		const BRect &rect = option->Bounds();
		where.x -= rect.Width() + 1;
		fSuper->DrawBitmap(option, where);
	}

	if (fModifiers & B_SHIFT_KEY) {
		const BBitmap *shift = MenuPrivate::MenuItemShift();
		const BRect &rect = shift->Bounds();
		where.x -= rect.Width() + 1;
		fSuper->DrawBitmap(shift, where);
	}
}


void
BMenuItem::_DrawSubmenuSymbol()
{
	fSuper->PushState();

	BRect r(fBounds);
	float rightMargin;
	MenuPrivate(fSuper).GetItemMargins(NULL, NULL, &rightMargin, NULL);
	r.left = r.right - rightMargin + 3;
	r.right -= 1;

	BPoint center(floorf((r.left + r.right) / 2.0),
		floorf((r.top + r.bottom) / 2.0));

	float size = min_c(r.Height() - 2, r.Width());
	r.top = floorf(center.y - size / 2 + 0.5);
	r.bottom = floorf(center.y + size / 2 + 0.5);
	r.left = floorf(center.x - size / 2 + 0.5);
	r.right = floorf(center.x + size / 2 + 0.5);

	BShape arrowShape;
	center.x += 0.5;
	center.y += 0.5;
	size *= 0.25;
	float hSize = size * 0.7;
	arrowShape.MoveTo(BPoint(center.x - hSize, center.y - size));
	arrowShape.LineTo(BPoint(center.x + hSize, center.y));
	arrowShape.LineTo(BPoint(center.x - hSize, center.y + size));

	fSuper->SetDrawingMode(B_OP_OVER);
	fSuper->SetPenSize(ceilf(size * 0.4));
	// NOTE: StrokeShape() offsets the shape by the current pen position,
	// it is not documented in the BeBook, but it is true!
	fSuper->MovePenTo(B_ORIGIN);
	fSuper->StrokeShape(&arrowShape);

	fSuper->PopState();
}


void
BMenuItem::_DrawControlChar(char shortcut, BPoint where)
{
	// TODO: If needed, take another font for the control characters
	//	(or have font overlays in the app_server!)
	const char* symbol = " ";
	if (kUTF8ControlMap[(int)fShortcutChar])
		symbol = kUTF8ControlMap[(int)fShortcutChar];

	fSuper->DrawString(symbol, where);
}


void
BMenuItem::SetAutomaticTrigger(int32 index, uint32 trigger)
{
	fTriggerIndex = index;
	fTrigger = trigger;
}
