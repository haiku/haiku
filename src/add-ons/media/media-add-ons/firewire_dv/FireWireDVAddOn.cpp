/*
 * FireWire DV media addon for Haiku
 *
 * Copyright (c) 2008, JiSheng Zhang (jszhang3@mail.ustc.edu.cn)
 * Distributed under the terms of the MIT License.
 *
 * Based on DVB media addon 
 * Copyright (c) 2004-2007 Marcus Overhagen <marcus@overhagen.de>
 */

#include "FireWireDVAddOn.h"
#include "FireWireCard.h"
#include "FireWireDVNode.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#include <Alert.h>
#include <Directory.h>
#include <Entry.h>
#include <Path.h>

#include "debug.h"

struct device_info
{
	FireWireCard*	card;
	char		name[16];
	media_format 	in_formats[1];
	media_format 	out_formats[1];
	flavor_info 	flavor;
};

extern "C" BMediaAddOn*
make_media_addon(image_id id)
{
	CALLED();
	return new FireWireDVAddOn(id);
}


FireWireDVAddOn::FireWireDVAddOn(image_id id)
	: BMediaAddOn(id)
{
	CALLED();
	ScanFolder("/dev/bus/fw");
}


FireWireDVAddOn::~FireWireDVAddOn()
{
	CALLED();
	FreeDeviceList();
}


status_t
FireWireDVAddOn::InitCheck(const char** out_failure_text)
{
	CALLED();
	if (!fDeviceList.CountItems()) {
		*out_failure_text = "No supported device found";
		return B_ERROR;
	}
	return B_OK;
}


int32
FireWireDVAddOn::CountFlavors()
{
	CALLED();
	return fDeviceList.CountItems();
}


status_t
FireWireDVAddOn::GetFlavorAt(int32 n, const flavor_info** out_info)
{
	CALLED();
	device_info* dev = fDeviceList.ItemAt(n);
	if (!dev)
		return B_ERROR;
	*out_info = &dev->flavor;
	return B_OK;
}


BMediaNode *
FireWireDVAddOn::InstantiateNodeFor(const flavor_info* info, BMessage* config,
	status_t* out_error)
{
	CALLED();
	device_info* dev = fDeviceList.ItemAt(info->internal_id);
	if (!dev || dev->flavor.internal_id != info->internal_id) {
		*out_error = B_ERROR;
		return NULL;
	}
	*out_error = B_OK;
	
	return new FireWireDVNode(this, dev->name, dev->flavor.internal_id, 
		dev->card);
}


bool
FireWireDVAddOn::WantsAutoStart()
{
	CALLED();
	return false;
}


status_t
FireWireDVAddOn::AutoStart(int index, BMediaNode** outNode, 
	int32* outInternalID, bool* outHasMore)
{
	CALLED();
	return B_ERROR;
}


void
FireWireDVAddOn::ScanFolder(const char* path)
{
	CALLED();
	BDirectory dir(path);
	if (dir.InitCheck() != B_OK)
		return;

	BEntry ent;

	while (dir.GetNextEntry(&ent) == B_OK) {
		BPath path(&ent);
		if (!ent.IsDirectory() && !ent.IsSymLink()) {
			FireWireCard *card = new FireWireCard(path.Path());
			if (card->InitCheck() == B_OK)
				AddDevice(card, path.Path());
			else
				delete card;
		}
	}
}


void
FireWireDVAddOn::AddDevice(FireWireCard* card, const char* path)
{
	const char* fwnumber;
	
	// get card name, info and type
		
	// get interface number
	const char *p = strrchr(path, '/');
	fwnumber = p ? p + 1 : "";
	
	// create device_info
	device_info *dev = new device_info;
	fDeviceList.AddItem(dev);
	
	dev->card = card;
	
	sprintf(dev->name, "FireWire-%s", fwnumber);
	
	// setup supported formats (the lazy way)
	memset(dev->in_formats, 0, sizeof(dev->in_formats));
	memset(dev->out_formats, 0, sizeof(dev->out_formats));
	dev->in_formats[0].type = B_MEDIA_ENCODED_VIDEO;
	dev->out_formats[0].type = B_MEDIA_ENCODED_VIDEO;
	
	// setup flavor info
	dev->flavor.name = dev->name;
	dev->flavor.info = (char *)"The FireWireDVNode outputs to fw_raw drivers.";
	dev->flavor.kinds = B_BUFFER_CONSUMER | B_BUFFER_PRODUCER 
		| B_CONTROLLABLE | B_PHYSICAL_OUTPUT | B_PHYSICAL_INPUT;
	dev->flavor.flavor_flags = B_FLAVOR_IS_GLOBAL;
	dev->flavor.internal_id = fDeviceList.CountItems() - 1;
	dev->flavor.possible_count = 1;
	dev->flavor.in_format_count = 1;
	dev->flavor.in_format_flags = 0;
	dev->flavor.in_formats = dev->in_formats;
	dev->flavor.out_format_count = 1;
	dev->flavor.out_format_flags = 0;
	dev->flavor.out_formats = dev->out_formats;
}


void
FireWireDVAddOn::FreeDeviceList()
{
	device_info* dev;
	while ((dev = fDeviceList.RemoveItemAt(0L))) {
		delete dev->card;
		delete dev;
	}
}

