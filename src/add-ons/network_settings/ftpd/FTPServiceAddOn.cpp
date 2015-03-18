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
#define B_TRANSLATION_CONTEXT "FTPServiceAddOn"


class FTPServiceAddOn : public BNetworkSettingsAddOn {
public:
								FTPServiceAddOn(image_id image,
									BNetworkSettings& settings);
	virtual						~FTPServiceAddOn();

	virtual	BNetworkSettingsItem*
								CreateNextItem(uint32& cookie);
};


class FTPServiceItem : public BNetworkSettingsItem {
public:
								FTPServiceItem(BNetworkSettings& settings);
	virtual						~FTPServiceItem();

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


FTPServiceItem::FTPServiceItem(BNetworkSettings& settings)
	:
	fSettings(settings),
	fItem(new ServiceListItem("ftp", B_TRANSLATE("FTP server"), settings)),
	fView(NULL)
{
}


FTPServiceItem::~FTPServiceItem()
{
	if (fView->Parent() == NULL)
		delete fView;

	delete fItem;
}


BNetworkSettingsType
FTPServiceItem::Type() const
{
	return B_NETWORK_SETTINGS_TYPE_SERVICE;
}


BListItem*
FTPServiceItem::ListItem()
{
	return fItem;
}


BView*
FTPServiceItem::View()
{
	if (fView == NULL) {
		fView = new ServiceView("ftp", "ftpd", B_TRANSLATE("FTP server"),
			B_TRANSLATE("The FTP server allows you to remotely access the "
				"files on your machine using the FTP protocol.\n\nPlease note "
				"that it is an insecure and unencrypted connection."),
				fSettings);
	}

	return fView;
}


status_t
FTPServiceItem::Revert()
{
	return fView != NULL ? fView->Revert() : B_OK;
}


bool
FTPServiceItem::IsRevertable()
{
	return fView != NULL ? fView->IsRevertable() : false;
}


void
FTPServiceItem::SettingsUpdated(uint32 which)
{
	if (fView != NULL)
		fView->SettingsUpdated(which);
}


// #pragma mark -


FTPServiceAddOn::FTPServiceAddOn(image_id image,
	BNetworkSettings& settings)
	:
	BNetworkSettingsAddOn(image, settings)
{
}


FTPServiceAddOn::~FTPServiceAddOn()
{
}


BNetworkSettingsItem*
FTPServiceAddOn::CreateNextItem(uint32& cookie)
{
	if (cookie++ == 0)
		return new FTPServiceItem(Settings());

	return NULL;
}


// #pragma mark -


extern "C"
BNetworkSettingsAddOn*
instantiate_network_settings_add_on(image_id image, BNetworkSettings& settings)
{
	return new FTPServiceAddOn(image, settings);
}
