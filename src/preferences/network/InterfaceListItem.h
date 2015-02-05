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
			void				_UpdateState();
			BBitmap*			_StateIcon() const;
			const char*			_StateText() const;

private:
			BBitmap* 			fIcon;
			BBitmap*			fIconOffline;
			BBitmap*			fIconPending;
			BBitmap*			fIconOnline;

			BNetworkInterface	fInterface;
				// Hardware Interface

			float				fFirstlineOffset;
			float				fSecondlineOffset;
			float				fThirdlineOffset;

			BString				fDeviceName;
			bool				fDisabled;
			bool				fHasLink;
			bool				fConnecting;
			BString				fAddress[2];
};


#endif // INTERFACE_LIST_ITEM_H
