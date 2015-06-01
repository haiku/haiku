/*
 * Copyright 2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, <axeld@pinc-software.de>
 */


#include <Catalog.h>
#include <NetworkSettings.h>
#include <NetworkSettingsAddOn.h>
#include <StringItem.h>

#include "DNSSettingsView.h"


using namespace BNetworkKit;


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "DNSClientServiceAddOn"


class DNSClientServiceAddOn : public BNetworkSettingsAddOn {
public:
								DNSClientServiceAddOn(image_id image,
									BNetworkSettings& settings);
	virtual						~DNSClientServiceAddOn();

	virtual	BNetworkSettingsItem*
								CreateNextItem(uint32& cookie);
};


class DNSClientServiceItem : public BNetworkSettingsItem {
public:
								DNSClientServiceItem(
									BNetworkSettings& settings);
	virtual						~DNSClientServiceItem();

	virtual	BNetworkSettingsType
								Type() const;

	virtual BListItem*			ListItem();
	virtual BView*				View();

	virtual	status_t			Revert();
	virtual bool				IsRevertable();

private:
			BNetworkSettings&	fSettings;
			BStringItem*		fItem;
			DNSSettingsView*	fView;
};


// #pragma mark -


DNSClientServiceItem::DNSClientServiceItem(BNetworkSettings& settings)
	:
	fSettings(settings),
	fItem(new BStringItem(B_TRANSLATE("DNS settings"))),
	fView(NULL)
{
}


DNSClientServiceItem::~DNSClientServiceItem()
{
	if (fView->Parent() == NULL)
		delete fView;

	delete fItem;
}


BNetworkSettingsType
DNSClientServiceItem::Type() const
{
	return B_NETWORK_SETTINGS_TYPE_SERVICE;
}


BListItem*
DNSClientServiceItem::ListItem()
{
	return fItem;
}


BView*
DNSClientServiceItem::View()
{
	if (fView == NULL)
		fView = new DNSSettingsView(this);

	return fView;
}


status_t
DNSClientServiceItem::Revert()
{
	return B_OK;
}


bool
DNSClientServiceItem::IsRevertable()
{
	return false;
}


// #pragma mark -


DNSClientServiceAddOn::DNSClientServiceAddOn(image_id image,
	BNetworkSettings& settings)
	:
	BNetworkSettingsAddOn(image, settings)
{
}


DNSClientServiceAddOn::~DNSClientServiceAddOn()
{
}


BNetworkSettingsItem*
DNSClientServiceAddOn::CreateNextItem(uint32& cookie)
{
	if (cookie++ == 0)
		return new DNSClientServiceItem(Settings());

	return NULL;
}


// #pragma mark -


extern "C"
BNetworkSettingsAddOn*
instantiate_network_settings_add_on(image_id image, BNetworkSettings& settings)
{
	return new DNSClientServiceAddOn(image, settings);
}
