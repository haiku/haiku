/*
	Author:
	Rudolf Cornelissen 7/2004-11/2004
*/

#define MODULE_BIT 0x04000000

#include "acc_std.h"

/* Get some info about the device */
status_t GET_ACCELERANT_DEVICE_INFO(accelerant_device_info * adi)
{
	LOG(4,("GET_ACCELERANT_DEVICE_INFO: returning info\n"));

	/* no info on version is provided, so presumably this is for my info */
	adi->version = 1;

	sprintf(adi->name, "nVidia chipset");
	switch (si->ps.card_type)
	{
	case NV04:
		sprintf(adi->chipset, "NV04");
		break;
	case NV05:
		sprintf(adi->chipset, "NV05");
		break;
	case NV05M64:
		sprintf(adi->chipset, "NV05 model 64");
		break;
	case NV06:
		sprintf(adi->chipset, "NV06");
		break;
	case NV10:
		sprintf(adi->chipset, "NV10");
		break;
	case NV11:
	case NV11M:
		sprintf(adi->chipset, "NV11");
		break;
	case NV15:
		sprintf(adi->chipset, "NV15");
		break;
	case NV17:
	case NV17M:
		sprintf(adi->chipset, "NV17");
		break;
	case NV18:
	case NV18M:
		sprintf(adi->chipset, "NV18");
		break;
	case NV20:
		sprintf(adi->chipset, "NV20");
		break;
	case NV25:
		sprintf(adi->chipset, "NV25");
		break;
	case NV28:
		sprintf(adi->chipset, "NV28");
		break;
	case NV30:
		sprintf(adi->chipset, "NV30");
		break;
	case NV31:
		sprintf(adi->chipset, "NV31");
		break;
	case NV34:
		sprintf(adi->chipset, "NV34");
		break;
	case NV35:
		sprintf(adi->chipset, "NV35");
		break;
	case NV36:
		sprintf(adi->chipset, "NV36");
		break;
	case NV38:
		sprintf(adi->chipset, "NV38");
		break;
	case NV40:
		sprintf(adi->chipset, "NV40");
		break;
	case NV41:
		sprintf(adi->chipset, "NV41");
		break;
	case NV43:
		sprintf(adi->chipset, "NV43");
		break;
	case NV45:
		sprintf(adi->chipset, "NV45");
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
