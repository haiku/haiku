/*
	Author:
	Rudolf Cornelissen 7/2004-1/2016
*/

#define MODULE_BIT 0x04000000

#include "acc_std.h"

/* Get some info about the device */
status_t GET_ACCELERANT_DEVICE_INFO(accelerant_device_info * adi)
{
	LOG(4,("GET_ACCELERANT_DEVICE_INFO: returning info\n"));

	/* no info on version is provided, so presumably this is for my info */
	adi->version = 1;

	sprintf(adi->name, "VIA chipset");
	switch (si->ps.card_type)
	{
	case VT3022:
		sprintf(adi->chipset, "CLE266 Unichrome Pro (VT3022)");
		break;
	case VT3108:
		sprintf(adi->chipset, "K8M800 Unichrome Pro (VT3108)");
		break;
	case VT3122:
		sprintf(adi->chipset, "CLE266 Unichrome Pro (VT3122)");
		break;
	case VT3205:
		sprintf(adi->chipset, "KM400 Unichrome (VT3205)");
		break;
	case VT7205:
		sprintf(adi->chipset, "KM400 Unichrome (VT7205)");
		break;
	default:
		sprintf(adi->chipset, "unknown");
		break;
	}
	sprintf(adi->serial_no, "unknown");
	adi->memory = si->ps.memory_size;
	adi->dac_speed = si->ps.max_dac1_clock;

	return B_OK;
}
