// Standard Includes -----------------------------------------------------------
#include <string.h>
#include <stdlib.h>

// System Includes -------------------------------------------------------------
#include <MenuItem.h>
#include <String.h>
#include <Message.h>
#include <Window.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
BMenuItem::BMenuItem(const char *label, BMessage *message, char shortcut,
					 uint32 modifiers)
	:	BInvoker(message, NULL),
		fSubmenu(NULL),
		fWindow(NULL),
		fSuper(NULL),
		fModifiers(modifiers),
		fCachedWidth(0.0f),
		fTriggerIndex(0),
		fUserTrigger(0),
		fSysTrigger(0),
		fShortcutChar(shortcut),
		fMark(false),
		fEnabled(true),
		fSelected(false)
{
	fLabel = strdup(label);
}
//------------------------------------------------------------------------------
BMenuItem::BMenuItem(BMenu *menu, BMessage *message)
	:	BInvoker(message, NULL),
		fSubmenu(menu),
		fWindow(NULL),
		fSuper(NULL),
		fModifiers(0),
		fCachedWidth(0.0f),
		fTriggerIndex(0),
		fUserTrigger(0),
		fSysTrigger(0),
		fShortcutChar(0),
		fMark(false),
		fEnabled(true),
		fSelected(false)
{
	fLabel = strdup(menu->Name());

	fSubmenu->fSuperitem = this;
}
//------------------------------------------------------------------------------
BMenuItem::BMenuItem(BMessage *data)
{
}
//------------------------------------------------------------------------------
BArchivable *BMenuItem::Instantiate(BMessage *data)
{
	if (validate_instantiation(data, "BMenuItem"))
		return new BMenuItem(data);

	return NULL;
}
//------------------------------------------------------------------------------
status_t BMenuItem::Archive(BMessage *data, bool deep) const
{
	if (Label())
		data->AddString("_label", Label());

	if (!IsEnabled())
		data->AddBool("_disable", true);

	if (IsMarked())
		data->AddBool("_marked", true);

	if (fUserTrigger)
		data->AddInt32("_user_trig", fUserTrigger);

	if (fShortcutChar && fModifiers)
	{
		data->AddInt32("_shortcut", fShortcutChar);
		data->AddInt32("_mods", fModifiers);
	}

	if (Message())
		data->AddMessage("_msg", Message());

	if (deep && Submenu())
	{
		BMessage submenu;
		Submenu()->Archive(&submenu);
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

	fLabel = strdup(string);

	Menu()->InvalidateLayout();
}
//------------------------------------------------------------------------------
void BMenuItem::SetEnabled(bool state)
{
	if (Menu()->IsEnabled() == false)
		return;

	fEnabled = state;

	if (Submenu())
		Submenu()->SetEnabled(state);
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
}
//------------------------------------------------------------------------------
void BMenuItem::SetShortcut(char ch, uint32 modifiers)
{
	fShortcutChar = ch;
	fModifiers = modifiers;
}
//------------------------------------------------------------------------------
const char *BMenuItem::Label() const
{
	return fLabel;
}
//------------------------------------------------------------------------------
bool BMenuItem::IsEnabled() const
{
	return fEnabled;
}
//------------------------------------------------------------------------------
bool BMenuItem::IsMarked() const
{
	return fMark;
}
//------------------------------------------------------------------------------
char BMenuItem::Trigger() const
{
	if (fUserTrigger)
		return fUserTrigger;
	else
		return fSysTrigger;
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
	BMenu *menu = Menu();
	
	if (menu->fFontHeight == -1)
		menu->CacheFontInfo();

	*width = (float)ceil(menu->StringWidth(fLabel));
	*height = menu->fFontHeight;
}
//------------------------------------------------------------------------------
void BMenuItem::TruncateLabel(float maxWidth, char *newLabel)
{
}
//------------------------------------------------------------------------------
void BMenuItem::DrawContent()
{
	fSuper->DrawString(fLabel);
}
//------------------------------------------------------------------------------
void BMenuItem::Draw()
{
	if (IsEnabled() && fSelected)
	{
		fSuper->SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR),
			B_DARKEN_2_TINT));
		fSuper->SetLowColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR),
			B_DARKEN_2_TINT));
		fSuper->FillRect(Frame());	
	}

	BPoint pt = ContentLocation();

	fSuper->MovePenTo(pt.x, pt.y + Menu()->fAscent);
	if (IsEnabled())
		fSuper->SetHighColor(ui_color(B_MENU_ITEM_TEXT_COLOR));
	else
		fSuper->SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR),
			B_DISABLED_LABEL_TINT));
	DrawContent();

	if (Menu()->Layout() == B_ITEMS_IN_COLUMN)
	{
		if (IsMarked())
			DrawMarkSymbol();

		if (Submenu())
			DrawSubmenuSymbol();
		else if (fShortcutChar)
			DrawShortcutSymbol();
	}
}
//------------------------------------------------------------------------------
void BMenuItem::Highlight(bool flag)
{
	//Menu()->InvertRect(Frame());
	Menu()->Invalidate(Frame());
}
//------------------------------------------------------------------------------
bool BMenuItem::IsSelected() const
{
	return fSelected;
}
//------------------------------------------------------------------------------
BPoint BMenuItem::ContentLocation () const
{
	if (!Menu())
		return BPoint();
	
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
}
//------------------------------------------------------------------------------
void BMenuItem::InitMenuData(BMenu *menu)
{
}
//------------------------------------------------------------------------------
void BMenuItem::Install(BWindow *window)
{
	if (Submenu())
		Submenu()->Install(window);

	char shortcut;
	uint32 modifiers;
	if (shortcut = Shortcut(&modifiers))
		window->AddShortcut(shortcut, modifiers, this);

	if (Target() == NULL)
		SetTarget(window);
}
//------------------------------------------------------------------------------
status_t BMenuItem::Invoke(BMessage *message)
{
	if (IsEnabled())
	{
		BMessage msg;

		if (message)
			msg = *message;
		else if (Message())
			msg = *Message();
		else
			return B_OK;

		msg.AddInt64("when", system_time()); // TODO
		msg.AddPointer("source", this);
		msg.AddInt32("index", Menu()->IndexOf(this));

		return BInvoker::Invoke(&msg);
	}
	else
		return B_OK;
}
//------------------------------------------------------------------------------
void BMenuItem::Uninstall()
{
}
//------------------------------------------------------------------------------
void BMenuItem::SetSuper(BMenu *super)
{
	fSuper = super;
}
//------------------------------------------------------------------------------
void BMenuItem::Select(bool on)
{
	fSelected = on;
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
		BPoint(fBounds.Width() - 14.0f - 22.0f, fBounds.Height() - 4.0f));
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

	return NULL;
}
//------------------------------------------------------------------------------
void BSeparatorItem::SetEnabled(bool state)
{
	BSeparatorItem::SetEnabled(state);
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
