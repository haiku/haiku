/*
 * FireWire DV media addon for Haiku
 *
 * Copyright (c) 2008, JiSheng Zhang (jszhang3@mail.ustc.edu.cn)
 * Distributed under the terms of the MIT License.
 *
 * Based on DVB media addon
 * Copyright (c) 2004-2007 Marcus Overhagen <marcus@overhagen.de>
 */
#ifndef _FIREWIRE_DV_ADDON_H_
#define _FIREWIRE_DV_ADDON_H_


#include <MediaAddOn.h>
#include <ObjectList.h>


class FireWireCard;
struct device_info;


class FireWireDVAddOn : public BMediaAddOn {
public:
			FireWireDVAddOn(image_id id);
			~FireWireDVAddOn();

	status_t	InitCheck(const char** out_failure_text);

	int32		CountFlavors();

	status_t	GetFlavorAt(int32 n, const flavor_info** out_info);

	BMediaNode*	InstantiateNodeFor(const flavor_info* info,
				BMessage* config, status_t* out_error);

	bool		WantsAutoStart();
	status_t	AutoStart(int index, BMediaNode** outNode,
				int32* outInternalID, bool* outHasMore);

protected:
	void		ScanFolder(const char* path);
	void		AddDevice(FireWireCard* card, const char* path);
	void		FreeDeviceList();

protected:
	BObjectList<device_info>	fDeviceList;
};

#endif
