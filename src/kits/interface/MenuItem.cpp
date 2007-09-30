/*
 * Copyright 2001-2007, Haiku, Inc.
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
#include <MenuItem.h>
#include <Shape.h>
#include <String.h>
#include <Window.h>

#include "utf8_functions.h"

const unsigned char kCtrlBits[] = {
	0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x14,
	0x1d,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x17,0x14,
	0x1d,0x1a,0x13,0x04,0x04,0x13,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x17,0x14,
	0x1d,0x1a,0x04,0x1a,0x1a,0x04,0x1a,0x04,0x04,0x04,0x1a,0x04,0x1a,0x1a,0x1a,0x17,0x14,
	0x1d,0x1a,0x04,0x1a,0x1a,0x1a,0x1a,0x1a,0x04,0x1a,0x1a,0x04,0x1a,0x1a,0x1a,0x17,0x14,
	0x1d,0x1a,0x04,0x1a,0x1a,0x1a,0x1a,0x1a,0x04,0x1a,0x1a,0x04,0x1a,0x1a,0x1a,0x17,0x14,
	0x1d,0x1a,0x04,0x1a,0x1a,0x04,0x1a,0x1a,0x04,0x1a,0x1a,0x04,0x1a,0x1a,0x1a,0x17,0x14,
	0x1d,0x1a,0x13,0x04,0x04,0x13,0x1a,0x1a,0x04,0x1a,0x1a,0x04,0x04,0x04,0x1a,0x17,0x14,
	0x1d,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x17,0x14,
	0x1d,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x14,
	0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14
};


const unsigned char kAltBits[] = {
	0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x14,
	0x1d,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x17,0x14,
	0x1d,0x1a,0x1a,0x13,0x04,0x04,0x13,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x17,0x14,
	0x1d,0x1a,0x1a,0x04,0x1a,0x1a,0x04,0x1a,0x04,0x1a,0x1a,0x04,0x04,0x04,0x1a,0x17,0x14,
	0x1d,0x1a,0x1a,0x04,0x1a,0x1a,0x04,0x1a,0x04,0x1a,0x1a,0x1a,0x04,0x1a,0x1a,0x17,0x14,
	0x1d,0x1a,0x1a,0x04,0x04,0x04,0x04,0x1a,0x04,0x1a,0x1a,0x1a,0x04,0x1a,0x1a,0x17,0x14,
	0x1d,0x1a,0x1a,0x04,0x1a,0x1a,0x04,0x1a,0x04,0x1a,0x1a,0x1a,0x04,0x1a,0x1a,0x17,0x14,
	0x1d,0x1a,0x1a,0x04,0x1a,0x1a,0x04,0x1a,0x04,0x04,0x04,0x1a,0x04,0x1a,0x1a,0x17,0x14,
	0x1d,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x17,0x14,
	0x1d,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x14,
	0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14
};


const unsigned char kShiftBits[] = {
	0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x14,
	0x1d,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x17,0x14,
	0x1d,0x1a,0x1a,0x17,0x04,0x04,0x17,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x17,0x14,
	0x1d,0x1a,0x1a,0x04,0x17,0x17,0x04,0x1a,0x04,0x1a,0x04,0x1a,0x04,0x04,0x04,0x1a,0x04,0x04,0x04,0x1a,0x17,0x14,
	0x1d,0x1a,0x1a,0x17,0x04,0x04,0x17,0x1a,0x04,0x1a,0x04,0x1a,0x04,0x1a,0x1a,0x1a,0x1a,0x04,0x1a,0x1a,0x17,0x14,
	0x1d,0x1a,0x1a,0x1a,0x1a,0x1a,0x04,0x1a,0x04,0x04,0x04,0x1a,0x04,0x04,0x1a,0x1a,0x1a,0x04,0x1a,0x1a,0x17,0x14,
	0x1d,0x1a,0x1a,0x04,0x17,0x17,0x04,0x1a,0x04,0x1a,0x04,0x1a,0x04,0x1a,0x1a,0x1a,0x1a,0x04,0x1a,0x1a,0x17,0x14,
	0x1d,0x1a,0x1a,0x17,0x04,0x04,0x17,0x1a,0x04,0x1a,0x04,0x1a,0x04,0x1a,0x1a,0x1a,0x1a,0x04,0x1a,0x1a,0x17,0x14,
	0x1d,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x17,0x14,
	0x1d,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x14,
	0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14
};

const float kLightBGTint = (B_LIGHTEN_1_TINT + B_LIGHTEN_1_TINT + B_NO_TINT) / 3.0;


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

	if (state && Menu() != NULL)
		Menu()->_ItemMarked(this);
}


void
BMenuItem::SetTrigger(char trigger)
{
	fUserTrigger = trigger;

	// try uppercase letters first

	const char* pos = strchr(Label(), toupper(trigger));
	if (pos == NULL) {
		// take lowercase, too
		pos = strchr(Label(), trigger);
	}
	if (pos != NULL) {
		fTriggerIndex = pos - Label();
		fTrigger = tolower(UTF8ToCharCode(&pos));
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
	fSuper->_CacheFontInfo();

	fCachedWidth = fSuper->StringWidth(fLabel);

	if (width)
		*width = (float)ceil(fCachedWidth);
	if (height)
		*height = fSuper->fFontHeight;
}


void
BMenuItem::TruncateLabel(float maxWidth, char *newLabel)
{
	BFont font;
	BString string(fLabel);

	font.TruncateString(&string, B_TRUNCATE_MIDDLE, maxWidth);

	string.CopyInto(newLabel, 0, string.Length());
	newLabel[string.Length()] = '\0';
}


void
BMenuItem::DrawContent()
{
	fSuper->_CacheFontInfo();

	fSuper->MovePenBy(0, fSuper->fAscent);
	BPoint lineStart = fSuper->PenLocation();

	float labelWidth, labelHeight;
	GetContentSize(&labelWidth, &labelHeight);		

	// truncate if needed
	// TODO: Actually, this is still never triggered
	if (fBounds.Width() > labelWidth)
		fSuper->DrawString(fLabel);
	else {
		char *truncatedLabel = new char[strlen(fLabel) + 4];
		TruncateLabel(fBounds.Width(), truncatedLabel);
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
	bool enabled = IsEnabled();
	bool selected = IsSelected();

//	rgb_color noTint = ui_color(B_MENU_BACKGROUND_COLOR);
// TODO: the above is currently broken, because ui_color is
// not informed of changes to the app_server palette yet
	rgb_color noTint = fSuper->LowColor();
	rgb_color bgColor = noTint;

	// set low color and fill background if selected
	if (selected && (enabled || Submenu()) /*&& fSuper->fRedrawAfterSticky*/) {
//		bgColor = ui_color(B_MENU_SELECTED_BACKGROUND_COLOR);
// see above
		bgColor = tint_color(bgColor, B_DARKEN_3_TINT);
		fSuper->SetLowColor(bgColor);
		fSuper->FillRect(Frame(), B_SOLID_LOW);
	} else
		fSuper->SetLowColor(bgColor);

	// set high color
	if (enabled)
		fSuper->SetHighColor(ui_color(B_MENU_ITEM_TEXT_COLOR));
	else
		fSuper->SetHighColor(tint_color(bgColor, B_DISABLED_LABEL_TINT));

	// draw content
	fSuper->MovePenTo(ContentLocation());
	DrawContent();

	// draw extra symbols
	if (fSuper->Layout() == B_ITEMS_IN_COLUMN) {
		if (IsMarked())
			_DrawMarkSymbol(bgColor);

		if (fShortcutChar)
			_DrawShortcutSymbol();

		if (Submenu())
			_DrawSubmenuSymbol(bgColor);
	}

	fSuper->SetLowColor(noTint);
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
	return BPoint(fBounds.left + Menu()->fPad.left,
		fBounds.top + Menu()->fPad.top);
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
	fSubmenu->fSuperitem = this;

	BMenuItem *item = menu->FindMarked();

	if (menu->IsRadioMode() && menu->IsLabelFromMarked() && item != NULL)
		SetLabel(item->Label());
	else
		SetLabel(menu->Name());
}


void
BMenuItem::Install(BWindow *window)
{
	if (fSubmenu)
		fSubmenu->_Install(window);

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
	if (fSubmenu != NULL)
		fSubmenu->_Uninstall();

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
		if (super)
			super->fSubmenus++;
		else if (fSuper)
			fSuper->fSubmenus--;	
		fSubmenu->fSuper = super;
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
BMenuItem::_DrawMarkSymbol(rgb_color bgColor)
{
	fSuper->PushState();

	BRect r(fBounds);
	float leftMargin;
	fSuper->GetItemMargins(&leftMargin, NULL, NULL, NULL);
	r.right = r.left + leftMargin - 3;
	r.left += 1;

	BPoint center(floorf((r.left + r.right) / 2.0),
		floorf((r.top + r.bottom) / 2.0));

	float size = min_c(r.Height() - 2, r.Width());
	r.top = floorf(center.y - size / 2 + 0.5);
	r.bottom = floorf(center.y + size / 2 + 0.5);
	r.left = floorf(center.x - size / 2 + 0.5);
	r.right = floorf(center.x + size / 2 + 0.5);

	fSuper->SetHighColor(tint_color(bgColor, kLightBGTint));
	fSuper->FillRoundRect(r, 2, 2);

	BShape arrowShape;
	center.x += 0.5;
	center.y += 0.5;
	size *= 0.3;
	arrowShape.MoveTo(BPoint(center.x - size, center.y - size * 0.25));
	arrowShape.LineTo(BPoint(center.x - size * 0.25, center.y + size));
	arrowShape.LineTo(BPoint(center.x + size, center.y - size));

	fSuper->SetDrawingMode(B_OP_OVER);
	fSuper->SetHighColor(tint_color(bgColor, B_DARKEN_MAX_TINT));
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
	if (menu->fSubmenus)
		where.x -= fBounds.Height() - 4;

	switch (fShortcutChar) {
		case B_DOWN_ARROW:
		case B_UP_ARROW:
		case B_LEFT_ARROW:
		case B_RIGHT_ARROW:
		case B_ENTER:
			_DrawControlChar(fShortcutChar, where + BPoint(0, fSuper->fAscent));
			break;

		default:
			fSuper->DrawChar(fShortcutChar, where + BPoint(0, fSuper->fAscent));
			break;
	}

	where.y += (fBounds.Height() - 11) / 2 - 1;
	where.x -= 4;	
	
	if (fModifiers & B_COMMAND_KEY) {
		BRect rect(0,0,16,10);
		BBitmap control(rect, B_CMAP8);

		if (BMenu::sAltAsCommandKey)
			control.ImportBits(kAltBits, sizeof(kAltBits), 17, 0, B_CMAP8);
		else
			control.ImportBits(kCtrlBits, sizeof(kCtrlBits), 17, 0, B_CMAP8);
		
		where.x -= rect.Width() + 1;
		fSuper->DrawBitmap(&control, where);
	}

	if (fModifiers & B_CONTROL_KEY) {
		BRect rect(0,0,16,10);
		BBitmap control(rect, B_CMAP8);

		if (BMenu::sAltAsCommandKey)
			control.ImportBits(kCtrlBits, sizeof(kCtrlBits), 17, 0, B_CMAP8);
		else	
			control.ImportBits(kAltBits, sizeof(kAltBits), 17, 0, B_CMAP8);
		where.x -= rect.Width() + 1;
		fSuper->DrawBitmap(&control, where);
	}

	if (fModifiers & B_SHIFT_KEY) {
		BRect rect(0,0,21,10);
		BBitmap shift(rect, B_CMAP8);
		shift.ImportBits(kShiftBits, sizeof(kShiftBits), 22, 0, B_CMAP8);
		where.x -= rect.Width() + 1;
		fSuper->DrawBitmap(&shift, where);
	}
}


void
BMenuItem::_DrawSubmenuSymbol(rgb_color bgColor)
{
	fSuper->PushState();

	BRect r(fBounds);
	float rightMargin;
	fSuper->GetItemMargins(NULL, NULL, &rightMargin, NULL);
	r.left = r.right - rightMargin + 3;
	r.right -= 1;

	BPoint center(floorf((r.left + r.right) / 2.0),
		floorf((r.top + r.bottom) / 2.0));

	float size = min_c(r.Height() - 2, r.Width());
	r.top = floorf(center.y - size / 2 + 0.5);
	r.bottom = floorf(center.y + size / 2 + 0.5);
	r.left = floorf(center.x - size / 2 + 0.5);
	r.right = floorf(center.x + size / 2 + 0.5);

	fSuper->SetHighColor(tint_color(bgColor, kLightBGTint));
	fSuper->FillRoundRect(r, 2, 2);

	BShape arrowShape;
	center.x += 0.5;
	center.y += 0.5;
	size *= 0.25;
	float hSize = size * 0.7;
	arrowShape.MoveTo(BPoint(center.x - hSize, center.y - size));
	arrowShape.LineTo(BPoint(center.x + hSize, center.y));
	arrowShape.LineTo(BPoint(center.x - hSize, center.y + size));

	fSuper->SetDrawingMode(B_OP_OVER);
	fSuper->SetHighColor(tint_color(bgColor, B_DARKEN_MAX_TINT));
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

	switch (shortcut) {
		case B_DOWN_ARROW:
			symbol = "\xe2\x86\x93";
			break;
		case B_UP_ARROW:
			symbol = "\xe2\x86\x91";
			break;
		case B_LEFT_ARROW:
			symbol = "\xe2\x86\x90";
			break;
		case B_RIGHT_ARROW:
			symbol = "\xe2\x86\x92";
			break;
		case B_ENTER:
			symbol = "\xe2\x86\xb5";
			break;
	}

	fSuper->DrawString(symbol, where);
}


void
BMenuItem::SetAutomaticTrigger(int32 index, uint32 trigger)
{
	fTriggerIndex = index;
	fTrigger = trigger;
}
