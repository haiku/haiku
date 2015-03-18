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
#define B_TRANSLATION_CONTEXT "IPv4InterfaceAddOn"


class IPv4InterfaceAddOn : public BNetworkSettingsAddOn {
public:
								IPv4InterfaceAddOn(image_id image,
									BNetworkSettings& settings);
	virtual						~IPv4InterfaceAddOn();

	virtual	BNetworkSettingsInterfaceItem*
								CreateNextInterfaceItem(uint32& cookie,
									const char* interface);
};


class IPv4InterfaceItem : public BNetworkSettingsInterfaceItem {
public:
								IPv4InterfaceItem(const char* interface,
									BNetworkSettings& settings);
	virtual						~IPv4InterfaceItem();

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


IPv4InterfaceItem::IPv4InterfaceItem(const char* interface,
	BNetworkSettings& settings)
	:
	BNetworkSettingsInterfaceItem(interface),
	fSettings(settings),
	fItem(new BNetworkInterfaceListItem(AF_INET, Interface(),
		B_TRANSLATE("IPv4"), settings)),
	fView(NULL)
{
}


IPv4InterfaceItem::~IPv4InterfaceItem()
{
	if (fView->Parent() == NULL)
		delete fView;

	delete fItem;
}


BListItem*
IPv4InterfaceItem::ListItem()
{
	return fItem;
}


BView*
IPv4InterfaceItem::View()
{
	if (fView == NULL)
		fView = new InterfaceAddressView(AF_INET, Interface(), fSettings);

	return fView;
}


status_t
IPv4InterfaceItem::Revert()
{
	return fView != NULL ? fView->Revert() : B_OK;
}


bool
IPv4InterfaceItem::IsRevertable()
{
	return fView != NULL ? fView->IsRevertable() : false;
}


void
IPv4InterfaceItem::ConfigurationUpdated(const BMessage& message)
{
	if (fView != NULL)
		fView->ConfigurationUpdated(message);
}


// #pragma mark -


IPv4InterfaceAddOn::IPv4InterfaceAddOn(image_id image,
	BNetworkSettings& settings)
	:
	BNetworkSettingsAddOn(image, settings)
{
}


IPv4InterfaceAddOn::~IPv4InterfaceAddOn()
{
}


BNetworkSettingsInterfaceItem*
IPv4InterfaceAddOn::CreateNextInterfaceItem(uint32& cookie,
	const char* interface)
{
	if (cookie++ == 0)
		return new IPv4InterfaceItem(interface, Settings());

	return NULL;
}


// #pragma mark -


extern "C"
BNetworkSettingsAddOn*
instantiate_network_settings_add_on(image_id image, BNetworkSettings& settings)
{
	return new IPv4InterfaceAddOn(image, settings);
}
