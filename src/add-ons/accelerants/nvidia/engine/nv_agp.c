/* Author:
   Rudolf Cornelissen 6/2004-7/2004
*/

#define MODULE_BIT 0x00000100

#include <unistd.h>
#include "nv_std.h"

static void nv_agp_list_info(agp_info ai);
static bool zooi(void);

status_t nv_agp_setup(void)
{
	nv_nth_agp_info nai;
	uint8 index;
	bool agp = false;

	/* first try to enable FW support on our card if user requested this
	 * ('unsupported' tweak!)
	 * This has no effect on PCI cards. */
	if (si->settings.unhide_fw)
	{
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
	nai.magic = NV_PRIVATE_DATA_MAGIC;

	/* contact driver and get a pointer to the registers and shared data */
	for (index = 0; index < 8; index++)
	{
		/* get nth AGP device info */
		nai.index = index;
		ioctl(fd, NV_GET_NTH_AGP_INFO, &nai, sizeof(nai));
		/* exit if we didn't get one */
		if (!nai.exist)
		{
			if (index != 0)
				LOG(4,("AGP: end of AGP capable devices list.\n"));
			else
				LOG(4,("AGP: no AGP busmanager or no AGP capable devices found.\n"));
			break;
		}

		LOG(4,("AGP: AGP capable device #%d:\n", (index + 1)));

		/* see if we are this one */
		if (((((uint32)(nai.agpi.device_id)) << 16) | nai.agpi.vendor_id) == CFGR(DEVID))
		{
			LOG(4,("AGP: (this is the device this accelerant controls)\n"));
			agp = true;
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
	if (!agp)
	{
		LOG(4,("AGP: assuming the graphicscard this accelerant controls is PCI type.\n"));

		/* make sure the card is in PCI mode if it's an AGP type after all */
		CFGW(AGPCMD, 0x00000000);

		return B_ERROR;
	}

}

static bool zooi(void)
{
	/* get host bridge info */
//	ioctl(agp_fd, GET_CONFIG, &ai_bridge); 

//	LOG(4,("AGP: AGP host bridge found, vendorID $%04x, deviceID $%04x\n",
//		ai_bridge.dev.vendor_id, ai_bridge.dev.device_id));
//	if (ai_bridge.status != B_OK)
	{
		LOG(4,("AGP: host bridge failed to respond correctly, aborting AGP setup!\n"));
		/* close host bridge driver */
//		close(agp_fd);
		/* program card for PCI access */
//		PCI_CFGW((adress + 8), 0x00000000);

		return B_ERROR;
	}

	/* list host bridge capabilities */
//	LOG(4,("AGP: host bridge supports specification %d.%d;\n",
//		((ai_bridge.config.agp_cap_id & AGP_rev_major) >> AGP_rev_major_shift),
//		((ai_bridge.config.agp_cap_id & AGP_rev_minor) >> AGP_rev_minor_shift)));
//	nv_agp_list_caps(ai_bridge);

	/* abort if specified by user in nv.settings */
	if (si->settings.force_pci)
	{
		/* user specified not to use AGP */
		LOG(4,("AGP: forcing PCI mode (specified in nv.settings).\n"));
		/* close host bridge driver */
//		close(agp_fd);
		/* program card for PCI access */
//		PCI_CFGW((adress + 8), 0x00000000);

		return B_ERROR;
	}

	/* find out if AGP modes are supported: devices exist that have the AGP style 
	 * connector with AGP style registers, but not the features!
	 * (confirmed Matrox Millenium II AGP for instance) */
//	if (!(ai_card.config.agp_stat & AGP_rates) ||
//		!(ai_bridge.config.agp_stat & AGP_rates))
	{
		LOG(4,("AGP: no AGP modes possible, aborting AGP setup!\n"));
		/* close host bridge driver */
//		close(agp_fd);
		/* program card for PCI access */
//		PCI_CFGW((adress + 8), 0x00000000);

		return B_ERROR;
	}

	/* find out shared AGP capabilities of card and host bridge */
//	if ((ai_card.config.agp_stat & AGP_rate_rev) !=
//		(ai_bridge.config.agp_stat & AGP_rate_rev))
	{
		LOG(4,("AGP: compatibility problem detected, aborting AGP setup!\n"));
		/* close host bridge driver */
//		close(agp_fd);
		/* program card for PCI access */
//		PCI_CFGW((adress + 8), 0x00000000);

		return B_ERROR;
	}

	/* both card and bridge are set to the same standard */
	/* (which is as it should be!) */
	LOG(4,("AGP: enabling AGP\n"));

	/* select highest AGP mode */
//	if (!(ai_bridge.config.agp_stat & AGP_rate_rev))
	{
		/* AGP 2.0 scheme applies */
//		if ((ai_card.config.agp_stat & AGP_2_4x) &&
//			(ai_bridge.config.agp_stat & AGP_2_4x))
		{
			LOG(4,("AGP: using AGP 2.0 4x mode\n"));
//			ai_card.config.agp_cmd = AGP_2_4x;
//			ai_bridge.config.agp_cmd = AGP_2_4x;
		}
//		else
		{
//			if ((ai_card.config.agp_stat & AGP_2_2x) &&
//				(ai_bridge.config.agp_stat & AGP_2_2x))
			{
				LOG(4,("AGP: using AGP 2.0 2x mode\n"));
//				ai_card.config.agp_cmd = AGP_2_2x;
//				ai_bridge.config.agp_cmd = AGP_2_2x;
			}
//			else
			{
				LOG(4,("AGP: using AGP 2.0 1x mode\n"));
//				ai_card.config.agp_cmd = AGP_2_1x;
//				ai_bridge.config.agp_cmd = AGP_2_1x;
			}
		}
	}
//	else
	{
		/* AGP 3.0 scheme applies */
//		if ((ai_card.config.agp_stat & AGP_3_8x) &&
//			(ai_bridge.config.agp_stat & AGP_3_8x))
		{
			LOG(4,("AGP: using AGP 3.0 8x mode\n"));
//			ai_card.config.agp_cmd = AGP_3_8x;
//			ai_bridge.config.agp_cmd = AGP_3_8x;
		}
//		else
		{
			LOG(4,("AGP: using AGP 3.0 4x mode\n"));
//			ai_card.config.agp_cmd = AGP_3_4x;
//			ai_bridge.config.agp_cmd = AGP_3_4x;
		}
	}

	/* activate sideband adressing if possible */
//	if ((ai_card.config.agp_stat & AGP_SBA) &&
//		(ai_bridge.config.agp_stat & AGP_SBA))
	{
		LOG(4,("AGP: enabling sideband adressing (SBA)\n"));
//		ai_card.config.agp_cmd |= AGP_SBA;
//		ai_bridge.config.agp_cmd |= AGP_SBA;
	}

	/* activate fast writes if possible */
//	if ((ai_card.config.agp_stat & AGP_FW) &&
//		(ai_bridge.config.agp_stat & AGP_FW))
	{
		LOG(4,("AGP: enabling fast writes (FW)\n"));
//		ai_card.config.agp_cmd |= AGP_FW;
//		ai_bridge.config.agp_cmd |= AGP_FW;
	}

	/* setup maximum request depth supported */
	/* note:
	 * this is writable only in the graphics card */
//	rq_depth_card = ((ai_card.config.agp_stat & AGP_RQ) >> AGP_RQ_shift);
//	rq_depth_bridge = ((ai_bridge.config.agp_stat & AGP_RQ) >> AGP_RQ_shift);
//	if (rq_depth_card < rq_depth_bridge)
	{
//		ai_card.config.agp_cmd |= (rq_depth_card << AGP_RQ_shift);
//		LOG(4,("AGP: max. AGP queued request depth will be set to %d\n",
//			(rq_depth_card + 1)));
	}
//	else
	{
//		ai_card.config.agp_cmd |= (rq_depth_bridge << AGP_RQ_shift);
//		LOG(4,("AGP: max. AGP queued request depth will be set to %d\n",
//			(rq_depth_bridge + 1)));
	}

	/* set the enable AGP bits */
	/* note:
	 * the AGP standard defines that this bit may be written to the AGPCMD
	 * registers simultanously with the other bits if a single 32bit write
	 * to each register is used. */
//	ai_card.config.agp_cmd |= AGP_enable;
//	ai_bridge.config.agp_cmd |= AGP_enable;

	/* finally program both the host bridge and the graphics card! */
	/* note:
	 * the AGP standard defines that the host bridge should be enabled first. */
//	ioctl(agp_fd, SET_CONFIG, &ai_bridge);
//	PCI_CFGW((adress + 8), ai_card.config.agp_cmd);

	LOG(4,("AGP: graphics card AGPCMD register readback $%08x\n", CFGR(AGPCMD)));

	/* close host bridge driver */
//	close(agp_fd);

	return B_OK;
}

static void nv_agp_list_info(agp_info ai)
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
		((ai.interface.agp_cap_id & AGP_rev_major) >> AGP_rev_major_shift),
		((ai.interface.agp_cap_id & AGP_rev_minor) >> AGP_rev_minor_shift)));

	/* the AGP devices determine AGP speed scheme version used on power-up/reset */
	if (!(ai.interface.agp_stat & AGP_rate_rev))
	{
		/* AGP 2.0 scheme applies */
		if (ai.interface.agp_stat & AGP_2_1x)
			LOG(4,("AGP: AGP 2.0 1x mode is available\n"));
		if (ai.interface.agp_stat & AGP_2_2x)
			LOG(4,("AGP: AGP 2.0 2x mode is available\n"));
		if (ai.interface.agp_stat & AGP_2_4x)
			LOG(4,("AGP: AGP 2.0 4x mode is available\n"));
	}
	else
	{
		/* AGP 3.0 scheme applies */
		if (ai.interface.agp_stat & AGP_3_4x)
			LOG(4,("AGP: AGP 3.0 4x mode is available\n"));
		if (ai.interface.agp_stat & AGP_3_8x)
			LOG(4,("AGP: AGP 3.0 8x mode is available\n"));
	}
	if (ai.interface.agp_stat & AGP_FW) LOG(4,("AGP: fastwrite transfers are supported\n"));
	if (ai.interface.agp_stat & AGP_SBA) LOG(4,("AGP: sideband adressing is supported\n"));
	LOG(4,("AGP: %d queued AGP requests can be handled.\n",
		(((ai.interface.agp_stat & AGP_RQ) >> AGP_RQ_shift) + 1)));

	/*
		list current settings
	 */
	LOG(4,("AGP: listing current active settings:\n"));
	if (!(ai.interface.agp_stat & AGP_rate_rev))
	{
		/* AGP 2.0 scheme applies */
		if (ai.interface.agp_cmd & AGP_2_1x)
			LOG(4,("AGP: AGP 2.0 1x mode is set\n"));
		if (ai.interface.agp_cmd & AGP_2_2x)
			LOG(4,("AGP: AGP 2.0 2x mode is set\n"));
		if (ai.interface.agp_cmd & AGP_2_4x)
			LOG(4,("AGP: AGP 2.0 4x mode is set\n"));
	}
	else
	{
		/* AGP 3.0 scheme applies */
		if (ai.interface.agp_cmd & AGP_3_4x)
			LOG(4,("AGP: AGP 3.0 4x mode is set\n"));
		if (ai.interface.agp_cmd & AGP_3_8x)
			LOG(4,("AGP: AGP 3.0 8x mode is set\n"));
	}
	if (ai.interface.agp_cmd & AGP_FW) LOG(4,("AGP: fastwrite transfers are enabled\n"));
	if (ai.interface.agp_cmd & AGP_SBA) LOG(4,("AGP: sideband adressing is enabled\n"));
	LOG(4,("AGP: max. AGP queued request depth is set to %d\n",
		(((ai.interface.agp_cmd & AGP_RQ) >> AGP_RQ_shift) + 1)));
	if (ai.interface.agp_cmd & AGP_enable)
		LOG(4,("AGP: this AGP interface is currently enabled.\n"));
	else
		LOG(4,("AGP: this AGP interface is currently disabled.\n"));
}
