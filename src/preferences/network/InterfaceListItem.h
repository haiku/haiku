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
#include <NetworkSettingsAddOn.h>


enum BNetworkInterfaceType {
	B_NETWORK_INTERFACE_TYPE_WIFI = 'wifi',
	B_NETWORK_INTERFACE_TYPE_ETHERNET = 'ethr',
	B_NETWORK_INTERFACE_TYPE_DIAL_UP = 'dial',
	B_NETWORK_INTERFACE_TYPE_VPN = 'nvpn',
	B_NETWORK_INTERFACE_TYPE_OTHER = 'othe',
};


class BBitmap;


class InterfaceListItem : public BListItem,
	public BNetworkKit::BNetworkConfigurationListener {
public:
								InterfaceListItem(const char* name,
									BNetworkInterfaceType type);
								~InterfaceListItem();

			void				DrawItem(BView* owner,
									BRect bounds, bool complete);
			void				Update(BView* owner, const BFont* font);

	inline	const char*			Name() const { return fInterface.Name(); }

	virtual	void				ConfigurationUpdated(const BMessage& message);

private:
			void 				_Init();
			void				_PopulateBitmaps(const char* mediaType);
			void				_UpdateState();
			BBitmap*			_StateIcon() const;
			const char*			_StateText() const;

private:
			BNetworkInterfaceType fType;

			BBitmap* 			fIcon;
			BBitmap*			fIconOffline;
			BBitmap*			fIconPending;
			BBitmap*			fIconOnline;

			BNetworkInterface	fInterface;
				// Hardware Interface

			float				fFirstLineOffset;
			float				fLineOffset;

			BString				fDeviceName;
			bool				fDisabled;
			bool				fHasLink;
			bool				fConnecting;
			BString				fSubtitle;
};


#endif // INTERFACE_LIST_ITEM_H
