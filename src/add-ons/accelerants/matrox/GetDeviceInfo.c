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

	sprintf(adi->name, "Matrox chipset");
	switch (si->ps.card_type)
	{
	case MIL1:
		sprintf(adi->chipset, "Millennium I");
		break;
	case MYST:
		sprintf(adi->chipset, "Mystique");
		break;
	case MIL2:
		sprintf(adi->chipset, "Millennium II");
		break;
	case G100:
		sprintf(adi->chipset, "G100");
		break;
	case G200:
		sprintf(adi->chipset, "G200");
		break;
	case G400:
	case G400MAX:
		sprintf(adi->chipset, "G400");
		break;
	case G450:
		sprintf(adi->chipset, "G450");
		break;
	case G550:
		sprintf(adi->chipset, "G550");
		break;
	default:
		sprintf(adi->chipset, "unknown");
		break;
	}
	sprintf(adi->serial_no, "unknown");
	adi->memory = (si->ps.memory_size * 1024 * 1024);
	adi->dac_speed = si->ps.max_dac1_clock;

	return B_OK;
}
