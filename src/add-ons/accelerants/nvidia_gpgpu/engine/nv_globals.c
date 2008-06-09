/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.

	Other authors:
	Mark Watson,
	Rudolf Cornelissen 8/2004-5/2005
*/

#include "nv_std.h"

int fd;
shared_info *si;
area_id shared_info_area;
area_id dma_cmd_buf_area;
vuint32 *regs;
area_id regs_area;
display_mode *my_mode_list;
area_id	my_mode_list_area;
int accelerantIsClone;

nv_get_set_pci nv_pci_access=
	{
		NV_PRIVATE_DATA_MAGIC,
		0,
		4,
		0
	};

nv_in_out_isa nv_isa_access=
	{
		NV_PRIVATE_DATA_MAGIC,
		0,
		1,
		0
	};
