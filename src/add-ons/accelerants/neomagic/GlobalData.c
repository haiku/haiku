/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.

	Other authors:
	Mark Watson,
	Rudolf Cornelissen 4/2003-
*/

#include "acc_std.h"

int fd;
shared_info *si;
area_id shared_info_area;
vuint32 *regs, *regs2;
area_id regs_area, regs2_area;
display_mode *my_mode_list;
area_id	my_mode_list_area;
int accelerantIsClone;

mn_get_set_pci mn_pci_access=
	{
		MN_PRIVATE_DATA_MAGIC,
		0,
		4,
		0
	};

mn_in_out_isa mn_isa_access=
	{
		MN_PRIVATE_DATA_MAGIC,
		0,
		1,
		0
	};
