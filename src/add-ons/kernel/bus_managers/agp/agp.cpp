/*
	written by Rudolf Cornelissen 7/2004
*/

/*
	Notes:
	-currently we just setup all found devices with AGP interface to the same highest common mode,
	we don't distinquish different AGP buses.

	-no aperture and GART support, just enabling AGP transfer mode. Aperture and GART support are
	probably easiest to setup for AGP3, as this version of the standard defines all registers needed
	so we can probably program in the same universal way as we are doing right now. AGP2 requires
	specific 'drivers' for each chipset outthere, as the registers are implemented in a lot of
	different ways here.
	Aperture and GART support is only usefull, once we setup hardware 3D acceleration.

	-AGP3 defines 'asynchronous request size' and 'calibration cycle' fields in the status and command
	registers. Currently programming zero's which will make it work, although further optimisation
	is possible.

	-AGP3.5 also defines isochronous transfers which are not implemented here: the hardware keeps them
	disabled by default.
*/

#include <malloc.h>
#include <driver_settings.h>
#include <stdlib.h> // for strtoXX
#include "agp.h"

typedef struct {
	sem_id	sem;
	int32	ben;
} benaphore;

#define INIT_BEN(x)		x.sem = create_sem(0, "AGP "#x" benaphore");  x.ben = 0;
#define AQUIRE_BEN(x)	if((atomic_add(&(x.ben), 1)) >= 1) acquire_sem(x.sem);
#define RELEASE_BEN(x)	if((atomic_add(&(x.ben), -1)) > 1) release_sem(x.sem);
#define	DELETE_BEN(x)	delete_sem(x.sem);

#define MAX_DEVICES	  8

/* read and write to PCI config space */
#define get_pci(o, s) (*pci_bus->read_pci_config)(pcii->bus, pcii->device, pcii->function, (o), (s))
#define set_pci(o, s, v) (*pci_bus->write_pci_config)(pcii->bus, pcii->device, pcii->function, (o), (s), (v))

/* these structures are private to the module */
typedef struct device_info device_info;

struct device_info {
	uint8		agp_adress;				/* location of AGP interface in PCI capabilities list */
	agp_info	agpi;					/* agp info for this device */
	pci_info	pcii;					/* pci info for this device */
};

typedef struct {
	long		count;							/* number of devices actually found */
	benaphore	kernel;							/* for serializing access */
	device_info	di[MAX_DEVICES];				/* device specific stuff */
} DeviceData;

typedef struct settings { //see comments in agp.settings
	uint32 max_speed;
	bool   block_agp;
	bool   block_sba;
	bool   block_fw;
} settings;

/* prototypes for our private functions */
static bool has_AGP_interface(pci_info *pcii, uint8 *adress);
static status_t probe_devices(void);
static void read_settings(void);
static void check_settings(uint32 *command);
static void check_capabilities(uint32 agp_stat, uint32 *command);


static DeviceData		*pd;
static pci_module_info	*pci_bus;

static settings current_settings = { //see comments in agp.settings
	0,          // max_speed
	false,		// block_agp
	false,      // block_sba
	false,      // block_fw
};


status_t init(void)
{
	/* get a handle for the pci bus */
	if (get_module(B_PCI_MODULE_NAME, (module_info **)&pci_bus) != B_OK)
		return B_ERROR;

	/* private data */
	pd = (DeviceData *)calloc(1, sizeof(DeviceData));
	if (!pd)
	{
		put_module(B_PCI_MODULE_NAME);
		return B_ERROR;
	}
	/* initialize the benaphore */
	INIT_BEN(pd->kernel);

	/* find all of our supported devices, if any */
	if (probe_devices() != B_OK)
	{
		/* free the driver data */
		DELETE_BEN(pd->kernel);
		free(pd);
		pd = NULL;

		put_module(B_PCI_MODULE_NAME);
		return B_ERROR;
	}

	/* parse settings file if there */
	read_settings();

	return B_OK;
}

static status_t probe_devices(void)
{
	uint32 pci_index = 0;
	long count = 0;
	device_info *di = pd->di;
	pci_info *pcii;
	uint8 adress;

	/* while there are more pci devices */
	while ((count < MAX_DEVICES) && ((*pci_bus->get_nth_pci_info)(pci_index, &(di->pcii)) == B_NO_ERROR))
	{
		/* if the device is a hostbridge or a graphicscard */
		if (((di->pcii.class_base == PCI_bridge) && (di->pcii.class_sub == PCI_host)) ||
			 (di->pcii.class_base == PCI_display))
		{
			pcii = &(di->pcii);

			/* if the device has an AGP interface */
			if (has_AGP_interface(pcii, &adress))
			{
				/* fill out the agp_info struct */
				pd->di[count].agpi.vendor_id = di->pcii.vendor_id;
				pd->di[count].agpi.device_id = di->pcii.device_id;
				pd->di[count].agpi.bus = di->pcii.bus;
				pd->di[count].agpi.device = di->pcii.device;
				pd->di[count].agpi.function = di->pcii.function;
				pd->di[count].agpi.class_sub = di->pcii.class_sub;
				pd->di[count].agpi.class_base = di->pcii.class_base;
				/* get the contents of the AGP registers from this device */
				pd->di[count].agpi.interface.agp_cap_id = get_pci(adress, 4);
				pd->di[count].agpi.interface.agp_stat = get_pci(adress + 4, 4);
				pd->di[count].agpi.interface.agp_cmd = get_pci(adress + 8, 4);

				/* remember the AGP interface location in this device */
				pd->di[count].agp_adress = adress;

				/* set OK status */
				pd->di[count].agpi.status = B_OK;

				/* inc pointer to device info */
				di++;
				/* inc count */
				count++;
				/* break out of these while loops */
				goto next_device;
			}
		}
next_device:
		/* next pci_info struct, please */
		pci_index++;
	}

	/* note actual number of AGP devices found */
	pd->count = count;
	TRACE("agp_man: found %d AGP capable device(s)\n", (uint16)count);

	/* if we didn't find any devices no AGP bus exists */
	if (!count) return B_ERROR;

	/* check if all devices are set to a compatible mode (all AGP1/2 or all AGP3 style rates scheme) */
	if (count > 1)
	{
		for (count = 1; count < pd->count; count++)
		{
			if ((pd->di[count - 1].agpi.interface.agp_stat & AGP_rate_rev) !=
				(pd->di[count].agpi.interface.agp_stat & AGP_rate_rev))
			{
				TRACE("agp_man: compatibility problem detected, aborting\n");
				return B_ERROR;
			}
		}
	}

	return B_OK;
}

static void read_settings(void)
{
	void *settings_handle;

	settings_handle  = load_driver_settings ("agp.settings");
	if (settings_handle != NULL) {
		const char *item;
		char       *end;
		uint32      value;

		item = get_driver_parameter (settings_handle, "max_speed", "0", "0");
		value = strtoul (item, &end, 0);
		if (*end == '\0') current_settings.max_speed = value;

		current_settings.block_agp = get_driver_boolean_parameter (settings_handle, "block_agp", false, false);
		current_settings.block_sba = get_driver_boolean_parameter (settings_handle, "block_sba", false, false);
		current_settings.block_fw = get_driver_boolean_parameter (settings_handle, "block_fw", false, false);

		unload_driver_settings (settings_handle);
	}
}

void uninit(void)
{
	/* free the driver data */
	DELETE_BEN(pd->kernel);
	free(pd);
	pd = NULL;

	/* put the pci module away */
	put_module(B_PCI_MODULE_NAME);
}

static bool has_AGP_interface(pci_info *pcii, uint8 *adress)
{
	bool has_AGP = false;

	/* check if device implements a list of capabilities */
	/* (PCI config space 'devctrl' register) */
	if (get_pci(PCI_status, 2) & 0x0010)
	{
		uint32 item;
		uint8 item_adress;

		/* get pointer to PCI capabilities list */
		item_adress = get_pci(0x34, 1);
		/* read first item from list */
		item = get_pci(item_adress, 4);
		while ((item & AGP_next_ptr) && ((item & AGP_id_mask) != AGP_id))
		{
			/* read next item from list */
			item_adress = ((item & AGP_next_ptr) >> AGP_next_ptr_shift);
			item = get_pci(item_adress, 4);
		}
		if ((item & AGP_id_mask) == AGP_id)
		{
			/* we found the AGP interface! */
			has_AGP = true;
			/* return the adress if the interface */
			*adress = item_adress;
		}
	}
	return has_AGP;
}

long get_nth_agp_info (long index, agp_info *info)
{
	pci_info *pcii;

	TRACE("agp_man: get_nth_agp_info called with index %d\n", (uint16)index);

	/* check if we have a device here */
	if (index >= pd->count) return B_ERROR;

	/* refresh from the contents of the AGP registers from this device */
	/* (note: also get agp_stat as some graphicsdriver may have been tweaking, like nvidia) */
	pcii = &(pd->di[index].pcii);
	pd->di[index].agpi.interface.agp_stat = get_pci(pd->di[index].agp_adress + 4, 4);
	pd->di[index].agpi.interface.agp_cmd = get_pci(pd->di[index].agp_adress + 8, 4);

	/* return requested info */
	*info = pd->di[index].agpi;

	return B_NO_ERROR;
}

void enable_agp (uint32 *command)
{
	long count;
	pci_info *pcii;

	TRACE("agp_man: enable_agp called\n");

	/* validate the given command: */
	/* if we should set PCI mode, reset all options */
	if (!(*command & AGP_enable)) *command = 0x00000000;
	/* make sure we accept all modes lower than requested one and we reset reserved bits */
	if (!(*command & AGP_rate_rev))
	{
		/* AGP 2.0 scheme applies */
		if (*command & AGP_2_4x) *command |= AGP_2_2x;
		if (*command & AGP_2_2x) *command |= AGP_2_1x;
	}
	else
	{
		/* AGP 3.0 scheme applies */
		if (*command & AGP_3_8x) *command |= AGP_3_4x;
		*command &= ~0x00000004;
		/* SBA is required for AGP3 */
		*command |= AGP_SBA;
	}
	/* reset all other reserved and currently unused bits */
	*command &= ~0x00fffce0;
	/* block features if requested by user in agp.settings file */
	check_settings(command);

	/* iterate through our device list to find the common capabilities supported */
	for (count = 0; count < pd->count; count++)
	{
		pcii = &(pd->di[count].pcii);

		/* refresh from the contents of the AGP capability registers */
		/* (note: some graphicsdriver may have been tweaking, like nvidia) */
		pd->di[count].agpi.interface.agp_stat = get_pci(pd->di[count].agp_adress + 4, 4);

		check_capabilities(pd->di[count].agpi.interface.agp_stat, command);
	}

	/* select only the highest remaining possible mode-bits */
	if (!(*command & AGP_rate_rev))
	{
		/* AGP 2.0 scheme applies */
		if (*command & AGP_2_4x) *command &= ~(AGP_2_2x | AGP_2_1x);
		if (*command & AGP_2_2x) *command &= ~AGP_2_1x;
	}
	else
	{
		/* AGP 3.0 scheme applies */
		if (*command & AGP_3_8x) *command &= ~AGP_3_4x;
	}

	/* note:
	 * we are not setting the enable AGP bits ourselves, as the user should have specified that
	 * in 'command' if we should be enabling AGP mode here (otherwise we set PCI mode). */
	/* note also:
	 * the AGP standard defines that this bit may be written to the AGPCMD
	 * registers simultanously with the other bits if a single 32bit write
	 * to each register is used. */

	/* first program all bridges */
	for (count = 0; count < pd->count; count++)
	{
		if (pd->di[count].agpi.class_base == PCI_bridge)
		{
			pcii = &(pd->di[count].pcii);
			/* program bridge, making sure not-implemented bits are written as zeros */
			set_pci(pd->di[count].agp_adress + 8, 4, (*command & ~(AGP_rate_rev | AGP_RQ)));
			/* update our agp_cmd info with read back setting from register just programmed */
			pd->di[count].agpi.interface.agp_cmd = get_pci(pd->di[count].agp_adress + 8, 4);
		}
	}

	/* wait 10mS for the bridges to recover (failsafe!) */
	/* note:
	 * some SiS bridge chipsets apparantly require 5mS to recover or the
	 * master (graphics card) cannot be initialized correctly! */
	snooze(10000);

	/* _now_ program all graphicscards */
	for (count = 0; count < pd->count; count++)
	{
		if (pd->di[count].agpi.class_base == PCI_display)
		{
			pci_info *pcii = &(pd->di[count].pcii);
			/* program graphicscard, making sure not-implemented bits are written as zeros */
			set_pci(pd->di[count].agp_adress + 8, 4, (*command & ~AGP_rate_rev));
			/* update our agp_cmd info with read back setting from register just programmed */
			pd->di[count].agpi.interface.agp_cmd = get_pci(pd->di[count].agp_adress + 8, 4);
		}
	}
}

static void check_settings(uint32 *command)
{
	if (current_settings.block_agp)
	{
		TRACE("agp_man: blocking all agp modes (agp.settings)\n");
		*command = 0x00000000;
		return;
	}
	if (current_settings.max_speed != 0)
	{
		if (!(*command & AGP_rate_rev))
		{
			/* AGP 2.0 scheme applies */
			switch (current_settings.max_speed)
			{
			case 8:
				TRACE("agp_man: max AGP 8x speed allowed, using max 4x (agp.settings)\n");
				break;
			case 4:
				TRACE("agp_man: max AGP 4x speed allowed (agp.settings)\n");
				break;
			case 2:
				TRACE("agp_man: max AGP 2x speed allowed (agp.settings)\n");
				*command &= ~AGP_2_4x;
				break;
			case 1:
				TRACE("agp_man: max AGP 1x speed allowed (agp.settings)\n");
				*command &= ~(AGP_2_4x | AGP_2_2x);
				break;
			default:
				TRACE("agp_man: illegal max AGP speed requested, ignoring (agp.settings)\n");
				break;
			}
		}
		else
		{
			/* AGP 3.0 scheme applies */
			switch (current_settings.max_speed)
			{
			case 8:
				TRACE("agp_man: max AGP 8x speed allowed (agp.settings)\n");
				break;
			case 4:
				TRACE("agp_man: max AGP 4x speed allowed (agp.settings)\n");
				*command &= ~AGP_3_8x;
				break;
			case 2:
				TRACE("agp_man: max AGP 2x speed allowed, forcing PCI mode (agp.settings)\n");
				*command = 0x00000000;
				return;
				break;
			case 1:
				TRACE("agp_man: max AGP 1x speed allowed, forcing PCI mode (agp.settings)\n");
				*command = 0x00000000;
				return;
				break;
			default:
				TRACE("agp_man: illegal max AGP speed requested, ignoring (agp.settings)\n");
				break;
			}
		}
	}
	if (current_settings.block_sba)
	{
		if (!(*command & AGP_rate_rev))
		{
			TRACE("agp_man: blocking SBA (agp.settings)\n");
			*command &= ~AGP_SBA;
		}
		else
		{
			TRACE("agp_man: SBA is required for AGP3, not blocking (agp.settings)\n");
		}
	}
	if (current_settings.block_fw)
	{
		TRACE("agp_man: blocking FW (agp.settings)\n");
		*command &= ~AGP_FW;
	}
}

static void check_capabilities(uint32 agp_stat, uint32 *command)
{
	uint8 rq_depth_card, rq_depth_cmd;

	/* block non-supported AGP modes */
	if (!(agp_stat & AGP_rate_rev))
	{
		/* AGP 2.0 scheme applies */
		if (!(agp_stat & AGP_2_4x)) *command &= ~AGP_2_4x;
		if (!(agp_stat & AGP_2_2x)) *command &= ~AGP_2_2x;
		if (!(agp_stat & AGP_2_1x)) *command &= ~AGP_2_1x;
	}
	else
	{
		/* AGP 3.0 scheme applies */
		if (!(agp_stat & AGP_3_8x)) *command &= ~AGP_3_8x;
		if (!(agp_stat & AGP_3_4x)) *command &= ~AGP_3_4x;
	}
	/* if no AGP mode is supported at all, nothing remains:
	 * devices exist that have the AGP style 
	 * connector with AGP style registers, but not the features!
	 * (confirmed Matrox Millenium II AGP for instance) */
	if (!(agp_stat & AGP_rates)) *command = 0x00000000;

	/* block sideband adressing if not supported */
	if (!(agp_stat & AGP_SBA)) *command &= ~AGP_SBA;

	/* block fast writes if not supported */
	if (!(agp_stat & AGP_FW)) *command &= ~AGP_FW;

	/* adjust maximum request depth to least depth supported */
	/* note:
	 * this is writable only in the graphics card */
	rq_depth_card = ((agp_stat & AGP_RQ) >> AGP_RQ_shift);
	rq_depth_cmd = ((*command & AGP_RQ) >> AGP_RQ_shift);
	if (rq_depth_card < rq_depth_cmd)
	{
		*command &= ~AGP_RQ;
		*command |= (rq_depth_card << AGP_RQ_shift);
	}
}
