/*
	Author:
	Rudolf Cornelissen 11/2004
*/

#define MODULE_BIT 0x04000000

#include "acc_std.h"

/* Get some info about the device */
status_t GET_ACCELERANT_DEVICE_INFO(accelerant_device_info * adi) 
{
	LOG(4,("GET_ACCELERANT_DEVICE_INFO: returning info\n"));

	/* no info on version is provided, so presumably this is for my info */
	adi->version = 1;

	sprintf(adi->name, "Neomagic chipset");
	switch (si->ps.card_type)
	{
	case NM2070:
		sprintf(adi->chipset, "MagicGraph NM2070");
		break;
	case NM2090:
		sprintf(adi->chipset, "MagicGraph NM2090");
		break;
	case NM2093:
		sprintf(adi->chipset, "MagicGraph NM2093");
		break;
	case NM2097:
		sprintf(adi->chipset, "MagicGraph NM2097");
		break;
	case NM2160:
		sprintf(adi->chipset, "MagicGraph NM2160");
		break;
	case NM2200:
		sprintf(adi->chipset, "MagicMedia NM2200");
		break;
	case NM2230:
		sprintf(adi->chipset, "MagicMedia NM2230");
		break;
	case NM2360:
		sprintf(adi->chipset, "MagicMedia NM2360");
		break;
	case NM2380:
		sprintf(adi->chipset, "MagicMedia NM2380");
		break;
	default:
		sprintf(adi->chipset, "unknown");
		break;
	}
	sprintf(adi->serial_no, "unknown");
	adi->memory = (si->ps.memory_size * 1024);
	adi->dac_speed = si->ps.max_dac1_clock;

	return B_OK;
}
