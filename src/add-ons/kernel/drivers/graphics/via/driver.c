/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.

	Other authors:
	Mark Watson;
	Rudolf Cornelissen 3/2002-1/2016.
*/

/* standard kernel driver stuff */
#include <KernelExport.h>
#include <ISA.h>
#include <PCI.h>
#include <OS.h>
#include <directories.h>
#include <driver_settings.h>
#include <malloc.h>
#include <stdlib.h> // for strtoXX
#include "AGP.h"

/* this is for the standardized portion of the driver API */
/* currently only one operation is defined: B_GET_ACCELERANT_SIGNATURE */
#include <graphic_driver.h>

/* this is for sprintf() */
#include <stdio.h>

/* this is for string compares */
#include <string.h>

/* The private interface between the accelerant and the kernel driver. */
#include "DriverInterface.h"
#include "macros.h"

#define get_pci(o, s) (*pci_bus->read_pci_config)(pcii->bus, pcii->device, pcii->function, (o), (s))
#define set_pci(o, s, v) (*pci_bus->write_pci_config)(pcii->bus, pcii->device, pcii->function, (o), (s), (v))

#define MAX_DEVICES	  8

#define DEVICE_FORMAT "%04x_%04x_%02x%02x%02x" // apsed

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
static int32 eng_interrupt(void *data);

static DeviceData		*pd;
static isa_module_info	*isa_bus = NULL;
static pci_module_info	*pci_bus = NULL;
static agp_gart_module_info *agp_bus = NULL;
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

#define VENDOR_ID_VIA	0x1106 /* Via */

static uint16 via_device_list[] = {
	0x3022, /* CLE266 Unichrome Pro (CLE3022) */
	0x3108, /* K8M800 Unichrome Pro (unknown chiptype) */
	0x3122, /* CLE266 Unichrome Pro (CLE3122) */
	0x3205, /* KM400 Unichrome (VT3205) */
//	0x3344, /* P4M800 Pro (VT3344) */
	0x7205, /* KM400 Unichrome (VT7205) */
	0
};

static struct {
	uint16	vendor;
	uint16	*devices;
} SupportedDevices[] = {
	{VENDOR_ID_VIA, via_device_list},
	{0x0000, NULL}
};

static settings current_settings = { // see comments in skel.settings
	// for driver
	DRIVER_PREFIX ".accelerant",
	false,      // dumprom
	// for accelerant
	0x00000000, // logmask
	0,          // memory
	true,       // usebios
	true,       // hardcursor
	false,		// switchhead
	false,		// force_pci
	false,		// unhide_fw
	true,		// pgm_panel
};

static void dumprom (void *rom, uint32 size)
{
	int fd;
	uint32 cnt;

	fd = open (kUserDirectory "/" DRIVER_PREFIX ".rom",
		O_WRONLY | O_CREAT, 0666);
	if (fd < 0) return;

	/* apparantly max. 32kb may be written at once;
	 * the ROM size is a multiple of that anyway. */
	for (cnt = 0; (cnt < size); cnt += 32768)
		write (fd, ((void *)(((uint8 *)rom) + cnt)), 32768);
	close (fd);
}

/* return 1 if vblank interrupt has occured */
static int caused_vbi(vuint32 * regs)
{
//	return (ENG_REG32(RG32_CRTC_INTS) & 0x00000001);
return 0;
}

/* clear the vblank interrupt */
static void clear_vbi(vuint32 * regs)
{
//	ENG_REG32(RG32_CRTC_INTS) = 0x00000001;
}

static void enable_vbi(vuint32 * regs)
{
	/* clear the vblank interrupt */
//	ENG_REG32(RG32_CRTC_INTS) = 0x00000001;
	/* enable nVidia interrupt source vblank */
//	ENG_REG32(RG32_CRTC_INTE) |= 0x00000001;
	/* enable nVidia interrupt system hardware (b0-1) */
//	ENG_REG32(RG32_MAIN_INTE) = 0x00000001;
}

static void disable_vbi(vuint32 * regs)
{
	/* disable nVidia interrupt source vblank */
//	ENG_REG32(RG32_CRTC_INTE) &= 0xfffffffe;
	/* clear the vblank interrupt */
//	ENG_REG32(RG32_CRTC_INTS) = 0x00000001;
	/* disable nVidia interrupt system hardware (b0-1) */
//	ENG_REG32(RG32_MAIN_INTE) = 0x00000000;
}

/*
	init_hardware() - Returns B_OK if one is
	found, otherwise returns B_ERROR so the driver will be unloaded.
*/
status_t
init_hardware(void) {
	long		pci_index = 0;
	pci_info	pcii;
	bool		found_one = false;

	/* choke if we can't find the PCI bus */
	if (get_module(B_PCI_MODULE_NAME, (module_info **)&pci_bus) != B_OK)
		return B_ERROR;

	/* choke if we can't find the ISA bus */
	if (get_module(B_ISA_MODULE_NAME, (module_info **)&isa_bus) != B_OK)
	{
		put_module(B_PCI_MODULE_NAME);
		return B_ERROR;
	}

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

						found_one = true;
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
		current_settings.switchhead = get_driver_boolean_parameter (settings_handle, "switchhead", false, false);
		current_settings.force_pci = get_driver_boolean_parameter (settings_handle, "force_pci", false, false);
		current_settings.unhide_fw = get_driver_boolean_parameter (settings_handle, "unhide_fw", false, false);
		current_settings.pgm_panel = get_driver_boolean_parameter (settings_handle, "pgm_panel", false, false);

		unload_driver_settings (settings_handle);
	}

	/* get a handle for the pci bus */
	if (get_module(B_PCI_MODULE_NAME, (module_info **)&pci_bus) != B_OK)
		return B_ERROR;

	/* get a handle for the isa bus */
	if (get_module(B_ISA_MODULE_NAME, (module_info **)&isa_bus) != B_OK)
	{
		put_module(B_PCI_MODULE_NAME);
		return B_ERROR;
	}

	/* get a handle for the agp bus if it exists */
	get_module(B_AGP_GART_MODULE_NAME, (module_info **)&agp_bus);

	/* driver private data */
	pd = (DeviceData *)calloc(1, sizeof(DeviceData));
	if (!pd) {
		if (agp_bus)
			put_module(B_AGP_GART_MODULE_NAME);
		put_module(B_ISA_MODULE_NAME);
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
	put_module(B_ISA_MODULE_NAME);

	/* put the agp module away if it's there */
	if (agp_bus)
		put_module(B_AGP_GART_MODULE_NAME);
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

	/* variables for making copy of ROM */
	uint8* rom_temp;
	area_id rom_area;

	/* Nvidia cards have registers in [0] and framebuffer in [1] */
	int registers = 1;
	int frame_buffer = 0;
//	int pseudo_dma = 2;

	/* enable memory mapped IO, disable VGA I/O - this is defined in the PCI standard */
	tmpUlong = get_pci(PCI_command, 2);
	/* enable PCI access */
	tmpUlong |= PCI_command_memory;
	/* enable busmastering */
	tmpUlong |= PCI_command_master;
	/* disable ISA I/O access */
	tmpUlong &= ~PCI_command_io;
	set_pci(PCI_command, 2, tmpUlong);

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
		/* WARNING: Nvidia needs to map regs as viewed from PCI space! */
		di->pcii.u.h0.base_registers_pci[registers],
		di->pcii.u.h0.base_register_sizes[registers],
		B_ANY_KERNEL_ADDRESS,
		B_CLONEABLE_AREA | (si->use_clone_bugfix ? B_READ_AREA|B_WRITE_AREA : 0),
		(void **)&(di->regs));
 	si->clone_bugfix_regs = (uint32 *) di->regs;

	/* if mapping registers to vmem failed then pass on error */
	if (si->regs_area < 0) return si->regs_area;

	/* work out a name for the ROM mapping*/
	sprintf(buffer, DEVICE_FORMAT " rom",
		di->pcii.vendor_id, di->pcii.device_id,
		di->pcii.bus, di->pcii.device, di->pcii.function);

	/* disable ROM shadowing, we want the guaranteed exact contents of the chip */
	/* warning:
	 * don't touch: (confirmed) NV04, NV05, NV05-M64, NV11 all shutoff otherwise.
	 * NV18, NV28 and NV34 keep working.
	 * confirmed NV28 and NV34 to use upper part of shadowed ROM for scratch purposes,
	 * however the actual ROM content (so the used part) is intact (confirmed). */
	//set_pci(ENCFG_ROMSHADOW, 4, 0);

	/* get ROM memory mapped base adress - this is defined in the PCI standard */
	tmpUlong = get_pci(PCI_rom_base, 4);
	if (tmpUlong)
	{
		/* ROM was assigned an adress, so enable ROM decoding - see PCI standard */
		tmpUlong |= 0x00000001;
		set_pci(PCI_rom_base, 4, tmpUlong);

		rom_area = map_physical_memory(
			buffer,
			di->pcii.u.h0.rom_base_pci,
			di->pcii.u.h0.rom_size,
			B_ANY_KERNEL_ADDRESS,
			B_READ_AREA,
			(void **)&(rom_temp)
		);

		/* check if we got the BIOS signature (might fail on laptops..) */
		if (rom_temp[0]!=0x55 || rom_temp[1]!=0xaa)
		{
			/* apparantly no ROM is mapped here */
			delete_area(rom_area);
			rom_area = -1;
			/* force using ISA legacy map as fall-back */
			tmpUlong = 0x00000000;
		}
	}

	if (!tmpUlong)
	{
		/* ROM was not assigned an adress, fetch it from ISA legacy memory map! */
		rom_area = map_physical_memory(
			buffer,
			0x000c0000,
			65536,
			B_ANY_KERNEL_ADDRESS,
			B_READ_AREA,
			(void **)&(rom_temp)
		);
	}

	/* if mapping ROM to vmem failed then clean up and pass on error */
	if (rom_area < 0) {
		delete_area(si->regs_area);
		si->regs_area = -1;
		return rom_area;
	}

	/* dump ROM to file if selected in skel.settings
	 * (ROM always fits in 64Kb: checked TNT1 - FX5950) */
	if (current_settings.dumprom) dumprom (rom_temp, 65536);
	/* make a copy of ROM for future reference */
	memcpy (si->rom_mirror, rom_temp, 65536);

	/* disable ROM decoding - this is defined in the PCI standard, and delete the area */
	tmpUlong = get_pci(PCI_rom_base, 4);
	tmpUlong &= 0xfffffffe;
	set_pci(PCI_rom_base, 4, tmpUlong);
	delete_area(rom_area);

	/* work out a name for the framebuffer mapping*/
	sprintf(buffer, DEVICE_FORMAT " framebuffer",
		di->pcii.vendor_id, di->pcii.device_id,
		di->pcii.bus, di->pcii.device, di->pcii.function);

	/* map the framebuffer into vmem, using Write Combining*/
	si->fb_area = map_physical_memory(
		buffer,
		/* WARNING: Nvidia needs to map framebuffer as viewed from PCI space! */
		di->pcii.u.h0.base_registers_pci[frame_buffer],
		di->pcii.u.h0.base_register_sizes[frame_buffer],
		B_ANY_KERNEL_BLOCK_ADDRESS | B_MTR_WC,
		B_READ_AREA | B_WRITE_AREA | B_CLONEABLE_AREA,
		&(si->framebuffer));

	/*if failed with write combining try again without*/
	if (si->fb_area < 0) {
		si->fb_area = map_physical_memory(
			buffer,
			/* WARNING: Nvidia needs to map framebuffer as viewed from PCI space! */
			di->pcii.u.h0.base_registers_pci[frame_buffer],
			di->pcii.u.h0.base_register_sizes[frame_buffer],
			B_ANY_KERNEL_BLOCK_ADDRESS,
			B_READ_AREA | B_WRITE_AREA | B_CLONEABLE_AREA,
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
eng_interrupt(void *data)
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
	di->shared_area = create_area(shared_name, (void **)&(di->si), B_ANY_KERNEL_ADDRESS, ((sizeof(shared_info) + (B_PAGE_SIZE - 1)) & ~(B_PAGE_SIZE - 1)), B_FULL_LOCK,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA | B_CLONEABLE_AREA);
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

	/* device at bus #0, device #0, function #0 holds byte value at byte-index 0xf6 */
	si->ps.chip_rev = ((*pci_bus->read_pci_config)(0, 0, 0, 0xf6, 1));

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
		/* we are aborting! */
		/* Note: the R4 graphics driver kit lacks this statement!! */
		result = B_ERROR;
		/* interrupt does not exist so exit without installing our handler */
		goto delete_the_sem;
	}
	else
	{
		/* otherwise install our interrupt handler */
		result = install_io_interrupt_handler(di->pcii.u.h0.interrupt_line, eng_interrupt, (void *)di, 0);
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
	remove_io_interrupt_handler(di->pcii.u.h0.interrupt_line, eng_interrupt, di);

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
	uint32 tmpUlong;

	switch (msg) {
		/* the only PUBLIC ioctl */
		case B_GET_ACCELERANT_SIGNATURE: {
			char *sig = (char *)buf;
			strcpy(sig, current_settings.accelerant);
			result = B_OK;
		} break;

		/* PRIVATE ioctl from here on */
		case ENG_GET_PRIVATE_DATA: {
			eng_get_private_data *gpd = (eng_get_private_data *)buf;
			if (gpd->magic == VIA_PRIVATE_DATA_MAGIC) {
				gpd->shared_info_area = di->shared_area;
				result = B_OK;
			}
		} break;
		case ENG_GET_PCI: {
			eng_get_set_pci *gsp = (eng_get_set_pci *)buf;
			if (gsp->magic == VIA_PRIVATE_DATA_MAGIC) {
				pci_info *pcii = &(di->pcii);
				gsp->value = get_pci(gsp->offset, gsp->size);
				result = B_OK;
			}
		} break;
		case ENG_SET_PCI: {
			eng_get_set_pci *gsp = (eng_get_set_pci *)buf;
			if (gsp->magic == VIA_PRIVATE_DATA_MAGIC) {
				pci_info *pcii = &(di->pcii);
				set_pci(gsp->offset, gsp->size, gsp->value);
				result = B_OK;
			}
		} break;
		case ENG_DEVICE_NAME: { // apsed
			eng_device_name *dn = (eng_device_name *)buf;
			if (dn->magic == VIA_PRIVATE_DATA_MAGIC) {
				strcpy(dn->name, di->name);
				result = B_OK;
			}
		} break;
		case ENG_RUN_INTERRUPTS: {
			eng_set_bool_state *ri = (eng_set_bool_state *)buf;
			if (ri->magic == VIA_PRIVATE_DATA_MAGIC) {
				vuint32 *regs = di->regs;
				if (ri->do_it) {
					enable_vbi(regs);
				} else {
					disable_vbi(regs);
				}
				result = B_OK;
			}
		} break;
		case ENG_GET_NTH_AGP_INFO: {
			eng_nth_agp_info *nai = (eng_nth_agp_info *)buf;
			if (nai->magic == VIA_PRIVATE_DATA_MAGIC) {
				nai->exist = false;
				nai->agp_bus = false;
				if (agp_bus) {
					nai->agp_bus = true;
					if ((*agp_bus->get_nth_agp_info)(nai->index, &(nai->agpi)) == B_NO_ERROR) {
						nai->exist = true;
					}
				}
				result = B_OK;
			}
		} break;
		case ENG_ENABLE_AGP: {
			eng_cmd_agp *nca = (eng_cmd_agp *)buf;
			if (nca->magic == VIA_PRIVATE_DATA_MAGIC) {
				if (agp_bus) {
					nca->agp_bus = true;
					nca->cmd = agp_bus->set_agp_mode(nca->cmd);
				} else {
					nca->agp_bus = false;
					nca->cmd = 0;
				}
				result = B_OK;
			}
		} break;
		case ENG_ISA_OUT: {
			eng_in_out_isa *io_isa = (eng_in_out_isa *)buf;
			if (io_isa->magic == VIA_PRIVATE_DATA_MAGIC) {
				pci_info *pcii = &(di->pcii);

				/* lock the driver:
				 * no other graphics card may have ISA I/O enabled when we enter */
				AQUIRE_BEN(pd->kernel);

				/* enable ISA I/O access */
				tmpUlong = get_pci(PCI_command, 2);
				tmpUlong |= PCI_command_io;
				set_pci(PCI_command, 2, tmpUlong);

				if (io_isa->size == 1)
  					isa_bus->write_io_8(io_isa->adress, (uint8)io_isa->data);
   				else
   					isa_bus->write_io_16(io_isa->adress, io_isa->data);
  				result = B_OK;

				/* disable ISA I/O access */
				tmpUlong = get_pci(PCI_command, 2);
				tmpUlong &= ~PCI_command_io;
				set_pci(PCI_command, 2, tmpUlong);

				/* end of critical section */
				RELEASE_BEN(pd->kernel);
   			}
		} break;
		case ENG_ISA_IN: {
			eng_in_out_isa *io_isa = (eng_in_out_isa *)buf;
			if (io_isa->magic == VIA_PRIVATE_DATA_MAGIC) {
				pci_info *pcii = &(di->pcii);

				/* lock the driver:
				 * no other graphics card may have ISA I/O enabled when we enter */
				AQUIRE_BEN(pd->kernel);

				/* enable ISA I/O access */
				tmpUlong = get_pci(PCI_command, 2);
				tmpUlong |= PCI_command_io;
				set_pci(PCI_command, 2, tmpUlong);

				if (io_isa->size == 1)
	   				io_isa->data = isa_bus->read_io_8(io_isa->adress);
	   			else
	   				io_isa->data = isa_bus->read_io_16(io_isa->adress);
   				result = B_OK;

				/* disable ISA I/O access */
				tmpUlong = get_pci(PCI_command, 2);
				tmpUlong &= ~PCI_command_io;
				set_pci(PCI_command, 2, tmpUlong);

				/* end of critical section */
				RELEASE_BEN(pd->kernel);
   			}
		} break;
	}
	return result;
}
