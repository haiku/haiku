/*
 * Copyright 2004-2011 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Philippe Houdoin
 *		Fredrik Modéen
 *		Alexander von Gluck, kallisti5@unixzen.com
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
#include <MenuItem.h>
#include <NetworkDevice.h>
#include <NetworkInterface.h>
#include <PopUpMenu.h>
#include <String.h>

#include "NetworkSettings.h"


#define ICON_SIZE 37


class InterfaceListItem : public BListItem {
public:
								InterfaceListItem(const char* name);
								~InterfaceListItem();

			void				DrawItem(BView* owner,
									BRect bounds, bool complete);
			void				Update(BView* owner, const BFont* font);

	inline	const char*			Name() {return fInterface.Name();}
	inline	bool				IsDisabled() {
										return fSettings->IsDisabled();}
	inline	void				SetDisabled(bool disable) {
										fSettings->SetDisabled(disable);}
	inline	NetworkSettings*	GetSettings() {return fSettings;}

private:
			void 				_Init();
			void				_PopulateBitmaps(const char* mediaType);

			BBitmap* 			fIcon;
			BBitmap*			fIconOffline;
			BBitmap*			fIconPending;
			BBitmap*			fIconOnline;

			NetworkSettings*	fSettings;
				// Interface configuration

			BNetworkInterface	fInterface;
				// Hardware Interface

			float				fFirstlineOffset;
			float				fSecondlineOffset;
			float				fThirdlineOffset;
			float				fStateWidth;
};


class InterfacesListView : public BListView {
public:
								InterfacesListView(BRect rect, const char* name,
									uint32 resizingMode
									= B_FOLLOW_LEFT | B_FOLLOW_TOP);

	virtual						~InterfacesListView();

			InterfaceListItem*	FindItem(const char* name);
			status_t			SaveItems();

protected:
	virtual	void				AttachedToWindow();
	virtual	void				DetachedFromWindow();
	virtual	void				FrameResized(float width, float height);

	virtual	void				MessageReceived(BMessage* message);

private:
			status_t			_InitList();
			status_t			_UpdateList();
			void				_HandleNetworkMessage(BMessage* message);
};

#endif /*INTERFACES_LIST_VIEW_H*/
