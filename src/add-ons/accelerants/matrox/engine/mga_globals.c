/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.

	Other authors:
	Mark Watson
*/

#include "mga_std.h"

int fd;
shared_info *si;
area_id shared_info_area;
vuint32 *regs;
area_id regs_area;
display_mode *my_mode_list;
area_id	my_mode_list_area;
int accelerantIsClone;

gx00_get_set_pci gx00_pci_access=
	{
		GX00_PRIVATE_DATA_MAGIC,
		0,
		4,
		0
	};
