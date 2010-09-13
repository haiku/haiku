/*
 * Copyright (c) 2004-2007 Marcus Overhagen <marcus@overhagen.de>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction, 
 * including without limitation the rights to use, copy, modify, 
 * merge, publish, distribute, sublicense, and/or sell copies of 
 * the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be 
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR 
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "DVBMediaAddon.h"
#include "DVBCard.h"
#include "DVBMediaNode.h"
#include "config.h"

#include <string.h>
#include <stdio.h>
#include <time.h>
#include <Alert.h>
#include <Directory.h>
#include <Entry.h>
#include <Path.h>

struct device_info
{
	DVBCard *		card;
	char			name[200];
	char			info[200];
	media_format 	formats[5];
	flavor_info 	flavor;
};


extern "C" BMediaAddOn * 
make_media_addon(image_id id)
{
	return new DVBMediaAddon(id);
}


DVBMediaAddon::DVBMediaAddon(image_id id)
 :	BMediaAddOn(id)
{
	ScanFolder("/dev/dvb");
}


DVBMediaAddon::~DVBMediaAddon()
{
	FreeDeviceList();
}


status_t
DVBMediaAddon::InitCheck(const char ** out_failure_text)
{
	if (!fDeviceList.CountItems()) {
		*out_failure_text = "No supported device found";
		return B_ERROR;
	}
	return B_OK;
}


int32
DVBMediaAddon::CountFlavors()
{
	return fDeviceList.CountItems();
}


status_t
DVBMediaAddon::GetFlavorAt(int32 n, const flavor_info ** out_info)
{
	device_info *dev = (device_info *)fDeviceList.ItemAt(n);
	if (!dev)
		return B_ERROR;
	*out_info = &dev->flavor;
	return B_OK;
}

	
BMediaNode *
DVBMediaAddon::InstantiateNodeFor(const flavor_info * info, BMessage * config,	status_t * out_error)
{
	printf("DVB: DVBMediaAddon::InstantiateNodeFor\n");
	
	device_info *dev = (device_info *)fDeviceList.ItemAt(info->internal_id);
	if (!dev || dev->flavor.internal_id != info->internal_id) {
		*out_error = B_ERROR;
		return NULL;
	}
	*out_error = B_OK;
	
	return new DVBMediaNode(this, dev->name, dev->flavor.internal_id, dev->card);
}
	

bool
DVBMediaAddon::WantsAutoStart()
{
#if BUILD_FOR_HAIKU
//	printf("DVB: DVBMediaAddon::WantsAutoStart - NO\n");
	return false;
#else
//	printf("DVB: DVBMediaAddon::WantsAutoStart - YES\n");
	return true;
#endif
}


status_t
DVBMediaAddon::AutoStart(int index, BMediaNode **outNode, int32 *outInternalID, bool *outHasMore)
{
	printf("DVB: DVBMediaAddon::AutoStart, index %d\n", index);

#if BUILD_FOR_HAIKU
	return B_ERROR;
#else

	device_info *dev = (device_info *)fDeviceList.ItemAt(index);
	if (!dev) {
		printf("DVB: DVBMediaAddon::AutoStart, bad index\n");
		return B_BAD_INDEX;
	}
		
	*outHasMore = fDeviceList.ItemAt(index + 1) ? true : false;
	*outInternalID = dev->flavor.internal_id;
	*outNode = new DVBMediaNode(this, dev->name, dev->flavor.internal_id, dev->card);

	printf("DVB: DVBMediaAddon::AutoStart, success\n");

	return B_OK;
#endif
}


void
DVBMediaAddon::ScanFolder(const char *path)
{
	BDirectory dir(path);
	if (dir.InitCheck() != B_OK)
		return;

	BEntry ent;

	while (dir.GetNextEntry(&ent) == B_OK) {
		BPath path(&ent);
		if (ent.IsDirectory()) {
			ScanFolder(path.Path());
		} else {
			DVBCard *card = new DVBCard(path.Path());
			if (card->InitCheck() == B_OK)
				AddDevice(card, path.Path());
			else
				delete card;
		}
	}
}


void
DVBMediaAddon::AddDevice(DVBCard *card, const char *path)
{
	dvb_type_t type;
	char name[250];	
	char info[1000];
	const char *dvbtype;
	const char *dvbnumber;
	
	// get card name, info and type
	if (B_OK != card->GetCardType(&type)) {
		printf("DVBMediaAddon::AddDevice: getting DVB card type failed\n");
		return;
	}
	if (B_OK != card->GetCardInfo(name, sizeof(name), info, sizeof(info))) {
		printf("DVBMediaAddon::AddDevice: getting DVB card info failed\n");
		return;
	}
	
	// get type
	switch (type) {
		case DVB_TYPE_DVB_C: dvbtype = "C"; break;
		case DVB_TYPE_DVB_H: dvbtype = "H"; break;
		case DVB_TYPE_DVB_S: dvbtype = "S"; break;
		case DVB_TYPE_DVB_T: dvbtype = "T"; break;
		default:
			printf("DVBMediaAddon::AddDevice: unsupported DVB type %d\n", type);
			return;
	}
	
	// get interface number
	const char *p = strrchr(path, '/');
	dvbnumber = p ? p + 1 : "";
	
	// create device_info
	device_info *dev = new device_info;
	fDeviceList.AddItem(dev);
	
	dev->card = card;
	
	// setup name and info, it's important that name starts with "DVB"
//	snprintf(dev->name, sizeof(dev->name), "DVB-%s %s (%s)", dvbtype, dvbnumber, name);
//	strlcpy(dev->info, info, sizeof(dev->info));
	sprintf(dev->name, "DVB-%s %s %s", dvbtype, name, dvbnumber);
	strcpy(dev->info, info);
	
	// setup supported formats (the lazy way)
	memset(dev->formats, 0, sizeof(dev->formats));
	dev->formats[0].type = B_MEDIA_RAW_VIDEO;
	dev->formats[1].type = B_MEDIA_RAW_AUDIO;
	dev->formats[2].type = B_MEDIA_ENCODED_VIDEO;
	dev->formats[3].type = B_MEDIA_ENCODED_AUDIO;
	dev->formats[4].type = B_MEDIA_MULTISTREAM;
	
	// setup flavor info
	dev->flavor.name = dev->name;
	dev->flavor.info = dev->info;
	dev->flavor.kinds = B_BUFFER_PRODUCER | B_CONTROLLABLE | B_PHYSICAL_INPUT;
	dev->flavor.flavor_flags = B_FLAVOR_IS_GLOBAL;
	dev->flavor.internal_id = fDeviceList.CountItems() - 1;
	dev->flavor.possible_count = 1;
	dev->flavor.in_format_count = 0;
	dev->flavor.in_format_flags = 0;
	dev->flavor.in_formats = 0;
	dev->flavor.out_format_count = 5;
	dev->flavor.out_format_flags = 0;
	dev->flavor.out_formats = dev->formats;
}


void
DVBMediaAddon::FreeDeviceList()
{
	device_info *dev;
	while ((dev = (device_info *)fDeviceList.RemoveItem((int32)0))) {
		delete dev->card;
		delete dev;
	}
}
