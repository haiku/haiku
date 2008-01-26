/* Author:
   Rudolf Cornelissen 6/2004-9/2004
*/

#define MODULE_BIT 0x00000100

#include <unistd.h>
#include "std.h"


status_t
eng_agp_setup(void)
{
	eng_nth_agp_info nai;
	eng_cmd_agp nca;
	uint8 index;
	agp_info eng_ai;
	bool agp = false;

	/* set the magic number so the via kerneldriver knows we're for real */
	nca.magic = nai.magic = VIA_PRIVATE_DATA_MAGIC;

	/* contact driver and get a pointer to the registers and shared data */
	for (index = 0; index < 8; index++) {
		/* get nth AGP device info */
		nai.index = index;
		ioctl(fd, ENG_GET_NTH_AGP_INFO, &nai, sizeof(nai));

		/* abort if no agp busmanager found */
		if (!nai.agp_bus) {
			LOG(4,("AGP: no AGP busmanager found.\n"));
			/* don't touch AGP command register, we don't know what has been setup:
			 * touching it anyway might 'hang' the graphics card! */

			return B_ERROR;
		}

		/* exit if we didn't get device info for this index */
		if (!nai.exist) {
			if (index != 0)
				LOG(4,("AGP: end of AGP capable devices list.\n"));
			else
				LOG(4,("AGP: no AGP capable devices found.\n"));
			break;
		}

		LOG(4,("AGP: AGP capable device #%d:\n", (index + 1)));

		/* see if we are this one */
		if (nai.agpi.device_id == si->device_id
			&& nai.agpi.vendor_id == si->vendor_id
			&& nai.agpi.bus == si->bus
			&& nai.agpi.device == si->device
			&& nai.agpi.function == si->function) {
			LOG(4,("AGP: (this is the device this accelerant controls)\n"));
			agp = true;
			/* remember our info */
			eng_ai = nai.agpi;
		}
	}

	/* if our card is not an AGP type, abort here */
	/* Note:
	 * We have to iterate through the capability list as specified in the PCI spec
	 * one way or the other, otherwise we cannot distinquish between PCI and
	 * AGP type cards as PCI cards still might have AGP registers that pretend to
	 * support AGP.
	 * We rely on the AGP busmanager to iterate trough this list for us. */
	if (!agp) {
		LOG(4,("AGP: the graphicscard this accelerant controls is PCI type.\n"));

		/* make sure card is set for PCI access */
//		CFGW(AGPCMD, 0x00000000);

		return B_ERROR;
	}

	if (si->settings.force_pci) {
		/* set PCI mode if specified by user in skel.settings */
		LOG(4,("AGP: forcing PCI mode (specified in via.settings)\n"));

		/* let the AGP busmanager setup PCI mode.
		 * (the AGP speed scheme is of no consequence now) */
		nca.cmd = 0x00000000;
		ioctl(fd, ENG_ENABLE_AGP, &nca, sizeof(nca));
	} else {
		/* activate AGP mode */
		LOG(4,("AGP: activating AGP mode...\n"));

		/* let the AGP busmanager worry about what mode to set.. */
		nca.cmd = 0xfffffff7;
		/* ..but we do need to select the right speed scheme fetched from our card */
		if (eng_ai.interface.status & AGP_3_MODE)
			nca.cmd |= AGP_3_MODE;
		ioctl(fd, ENG_ENABLE_AGP, &nca, sizeof(nca));
	}

	/* extra check */
//	LOG(4,("AGP: graphics card AGPCMD register readback $%08x\n", CFGR(AGPCMD)));
	return B_OK;
}

