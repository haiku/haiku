/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.

	Other authors:
	Mark Watson;
	Rudolf Cornelissen 3/2002-1/2006.
*/

/* standard kernel driver stuff */
#include <KernelExport.h>
#include <PCI.h>
#include <OS.h>
#include <directories.h>
#include <driver_settings.h>
#include <malloc.h>
#include <stdlib.h> // for strtoXX

/* this is for the standardized portion of the driver API */
/* currently only one operation is defined: B_GET_ACCELERANT_SIGNATURE */
#include <graphic_driver.h>

/* this is for sprintf() */
#include <stdio.h>

/* this is for string compares */
#include <string.h>

/* The private interface between the accelerant and the kernel driver. */
#include "DriverInterface.h"
#include "mga_macros.h"

#define get_pci(o, s) (*pci_bus->read_pci_config)(pcii->bus, pcii->device, pcii->function, (o), (s))
#define set_pci(o, s, v) (*pci_bus->write_pci_config)(pcii->bus, pcii->device, pcii->function, (o), (s), (v))

#define MAX_DEVICES	  8

#ifndef __HAIKU__
#	undef B_USER_CLONEABLE_AREA
#	define B_USER_CLONEABLE_AREA 0
#endif

/* Tell the kernel what revision of the driver API we support */
int32	api_version = B_CUR_DRIVER_API_VERSION; // apsed, was 2, is 2 in R5

/* these structures are private to the kernel driver */
typedef struct device_info device_info;

typedef struct {
	timer		te;				/* timer entry for add_timer() */
	device_info	*di;			/* pointer to the owning device */
	bigtime_t	when_target;	/* when we're supposed to wake up */
} timer_info;

struct device_info {
	uint32		is_open;			/* a count of how many times the devices has been opened */
	area_id		shared_area;		/* the area shared between the driver and all of the accelerants */
	shared_info	*si;				/* a pointer to the shared area, for convenience */
	vuint32		*regs;				/* kernel's pointer to memory mapped registers */
	pci_info	pcii;					/* a convenience copy of the pci info for this device */
	char		name[B_OS_NAME_LENGTH];	/* where we keep the name of the device for publishing and comparing */
	uint8 		rom_mirror[32768];	/* mirror of the ROM (is needed for MMS cards) */
};

typedef struct {
	uint32		count;				/* number of devices actually found */
	benaphore	kernel;				/* for serializing opens/closes */
	char		*device_names[MAX_DEVICES+1];	/* device name pointer storage */
	device_info	di[MAX_DEVICES];	/* device specific stuff */
} DeviceData;

/* prototypes for our private functions */
static status_t open_hook (const char* name, uint32 flags, void** cookie);
static status_t close_hook (void* dev);
static status_t free_hook (void* dev);
static status_t read_hook (void* dev, off_t pos, void* buf, size_t* len);
static status_t write_hook (void* dev, off_t pos, const void* buf, size_t* len);
static status_t control_hook (void* dev, uint32 msg, void *buf, size_t len);
static status_t map_device(device_info *di);
static void unmap_device(device_info *di);
static void copy_rom(device_info *di);
static void probe_devices(void);
static int32 gx00_interrupt(void *data);

static DeviceData		*pd;
static pci_module_info	*pci_bus;
static device_hooks graphics_device_hooks = {
	open_hook,
	close_hook,
	free_hook,
	control_hook,
	read_hook,
	write_hook,
	NULL,
	NULL,
	NULL,
	NULL
};

#define VENDOR_ID			0x102b	/* Matrox graphics inc. */

static uint16 gx00_device_list[] = {
	0x2527, /* G450AGP, G550AGP */
	0x0525, /* G400AGP */
	0x0520, /* G200PCI */
	0x0521, /* G200AGP */
	0x1000, /* G100PCI */
	0x1001, /* G100AGP */
	0x051B, /* MGA-2164 PCI Millennium 2 */
	0x051F, /* MGA-2164 AGP Millennium 2 */
	0x0519, /* MGA-2064 PCI Millennium 1 */
//fixme? only support Mystique once we are sure we actually support it...
//	0x051A, /* MGA-1064 PCI Mystique 170/220 */
	0
};

static struct {
	uint16	vendor;
	uint16	*devices;
} SupportedDevices[] = {
	{VENDOR_ID, gx00_device_list},
	{0x0000, NULL}
};

/* see comments in mga.settings */
static settings current_settings =
{
	/* for kerneldriver */
	DRIVER_PREFIX ".accelerant",
	"none",					// primary
	false,      			// dumprom
	/* for accelerant */
	0x00000000, 			// logmask
	0,          			// memory
	false,      			// usebios
	false,      			// hardcursor
	false,					// greensync
};

static void dumprom (void *rom, size_t size, pci_info pcii)
{
	int fd;
	char fname[64];

	/* determine the romfile name: we need split-up per card in the system */
	sprintf (fname, kUserDirectory "/" DRIVER_PREFIX "." DEVICE_FORMAT ".rom",
		pcii.vendor_id, pcii.device_id, pcii.bus, pcii.device, pcii.function);

	fd = open (fname, O_WRONLY | O_CREAT, 0666);
	if (fd < 0) return;
	write (fd, rom, size);
	close (fd);
}

/*return 1, is interrupt has occured*/
static int caused_vbi(vuint32 * regs)
{
	return (ACCR(STATUS)&0x20);
}

/*clear the interrupt*/
static void clear_vbi(vuint32 * regs)
{
	ACCW(ICLEAR,0x20);
}

static void enable_vbi(vuint32 * regs)
{
	ACCW(IEN,ACCR(IEN)|0x20);
}

static void disable_vbi(vuint32 * regs)
{
	ACCW(IEN,(ACCR(IEN)&~0x20));
	ACCW(ICLEAR,0x20);
}


/*
	init_hardware() - Returns B_OK if one is
	found, otherwise returns B_ERROR so the driver will be unloaded.
*/
status_t
init_hardware(void) {
	long		pci_index = 0;
	pci_info	pcii;
	bool		found_one = FALSE;

	/* choke if we can't find the PCI bus */
	if (get_module(B_PCI_MODULE_NAME, (module_info **)&pci_bus) != B_OK)
		return B_ERROR;

	/* while there are more pci devices */
	while ((*pci_bus->get_nth_pci_info)(pci_index, &pcii) == B_NO_ERROR) {
		int vendor = 0;

		/* if we match a supported vendor */
		while (SupportedDevices[vendor].vendor) {
			if (SupportedDevices[vendor].vendor == pcii.vendor_id) {
				uint16 *devices = SupportedDevices[vendor].devices;
				/* while there are more supported devices */
				while (*devices) {
					/* if we match a supported device */
					if (*devices == pcii.device_id ) {

						found_one = TRUE;
						goto done;
					}
					/* next supported device */
					devices++;
				}
			}
			vendor++;
		}
		/* next pci_info struct, please */
		pci_index++;
	}

done:
	/* put away the module manager */
	put_module(B_PCI_MODULE_NAME);
	return (found_one ? B_OK : B_ERROR);
}

status_t
init_driver(void) {
	void *settings_handle;

	// get driver/accelerant settings, apsed
	settings_handle  = load_driver_settings (DRIVER_PREFIX ".settings");
	if (settings_handle != NULL) {
		const char *item;
		char       *end;
		uint32      value;

		// for driver
		item = get_driver_parameter (settings_handle, "accelerant", "", "");
		if ((strlen (item) > 0) && (strlen (item) < sizeof (current_settings.accelerant) - 1)) {
			strcpy (current_settings.accelerant, item);
		}
		item = get_driver_parameter (settings_handle, "primary", "", "");
		if ((strlen (item) > 0) && (strlen (item) < sizeof (current_settings.primary) - 1)) {
			strcpy (current_settings.primary, item);
		}
		current_settings.dumprom = get_driver_boolean_parameter (settings_handle, "dumprom", false, false);

		// for accelerant
		item = get_driver_parameter (settings_handle, "logmask", "0x00000000", "0x00000000");
		value = strtoul (item, &end, 0);
		if (*end == '\0') current_settings.logmask = value;

		item = get_driver_parameter (settings_handle, "memory", "0", "0");
		value = strtoul (item, &end, 0);
		if (*end == '\0') current_settings.memory = value;

		current_settings.hardcursor = get_driver_boolean_parameter (settings_handle, "hardcursor", false, false);
		current_settings.usebios = get_driver_boolean_parameter (settings_handle, "usebios", false, false);
		current_settings.greensync = get_driver_boolean_parameter (settings_handle, "greensync", false, false);

		unload_driver_settings (settings_handle);
	}

	/* get a handle for the pci bus */
	if (get_module(B_PCI_MODULE_NAME, (module_info **)&pci_bus) != B_OK)
		return B_ERROR;

	/* driver private data */
	pd = (DeviceData *)calloc(1, sizeof(DeviceData));
	if (!pd) {
		put_module(B_PCI_MODULE_NAME);
		return B_ERROR;
	}
	/* initialize the benaphore */
	INIT_BEN(pd->kernel);
	/* find all of our supported devices */
	probe_devices();
	return B_OK;
}

const char **
publish_devices(void) {
	/* return the list of supported devices */
	return (const char **)pd->device_names;
}

device_hooks *
find_device(const char *name) {
	int index = 0;
	while (pd->device_names[index]) {
		if (strcmp(name, pd->device_names[index]) == 0)
			return &graphics_device_hooks;
		index++;
	}
	return NULL;

}

void uninit_driver(void) {

	/* free the driver data */
	DELETE_BEN(pd->kernel);
	free(pd);
	pd = NULL;

	/* put the pci module away */
	put_module(B_PCI_MODULE_NAME);
}

static status_t map_device(device_info *di)
{
	char buffer[B_OS_NAME_LENGTH]; /*memory for device name*/
	shared_info *si = di->si;
	uint32	tmpUlong;
	pci_info *pcii = &(di->pcii);
	system_info sysinfo;

	/*storage for the physical to virtual table (used for dma buffer)*/
//	physical_entry physical_memory[2];
//	#define G400_DMA_BUFFER_SIZE 1024*1024

	/*variables for making copy of ROM*/
	uint8 *rom_temp;
	area_id rom_area;

	/* MIL1 has frame_buffer in [1], control_regs in [0], and nothing in [2], while
	 * MIL2 and later have frame_buffer in [0], control_regs in [1], pseudo_dma in [2] */
	int frame_buffer = 0;
	int registers = 1;
	int pseudo_dma = 2;

	/* correct layout for MIL1 */
	//fixme: checkout Mystique 170 and 220...
	if (di->pcii.device_id == 0x0519)
	{
		frame_buffer = 1;
		registers = 0;
	}

	/* enable memory mapped IO, disable VGA I/O - this is standard*/
	tmpUlong = get_pci(PCI_command, 4);
	tmpUlong |= 0x00000002;
	tmpUlong &= 0xfffffffe;
	set_pci(PCI_command, 4, tmpUlong);

 	/*work out which version of BeOS is running*/
 	get_system_info(&sysinfo);
 	if (0)//sysinfo.kernel_build_date[0]=='J')/*FIXME - better ID version*/
 	{
 		si->use_clone_bugfix = 1;
 	}
 	else
 	{
 		si->use_clone_bugfix = 0;
 	}

	/* work out a name for the register mapping */
	sprintf(buffer, DEVICE_FORMAT " regs",
		di->pcii.vendor_id, di->pcii.device_id,
		di->pcii.bus, di->pcii.device, di->pcii.function);

	/* get a virtual memory address for the registers*/
	si->regs_area = map_physical_memory(
		buffer,
		di->pcii.u.h0.base_registers[registers],
		di->pcii.u.h0.base_register_sizes[registers],
		B_ANY_KERNEL_ADDRESS,
 		B_USER_CLONEABLE_AREA | (si->use_clone_bugfix ? B_READ_AREA|B_WRITE_AREA : 0),
		(void **)&(di->regs));
 	si->clone_bugfix_regs = (uint32 *) di->regs;

	/* if mapping registers to vmem failed then pass on error */
	if (si->regs_area < 0) return si->regs_area;

	/* work out a name for the ROM mapping*/
	sprintf(buffer, DEVICE_FORMAT " rom",
		di->pcii.vendor_id, di->pcii.device_id,
		di->pcii.bus, di->pcii.device, di->pcii.function);

	/*place ROM over the fbspace (this is definately safe)*/
	tmpUlong = di->pcii.u.h0.base_registers[frame_buffer];
	tmpUlong |= 0x00000001;
	set_pci(PCI_rom_base, 4, tmpUlong);

	rom_area = map_physical_memory(
		buffer,
		di->pcii.u.h0.base_registers[frame_buffer],
		32768,
		B_ANY_KERNEL_ADDRESS,
		B_READ_AREA,
		(void **)&(rom_temp)
	);

	/* if mapping ROM to vmem failed then clean up and pass on error */
	if (rom_area < 0) {
		delete_area(si->regs_area);
		si->regs_area = -1;
		return rom_area;
	}

	/* if we have a MMS card which only has a BIOS on the primary card, copy the
	 * primary card's BIOS for our reference too if we aren't primary ourselves.
	 * (confirmed OK on 'quad' G200MMS.) */
	if ((di->pcii.class_base == PCI_display) &&
		(di->pcii.class_sub == PCI_display_other) &&
		((rom_temp[0] != 0x55) || (rom_temp[1] != 0xaa)) && di->pcii.device)
	{
		/* locate the main VGA adaptor on our bus, should sit on device #0
		 * (MMS cards have a own bridge: so there are only graphics cards on it's bus). */
		uint8 index = 0;
		bool found = false;
		for (index = 0; index < pd->count; index++)
		{
			if ((pd->di[index].pcii.bus == di->pcii.bus) &&
				(pd->di[index].pcii.device == 0x00))
			{
				found = true;
				break;
			}
		}
		if (found)
		{
			/* make the copy from the primary VGA card on our bus */
			memcpy (si->rom_mirror, pd->di[index].rom_mirror, 32768);
		}
		else
		{
			/* make a copy of 'non-ok' ROM area for future reference just in case */
			memcpy (si->rom_mirror, rom_temp, 32768);
		}
	}
	else
	{
		/* make a copy of ROM for future reference */
		memcpy (si->rom_mirror, rom_temp, 32768);
	}

	if (current_settings.dumprom) dumprom (si->rom_mirror, 32768, di->pcii);

	/*disable ROM and delete the area*/
	set_pci(PCI_rom_base,4,0);
	delete_area(rom_area);

	/* (pseudo)DMA does not exist on MIL1 */
	//fixme: checkout Mystique 170 and 220...
	if (di->pcii.device_id != 0x0519)
	{
		/* work out a name for the pseudo dma mapping*/
		sprintf(buffer, DEVICE_FORMAT " pseudodma",
			di->pcii.vendor_id, di->pcii.device_id,
			di->pcii.bus, di->pcii.device, di->pcii.function);

		/* map the pseudo dma into vmem (write-only)*/
		si->pseudo_dma_area = map_physical_memory(
			buffer,
			di->pcii.u.h0.base_registers[pseudo_dma],
			di->pcii.u.h0.base_register_sizes[pseudo_dma],
			B_ANY_KERNEL_ADDRESS,
			B_WRITE_AREA,
			&(si->pseudo_dma));

		/* if there was an error, delete our other areas and pass on error*/
		if (si->pseudo_dma_area < 0) {
			delete_area(si->regs_area);
			si->regs_area = -1;
			return si->pseudo_dma_area;
		}

		/* work out a name for the a dma buffer*/
//		sprintf(buffer, DEVICE_FORMAT " dmabuffer",
//			di->pcii.vendor_id, di->pcii.device_id,
//			di->pcii.bus, di->pcii.device, di->pcii.function);

		/* create an area for the dma buffer*/
//		si->dma_buffer_area = create_area(
//			buffer,
//			&si->dma_buffer,
//			B_ANY_ADDRESS,
//			G400_DMA_BUFFER_SIZE,
//			B_CONTIGUOUS,
//			B_READ_AREA|B_WRITE_AREA);

		/* if there was an error, delete our other areas and pass on error*/
//		if (si->dma_buffer_area < 0) {
//			delete_area(si->pseudo_dma_area);
//			si->pseudo_dma_area = -1;
//			delete_area(si->regs_area);
//			si->regs_area = -1;
//			return si->dma_buffer_area;
//		}

		/*find where it is in real memory*/
//		get_memory_map(si->dma_buffer,4,physical_memory,1);
//		si->dma_buffer_pci = physical_memory[0].address; /*addr from PCI space*/
	}

	/* work out a name for the framebuffer mapping*/
	sprintf(buffer, DEVICE_FORMAT " framebuffer",
		di->pcii.vendor_id, di->pcii.device_id,
		di->pcii.bus, di->pcii.device, di->pcii.function);

	/* map the framebuffer into vmem, using Write Combining*/
	si->fb_area = map_physical_memory(
		buffer,
		di->pcii.u.h0.base_registers[frame_buffer],
		di->pcii.u.h0.base_register_sizes[frame_buffer],
		B_ANY_KERNEL_BLOCK_ADDRESS | B_MTR_WC,
		B_READ_AREA | B_WRITE_AREA,
		&(si->framebuffer));

	/*if failed with write combining try again without*/
	if (si->fb_area < 0) {
		si->fb_area = map_physical_memory(
			buffer,
			di->pcii.u.h0.base_registers[frame_buffer],
			di->pcii.u.h0.base_register_sizes[frame_buffer],
			B_ANY_KERNEL_BLOCK_ADDRESS,
			B_READ_AREA | B_WRITE_AREA,
			&(si->framebuffer));
	}

	/* if there was an error, delete our other areas and pass on error*/
	if (si->fb_area < 0)
	{
		/* (pseudo)DMA does not exist on MIL1 */
		//fixme: checkout Mystique 170 and 220...
		if (di->pcii.device_id != 0x0519)
		{
			delete_area(si->dma_buffer_area);
			si->dma_buffer_area = -1;
			delete_area(si->pseudo_dma_area);
			si->pseudo_dma_area = -1;
		}
		delete_area(si->regs_area);
		si->regs_area = -1;
		return si->fb_area;
	}
	/* remember the DMA address of the frame buffer for BDirectWindow?? purposes */
	si->framebuffer_pci = (void *) di->pcii.u.h0.base_registers_pci[frame_buffer];

	// remember settings for use here and in accelerant
	si->settings = current_settings;

	/* in any case, return the result */
	return si->fb_area;
}

static void unmap_device(device_info *di) {
	shared_info *si = di->si;
	uint32	tmpUlong;
	pci_info *pcii = &(di->pcii);

	/* disable memory mapped IO */
	tmpUlong = get_pci(PCI_command, 4);
	tmpUlong &= 0xfffffffc;
	set_pci(PCI_command, 4, tmpUlong);
	/* delete the areas */
	if (si->regs_area >= 0) delete_area(si->regs_area);
	if (si->fb_area >= 0) delete_area(si->fb_area);
	si->regs_area = si->fb_area = -1;

	/* (pseudo)DMA does not exist on MIL1 */
	//fixme: checkout Mystique 170 and 220...
	if (di->pcii.device_id != 0x0519)
	{
		delete_area(si->dma_buffer_area);
		si->dma_buffer_area = -1;
		delete_area(si->pseudo_dma_area);
		si->pseudo_dma_area = -1;
	}

	si->framebuffer = NULL;
	di->regs = NULL;
}

static void copy_rom(device_info *di)
{
	char buffer[B_OS_NAME_LENGTH];
	uint8 *rom_temp;
	area_id rom_area;
	uint32 tmpUlong;
	pci_info *pcii = &(di->pcii);

	/* MIL1 has frame_buffer in [1], while MIL2 and later have frame_buffer in [0] */
	int frame_buffer = 0;
	/* correct layout for MIL1 */
	//fixme: checkout Mystique 170 and 220...
	if (di->pcii.device_id == 0x0519) frame_buffer = 1;

	/* enable memory mapped IO, disable VGA I/O - this is standard*/
	tmpUlong = get_pci(PCI_command, 4);
	tmpUlong |= 0x00000002;
	tmpUlong &= 0xfffffffe;
	set_pci(PCI_command, 4, tmpUlong);

	/* work out a name for the ROM mapping*/
	sprintf(buffer, DEVICE_FORMAT " rom",
		di->pcii.vendor_id, di->pcii.device_id,
		di->pcii.bus, di->pcii.device, di->pcii.function);

	/*place ROM over the fbspace (this is definately safe)*/
	tmpUlong = di->pcii.u.h0.base_registers[frame_buffer];
	tmpUlong |= 0x00000001;
	set_pci(PCI_rom_base, 4, tmpUlong);

	rom_area = map_physical_memory(
		buffer,
		di->pcii.u.h0.base_registers[frame_buffer],
		32768,
		B_ANY_KERNEL_ADDRESS,
		B_READ_AREA,
		(void **)&(rom_temp)
	);

	/* if mapping ROM to vmem was successfull copy it */
	if (rom_area >= 0)
	{
		/* copy ROM for future reference (MMS cards) */
		memcpy (di->rom_mirror, rom_temp, 32768);

		/* disable ROM and delete the area */
		set_pci(PCI_rom_base, 4, 0);
		delete_area(rom_area);
	}

	/* disable memory mapped IO */
	tmpUlong = get_pci(PCI_command, 4);
	tmpUlong &= 0xfffffffc;
	set_pci(PCI_command, 4, tmpUlong);
}

static void probe_devices(void)
{
	uint32 pci_index = 0;
	uint32 count = 0;
	device_info *di = pd->di;
	char tmp_name[B_OS_NAME_LENGTH];

	/* while there are more pci devices */
	while ((count < MAX_DEVICES) && ((*pci_bus->get_nth_pci_info)(pci_index, &(di->pcii)) == B_NO_ERROR))
	{
		int vendor = 0;

		/* if we match a supported vendor */
		while (SupportedDevices[vendor].vendor)
		{
			if (SupportedDevices[vendor].vendor == di->pcii.vendor_id)
			{
				uint16 *devices = SupportedDevices[vendor].devices;
				/* while there are more supported devices */
				while (*devices)
				{
					/* if we match a supported device */
					if (*devices == di->pcii.device_id )
					{
						/* publish the device name */
						sprintf(tmp_name, DEVICE_FORMAT,
							di->pcii.vendor_id, di->pcii.device_id,
							di->pcii.bus, di->pcii.device, di->pcii.function);
						/* tweak the exported name to show first in the alphabetically ordered /dev/
						 * hierarchy folder, so the system will use it as primary adaptor if requested
						 * via mga.settings. */
						if (strcmp(tmp_name, current_settings.primary) == 0)
							sprintf(tmp_name, "-%s", current_settings.primary);
						/* add /dev/ hierarchy path */
						sprintf(di->name, "graphics/%s", tmp_name);
						/* remember the name */
						pd->device_names[count] = di->name;
						/* mark the driver as available for R/W open */
						di->is_open = 0;
						/* mark areas as not yet created */
						di->shared_area = -1;
						/* mark pointer to shared data as invalid */
						di->si = NULL;
						/* copy ROM for MMS (multihead with multiple GPU) cards:
						 * they might only have a ROM on the primary adaptor.
						 * (Confirmed G200MMS.) */
						copy_rom(di);
						/* inc pointer to device info */
						di++;
						/* inc count */
						count++;
						/* break out of these while loops */
						goto next_device;
					}
					/* next supported device */
					devices++;
				}
			}
			vendor++;
		}
next_device:
		/* next pci_info struct, please */
		pci_index++;
	}

	/* propagate count */
	pd->count = count;
	/* terminate list of device names with a null pointer */
	pd->device_names[pd->count] = NULL;
}

static uint32 thread_interrupt_work(int32 *flags, vuint32 *regs, shared_info *si) {
	uint32 handled = B_HANDLED_INTERRUPT;
	/* release the vblank semaphore */
	if (si->vblank >= 0) {
		int32 blocked;
		if ((get_sem_count(si->vblank, &blocked) == B_OK) && (blocked < 0)) {
			release_sem_etc(si->vblank, -blocked, B_DO_NOT_RESCHEDULE);
			handled = B_INVOKE_SCHEDULER;
		}
	}
	return handled;
}

static int32
gx00_interrupt(void *data)
{
	int32 handled = B_UNHANDLED_INTERRUPT;
	device_info *di = (device_info *)data;
	shared_info *si = di->si;
	int32 *flags = &(si->flags);
	vuint32 *regs;

	/* is someone already handling an interrupt for this device? */
	if (atomic_or(flags, SKD_HANDLER_INSTALLED) & SKD_HANDLER_INSTALLED) {
		goto exit0;
	}
	/* get regs */
	regs = di->regs;

	/* was it a VBI? */
	if (caused_vbi(regs)) {
		/*clear the interrupt*/
		clear_vbi(regs);
		/*release the semaphore*/
		handled = thread_interrupt_work(flags, regs, si);
	}

	/* note that we're not in the handler any more */
	atomic_and(flags, ~SKD_HANDLER_INSTALLED);

exit0:
	return handled;
}

static status_t open_hook (const char* name, uint32 flags, void** cookie) {
	int32 index = 0;
	device_info *di;
	shared_info *si;
	thread_id	thid;
	thread_info	thinfo;
	status_t	result = B_OK;
	char shared_name[B_OS_NAME_LENGTH];

	/* find the device name in the list of devices */
	/* we're never passed a name we didn't publish */
	while (pd->device_names[index] && (strcmp(name, pd->device_names[index]) != 0)) index++;

	/* for convienience */
	di = &(pd->di[index]);

	/* make sure no one else has write access to the common data */
	AQUIRE_BEN(pd->kernel);

	/* if it's already open for writing */
	if (di->is_open) {
		/* mark it open another time */
		goto mark_as_open;
	}
	/* create the shared area */
	sprintf(shared_name, DEVICE_FORMAT " shared",
		di->pcii.vendor_id, di->pcii.device_id,
		di->pcii.bus, di->pcii.device, di->pcii.function);
	/* create this area with NO user-space read or write permissions, to prevent accidental dammage */
	di->shared_area = create_area(shared_name, (void **)&(di->si), B_ANY_KERNEL_ADDRESS,
		((sizeof(shared_info) + (B_PAGE_SIZE - 1)) & ~(B_PAGE_SIZE - 1)), B_FULL_LOCK,
		B_USER_CLONEABLE_AREA);
	if (di->shared_area < 0) {
		/* return the error */
		result = di->shared_area;
		goto done;
	}

	/* save a few dereferences */
	si = di->si;

	/* save the vendor and device IDs */
	si->vendor_id = di->pcii.vendor_id;
	si->device_id = di->pcii.device_id;
	si->revision = di->pcii.revision;
	si->bus = di->pcii.bus;
	si->device = di->pcii.device;
	si->function = di->pcii.function;

	/* ensure that the accelerant's INIT_ACCELERANT function can be executed */
	si->accelerant_in_use = false;

	/* map the device */
	result = map_device(di);
	if (result < 0) goto free_shared;

	/* we will be returning OK status for sure now */
	result = B_OK;

	/* disable and clear any pending interrupts */
	disable_vbi(di->regs);

	/* preset we can't use INT related functions */
	si->ps.int_assigned = false;

	/* create a semaphore for vertical blank management */
	si->vblank = create_sem(0, di->name);
	if (si->vblank < 0) goto mark_as_open;

	/* change the owner of the semaphores to the opener's team */
	/* this is required because apps can't aquire kernel semaphores */
	thid = find_thread(NULL);
	get_thread_info(thid, &thinfo);
	set_sem_owner(si->vblank, thinfo.team);

	/* If there is a valid interrupt line assigned then set up interrupts */
	if ((di->pcii.u.h0.interrupt_pin == 0x00) ||
	    (di->pcii.u.h0.interrupt_line == 0xff) || /* no IRQ assigned */
	    (di->pcii.u.h0.interrupt_line <= 0x02))   /* system IRQ assigned */
	{
		/* delete the semaphore as it won't be used */
		delete_sem(si->vblank);
		si->vblank = -1;
	}
	else
	{
		/* otherwise install our interrupt handler */
		result = install_io_interrupt_handler(di->pcii.u.h0.interrupt_line, gx00_interrupt, (void *)di, 0);
		/* bail if we couldn't install the handler */
		if (result != B_OK)
		{
			/* delete the semaphore as it won't be used */
			delete_sem(si->vblank);
			si->vblank = -1;
		}
		else
		{
			/* inform accelerant(s) we can use INT related functions */
			si->ps.int_assigned = true;
		}
	}

mark_as_open:
	/* mark the device open */
	di->is_open++;

	/* send the cookie to the opener */
	*cookie = di;

	goto done;


free_shared:
	/* clean up our shared area */
	delete_area(di->shared_area);
	di->shared_area = -1;
	di->si = NULL;

done:
	/* end of critical section */
	RELEASE_BEN(pd->kernel);

	/* all done, return the status */
	return result;
}

/* ----------
	read_hook - does nothing, gracefully
----- */
static status_t
read_hook (void* dev, off_t pos, void* buf, size_t* len)
{
	*len = 0;
	return B_NOT_ALLOWED;
}


/* ----------
	write_hook - does nothing, gracefully
----- */
static status_t
write_hook (void* dev, off_t pos, const void* buf, size_t* len)
{
	*len = 0;
	return B_NOT_ALLOWED;
}

/* ----------
	close_hook - does nothing, gracefully
----- */
static status_t
close_hook (void* dev)
{
	/* we don't do anything on close: there might be dup'd fd */
	return B_NO_ERROR;
}

/* -----------
	free_hook - close down the device
----------- */
static status_t
free_hook (void* dev) {
	device_info *di = (device_info *)dev;
	shared_info	*si = di->si;
	vuint32 *regs = di->regs;

	/* lock the driver */
	AQUIRE_BEN(pd->kernel);

	/* if opened multiple times, decrement the open count and exit */
	if (di->is_open > 1)
		goto unlock_and_exit;

	/* disable and clear any pending interrupts */
	disable_vbi(regs);

	if (si->ps.int_assigned)
	{
		/* remove interrupt handler */
		remove_io_interrupt_handler(di->pcii.u.h0.interrupt_line, gx00_interrupt, di);

		/* delete the semaphores, ignoring any errors ('cause the owning team may have died on us) */
		delete_sem(si->vblank);
		si->vblank = -1;
	}

	/* free regs and framebuffer areas */
	unmap_device(di);

	/* clean up our shared area */
	delete_area(di->shared_area);
	di->shared_area = -1;
	di->si = NULL;

unlock_and_exit:
	/* mark the device available */
	di->is_open--;
	/* unlock the driver */
	RELEASE_BEN(pd->kernel);
	/* all done */
	return B_OK;
}

/* -----------
	control_hook - where the real work is done
----------- */
static status_t
control_hook (void* dev, uint32 msg, void *buf, size_t len) {
	device_info *di = (device_info *)dev;
	status_t result = B_DEV_INVALID_IOCTL;

	switch (msg) {
		/* the only PUBLIC ioctl */
		case B_GET_ACCELERANT_SIGNATURE: {
			char *sig = (char *)buf;
			strcpy(sig, current_settings.accelerant);
			result = B_OK;
		} break;

		/* PRIVATE ioctl from here on */
		case GX00_GET_PRIVATE_DATA: {
			gx00_get_private_data *gpd = (gx00_get_private_data *)buf;
			if (gpd->magic == GX00_PRIVATE_DATA_MAGIC) {
				gpd->shared_info_area = di->shared_area;
				result = B_OK;
			}
		} break;
		case GX00_GET_PCI: {
			gx00_get_set_pci *gsp = (gx00_get_set_pci *)buf;
			if (gsp->magic == GX00_PRIVATE_DATA_MAGIC) {
				pci_info *pcii = &(di->pcii);
				gsp->value = get_pci(gsp->offset, gsp->size);
				result = B_OK;
			}
		} break;
		case GX00_SET_PCI: {
			gx00_get_set_pci *gsp = (gx00_get_set_pci *)buf;
			if (gsp->magic == GX00_PRIVATE_DATA_MAGIC) {
				pci_info *pcii = &(di->pcii);
				set_pci(gsp->offset, gsp->size, gsp->value);
				result = B_OK;
			}
		} break;
		case GX00_DEVICE_NAME: { // apsed
			gx00_device_name *dn = (gx00_device_name *)buf;
			if (dn->magic == GX00_PRIVATE_DATA_MAGIC) {
				strcpy(dn->name, di->name);
				result = B_OK;
			}
		} break;
		case GX00_RUN_INTERRUPTS: {
			gx00_set_bool_state *ri = (gx00_set_bool_state *)buf;
			if (ri->magic == GX00_PRIVATE_DATA_MAGIC) {
				vuint32 *regs = di->regs;
				if (ri->do_it) {
					enable_vbi(regs);
				} else {
					disable_vbi(regs);
				}
				result = B_OK;
			}
		} break;
	}
	return result;
}

