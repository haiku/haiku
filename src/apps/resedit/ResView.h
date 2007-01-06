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
	
	// BResources mirror API
	const void *	LoadResource(const type_code &type, const int32 &id,
								size_t *out_size);
	const void *	LoadResource(const type_code &type, const char *name,
								size_t *out_size);
	status_t		AddResource(const type_code &type, const int32 &id, 
								const void *data, const size_t &data_size, 
								const char *name = NULL);
	bool			HasResource(const type_code &type, const int32 &id);
	bool			HasResource(const type_code &type, const char *name);
	
private:
	BColumnListView	*fListView;
	entry_ref		*fRef;
	BString			fFileName;
	BMenuBar		*fBar;
	bool			fIsDirty;
	
	BResources		fResources;
};

extern ResourceRoster gResRoster;

#endif
