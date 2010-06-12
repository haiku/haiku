//--------------------------------------------------------------------
//	
//	MenuWindow.h
//
//	Written by: Owen Smith
//	
//--------------------------------------------------------------------

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _MenuWindow_h
#define _MenuWindow_h

#include <Message.h>

#include <Window.h>
#include "TestMenuBuilder.h"

class MenuView;
class BButton;
class BListItem;
class BOutlineListView;
class BStringView;
class BTextControl;

//====================================================================
//	CLASS: MenuWindow

class MenuWindow : public BWindow
{
	//----------------------------------------------------------------
	//	Constructors, destructors, operators

public:
					MenuWindow(const char* name);
				
				
	//----------------------------------------------------------------
	//	Virtual member function overrides

public:	
	void			MenusBeginning(void);
	void			MessageReceived(BMessage* message);
	bool			QuitRequested(void);

	
	//----------------------------------------------------------------
	//	Operations

public:
	void			UpdateStatus(const char* str1 = NULL,
						const char* str2 = NULL);
	
	
	//----------------------------------------------------------------
	//	Message handlers

private:
	void			AddMenu(BMessage* message);
	void			DeleteMenu(BMessage* message);
	void			TestMenu(BMessage* message);
	void			UserMenu(BMessage* message);
	void			ToggleUserMenus(BMessage* message);
	void			ToggleTestIcons(BMessage* message);
	
	
	//----------------------------------------------------------------
	//	Implementation member functions
	
private:
	bool			Valid(void) const;
	BMenu*			BuildFileMenu(void) const;
	void			ReplaceTestMenu(BMenuBar* pMenuBar, icon_size size);
	
	//----------------------------------------------------------------
	//	Member variables
	
private:
	BMenuBar* 			m_pFullMenuBar;
	BMenuBar*			m_pHiddenMenuBar;
	bool				m_bUsingFullMenuBar;
	BStringView*		m_pStatusView;
	MenuView*			m_pMenuView;	
	TestMenuBuilder		m_testMenuBuilder;
};

#endif /* _MenuWindow_h */
