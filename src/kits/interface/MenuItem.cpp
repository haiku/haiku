/*
 * Copyright 2001-2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stefano Ceccherini, stefano.ceccherini@gmail.com
 *		Marc Flerackers, mflerackers@androme.be
 *		Bill Hayden, haydentech@users.sourceforge.net
 *		Olivier Milla
 *		John Scipione, jscipione@gmail.com
 */


#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>

#include <Bitmap.h>
#include <ControlLook.h>
#include <MenuItem.h>
#include <Shape.h>
#include <String.h>
#include <Window.h>

#include <MenuPrivate.h>

#include "utf8_functions.h"


static const float kMarkTint = 0.75f;

// map control key shortcuts to drawable Unicode characters
// cf. http://unicode.org/charts/PDF/U2190.pdf
const char* kUTF8ControlMap[] = {
	NULL,
	"\xe2\x86\xb8", /* B_HOME U+21B8 */
	NULL, NULL,
	NULL, /* B_END */
	NULL, /* B_INSERT */
	NULL, NULL,
	NULL, /* B_BACKSPACE */
	"\xe2\x86\xb9", /* B_TAB U+21B9 */
	"\xe2\x8f\x8e", /* B_ENTER, U+23CE */
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

BMenuItem::BMenuItem(const char* label, BMessage* message, char shortcut,
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


BMenuItem::BMenuItem(BMenu* menu, BMessage* message)
{
	_InitData();
	SetMessage(message);
	_InitMenuData(menu);
}


BMenuItem::BMenuItem(BMessage* data)
{
	_InitData();

	if (data->HasString("_label")) {
		const char* string;

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
		BMessage* message = new BMessage;
		data->FindMessage("_msg", message);
		SetMessage(message);
	}

	BMessage subMessage;
	if (data->FindMessage("_submenu", &subMessage) == B_OK) {
		BArchivable* object = instantiate_object(&subMessage);
		if (object != NULL) {
			BMenu* menu = dynamic_cast<BMenu*>(object);
			if (menu != NULL)
				_InitMenuData(menu);
		}
	}
}


BArchivable*
BMenuItem::Instantiate(BMessage* data)
{
	if (validate_instantiation(data, "BMenuItem"))
		return new BMenuItem(data);

	return NULL;
}


status_t
BMenuItem::Archive(BMessage* data, bool deep) const
{
	status_t status = BArchivable::Archive(data, deep);

	if (status == B_OK && fLabel)
		status = data->AddString("_label", Label());

	if (status == B_OK && !IsEnabled())
		status = data->AddBool("_disable", true);

	if (status == B_OK && IsMarked())
		status = data->AddBool("_marked", true);

	if (status == B_OK && fUserTrigger)
		status = data->AddInt32("_user_trig", fUserTrigger);

	if (status == B_OK && fShortcutChar) {
		status = data->AddInt32("_shortcut", fShortcutChar);
		if (status == B_OK)
			status = data->AddInt32("_mods", fModifiers);
	}

	if (status == B_OK && Message() != NULL)
		status = data->AddMessage("_msg", Message());

	if (status == B_OK && deep && fSubmenu) {
		BMessage submenu;
		if (fSubmenu->Archive(&submenu, true) == B_OK)
			status = data->AddMessage("_submenu", &submenu);
	}

	return status;
}


BMenuItem::~BMenuItem()
{
	if (fSuper != NULL)
		fSuper->RemoveItem(this);

	free(fLabel);
	delete fSubmenu;
}


void
BMenuItem::SetLabel(const char* string)
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
BMenuItem::SetEnabled(bool enable)
{
	if (fEnabled == enable)
		return;

	fEnabled = enable;

	if (fSubmenu != NULL)
		fSubmenu->SetEnabled(enable);

	BMenu* menu = fSuper;
	if (menu != NULL && menu->LockLooper()) {
		menu->Invalidate(fBounds);
		menu->UnlockLooper();
	}
}


void
BMenuItem::SetMarked(bool mark)
{
	fMark = mark;

	if (mark && fSuper != NULL) {
		MenuPrivate priv(fSuper);
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
BMenuItem::SetShortcut(char shortcut, uint32 modifiers)
{
	if (fShortcutChar != 0 && (fModifiers & B_COMMAND_KEY) != 0
		&& fWindow != NULL) {
		fWindow->RemoveShortcut(fShortcutChar, fModifiers);
	}

	fShortcutChar = shortcut;

	if (shortcut != 0)
		fModifiers = modifiers | B_COMMAND_KEY;
	else
		fModifiers = 0;

	if (fShortcutChar != 0 && (fModifiers & B_COMMAND_KEY) && fWindow)
		fWindow->AddShortcut(fShortcutChar, fModifiers, this);

	if (fSuper != NULL) {
		fSuper->InvalidateLayout();

		if (fSuper->LockLooper()) {
			fSuper->Invalidate();
			fSuper->UnlockLooper();
		}
	}
}


const char*
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

	return fSuper != NULL ? fSuper->IsEnabled() : true;
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
BMenuItem::Shortcut(uint32* modifiers) const
{
	if (modifiers)
		*modifiers = fModifiers;

	return fShortcutChar;
}


BMenu*
BMenuItem::Submenu() const
{
	return fSubmenu;
}


BMenu*
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
BMenuItem::GetContentSize(float* _width, float* _height)
{
	// TODO: Get rid of this. BMenu should handle this
	// automatically. Maybe it's not even needed, since our
	// BFont::Height() caches the value locally
	MenuPrivate(fSuper).CacheFontInfo();

	fCachedWidth = fSuper->StringWidth(fLabel);

	if (_width)
		*_width = (float)ceil(fCachedWidth);
	if (_height)
		*_height = MenuPrivate(fSuper).FontHeight();
}


void
BMenuItem::TruncateLabel(float maxWidth, char* newLabel)
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

	fSuper->SetDrawingMode(B_OP_OVER);

	float labelWidth;
	float labelHeight;
	GetContentSize(&labelWidth, &labelHeight);

	const BRect& padding = menuPrivate.Padding();
	float maxContentWidth = fSuper->MaxContentWidth();
	float frameWidth = maxContentWidth > 0 ? maxContentWidth
		: fSuper->Frame().Width() - padding.left - padding.right;

	if (roundf(frameWidth) >= roundf(labelWidth))
		fSuper->DrawString(fLabel);
	else {
		// truncate label to fit
		char* truncatedLabel = new char[strlen(fLabel) + 4];
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
	const color_which lowColor = fSuper->LowUIColor();
	const color_which highColor = fSuper->HighUIColor();

	fSuper->SetLowColor(_LowColor());
	fSuper->SetHighColor(_HighColor());

	if (_IsActivated()) {
		// fill in the background
		BRect frame(Frame());
		be_control_look->DrawMenuItemBackground(fSuper, frame, frame,
			fSuper->LowColor(), BControlLook::B_ACTIVATED);
	}

	// draw content
	fSuper->MovePenTo(ContentLocation());
	DrawContent();

	// draw extra symbols
	MenuPrivate privateAccessor(fSuper);
	const menu_layout layout = privateAccessor.Layout();
	if (layout == B_ITEMS_IN_COLUMN) {
		if (IsMarked())
			_DrawMarkSymbol();

		if (fShortcutChar)
			_DrawShortcutSymbol(privateAccessor.HasSubmenus());

		if (Submenu() != NULL)
			_DrawSubmenuSymbol();
	}

	// restore the parent menu's low color and high color
	fSuper->SetLowUIColor(lowColor);
	fSuper->SetHighUIColor(highColor);
}


void
BMenuItem::Highlight(bool highlight)
{
	fSuper->Invalidate(Frame());
}


bool
BMenuItem::IsSelected() const
{
	return fSelected;
}


BPoint
BMenuItem::ContentLocation() const
{
	const BRect& padding = MenuPrivate(fSuper).Padding();

	return BPoint(fBounds.left + padding.left, fBounds.top + padding.top);
}


void BMenuItem::_ReservedMenuItem1() {}
void BMenuItem::_ReservedMenuItem2() {}
void BMenuItem::_ReservedMenuItem3() {}
void BMenuItem::_ReservedMenuItem4() {}


BMenuItem::BMenuItem(const BMenuItem &)
{
}


BMenuItem&
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
BMenuItem::_InitMenuData(BMenu* menu)
{
	fSubmenu = menu;

	MenuPrivate(fSubmenu).SetSuperItem(this);

	BMenuItem* item = menu->FindMarked();

	if (menu->IsRadioMode() && menu->IsLabelFromMarked() && item != NULL)
		SetLabel(item->Label());
	else
		SetLabel(menu->Name());
}


void
BMenuItem::Install(BWindow* window)
{
	if (fSubmenu != NULL)
		MenuPrivate(fSubmenu).Install(window);

	fWindow = window;

	if (fShortcutChar != 0 && (fModifiers & B_COMMAND_KEY) && fWindow)
		window->AddShortcut(fShortcutChar, fModifiers, this);

	if (!Messenger().IsValid())
		SetTarget(window);
}


status_t
BMenuItem::Invoke(BMessage* message)
{
	if (!IsEnabled())
		return B_ERROR;

	if (fSuper->IsRadioMode())
		SetMarked(true);

	bool notify = false;
	uint32 kind = InvokeKind(&notify);

	BMessage clone(kind);
	status_t err = B_BAD_VALUE;

	if (message == NULL && !notify)
		message = Message();

	if (message == NULL) {
		if (!fSuper->IsWatched())
			return err;
	} else
		clone = *message;

	clone.AddInt32("index", fSuper->IndexOf(this));
	clone.AddInt64("when", (int64)system_time());
	clone.AddPointer("source", this);
	clone.AddMessenger("be:sender", BMessenger(fSuper));

	if (message != NULL)
		err = BInvoker::Invoke(&clone);

//	TODO: assynchronous messaging
//	SendNotices(kind, &clone);

	return err;
}


void
BMenuItem::Uninstall()
{
	if (fSubmenu != NULL)
		MenuPrivate(fSubmenu).Uninstall();

	if (Target() == fWindow)
		SetTarget(BMessenger());

	if (fShortcutChar != 0 && (fModifiers & B_COMMAND_KEY) != 0
		&& fWindow != NULL) {
		fWindow->RemoveShortcut(fShortcutChar, fModifiers);
	}

	fWindow = NULL;
}


void
BMenuItem::SetSuper(BMenu* super)
{
	if (fSuper != NULL && super != NULL) {
		debugger("Error - can't add menu or menu item to more than 1 container"
			" (either menu or menubar).");
	}

	if (fSubmenu != NULL)
		MenuPrivate(fSubmenu).SetSuper(super);

	fSuper = super;
}


void
BMenuItem::Select(bool selected)
{
	if (fSelected == selected)
		return;

	if (Submenu() != NULL || IsEnabled()) {
		fSelected = selected;
		Highlight(selected);
	}
}


bool
BMenuItem::_IsActivated()
{
	return IsSelected() && (IsEnabled() || fSubmenu != NULL);
}


rgb_color
BMenuItem::_LowColor()
{
	return _IsActivated() ? ui_color(B_MENU_SELECTED_BACKGROUND_COLOR)
		: ui_color(B_MENU_BACKGROUND_COLOR);
}


rgb_color
BMenuItem::_HighColor()
{
	rgb_color highColor;

	bool isEnabled = IsEnabled();
	bool isSelected = IsSelected();

	if (isEnabled && isSelected)
		highColor = ui_color(B_MENU_SELECTED_ITEM_TEXT_COLOR);
	else if (isEnabled)
		highColor = ui_color(B_MENU_ITEM_TEXT_COLOR);
	else {
		rgb_color bgColor = fSuper->LowColor();
		if (bgColor.red + bgColor.green + bgColor.blue > 128 * 3)
			highColor = tint_color(bgColor, B_DISABLED_LABEL_TINT);
		else
			highColor = tint_color(bgColor, B_LIGHTEN_2_TINT);
	}

	return highColor;
}


void
BMenuItem::_DrawMarkSymbol()
{
	fSuper->PushState();

	BRect r(fBounds);
	float leftMargin;
	MenuPrivate(fSuper).GetItemMargins(&leftMargin, NULL, NULL, NULL);
	float gap = leftMargin / 4;
	r.right = r.left + leftMargin - gap;
	r.left += gap / 3;

	BPoint center(floorf((r.left + r.right) / 2.0),
		floorf((r.top + r.bottom) / 2.0));

	float size = std::min(r.Height() - 2, r.Width());
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

	fSuper->SetHighColor(tint_color(_HighColor(), kMarkTint));
	fSuper->SetDrawingMode(B_OP_OVER);
	fSuper->SetPenSize(2.0);
	// NOTE: StrokeShape() offsets the shape by the current pen position,
	// it is not documented in the BeBook, but it is true!
	fSuper->MovePenTo(B_ORIGIN);
	fSuper->StrokeShape(&arrowShape);

	fSuper->PopState();
}


void
BMenuItem::_DrawShortcutSymbol(bool submenus)
{
	BMenu* menu = fSuper;
	BFont font;
	menu->GetFont(&font);
	BPoint where = ContentLocation();
	// Start from the right and walk our way back
	where.x = fBounds.right - font.Size();

	// Leave space for the submenu arrow if any item in the menu has a submenu
	if (submenus)
		where.x -= fBounds.Height() / 2;

	const float ascent = MenuPrivate(fSuper).Ascent();
	if (fShortcutChar < B_SPACE && kUTF8ControlMap[(int)fShortcutChar])
		_DrawControlChar(fShortcutChar, where + BPoint(0, ascent));
	else
		fSuper->DrawChar(fShortcutChar, where + BPoint(0, ascent));

	where.y += (fBounds.Height() - 11) / 2 - 1;
	where.x -= 4;

	// TODO: It would be nice to draw these taking into account the text (low)
	// color.
	if ((fModifiers & B_COMMAND_KEY) != 0) {
		const BBitmap* command = MenuPrivate::MenuItemCommand();
		const BRect &rect = command->Bounds();
		where.x -= rect.Width() + 1;
		fSuper->DrawBitmap(command, where);
	}

	if ((fModifiers & B_CONTROL_KEY) != 0) {
		const BBitmap* control = MenuPrivate::MenuItemControl();
		const BRect &rect = control->Bounds();
		where.x -= rect.Width() + 1;
		fSuper->DrawBitmap(control, where);
	}

	if ((fModifiers & B_OPTION_KEY) != 0) {
		const BBitmap* option = MenuPrivate::MenuItemOption();
		const BRect &rect = option->Bounds();
		where.x -= rect.Width() + 1;
		fSuper->DrawBitmap(option, where);
	}

	if ((fModifiers & B_SHIFT_KEY) != 0) {
		const BBitmap* shift = MenuPrivate::MenuItemShift();
		const BRect &rect = shift->Bounds();
		where.x -= rect.Width() + 1;
		fSuper->DrawBitmap(shift, where);
	}
}


void
BMenuItem::_DrawSubmenuSymbol()
{
	fSuper->PushState();

	float symbolSize = roundf(Frame().Height() * 2 / 3);

	BRect rect(fBounds);
	rect.left = rect.right - symbolSize;

	// 14px by default, scaled with font size up to right margin - padding
	BRect symbolRect(0, 0, symbolSize, symbolSize);
	symbolRect.OffsetTo(BPoint(rect.left,
		fBounds.top + (fBounds.Height() - symbolSize) / 2));

	be_control_look->DrawArrowShape(Menu(), symbolRect, symbolRect,
		_HighColor(), BControlLook::B_RIGHT_ARROW, 0, kMarkTint);

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
