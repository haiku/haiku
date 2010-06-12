//--------------------------------------------------------------------
//	
//	MenuView.cpp
//
//	Written by: Owen Smith
//	
//--------------------------------------------------------------------

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <Alert.h>
#include <Button.h>
#include <CheckBox.h>
#include <Menu.h>
#include <MenuItem.h>
#include <OutlineListView.h>
#include <ScrollView.h>
#include <TextControl.h>
#include <List.h>
#include <string.h>

#include "constants.h"
#include "MenuView.h"
#include "MenuWindow.h"
#include "PostDispatchInvoker.h"
#include "stddlg.h"
#include "ViewLayoutFactory.h"


//====================================================================
//	MenuView Implementation


//--------------------------------------------------------------------
//	MenuView constructors, destructors, operators

MenuView::MenuView(uint32 resizingMode)
	: BView(BRect(0, 0, 0, 0), "Menu View", resizingMode,
		B_WILL_DRAW)
{
	ViewLayoutFactory aFactory; // for semi-intelligent layout
	
	SetViewColor(BKG_GREY);

	// "hide user menus" check box
	float fCheck_x = 20.0f;
	float fCheck_y = 100.0f;
	m_pHideUserCheck = aFactory.MakeCheckBox("Hide User Menus",
		STR_HIDE_USER_MENUS, MSG_WIN_HIDE_USER_MENUS,
		BPoint(fCheck_x, fCheck_y));
	m_pHideUserCheck->SetValue(B_CONTROL_OFF);
	AddChild(m_pHideUserCheck);
	
	// "large test icons" check box
	fCheck_y = m_pHideUserCheck->Frame().bottom + 10;
	m_pLargeTestIconCheck = aFactory.MakeCheckBox("Large Test Icons",
		STR_LARGE_TEST_ICONS, MSG_WIN_LARGE_TEST_ICONS,
		BPoint(fCheck_x, fCheck_y));
	m_pLargeTestIconCheck->SetValue(B_CONTROL_OFF);
	AddChild(m_pLargeTestIconCheck);
	
	// "add menu", "add item", "delete" buttons
	BList buttons;
	float fButton_x = m_pHideUserCheck->Frame().right + 15;
	float fButton_y = m_pHideUserCheck->Frame().top;
	
	m_pAddMenuButton = aFactory.MakeButton("Add Menu Bar", STR_ADD_MENU,
		MSG_VIEW_ADD_MENU, BPoint(fButton_x, fButton_y));
	AddChild(m_pAddMenuButton);
	buttons.AddItem(m_pAddMenuButton);
	
	// for purposes of size calculation, use the longest piece
	// of text we're going to stuff into the button
	const char* addItemText;
	float itemLen, sepLen;
	itemLen = be_plain_font->StringWidth(STR_ADD_ITEM);
	sepLen = be_plain_font->StringWidth(STR_ADD_SEP);
	addItemText = (itemLen > sepLen) ? STR_ADD_ITEM : STR_ADD_SEP;
	m_pAddItemButton = aFactory.MakeButton("Add Item To Menu",
		addItemText, MSG_VIEW_ADD_ITEM,
		BPoint(fButton_x, fButton_y));
	m_pAddItemButton->SetEnabled(false);
	AddChild(m_pAddItemButton);
	buttons.AddItem(m_pAddItemButton);
	
	m_pDelButton = aFactory.MakeButton("Delete Menu Bar", STR_DELETE_MENU,
		MSG_VIEW_DELETE_MENU, BPoint(fButton_x, fButton_y));
	m_pDelButton->SetEnabled(false);
	AddChild(m_pDelButton);
	buttons.AddItem(m_pDelButton);
	
	// now resize list of buttons to max width
	aFactory.ResizeToListMax(buttons, RECT_WIDTH);
	
	// now align buttons along left side
	aFactory.Align(buttons, ALIGN_LEFT,
		m_pAddItemButton->Frame().Height() + 15);
		
	// now make add menu the default, AFTER we've
	// laid out the buttons, since the button bloats
	// when it's the default
	m_pAddMenuButton->MakeDefault(true);

	// item "label" control
	float fEdit_left = 20.0f, fEdit_bottom = m_pHideUserCheck->Frame().top - 20;
	float fEdit_right = m_pAddItemButton->Frame().right;
	
	m_pLabelCtrl = aFactory.MakeTextControl("Menu Bar Control",
		STR_LABEL_CTRL, NULL, BPoint(fEdit_left, fEdit_bottom),
		fEdit_right - fEdit_left, CORNER_BOTTOMLEFT);
	AddChild(m_pLabelCtrl);
	
	// menu outline view
	BRect r;
	r.left = m_pAddItemButton->Frame().right + 30;
	r.top = 20.0f;
	r.right = r.left + 200 - B_V_SCROLL_BAR_WIDTH;
	// API quirk here: <= 12 (hscroll height - 2) height
	// results in scrollbar drawing error
	r.bottom = r.top + 100 - B_H_SCROLL_BAR_HEIGHT;

	m_pMenuOutlineView = new BOutlineListView(r, "Menu Outline",
		B_SINGLE_SELECTION_LIST, B_FOLLOW_ALL);
	m_pMenuOutlineView->SetSelectionMessage(
		new BMessage(MSG_MENU_OUTLINE_SEL));
	
	// wrap outline view in scroller
	m_pScrollView = new BScrollView("Menu Outline Scroller",
		m_pMenuOutlineView, B_FOLLOW_LEFT | B_FOLLOW_TOP, 0,
		true, true);
	m_pScrollView->SetViewColor(BKG_GREY);
	AddChild(m_pScrollView);
	
}



//--------------------------------------------------------------------
//	MenuView virtual function overrides

void MenuView::MessageReceived(BMessage* message)
{
	switch (message->what) {
	case MSG_VIEW_ADD_MENU:
		AddMenu(message);
		break;
	case MSG_VIEW_DELETE_MENU:
		DeleteMenu(message);
		break;
	case MSG_VIEW_ADD_ITEM:
		AddMenuItem(message);
		break;
	case MSG_MENU_OUTLINE_SEL:
		MenuSelectionChanged(message);
		break;
	case MSG_LABEL_EDIT:
		SetButtonState();
		break;
	default:
		BView::MessageReceived(message);
		break;
	}
}

void MenuView::AllAttached(void)
{
	if (! Valid()) {
		return;
	}

	// Set button and view targets (now that window looper is available).
	m_pAddMenuButton->SetTarget(this);
	m_pDelButton->SetTarget(this);
	m_pAddItemButton->SetTarget(this);
	m_pMenuOutlineView->SetTarget(this);
	
	// Set button's initial state
	SetButtonState();
	
	// Wrap a message filter/invoker around the label control's text field
	// that will post us B_LABEL_EDIT msg after the text field processes a
	// B_KEY_DOWN message.
	m_pLabelCtrl->TextView()->AddFilter(
		new PostDispatchInvoker(B_KEY_DOWN, new BMessage(MSG_LABEL_EDIT), this));
	
	// Get one item's height by adding a dummy item to
	// the BOutlineListView and calculating its height.
	m_pMenuOutlineView->AddItem(new BStringItem("Dummy"));
	float itemHeight = m_pMenuOutlineView->ItemFrame(0).Height();
	itemHeight++;	// account for 1-pixel offset between items
					// which eliminates overlap
	delete m_pMenuOutlineView->RemoveItem((int32)0);
	
	// Resize outline list view to integral item height.
	// fudge factor of 4 comes from 2-pixel space between
	// scrollview and outline list view on top & bottom
	float viewHeight = 16*itemHeight;
	m_pScrollView->ResizeTo(m_pScrollView->Frame().Width(),
		viewHeight + B_H_SCROLL_BAR_HEIGHT + 4);
	BScrollBar *pBar = m_pScrollView->ScrollBar(B_HORIZONTAL);
	if (pBar) {
		pBar->SetRange(0, 300);
	}
	
	// Resize view to surround contents.
	ViewLayoutFactory aFactory;
	aFactory.ResizeAroundChildren(*this, BPoint(20,20));	
}



//--------------------------------------------------------------------
//	MenuView operations

void MenuView::PopulateUserMenu(BMenu* pMenu, int32 index)
{
	if ((! pMenu) || (! Valid())) {
		return;
	}
		
	// blow away all menu items
	BMenuItem* pMenuItem = pMenu->RemoveItem((int32)0);
	while(pMenuItem) {
		delete pMenuItem;
		pMenuItem = pMenu->RemoveItem((int32)0);
	}
	
	// build menu up from outline list view	
	BListItem* pListItem = m_pMenuOutlineView->ItemUnderAt(NULL, true, index);
	
	// recursive buildup of menu items
	BuildMenuItems(pMenu, pListItem, m_pMenuOutlineView);
}



//--------------------------------------------------------------------
//	MenuView message handlers

void MenuView::AddMenu(BMessage* message)
{
	if (! Valid()) {
		return;
	}
	
	const char* menuName = m_pLabelCtrl->Text();
	if ((! menuName) || (! *menuName)) {
		BAlert* pAlert = new BAlert("Add menu alert",
			"Please specify the menu name first.", "OK");
		pAlert->Go();
		return;
	}
	
	m_pMenuOutlineView->AddItem(new BStringItem(menuName));	
	
	// add some info and pass to window
	BMessage newMsg(MSG_WIN_ADD_MENU);
	newMsg.AddString("Menu Name", menuName);
	BWindow* pWin = Window();
	if (pWin) {
		pWin->PostMessage(&newMsg);
	}
	
	// Reset the label control and buttons
	m_pLabelCtrl->SetText("");
	SetButtonState();
}

void MenuView::DeleteMenu(BMessage* message)
{
	if (! Valid())
		return;

	int32 itemCount;	
	int32 selected = m_pMenuOutlineView->CurrentSelection();
	if (selected < 0)
		return;
	
	BStringItem* pSelItem = dynamic_cast<BStringItem*>
		(m_pMenuOutlineView->ItemAt(selected));
	if (! pSelItem)
		return;
	
	if (pSelItem->OutlineLevel() == 0) {
		// get index of item among items in 0 level
		itemCount = m_pMenuOutlineView->CountItemsUnder(NULL, true);
		int32 i;
		for (i=0; i<itemCount; i++) {
			BListItem* pItem = m_pMenuOutlineView->ItemUnderAt(NULL, true, i);
			if (pItem == pSelItem) {
				break;
			}
		}
		
		// Stuff the index into the message and pass along to
		// the window.
		BMessage newMsg(MSG_WIN_DELETE_MENU);
		newMsg.AddInt32("Menu Index", i);
		BWindow* pWin = Window();
		if (pWin) {
			pWin->PostMessage(&newMsg);
		}
	}			
				
	// Find & record all subitems of selection, because we'll
	// have to delete them after they're removed from the list.
	BList subItems;
	int32 j;
	itemCount = m_pMenuOutlineView->CountItemsUnder(pSelItem, false);
	for (j=0; j<itemCount; j++) {
		BListItem* pItem = m_pMenuOutlineView->ItemUnderAt(pSelItem, false, j);
		subItems.AddItem(pItem);
	}
	
	// Note the superitem for reference just below
	BStringItem* pSuperitem = dynamic_cast<BStringItem*>
		(m_pMenuOutlineView->Superitem(pSelItem));
		
	// Remove selected item and all subitems from
	// the outline list view.
	m_pMenuOutlineView->RemoveItem(pSelItem); // removes super- and subitems
	
	// Update window status
	MenuWindow* pWin = dynamic_cast<MenuWindow*>(Window());
	if (pWin) {
		const char* itemName = pSelItem->Text();
		if (strcmp(itemName, STR_SEPARATOR)) {
			pWin->UpdateStatus(STR_STATUS_DELETE_ITEM, itemName);
		} else {
			pWin->UpdateStatus(STR_STATUS_DELETE_SEPARATOR);
		}
	}

	// Delete the selected item and all subitems	
	for (j=0; j<itemCount; j++) {
		BListItem* pItem = reinterpret_cast<BListItem*>(subItems.ItemAt(j));
		delete pItem; // deletes subitems
	}
	delete pSelItem; // deletes superitem
	
	if (pSuperitem) {
		// There's a bug in outline list view which does not
		// update the superitem correctly. The only way to do so
		// is to remove and add a brand-new superitem afresh.
		if (! m_pMenuOutlineView->CountItemsUnder(pSuperitem, true))
		{
			int32 index = m_pMenuOutlineView->FullListIndexOf(pSuperitem);
			m_pMenuOutlineView->RemoveItem(pSuperitem);
			BStringItem* pCloneItem = new BStringItem(
				pSuperitem->Text(), pSuperitem->OutlineLevel());
			m_pMenuOutlineView->AddItem(pCloneItem, index);
			delete pSuperitem;
		}
	}
}

void MenuView::AddMenuItem(BMessage* message)
{
	if (! Valid()) {
		return;
	}		
	
	// Add item to outline list view but DON'T update the
	// window right away; the actual menu items will be
	// created dynamically later on.
	int32 selected = m_pMenuOutlineView->CurrentSelection();
	if (selected >= 0) {
		BListItem* pSelItem = m_pMenuOutlineView->ItemAt(selected);
		if (pSelItem) {		

			int32 level = pSelItem->OutlineLevel() + 1;
			int32 index = m_pMenuOutlineView->FullListIndexOf(pSelItem)
				+ m_pMenuOutlineView->CountItemsUnder(pSelItem, false) + 1;
			const char* itemName = m_pLabelCtrl->Text();
			bool bIsSeparator = IsSeparator(itemName);

			if (bIsSeparator) {
				m_pMenuOutlineView->AddItem(new BStringItem(STR_SEPARATOR, level),
					index);
			} else {
				m_pMenuOutlineView->AddItem(new BStringItem(itemName, level),
					index);
			}

			MenuWindow* pWin = dynamic_cast<MenuWindow*>(Window());
			if (pWin) {
				if (! bIsSeparator) {
					pWin->UpdateStatus(STR_STATUS_ADD_ITEM, itemName);
				} else {
					pWin->UpdateStatus(STR_STATUS_ADD_SEPARATOR);
				}
			}

			m_pMenuOutlineView->Invalidate();	// outline view doesn't
												// invalidate correctly
			
			// Reset the label control the buttons
			m_pLabelCtrl->SetText("");
			SetButtonState();
		}
	}	
}

void MenuView::MenuSelectionChanged(BMessage* message)
{
	SetButtonState();
}



//--------------------------------------------------------------------
//	MenuView implementation member functions

void MenuView::BuildMenuItems(BMenu* pMenu, BListItem* pSuperitem,
	BOutlineListView* pView)
{
	if ((! pMenu) || (! pSuperitem) || (! pView)) {
		return;
	}
	
	int32 len = pView->CountItemsUnder(pSuperitem, true);
	if (len == 0) {
		BMenuItem* pEmptyItem = new BMenuItem(STR_MNU_EMPTY_ITEM, NULL);
		pEmptyItem->SetEnabled(false);
		pMenu->AddItem(pEmptyItem);
	}
	
	for (int32 i=0; i<len; i++) {
		BStringItem* pItem = dynamic_cast<BStringItem*>
			(pView->ItemUnderAt(pSuperitem, true, i));
		if (pItem) {
			if (pView->CountItemsUnder(pItem, true) > 0) {
				// add a submenu & fill it
				BMenu* pNewMenu = new BMenu(pItem->Text());
				BuildMenuItems(pNewMenu, pItem, pView);
				pMenu->AddItem(pNewMenu);
			} else {
				if (strcmp(pItem->Text(), STR_SEPARATOR)) {
					// add a string item
					BMessage* pMsg = new BMessage(MSG_USER_ITEM);
					pMsg->AddString("Item Name", pItem->Text());
					pMenu->AddItem(new BMenuItem(pItem->Text(), pMsg));
				} else {
					// add a separator item
					pMenu->AddItem(new BSeparatorItem());
				}
			}
		}
	}
}

bool MenuView::IsSeparator(const char* text) const
{
	if (! text) {
		return true;
	}
	
	if (! *text) {
		return true;
	}
	
	int32 len = strlen(text);
	for (int32 i = 0; i < len; i++) {
		char ch = text[i];
		if ((ch != ' ') && (ch != '-')) {
			return false;
		}
	}
	
	return true;
}

void MenuView::SetButtonState(void)
{
	if (! Valid()) {
		return;
	}

	// See if an item in the outline list is selected
	int32 index = m_pMenuOutlineView->CurrentSelection();
	bool bIsSelected = (index >= 0);
	
	// If an item is selected, see if the selected
	// item is a separator
	bool bSeparatorSelected = false;
	if (bIsSelected) {
		BStringItem* pItem = dynamic_cast<BStringItem*>
			(m_pMenuOutlineView->ItemAt(index));
		if (pItem) {
			bSeparatorSelected = (! strcmp(pItem->Text(), STR_SEPARATOR));
		}
	}

	// Delete: enable if anything is selected
	m_pDelButton->SetEnabled(bIsSelected);
	
	// Add Item: enable if anything but a separator is selected	
	bool bEnableAddItem = bIsSelected && (! bSeparatorSelected);
	m_pAddItemButton->SetEnabled(bEnableAddItem);

	// Add Menu: enable if there's any text in the label field
	const char* labelText = m_pLabelCtrl->Text();
	m_pAddMenuButton->SetEnabled(labelText && (*labelText));
		
	// Add Item: text says Add Separator if button is enabled
	// and the label field contains separator text,
	// Add Item otherwise
	const char* itemText;
	if (bEnableAddItem && IsSeparator(labelText)) {
		itemText = STR_ADD_SEP;
	} else {
		itemText = STR_ADD_ITEM;
	}
	m_pAddItemButton->SetLabel(itemText);

	// If add item button is enabled, it should be the default	
	if (bEnableAddItem) {
		m_pAddItemButton->MakeDefault(true);
	} else {
		m_pAddMenuButton->MakeDefault(true);
	}
}

bool MenuView::Valid(void)
{
	if (! m_pLabelCtrl) {
		ierror(STR_NO_LABEL_CTRL);
		return false;
	}
	if (! m_pHideUserCheck) {
		ierror(STR_NO_HIDE_USER_CHECK);
		return false;
	}
	if (! m_pLargeTestIconCheck) {
		ierror(STR_NO_LARGE_ICON_CHECK);
		return false;
	}
	if (! m_pAddMenuButton) {
		ierror(STR_NO_ADDMENU_BUTTON);
		return false;
	}
	if (! m_pAddItemButton) {
		ierror(STR_NO_ADDITEM_BUTTON);
		return false;
	}
	if (! m_pDelButton) {
		ierror(STR_NO_DELETE_BUTTON);
		return false;
	}
	if (! m_pMenuOutlineView) {
		ierror(STR_NO_MENU_OUTLINE);
		return false;
	}
	if (! m_pScrollView) {
		ierror(STR_NO_MENU_SCROLL_VIEW);
		return false;
	}
	return true;		
}

