/*
 * Copyright 2003-2004 Waldemar Kornewald. All rights reserved.
 * Copyright 2017 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dirent.h>
#include <sys/stat.h>

#include <String.h>
#include <ListView.h>
#include <ListItem.h>
#include <ScrollView.h>
#include <Box.h>
#include <Button.h>
#include <Bitmap.h>
#include <Alert.h>

#include <NetworkSettings.h>
#include <NetworkSettingsAddOn.h>
#include "InterfaceListItem.h"

#include "DialUpView.h"


using namespace BNetworkKit;


class AddOn : public BNetworkSettingsAddOn {
public:
		AddOn(image_id addon_image, BNetworkSettings& settings);
		virtual ~AddOn();

		virtual	BNetworkSettingsItem*
								CreateNextItem(uint32& cookie);
};


class DialUpInterfaceItem : public BNetworkSettingsItem {
public:
								DialUpInterfaceItem(BNetworkSettings& settings);
	virtual						~DialUpInterfaceItem();

	virtual	BNetworkSettingsType
								Type() const;

	virtual BListItem*			ListItem();
	virtual BView*				View();

	virtual status_t			Revert();
	virtual bool				IsRevertable();

private:
			BNetworkSettings&	fSettings;
			BListItem*			fItem;
			BView*				fView;
};


// #pragma mark -


DialUpInterfaceItem::DialUpInterfaceItem(BNetworkSettings& settings)
	:
	fSettings(settings),
	fItem(new InterfaceListItem("Dialup",
		B_NETWORK_INTERFACE_TYPE_DIAL_UP)),
	fView(NULL)
{
}


DialUpInterfaceItem::~DialUpInterfaceItem()
{
	if (fView->Parent() == NULL)
		delete fView;

	delete fItem;
}


BNetworkSettingsType
DialUpInterfaceItem::Type() const
{
	return B_NETWORK_SETTINGS_TYPE_DIAL_UP;
}


BListItem*
DialUpInterfaceItem::ListItem()
{
	return fItem;
}


BView*
DialUpInterfaceItem::View()
{
	if (fView == NULL)
		fView = new DialUpView(/*fSettings*/);

	return fView;
}


status_t
DialUpInterfaceItem::Revert()
{
	return B_OK;
}

bool
DialUpInterfaceItem::IsRevertable()
{
	// TODO
	return false;
}


// #pragma mark -


AddOn::AddOn(image_id image, BNetworkSettings& settings)
	: BNetworkSettingsAddOn(image, settings)
{
}


AddOn::~AddOn()
{
}


BNetworkSettingsItem*
AddOn::CreateNextItem(uint32& cookie)
{
	if (cookie++ == 0)
		return new DialUpInterfaceItem(Settings());

	return NULL;
}


// #pragma mark -


extern "C"
BNetworkSettingsAddOn*
instantiate_network_settings_add_on(image_id image, BNetworkSettings& settings)
{
	return new AddOn(image, settings);
}
