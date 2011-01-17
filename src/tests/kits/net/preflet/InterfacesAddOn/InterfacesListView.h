/*
 * Copyright 2004-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Philippe Houdoin
 * 		Fredrik Modéen
 */


#ifndef INTERFACES_LIST_VIEW_H
#define INTERFACES_LIST_VIEW_H

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_types.h>

#include <Bitmap.h>
#include <ListView.h>
#include <ListItem.h>
#include <NetworkDevice.h>
#include <NetworkInterface.h>
#include <String.h>

#include "Settings.h"

class InterfaceListItem : public BListItem {
public:
	InterfaceListItem(const char* name);
	~InterfaceListItem();

	void DrawItem(BView* owner, BRect bounds, bool complete);
	void Update(BView* owner, const BFont* font);

	inline const char*		Name()			{ return fInterface.Name(); }
	inline bool				IsDisabled()	{ return fSettings->IsDisabled(); }
	inline void				SetDisabled(bool disable){ fSettings->SetDisabled(disable); }
	inline Settings*		GetSettings()	{ return fSettings; }

private:
	void 					_Init();

	BBitmap* 				fIcon;
	BNetworkInterface		fInterface;
	Settings*				fSettings;
};


class InterfacesListView : public BListView {
public:
	InterfacesListView(BRect rect, const char* name,
		uint32 resizingMode = B_FOLLOW_LEFT | B_FOLLOW_TOP);
	virtual ~InterfacesListView();

	InterfaceListItem* FindItem(const char* name);

protected:
	virtual void AttachedToWindow();
	virtual void DetachedFromWindow();

	virtual void MessageReceived(BMessage* message);

private:
	status_t	_InitList();
	status_t	_UpdateList();
	void		_HandleNetworkMessage(BMessage* message);
};

#endif /*INTERFACES_LIST_VIEW_H*/
