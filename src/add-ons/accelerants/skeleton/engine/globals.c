/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.

	Other authors:
	Mark Watson,
	Rudolf Cornelissen 8/2004
*/

#include "std.h"

int fd;
shared_info *si;
area_id shared_info_area;
vuint32 *regs;
area_id regs_area;
display_mode *my_mode_list;
area_id	my_mode_list_area;
int accelerantIsClone;

eng_get_set_pci eng_pci_access=
	{
		SKEL_PRIVATE_DATA_MAGIC,
		0,
		4,
		0
	};

eng_in_out_isa eng_isa_access=
	{
		SKEL_PRIVATE_DATA_MAGIC,
		0,
		1,
		0
	};
