//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		BMenuItem.cpp
//	Author:			Marc Flerackers (mflerackers@androme.be)
//					Bill Hayden (haydentech@users.sourceforge.net)
//	Description:	Display item for BMenu class
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <string.h>
#include <stdlib.h>

// System Includes -------------------------------------------------------------
#include <MenuItem.h>
#include <String.h>
#include <Message.h>
#include <Window.h>
#include <Bitmap.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
BMenuItem::BMenuItem(const char *label, BMessage *message, char shortcut,
					 uint32 modifiers)
{
	InitData();
	fLabel = strdup(label);
	SetMessage(message);

	fShortcutChar = shortcut;

	if (shortcut != 0)
		fModifiers = modifiers | B_COMMAND_KEY;
	else
		fModifiers = 0;
}
//------------------------------------------------------------------------------
BMenuItem::BMenuItem(BMenu *menu, BMessage *message)
{
	InitData();
	SetMessage(message);
	InitMenuData(menu);
}
//------------------------------------------------------------------------------
BMenuItem::BMenuItem(BMessage *data)
{
	InitData();

	if (data->HasString("_label"))
	{
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

	if (data->HasInt32("_user_trig"))
	{
		int32 user_trig;

		data->FindInt32("_user_trig", &user_trig);

		SetTrigger(user_trig);
	}

	if (data->HasInt32("_shortcut"))
	{
		int32 shortcut, mods;

		data->FindInt32("_shortcut", &shortcut);
		data->FindInt32("_mods", &mods);

		SetShortcut(shortcut, mods);
	}

	if (data->HasMessage("_msg"))
	{
		BMessage *msg = new BMessage;

		data->FindMessage("_msg", msg);
		SetMessage(msg);
	}

	BMessage subMessage;

	if (data->FindMessage("_submenu", &subMessage) == B_OK)
	{
		BArchivable *object = instantiate_object(&subMessage);

		if (object)
		{
			BMenu *menu = dynamic_cast<BMenu*>(object);

			if (menu)
				InitMenuData(menu);
		}
	}
}
//------------------------------------------------------------------------------
BArchivable *BMenuItem::Instantiate(BMessage *data)
{
	if (validate_instantiation(data, "BMenuItem"))
		return new BMenuItem(data);
	else
		return NULL;
}
//------------------------------------------------------------------------------
status_t BMenuItem::Archive(BMessage *data, bool deep) const
{
	if (fLabel)
		data->AddString("_label", Label());

	if (!IsEnabled())
		data->AddBool("_disable", true);

	if (IsMarked())
		data->AddBool("_marked", true);

	if (fUserTrigger)
		data->AddInt32("_user_trig", fUserTrigger);

	if (fShortcutChar)
	{
		data->AddInt32("_shortcut", fShortcutChar);
		data->AddInt32("_mods", fModifiers);
	}

	if (Message())
		data->AddMessage("_msg", Message());

	if (deep && fSubmenu)
	{
		BMessage submenu;

		if (fSubmenu->Archive(&submenu, true) == B_OK)
			data->AddMessage("_submenu", &submenu);
	}

	return B_OK;
}
//------------------------------------------------------------------------------
BMenuItem::~BMenuItem()
{
	if (fLabel)
		free(fLabel);

	if (fSubmenu)
		delete fSubmenu;
}
//------------------------------------------------------------------------------
void BMenuItem::SetLabel(const char *string)
{
	if (fLabel)
		free(fLabel);

	if (string)
		fLabel = strdup(string);
	else
		string = NULL;

	if (fSuper)
	{
		fSuper->InvalidateLayout();

		if (fSuper->LockLooper())
		{
			fSuper->Invalidate();
			fSuper->UnlockLooper();
		}
	}
}
//------------------------------------------------------------------------------
void BMenuItem::SetEnabled(bool state)
{
	if (fSubmenu)
		fSubmenu->SetEnabled(state);

	fEnabled = state;
	BMenu *menu = Menu();

	if (menu && menu->LockLooper())
	{
		menu->Invalidate(fBounds);
		menu->UnlockLooper();
	}
}
//------------------------------------------------------------------------------
void BMenuItem::SetMarked(bool state)
{
	fMark = state;

	if (state && Menu())
		Menu()->ItemMarked(this);
}
//------------------------------------------------------------------------------
void BMenuItem::SetTrigger(char ch)
{
	fUserTrigger = ch;

	if (strchr(fLabel, ch) != 0)
		fSysTrigger = ch;
	else
		fSysTrigger = -1;

	if (fSuper)
		fSuper->InvalidateLayout();
}
//------------------------------------------------------------------------------
void BMenuItem::SetShortcut(char ch, uint32 modifiers)
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

	if (fSuper)
	{
		fSuper->InvalidateLayout();

		if (fSuper->LockLooper())
		{
			fSuper->Invalidate();
			fSuper->UnlockLooper();
		}
	}
}
//------------------------------------------------------------------------------
const char *BMenuItem::Label() const
{
	return fLabel;
}
//------------------------------------------------------------------------------
bool BMenuItem::IsEnabled() const
{
	if (fSubmenu)
		return fSubmenu->IsEnabled();

	if (!fEnabled)
		return false;

	return fSuper ? fSuper->IsEnabled() : true;
}
//------------------------------------------------------------------------------
bool BMenuItem::IsMarked() const
{
	return fMark;
}
//------------------------------------------------------------------------------
char BMenuItem::Trigger() const
{
	return fUserTrigger;
}
//------------------------------------------------------------------------------
char BMenuItem::Shortcut(uint32 *modifiers) const
{
	if (modifiers)
		*modifiers = fModifiers;

	return fShortcutChar;
}
//------------------------------------------------------------------------------
BMenu *BMenuItem::Submenu() const
{
	return fSubmenu;
}
//------------------------------------------------------------------------------
BMenu *BMenuItem::Menu() const
{
	return fSuper;
}
//------------------------------------------------------------------------------
BRect BMenuItem::Frame() const
{
	return fBounds;
}
//------------------------------------------------------------------------------
void BMenuItem::GetContentSize(float *width, float *height)
{
	fSuper->CacheFontInfo();
	
	fCachedWidth = fSuper->StringWidth(fLabel);

	*width = (float)ceil(fCachedWidth);
	*height = fSuper->fFontHeight;
}
//------------------------------------------------------------------------------
void BMenuItem::TruncateLabel(float maxWidth, char *newLabel)
{
	// ToDo: implement me!
}
//------------------------------------------------------------------------------
void BMenuItem::DrawContent()
{
	fSuper->MovePenBy(0, fSuper->fAscent);
	fSuper->DrawString(fLabel);
	
	// ToDo: label truncation is missing
	// ToDo: draw trigger is missing!
}
//------------------------------------------------------------------------------
void BMenuItem::Draw()
{
	bool enabled = IsEnabled();

	fSuper->CacheFontInfo();

	if (IsSelected() && (enabled || Submenu())/* && fSuper->fRedrawAfterSticky*/)
	{
		fSuper->SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR),
			B_DARKEN_2_TINT));
		fSuper->SetLowColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR),
			B_DARKEN_2_TINT));
		fSuper->FillRect(Frame());
	}

	if (IsEnabled())
		fSuper->SetHighColor(ui_color(B_MENU_ITEM_TEXT_COLOR));
	else if (IsSelected())
		fSuper->SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR),
			B_LIGHTEN_1_TINT));
	else
		fSuper->SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR),
			B_DISABLED_LABEL_TINT));

	fSuper->MovePenTo(ContentLocation());
	DrawContent();

	if (fSuper->Layout() == B_ITEMS_IN_COLUMN)
	{
		if (IsMarked())
			DrawMarkSymbol();

		if (fShortcutChar)
			DrawShortcutSymbol();

		if (Submenu())
			DrawSubmenuSymbol();
	}
}
//------------------------------------------------------------------------------
void BMenuItem::Highlight(bool flag)
{
	Menu()->Draw(Frame());
}
//------------------------------------------------------------------------------
bool BMenuItem::IsSelected() const
{
	return fSelected;
}
//------------------------------------------------------------------------------
BPoint BMenuItem::ContentLocation() const
{	
	return BPoint(fBounds.left + Menu()->fPad.left,
		fBounds.top + Menu()->fPad.top);
}
//------------------------------------------------------------------------------
void BMenuItem::_ReservedMenuItem1() {}
void BMenuItem::_ReservedMenuItem2() {}
void BMenuItem::_ReservedMenuItem3() {}
void BMenuItem::_ReservedMenuItem4() {}
//------------------------------------------------------------------------------
BMenuItem::BMenuItem(const BMenuItem &)
{
}
//------------------------------------------------------------------------------
BMenuItem &BMenuItem::operator=(const BMenuItem &)
{
	return *this;
}
//------------------------------------------------------------------------------
void BMenuItem::InitData()
{
	fLabel = NULL;
	fSubmenu = 0;
	fWindow = NULL;
	fSuper = NULL;
	fModifiers = 0;
	fCachedWidth = 0;
	fTriggerIndex = -1;
	fUserTrigger = 0;
	fSysTrigger = 0;
	fShortcutChar = 0;
	fMark = false;
	fEnabled = true;
	fSelected = false;
}
//------------------------------------------------------------------------------
void BMenuItem::InitMenuData(BMenu *menu)
{
	fSubmenu = menu;
	fSubmenu->fSuperitem = this;

	BMenuItem *item;

	if (menu->IsRadioMode() && menu->IsLabelFromMarked() &&
		(item = menu->FindMarked()) != NULL)
		SetLabel(item->Label());
	else
		SetLabel(menu->Name());
}
//------------------------------------------------------------------------------
void BMenuItem::Install(BWindow *window)
{
	if (fSubmenu)
		fSubmenu->Install(window);

	fWindow = window;

	if (fShortcutChar != 0 && (fModifiers & B_COMMAND_KEY) && fWindow)
		window->AddShortcut(fShortcutChar, fModifiers, this);

	if (!Messenger().IsValid())
		SetTarget(window);
}
//------------------------------------------------------------------------------
status_t BMenuItem::Invoke(BMessage *message)
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
		
	if (!message)
	{
		if (!fSuper->IsWatched())
			return err;
	}
	else
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
//------------------------------------------------------------------------------
void BMenuItem::Uninstall()
{
	if (fSubmenu)
		fSubmenu->Uninstall();

	if (Target() == fWindow)
		SetTarget(BMessenger());

	if (0x6c != 0 && fModifiers & B_COMMAND_KEY && fWindow)
		fWindow->RemoveShortcut(fShortcutChar, fModifiers);

	fWindow = NULL;
}
//------------------------------------------------------------------------------
void BMenuItem::SetSuper(BMenu *super)
{
	if (fSuper != NULL && super != NULL)
	{
		debugger("Error - can't add menu or menu item to more than 1 container (either menu or menubar).");
		return;
	}
		
	fSuper = super;

	if (fSubmenu)
		fSubmenu->fSuper = super;
}
//------------------------------------------------------------------------------
void BMenuItem::Select(bool on)
{
	if (Submenu())
	{
		fSelected = on;
		Highlight(on);
	}
	else if (IsEnabled())
	{
		fSelected = on;
		Highlight(on);
	}
		
}
//------------------------------------------------------------------------------
void BMenuItem::DrawMarkSymbol()
{
	fSuper->SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR),
		B_DARKEN_1_TINT));
	fSuper->StrokeLine(BPoint(fBounds.left + 6.0f, fBounds.bottom - 3.0f),
		BPoint(fBounds.left + 10.0f, fBounds.bottom - 12.0f));
	fSuper->StrokeLine(BPoint(fBounds.left + 7.0f, fBounds.bottom - 3.0f),
		BPoint(fBounds.left + 11.0f, fBounds.bottom - 12.0f));

	fSuper->SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR),
		B_DARKEN_4_TINT));

	fSuper->StrokeLine(BPoint(fBounds.left + 6.0f, fBounds.bottom - 4.0f),
		BPoint(fBounds.left + 10.0f, fBounds.bottom - 13.0f));
	fSuper->StrokeLine(BPoint(fBounds.left + 5.0f, fBounds.bottom - 4.0f),
		BPoint(fBounds.left + 9.0f, fBounds.bottom - 13.0f));
	fSuper->StrokeLine(BPoint(fBounds.left + 5.0f, fBounds.bottom - 3.0f),
		BPoint(fBounds.left + 3.0f, fBounds.bottom - 9.0f));
	fSuper->StrokeLine(BPoint(fBounds.left + 4.0f, fBounds.bottom - 4.0f),
		BPoint(fBounds.left + 2.0f, fBounds.bottom - 9.0f));
}
//------------------------------------------------------------------------------
void BMenuItem::DrawShortcutSymbol()
{
	BString shortcut("");

	if (fModifiers & B_CONTROL_KEY)
		shortcut += "ctl+";

	shortcut += fShortcutChar;

	fSuper->DrawString(shortcut.String(), ContentLocation() +
		BPoint(fBounds.Width() - 14.0f - 32.0f, fBounds.Height() - 4.0f));
}
//------------------------------------------------------------------------------
void BMenuItem::DrawSubmenuSymbol()
{
	fSuper->SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR),
		B_LIGHTEN_MAX_TINT));
	fSuper->FillTriangle(BPoint(fBounds.right - 14.0f, fBounds.bottom - 4.0f),
		BPoint(fBounds.right - 14.0f, fBounds.bottom - 12.0f),
		BPoint(fBounds.right - 5.0f, fBounds.bottom - 8.0f));

	fSuper->SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR),
		B_DARKEN_2_TINT));
	fSuper->StrokeLine(BPoint(fBounds.right - 14.0f, fBounds.bottom - 5),
		BPoint(fBounds.right - 9.0f, fBounds.bottom - 7));
	fSuper->StrokeLine(BPoint(fBounds.right - 7.0f, fBounds.bottom - 8),
		BPoint(fBounds.right - 7.0f, fBounds.bottom - 8));

	fSuper->SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR),
		B_DARKEN_3_TINT));
	fSuper->StrokeTriangle(BPoint(fBounds.right - 14.0f, fBounds.bottom - 4.0f),
		BPoint(fBounds.right - 14.0f, fBounds.bottom - 12.0f),
		BPoint(fBounds.right - 5.0f, fBounds.bottom - 8.0f));

	fSuper->SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR),
		B_LIGHTEN_1_TINT));
	fSuper->StrokeTriangle(BPoint(fBounds.right - 12.0f, fBounds.bottom - 7.0f),
		BPoint(fBounds.right - 12.0f, fBounds.bottom - 9.0f),
		BPoint(fBounds.right - 9.0f, fBounds.bottom - 8.0f));
	fSuper->FillTriangle(BPoint(fBounds.right - 12.0f, fBounds.bottom - 7.0f),
		BPoint(fBounds.right - 12.0f, fBounds.bottom - 9.0f),
		BPoint(fBounds.right - 9.0f, fBounds.bottom - 8.0f));
}
//------------------------------------------------------------------------------
void BMenuItem::DrawControlChar(const char *control)
{
}
//------------------------------------------------------------------------------
void BMenuItem::SetSysTrigger(char ch)
{
	fSysTrigger = ch;
}
//------------------------------------------------------------------------------
BSeparatorItem::BSeparatorItem()
	:	BMenuItem(NULL, NULL, 0, 0)
{
}
//------------------------------------------------------------------------------
BSeparatorItem::BSeparatorItem(BMessage *data)
	:	BMenuItem(data)
{
}
//------------------------------------------------------------------------------
BSeparatorItem::~BSeparatorItem()
{
}
//------------------------------------------------------------------------------
status_t BSeparatorItem::Archive(BMessage *data, bool deep) const
{
	return BMenuItem::Archive(data, deep);
}
//------------------------------------------------------------------------------
BArchivable *BSeparatorItem::Instantiate(BMessage *data)
{
	if (validate_instantiation(data, "BSeparatorItem"))
		return new BSeparatorItem(data);
	else
		return NULL;
}
//------------------------------------------------------------------------------
void BSeparatorItem::SetEnabled(bool state)
{
}
//------------------------------------------------------------------------------
void BSeparatorItem::GetContentSize(float *width, float *height)
{
	*width = 2.0f;
	*height = 8.0f;
}
//------------------------------------------------------------------------------
void BSeparatorItem::Draw()
{
	BRect bounds = Frame();

	Menu()->SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR),
		B_DARKEN_2_TINT));
	Menu()->StrokeLine(BPoint(bounds.left + 1.0f, bounds.top + 4.0f),
		BPoint(bounds.right - 1.0f, bounds.top + 4.0f));
	Menu()->SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR),
		B_LIGHTEN_2_TINT));
	Menu()->StrokeLine(BPoint(bounds.left + 1.0f, bounds.top + 5.0f),
		BPoint(bounds.right - 1.0f, bounds.top + 5.0f));

	Menu()->SetHighColor(0, 0, 0);
}
//------------------------------------------------------------------------------
void BSeparatorItem::_ReservedSeparatorItem1() {}
void BSeparatorItem::_ReservedSeparatorItem2() {}
//------------------------------------------------------------------------------
BSeparatorItem &BSeparatorItem::operator=(const BSeparatorItem &)
{
	return *this;
}
//------------------------------------------------------------------------------
