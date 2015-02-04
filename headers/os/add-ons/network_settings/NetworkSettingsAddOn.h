/*
 * Copyright 2004-2015 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 */
#ifndef _NETWORK_SETTINGS_ADD_ON_H
#define _NETWORK_SETTINGS_ADD_ON_H


#include <image.h>
#include <ListItem.h>
#include <Resources.h>
#include <View.h>


namespace BNetworkKit {


enum BNetworkSettingsType {
	B_NETWORK_SETTINGS_TYPE_INTERFACE = 'intf',
	B_NETWORK_SETTINGS_TYPE_SERVICE = 'serv',
	B_NETWORK_SETTINGS_TYPE_DIAL_UP = 'dial',
	B_NETWORK_SETTINGS_TYPE_OTHER = 'othr'
};

class BNetworkProfile;


class BNetworkSettingsItem {
public:
								BNetworkSettingsItem();
	virtual						~BNetworkSettingsItem();

	virtual	BNetworkSettingsType
								Type() const = 0;
	virtual BListItem*			ListItem() = 0;
	virtual BView*				View() = 0;

	virtual status_t			ProfileChanged(
									const BNetworkProfile* newProfile);
			const BNetworkProfile*
								Profile() const;

	virtual	status_t			Save() = 0;
	virtual	status_t			Revert() = 0;

private:
			const BNetworkProfile*
								fProfile;
};


class BNetworkSettingsInterfaceItem : public BNetworkSettingsItem {
public:
								BNetworkSettingsInterfaceItem(
									const char* interface);

	virtual	BNetworkSettingsType
								Type() const;
			const char*			Interface() const;

private:
			const char*			fInterface;
};


class BNetworkSettingsAddOn {
public:
								BNetworkSettingsAddOn(image_id image);
	virtual						~BNetworkSettingsAddOn();

	virtual	BNetworkSettingsInterfaceItem*
								CreateNextInterfaceItem(uint32& cookie,
									const char* interface);
	virtual	BNetworkSettingsItem*
								CreateNextItem(uint32& cookie);

			image_id			Image();
			BResources*			Resources();

private:
			image_id			fImage;
			BResources*			fResources;
};


// Your add-on needs to export this hook in order to be picked up
extern "C" BNetworkSettingsAddOn* instantiate_network_settings_add_on(
	image_id image);


}	// namespace BNetworkKit


#endif // _NETWORK_SETTINGS_ADD_ON_H
