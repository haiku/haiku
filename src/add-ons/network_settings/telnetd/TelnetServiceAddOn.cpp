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

#include "ServiceListItem.h"
#include "ServiceView.h"


using namespace BNetworkKit;


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "TelnetServiceAddOn"


class TelnetServiceAddOn : public BNetworkSettingsAddOn {
public:
								TelnetServiceAddOn(image_id image,
									BNetworkSettings& settings);
	virtual						~TelnetServiceAddOn();

	virtual	BNetworkSettingsItem*
								CreateNextItem(uint32& cookie);
};


class TelnetServiceItem : public BNetworkSettingsItem {
public:
								TelnetServiceItem(BNetworkSettings& settings);
	virtual						~TelnetServiceItem();

	virtual	BNetworkSettingsType
								Type() const;

	virtual BListItem*			ListItem();
	virtual BView*				View();

	virtual	status_t			Revert();
	virtual bool				IsRevertable();

	virtual void				SettingsUpdated(uint32 which);

private:
			BNetworkSettings&	fSettings;
			BListItem*			fItem;
			ServiceView*		fView;
};


// #pragma mark -


TelnetServiceItem::TelnetServiceItem(BNetworkSettings& settings)
	:
	fSettings(settings),
	fItem(new ServiceListItem("telnet", B_TRANSLATE("Telnet server"),
		settings)),
	fView(NULL)
{
}


TelnetServiceItem::~TelnetServiceItem()
{
	if (fView->Parent() == NULL)
		delete fView;

	delete fItem;
}


BNetworkSettingsType
TelnetServiceItem::Type() const
{
	return B_NETWORK_SETTINGS_TYPE_SERVICE;
}


BListItem*
TelnetServiceItem::ListItem()
{
	return fItem;
}


BView*
TelnetServiceItem::View()
{
	if (fView == NULL) {
		fView = new ServiceView("telnet", "telnetd",
			B_TRANSLATE("Telnet server"),
			B_TRANSLATE("The Telnet server allows you to remotely access "
				"your machine with a terminal session using the telnet "
				"protocol.\n\nPlease note that it is an insecure and "
				"unencrypted connection."), fSettings);
	}

	return fView;
}


status_t
TelnetServiceItem::Revert()
{
	return fView != NULL ? fView->Revert() : B_OK;
}


bool
TelnetServiceItem::IsRevertable()
{
	return fView != NULL ? fView->IsRevertable() : false;
}


void
TelnetServiceItem::SettingsUpdated(uint32 which)
{
	if (fView != NULL)
		fView->SettingsUpdated(which);
}


// #pragma mark -


TelnetServiceAddOn::TelnetServiceAddOn(image_id image,
	BNetworkSettings& settings)
	:
	BNetworkSettingsAddOn(image, settings)
{
}


TelnetServiceAddOn::~TelnetServiceAddOn()
{
}


BNetworkSettingsItem*
TelnetServiceAddOn::CreateNextItem(uint32& cookie)
{
	if (cookie++ == 0)
		return new TelnetServiceItem(Settings());

	return NULL;
}


// #pragma mark -


extern "C"
BNetworkSettingsAddOn*
instantiate_network_settings_add_on(image_id image, BNetworkSettings& settings)
{
	return new TelnetServiceAddOn(image, settings);
}
