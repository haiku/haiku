/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.

	Other authors:
	Mark Watson;
	Rudolf Cornelissen 3/2002-7/2003.
*/

/* standard kernel driver stuff */
#include <KernelExport.h>
#include <PCI.h>
#include <OS.h>
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
#include "nv_macros.h"

#define get_pci(o, s) (*pci_bus->read_pci_config)(pcii->bus, pcii->device, pcii->function, (o), (s))
#define set_pci(o, s, v) (*pci_bus->write_pci_config)(pcii->bus, pcii->device, pcii->function, (o), (s), (v))

#define MAX_DEVICES	  8

#define DEVICE_FORMAT "%04X_%04X_%02X%02X%02X" // apsed

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
static void probe_devices(void);
static int32 nv_interrupt(void *data);

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

#define VENDOR_ID_NVIDIA	0x10de /* Nvidia */
#define VENDOR_ID_ELSA		0x1048 /* Elsa GmbH */
#define VENDOR_ID_NVSTBSGS	0x12d2 /* Nvidia STB/SGS-Thompson */
#define VENDOR_ID_VARISYS	0x1888 /* Varisys Limited */

static uint16 nvidia_device_list[] = {
	0x0020, /* Nvidia TNT1 */
	0x0028, /* Nvidia TNT2 (pro) */
	0x0029, /* Nvidia TNT2 Ultra */
	0x002a, /* Nvidia TNT2 */
	0x002b, /* Nvidia TNT2 */
	0x002c, /* Nvidia Vanta (Lt) */
	0x002d, /* Nvidia TNT2-M64 (Pro) */
	0x002e, /* Nvidia NV06 Vanta */
	0x002f, /* Nvidia NV06 Vanta */
	0x00a0, /* Nvidia Aladdin TNT2 */
	0x0100, /* Nvidia GeForce256 SDR */
	0x0101, /* Nvidia GeForce256 DDR */
	0x0102, /* Nvidia GeForce256 Ultra */
	0x0103, /* Nvidia Quadro */
	0x0110, /* Nvidia GeForce2 MX/MX400 */
	0x0111, /* Nvidia GeForce2 MX100/MX200 DDR */
	0x0112, /* Nvidia GeForce2 Go */
	0x0113, /* Nvidia Quadro2 MXR/EX/Go */
	0x0150, /* Nvidia GeForce2 GTS/Pro */
	0x0151, /* Nvidia GeForce2 Ti DDR */
	0x0152, /* Nvidia GeForce2 Ultra */
	0x0153, /* Nvidia Quadro2 Pro */
	0x0170, /* Nvidia GeForce4 MX 460 */
	0x0171, /* Nvidia GeForce4 MX 440 */
	0x0172, /* Nvidia GeForce4 MX 420 */ 
	0x0173, /* Nvidia GeForce4 MX 440SE */ 
	0x0174, /* Nvidia GeForce4 440 Go */ 
	0x0175, /* Nvidia GeForce4 420 Go */
	0x0176, /* Nvidia GeForce4 420 Go 32M */
	0x0177, /* Nvidia GeForce4 460 Go */
	0x0179, /* Nvidia GeForce4 440 Go 64M */
	0x0178, /* Nvidia Quadro4 500 XGL/550 XGL */
	0x017a, /* Nvidia Quadro4 200 NVS/400 NVS */
	0x017c, /* Nvidia Quadro4 500 GoGL */
	//fixme: three IDs below correct??
	0x0180, /* Nvidia GeForce4 MX 440 AGP8X */
	0x0181, /* Nvidia GeForce4 MX 440SE AGP8X */
	0x0182, /* Nvidia GeForce4 MX 420 AGP8X */
	0x0188, /* Nvidia Quadro4 580 XGL */
	0x018a, /* Nvidia Quadro4 280 NVS */
	0x018b, /* Nvidia Quadro4 380 XGL */
	0x01a0, /* Nvidia GeForce2 Integrated GPU */
	0x01f0, /* Nvidia GeForce4 MX Integrated GPU */
	0x0200, /* Nvidia GeForce3 */
	0x0201, /* Nvidia GeForce3 Ti 200 */
	0x0202, /* Nvidia GeForce3 Ti 500 */
	0x0203, /* Nvidia Quadro DCC */
	0x0250, /* Nvidia GeForce4 Ti 4600 */
	0x0251, /* Nvidia GeForce4 Ti 4400 */
	0x0253, /* Nvidia GeForce4 Ti 4200 */
	0x0258, /* Nvidia Quadro4 900 XGL */
	0x0259, /* Nvidia Quadro4 750 XGL */
	0x025b, /* Nvidia Quadro4 700 XGL */
	0x0280, /* Nvidia GeForce4 Ti 4600 AGP8X */
	0x0281, /* Nvidia GeForce4 Ti 4200 AGP8X */
	0x0282, /* Nvidia GeForce4 Ti 4800SE */
	0x0286, /* Nvidia GeForce4 4200 Go */
	0x0288, /* Nvidia Quadro4 980 XGL */
	0x0289, /* Nvidia Quadro4 780 XGL */
	0x02a0, /* Nvidia GeForce3 Integrated GPU */
	0x0301, /* Nvidia GeForce FX 5800 Ultra */
	0x0302, /* Nvidia GeForce FX 5800 */
	0x0308, /* Nvidia Quadro FX 2000 */
	0x0309, /* Nvidia Quadro FX 1000 */
	0x0311, /* Nvidia GeForce FX 5600 Ultra */
	0x0312, /* Nvidia GeForce FX 5600 */
	0x031a, /* Nvidia GeForce FX 5600 Go */
	0x0321, /* Nvidia GeForce FX 5200 Ultra */
	0x0322, /* Nvidia GeForce FX 5200 */
	0x032b, /* Nvidia Quadro FX 500 */
	0x0330, /* Nvidia GeForce FX 5900 Ultra */
	0x0331, /* Nvidia GeForce FX 5900 */
	0x0338, /* Nvidia Quadro FX 3000 */
	0
};

static uint16 elsa_device_list[] = {
	0x0c60, /* Elsa Gladiac Geforce2 MX */
	0
};

static uint16 nvstbsgs_device_list[] = {
	0x0020, /* Nvidia STB/SGS-Thompson TNT1 */
	0x0028, /* Nvidia STB/SGS-Thompson TNT2 (pro) */
	0x0029, /* Nvidia STB/SGS-Thompson TNT2 Ultra */
	0x002a, /* Nvidia STB/SGS-Thompson TNT2 */
	0x002b, /* Nvidia STB/SGS-Thompson TNT2 */
	0x002c, /* Nvidia STB/SGS-Thompson Vanta (Lt) */
	0x002d, /* Nvidia STB/SGS-Thompson TNT2-M64 (Pro) */
	0x002e, /* Nvidia STB/SGS-Thompson NV06 Vanta */
	0x002f, /* Nvidia STB/SGS-Thompson NV06 Vanta */
	0x00a0, /* Nvidia STB/SGS-Thompson Aladdin TNT2 */
	0
};

static uint16 varisys_device_list[] = {
	0x3503, /* Varisys GeForce4 MX440 */
	0x3505, /* Varisys GeForce4 Ti 4200 */
	0
};

static struct {
	uint16	vendor;
	uint16	*devices;
} SupportedDevices[] = {
	{VENDOR_ID_NVIDIA, nvidia_device_list},
	{VENDOR_ID_ELSA, elsa_device_list},
	{VENDOR_ID_NVSTBSGS, nvstbsgs_device_list},
	{VENDOR_ID_VARISYS, varisys_device_list},
	{0x0000, NULL}
};

static settings current_settings = { // see comments in nv.settings 
	// for driver
	DRIVER_PREFIX ".accelerant",
	false,      // dumprom
	// for accelerant
	0x00000000, // logmask
	0,          // memory
	false,      // usebios
	false,      // hardcursor
	false,		// greensync
};

static void dumprom (void *rom, size_t size)
{
	int fd = open ("/boot/home/" DRIVER_PREFIX ".rom", O_WRONLY | O_CREAT, 0666);
	if (fd < 0) return;
	write (fd, rom, size);
	close (fd);
}

/*return 1, is interrupt has occured*/
int caused_vbi(vuint32 * regs)
{
//	return (ACCR(STATUS)&0x20);
//temp:
return 0;
}

/*clear the interrupt*/
void clear_vbi(vuint32 * regs)
{
//	ACCW(ICLEAR,0x20);
}

void enable_vbi(vuint32 * regs)
{
//	ACCW(IEN,ACCR(IEN)|0x20);
}

void disable_vbi(vuint32 * regs)
{
//	ACCW(IEN,(ACCR(IEN)&~0x20));
//	ACCW(ICLEAR,0x20);
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
	char * rom_temp;
	area_id rom_area;

	/* Nvidia cards have registers in [0] and framebuffer in [1] */
	int registers = 0;
	int frame_buffer = 1;
//	int pseudo_dma = 2;

	/* enable memory mapped IO, disable VGA I/O - this is standard*/
	tmpUlong = get_pci(PCI_command, 4);
	/* enable PCI access */
	tmpUlong |= PCI_command_memory;
	/* enable busmastering */
	tmpUlong |= PCI_command_master;
	/* disable ISA I/O access */
	tmpUlong &= ~PCI_command_io;
	set_pci(PCI_command, 4, tmpUlong);

 	/*work out which version of BeOS is running*/
 	get_system_info(&sysinfo);
 	if (sysinfo.kernel_build_date[0]=='J')/*FIXME - better ID version*/
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
		/* WARNING: Nvidia needs to map regs as viewed from PCI space! */
		(void *) di->pcii.u.h0.base_registers_pci[registers],
		di->pcii.u.h0.base_register_sizes[registers],
		B_ANY_KERNEL_ADDRESS,
 		(si->use_clone_bugfix ? B_READ_AREA|B_WRITE_AREA : 0),
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
		(void *)di->pcii.u.h0.base_registers[frame_buffer],
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

	/* make a copy of ROM for future reference*/
	memcpy (si->rom_mirror, rom_temp, 32768);
	if (current_settings.dumprom) dumprom (rom_temp, 32768);

	/*disable ROM and delete the area*/
	set_pci(PCI_rom_base,4,0);
	delete_area(rom_area);

	/* work out a name for the framebuffer mapping*/
	sprintf(buffer, DEVICE_FORMAT " framebuffer",
		di->pcii.vendor_id, di->pcii.device_id,
		di->pcii.bus, di->pcii.device, di->pcii.function);

	/* map the framebuffer into vmem, using Write Combining*/
	si->fb_area = map_physical_memory(
		buffer,
		/* WARNING: Nvidia needs to map framebuffer as viewed from PCI space! */
		(void *) di->pcii.u.h0.base_registers_pci[frame_buffer],
		di->pcii.u.h0.base_register_sizes[frame_buffer],
		B_ANY_KERNEL_BLOCK_ADDRESS | B_MTR_WC,
		B_READ_AREA + B_WRITE_AREA,
		&(si->framebuffer));

	/*if failed with write combining try again without*/
	if (si->fb_area < 0) {
		si->fb_area = map_physical_memory(
			buffer,
			/* WARNING: Nvidia needs to map framebuffer as viewed from PCI space! */
			(void *) di->pcii.u.h0.base_registers_pci[frame_buffer],
			di->pcii.u.h0.base_register_sizes[frame_buffer],
			B_ANY_KERNEL_BLOCK_ADDRESS,
			B_READ_AREA + B_WRITE_AREA,
			&(si->framebuffer));
	}
		
	/* if there was an error, delete our other areas and pass on error*/
	if (si->fb_area < 0)
	{
		delete_area(si->regs_area);
		si->regs_area = -1;
		return si->fb_area;
	}
//fixme: retest for card coldstart and PCI/virt_mem mapping!!
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
	si->framebuffer = NULL;
	di->regs = NULL;
}

static void probe_devices(void) {
	uint32 pci_index = 0;
	uint32 count = 0;
	device_info *di = pd->di;

	/* while there are more pci devices */
	while ((count < MAX_DEVICES) && ((*pci_bus->get_nth_pci_info)(pci_index, &(di->pcii)) == B_NO_ERROR)) {
		int vendor = 0;
		
		/* if we match a supported vendor */
		while (SupportedDevices[vendor].vendor) {
			if (SupportedDevices[vendor].vendor == di->pcii.vendor_id) {
				uint16 *devices = SupportedDevices[vendor].devices;
				/* while there are more supported devices */
				while (*devices) {
					/* if we match a supported device */
					if (*devices == di->pcii.device_id ) {
						/* publish the device name */
						sprintf(di->name, "graphics/" DEVICE_FORMAT,
							di->pcii.vendor_id, di->pcii.device_id,
							di->pcii.bus, di->pcii.device, di->pcii.function);

						/* remember the name */
						pd->device_names[count] = di->name;
						/* mark the driver as available for R/W open */
						di->is_open = 0;
						/* mark areas as not yet created */
						di->shared_area = -1;
						/* mark pointer to shared data as invalid */
						di->si = NULL;
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
nv_interrupt(void *data)
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
	vuint32		*regs;
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
	di->shared_area = create_area(shared_name, (void **)&(di->si), B_ANY_KERNEL_ADDRESS, ((sizeof(shared_info) + (B_PAGE_SIZE - 1)) & ~(B_PAGE_SIZE - 1)), B_FULL_LOCK, 0);
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

	/* map the device */
	result = map_device(di);
	if (result < 0) goto free_shared;
	result = B_OK;

	/* create a semaphore for vertical blank management */
	si->vblank = create_sem(0, di->name);
	if (si->vblank < 0) {
		result = si->vblank;
		goto unmap;
	}

	/* change the owner of the semaphores to the opener's team */
	/* this is required because apps can't aquire kernel semaphores */
	thid = find_thread(NULL);
	get_thread_info(thid, &thinfo);
	set_sem_owner(si->vblank, thinfo.team);

	/* assign local regs pointer for SAMPLExx() macros */
	regs = di->regs;

	/* disable and clear any pending interrupts */
	disable_vbi(regs);

	/* If there is a valid interrupt line assigned then set up interrupts */
	if ((di->pcii.u.h0.interrupt_pin == 0x00) ||
	    (di->pcii.u.h0.interrupt_line == 0xff) || /* no IRQ assigned */
	    (di->pcii.u.h0.interrupt_line <= 0x02))   /* system IRQ assigned */
	{
		/* interrupt does not exist so exit without installing our handler */
		goto delete_the_sem;
	}
	else
	{
		/* otherwise install our interrupt handler */
		result = install_io_interrupt_handler(di->pcii.u.h0.interrupt_line, nv_interrupt, (void *)di, 0);
		/* bail if we couldn't install the handler */
		if (result != B_OK) goto delete_the_sem;
	}

mark_as_open:
	/* mark the device open */
	di->is_open++;

	/* send the cookie to the opener */
	*cookie = di;
	
	goto done;


delete_the_sem:
	delete_sem(si->vblank);

unmap:
	unmap_device(di);

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
	
	/* remove interrupt handler */
	remove_io_interrupt_handler(di->pcii.u.h0.interrupt_line, nv_interrupt, di);

	/* delete the semaphores, ignoring any errors ('cause the owning team may have died on us) */
	delete_sem(si->vblank);
	si->vblank = -1;

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
		case NV_GET_PRIVATE_DATA: {
			nv_get_private_data *gpd = (nv_get_private_data *)buf;
			if (gpd->magic == NV_PRIVATE_DATA_MAGIC) {
				gpd->shared_info_area = di->shared_area;
				result = B_OK;
			}
		} break;
		case NV_GET_PCI: {
			nv_get_set_pci *gsp = (nv_get_set_pci *)buf;
			if (gsp->magic == NV_PRIVATE_DATA_MAGIC) {
				pci_info *pcii = &(di->pcii);
				gsp->value = get_pci(gsp->offset, gsp->size);
				result = B_OK;
			}
		} break;
		case NV_SET_PCI: {
			nv_get_set_pci *gsp = (nv_get_set_pci *)buf;
			if (gsp->magic == NV_PRIVATE_DATA_MAGIC) {
				pci_info *pcii = &(di->pcii);
				set_pci(gsp->offset, gsp->size, gsp->value);
				result = B_OK;
			}
		} break;
		case NV_DEVICE_NAME: { // apsed
			nv_device_name *dn = (nv_device_name *)buf;
			if (dn->magic == NV_PRIVATE_DATA_MAGIC) {
				strcpy(dn->name, di->name);
				result = B_OK;
			}
		} break;
		case NV_RUN_INTERRUPTS: {
			nv_set_bool_state *ri = (nv_set_bool_state *)buf;
			if (ri->magic == NV_PRIVATE_DATA_MAGIC) {
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

