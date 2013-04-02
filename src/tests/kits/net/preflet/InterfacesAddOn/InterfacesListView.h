/*
 * Copyright 2004-2013 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck, kallisti5@unixzen.com
 *		Philippe Houdoin
 *		Fredrik Modéen
 *		John Scipione, jscipione@gmail.com
 */
#ifndef INTERFACES_LIST_VIEW_H
#define INTERFACES_LIST_VIEW_H


#include <ListView.h>
#include <ListItem.h>

#include "NetworkSettings.h"


class BBitmap;
class BMenuItem;
class BNetworkInterface;
class BPoint;
class BPopUpMenu;
class BSeparatorItem;
class BString;

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
								InterfacesListView(const char* name);
	virtual						~InterfacesListView();

			InterfaceListItem*	FindItem(const char* name);
			InterfaceListItem*	FindItem(BPoint where);
			status_t			SaveItems();

protected:
	virtual	void				AttachedToWindow();
	virtual	void				DetachedFromWindow();
	virtual	void				FrameResized(float width, float height);

	virtual	void				MessageReceived(BMessage* message);
	virtual	void				MouseDown(BPoint where);

private:
			// Context menu
			BPopUpMenu*			fContextMenu;

			status_t			_InitList();
			status_t			_UpdateList();
			void				_HandleNetworkMessage(BMessage* message);
};


#endif // INTERFACES_LIST_VIEW_H
