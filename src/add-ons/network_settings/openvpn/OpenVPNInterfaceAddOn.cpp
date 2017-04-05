/*
 * Copyright 2015-2017 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, <axeld@pinc-software.de>
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 *		Alexander von Gluck IV <kallisti5@unixzen.com>
 */


#include <pwd.h>

#include <Button.h>
#include <Catalog.h>
#include <LayoutBuilder.h>
#include <NetworkSettings.h>
#include <NetworkSettingsAddOn.h>
#include <StringView.h>
#include <TextView.h>

#include <NetServer.h>
#include <RegistrarDefs.h>
#include <user_group.h>
#include <util/KMessage.h>

#include "InterfaceListItem.h"
#include "InterfaceView.h"


using namespace BNetworkKit;


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "OpenVPNInterfaceAddOn"


static const uint32 kMsgToggleService = 'tgls';


class OpenVPNInterfaceAddOn : public BNetworkSettingsAddOn {
public:
								OpenVPNInterfaceAddOn(image_id image,
									BNetworkSettings& settings);
	virtual						~OpenVPNInterfaceAddOn();

	virtual	BNetworkSettingsItem*
								CreateNextItem(uint32& cookie);
};


class OpenVPNInterfaceView : public InterfaceView {
public:
								OpenVPNInterfaceView(BNetworkSettings& settings);
	virtual						~OpenVPNInterfaceView();
};


class OpenVPNInterfaceItem : public BNetworkSettingsItem {
public:
								OpenVPNInterfaceItem(BNetworkSettings& settings);
	virtual						~OpenVPNInterfaceItem();

	virtual	BNetworkSettingsType
								Type() const;

	virtual BListItem*			ListItem();
	virtual BView*				View();

	virtual status_t			Revert();
	virtual bool				IsRevertable();

private:
			BNetworkSettings&	fSettings;
			BListItem*			fItem;
			InterfaceView*		fView;
};


// #pragma mark -


OpenVPNInterfaceView::OpenVPNInterfaceView(BNetworkSettings& settings)
	:
	InterfaceView()
{
}


OpenVPNInterfaceView::~OpenVPNInterfaceView()
{
}


// #pragma mark -


OpenVPNInterfaceItem::OpenVPNInterfaceItem(BNetworkSettings& settings)
	:
	fSettings(settings),
	fItem(new InterfaceListItem("My Awesome VPN",
		B_NETWORK_INTERFACE_TYPE_VPN)),
	fView(NULL)
{
}


OpenVPNInterfaceItem::~OpenVPNInterfaceItem()
{
	if (fView->Parent() == NULL)
		delete fView;

	delete fItem;
}


BNetworkSettingsType
OpenVPNInterfaceItem::Type() const
{
	return B_NETWORK_SETTINGS_TYPE_VPN;
}


BListItem*
OpenVPNInterfaceItem::ListItem()
{
	return fItem;
}


BView*
OpenVPNInterfaceItem::View()
{
	if (fView == NULL)
		fView = new OpenVPNInterfaceView(fSettings);

	return fView;
}


status_t
OpenVPNInterfaceItem::Revert()
{
	return B_OK;
}

bool
OpenVPNInterfaceItem::IsRevertable()
{
	// TODO
	return false;
}


// #pragma mark -


OpenVPNInterfaceAddOn::OpenVPNInterfaceAddOn(image_id image,
	BNetworkSettings& settings)
	:
	BNetworkSettingsAddOn(image, settings)
{
}


OpenVPNInterfaceAddOn::~OpenVPNInterfaceAddOn()
{
}


BNetworkSettingsItem*
OpenVPNInterfaceAddOn::CreateNextItem(uint32& cookie)
{
	if (cookie++ == 0)
		return new OpenVPNInterfaceItem(Settings());

	return NULL;
}


// #pragma mark -


extern "C"
BNetworkSettingsAddOn*
instantiate_network_settings_add_on(image_id image, BNetworkSettings& settings)
{
	return new OpenVPNInterfaceAddOn(image, settings);
}
