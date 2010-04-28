/*
 * Copyright (c) 2005-2010, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		DarkWyrm <darkwyrm@gmail.com>
 */
#ifndef RESVIEW_H
#define RESVIEW_H

#include <Entry.h>
#include <FilePanel.h>
#include <List.h>
#include <ListItem.h>
#include <MenuBar.h>
#include <Resources.h>
#include <String.h>
#include <View.h>

#include "ResourceRoster.h"

class BMenuBar;
class ResListView;

enum {
	FILE_CLEAN = 0,
	FILE_DIRTY,
	FILE_INIT,
	FILE_QUIT_AFTER_SAVE
};

class ResView : public BView
{
public:
					ResView(const BRect &frame, const char *name,
							const int32 &resize, const int32 &flags,
							const entry_ref *ref = NULL);
					~ResView(void);
	void			AttachedToWindow(void);
	void			MessageReceived(BMessage *msg);
	
	const char *	Filename(void) const { return fFileName.String(); }
	uint8			GetSaveStatus(void) const { return fSaveStatus; }
	
	status_t		SetTo(const entry_ref &dir, const BString &name);
	void			OpenFile(const entry_ref &ref);
	void			SaveFile(void);
	void			SaveAndQuit(void);
	
private:
	void			BuildMenus(BMenuBar *menuBar);
	void			EmptyDataList(void);
	void			UpdateRow(BRow *row);
	void			HandleDrop(BMessage *msg);
	void			AddResource(const entry_ref &ref);
	void			DeleteSelectedResources(void);
	void			SetSaveStatus(uint8 value);
	
	ResListView		*fListView;
	entry_ref		*fRef;
	BString			fFileName;
	BMenuBar		*fBar;
	uint8			fSaveStatus;
	BList			fDataList;
	BFilePanel		*fOpenPanel,
					*fSavePanel;
};


class ResDataRow : public BRow
{
public:
					ResDataRow(ResourceData *data);
	ResourceData *	GetData(void) const;
	
private:
	ResourceData	*fResData;
};

extern ResourceRoster gResRoster;

#endif
