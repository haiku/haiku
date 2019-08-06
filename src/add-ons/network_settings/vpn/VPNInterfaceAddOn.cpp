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
#define B_TRANSLATION_CONTEXT "VPNInterfaceAddOn"


static const uint32 kMsgToggleService = 'tgls';


class VPNInterfaceAddOn : public BNetworkSettingsAddOn {
public:
								VPNInterfaceAddOn(image_id image,
									BNetworkSettings& settings);
	virtual						~VPNInterfaceAddOn();

	virtual	BNetworkSettingsItem*
								CreateNextItem(uint32& cookie);
};


class VPNInterfaceView : public InterfaceView {
public:
								VPNInterfaceView(BNetworkSettings& settings);
	virtual						~VPNInterfaceView();
};


class VPNInterfaceItem : public BNetworkSettingsItem {
public:
								VPNInterfaceItem(BNetworkSettings& settings);
	virtual						~VPNInterfaceItem();

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


VPNInterfaceView::VPNInterfaceView(BNetworkSettings& settings)
	:
	InterfaceView()
{
}


VPNInterfaceView::~VPNInterfaceView()
{
}


// #pragma mark -


VPNInterfaceItem::VPNInterfaceItem(BNetworkSettings& settings)
	:
	fSettings(settings),
	fItem(new InterfaceListItem("My Awesome VPN",
		B_NETWORK_INTERFACE_TYPE_VPN)),
	fView(NULL)
{
}


VPNInterfaceItem::~VPNInterfaceItem()
{
	if (fView->Parent() == NULL)
		delete fView;

	delete fItem;
}


BNetworkSettingsType
VPNInterfaceItem::Type() const
{
	return B_NETWORK_SETTINGS_TYPE_VPN;
}


BListItem*
VPNInterfaceItem::ListItem()
{
	return fItem;
}


BView*
VPNInterfaceItem::View()
{
	if (fView == NULL)
		fView = new VPNInterfaceView(fSettings);

	return fView;
}


status_t
VPNInterfaceItem::Revert()
{
	return B_OK;
}

bool
VPNInterfaceItem::IsRevertable()
{
	// TODO
	return false;
}


// #pragma mark -


VPNInterfaceAddOn::VPNInterfaceAddOn(image_id image,
	BNetworkSettings& settings)
	:
	BNetworkSettingsAddOn(image, settings)
{
}


VPNInterfaceAddOn::~VPNInterfaceAddOn()
{
}


BNetworkSettingsItem*
VPNInterfaceAddOn::CreateNextItem(uint32& cookie)
{
	if (cookie++ == 0)
		return new VPNInterfaceItem(Settings());

	return NULL;
}


// #pragma mark -


extern "C"
BNetworkSettingsAddOn*
instantiate_network_settings_add_on(image_id image, BNetworkSettings& settings)
{
	return new VPNInterfaceAddOn(image, settings);
}
