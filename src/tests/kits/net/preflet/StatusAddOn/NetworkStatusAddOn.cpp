/*
 * Copyright 2004-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andre Alves Garzia, andre@andregarzia.com
 *		Stephan Assmuß
 *		Axel Dörfler
 *		Philippe Houdoin
 * 		Fredrik Modéen
 *		Hugo Santos
 *		Philippe Saint-Pierre
 */


#include "NetworkStatusAddOn.h"

#include <net/if.h>

#include <sys/sockio.h>

#include <stdio.h>
#include <AutoDeleter.h>

#include <ScrollView.h>
#include <Alert.h>

#include "InterfaceStatusItem.h"
#include "NetworkWindow.h"

NetworkSetupAddOn*
get_nth_addon(image_id image, int index)
{
	if (index == 0)
		return new NetworkStatusAddOn(image);
	return NULL;
}


// #pragma mark -
NetworkStatusAddOn::NetworkStatusAddOn(image_id image)
	:
	NetworkSetupAddOn(image),
	BBox(BRect(0, 0, 0, 0), NULL, B_FOLLOW_ALL, B_NAVIGABLE_JUMP, B_NO_BORDER)
{
}


NetworkStatusAddOn::~NetworkStatusAddOn()
{
}


const char*
NetworkStatusAddOn::Name()
{
	return "Overview";
}


BView*
NetworkStatusAddOn::CreateView(BRect *bounds)
{
	float w, h;
	BRect r = *bounds;
	
#define H_MARGIN	10
#define V_MARGIN	10
#define SMALL_MARGIN	3
	
	if (r.Width() < 100 || r.Height() < 100)
		r.Set(0, 0, 100, 100);
		
	ResizeTo(r.Width(), r.Height());

	BRect rlv = r;
	rlv.bottom -= 72;
	rlv.InsetBy(2, 2);
	rlv.right -= B_V_SCROLL_BAR_WIDTH;
	fListview = new BListView(rlv, NULL, B_SINGLE_SELECTION_LIST, 
		B_FOLLOW_ALL_SIDES);
	fListview->SetSelectionMessage(new BMessage(INTERFACE_SELECTED_MSG));
	fListview->SetInvocationMessage(new BMessage(CONFIGURE_INTERFACE_MSG));	
	AddChild(new BScrollView(NULL, fListview, B_FOLLOW_ALL_SIDES, B_WILL_DRAW 
			| B_FRAME_EVENTS, false, true));
	
	r.top = r.bottom - 60;
	fConfigure = new BButton(r, "configure", "Configure" B_UTF8_ELLIPSIS, 
		new BMessage(CONFIGURE_INTERFACE_MSG), B_FOLLOW_BOTTOM | B_FOLLOW_LEFT);
		
	fConfigure->GetPreferredSize(&w, &h);
	fConfigure->ResizeToPreferred();
	fConfigure->SetEnabled(false);
	AddChild(fConfigure);

	r.left += w + SMALL_MARGIN;

	fOnOff = new BButton(r, "onoff", "Disable", new BMessage(ONOFF_INTERFACE_MSG),
					B_FOLLOW_BOTTOM | B_FOLLOW_LEFT);
	fOnOff->GetPreferredSize(&w, &h);
	fOnOff->ResizeToPreferred();
	fOnOff->Hide();
	AddChild(fOnOff);

	fSocket = socket(AF_INET, SOCK_DGRAM, 0);
	_LookupDevices();

	*bounds = Bounds();	
	return this;
}


void
NetworkStatusAddOn::AttachedToWindow()
{
	fListview->SetTarget(this);
	fConfigure->SetTarget(this);
	fOnOff->SetTarget(this);

}


void
NetworkStatusAddOn::MessageReceived(BMessage* msg)
{
	int nr = fListview->CurrentSelection();
	InterfaceStatusItem *item = NULL;
	if(nr != -1) {		
		item = dynamic_cast<InterfaceStatusItem*>(fListview->ItemAt(nr));
	}

	switch (msg->what) {
	case INTERFACE_SELECTED_MSG: {
		if (!item)
			break;

		fConfigure->SetEnabled(item != NULL);
		fOnOff->SetEnabled(item != NULL);
		if (item == NULL) {
			fOnOff->Hide();
			break;
		}
		fOnOff->SetLabel(item->Enabled() ? "Disable" : "Enable");
		fOnOff->Show();		
		break;
	}
		
	case CONFIGURE_INTERFACE_MSG: {
		if (!item)
			break;

		NetworkWindow* nw = new NetworkWindow(item->GetSetting());
		nw->Show();
		break;
	}
		
	case ONOFF_INTERFACE_MSG:
		if (!item)
			break;
		
		item->SetEnabled(!item->Enabled());
		fOnOff->SetLabel(item->Enabled() ? "Disable" : "Enable");
		fListview->Invalidate();
		break;

	default:
		BBox::MessageReceived(msg);
	}
}


status_t
NetworkStatusAddOn::_LookupDevices()
{
	// iterate over all interfaces and retrieve minimal status
	ifconf config;
	config.ifc_len = sizeof(config.ifc_value);
	if (ioctl(fSocket, SIOCGIFCOUNT, &config, sizeof(struct ifconf)) < 0)
		return B_ERROR;

	uint32 count = (uint32)config.ifc_value;
	if (count == 0)
		return B_ERROR;

	void* buffer = malloc(count * sizeof(struct ifreq));
	if (buffer == NULL)
		return B_ERROR;

	MemoryDeleter deleter(buffer);
	
	config.ifc_len = count * sizeof(struct ifreq);
	config.ifc_buf = buffer;
	if (ioctl(fSocket, SIOCGIFCONF, &config, sizeof(struct ifconf)) < 0)
		return B_ERROR;

	ifreq* interface = (ifreq*)buffer;
	fListview->MakeEmpty();

	for (uint32 i = 0; i < count; i++) {
		fListview->AddItem(new InterfaceStatusItem(interface->ifr_name, this));
//		printf("Name = %s\n", interface->ifr_name);
		interface = (ifreq*)((addr_t)interface + IF_NAMESIZE 
			+ interface->ifr_addr.sa_len);
	}	
	return B_OK;
}
