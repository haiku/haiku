/* Author:
   Rudolf Cornelissen 6/2004
*/

#define MODULE_BIT 0x00000100

#include <unistd.h>
#include "nv_std.h"

static void nv_agp_list_caps(agp_info ai);

status_t nv_agp_setup(void)
{
	char path[MAXPATHLEN];
	int agp_fd;
	agp_info ai_card, ai_bridge;
	uint8 rq_depth_card, rq_depth_bridge;

	/* check for card's AGP capabilities and list them */
	ai_card.config.agp_cap_id = CFGR(AGPREF);
	ai_card.config.agp_stat = CFGR(AGPSTAT);
	ai_card.config.agp_cmd = CFGR(AGPCMD);

	if ((ai_card.config.agp_cap_id & 0x00ff0000) && ((ai_card.config.agp_cap_id & AGP_id_mask) == AGP_id))
	{
		LOG(4,("AGP: graphicscard is AGP type, supporting specification %d.%d;\n",
			((ai_card.config.agp_cap_id & AGP_rev_major) >> AGP_rev_major_shift),
			((ai_card.config.agp_cap_id & AGP_rev_minor) >> AGP_rev_minor_shift)));

		nv_agp_list_caps(ai_card);

		/* check for motherboard AGP host bridge */
		/* open the BeOS AGP kernel driver, the permissions aren't important */
		strcpy(path, "/dev/graphics/agp/1");
		agp_fd = open(path, B_READ_WRITE);
		if (agp_fd < 0)
		{
			LOG(4,("AGP: cannot open AGP host bridge driver, aborting!\n"));
			/* program card for PCI access */
			CFGW(AGPCMD, 0x00000000);

			return B_ERROR;
		}

		/* get host bridge info */
		ioctl(agp_fd, GET_CONFIG, &ai_bridge); 

		LOG(4,("AGP: AGP host bridge found, vendorID $%04x, deviceID $%04x\n",
			ai_bridge.dev.vendor_id, ai_bridge.dev.device_id));
		if (ai_bridge.status != B_OK)
		{
			LOG(4,("AGP: host bridge failed to respond correctly, aborting AGP setup!\n"));
			/* close host bridge driver */
			close(agp_fd);
			/* program card for PCI access */
			CFGW(AGPCMD, 0x00000000);

			return B_ERROR;
		}

		/* list host bridge capabilities */
		LOG(4,("AGP: host bridge supports specification %d.%d;\n",
			((ai_bridge.config.agp_cap_id & AGP_rev_major) >> AGP_rev_major_shift),
			((ai_bridge.config.agp_cap_id & AGP_rev_minor) >> AGP_rev_minor_shift)));
		nv_agp_list_caps(ai_bridge);

		/* abort if specified by user in nv.settings */
		if (si->settings.force_pci)
		{
			/* user specified not to use AGP */
			LOG(4,("AGP: forcing PCI mode (specified in nv.settings).\n"));
			/* close host bridge driver */
			close(agp_fd);
			/* program card for PCI access */
			CFGW(AGPCMD, 0x00000000);

			return B_OK;
		}

		/* find out shared AGP capabilities of card and host bridge */
		if ((ai_card.config.agp_stat & AGP_rate_rev) !=
			(ai_bridge.config.agp_stat & AGP_rate_rev))
		{
			LOG(4,("AGP: compatibility problem detected, aborting AGP setup!\n"));
			/* close host bridge driver */
			close(agp_fd);
			/* program card for PCI access */
			CFGW(AGPCMD, 0x00000000);

			return B_ERROR;
		}

		/* both card and bridge are set to the same standard */
		/* (which is as it should be!) */
		LOG(4,("AGP: enabling AGP\n"));

		/* select highest AGP mode */
		if (!(ai_bridge.config.agp_stat & AGP_rate_rev))
		{
			/* AGP 2.0 scheme applies */
			if ((ai_card.config.agp_stat & AGP_2_4x) &&
				(ai_bridge.config.agp_stat & AGP_2_4x))
			{
				LOG(4,("AGP: using AGP 2.0 4x mode\n"));
				ai_card.config.agp_cmd = AGP_2_4x;
				ai_bridge.config.agp_cmd = AGP_2_4x;
			}
			else
			{
				if ((ai_card.config.agp_stat & AGP_2_2x) &&
					(ai_bridge.config.agp_stat & AGP_2_2x))
				{
					LOG(4,("AGP: using AGP 2.0 2x mode\n"));
					ai_card.config.agp_cmd = AGP_2_2x;
					ai_bridge.config.agp_cmd = AGP_2_2x;
				}
				else
				{
					LOG(4,("AGP: using AGP 2.0 1x mode\n"));
					ai_card.config.agp_cmd = AGP_2_1x;
					ai_bridge.config.agp_cmd = AGP_2_1x;
				}
			}
		}
		else
		{
			/* AGP 3.0 scheme applies */
			if ((ai_card.config.agp_stat & AGP_3_8x) &&
				(ai_bridge.config.agp_stat & AGP_3_8x))
			{
				LOG(4,("AGP: using AGP 3.0 8x mode\n"));
				ai_card.config.agp_cmd = AGP_3_8x;
				ai_bridge.config.agp_cmd = AGP_3_8x;
			}
			else
			{
				LOG(4,("AGP: using AGP 3.0 4x mode\n"));
				ai_card.config.agp_cmd = AGP_3_4x;
				ai_bridge.config.agp_cmd = AGP_3_4x;
			}
		}

		/* activate sideband adressing if possible */
		if ((ai_card.config.agp_stat & AGP_SBA) &&
			(ai_bridge.config.agp_stat & AGP_SBA))
		{
			LOG(4,("AGP: enabling sideband adressing (SBA)\n"));
			ai_card.config.agp_cmd |= AGP_SBA;
			ai_bridge.config.agp_cmd |= AGP_SBA;
		}

		/* activate fast writes if possible */
		if ((ai_card.config.agp_stat & AGP_FW) &&
			(ai_bridge.config.agp_stat & AGP_FW))
		{
			LOG(4,("AGP: enabling fast writes (FW)\n"));
			ai_card.config.agp_cmd |= AGP_FW;
			ai_bridge.config.agp_cmd |= AGP_FW;
		}

		/* setup maximum request depth supported */
		/* note:
		 * this is writable only in the graphics card */
		rq_depth_card = ((ai_card.config.agp_stat & AGP_RQ) >> AGP_RQ_shift);
		rq_depth_bridge = ((ai_bridge.config.agp_stat & AGP_RQ) >> AGP_RQ_shift);
		if (rq_depth_card < rq_depth_bridge)
		{
			ai_card.config.agp_cmd |= (rq_depth_card << AGP_RQ_shift);
			LOG(4,("AGP: max. AGP queued request depth will be set to %d\n",
				(rq_depth_card + 1)));
		}
		else
		{
			ai_card.config.agp_cmd |= (rq_depth_bridge << AGP_RQ_shift);
			LOG(4,("AGP: max. AGP queued request depth will be set to %d\n",
				(rq_depth_bridge + 1)));
		}

		/* set the enable AGP bits */
		/* note:
		 * the AGP standard defines that this bit may be written to the AGPCMD
		 * registers simultanously with the other bits if a single 32bit write
		 * to each register is used. */
		ai_card.config.agp_cmd |= AGP_enable;
		ai_bridge.config.agp_cmd |= AGP_enable;

		/* finally program both the host bridge and the graphics card! */
		/* note:
		 * the AGP standard defines that the host bridge should be enabled first. */
		ioctl(agp_fd, SET_CONFIG, &ai_bridge);
		CFGW(AGPCMD, ai_card.config.agp_cmd);

		LOG(4,("AGP: graphics card AGPCMD register readback $%08x\n", CFGR(AGPCMD)));

		/* close host bridge driver */
		close(agp_fd);
	}
	else
	{
		LOG(4,("AGP: graphicscard is PCI type.\n"));
	}
}

static void nv_agp_list_caps(agp_info ai)
{
	/*
		list capabilities
	 */
	/* the mainboard and graphicscard determine AGP version used on power-up/reset */
	if (!(ai.config.agp_stat & AGP_rate_rev))
	{
		/* AGP 2.0 scheme applies */
		if (ai.config.agp_stat & AGP_2_1x)
		{
			LOG(4,("AGP: AGP 2.0 1x mode is available\n"));
		}
		if (ai.config.agp_stat & AGP_2_2x)
		{
			LOG(4,("AGP: AGP 2.0 2x mode is available\n"));
		}
		if (ai.config.agp_stat & AGP_2_4x)
		{
			LOG(4,("AGP: AGP 2.0 4x mode is available\n"));
		}
	}
	else
	{
		/* AGP 3.0 scheme applies */
		if (ai.config.agp_stat & AGP_3_4x)
		{
			LOG(4,("AGP: AGP 3.0 4x mode is available\n"));
		}
		if (ai.config.agp_stat & AGP_3_8x)
			{
			LOG(4,("AGP: AGP 3.0 8x mode is available\n"));
		}
	}
	if (ai.config.agp_stat & AGP_FW) LOG(4,("AGP: fastwrite transfers are supported\n"));
	if (ai.config.agp_stat & AGP_SBA) LOG(4,("AGP: sideband adressing is supported\n"));
	LOG(4,("AGP: %d queued AGP requests can be handled.\n",
		(((ai.config.agp_stat & AGP_RQ) >> AGP_RQ_shift) + 1)));

	/*
		list current settings
	 */
	LOG(4,("AGP: listing current active settings:\n"));
	/* the mainboard and graphicscard determine AGP version used on power-up/reset */
	if (!(ai.config.agp_stat & AGP_rate_rev))
	{
		/* AGP 2.0 scheme applies */
		if (ai.config.agp_cmd & AGP_2_1x)
		{
			LOG(4,("AGP: AGP 2.0 1x mode is set\n"));
		}
		if (ai.config.agp_cmd & AGP_2_2x)
		{
			LOG(4,("AGP: AGP 2.0 2x mode is set\n"));
		}
		if (ai.config.agp_cmd & AGP_2_4x)
		{
			LOG(4,("AGP: AGP 2.0 4x mode is set\n"));
		}
	}
	else
	{
		/* AGP 3.0 scheme applies */
		if (ai.config.agp_cmd & AGP_3_4x)
		{
			LOG(4,("AGP: AGP 3.0 4x mode is set\n"));
		}
		if (ai.config.agp_cmd & AGP_3_8x)
			{
			LOG(4,("AGP: AGP 3.0 8x mode is set\n"));
		}
	}
	if (ai.config.agp_cmd & AGP_FW) LOG(4,("AGP: fastwrite transfers are enabled\n"));
	if (ai.config.agp_cmd & AGP_SBA) LOG(4,("AGP: sideband adressing is enabled\n"));
	LOG(4,("AGP: max. AGP queued request depth is set to %d\n",
		(((ai.config.agp_cmd & AGP_RQ) >> AGP_RQ_shift) + 1)));
	if (ai.config.agp_cmd & AGP_enable)
		LOG(4,("AGP: this AGP interface is currently enabled.\n"));
	else
		LOG(4,("AGP: this AGP interface is currently disabled.\n"));
}
