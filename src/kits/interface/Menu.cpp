/*
 * Copyright 2001-2006, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Stefano Ceccherini (burton666@libero.it)
 */

#include <new>
#include <string.h>

#include <Debug.h>
#include <File.h>
#include <FindDirectory.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Path.h>
#include <PropertyInfo.h>
#include <Screen.h>
#include <Window.h>

#include <AppServerLink.h>
#include <BMCPrivate.h>
#include <MenuPrivate.h>
#include <MenuWindow.h>
#include <ServerProtocol.h>

using std::nothrow;

class _ExtraMenuData_ {
public:
	menu_tracking_hook trackingHook;
	void *trackingState;

	_ExtraMenuData_(menu_tracking_hook func, void *state)
	{
		trackingHook = func;
		trackingState = state;
	};
};


menu_info BMenu::sMenuInfo;
bool BMenu::sAltAsCommandKey;


static property_info
sPropList[] = {
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
	{0}
};


const char *kEmptyMenuLabel = "<empty>";


BMenu::BMenu(const char *name, menu_layout layout)
	:	BView(BRect(0, 0, 0, 0), name, 0, B_WILL_DRAW),
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
		fUseCachedMenuLayout(false),
		fEnabled(true),
		fDynamicName(false),
		fRadioMode(false),
		fTrackNewBounds(false),
		fStickyMode(false),
		fIgnoreHidden(true),
		fTriggerEnabled(true),
		fRedrawAfterSticky(false),
		fAttachAborted(false)
{
	InitData(NULL);
}


BMenu::BMenu(const char *name, float width, float height)
	:	BView(BRect(0.0f, width, 0.0f, height), name, 0, B_WILL_DRAW),
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
		fUseCachedMenuLayout(false),
		fEnabled(true),
		fDynamicName(false),
		fRadioMode(false),
		fTrackNewBounds(false),
		fStickyMode(false),
		fIgnoreHidden(true),
		fTriggerEnabled(true),
		fRedrawAfterSticky(false),
		fAttachAborted(false)
{
	InitData(NULL);
}


BMenu::~BMenu()
{
	DeleteMenuWindow();

	RemoveItems(0, CountItems(), true);
		
	delete fInitMatrixSize;
	delete fExtraMenuData;
}


BMenu::BMenu(BMessage *archive)
	:	BView(archive),
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
		fLayout(B_ITEMS_IN_ROW),
		fExtraRect(NULL),
		fMaxContentWidth(0.0f),
		fInitMatrixSize(NULL),
		fExtraMenuData(NULL),
		fTrigger(0),
		fResizeToFit(true),
		fUseCachedMenuLayout(false),
		fEnabled(true),
		fDynamicName(false),
		fRadioMode(false),
		fTrackNewBounds(false),
		fStickyMode(false),
		fIgnoreHidden(true),
		fTriggerEnabled(true),
		fRedrawAfterSticky(false),
		fAttachAborted(false)
{
	InitData(archive);
}


BArchivable *
BMenu::Instantiate(BMessage *data)
{
	if (validate_instantiation(data, "BMenu"))
		return new (nothrow) BMenu(data);
	
	return NULL;
}


status_t
BMenu::Archive(BMessage *data, bool deep) const
{
	status_t err = BView::Archive(data, deep);

	if (err == B_OK && Layout() != B_ITEMS_IN_ROW)
		err = data->AddInt32("_layout", Layout());
	if (err == B_OK)
		err = data->AddBool("_rsize_to_fit", fResizeToFit);
	if (err == B_OK)
		err = data->AddBool("_disable", !IsEnabled());
	if (err ==  B_OK)
		err = data->AddBool("_radio", IsRadioMode());
	if (err == B_OK)
		err = data->AddBool("_trig_disabled", AreTriggersEnabled());
	if (err == B_OK)
		err = data->AddBool("_dyn_label", fDynamicName);
	if (err == B_OK)
		err = data->AddFloat("_maxwidth", fMaxContentWidth);
	if (err == B_OK && deep) {
		// TODO store items and rects
	}

	return err;
}


void
BMenu::AttachedToWindow()
{
	BView::AttachedToWindow();

	sAltAsCommandKey = true;
	key_map *keys = NULL; 
	char *chars = NULL; 
	get_key_map(&keys, &chars);
	if (keys == NULL || keys->left_command_key != 0x5d || keys->right_command_key != 0x5f)
		sAltAsCommandKey = false;
	free(chars);
	free(keys);

	BMenuItem *superItem = Superitem();
	BMenu *superMenu = Supermenu();
	if (AddDynamicItem(B_INITIAL_ADD)) {
		do {
			if (superMenu != NULL && !superMenu->OkToProceed(superItem)) {
				AddDynamicItem(B_ABORT);
				fAttachAborted = true;
				break;
			}
		} while (AddDynamicItem(B_PROCESSING));
	}

	if (!fAttachAborted) {
		CacheFontInfo();
		LayoutItems(0);	
	}
}


void
BMenu::DetachedFromWindow()
{
	BView::DetachedFromWindow();
}


bool
BMenu::AddItem(BMenuItem *item)
{
	return AddItem(item, CountItems());
}


bool
BMenu::AddItem(BMenuItem *item, int32 index)
{
	if (fLayout == B_ITEMS_IN_MATRIX)
		debugger("BMenu::AddItem(BMenuItem *, int32) this method can only "
				"be called if the menu layout is not B_ITEMS_IN_MATRIX");

	if (!_AddItem(item, index))
		return false;

	InvalidateLayout();
	if (LockLooper()) {
		if (!Window()->IsHidden()) {
			LayoutItems(index);
			Invalidate();
		} 
		UnlockLooper();
	}
	return true;
}


bool
BMenu::AddItem(BMenuItem *item, BRect frame)
{
	if (fLayout != B_ITEMS_IN_MATRIX)
		debugger("BMenu::AddItem(BMenuItem *, BRect) this method can only "
			"be called if the menu layout is B_ITEMS_IN_MATRIX");
			
	if (!item)
		return false;
	
	item->fBounds = frame;

	int32 index = CountItems();
	if (!_AddItem(item, index)) {
		return false;
	}

	if (LockLooper()) {
		if (!Window()->IsHidden()) {
			LayoutItems(index);
			Invalidate();
		} 
		UnlockLooper();
	}

	return true;
}


bool 
BMenu::AddItem(BMenu *submenu)
{
	BMenuItem *item = new (nothrow) BMenuItem(submenu);
	if (!item)
		return false;
	
	if (!AddItem(item, CountItems())) {
		item->fSubmenu = NULL;
		delete item;
		return false;
	}

	return true;
}


bool
BMenu::AddItem(BMenu *submenu, int32 index)
{
	if (fLayout == B_ITEMS_IN_MATRIX)
		debugger("BMenu::AddItem(BMenuItem *, int32) this method can only "
				"be called if the menu layout is not B_ITEMS_IN_MATRIX");

	BMenuItem *item = new (nothrow) BMenuItem(submenu);
	if (!item)
		return false;
		
	if (!AddItem(item, index)) {
		item->fSubmenu = NULL;
		delete item;
		return false;
	}

	return true;
}


bool
BMenu::AddItem(BMenu *submenu, BRect frame)
{
	if (fLayout != B_ITEMS_IN_MATRIX)
		debugger("BMenu::AddItem(BMenu *, BRect) this method can only "
			"be called if the menu layout is B_ITEMS_IN_MATRIX");
	
	BMenuItem *item = new (nothrow) BMenuItem(submenu);
	if (!item)
		return false;

	if (!AddItem(item, frame)) {
		item->fSubmenu = NULL;
		delete item;
		return false;
	}

	return true;
}


bool
BMenu::AddList(BList *list, int32 index)
{
	// TODO: test this function, it's not documented in the bebook.
	if (list == NULL)
		return false;
	
	bool locked = LockLooper();

	int32 numItems = list->CountItems();
	for (int32 i = 0; i < numItems; i++) {
		BMenuItem *item = static_cast<BMenuItem *>(list->ItemAt(i));
		if (item != NULL) {
			if (!_AddItem(item, index + i))
				break;
		}
	}
	
	InvalidateLayout();
	if (locked && Window() != NULL && !Window()->IsHidden()) {	
		// Make sure we update the layout if needed.
		LayoutItems(index);
		//UpdateWindowViewSize();
		Invalidate();		
	}

	if (locked)
		UnlockLooper();

	return true;
}


bool
BMenu::AddSeparatorItem()
{
	BMenuItem *item = new (nothrow) BSeparatorItem();
	if (!item || !AddItem(item, CountItems())) {
		delete item;
		return false;
	}

	return true;
}


bool
BMenu::RemoveItem(BMenuItem *item)
{
	// TODO: Check if item is also deleted
	return RemoveItems(0, 0, item, false);
}


BMenuItem *
BMenu::RemoveItem(int32 index)
{
	BMenuItem *item = ItemAt(index);
	if (item != NULL)
		RemoveItems(0, 0, item, false);
	return item;
}


bool
BMenu::RemoveItems(int32 index, int32 count, bool del)
{
	return RemoveItems(index, count, NULL, del);	
}


bool
BMenu::RemoveItem(BMenu *submenu)
{
	for (int32 i = 0; i < fItems.CountItems(); i++) {
		if (static_cast<BMenuItem *>(fItems.ItemAtFast(i))->Submenu() == submenu)
			return RemoveItems(i, 1, NULL, false);
	}

	return false;
}


int32
BMenu::CountItems() const
{
	return fItems.CountItems();
}


BMenuItem *
BMenu::ItemAt(int32 index) const
{
	return static_cast<BMenuItem *>(fItems.ItemAt(index));
}


BMenu *
BMenu::SubmenuAt(int32 index) const
{
	BMenuItem *item = static_cast<BMenuItem *>(fItems.ItemAt(index));
	return (item != NULL) ? item->Submenu() : NULL;
}


int32
BMenu::IndexOf(BMenuItem *item) const
{
	return fItems.IndexOf(item);
}


int32
BMenu::IndexOf(BMenu *submenu) const
{
	for (int32 i = 0; i < fItems.CountItems(); i++) {
		if (ItemAt(i)->Submenu() == submenu)
			return i;
	}

	return -1;
}


BMenuItem *
BMenu::FindItem(const char *label) const
{
	BMenuItem *item = NULL;

	for (int32 i = 0; i < CountItems(); i++) {
		item = ItemAt(i);

		if (item->Label() && strcmp(item->Label(), label) == 0)
			return item;

		if (item->Submenu() != NULL) {
			item = item->Submenu()->FindItem(label);
			if (item != NULL)
				return item;
		}
	}

	return NULL;
}


BMenuItem *
BMenu::FindItem(uint32 command) const
{
	BMenuItem *item = NULL;

	for (int32 i = 0; i < CountItems(); i++) {
		item = ItemAt(i);

		if (item->Command() == command)
			return item;

		if (item->Submenu() != NULL) {
			item = item->Submenu()->FindItem(command);
			if (item != NULL)
				return item;
		}
	}

	return NULL;
}


status_t 
BMenu::SetTargetForItems(BHandler *handler)
{
	status_t status = B_OK;
	for (int32 i = 0; i < fItems.CountItems(); i++) {
		status = ItemAt(i)->SetTarget(handler);
		if (status < B_OK)
			break;
	}
	
	return status;
}


status_t
BMenu::SetTargetForItems(BMessenger messenger)
{
	status_t status = B_OK;
	for (int32 i = 0; i < fItems.CountItems(); i++) {
		status = ItemAt(i)->SetTarget(messenger);
		if (status < B_OK)
			break;
	}

	return status;
}


void
BMenu::SetEnabled(bool enabled)
{
	if (fEnabled == enabled)
		return;

	fEnabled = enabled;
	
	if (fSuperitem)
		fSuperitem->SetEnabled(enabled);
}


void
BMenu::SetRadioMode(bool flag)
{
	fRadioMode = flag;
	if (!flag)
		SetLabelFromMarked(false);
}


void
BMenu::SetTriggersEnabled(bool flag)
{
	fTriggerEnabled = flag;
}


void
BMenu::SetMaxContentWidth(float width)
{
	fMaxContentWidth = width;
}


void
BMenu::SetLabelFromMarked(bool flag)
{
	fDynamicName = flag;
	if (flag)
		SetRadioMode(true);
}


bool
BMenu::IsLabelFromMarked()
{
	return fDynamicName;
}


bool
BMenu::IsEnabled() const
{
	if (!fEnabled)
		return false;

	return fSuper ? fSuper->IsEnabled() : true ;
}


bool
BMenu::IsRadioMode() const
{
	return fRadioMode;
}


bool
BMenu::AreTriggersEnabled() const
{
	return fTriggerEnabled;
}


bool
BMenu::IsRedrawAfterSticky() const
{
	return fRedrawAfterSticky;
}


float
BMenu::MaxContentWidth() const
{
	return fMaxContentWidth;
}


BMenuItem *
BMenu::FindMarked()
{
	for (int32 i = 0; i < fItems.CountItems(); i++) {
		BMenuItem *item = ItemAt(i);
		if (item->IsMarked())
			return item;
	}

	return NULL;
}


BMenu *
BMenu::Supermenu() const
{
	return fSuper;
}


BMenuItem *
BMenu::Superitem() const
{
	return fSuperitem;
}


void
BMenu::MessageReceived(BMessage *msg)
{
	BView::MessageReceived(msg);
}


void
BMenu::KeyDown(const char *bytes, int32 numBytes)
{
	switch (bytes[0]) {
		case B_UP_ARROW:
		{
			if (fSelected) 	{
				fSelected->fSelected = false;

				if (fSelected == fItems.FirstItem())
					fSelected = static_cast<BMenuItem *>(fItems.LastItem());
				else
					fSelected = ItemAt(IndexOf(fSelected) - 1);
			} else
				fSelected = static_cast<BMenuItem *>(fItems.LastItem());

			fSelected->fSelected = true;

			break;
		}
		case B_DOWN_ARROW:
		{
			if (fSelected) {
				fSelected->fSelected = false;

				if (fSelected == fItems.LastItem())
					fSelected = static_cast<BMenuItem *>(fItems.FirstItem());
				else
					fSelected = ItemAt(IndexOf(fSelected) + 1);
			} else
				fSelected = static_cast<BMenuItem *>(fItems.FirstItem());

			fSelected->fSelected = true;

			break;
		}
		case B_HOME:
		{
			if (fSelected)
				fSelected->fSelected = false;

			fSelected = static_cast<BMenuItem *>(fItems.FirstItem());
			fSelected->fSelected = true;

			break;
		}
		case B_END:
		{
			if (fSelected)
				fSelected->fSelected = false;

			fSelected = static_cast<BMenuItem *>(fItems.LastItem());
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


void
BMenu::Draw(BRect updateRect)
{
	if (RelayoutIfNeeded()) {
		Invalidate();
		return;
	}

	DrawBackground(updateRect);
	DrawItems(updateRect);
}


void
BMenu::GetPreferredSize(float *_width, float *_height)
{
	ComputeLayout(0, true, false, _width, _height);
}


void
BMenu::ResizeToPreferred()
{
	BView::ResizeToPreferred();
}


void
BMenu::FrameMoved(BPoint new_position)
{
	BView::FrameMoved(new_position);
}


void
BMenu::FrameResized(float new_width, float new_height)
{
	BView::FrameResized(new_width, new_height);
}


void
BMenu::InvalidateLayout()
{
	fUseCachedMenuLayout = false;
}


BHandler *
BMenu::ResolveSpecifier(BMessage *msg, int32 index, BMessage *specifier,
						int32 form, const char *property)
{
	BPropertyInfo propInfo(sPropList);
	BHandler *target = NULL;

	switch (propInfo.FindMatch(msg, 0, specifier, form, property)) {
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


status_t
BMenu::GetSupportedSuites(BMessage *data)
{
	if (data == NULL)
		return B_BAD_VALUE;

	status_t err = data->AddString("suites", "suite/vnd.Be-menu");

	if (err < B_OK)
		return err;
	
	BPropertyInfo propertyInfo(sPropList);
	err = data->AddFlat("messages", &propertyInfo);

	if (err < B_OK)
		return err;
	
	return BView::GetSupportedSuites(data);
}


status_t 
BMenu::Perform(perform_code d, void *arg)
{
	return BView::Perform(d, arg);
}


void
BMenu::MakeFocus(bool focused)
{
	BView::MakeFocus(focused);
}


void
BMenu::AllAttached()
{
	BView::AllAttached();
}


void
BMenu::AllDetached()
{
	BView::AllDetached();
}


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
		fUseCachedMenuLayout(false),
		fEnabled(true),
		fDynamicName(false),
		fRadioMode(false),
		fTrackNewBounds(false),
		fStickyMode(false),
		fIgnoreHidden(true),
		fTriggerEnabled(true),
		fRedrawAfterSticky(false),
		fAttachAborted(false)
{
	InitData(NULL);
}


void
BMenu::SetItemMargins(float left, float top, float right, float bottom)
{
	fPad.Set(left, top, right, bottom);
}


void
BMenu::GetItemMargins(float *left, float *top, float *right,
						   float *bottom) const
{
	if (left != NULL)
		*left = fPad.left;
	if (top != NULL)
		*top = fPad.top;
	if (right != NULL)
		*right = fPad.right;
	if (bottom != NULL)
		*bottom = fPad.bottom;
}


menu_layout
BMenu::Layout() const
{
	return fLayout;
}


void
BMenu::Show()
{
	Show(false);
}


void
BMenu::Show(bool selectFirst)
{
	Install(NULL);
	_show(selectFirst);
}


void
BMenu::Hide()
{
	_hide();
	Uninstall();
}


BMenuItem *
BMenu::Track(bool sticky, BRect *clickToOpenRect)
{	
	if (sticky && LockLooper()) {
		RedrawAfterSticky(Bounds());
		UnlockLooper();
	}
	
	if (clickToOpenRect != NULL && LockLooper()) {
		fExtraRect = clickToOpenRect;
		ConvertFromScreen(fExtraRect);
		UnlockLooper();
	}

	// If sticky is false, pass 0 to the tracking function
	// so the menu will stay in nonsticky mode
	const bigtime_t trackTime = sticky ? system_time() : 0;
	int action;
	BMenuItem *menuItem = _track(&action, trackTime);
	
	SetStickyMode(false);
	fExtraRect = NULL;
	
	return menuItem;
}


bool
BMenu::AddDynamicItem(add_state s)
{
	// Implemented in subclasses
	return false;
}


void
BMenu::DrawBackground(BRect update)
{
	rgb_color oldColor = HighColor();
	SetHighColor(sMenuInfo.background_color);
	FillRect(Bounds() & update, B_SOLID_HIGH);
	SetHighColor(oldColor);
}


void
BMenu::SetTrackingHook(menu_tracking_hook func, void *state)
{
	delete fExtraMenuData;
	fExtraMenuData = new (nothrow) _ExtraMenuData_(func, state);
}


void BMenu::_ReservedMenu3() {}
void BMenu::_ReservedMenu4() {}
void BMenu::_ReservedMenu5() {}
void BMenu::_ReservedMenu6() {}


BMenu &
BMenu::operator=(const BMenu &)
{
	return *this;
}


void
BMenu::InitData(BMessage *data)
{
	// TODO: Get _color, _fname, _fflt from the message, if present
	BFont font;
	font.SetFamilyAndStyle(sMenuInfo.f_family, sMenuInfo.f_style);
	font.SetSize(sMenuInfo.font_size);
	SetFont(&font, B_FONT_FAMILY_AND_STYLE | B_FONT_SIZE);
	
	SetLowColor(sMenuInfo.background_color);
	SetViewColor(sMenuInfo.background_color);
				
	if (data != NULL) {
		data->FindInt32("_layout", (int32 *)&fLayout);
		data->FindBool("_rsize_to_fit", &fResizeToFit);
		data->FindBool("_disable", &fEnabled);
		data->FindBool("_radio", &fRadioMode);
		
		bool disableTrigger = false;
		data->FindBool("_trig_disabled", &disableTrigger);
		fTriggerEnabled = !disableTrigger;
		
		data->FindBool("_dyn_label", &fDynamicName);
		data->FindFloat("_maxwidth", &fMaxContentWidth);
	}
}


bool
BMenu::_show(bool selectFirstItem)
{
	// See if the supermenu has a cached menuwindow,
	// and use that one if possible.
	BMenuWindow *window = NULL;
	bool ourWindow = false;
	if (fSuper != NULL) {
		fSuperbounds = fSuper->ConvertToScreen(fSuper->Bounds());
		window = fSuper->MenuWindow();
	}
	
	// Otherwise, create a new one
	// This happens for "stand alone" BPopUpMenus
	// (i.e. not within a BMenuField)
	if (window == NULL) {
		// Menu windows get the BMenu's handler name
		window = new (nothrow) BMenuWindow(Name());
		ourWindow = true;
	}

	if (window == NULL)
		return false;
	
	if (window->Lock()) {
		fAttachAborted = false;
		window->AttachMenu(this);

		// Menu didn't have the time to add its items: aborting...
		if (fAttachAborted) {
			window->DetachMenu();
			// TODO: Probably not needed, we can just let _hide() quit the window
			if (ourWindow)
				window->Quit();
			else
				window->Unlock();
			return false;
		}
	
		// Move the BMenu to 1, 1, if it's attached to a BMenuWindow,
		// (that means it's a BMenu, BMenuBars are attached to regular BWindows).
		// This is needed to be able to draw the frame around the BMenu.
		if (dynamic_cast<BMenuWindow *>(window) != NULL)
			MoveTo(1, 1);
	
		UpdateWindowViewSize();
		window->Show();
		
		if (selectFirstItem)
			SelectItem(ItemAt(0));
		
		window->Unlock();
	}
	
	return true;
}


void
BMenu::_hide()
{
	BMenuWindow *window = static_cast<BMenuWindow *>(Window());
	if (window == NULL || !window->Lock())
		return;
	
	if (fSelected != NULL)
		SelectItem(NULL);

	window->Hide();
	window->DetachMenu();
		// we don't want to be deleted when the window is removed

	// Delete the menu window used by our submenus
	DeleteMenuWindow();

	window->Unlock();

	if (fSuper == NULL && window->Lock())
		window->Quit();
}


const bigtime_t kHysteresis = 200000; // TODO: Test and reduce if needed.
	

BMenuItem *
BMenu::_track(int *action, bigtime_t trackTime, long start)
{
	// TODO: cleanup
	BMenuItem *item = NULL;
	bigtime_t openTime = system_time();
	bigtime_t closeTime = 0; 
	
	fState = MENU_STATE_TRACKING;
	if (fSuper != NULL)
		fSuper->fState = MENU_STATE_TRACKING_SUBMENU;

	while (true) {
		if (fExtraMenuData != NULL && fExtraMenuData->trackingHook != NULL
			&& fExtraMenuData->trackingState != NULL) {
			/*bool result =*/ fExtraMenuData->trackingHook(this, fExtraMenuData->trackingState);
			//printf("tracking hook returned %s\n", result ? "true" : "false");
		}
		
		bool locked = LockLooper();
		if (!locked)
			break;

		bigtime_t snoozeAmount = 50000;
		BPoint location;
		ulong buttons;
		GetMouse(&location, &buttons, false);
			// Get the current mouse state
		
		Window()->UpdateIfNeeded();
		BPoint screenLocation = ConvertToScreen(location);
		item = HitTestItems(location, B_ORIGIN);
		if (item != NULL) {
			if (item != fSelected && system_time() > closeTime + kHysteresis) {
				SelectItem(item, -1);
				openTime = system_time();
			} else if (system_time() > kHysteresis + openTime && item->Submenu() != NULL
				&& item->Submenu()->Window() == NULL) {
				// Open the submenu if it's not opened yet, but only if
				// the mouse pointer stayed over there for some time
				// (hysteresis)
				SelectItem(item);
				closeTime = system_time();
			}
			fState = MENU_STATE_TRACKING;
		}

		// Track the submenu
		if (fSelected != NULL && OverSubmenu(fSelected, screenLocation)) {
			UnlockLooper();
			locked = false;
			int submenuAction = MENU_STATE_TRACKING;
			BMenu *submenu = fSelected->Submenu();
			if (IsStickyMode())
				submenu->SetStickyMode(true);
			BMenuItem *submenuItem = submenu->_track(&submenuAction, trackTime);
			if (submenuAction == MENU_STATE_CLOSED) {
				item = submenuItem;
				fState = submenuAction;
				break;
			}

			locked = LockLooper();
			if (!locked)
				break;
		}

		if (item == NULL) {
			if (OverSuper(screenLocation)) {
				fState = MENU_STATE_TRACKING;
				UnlockLooper();
				break;
			}

			if (fSelected != NULL && !OverSubmenu(fSelected, screenLocation)
				&& system_time() > closeTime + kHysteresis
				&& fState != MENU_STATE_TRACKING_SUBMENU) {
				SelectItem(NULL);
				fState = MENU_STATE_TRACKING;
			}

			if (fSuper != NULL) {
				if (locked)
					UnlockLooper();
				*action = fState;
				return NULL;				
			}
		}
		
		if (locked)
			UnlockLooper();
		
		if (buttons != 0 && IsStickyMode())
			fState = MENU_STATE_CLOSED;
		else if (buttons == 0 && !IsStickyMode()) {
			if (system_time() < trackTime + 1000000
				|| (fExtraRect != NULL && fExtraRect->Contains(location)))
				SetStickyMode(true);
			else
				fState = MENU_STATE_CLOSED;
		}

		if (fState == MENU_STATE_CLOSED)
			break;

		snooze(snoozeAmount);
	}

	if (action != NULL)
		*action = fState;

	if (fSelected != NULL && LockLooper()) {
		SelectItem(NULL);
		UnlockLooper();
	}

	if (IsStickyMode())
		SetStickyMode(false);

	// delete the menu window recycled for all the child menus
	DeleteMenuWindow();
	
	return item;
}


bool
BMenu::_AddItem(BMenuItem *item, int32 index)
{
	ASSERT(item != NULL);
	if (index < 0 || index > CountItems())
		return false;
	
	if (!fItems.AddItem(item, index))
		return false;
	
	// install the item on the supermenu's window 
	// or onto our window, if we are a root menu
	BWindow* window = NULL;
	if (Superitem() != NULL)
		window = Superitem()->fWindow;
	else
		window = Window();
	if (window != NULL)
		item->Install(window);

	item->SetSuper(this);

	return true;
}


bool
BMenu::RemoveItems(int32 index, int32 count, BMenuItem *item, bool deleteItems)
{
	bool success = false;
	bool invalidateLayout = false;

	bool locked = LockLooper();
	BWindow *window = Window();

	// The plan is simple: If we're given a BMenuItem directly, we use it
	// and ignore index and count. Otherwise, we use them instead.
	if (item != NULL) {
		if (fItems.RemoveItem(item)) {
			if (item == fSelected && window != NULL)
				SelectItem(NULL);
			item->Uninstall();	
			item->SetSuper(NULL);
			if (deleteItems)
				delete item;
			success = invalidateLayout = true;
		}
	} else {
		// We iterate backwards because it's simpler
		int32 i = min_c(index + count - 1, fItems.CountItems() - 1);
		// NOTE: the range check for "index" is done after
		// calculating the last index to be removed, so
		// that the range is not "shifted" unintentionally
		index = max_c(0, index);
		for (; i >= index; i--) {
			item = static_cast<BMenuItem*>(fItems.ItemAt(i));
			if (item != NULL) {
				if (fItems.RemoveItem(item)) {
					if (item == fSelected && window != NULL)
						SelectItem(NULL);
					item->Uninstall();
					item->SetSuper(NULL);
					if (deleteItems)
						delete item;
					success = true;
					invalidateLayout = true;
				} else {
					// operation not entirely successful
					success = false;
					break;
				}
			}
		}
	}

	if (invalidateLayout && locked && window != NULL) {
		LayoutItems(0);
		//UpdateWindowViewSize();
		Invalidate();
	}

	if (locked)
		UnlockLooper();

	return success;
}


bool
BMenu::RelayoutIfNeeded()
{
	if (!fUseCachedMenuLayout) {
		fUseCachedMenuLayout = true;
		CacheFontInfo();
		LayoutItems(0);
		return true;
	}
	return false;
}


void
BMenu::LayoutItems(int32 index)
{
	CalcTriggers();

	float width, height;
	ComputeLayout(index, fResizeToFit, true, &width, &height);

	ResizeTo(width, height);
}


void
BMenu::ComputeLayout(int32 index, bool bestFit, bool moveItems,
	float* _width, float* _height)
{
	// TODO: Take "bestFit", "moveItems", "index" into account,
	// Recalculate only the needed items,
	// not the whole layout every time

	BRect frame(0, 0, 0, 0);
	float iWidth, iHeight;
	BMenuItem *item = NULL;

	switch (fLayout) {
		case B_ITEMS_IN_COLUMN:
		{
			for (int32 i = 0; i < fItems.CountItems(); i++) {
				item = ItemAt(i);	
				if (item != NULL) {
					item->GetContentSize(&iWidth, &iHeight);

					if (item->fModifiers && item->fShortcutChar)
						iWidth += 25.0f;
					if (item->fSubmenu != NULL)
						iWidth += 20.0f;

					item->fBounds.left = 0.0f;
					item->fBounds.top = frame.bottom;
					item->fBounds.bottom = item->fBounds.top + iHeight + fPad.top + fPad.bottom;

					frame.right = max_c(frame.right, iWidth + fPad.left + fPad.right);
					frame.bottom = item->fBounds.bottom + 1.0f;
				}
			}
			if (fMaxContentWidth > 0)
				frame.right = min_c(frame.right, fMaxContentWidth);

			if (moveItems) {
				for (int32 i = 0; i < fItems.CountItems(); i++)
					ItemAt(i)->fBounds.right = frame.right;
			}
			frame.right = ceilf(frame.right);
			frame.bottom--;
			break;
		}

		case B_ITEMS_IN_ROW:
		{
			font_height fh;
			GetFontHeight(&fh);
			frame = BRect(0.0f, 0.0f, 0.0f,	ceilf(fh.ascent + fh.descent + fPad.top + fPad.bottom));	

			for (int32 i = 0; i < fItems.CountItems(); i++) {
				item = ItemAt(i);
				if (item != NULL) {
					item->GetContentSize(&iWidth, &iHeight);

					item->fBounds.left = frame.right;
					item->fBounds.top = 0.0f;
					item->fBounds.right = item->fBounds.left + iWidth + fPad.left + fPad.right;

					frame.right = item->Frame().right + 1.0f;
					frame.bottom = max_c(frame.bottom, iHeight + fPad.top + fPad.bottom);
				}
			}
			
			if (moveItems) {
				for (int32 i = 0; i < fItems.CountItems(); i++)
					ItemAt(i)->fBounds.bottom = frame.bottom;			
			}

			frame.right = ceilf(frame.right);
			break;
		}

		case B_ITEMS_IN_MATRIX:
		{
			for (int32 i = 0; i < CountItems(); i++) {
				item = ItemAt(i);
				if (item != NULL) {
					frame.left = min_c(frame.left, item->Frame().left);
					frame.right = max_c(frame.right, item->Frame().right);
					frame.top = min_c(frame.top, item->Frame().top);
					frame.bottom = max_c(frame.bottom, item->Frame().bottom);
				}			
			}		
			break;
		}

		default:
			break;
	}

	if (_width) {
		// change width depending on resize mode - BMenuBars always span
		// over the whole width of its parent
		// TODO: this is certainly ugly, and it would be nice if there
		//	would be a cleaner solution to this problem
		if ((ResizingMode() & B_FOLLOW_LEFT_RIGHT) == B_FOLLOW_LEFT_RIGHT) {
			if (Parent())
				*_width = Parent()->Frame().Width() + 1;
			else if (Window())
				*_width = Window()->Frame().Width() + 1;
			else
				*_width = Bounds().Width();
		} else
			*_width = frame.Width();
	}

	if (_height)
		*_height = frame.Height();
	
	if (moveItems)
		fUseCachedMenuLayout = true;
}


BRect
BMenu::Bump(BRect current, BPoint extent, int32 index) const
{
	// ToDo: what's this?
	return BRect();
}


BPoint
BMenu::ItemLocInRect(BRect frame) const
{
	// ToDo: what's this?
	return BPoint();
}


BPoint
BMenu::ScreenLocation()
{
	BMenu *superMenu = Supermenu();
	BMenuItem *superItem = Superitem();

	if (superMenu == NULL || superItem == NULL) {
		debugger("BMenu can't determine where to draw."
			"Override BMenu::ScreenLocation() to determine location.");
	}

	BPoint point;
	if (superMenu->Layout() == B_ITEMS_IN_COLUMN)
		point = superItem->Frame().RightTop() + BPoint(1.0f, 1.0f);
	else
		point = superItem->Frame().LeftBottom() + BPoint(1.0f, 1.0f);

	superMenu->ConvertToScreen(&point);

	return point;
}


BRect
BMenu::CalcFrame(BPoint where, bool *scrollOn)
{
	// TODO: Improve me
	BRect bounds = Bounds();
	BRect frame = bounds.OffsetToCopy(where);

	BScreen screen(Window());
	BRect screenFrame = screen.Frame();

	BMenu *superMenu = Supermenu();
	BMenuItem *superItem = Superitem();

	if (scrollOn != NULL) {
		// basically, if this returns false, it means
		// that the menu frame won't fit completely inside the screen
		*scrollOn = !screenFrame.Contains(bounds);
	}

	// TODO: Horrible hack:
	// When added to a BMenuField, a BPopUpMenu is the child of
	// a _BMCItem_ inside a _BMCMenuBar_ to "fake" the menu hierarchy
	if (superMenu == NULL || superItem == NULL
		|| dynamic_cast<_BMCMenuBar_ *>(superMenu) != NULL) {
		// just move the window on screen

		if (frame.bottom > screenFrame.bottom)
			frame.OffsetBy(0, screenFrame.bottom - frame.bottom);
		else if (frame.top < screenFrame.top)
			frame.OffsetBy(0, -frame.top);

		if (frame.right > screenFrame.right)
			frame.OffsetBy(screenFrame.right - frame.right, 0);
		else if (frame.left < screenFrame.left)
			frame.OffsetBy(-frame.left, 0);

		return frame;
	}

	if (superMenu->Layout() == B_ITEMS_IN_COLUMN) {
		if (frame.right > screenFrame.right)
			frame.OffsetBy(-superItem->Frame().Width() - frame.Width() - 2, 0);

		if (frame.bottom > screenFrame.bottom)
			frame.OffsetBy(0, screenFrame.bottom - frame.bottom);
	} else {
		if (frame.bottom > screenFrame.bottom)
			frame.OffsetBy(0, -superItem->Frame().Height() - frame.Height() - 3);

		if (frame.right > screenFrame.right)
			frame.OffsetBy(screenFrame.right - frame.right, 0);
	}

	return frame;
}


bool
BMenu::ScrollMenu(BRect bounds, BPoint loc, bool *fast)
{
	return false;
}


void
BMenu::ScrollIntoView(BMenuItem *item)
{
}


void
BMenu::DrawItems(BRect updateRect)
{
	int32 itemCount = fItems.CountItems();
	for (int32 i = 0; i < itemCount; i++) {
		BMenuItem *item = ItemAt(i);
		if (item->Frame().Intersects(updateRect))
			item->Draw();			
	}
}


int
BMenu::State(BMenuItem **item) const
{
	return 0;
}


void
BMenu::InvokeItem(BMenuItem *item, bool now)
{
	if (!item->IsEnabled())
		return;
	
	// Do the "selected" animation
	if (!item->Submenu() && LockLooper()) {
		snooze(50000);
		item->Select(true);
		Sync();
		snooze(50000);
		item->Select(false);
		Sync();
		snooze(50000);	
		item->Select(true);
		Sync();
		snooze(50000);	
		item->Select(false);
		Sync();
		UnlockLooper();
	}
	
	item->Invoke();
}


bool
BMenu::OverSuper(BPoint location)
{
	if (!Supermenu())
		return false;

	return fSuperbounds.Contains(location);
}


bool
BMenu::OverSubmenu(BMenuItem *item, BPoint loc)
{
	// we assume that loc is in screen coords
	BMenu *subMenu = item->Submenu();
	if (subMenu == NULL || subMenu->Window() == NULL)
		return false;

	if (subMenu->Window()->Frame().Contains(loc))
		return true;

	if (subMenu->fSelected == NULL)
		return false;

	return subMenu->OverSubmenu(subMenu->fSelected, loc);
}


BMenuWindow *
BMenu::MenuWindow()
{
	if (fCachedMenuWindow == NULL) {
		char windowName[64];
		snprintf(windowName, 64, "%s cached menu", Name());
		fCachedMenuWindow = new (nothrow) BMenuWindow(windowName);
	}

	return fCachedMenuWindow;
}


void
BMenu::DeleteMenuWindow()
{
	if (fCachedMenuWindow != NULL) {
		fCachedMenuWindow->Lock();
		fCachedMenuWindow->Quit();
		fCachedMenuWindow = NULL;
	}
}


BMenuItem *
BMenu::HitTestItems(BPoint where, BPoint slop) const
{
	// TODO: Take "slop" into account ?
	
	// if the point doesn't lie within the menu's
	// bounds, bail out immediately
	if (!Bounds().Contains(where))
		return NULL;
	
	int32 itemCount = CountItems();
	for (int32 i = 0; i < itemCount; i++) {
		BMenuItem *item = ItemAt(i);
		if (item->Frame().Contains(where))
			return item;
	}

	return NULL;
}


BRect
BMenu::Superbounds() const
{
	return fSuperbounds;
}


void
BMenu::CacheFontInfo()
{
	font_height fh;
	GetFontHeight(&fh);
	fAscent = fh.ascent;
	fDescent = fh.descent;
	fFontHeight = ceilf(fh.ascent + fh.descent + fh.leading);
}


void
BMenu::ItemMarked(BMenuItem *item)
{
	if (IsRadioMode()) {
		for (int32 i = 0; i < CountItems(); i++)
			if (ItemAt(i) != item) 
				ItemAt(i)->SetMarked(false);
		InvalidateLayout();
	}
	
	if (IsLabelFromMarked() && Superitem())
		Superitem()->SetLabel(item->Label());
}


void
BMenu::Install(BWindow *target)
{
	for (int32 i = 0; i < CountItems(); i++)
		ItemAt(i)->Install(target);
}


void
BMenu::Uninstall()
{
	for (int32 i = 0; i < CountItems(); i++)
		ItemAt(i)->Uninstall();
}


void
BMenu::SelectItem(BMenuItem *menuItem, uint32 showSubmenu, bool selectFirstItem)
{
	// TODO: make use of "selectFirstItem"
		
	// Avoid deselecting and then reselecting the same item
	// which would cause flickering
	if (menuItem != fSelected) {	
		if (fSelected != NULL) {
			fSelected->Select(false);
			BMenu *subMenu = fSelected->Submenu();
			if (subMenu != NULL && subMenu->Window() != NULL)
				subMenu->_hide();
		}
		
		fSelected = menuItem;
		if (fSelected != NULL)
			fSelected->Select(true);
	}
	
	if (fSelected != NULL && showSubmenu == 0) {
		BMenu *subMenu = fSelected->Submenu();
		if (subMenu != NULL && subMenu->Window() == NULL)
			subMenu->_show();
	}
}


BMenuItem *
BMenu::CurrentSelection() const
{
	return fSelected;
}


bool 
BMenu::SelectNextItem(BMenuItem *item, bool forward)
{
	BMenuItem *nextItem = NextItem(item, forward);
	if (nextItem == NULL)
		return false;
	
	SelectItem(nextItem);
	return true;
}


BMenuItem *
BMenu::NextItem(BMenuItem *item, bool forward) const
{
	int32 index = fItems.IndexOf(item);
	if (forward)
		index++;
	else
		index--;
		
	if (index < 0 || index >= fItems.CountItems())
		return NULL;

	return ItemAt(index);	
}


bool 
BMenu::IsItemVisible(BMenuItem *item) const
{
	BRect itemFrame = item->Frame();
	ConvertToScreen(&itemFrame);
	
	BRect visibilityFrame = Window()->Frame() & BScreen(Window()).Frame();
		
	return visibilityFrame.Intersects(itemFrame);
}


void 
BMenu::SetIgnoreHidden(bool on)
{
	fIgnoreHidden = on;
}


void
BMenu::SetStickyMode(bool on)
{
	if (fStickyMode != on) {
		// TODO: Ugly hack, but it needs to be done right here in this method
		BMenuBar *menuBar = dynamic_cast<BMenuBar *>(this);
		if (on && menuBar != NULL && menuBar->LockLooper()) {
			// Steal the focus from the current focus view
			// (needed to handle keyboard navigation)
			menuBar->StealFocus();
			menuBar->UnlockLooper();
		}
		
		fStickyMode = on;
	}

	// If we are switching to sticky mode, propagate the status
	// back to the super menu
	if (on && fSuper != NULL)
		fSuper->SetStickyMode(on);
}


bool
BMenu::IsStickyMode() const
{
	return fStickyMode;
}


void
BMenu::CalcTriggers()
{
	BList triggersList;
	
	// Gathers the existing triggers
	// TODO: Oh great, reinterpret_cast.
	for (int32 i = 0; i < CountItems(); i++) {
		char trigger = ItemAt(i)->Trigger();
		if (trigger != 0)
			triggersList.AddItem(reinterpret_cast<void *>((uint32)trigger));
	}

	// Set triggers for items which don't have one yet
	for (int32 i = 0; i < CountItems(); i++) {
		BMenuItem *item = ItemAt(i);
		if (item->Trigger() == 0) {
			const char *newTrigger = ChooseTrigger(item->Label(), &triggersList);
			if (newTrigger != NULL)
				item->SetAutomaticTrigger(*newTrigger);			
		}
	}
}


const char *
BMenu::ChooseTrigger(const char *title, BList *chars)
{
	ASSERT(chars != NULL);
	
	if (title == NULL)
		return NULL;

	char trigger;
	// TODO: Oh great, reinterpret_cast all around
	while ((trigger = title[0]) != '\0') {
		if (!chars->HasItem(reinterpret_cast<void *>((uint32)trigger)))	{
			chars->AddItem(reinterpret_cast<void *>((uint32)trigger));
			return title;
		}

		title++;
	}

	return NULL;
}


void
BMenu::UpdateWindowViewSize(bool upWind)
{
	BWindow *window = Window();
	
	ASSERT(window != NULL);
	
	if (window == NULL)
		return;

	bool scroll;
	BRect frame = CalcFrame(ScreenLocation(), &scroll);
	ResizeTo(frame.Width(), frame.Height());

	if (fItems.CountItems() > 0)
		window->ResizeTo(Bounds().Width() + 2, Bounds().Height() + 2);
	else {
		CacheFontInfo();
		window->ResizeTo(StringWidth(kEmptyMenuLabel) + fPad.left + fPad.right,
						fFontHeight + fPad.top + fPad.bottom);
	}
	window->MoveTo(frame.LeftTop());
}


bool
BMenu::IsStickyPrefOn()
{
	return true;
}


void
BMenu::RedrawAfterSticky(BRect bounds)
{
}


bool
BMenu::OkToProceed(BMenuItem* item)
{
	bool proceed = true;
	BPoint where;
	ulong buttons;
	GetMouse(&where, &buttons, false);
	// Quit if user releases the mouse button (in nonsticky mode)
	// or click the mouse button (in sticky mode)
	// or moves the pointer over another item
	if ((buttons == 0 && !IsStickyMode())
		|| (buttons != 0 && IsStickyMode())
		|| HitTestItems(where) != item)
		proceed = false;
	
	return proceed;
}


void
BMenu::QuitTracking()
{
	if (IsStickyMode())
		SetStickyMode(false);
	if (fSelected != NULL)
		SelectItem(NULL);
	if (BMenuBar *menuBar = dynamic_cast<BMenuBar *>(this))
		menuBar->RestoreFocus();

	fState = MENU_STATE_CLOSED;
}


status_t 
BMenu::ParseMsg(BMessage *msg, int32 *sindex, BMessage *spec,
						 int32 *form, const char **prop, BMenu **tmenu,
						 BMenuItem **titem, int32 *user_data,
						 BMessage *reply) const
{
	return B_ERROR;
}


status_t
BMenu::DoMenuMsg(BMenuItem **next, BMenu *tar, BMessage *m,
						  BMessage *r, BMessage *spec, int32 f) const
{
	return B_ERROR;
}


status_t
BMenu::DoMenuItemMsg(BMenuItem **next, BMenu *tar, BMessage *m,
							  BMessage *r, BMessage *spec, int32 f) const
{
	return B_ERROR;
}


status_t 
BMenu::DoEnabledMsg(BMenuItem *ti, BMenu *tm, BMessage *m,
							 BMessage *r) const
{
	return B_ERROR;
}


status_t
BMenu::DoLabelMsg(BMenuItem *ti, BMenu *tm, BMessage *m,
						   BMessage *r) const
{
	return B_ERROR;
}


status_t 
BMenu::DoMarkMsg(BMenuItem *ti, BMenu *tm, BMessage *m,
						  BMessage *r) const
{
	return B_ERROR;
}


status_t
BMenu::DoDeleteMsg(BMenuItem *ti, BMenu *tm, BMessage *m,
							BMessage *r) const
{
	return B_ERROR;
}


status_t
BMenu::DoCreateMsg(BMenuItem *ti, BMenu *tm, BMessage *m,
							BMessage *r, bool menu) const
{
	return B_ERROR;
}


// TODO: Maybe the following two methods would fit better into InterfaceDefs.cpp
// In R5, they do all the work client side, we let the app_server handle the details.
status_t
set_menu_info(menu_info *info)
{
	if (!info)
		return B_BAD_VALUE;

	BPrivate::AppServerLink link;
	link.StartMessage(AS_SET_MENU_INFO);
	link.Attach<menu_info>(*info);
	
	status_t status = B_ERROR;
	if (link.FlushWithReply(status) == B_OK && status == B_OK)
		BMenu::sMenuInfo = *info;
		// Update also the local copy, in case anyone relies on it

	return status;
}


status_t
get_menu_info(menu_info *info)
{
	if (!info)
		return B_BAD_VALUE;
	
	BPrivate::AppServerLink link;
	link.StartMessage(AS_GET_MENU_INFO);
	
	status_t status = B_ERROR;
	if (link.FlushWithReply(status) == B_OK && status == B_OK)
		link.Read<menu_info>(info);
	
	return status;
}
