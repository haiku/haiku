/*
 * Copyright 2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, <axeld@pinc-software.de>
 */


#include <Catalog.h>
#include <NetworkSettingsAddOn.h>
#include <StringItem.h>

#include "InterfaceAddressView.h"


using namespace BNetworkKit;


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "IPv6InterfaceAddOn"


class IPv6InterfaceAddOn : public BNetworkSettingsAddOn {
public:
								IPv6InterfaceAddOn(image_id image,
									BNetworkSettings& settings);
	virtual						~IPv6InterfaceAddOn();

	virtual	BNetworkSettingsInterfaceItem*
								CreateNextInterfaceItem(uint32& cookie,
									const char* interface);
};


class IPv6InterfaceItem : public BNetworkSettingsInterfaceItem {
public:
								IPv6InterfaceItem(const char* interface,
									BNetworkSettings& settings);
	virtual						~IPv6InterfaceItem();

	virtual BListItem*			ListItem();
	virtual BView*				View();

	virtual	status_t			Revert();
	virtual bool				IsRevertable();

	virtual void				ConfigurationUpdated(const BMessage& message);

private:
			BNetworkSettings&	fSettings;
			BNetworkInterfaceListItem*
								fItem;
			InterfaceAddressView*
								fView;
};


// #pragma mark -


IPv6InterfaceItem::IPv6InterfaceItem(const char* interface,
	BNetworkSettings& settings)
	:
	BNetworkSettingsInterfaceItem(interface),
	fSettings(settings),
	fItem(new BNetworkInterfaceListItem(AF_INET6, Interface(),
		B_TRANSLATE("IPv6"), settings)),
	fView(NULL)
{
}


IPv6InterfaceItem::~IPv6InterfaceItem()
{
	if (fView->Parent() == NULL)
		delete fView;

	delete fItem;
}


BListItem*
IPv6InterfaceItem::ListItem()
{
	return fItem;
}


BView*
IPv6InterfaceItem::View()
{
	if (fView == NULL)
		fView = new InterfaceAddressView(AF_INET6, Interface(), fSettings);

	return fView;
}


status_t
IPv6InterfaceItem::Revert()
{
	return fView != NULL ? fView->Revert() : B_OK;
}


bool
IPv6InterfaceItem::IsRevertable()
{
	return fView != NULL ? fView->IsRevertable() : false;
}


void
IPv6InterfaceItem::ConfigurationUpdated(const BMessage& message)
{
	if (fView != NULL)
		fView->ConfigurationUpdated(message);
}


// #pragma mark -


IPv6InterfaceAddOn::IPv6InterfaceAddOn(image_id image,
	BNetworkSettings& settings)
	:
	BNetworkSettingsAddOn(image, settings)
{
}


IPv6InterfaceAddOn::~IPv6InterfaceAddOn()
{
}


BNetworkSettingsInterfaceItem*
IPv6InterfaceAddOn::CreateNextInterfaceItem(uint32& cookie,
	const char* interface)
{
	if (cookie++ == 0)
		return new IPv6InterfaceItem(interface, Settings());

	return NULL;
}


// #pragma mark -


extern "C"
BNetworkSettingsAddOn*
instantiate_network_settings_add_on(image_id image, BNetworkSettings& settings)
{
	return new IPv6InterfaceAddOn(image, settings);
}
