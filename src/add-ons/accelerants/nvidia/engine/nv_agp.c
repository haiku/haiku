/* Author:
   Rudolf Cornelissen 6/2004-4/2006
*/

#define MODULE_BIT 0x00000100

#include <unistd.h>
#include "nv_std.h"

static void nv_agp_list_info(agp_info ai);
static void nv_agp_list_active(uint32 cmd);


status_t
nv_agp_setup(bool enable_agp)
{
	nv_nth_agp_info nai;
	nv_cmd_agp nca;
	uint8 index;
	agp_info nv_ai;
	bool agp = false;

	/* preset we are running in PCI mode: so acc engine may not use AGP transfers */
	si->engine.agp_mode = false;

	/* first try to enable FW support on our card if user requested this
	 * ('unsupported' tweak!)
	 * This has no effect on PCI cards. */
	if (si->settings.unhide_fw) {
		uint32 reg;

		LOG(4, ("AGP: STRAPINFO2 contains $%08x\n", NV_REG32(NV32_NVSTRAPINFO2)));

		LOG(4, ("AGP: attempting to enable fastwrite support..\n"));
		/* 'force' FW support */
		reg = (NV_REG32(NV32_NVSTRAPINFO2) & ~0x00000800);
		/* enable strapinfo overwrite */
		NV_REG32(NV32_NVSTRAPINFO2) = (reg | 0x80000000);

		LOG(4, ("AGP: STRAPINFO2 now contains $%08x\n", NV_REG32(NV32_NVSTRAPINFO2)));
	}

	/* set the magic number so the nvidia kerneldriver knows we're for real */
	nca.magic = nai.magic = NV_PRIVATE_DATA_MAGIC;

	/* contact driver and get a pointer to the registers and shared data */
	for (index = 0; index < 8; index++) {
		/* get nth AGP device info */
		nai.index = index;
		ioctl(fd, NV_GET_NTH_AGP_INFO, &nai, sizeof(nai));

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
			nv_ai = nai.agpi;
		}

		/* log capabilities */
		nv_agp_list_info(nai.agpi);
	}

	/* if our card is not an AGP type, abort here */
	/* Note:
	 * We have to iterate through the capability list as specified in the PCI spec
	 * one way or the other, otherwise we cannot distinquish between nVidia PCI and
	 * AGP type cards as nVidia PCI cards still have AGP registers that pretend to
	 * support AGP.
	 * We rely on the AGP busmanager to iterate trough this list for us. */
	if (!agp) {
		LOG(4,("AGP: the graphicscard this accelerant controls is PCI type.\n"));

		/* make sure card is set for PCI access */
		CFGW(AGPCMD, 0x00000000);

		return B_ERROR;
	}

	if (si->settings.force_pci || !enable_agp) {
		/* set PCI mode if specified by user in nv.settings */
		if (enable_agp)
			LOG(4,("AGP: forcing PCI mode (specified in nv.settings)\n"));
		else
			LOG(4,("AGP: forcing PCI mode during coldstart (required)\n"));

		/* let the AGP busmanager setup PCI mode.
		 * (the AGP speed scheme is of no consequence now) */
		nca.cmd = 0x00000000;
		ioctl(fd, NV_ENABLE_AGP, &nca, sizeof(nca));
	} else {
		/* activate AGP mode */
		LOG(4,("AGP: activating AGP mode...\n"));

		/* let the AGP busmanager worry about what mode to set.. */
		nca.cmd = 0xfffffff7;
		/* ..but we do need to select the right speed scheme fetched from our card */
		if (nv_ai.interface.status & AGP_3_MODE)
			nca.cmd |= AGP_3_MODE;
		ioctl(fd, NV_ENABLE_AGP, &nca, sizeof(nca));
		/* tell the engine in may use AGP transfers if AGP is up and running */
		if (nca.cmd & AGP_ENABLE)
			si->engine.agp_mode = true;
	}

	/* list mode now activated,
	 * make sure we have the correct speed scheme for logging */
	nv_agp_list_active(nca.cmd | (nv_ai.interface.status & AGP_3_MODE));

	/* extra check */
	LOG(4,("AGP: graphics card AGPCMD register readback $%08x\n", CFGR(AGPCMD)));
	return B_OK;
}


static void
nv_agp_list_info(agp_info ai)
{
	/*
		list device
	*/
	if (ai.class_base == PCI_display)
		LOG(4,("AGP: device is a graphicscard, subclass ID is $%02x\n", ai.class_sub));
	else
		LOG(4,("AGP: device is a hostbridge, subclass ID is $%02x\n", ai.class_sub));
	LOG(4,("AGP: vendor ID $%04x\n", ai.vendor_id));
	LOG(4,("AGP: device ID $%04x\n", ai.device_id));
	LOG(4,("AGP: bus %d, device %d, function %d\n", ai.bus, ai.device, ai.function));

	/*
		list capabilities
	*/
	LOG(4,("AGP: this device supports AGP specification %d.%d;\n",
		((ai.interface.capability_id & AGP_REV_MAJOR) >> AGP_REV_MAJOR_SHIFT),
		((ai.interface.capability_id & AGP_REV_MINOR) >> AGP_REV_MINOR_SHIFT)));

	/* the AGP devices determine AGP speed scheme version used on power-up/reset */
	if (!(ai.interface.status & AGP_3_MODE)) {
		/* AGP 2.0 scheme applies */
		if (ai.interface.status & AGP_2_1x)
			LOG(4,("AGP: AGP 2.0 1x mode is available\n"));
		if (ai.interface.status & AGP_2_2x)
			LOG(4,("AGP: AGP 2.0 2x mode is available\n"));
		if (ai.interface.status & AGP_2_4x)
			LOG(4,("AGP: AGP 2.0 4x mode is available\n"));
	} else {
		/* AGP 3.0 scheme applies */
		if (ai.interface.status & AGP_3_4x)
			LOG(4,("AGP: AGP 3.0 4x mode is available\n"));
		if (ai.interface.status & AGP_3_8x)
			LOG(4,("AGP: AGP 3.0 8x mode is available\n"));
	}
	if (ai.interface.status & AGP_FAST_WRITE)
		LOG(4,("AGP: fastwrite transfers are supported\n"));
	if (ai.interface.status & AGP_SBA)
		LOG(4,("AGP: sideband adressing is supported\n"));
	LOG(4,("AGP: %d queued AGP requests can be handled.\n",
		(((ai.interface.status & AGP_REQUEST) >> AGP_REQUEST_SHIFT) + 1)));

	/*
		list current settings,
		make sure we have the correct speed scheme for logging
	 */
	nv_agp_list_active(ai.interface.command
		| (ai.interface.status & AGP_3_MODE));
}


static void
nv_agp_list_active(uint32 cmd)
{
	LOG(4,("AGP: listing settings now in use:\n"));
	if (!(cmd & AGP_3_MODE)) {
		/* AGP 2.0 scheme applies */
		if (cmd & AGP_2_1x)
			LOG(4,("AGP: AGP 2.0 1x mode is set\n"));
		if (cmd & AGP_2_2x)
			LOG(4,("AGP: AGP 2.0 2x mode is set\n"));
		if (cmd & AGP_2_4x)
			LOG(4,("AGP: AGP 2.0 4x mode is set\n"));
	} else {
		/* AGP 3.0 scheme applies */
		if (cmd & AGP_3_4x)
			LOG(4,("AGP: AGP 3.0 4x mode is set\n"));
		if (cmd & AGP_3_8x)
			LOG(4,("AGP: AGP 3.0 8x mode is set\n"));
	}
	if (cmd & AGP_FAST_WRITE)
		LOG(4,("AGP: fastwrite transfers are enabled\n"));
	if (cmd & AGP_SBA)
		LOG(4,("AGP: sideband adressing is enabled\n"));
	LOG(4,("AGP: max. AGP queued request depth is set to %d\n",
		(((cmd & AGP_REQUEST) >> AGP_REQUEST_SHIFT) + 1)));
	if (cmd & AGP_ENABLE)
		LOG(4,("AGP: the AGP interface is enabled.\n"));
	else
		LOG(4,("AGP: the AGP interface is disabled.\n"));
}
