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
//	File Name:		Menu.cpp
//	Author:			Marc Flerackers (mflerackers@androme.be)
//	Description:	BMenu display a menu of selectable items.
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <Menu.h>
#include <MenuItem.h>
#include <Window.h>
#include <PropertyInfo.h>
#include <Errors.h>
#include <File.h>
#include <FindDirectory.h>
#include <Path.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

menu_info BMenu::sMenuInfo;

static property_info prop_list[] =
{
	{ "Enabled", { B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Returns true if menu or menu item is enabled; false "
		"otherwise." },
	{ "Enabled", { B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Enables or disables menu or menu item." },
	{ "Label", { B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Returns the string label of the menu or menu item." },
	{ "Label", { B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Sets the string label of the menu or menu item." },
	{ "Mark", { B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Returns true if the menu item or the menu's superitem "
		"is marked; false otherwise." },
	{ "Mark", { B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Marks or unmarks the menu item or the menu's superitem." },
	{ "Menu", { B_CREATE_PROPERTY, 0 },
		{ B_NAME_SPECIFIER, B_INDEX_SPECIFIER, B_REVERSE_INDEX_SPECIFIER, 0 },
		"Adds a new menu item at the specified index with the text label found in \"data\" "
		"and the int32 command found in \"what\" (used as the what field in the CMessage "
		"sent by the item)." },
	{ "Menu", { B_DELETE_PROPERTY, 0 },
		{ B_NAME_SPECIFIER, B_INDEX_SPECIFIER, B_REVERSE_INDEX_SPECIFIER, 0 },
		"Removes the selected menu or menus." },
	{ "Menu", { B_GET_PROPERTY, B_SET_PROPERTY, 0 },
		{ B_NAME_SPECIFIER, B_INDEX_SPECIFIER, B_REVERSE_INDEX_SPECIFIER, 0 },
		"Directs scripting message to the specified menu, first popping the current "
		"specifier off the stack." },
	{ "MenuItem", { B_COUNT_PROPERTIES, 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Counts the number of menu items in the specified menu." },
	{ "MenuItem", { B_CREATE_PROPERTY, 0 },
		{ B_NAME_SPECIFIER, B_INDEX_SPECIFIER, B_REVERSE_INDEX_SPECIFIER, 0 },
		"Adds a new menu item at the specified index with the text label found in \"data\" "
		"and the int32 command found in \"what\" (used as the what field in the CMessage "
		"sent by the item)." },
	{ "MenuItem", { B_DELETE_PROPERTY, 0 },
		{ B_NAME_SPECIFIER, B_INDEX_SPECIFIER, B_REVERSE_INDEX_SPECIFIER, 0 },
		"Removes the specified menu item from its parent menu." },
	{ "MenuItem", { B_EXECUTE_PROPERTY, 0 },
		{ B_NAME_SPECIFIER, B_INDEX_SPECIFIER, B_REVERSE_INDEX_SPECIFIER, 0 },
		"Invokes the specified menu item." },
	{ "MenuItem", { B_GET_PROPERTY, B_SET_PROPERTY, 0 },
		{ B_NAME_SPECIFIER, B_INDEX_SPECIFIER, B_REVERSE_INDEX_SPECIFIER, 0 },
		"Directs scripting message to the specified menu, first popping the current "
		"specifier off the stack." },
	{}
};

//------------------------------------------------------------------------------
class BMenuWindow : public BWindow
{
public:
	BMenuWindow(BRect frame, BMenu *menu) :
		BWindow(frame, "Menu", B_NO_BORDER_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL,
			B_NOT_ZOOMABLE)
	{
		fMenu = menu;
		AddChild(fMenu);	
		ResizeTo(fMenu->Bounds().Width() + 1, fMenu->Bounds().Height() + 1);
		fMenu->MakeFocus(true);
	}

	virtual ~BMenuWindow() {}

	virtual void WindowActivated(bool active)
	{
		if (!active)
		{
			RemoveChild(fMenu);

			if (Lock())
				Quit();
		}
	}

private:
	BMenu *fMenu;
};

//------------------------------------------------------------------------------
BMenu::BMenu(const char *name, menu_layout layout)
	:	BView(BRect(), name, B_FOLLOW_LEFT | B_FOLLOW_TOP,
			B_WILL_DRAW | B_NAVIGABLE),
		fChosenItem(NULL),
		fPad(14.0f, 2.0f, 20.0f, 0.0f),
		fSelected(NULL),
		fCachedMenuWindow(NULL),
		fSuper(NULL),
		fSuperitem(NULL),
		fAscent(-1.0f),
		fDescent(-1.0f),
		fFontHeight(-1.0f),
		fState(0),
		fLayout(layout),
		fExtraRect(NULL),
		fMaxContentWidth(0.0f),
		fInitMatrixSize(NULL),
		fExtraMenuData(NULL),
		fTrigger(0),
		fResizeToFit(true),
		fUseCachedMenuLayout(true),
		fEnabled(true),
		fDynamicName(false),
		fRadioMode(false),
		fTrackNewBounds(false),
		fStickyMode(false),
		fIgnoreHidden(true),
		fTriggerEnabled(true),
		fRedrawAfterSticky(true),
		fAttachAborted(false)
{
	SetViewColor(ui_color(B_MENU_BACKGROUND_COLOR));
	SetFontSize(10);
}
//------------------------------------------------------------------------------
BMenu::BMenu(const char *name, float width, float height)
	:	BView(BRect(0.0f, width, 0.0f, height), name,
		B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE),
		fChosenItem(NULL),
		fSelected(NULL),
		fCachedMenuWindow(NULL),
		fSuper(NULL),
		fSuperitem(NULL),
		fAscent(-1.0f),
		fDescent(-1.0f),
		fFontHeight(-1.0f),
		fState(0),
		fLayout(B_ITEMS_IN_MATRIX),
		fExtraRect(NULL),
		fMaxContentWidth(0.0f),
		fInitMatrixSize(NULL),
		fExtraMenuData(NULL),
		fTrigger(0),
		fResizeToFit(true),
		fUseCachedMenuLayout(true),
		fEnabled(true),
		fDynamicName(false),
		fRadioMode(false),
		fTrackNewBounds(false),
		fStickyMode(false),
		fIgnoreHidden(true),
		fTriggerEnabled(true),
		fRedrawAfterSticky(true),
		fAttachAborted(false)
{
	SetViewColor(ui_color(B_MENU_BACKGROUND_COLOR));
	SetFontSize(10);
}
//------------------------------------------------------------------------------
BMenu::~BMenu()
{
	for (int i = 0; i < CountItems(); i++)
		delete ItemAt(i);
}
//------------------------------------------------------------------------------
BMenu::BMenu(BMessage *archive)
	:	BView(archive)
{
	SetViewColor(ui_color(B_MENU_BACKGROUND_COLOR));
}
//------------------------------------------------------------------------------
BArchivable *BMenu::Instantiate(BMessage *data)
{
	if (validate_instantiation(data, "BMenu"))
		return new BMenu(data);
	else
		return NULL;
}
//------------------------------------------------------------------------------
status_t BMenu::Archive(BMessage *data, bool deep) const
{
	status_t err = BView::Archive(data, deep);

	if (err != B_OK)
		return err;

	if (Layout() != B_ITEMS_IN_ROW)
	{
		err = data->AddInt32("_layout", Layout());

		if (err != B_OK)
			return err;
	}

	err = data->AddBool("_rsize_to_fit", fResizeToFit);

	if (err != B_OK)
		return err;

	err = data->AddBool("_disable", !IsEnabled());

	if (err != B_OK)
		return err;

	err = data->AddBool("_radio", IsRadioMode());

	if (err != B_OK)
		return err;

	err = data->AddBool("_trig_disabled", AreTriggersEnabled());

	if (err != B_OK)
		return err;

	err = data->AddBool("_dyn_label", fDynamicName);

	if (err != B_OK)
		return err;

	err = data->AddFloat("_maxwidth", fMaxContentWidth);

	if (err != B_OK)
		return err;

	if (deep)
	{
		// TODO store items and rects
	}

	return err;
}
//------------------------------------------------------------------------------
void BMenu::AttachedToWindow()
{
	BView::AttachedToWindow();

	LayoutItems(0);
}
//------------------------------------------------------------------------------
void BMenu::DetachedFromWindow()
{
}
//------------------------------------------------------------------------------
bool BMenu::AddItem(BMenuItem *item)
{
	return AddItem(item, CountItems());
}
//------------------------------------------------------------------------------
bool BMenu::AddItem(BMenuItem *item, int32 index)
{
	item->fSuper = this;

	bool err = fItems.AddItem(item, index);

	if (!err)
		return err;

	// Make sure we update the layout in case we are already attached.
	if (Window() && fResizeToFit)
		LayoutItems(index);

	// Find the root menu window, so we can install this item.
	BMenu *root = this;
	while (root->Supermenu())
		root = root->Supermenu();

	if (root->Window())
		Install(root->Window());
	
	return err;
}
//------------------------------------------------------------------------------
bool BMenu::AddItem(BMenuItem *item, BRect frame)
{
	item->fBounds = frame;
	
	return AddItem(item, CountItems());
}
//------------------------------------------------------------------------------
bool BMenu::AddItem(BMenu *submenu)
{
	BMenuItem *item = new BMenuItem(submenu);
	submenu->fSuper = this;

	return AddItem(item);
}
//------------------------------------------------------------------------------
bool BMenu::AddItem(BMenu *submenu, int32 index)
{
	BMenuItem *item = new BMenuItem(submenu);
	submenu->fSuper = this;
	
	return AddItem(item, index);
}
//------------------------------------------------------------------------------
bool BMenu::AddItem(BMenu *submenu, BRect frame)
{
	BMenuItem *item = new BMenuItem(submenu);
	submenu->fSuper = this;
	item->fBounds = frame;
	
	return AddItem(item);
}
//------------------------------------------------------------------------------
bool BMenu::AddList(BList *list, int32 index)
{
	return false;
}
//------------------------------------------------------------------------------
bool BMenu::AddSeparatorItem()
{
	BMenuItem *item = new BSeparatorItem();
	item->fSuper = this;
	
	return fItems.AddItem(item);
}
//------------------------------------------------------------------------------
bool BMenu::RemoveItem(BMenuItem *item)
{
	return fItems.RemoveItem(item);
}
//------------------------------------------------------------------------------
BMenuItem *BMenu::RemoveItem(int32 index)
{
	return (BMenuItem*)fItems.RemoveItem ( index );
}
//------------------------------------------------------------------------------
bool BMenu::RemoveItems(int32 index, int32 count, bool del)
{
	return false;
}
//------------------------------------------------------------------------------
bool BMenu::RemoveItem(BMenu *submenu)
{
	for (int i =0; i < fItems.CountItems(); i++)
		if (((BMenuItem*)fItems.ItemAt(i))->Submenu() == submenu)
			return (BMenuItem*)fItems.RemoveItem(fItems.ItemAt(i));

	return false;
}
//------------------------------------------------------------------------------
int32 BMenu::CountItems() const
{
	return fItems.CountItems();
}
//------------------------------------------------------------------------------
BMenuItem *BMenu::ItemAt(int32 index) const
{
	return (BMenuItem*)fItems.ItemAt(index);
}
//------------------------------------------------------------------------------
BMenu *BMenu::SubmenuAt(int32 index) const
{
	return ((BMenuItem*)fItems.ItemAt(index))->Submenu();
}
//------------------------------------------------------------------------------
int32 BMenu::IndexOf(BMenuItem *item) const
{
	return fItems.IndexOf(item);
}
//------------------------------------------------------------------------------
int32 BMenu::IndexOf(BMenu *submenu) const
{
	for (int32 i =0; i < fItems.CountItems(); i++)
		if (((BMenuItem*)fItems.ItemAt(i))->Submenu() == submenu)
			return i;

	return -1;
}
//------------------------------------------------------------------------------
BMenuItem *BMenu::FindItem(const char *label) const
{
	BMenuItem *item;

	for (int32 i =0; i < CountItems(); i++)
	{
		item = ItemAt(i);

		if (item->Label() && strcmp(item->Label(), label) == 0)
			return (item);

		if (item->Submenu())
		{
			item = item->Submenu()->FindItem(label);
			if (item)
				return item;
		}
	}

	return NULL;
}
//------------------------------------------------------------------------------
BMenuItem *BMenu::FindItem(uint32 command) const
{
	return NULL;
}
//------------------------------------------------------------------------------
status_t BMenu::SetTargetForItems(BHandler *handler)
{
	for (int32 i = 0; i < fItems.CountItems(); i++)
		if (((BMenuItem*)fItems.ItemAt(i))->SetTarget(handler) != B_OK)
			return B_ERROR;

	return B_OK;
}
//------------------------------------------------------------------------------
status_t BMenu::SetTargetForItems(BMessenger messenger)
{
	for (int32 i = 0; i < fItems.CountItems (); i++)
		if (((BMenuItem*)fItems.ItemAt(i))->SetTarget(messenger) != B_OK)
			return B_ERROR;

	return B_OK;
}
//------------------------------------------------------------------------------
void BMenu::SetEnabled(bool enabled)
{
	fEnabled = enabled;

	for (int32 i = 0; i < CountItems(); i++)
		ItemAt(i)->SetEnabled(enabled);
}
//------------------------------------------------------------------------------
void BMenu::SetRadioMode(bool flag)
{
	fRadioMode = flag;
}
//------------------------------------------------------------------------------
void BMenu::SetTriggersEnabled(bool flag)
{
	fTriggerEnabled = flag;
}
//------------------------------------------------------------------------------
void BMenu::SetMaxContentWidth(float width)
{
	fMaxContentWidth = width;
}
//------------------------------------------------------------------------------
void BMenu::SetLabelFromMarked(bool flag)
{
}
//------------------------------------------------------------------------------
bool BMenu::IsLabelFromMarked()
{
	return false;
}
//------------------------------------------------------------------------------
bool BMenu::IsEnabled() const
{
	return fEnabled;
}
//------------------------------------------------------------------------------
bool BMenu::IsRadioMode() const
{
	return fRadioMode;
}
//------------------------------------------------------------------------------
bool BMenu::AreTriggersEnabled() const
{
	return fTriggerEnabled;
}
//------------------------------------------------------------------------------
bool BMenu::IsRedrawAfterSticky() const
{
	return fRedrawAfterSticky;
}
//------------------------------------------------------------------------------
float BMenu::MaxContentWidth() const
{
	return fMaxContentWidth;
}
//------------------------------------------------------------------------------
BMenuItem *BMenu::FindMarked()
{
	for (int i =0; i < fItems.CountItems(); i++)
		if (((BMenuItem*)fItems.ItemAt(i))->IsMarked())
			return (BMenuItem*)fItems.ItemAt(i);

	return NULL;
}
//------------------------------------------------------------------------------
BMenu *BMenu::Supermenu() const
{
	return fSuper;
}
//------------------------------------------------------------------------------
BMenuItem *BMenu::Superitem() const
{
	return fSuperitem;
}
//------------------------------------------------------------------------------
void BMenu::MessageReceived(BMessage *msg)
{
	BView::MessageReceived(msg);
}
//------------------------------------------------------------------------------
void BMenu::KeyDown(const char *bytes, int32 numBytes)
{
	switch (bytes[0])
	{
		case B_UP_ARROW:
		{
			if (fSelected)
			{
				fSelected->fSelected = false;

				if ( fSelected == fItems.FirstItem())
					fSelected = (BMenuItem*)fItems.LastItem();
				else
					fSelected = ItemAt(IndexOf(fSelected) - 1);
			}
			else
				fSelected = (BMenuItem*)fItems.LastItem();

			fSelected->fSelected = true;

			break;
		}
		case B_DOWN_ARROW:
		{
			if (fSelected)
			{
				fSelected->fSelected = false;

				if (fSelected == fItems.LastItem())
					fSelected = (BMenuItem*)fItems.FirstItem();
				else
					fSelected = ItemAt(IndexOf(fSelected) + 1);
			}
			else
				fSelected = (BMenuItem*)fItems.FirstItem();

			fSelected->fSelected = true;

			break;
		}
		case B_HOME:
		{
			if (fSelected)
				fSelected->fSelected = false;

			fSelected = (BMenuItem*)fItems.FirstItem();
			fSelected->fSelected = true;

			break;
		}
		case B_END:
		{
			if (fSelected)
				fSelected->fSelected = false;

			fSelected = (BMenuItem*)fItems.LastItem();
			fSelected->fSelected = true;

			break;
		}
		case B_ENTER:
		case B_SPACE:
		{
			if (fSelected)
				InvokeItem(fSelected);

			break;
		}
		default:
			BView::KeyDown(bytes, numBytes);
	}
}
//------------------------------------------------------------------------------
void BMenu::Draw(BRect updateRect)
{
	BRect bounds(Bounds());

	SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR), B_DARKEN_4_TINT));
	StrokeRect(bounds);
	SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR), B_DARKEN_2_TINT));
	StrokeLine(BPoint(bounds.left + 2, bounds.bottom - 1),
		BPoint(bounds.right - 1, bounds.bottom - 1));
	StrokeLine(BPoint(bounds.right - 1, bounds.top + 1));
	SetHighColor(tint_color ( ui_color ( B_MENU_BACKGROUND_COLOR ), B_LIGHTEN_2_TINT));
	StrokeLine(BPoint(bounds.right - 2, bounds.top + 1),
		BPoint(bounds.left + 1, bounds.top + 1));
	StrokeLine(BPoint(bounds.left + 1, bounds.bottom - 2));

	DrawItems(updateRect);
}
//------------------------------------------------------------------------------
void BMenu::GetPreferredSize(float *width, float *height)
{
}
//------------------------------------------------------------------------------
void BMenu::ResizeToPreferred()
{
}
//------------------------------------------------------------------------------
void BMenu::FrameMoved(BPoint new_position)
{
}
//------------------------------------------------------------------------------
void BMenu::FrameResized(float new_width, float new_height)
{
}
//------------------------------------------------------------------------------
void BMenu::InvalidateLayout()
{
	CacheFontInfo();
	LayoutItems(0);
}
//------------------------------------------------------------------------------
BHandler *BMenu::ResolveSpecifier(BMessage *msg, int32 index,
								  BMessage *specifier, int32 form,
								  const char *property)
{
	BPropertyInfo propInfo(prop_list);
	BHandler *target = NULL;

	switch (propInfo.FindMatch(msg, 0, specifier, form, property))
	{
		case B_ERROR:
			break;

		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
			target = this;
			break;
		case 8:
			// TODO: redirect to menu
			target = this;
			break;
		case 9:
		case 10:
		case 11:
		case 12:
			target = this;
			break;
		case 13:
			// TODO: redirect to menuitem
			target = this;
			break;
	}

	if (!target)
		target = BView::ResolveSpecifier(msg, index, specifier, form,
		property);

	return target;
}
//------------------------------------------------------------------------------
status_t BMenu::GetSupportedSuites(BMessage *data)
{
	status_t err;

	if (data == NULL)
		return B_BAD_VALUE;

	err = data->AddString("suites", "suite/vnd.Be-menu");

	if (err != B_OK)
		return err;
	
	BPropertyInfo prop_info(prop_list);
	err = data->AddFlat("messages", &prop_info);

	if (err != B_OK)
		return err;
	
	return BView::GetSupportedSuites(data);
}
//------------------------------------------------------------------------------
status_t BMenu::Perform(perform_code d, void *arg)
{
	return B_ERROR;
}
//------------------------------------------------------------------------------
void BMenu::MakeFocus(bool focused)
{
	if (focused)
		SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);

	BView::MakeFocus(focused);
}
//------------------------------------------------------------------------------
void BMenu::AllAttached()
{
}
//------------------------------------------------------------------------------
void BMenu::AllDetached()
{
}
//------------------------------------------------------------------------------
BMenu::BMenu(BRect frame, const char *name, uint32 resizingMode, uint32 flags,
			 menu_layout layout, bool resizeToFit)
	:	BView(frame, name, resizingMode, flags),
		fChosenItem(NULL),
		fSelected(NULL),
		fCachedMenuWindow(NULL),
		fSuper(NULL),
		fSuperitem(NULL),
		fAscent(-1.0f),
		fDescent(-1.0f),
		fFontHeight(-1.0f),
		fState(0),
		fLayout(layout),
		fExtraRect(NULL),
		fMaxContentWidth(0.0f),
		fInitMatrixSize(NULL),
		fExtraMenuData(NULL),
		fTrigger(0),
		fResizeToFit(resizeToFit),
		fUseCachedMenuLayout(true),
		fEnabled(true),
		fDynamicName(false),
		fRadioMode(false),
		fTrackNewBounds(false),
		fStickyMode(false),
		fIgnoreHidden(true),
		fTriggerEnabled(true),
		fRedrawAfterSticky(true),
		fAttachAborted(false)
{
	SetViewColor(ui_color(B_MENU_BACKGROUND_COLOR));
	SetFontSize(10);
}
//------------------------------------------------------------------------------
BPoint BMenu::ScreenLocation()
{
	BMenu *superMenu = Supermenu();
	
	if (superMenu == NULL)
		return BPoint();

	BMenuItem *superItem = Superitem();

	if (superItem == NULL)
		return BPoint();

	BPoint point;
	
	if (superMenu->Layout() == B_ITEMS_IN_COLUMN)
		point = superItem->Frame().RightTop();
	else
		point = superItem->Frame().LeftBottom() + BPoint(0.0f, 1.0f);
	
	superMenu->ConvertToScreen(&point);

	return point;
}
//------------------------------------------------------------------------------
void BMenu::SetItemMargins(float left, float top, float right, float bottom)
{
	fPad.Set(left, top, right, bottom);
}
//------------------------------------------------------------------------------
void BMenu::GetItemMargins(float *left, float *top, float *right,
						   float *bottom) const
{
	*left = fPad.left;
	*top = fPad.top;
	*right = fPad.right;
	*bottom = fPad.bottom;
}
//------------------------------------------------------------------------------
menu_layout BMenu::Layout() const
{
	return fLayout;
}
//------------------------------------------------------------------------------
void BMenu::Show()
{
	Show(false);
}
//------------------------------------------------------------------------------
void BMenu::Show(bool selectFirst)
{
	_show(selectFirst);
}
//------------------------------------------------------------------------------
void BMenu::Hide()
{
	_hide();
}
//------------------------------------------------------------------------------
BMenuItem *BMenu::Track(bool openAnyway, BRect *clickToOpenRect)
{
	return NULL;
}
//------------------------------------------------------------------------------
bool BMenu::AddDynamicItem(add_state s)
{
	return false;
}
//------------------------------------------------------------------------------
void BMenu::DrawBackground(BRect update)
{
}
//------------------------------------------------------------------------------
void BMenu::SetTrackingHook(menu_tracking_hook func, void *state)
{
}
//------------------------------------------------------------------------------
void BMenu::_ReservedMenu3() {}
void BMenu::_ReservedMenu4() {}
void BMenu::_ReservedMenu5() {}
void BMenu::_ReservedMenu6() {}
//------------------------------------------------------------------------------
BMenu &BMenu::operator=(const BMenu &)
{
	return *this;
}
//------------------------------------------------------------------------------
void BMenu::InitData(BMessage *data)
{
}
//------------------------------------------------------------------------------
bool BMenu::_show(bool selectFirstItem)
{
	BPoint point = ScreenLocation();

	BWindow *window = new BMenuWindow(BRect(point.x, point.y,
		point.x + 20, point.y + 200), this);

	window->Show();

	return true;
}
//------------------------------------------------------------------------------
void BMenu::_hide()
{
	BWindow *window = Window();

	if (window)
	{
		window->RemoveChild(this);

		window->Lock();
		window->Quit();
	}
}
//------------------------------------------------------------------------------
BMenuItem *BMenu::_track(int *action, long start)
{
	return NULL;
}
//------------------------------------------------------------------------------
bool BMenu::_AddItem(BMenuItem *item, int32 index)
{
	return false;
}
//------------------------------------------------------------------------------
bool BMenu::RemoveItems(int32 index, int32 count, BMenuItem *item, bool del)
{
	return false;
}
//------------------------------------------------------------------------------
void BMenu::LayoutItems(int32 index)
{
	CalcTriggers();

	float width, height;

	ComputeLayout(index, true, true, &width, &height);

	ResizeTo(width, height);
}
//------------------------------------------------------------------------------
void BMenu::ComputeLayout(int32 index, bool bestFit, bool moveItems,
						  float* width, float* height)
{
	BRect frame;
	float iWidth, iHeight;
	BMenuItem *item;

	if (fLayout == B_ITEMS_IN_COLUMN)
	{
		frame = BRect(0.0f, 0.0f, 0.0f, 2.0f);

		for (int i = 0; i < fItems.CountItems(); i++)
		{
			item = (BMenuItem*)fItems.ItemAt(i);
			
			item->GetContentSize(&iWidth, &iHeight);

			if (item->fModifiers && item->fShortcutChar)
				iWidth += 25.0f;

			item->fBounds.left = 2.0f;
			item->fBounds.top = frame.bottom;
			item->fBounds.bottom = item->fBounds.top + iHeight + fPad.top + fPad.bottom;

			frame.right = max_c(frame.right, iWidth + fPad.left + fPad.right);
			frame.bottom = item->fBounds.bottom + 1.0f;
		}

		for (int i = 0; i < fItems.CountItems(); i++)
			ItemAt(i)->fBounds.right = frame.right;

		frame.right = (float)ceil(frame.right) + 2.0f;
		frame.bottom += 1.0f;
	}
	else if (fLayout == B_ITEMS_IN_ROW)
	{
		font_height fh;
		GetFontHeight(&fh);
		frame = BRect(0.0f, 0.0f, 0.0f,
			(float)ceil(fh.ascent) + (float)ceil(fh.descent) + fPad.top + fPad.bottom);

		for (int i = 0; i < fItems.CountItems(); i++)
		{
			item = (BMenuItem*)fItems.ItemAt(i);
			
			item->GetContentSize(&iWidth, &iHeight);

			item->fBounds.left = frame.right;
			item->fBounds.top = 0.0f;
			item->fBounds.right = item->fBounds.left + iWidth + fPad.left + fPad.right;

			frame.right = item->fBounds.right + 1.0f;
			frame.bottom = max_c(frame.bottom, iHeight + fPad.top + fPad.bottom);
		}

		for (int i = 0; i < fItems.CountItems(); i++)
			ItemAt(i)->fBounds.bottom = frame.bottom;

		frame.right = (float)ceil(frame.right) + 8.0f;
	}

	if ((ResizingMode() & B_FOLLOW_LEFT_RIGHT) ==  B_FOLLOW_LEFT_RIGHT)
	{
		if (Parent())
			*width = Parent()->Frame().Width() + 1.0f;
		else
			*width = Window()->Frame().Width() + 1.0f;

		*height = frame.Height();
	}
	else
	{
		*width = frame.Width();
		*height = frame.Height();
	}
}
//------------------------------------------------------------------------------
BRect BMenu::Bump(BRect current, BPoint extent, int32 index) const
{
	return BRect();
}
//------------------------------------------------------------------------------
BPoint BMenu::ItemLocInRect(BRect frame) const
{
	return BPoint();
}
//------------------------------------------------------------------------------
BRect BMenu::CalcFrame(BPoint where, bool *scrollOn)
{
	return BRect();
}
//------------------------------------------------------------------------------
bool BMenu::ScrollMenu(BRect bounds, BPoint loc, bool *fast)
{
	return false;
}
//------------------------------------------------------------------------------
void BMenu::ScrollIntoView(BMenuItem *item)
{
}
//------------------------------------------------------------------------------
void BMenu::DrawItems(BRect updateRect)
{
	for (int i =0; i < fItems.CountItems(); i++)
	{
		if (ItemAt(i)->Frame().Intersects(updateRect))
			ItemAt(i)->Draw();
	}
}
//------------------------------------------------------------------------------
int BMenu::State(BMenuItem **item) const
{
	return 0;
}
//------------------------------------------------------------------------------
void BMenu::InvokeItem(BMenuItem *item, bool now)
{
	if (item->Submenu())
		item->Submenu()->Show();
	else if (IsRadioMode())
		item->SetMarked(true);

	item->Invoke();
}
//------------------------------------------------------------------------------
bool BMenu::OverSuper(BPoint loc)
{
	// TODO: we assume that loc is in screen coords
	if (!Supermenu())
		return false;
	
	return Supermenu()->Window()->Frame().Contains(loc);
}
//------------------------------------------------------------------------------
bool BMenu::OverSubmenu(BMenuItem *item, BPoint loc)
{
	// TODO: we assume that loc is in screen coords
	if (!item->Submenu())
		return false;
	
	return item->Submenu()->Window()->Frame().Contains(loc);
}
//------------------------------------------------------------------------------
BMenuWindow	*BMenu::MenuWindow()
{
	return fCachedMenuWindow;
}
//------------------------------------------------------------------------------
void BMenu::DeleteMenuWindow()
{
	delete fCachedMenuWindow;
	fCachedMenuWindow = NULL;
}
//------------------------------------------------------------------------------
BMenuItem *BMenu::HitTestItems(BPoint where, BPoint slop) const
{
	for (int i =0; i < CountItems(); i++)
		if (ItemAt(i)->fBounds.Contains(where))
			return ItemAt(i);

	return NULL;
}
//------------------------------------------------------------------------------
BRect BMenu::Superbounds() const
{
	return BRect();
}
//------------------------------------------------------------------------------
void BMenu::CacheFontInfo()
{
	font_height fh;
	GetFontHeight(&fh);
	fAscent = fh.ascent;
	fDescent = fh.descent;
	fFontHeight = (float)ceil(fh.ascent + fh.descent + 0.5f);
}
//------------------------------------------------------------------------------
void BMenu::ItemMarked(BMenuItem *item)
{
	if (IsRadioMode())
	{
		for (int32 i = 0; i < CountItems(); i++)
			if (ItemAt(i) != item)
				ItemAt(i)->SetMarked(false);
	}

	if (IsLabelFromMarked() && Superitem())
		Superitem()->SetLabel(item->Label());
}
//------------------------------------------------------------------------------
void BMenu::Install(BWindow *target)
{
	for (int i =0; i < CountItems(); i++)
		ItemAt(i)->Install(target);
}
//------------------------------------------------------------------------------
void BMenu::Uninstall()
{
}
//------------------------------------------------------------------------------
void BMenu::SelectItem(BMenuItem *m, uint32 showSubmenu,bool selectFirstItem)
{
}
//------------------------------------------------------------------------------
BMenuItem *BMenu::CurrentSelection() const
{
	return fSelected;
}
//------------------------------------------------------------------------------
bool BMenu::SelectNextItem(BMenuItem *item, bool forward)
{
	return false;
}
//------------------------------------------------------------------------------
BMenuItem *BMenu::NextItem(BMenuItem *item, bool forward) const
{
	return NULL;
}
//------------------------------------------------------------------------------
bool BMenu::IsItemVisible(BMenuItem *item) const
{
	return false;
}
//------------------------------------------------------------------------------
void BMenu::SetIgnoreHidden(bool on)
{
	fIgnoreHidden = on;
}
//------------------------------------------------------------------------------
void BMenu::SetStickyMode(bool on)
{
	fStickyMode = on;
}
//------------------------------------------------------------------------------
bool BMenu::IsStickyMode() const
{
	return fStickyMode;
}
//------------------------------------------------------------------------------
void BMenu::CalcTriggers()
{
}
//------------------------------------------------------------------------------
const char *BMenu::ChooseTrigger(const char *title, BList *chars)
{
	return NULL;
}
//------------------------------------------------------------------------------
void BMenu::UpdateWindowViewSize(bool upWind)
{
}
//------------------------------------------------------------------------------
bool BMenu::IsStickyPrefOn()
{
	return false;
}
//------------------------------------------------------------------------------
void BMenu::RedrawAfterSticky(BRect bounds)
{
}
//------------------------------------------------------------------------------
bool BMenu::OkToProceed(BMenuItem *)
{
	return false;
}
//------------------------------------------------------------------------------
status_t BMenu::ParseMsg(BMessage *msg, int32 *sindex, BMessage *spec,
						 int32 *form, const char **prop, BMenu **tmenu,
						 BMenuItem **titem, int32 *user_data,
						 BMessage *reply) const
{
	return B_ERROR;
}
//------------------------------------------------------------------------------
status_t BMenu::DoMenuMsg(BMenuItem **next, BMenu *tar, BMessage *m,
						  BMessage *r, BMessage *spec, int32 f) const
{
	return B_ERROR;
}
//------------------------------------------------------------------------------
status_t BMenu::DoMenuItemMsg(BMenuItem **next, BMenu *tar, BMessage *m,
							  BMessage *r, BMessage *spec, int32 f) const
{
	return B_ERROR;
}
//------------------------------------------------------------------------------
status_t BMenu::DoEnabledMsg(BMenuItem *ti, BMenu *tm, BMessage *m,
							 BMessage *r) const
{
	return B_ERROR;
}
//------------------------------------------------------------------------------
status_t BMenu::DoLabelMsg(BMenuItem *ti, BMenu *tm, BMessage *m,
						   BMessage *r) const
{
	return B_ERROR;
}
//------------------------------------------------------------------------------
status_t BMenu::DoMarkMsg(BMenuItem *ti, BMenu *tm, BMessage *m,
						  BMessage *r) const
{
	return B_ERROR;
}
//------------------------------------------------------------------------------
status_t BMenu::DoDeleteMsg(BMenuItem *ti, BMenu *tm, BMessage *m,
							BMessage *r) const
{
	return B_ERROR;
}
//------------------------------------------------------------------------------
status_t BMenu::DoCreateMsg(BMenuItem *ti, BMenu *tm, BMessage *m,
							BMessage *r, bool menu) const
{
	return B_ERROR;
}
//------------------------------------------------------------------------------



// Temporary mouse hooks
#if 0
//------------------------------------------------------------------------------
void BMenu::MouseDown(BPoint point)
{
	BMenuItem *item = HitTestItems(point);

	if (item)
		InvokeItem(item);

	Hide();
}
//------------------------------------------------------------------------------
void BMenu::MouseMoved(BPoint point, uint32 transit, const BMessage *message)
{
	BMenuItem *item = HitTestItems(point);
	
	if (fSelected)
	{
		if (fSelected == item)
			return;
		
		fSelected->fSelected = false;
		//if (fSelected->Submenu())
		//	fSelected->Submenu()->Hide();
		Invalidate(fSelected->Frame());
	}

	fSelected = item;

	if (fSelected)
	{
		fSelected->fSelected = true;
		//if (fSelected->Submenu())
		//	fSelected->Submenu()->Show();
		Invalidate(fSelected->Frame());
	}
}
//------------------------------------------------------------------------------
void BMenu::MouseUp(BPoint point)
{

}
#endif
//------------------------------------------------------------------------------
status_t set_menu_info(menu_info *info)
{
	BPath path;

	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
		return B_OK;

	path.Append("menu_settings");

	BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE);

	if (file.InitCheck() != B_OK)
		return B_OK;

	file.Write(info, sizeof(menu_info));

	return B_OK;
}
//------------------------------------------------------------------------------
status_t get_menu_info(menu_info *info)
{
	info->font_size = 8;
	memcpy(info->f_family, "Arial", 6);
	memcpy(info->f_style, "Regular", 8);
	info->background_color = ui_color(B_MENU_BACKGROUND_COLOR);
	info->separator = 0;
	info->click_to_open = true;
	info->triggers_always_shown = false;

	BPath path;

	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
		return B_OK;

	path.Append("menu_settings");

	BFile file(path.Path(), B_READ_ONLY);

	if (file.InitCheck() != B_OK)
		return B_OK;

	file.Read(info, sizeof(menu_info));

	return B_OK;
}
//------------------------------------------------------------------------------
