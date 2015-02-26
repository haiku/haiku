/*
 * Copyright 2004-2015 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <NetworkSettingsAddOn.h>

#include <stdio.h>
#include <stdlib.h>


using namespace BNetworkKit;


BNetworkSettingsItem::BNetworkSettingsItem()
	:
	fProfile(NULL)
{
}


BNetworkSettingsItem::~BNetworkSettingsItem()
{
}


status_t
BNetworkSettingsItem::ProfileChanged(const BNetworkProfile* newProfile)
{
	fProfile = newProfile;
	return B_OK;
}


const BNetworkProfile*
BNetworkSettingsItem::Profile() const
{
	return fProfile;
}


void
BNetworkSettingsItem::SettingsUpdated(uint32 type)
{
}


void
BNetworkSettingsItem::ConfigurationUpdated(const BMessage& message)
{
}


// #pragma mark -


BNetworkSettingsInterfaceItem::BNetworkSettingsInterfaceItem(
	const char* interface)
	:
	fInterface(interface)
{
}


BNetworkSettingsInterfaceItem::~BNetworkSettingsInterfaceItem()
{
}


BNetworkSettingsType
BNetworkSettingsInterfaceItem::Type() const
{
	return B_NETWORK_SETTINGS_TYPE_INTERFACE;
}


const char*
BNetworkSettingsInterfaceItem::Interface() const
{
	return fInterface;
}


// #pragma mark -


BNetworkSettingsAddOn::BNetworkSettingsAddOn(image_id image,
	BNetworkSettings& settings)
	:
	fImage(image),
	fResources(NULL),
	fSettings(settings)
{
}


BNetworkSettingsAddOn::~BNetworkSettingsAddOn()
{
	delete fResources;
}


BNetworkSettingsInterfaceItem*
BNetworkSettingsAddOn::CreateNextInterfaceItem(uint32& cookie,
	const char* interface)
{
	return NULL;
}


BNetworkSettingsItem*
BNetworkSettingsAddOn::CreateNextItem(uint32& cookie)
{
	return NULL;
}


image_id
BNetworkSettingsAddOn::Image()
{
	return fImage;
}


BResources*
BNetworkSettingsAddOn::Resources()
{
	if (fResources == NULL) {
		image_info info;
		if (get_image_info(fImage, &info) != B_OK)
			return NULL;

		BResources* resources = new BResources();
		BFile file(info.name, B_READ_ONLY);
		if (resources->SetTo(&file) == B_OK)
			fResources = resources;
		else
			delete resources;
	}
	return fResources;
}


BNetworkSettings&
BNetworkSettingsAddOn::Settings()
{
	return fSettings;
}
