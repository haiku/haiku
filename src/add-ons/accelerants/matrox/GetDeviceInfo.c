/*
	Author:
	Rudolf Cornelissen 1/2006
*/

#define MODULE_BIT 0x04000000

#include "acc_std.h"

/* Get some info about the device */
status_t GET_ACCELERANT_DEVICE_INFO(accelerant_device_info * adi) 
{
	LOG(4,("GET_ACCELERANT_DEVICE_INFO: returning info\n"));

	/* no info on version is provided, so presumably this is for my info */
	adi->version = 1;

	strcpy(adi->name, si->adi.name);
	strcpy(adi->chipset, si->adi.chipset);
	strcpy(adi->serial_no, "unknown");
	adi->memory = (si->ps.memory_size * 1024 * 1024);
	adi->dac_speed = si->ps.max_dac1_clock;

	return B_OK;
}
