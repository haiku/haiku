/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.

	Other authors:
	Mark Watson;
	Apsed;
	Rudolf Cornelissen 5/2002-6/2003.
*/

/* standard kernel driver stuff */
#include <KernelExport.h>
#include <ISA.h>
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
#include "nm_macros.h"

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
	vuint32		*regs, *regs2;		/* kernel's pointer to memory mapped registers */
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
static int32 mn_interrupt(void *data);

static DeviceData		*pd;
static pci_module_info	*pci_bus;
static isa_module_info	*isa_bus;

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

#define VENDOR_ID			0x10c8	/* Neomagic inc. */

static uint16 mn_device_list[] = {
	0x0001,	/* MagicGraph 128 (NM2070) */
	0x0002,	/* MagicGraph 128V (NM2090)	0x42 */
	0x0003,	/* MagicGraph 128ZV (NM2093) 0x43 */
	0x0083, /* MagicGraph 128ZV+ (NM2097) */
	0x0004, /* MagicGraph 128XD (NM2160) 0x44 */
	0x0005,	/* MagicMedia 256AV (NM2200) 0x45 */
	0x0025,	/* MagicMedia 256AV+ (NM2230) */
	0x0006,	/* MagicMedia 256ZX (NM2360) */
	0x0016,	/* MagicMedia 256XL+ (NM2380) */
	0
};
 
static struct {
	uint16	vendor;
	uint16	*devices;
} SupportedDevices[] = {
	{VENDOR_ID, mn_device_list},
	{0x0000, NULL}
};

static settings current_settings = { // see comments in nm.settings 
	// for driver
	DRIVER_PREFIX ".accelerant",
	false,      // dumprom
	// for accelerant
	0x00000000, // logmask
	0,          // memory
	false,      // usebios
	false,      // hardcursor
};

static void dumprom (void *rom, size_t size)
{
	int fd = open ("/boot/home/" DRIVER_PREFIX ".rom", O_WRONLY | O_CREAT, 0666);
	if (fd < 0) return;
	write (fd, rom, size);
	close (fd);
}

/*return 1, is interrupt has occured*/
int caused_vbi(vuint32 * regs, vuint32 * regs2)
{
//	return (ACCR(STATUS)&0x20);
return 0;
}

/*clear the interrupt*/
void clear_vbi(vuint32 * regs, vuint32 * regs2)
{
//	ACCW(ICLEAR,0x20);
}

void enable_vbi(vuint32 * regs, vuint32 * regs2)
{
//	ACCW(IEN,ACCR(IEN)|0x20);
}

void disable_vbi(vuint32 * regs, vuint32 * regs2)
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

	/* choke if we can't find the ISA bus */
	if (get_module(B_ISA_MODULE_NAME, (module_info **)&isa_bus) != B_OK)
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
	/* put away the module managers */
	put_module(B_PCI_MODULE_NAME);
	put_module(B_ISA_MODULE_NAME);

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

		unload_driver_settings (settings_handle);
	}

	/* get a handle for the pci bus */
	if (get_module(B_PCI_MODULE_NAME, (module_info **)&pci_bus) != B_OK)
		return B_ERROR;

	/* get a handle for the isa bus */
	if (get_module(B_ISA_MODULE_NAME, (module_info **)&isa_bus) != B_OK)
		return B_ERROR;

	/* driver private data */
	pd = (DeviceData *)calloc(1, sizeof(DeviceData));
	if (!pd) {
		put_module(B_PCI_MODULE_NAME);
		put_module(B_ISA_MODULE_NAME);
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

	/* put the pci modules away */
	put_module(B_PCI_MODULE_NAME);
	put_module(B_ISA_MODULE_NAME);
}

static status_t map_device(device_info *di)
{
	char buffer[B_OS_NAME_LENGTH]; /*memory for device name*/
	shared_info *si = di->si;
	uint32	tmpUlong;
	pci_info *pcii = &(di->pcii);
	system_info sysinfo;

	/*variables for making copy of ROM*/
	char * rom_temp;
	area_id rom_area;

	int frame_buffer = 0;
	/* only NM2097 and later cards have two register area's:
	 * older cards have their PCI registers in the frame_buffer space */
	int registers = 1;
	int registers2 = 2;

	/* enable memory mapped IO, disable VGA I/O - this is standard*/
	tmpUlong = get_pci(PCI_command, 4);
	/* make sure ISA access is enabled: all Neomagic cards still partly rely on this */
	tmpUlong |= PCI_command_io;
	/* make sure PCI access is enabled */
	tmpUlong |= PCI_command_memory;
	tmpUlong |= PCI_command_master;
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

	/* map card's register area's */
	switch (di->pcii.device_id)
	{
	case 0x0001:
	case 0x0002:
	case 0x0003:
		/* NM2070, NM2090 and NM2093 have no seperate register area's,
		 * so do nothing here. */
		break;
	default:
		/* work out a name for the register mapping */
		sprintf(buffer, DEVICE_FORMAT " regs",
			di->pcii.vendor_id, di->pcii.device_id,
			di->pcii.bus, di->pcii.device, di->pcii.function);

		/* get a virtual memory address for the registers*/
		si->regs_area = map_physical_memory(
			buffer,
			(void *) di->pcii.u.h0.base_registers[registers],
			di->pcii.u.h0.base_register_sizes[registers],
			B_ANY_KERNEL_ADDRESS,
 			(si->use_clone_bugfix ? B_READ_AREA|B_WRITE_AREA : 0),
			(void **)&(di->regs));
 		si->clone_bugfix_regs = (uint32 *) di->regs;

		/* if mapping registers to vmem failed then pass on error */
		if (si->regs_area < 0) return si->regs_area;

		/* work out a name for the register mapping */
		sprintf(buffer, DEVICE_FORMAT " regs2",
			di->pcii.vendor_id, di->pcii.device_id,
			di->pcii.bus, di->pcii.device, di->pcii.function);

		si->regs2_area = map_physical_memory(
			buffer,
			(void *) di->pcii.u.h0.base_registers[registers2],
			di->pcii.u.h0.base_register_sizes[registers2],
			B_ANY_KERNEL_ADDRESS,
			(si->use_clone_bugfix ? B_READ_AREA|B_WRITE_AREA : 0),
			(void **)&(di->regs2));
	 	si->clone_bugfix_regs2 = (uint32 *) di->regs2;

		/* if mapping registers to vmem failed then pass on error */
		if (si->regs2_area < 0)
		{
			delete_area(si->regs_area);
			si->regs_area = -1;

			return si->regs2_area;
		}
		break;
	}

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
		if (si->regs_area >= 0)
		{
			delete_area(si->regs_area);
			si->regs_area = -1;
		}
		if (si->regs2_area >= 0)
		{
			delete_area(si->regs2_area);
			si->regs2_area = -1;
		}
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
		(void *) di->pcii.u.h0.base_registers[frame_buffer],
		di->pcii.u.h0.base_register_sizes[frame_buffer],
		B_ANY_KERNEL_BLOCK_ADDRESS | B_MTR_WC,
		B_READ_AREA + B_WRITE_AREA,
		&(si->framebuffer));

	/*if failed with write combining try again without*/
	if (si->fb_area < 0) {
		si->fb_area = map_physical_memory(
			buffer,
			(void *) di->pcii.u.h0.base_registers[frame_buffer],
			di->pcii.u.h0.base_register_sizes[frame_buffer],
			B_ANY_KERNEL_BLOCK_ADDRESS,
			B_READ_AREA + B_WRITE_AREA,
			&(si->framebuffer));
	}
		
	/* if there was an error, delete our other areas and pass on error*/
	if (si->fb_area < 0)
	{
		if (si->regs_area >= 0)
		{
			delete_area(si->regs_area);
			si->regs_area = -1;
		}
		if (si->regs2_area >= 0)
		{
			delete_area(si->regs2_area);
			si->regs2_area = -1;
		}
		return si->fb_area;
	}

	/* older cards have their registers in the framebuffer area */
	switch (di->pcii.device_id)
	{
	case 0x0001:
		/* NM2070 cards */
		di->regs = (uint32 *)((uint8 *)si->framebuffer + 0x100000);
		break;
	case 0x0002:
	case 0x0003:
		/* NM2090 and NM2093 cards */
		di->regs = (uint32 *)((uint8 *)si->framebuffer + 0x200000);
		break;
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
	if (si->regs2_area >= 0) delete_area(si->regs2_area);
	if (si->regs_area >= 0) delete_area(si->regs_area);
	if (si->fb_area >= 0) delete_area(si->fb_area);
	si->regs2_area = si->regs_area = si->fb_area = -1;
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

static uint32 thread_interrupt_work(int32 *flags, vuint32 *regs, vuint32 *regs2, shared_info *si) {
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
mn_interrupt(void *data)
{
	int32 handled = B_UNHANDLED_INTERRUPT;
	device_info *di = (device_info *)data;
	shared_info *si = di->si;
	int32 *flags = &(si->flags);
	vuint32 *regs, *regs2;

	/* is someone already handling an interrupt for this device? */
	if (atomic_or(flags, SKD_HANDLER_INSTALLED) & SKD_HANDLER_INSTALLED) {
		goto exit0;
	}
	/* get regs */
	regs = di->regs;
	regs2 = di->regs2;

	/* was it a VBI? */
	if (caused_vbi(regs, regs2)) {
		/*clear the interrupt*/
		clear_vbi(regs, regs2);
		/*release the semaphore*/
		handled = thread_interrupt_work(flags, regs, regs2, si);
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
	vuint32		*regs, *regs2;
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
	regs2 = di->regs2;

	/* disable and clear any pending interrupts */
	disable_vbi(regs, regs2);

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
		result = install_io_interrupt_handler(di->pcii.u.h0.interrupt_line, mn_interrupt, (void *)di, 0);
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
	vuint32 *regs2 = di->regs2;

	/* lock the driver */
	AQUIRE_BEN(pd->kernel);

	/* if opened multiple times, decrement the open count and exit */
	if (di->is_open > 1)
		goto unlock_and_exit;

	/* disable and clear any pending interrupts */
	disable_vbi(regs, regs2);
	
	/* remove interrupt handler */
	remove_io_interrupt_handler(di->pcii.u.h0.interrupt_line, mn_interrupt, di);

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
		case MN_GET_PRIVATE_DATA: {
			mn_get_private_data *gpd = (mn_get_private_data *)buf;
			if (gpd->magic == MN_PRIVATE_DATA_MAGIC) {
				gpd->shared_info_area = di->shared_area;
				result = B_OK;
			}
		} break;
		case MN_GET_PCI: {
			mn_get_set_pci *gsp = (mn_get_set_pci *)buf;
			if (gsp->magic == MN_PRIVATE_DATA_MAGIC) {
				pci_info *pcii = &(di->pcii);
				gsp->value = get_pci(gsp->offset, gsp->size);
				result = B_OK;
			}
		} break;
		case MN_SET_PCI: {
			mn_get_set_pci *gsp = (mn_get_set_pci *)buf;
			if (gsp->magic == MN_PRIVATE_DATA_MAGIC) {
				pci_info *pcii = &(di->pcii);
				set_pci(gsp->offset, gsp->size, gsp->value);
				result = B_OK;
			}
		} break;
		case MN_DEVICE_NAME: {
			mn_device_name *dn = (mn_device_name *)buf;
			if (dn->magic == MN_PRIVATE_DATA_MAGIC) {
				strcpy(dn->name, di->name);
				result = B_OK;
			}
		} break;
		case MN_RUN_INTERRUPTS: {
			mn_set_bool_state *ri = (mn_set_bool_state *)buf;
			if (ri->magic == MN_PRIVATE_DATA_MAGIC) {
				vuint32 *regs = di->regs;
				vuint32 *regs2 = di->regs2;
				if (ri->do_it) {
					enable_vbi(regs, regs2);
				} else {
					disable_vbi(regs, regs2);
				}
				result = B_OK;
			}
		} break;
		case MN_ISA_OUT: {
			mn_in_out_isa *io_isa = (mn_in_out_isa *)buf;
			if (io_isa->magic == MN_PRIVATE_DATA_MAGIC) {
				if (io_isa->size == 1)
  					isa_bus->write_io_8(io_isa->adress, (uint8)io_isa->data);
   				else
   					isa_bus->write_io_16(io_isa->adress, io_isa->data);
  				result = B_OK;
   			}
		} break;
		case MN_ISA_IN: {
			mn_in_out_isa *io_isa = (mn_in_out_isa *)buf;
			if (io_isa->magic == MN_PRIVATE_DATA_MAGIC) {
				if (io_isa->size == 1)
	   				io_isa->data = isa_bus->read_io_8(io_isa->adress);
	   			else
	   				io_isa->data = isa_bus->read_io_16(io_isa->adress);
   				result = B_OK;
   			}
		} break;
	}
	return result;
}
