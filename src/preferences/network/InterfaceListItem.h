/*
 * Copyright 2004-2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck, kallisti5@unixzen.com
 *		Philippe Houdoin
 *		Fredrik Modéen
 *		John Scipione, jscipione@gmail.com
 */
#ifndef INTERFACE_LIST_ITEM_H
#define INTERFACE_LIST_ITEM_H


#include <ListItem.h>
#include <NetworkInterface.h>


class BBitmap;


class InterfaceListItem : public BListItem {
public:
								InterfaceListItem(const char* name);
								~InterfaceListItem();

			void				DrawItem(BView* owner,
									BRect bounds, bool complete);
			void				Update(BView* owner, const BFont* font);

	inline	const char*			Name() { return fInterface.Name(); }

private:
			void 				_Init();
			void				_PopulateBitmaps(const char* mediaType);

			BBitmap* 			fIcon;
			BBitmap*			fIconOffline;
			BBitmap*			fIconPending;
			BBitmap*			fIconOnline;

			BNetworkInterface	fInterface;
				// Hardware Interface

			float				fFirstlineOffset;
			float				fSecondlineOffset;
			float				fThirdlineOffset;
};


#endif // INTERFACE_LIST_ITEM_H
