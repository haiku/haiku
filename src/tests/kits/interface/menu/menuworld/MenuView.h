//--------------------------------------------------------------------
//	
//	MenuView.h
//
//	Written by: Owen Smith
//	
//--------------------------------------------------------------------

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _MenuView_h
#define _MenuView_h


#include <View.h>

class BButton;
class BListItem;
class BOutlineListView;
class BStringView;
class BTextControl;
//====================================================================
//	CLASS: MenuView

class MenuView : public BView
{
	//----------------------------------------------------------------
	//	Constructors, destructors, operators

public:
					MenuView(uint32 resizingMode);
				
				
	//----------------------------------------------------------------
	//	Virtual member function overrides

public:	
	void				MessageReceived(BMessage* message);
	void				AllAttached(void);
	

	//----------------------------------------------------------------
	//	Operations
public:
	void				PopulateUserMenu(BMenu* pMenu, int32 index);

	//----------------------------------------------------------------
	//	Message handlers

private:
	void				AddMenu(BMessage* message);
	void				DeleteMenu(BMessage* message);
	void				AddMenuItem(BMessage* message);
	void				MenuSelectionChanged(BMessage* message);
	
	
	//----------------------------------------------------------------
	//	Implementation member functions
	
private:
	void				BuildMenuItems(BMenu* pMenu, BListItem* superitem,
							BOutlineListView* pView);
	bool				IsSeparator(const char* text) const;
	void				SetButtonState(void);
	bool				Valid(void);
	

	//----------------------------------------------------------------
	//	Member variables
	
private:
	BTextControl*		m_pLabelCtrl;
	BCheckBox*			m_pHideUserCheck;
	BCheckBox*			m_pLargeTestIconCheck;
	BButton*			m_pAddMenuButton;
	BButton*			m_pDelButton;
	BButton*			m_pAddItemButton;
	BOutlineListView*	m_pMenuOutlineView;
	BScrollView*		m_pScrollView;	
};

#endif /* _MenuView_h */
