/*
 * Copyright (c) 2005-2006, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		DarkWyrm <darkwyrm@earthlink.net>
 */
#ifndef RESVIEW_H
#define RESVIEW_H

#include <View.h>
#include <ColumnListView.h>
#include <Entry.h>
#include <String.h>
#include <ListItem.h>
#include <List.h>
#include <Resources.h>
#include "ResourceRoster.h"

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
	bool			IsDirty(void) const { return fIsDirty; }
	
	void			OpenFile(const entry_ref &ref);
	
private:
	void			EmptyDataList(void);
	void			UpdateRow(BRow *row);
	
	BColumnListView	*fListView;
	entry_ref		*fRef;
	BString			fFileName;
	BMenuBar		*fBar;
	bool			fIsDirty;
	BList			fDataList;
};

extern ResourceRoster gResRoster;

#endif
