//--------------------------------------------------------------------
//	
//	MenuWindow.cpp
//
//	Written by: Owen Smith
//	
//--------------------------------------------------------------------

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <Application.h>
#include <Box.h>
#include <CheckBox.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <StringView.h>
#include <stdio.h>
#include <string.h>

#include "constants.h"
#include "MenuView.h"
#include "MenuWindow.h"
#include "stddlg.h"

//====================================================================
//	MenuWindow Implementation

#define MAX_TEST_STATUS_CHARS 25


//--------------------------------------------------------------------
//	MenuWindow constructors, destructors, operators

MenuWindow::MenuWindow(const char* name)
	: BWindow(BRect(60,60,60,60), name, B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	m_bUsingFullMenuBar = true;
	
	// menu bars
	BRect dummyFrame(0, 0, 0, 0);
	m_pFullMenuBar = new BMenuBar(dummyFrame, "Full Menu Bar");
	m_pHiddenMenuBar = new BMenuBar(dummyFrame, "Menu Bar w. Hidden User Menus");
	
	// File menu
	BMenu* pMenu = BuildFileMenu();
	if (pMenu) {
		m_pFullMenuBar->AddItem(pMenu);
	}
	pMenu = BuildFileMenu();
	if (pMenu) {
		m_pHiddenMenuBar->AddItem(pMenu);
	}
	
	// Test menu
	pMenu = m_testMenuBuilder.BuildTestMenu(B_MINI_ICON);
	if (pMenu) {
		m_pFullMenuBar->AddItem(pMenu);
	}
	pMenu = m_testMenuBuilder.BuildTestMenu(B_MINI_ICON);
	if (pMenu) {
		m_pHiddenMenuBar->AddItem(pMenu);
	}
	 
	// add child after menus are added so its initially
	// calculated app_server bounds take added menus into
	// account
	AddChild(m_pFullMenuBar);
	
	float menuHeight = m_pFullMenuBar->Bounds().Height();
	
	// Menu view
	m_pMenuView = new MenuView(B_FOLLOW_NONE); // don't follow window just yet!
	m_pMenuView->MoveBy(0, menuHeight + 1);
	AddChild(m_pMenuView);

	// Status view	
	BRect menuViewRect = m_pMenuView->Frame();
	float top = menuViewRect.bottom + 1;
	font_height plainHeight;
	be_plain_font->GetHeight(&plainHeight);
	
	// Simulate a vertical divider by making a BBox where only the top side
	// can be seen in the window.
	BRect boxFrame;
	boxFrame.Set(menuViewRect.left - 2, 
		top,
		menuViewRect.right + 2,
		top + plainHeight.ascent + plainHeight.descent + plainHeight.leading + 4);
	
	BBox* pStatusBox = new BBox(boxFrame);
	AddChild(pStatusBox);

	BRect statusFrame = pStatusBox->Bounds();
	statusFrame.InsetBy(2,2);	
	m_pStatusView = new BStringView(statusFrame, "Status View", STR_STATUS_DEFAULT,
		B_FOLLOW_ALL); // don't follow window just yet!
	m_pStatusView->SetViewColor(BKG_GREY);
	pStatusBox->AddChild(m_pStatusView);
	
	// Resize window dynamically to fit MenuView (and Status View)
	float windowWidth = m_pMenuView->Frame().right;
	float windowHeight = boxFrame.bottom - 4;
	ResizeTo(windowWidth, windowHeight);
}



//--------------------------------------------------------------------
//	MenuWindow virtual function overrides

void MenuWindow::MenusBeginning(void)
{
	if ((! Valid()) || (! m_bUsingFullMenuBar)) {
		return;
	}

	int32 len = m_pFullMenuBar->CountItems();
	for (int32 i = 2; i < len; i++) // skipping File and Test menus
	{
		BMenu* pMenu = m_pFullMenuBar->SubmenuAt(i);
		if (pMenu) {
			m_pMenuView->PopulateUserMenu(pMenu, i - 2);
		}
	}
}

void MenuWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
	case MSG_WIN_ADD_MENU:
		AddMenu(message);
		break;
	case MSG_WIN_DELETE_MENU:
		DeleteMenu(message);
		break;
	case MSG_TEST_ITEM:
		TestMenu(message);
		break;
	case MSG_USER_ITEM:
		UserMenu(message);
		break;
	case MSG_WIN_HIDE_USER_MENUS:
		ToggleUserMenus(message);
		break;
	case MSG_WIN_LARGE_TEST_ICONS:
		ToggleTestIcons(message);
		break;
	default:
		BWindow::MessageReceived(message);
		break;
	}
}

bool MenuWindow::QuitRequested(void)
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}



//--------------------------------------------------------------------
//	MenuWindow operations

void MenuWindow::UpdateStatus(const char* str1, const char* str2)
{
	uint32 lenTotal = 0, len1 = 0, len2 = 0;
	
	if (str1)
		len1 = strlen(str1);
	if (str2)
		len2 = strlen(str2);
	
	lenTotal = len1 + len2;	
	char* updateText = new char[lenTotal+1];
	updateText[0] = '\0'; // in case str1 and str2 are both NULL

	if (str1)
		strcpy(updateText, str1);
	if (str2)
		strcpy(updateText + len1, str2);
	
	if (Lock() && Valid()) {
		m_pStatusView->SetText(updateText);
		Unlock();
	}
	
	delete [] updateText;
}



//--------------------------------------------------------------------
//	MenuWindow message handlers

void MenuWindow::AddMenu(BMessage* message)
{
	if (! Valid()) {
		return;
	}
	
	const char* menuName;
	if (message->FindString("Menu Name", &menuName) == B_OK) {
		m_pFullMenuBar->AddItem(new BMenu(menuName));
		UpdateStatus(STR_STATUS_ADD_MENU, menuName);
	}
}

void MenuWindow::DeleteMenu(BMessage* message)
{
	if (! Valid()) {
		return;
	}		
	
	int32 i; 
	if (message->FindInt32("Menu Index", &i) == B_OK) {
		BMenuItem* pItem = m_pFullMenuBar->ItemAt(i+2);
		if (pItem) {
			// menu index is the above index + 2 (for File and Test menus)
			m_pFullMenuBar->RemoveItem(pItem);
			UpdateStatus(STR_STATUS_DELETE_MENU, pItem->Label());
			delete pItem;
		}
	}
}

void MenuWindow::TestMenu(BMessage* message)
{
	if (! Valid()) {
		return;
	}
	
	int32 i;
	if (message->FindInt32("Item Index", &i) == B_OK) {
		char numText[3];
		sprintf(numText, "%ld", i);
		UpdateStatus(STR_STATUS_TEST, numText);
	}
}

void MenuWindow::UserMenu(BMessage* message)
{
	if (! Valid()) {
		return;
	}
	
	const char* itemName;
	if (message->FindString("Item Name", &itemName) == B_OK) {
		UpdateStatus(STR_STATUS_USER, itemName);
	}
}

void MenuWindow::ToggleUserMenus(BMessage* message)
{
	if (! Valid()) {
		return;
	}
	
	void* pSrc;
	bool useFullMenus = false;
	
	if (message->FindPointer("source", &pSrc) == B_OK) {
		BCheckBox* pCheckBox = reinterpret_cast<BCheckBox*>(pSrc);
		useFullMenus = (pCheckBox->Value() == B_CONTROL_OFF);
	}
	
	if ((! useFullMenus) && m_bUsingFullMenuBar) {
		RemoveChild(m_pFullMenuBar);
		AddChild(m_pHiddenMenuBar);
		m_bUsingFullMenuBar = false;
	} else if (useFullMenus && (! m_bUsingFullMenuBar)) {
		RemoveChild(m_pHiddenMenuBar);
		AddChild(m_pFullMenuBar);
		m_bUsingFullMenuBar = true;
	}
}

void MenuWindow::ToggleTestIcons(BMessage* message)
{
	if (! Valid()) {
		return;
	}
	
	void* pSrc;
	icon_size size = B_MINI_ICON;
	
	if (message->FindPointer("source", &pSrc) == B_OK) {
		BCheckBox* pCheckBox = reinterpret_cast<BCheckBox*>(pSrc);
		size = (pCheckBox->Value() == B_CONTROL_ON) ? B_LARGE_ICON : B_MINI_ICON;
	}
	
	ReplaceTestMenu(m_pFullMenuBar, size);
	ReplaceTestMenu(m_pHiddenMenuBar, size);	
}



//--------------------------------------------------------------------
//	MenuWindow implementation member functions

bool MenuWindow::Valid(void) const
{
	if (! m_pFullMenuBar) {
		ierror(STR_NO_FULL_MENU_BAR);
		return false;
	}
	if (! m_pHiddenMenuBar) {
		ierror(STR_NO_HIDDEN_MENU_BAR);
		return false;
	}
	if (! m_pMenuView) {
		ierror(STR_NO_MENU_VIEW);
		return false;
	}
	if (! m_pStatusView) {
		ierror(STR_NO_STATUS_VIEW);
		return false;
	}
	return true;		
}

BMenu* MenuWindow::BuildFileMenu(void) const
{
	BMenu* pMenu = new BMenu(STR_MNU_FILE);
	
	BMenuItem* pAboutItem = new BMenuItem(STR_MNU_FILE_ABOUT,
		new BMessage(B_ABOUT_REQUESTED));
	pAboutItem->SetTarget(NULL, be_app);
	pMenu->AddItem(pAboutItem);
	
	pMenu->AddSeparatorItem();
	
	pMenu->AddItem(new BMenuItem(STR_MNU_FILE_CLOSE,
		new BMessage(B_QUIT_REQUESTED), CMD_FILE_CLOSE));
		
	return pMenu;
}

void MenuWindow::ReplaceTestMenu(BMenuBar* pMenuBar, icon_size size)
{
	if (! pMenuBar) {
		return;
	}
	
	BMenu* pTestMenu = m_testMenuBuilder.BuildTestMenu(size);
	if (pTestMenu) {
		BMenuItem* pPrevItem = pMenuBar->RemoveItem(1);
		if (pPrevItem) {
			delete pPrevItem;
		}
		pMenuBar->AddItem(pTestMenu, 1);
	}
}
