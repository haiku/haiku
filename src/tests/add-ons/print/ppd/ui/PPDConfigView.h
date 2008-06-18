/*
 * Copyright 2008, Haiku.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */

#ifndef _PPD_CONFIG_VIEW_H
#define _PPD_CONFIG_VIEW_H

#include "PPD.h"

#include <View.h>
#include <ListItem.h>
#include <OutlineListView.h>

class CategoryItem : public BStringItem
{
private:
	Statement* fStatement;
	
public:
	CategoryItem(const char* text, Statement* statement, uint32 level)
		: BStringItem(text, level)
		, fStatement(statement)
	{
	}
		
	Statement* GetStatement() { return fStatement; }
};

class PPDConfigView : public BView {
private:
	PPD* fPPD;
	
	BView*            fDetails;
	BMessage          fSettings;
	
	void SetupSettings(const BMessage& settings);

	void BooleanChanged(BMessage* msg);
	void StringChanged(BMessage* msg);
	
public:
	PPDConfigView(BRect rect, const char *name, uint32 resizeMask, uint32 flags);

	// The view has to be attached to a window when this
	// method is called.
	void Set(const char* ppdFile, const BMessage& settings);
	const BMessage& GetSettings();
	
	void FillCategories();
	void FillDetails(Statement* statement);
	void MessageReceived(BMessage* msg);
};
#endif
