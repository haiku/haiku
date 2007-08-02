/*
 * Copyright 2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 *	Authors:
 *		Stefano Ceccherini (burton666@libero.it)
 */

#include "SmartTabView.h"

#include <TabView.h>


SmartTabView::SmartTabView(BRect frame, const char *name, button_width width, 
				uint32 resizingMode, uint32 flags)
	:
	BView(frame, name, resizingMode, flags),
	fTabView(NULL),
	fView(NULL)
{
}


SmartTabView::~SmartTabView()
{
}


BView *
SmartTabView::ContainerView()
{
	return fTabView != NULL ? fTabView->ContainerView() : this;
}


BView *
SmartTabView::ViewForTab(int32 tab)
{
	return fTabView != NULL ? fTabView->ViewForTab(tab) : fView; 
}


void
SmartTabView::Select(int32 index)
{
	if (fView != NULL) {
		fView->ResizeTo(Bounds().Width(), Bounds().Height());	
	} else if (fTabView != NULL) {
		fTabView->Select(index);
		BTab *tab = fTabView->TabAt(fTabView->Selection());
		if (tab != NULL) {	
			BView *view = tab->View();
			if (view != NULL)
				view->ResizeTo(fTabView->Bounds().Width(), fTabView->Bounds().Height());
		}
	}
}


int32
SmartTabView::Selection() const
{
	return fTabView != NULL ? fTabView->Selection() : 0;
}


void
SmartTabView::AddTab(BView *target, BTab *tab)
{
	if (target == NULL)
		return;

	if (fView == NULL && fTabView == NULL) {
		fView = target;
		AddChild(target);	
	} else {
		if (fTabView == NULL) {
			RemoveChild(fView);
			fTabView = new BTabView(Bounds(), "tabview");			
			AddChild(fTabView);
			fTabView->MoveTo(B_ORIGIN);
			fTabView->ResizeTo(Bounds().Width(), Bounds().Height());
			fTabView->AddTab(fView);
			fTabView->Select(0);
			fView = NULL;
		}					
		fTabView->AddTab(target, tab);
	}
}


BTab *
SmartTabView::RemoveTab(int32 index)
{
	BTab *returnTab = NULL;
	if (fTabView != NULL) {
		returnTab = fTabView->RemoveTab(index);
		if (fTabView->CountTabs() == 1) {
			BTab *tab = fTabView->RemoveTab(0);			
			RemoveChild(fTabView);
			delete fTabView;			
			fTabView = NULL;
			fView = tab->View();
			tab->SetView(NULL);			
			AddChild(fView);
			fView->MoveTo(B_ORIGIN);
			fView->ResizeTo(Bounds().Width(), Bounds().Height());
			delete tab;
		}
	} else if (fView != NULL) {		
		RemoveChild(fView);
		returnTab = new BTab();
		returnTab->SetView(fView);		
		fView = NULL;
	}
	
	return returnTab;
}


int32
SmartTabView::CountTabs() const
{
	if (fTabView != NULL)
		return fTabView->CountTabs();

	return fView != NULL ? 1 : 0;	
}


/*
void
SmartTabView::Draw(BRect updateRect)
{
	if (fView != NULL) {
		
	}	
}
*/


