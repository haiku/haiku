/*
 * Copyright 2013, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <new>

#include <List.h>

#include "GPTDiskAddOn.h"


status_t
get_disk_system_add_ons(BList* addOns)
{
	GPTDiskAddOn* addOn = new(std::nothrow) GPTDiskAddOn();
	if (!addOns->AddItem(addOn)) {
		delete addOn;
		return B_NO_MEMORY;
	}

	return B_OK;
}
