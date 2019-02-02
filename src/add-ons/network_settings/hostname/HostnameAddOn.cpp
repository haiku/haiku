/*
 * Copyright 2019 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, <axeld@pinc-software.de>
 *		Rob Gill, <rrobgill@protonmail.com>
 */


#include <Catalog.h>
#include <NetworkSettings.h>
#include <NetworkSettingsAddOn.h>
#include <StringItem.h>

#include "HostnameView.h"


using namespace BNetworkKit;


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "HostnameAddOn"


class HostnameAddOn : public BNetworkSettingsAddOn {
public:
								HostnameAddOn(image_id image,
									BNetworkSettings& settings);
	virtual						~HostnameAddOn();

	virtual	BNetworkSettingsItem*
								CreateNextItem(uint32& cookie);
};


class HostnameItem : public BNetworkSettingsItem {
public:
								HostnameItem(
									BNetworkSettings& settings);
	virtual						~HostnameItem();

	virtual	BNetworkSettingsType
								Type() const;

	virtual	BListItem*			ListItem();
	virtual	BView*				View();

	virtual	status_t			Revert();
	virtual	bool				IsRevertable();

private:
			BNetworkSettings&	fSettings;
			BStringItem*		fItem;
			HostnameView*		fView;
};


// #pragma mark -


HostnameItem::HostnameItem(BNetworkSettings& settings)
	:
	fSettings(settings),
	fItem(new BStringItem(B_TRANSLATE("Hostname settings"))),
	fView(NULL)
{
}


HostnameItem::~HostnameItem()
{
	if (fView->Parent() == NULL)
		delete fView;

	delete fItem;
}


BNetworkSettingsType
HostnameItem::Type() const
{
	return B_NETWORK_SETTINGS_TYPE_SERVICE;
}


BListItem*
HostnameItem::ListItem()
{
	return fItem;
}


BView*
HostnameItem::View()
{
	if (fView == NULL)
		fView = new HostnameView(this);

	return fView;
}


status_t
HostnameItem::Revert()
{
	return fView != NULL ? fView->Revert() : B_OK;
}


bool
HostnameItem::IsRevertable()
{
	return fView != NULL ? fView->IsRevertable() : false;
}


// #pragma mark -


HostnameAddOn::HostnameAddOn(image_id image,
	BNetworkSettings& settings)
	:
	BNetworkSettingsAddOn(image, settings)
{
}


HostnameAddOn::~HostnameAddOn()
{
}


BNetworkSettingsItem*
HostnameAddOn::CreateNextItem(uint32& cookie)
{
	if (cookie++ == 0)
		return new HostnameItem(Settings());

	return NULL;
}


// #pragma mark -


extern "C"
BNetworkSettingsAddOn*
instantiate_network_settings_add_on(image_id image, BNetworkSettings& settings)
{
	return new HostnameAddOn(image, settings);
}
