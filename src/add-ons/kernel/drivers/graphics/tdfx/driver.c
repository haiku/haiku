/*
    Copyright 1999, Be Incorporated.   All Rights Reserved.
    This file may be used under the terms of the Be Sample Code License.

   - PPC Port: Andreas Drewke (andreas_dr@gmx.de)
   - Voodoo3Driver 0.02 (c) by Carwyn Jones (2002)
   
*/

/* standard kernel driver stuff */
#include <KernelExport.h>
#include <PCI.h>
#include <OS.h>
#include <malloc.h>
#include <ByteOrder.h>

/* this is for the standardized portion of the driver API */
/* currently only one operation is defined: B_GET_ACCELERANT_SIGNATURE */
#include <graphic_driver.h>

/* this is for sprintf() */
#include <stdio.h>

/* this is for string compares */
#include <string.h>

/* The private interface between the accelerant and the kernel driver. */
#include <video_overlay.h>
#include <DriverInterface.h>

#define DEBUG 1
#if DEBUG > 0
#define ddprintf(a)    dprintf a
#else
#define    ddprintf(a)
#endif

#define get_pci(o, s)    (*pci_bus->read_pci_config)(pcii->bus, pcii->device, pcii->function, (o), (s))
#define set_pci(o, s, v) (*pci_bus->write_pci_config)(pcii->bus, pcii->device, pcii->function, (o), (s), (v))

#define MAX_DEVICES    8

/* Tell the kernel what revision of the driver API we support */
int32    api_version = 2;

/* these structures are private to the kernel driver */
typedef struct device_info device_info;

#if defined(POST_R4_0)
typedef struct
{
    timer        te;            /* timer entry for add_timer() */
    device_info *di;            /* pointer to the owning device */
    bigtime_t    when_target;   /* when we're supposed to wake up */
} timer_info;
#endif

struct device_info
{
    uint32      is_open;        /* a count of how many times the devices has been opened */
    area_id     shared_area;    /* the area shared between the driver and all of the accelerants */
    shared_info *si;            /* a pointer to the shared area, for convenience */
    vuint32     *regs;          /* kernel's pointer to memory mapped registers */
    int32       can_interrupt;  /* when we're faking interrupts, let's us know if we should generate one */
#if defined(POST_R4_0)
    timer_info  ti_a;           /* a pool of two timer managment buffers */
    timer_info  ti_b;
    timer_info  *current_timer; /* the timer buffer that's currently in use */
#else
    thread_id   tid;
#endif
#if DEBUG > 0
    uint32      interrupt_count;    /* if we're debugging, a count of how many times the interrupt handler has
                                       been called for this device */
#endif
    pci_info    pcii;                   /* a convenience copy of the pci info for this device */
    char        name[B_OS_NAME_LENGTH]; /* where we keep the name of the device for publishing and comparing */
};

typedef struct
{
#if DEBUG > 0
    uint32      total_interrupts;   /* total number of interrupts seen by our handler */
#endif
    uint32      count;              /* number of devices actually found */
    benaphore   kernel;             /* for serializing opens/closes */
    char        *device_names[MAX_DEVICES+1];   /* device name pointer storage */
    device_info di[MAX_DEVICES];    /* device specific stuff */
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
static int32 voodoo_interrupt(void *data);

#if DEBUG > 0
static int voodoodump(int argc, char **argv);
#endif

static DeviceData       *pd;
static pci_module_info  *pci_bus;
static device_hooks graphics_device_hooks =
{
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

#define VENDOR_ID			0x121A	/* 3dfx vendor */

#define BANSHEE 0x3
#define VOODOO3 0x5

static uint16 voodoo_device_list[] =
{
	BANSHEE, // Voodoo Banshee
	VOODOO3, // Voodoo 
    0
};

static struct
{
    uint16  vendor;
    uint16  *devices;
} SupportedDevices[] =
{
    {VENDOR_ID, voodoo_device_list},
    {0x0000, NULL}
};

// IO Access Macros for accessing IOBase

#if __INTEL__

#define outportb(offset, value)	(*pci_bus->write_io_8)((offset), (value))
#define outportw(offset, value)	(*pci_bus->write_io_16)((offset), (value))
#define outportl(offset, value)	(*pci_bus->write_io_32)((offset), (value))
#define inportb(offset)			((*pci_bus->read_io_8)(offset))
#define inportw(offset)			((*pci_bus->read_io_16)(offset))
#define inportl(offset)			((*pci_bus->read_io_32)(offset))

#elif __POWERPC__

#define inportb(offset)  			(*((volatile uint8*)(offset))) //__eieio()
#define outportb(offset, value)  	(*((volatile uint8*)(offset)) = (value)); //__eieio()
#define inportw(offset)  			B_LENDIAN_TO_HOST_INT16((*((volatile uint16*)(offset)))) //__eieio()
#define outportw(offset, value)  	(*((volatile uint16*)(offset)) = B_HOST_TO_LENDIAN_INT16((value))); //__eieio()
#define inportl(offset)  			B_LENDIAN_TO_HOST_INT32((*((volatile uint32*)(offset)))) //__eieio()
#define outportl(offset, value)  	(*((volatile uint32*)(offset)) = B_HOST_TO_LENDIAN_INT32((value))); //__eieio()

#endif


/* TODO */
#define voodoo3_disableirq() 
#define voodoo3_enableirq() 
#define voodoo3_clearirq() 
#define voodoo3_irqpending() 0


/*
    init_hardware() - Returns B_OK if one is
    found, otherwise returns B_ERROR so the driver will be unloaded.
*/


status_t
init_hardware(void)
{
    long        pci_index = 0;
    pci_info    pcii;
    bool        found_one = FALSE;
    
    /* choke if we can't find the PCI bus */
    if (get_module(B_PCI_MODULE_NAME, (module_info **)&pci_bus) != B_OK)
        return B_ERROR;

    /* while there are more pci devices */
    while ((*pci_bus->get_nth_pci_info)(pci_index, &pcii) == B_NO_ERROR)
    {
        int vendor = 0;
        
        ddprintf(("TDFXV3: init_hardware(): checking pci index %ld, device 0x%04x/0x%04x\n", pci_index, pcii.vendor_id, pcii.device_id));
        /* if we match a supported vendor */
        while (SupportedDevices[vendor].vendor)
        {
            if (SupportedDevices[vendor].vendor == pcii.vendor_id)
            {
                uint16 *devices = SupportedDevices[vendor].devices;
                /* while there are more supported devices */
                while (*devices)
                {
                    /* if we match a supported device */
                    if (*devices == pcii.device_id )
                    {
                        
                        ddprintf(("TDFXV3: we support this device\n"));
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
    ddprintf(("TDFXV3: init_hardware - no supported devices\n"));

done:
    /* put away the module manager */
    put_module(B_PCI_MODULE_NAME);
    return (found_one ? B_OK : B_ERROR);
}

status_t
init_driver(void)
{
    ddprintf(("TDFXV3: enter init_driver\n"));
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
#if DEBUG > 0
    add_debugger_command("voodoodump", voodoodump, "dump VOODOO kernel driver persistant data");
#endif
    ddprintf(("TDFXV3: leave init_driver\n"));
    return B_OK;
}

const char **
publish_devices(void)
{
    /* return the list of supported devices */
    return (const char **)pd->device_names;
}

device_hooks *
find_device(const char *name)
{
    int index = 0;
    while (pd->device_names[index])
    {
        if (strcmp(name, pd->device_names[index]) == 0)
            return &graphics_device_hooks;
        index++;
    }
    return NULL;

}

void uninit_driver(void)
{
#if DEBUG > 0
    remove_debugger_command("voodoodump", voodoodump);
#endif

    /* free the driver data */
    DELETE_BEN(pd->kernel);
    free(pd);
    pd = NULL;

    /* put the pci module away */
    put_module(B_PCI_MODULE_NAME);
}

static status_t map_device(device_info *di)
{
    char buffer[B_OS_NAME_LENGTH];
    shared_info *si = di->si;
    uint16 tmp;
    pci_info *pcii = &(di->pcii);
#if __POWERPC__
	int32 offset=0;
#endif
	int32 _regs = di->pcii.u.h0.base_registers[0];
	int32 iobase = di->pcii.u.h0.base_registers[2];
	int32 fb = di->pcii.u.h0.base_registers[1];

#if __POWERPC__
	/* On PowerPC we would get INSANE Values, so we assume them */
	int32 _regs_size = 1 << 25;
	int32 iobase_size = di->pcii.u.h0.base_register_sizes[2];
	int32 fb_size = 1 << 24;
#elif __INTEL__
	int32 _regs_size = di->pcii.u.h0.base_register_sizes[0];
	int32 iobase_size = di->pcii.u.h0.base_register_sizes[2];
	int32 fb_size = di->pcii.u.h0.base_register_sizes[1];
#endif

    ddprintf(("TDFXV3: enter map_device\n"));
    /* enable memory mapped IO, disable VGA I/O */
	tmp = get_pci(PCI_command, 2);
	tmp |= (PCI_command_memory | PCI_command_io);		
	set_pci(PCI_command, 2, tmp);

    ddprintf (("TDFXV3: regs: 0x%lx, size:0x%lx\n",_regs, _regs_size));    
    ddprintf (("TDFXV3: io: 0x%lx, size:0x%lx\n",iobase, iobase_size));    
    ddprintf (("TDFXV3: fb: 0x%lx, size:0x%lx\n",fb, fb_size));    
 
    /* map the areas */
    sprintf(buffer, "%04X_%04X_%02X%02X%02X regs",
        di->pcii.vendor_id, di->pcii.device_id,
        di->pcii.bus, di->pcii.device, di->pcii.function);
    si->regs_area = map_physical_memory(
        buffer,
        (void *)_regs,
        _regs_size,
        B_ANY_KERNEL_ADDRESS,
        0, /* B_READ_AREA + B_WRITE_AREA, */ /* neither read nor write, to hide it from user space apps */
        (void **)&(di->regs));
    /* return the error if there was some problem */
    if (si->regs_area < 0) return si->regs_area;
	
#if __INTEL__
	iobase_size = (iobase_size+(B_PAGE_SIZE-1))&~(B_PAGE_SIZE-1);
	
    sprintf(buffer, "%04X_%04X_%02X%02X%02X io",
        di->pcii.vendor_id, di->pcii.device_id,
        di->pcii.bus, di->pcii.device, di->pcii.function);
    si->io_area = map_physical_memory(
        buffer,
        (void*)iobase,
        iobase_size,
        B_ANY_KERNEL_ADDRESS,
        B_READ_AREA | B_WRITE_AREA,
        (void **)&(si->io));
    // return the error if there was some problem 

	/* remember the io_base address for inb, outb */
    si->iobase = iobase;
    

#else
// PPC comes here

	/* Round down to nearest page boundary */
	iobase = iobase & ~(B_PAGE_SIZE-1);
	/* Adjust the size */
	offset = di->pcii.u.h0.base_registers[2] - iobase;
	iobase_size += offset;
	iobase_size=(iobase_size +(B_PAGE_SIZE-1)) & ~(B_PAGE_SIZE-1);

    sprintf(buffer, "%04X_%04X_%02X%02X%02X io",
        di->pcii.vendor_id, di->pcii.device_id,
        di->pcii.bus, di->pcii.device, di->pcii.function);
    si->io_area = map_physical_memory(
        buffer,
        (void*)iobase,
        iobase_size,
        B_ANY_KERNEL_ADDRESS,
        B_READ_AREA | B_WRITE_AREA,
        (void **)&si->io);
    // return the error if there was some problem 

	/* remember the io_base address for inb, outb */
    si->iobase = si->io + offset;

    ddprintf (("TDFXV3: ioppc_3_cor: 0x%x, size:%li\n",iobase+offset, iobase_size));    	
    ddprintf (("TDFXV3: ioppc_4(vM): 0x%x, size:%li\n",si->iobase, iobase_size));    	

#endif
    if (si->io_area < 0)
    {
	    ddprintf(("TDFXV3: could not map iobase"));
        delete_area(si->regs_area);
        si->regs_area = -1;
        return si->io_area;
    }
    sprintf(buffer, "%04X_%04X_%02X%02X%02X framebuffer",
        di->pcii.vendor_id, di->pcii.device_id,
        di->pcii.bus, di->pcii.device, di->pcii.function);
    si->fb_area = map_physical_memory(
        buffer,
        (void *)fb,
        fb_size,
#if defined(__INTEL__)
#if defined(POST_R4_0)
        B_ANY_KERNEL_BLOCK_ADDRESS | B_MTR_WC,
#else
        B_ANY_KERNEL_ADDRESS | B_MTR_WC,
#endif
#else
        B_ANY_KERNEL_BLOCK_ADDRESS,
#endif
        B_READ_AREA + B_WRITE_AREA,
        &(si->framebuffer));

#if defined(__INTEL__)
    if (si->fb_area < 0) {
        /* try to map this time without write combining */
        /*
        After R4.0 (Intel), map_physical_memory() will try B_ANY_KERNEL_ADDRESS if
        a call with B_ANY_KERNEL_BLOCK_ADDRESS would fail.  It always worked this way
        under PPC.
        */
        si->fb_area = map_physical_memory(
            buffer,
            (void *)fb,
            fb_size,
#if defined(POST_R4_0)
            B_ANY_KERNEL_BLOCK_ADDRESS,
#else
            B_ANY_KERNEL_ADDRESS,
#endif
            B_READ_AREA + B_WRITE_AREA,
            &(si->framebuffer));
    }
#endif
        
    /* if there was an error, delete our other areas */
    if (si->fb_area < 0) {
        delete_area(si->regs_area);
        si->regs_area = -1;
        delete_area(si->io_area);
        si->io_area = -1;
    }
    /* remember the DMA address of the frame buffer for BDirectWindow purposes */
    si->framebuffer_pci = (void *)fb;
       
    /* in any case, return the result */
    ddprintf(("TDFXV3: leave map_device\n"));
    return si->fb_area;
}

static void unmap_device(device_info *di)
{
    shared_info *si = di->si;
    uint32    tmp;
    pci_info *pcii = &(di->pcii);

    ddprintf(("TDFXV3: unmap_device(%08lx) begins...\n", (uint32)di));
    ddprintf(("\tregs_area: %ld\n\tfb_area: %ld\n", si->regs_area, si->fb_area));
    
    /* disable memory mapped IO */
    tmp = get_pci(PCI_command, 4);
    tmp &= 0xfffffffc;
    set_pci(PCI_command, 4, tmp);
    /* delete the areas */
    if (si->io_area >= 0) delete_area(si->io_area);
    if (si->regs_area >= 0) delete_area(si->regs_area);
    if (si->fb_area >= 0) delete_area(si->fb_area);
    si->io_area = si->regs_area = si->fb_area = -1;
    si->framebuffer = NULL;
    di->regs = NULL;
    si->io = NULL;
    ddprintf(("TDFXV3: unmap_device() ends.\n"));
}

static void probe_devices(void)
{
    uint32 pci_index = 0;
    uint32 count = 0;
    device_info *di = pd->di;

    ddprintf(("TDFXV3: enter probe_devices\n"));
    /* while there are more pci devices */
    while ((count < MAX_DEVICES) && ((*pci_bus->get_nth_pci_info)(pci_index, &(di->pcii)) == B_NO_ERROR))
    {
        int vendor = 0;
        
        ddprintf(("TDFXV3: checking pci index %ld, device 0x%04x/0x%04x\n", pci_index, di->pcii.vendor_id, di->pcii.device_id));
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
                        sprintf(di->name, "graphics/%04X_%04X_%02X%02X%02X",
                            di->pcii.vendor_id, di->pcii.device_id,
                            di->pcii.bus, di->pcii.device, di->pcii.function);
                        ddprintf(("TDFXV3: making /dev/%s\n", di->name));
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

    ddprintf(("TDFXV3: probe_devices: %ld supported devices\n", pd->count));
}

static uint32 thread_interrupt_work(int32 *flags, vuint32 *regs, shared_info *si)
{
 
    uint32 handled = B_HANDLED_INTERRUPT;
    /* release the vblank semaphore */
    if (si->vblank >= 0)
    {
        int32 blocked;
        if ((get_sem_count(si->vblank, &blocked) == B_OK) && (blocked < 0))
        {
            release_sem_etc(si->vblank, -blocked, B_DO_NOT_RESCHEDULE);
            handled = B_INVOKE_SCHEDULER;
        }
    }
    return handled;
}

static int32
voodoo_interrupt(void *data)
{
    int32 handled = B_UNHANDLED_INTERRUPT;
    device_info *di = (device_info *)data;
    shared_info *si = di->si;
    int32 *flags = &(si->flags);
    vuint32 *regs;

#if DEBUG > 0
    pd->total_interrupts++;
#endif

    /* is someone already handling an interrupt for this device? */
    if (atomic_or(flags, SKD_HANDLER_INSTALLED) & SKD_HANDLER_INSTALLED)
    {
#if DEBUG > 0
        kprintf("SKD: Already in handler!\n");
#endif
        goto exit0;
    }
    /* get regs */
    regs = di->regs;

    /* read the interrrupt status register */

    /* did this card cause an interrupt */
    if (voodoo3_irqpending()==1)
    {
        /* do our stuff */
        handled = thread_interrupt_work(flags, regs, si);
#if DEBUG > 0
        /* increment the counter for this device */
        di->interrupt_count++;
#endif
		voodoo3_clearirq();
    }

    /* note that we're not in the handler any more */
    atomic_and(flags, ~SKD_HANDLER_INSTALLED);

exit0:
    return handled;                
}

#if defined(POST_R4_0)
static int32 timer_interrupt_func(timer *te, uint32 pc)
{
    bigtime_t now = system_time();
    /* get the pointer to the device we're handling this time */
    device_info *di = ((timer_info *)te)->di;
    shared_info *si = di->si;
    int32 *flags = &(si->flags);
    vuint32 *regs = di->regs;
    uint32 vbl_status = 0 /* read vertical blank status */;
    int32 result = B_HANDLED_INTERRUPT;

    /* are we suppoesed to handle interrupts still? */
    if (atomic_and(flags, -1) & SKD_HANDLER_INSTALLED) {
        /* reschedule with same period by default */
        bigtime_t when = si->refresh_period;
        timer *to;

        /* if interrupts are "enabled", do our thing */
        if (di->can_interrupt) {
            /* insert code to sync to interrupts here */
            if (!vbl_status) {
                when -= si->blank_period - 4;
            } 
            /* do the things we do when we notice a vertical retrace */
            result = thread_interrupt_work(flags, regs, si);

        }

        /* pick the "other" timer */
        to = (timer *)&(di->ti_a);
        if (to == te) to = (timer *)&(di->ti_b);
        /* our guess as to when we should be back */
        ((timer_info *)to)->when_target = now + when;
        /* reschedule the interrupt */
        add_timer(to, timer_interrupt_func, ((timer_info *)to)->when_target, B_ONE_SHOT_ABSOLUTE_TIMER);
        /* remember the currently active timer */
        di->current_timer = (timer_info *)to;
    }

    return result;
}
#else
static int32 fake_interrupt_thread_func(void *_di)
{
    device_info *di = (device_info *)_di;
    shared_info *si = di->si;
    int32 *flags = &(si->flags);
    vuint32 *regs = di->regs;
    
    bigtime_t last_sync;
    bigtime_t this_sync;
    bigtime_t diff_sync;
    
    uint32 counter = 1;
    
    /* a lie, but we have to start somewhen */
    
    last_sync = system_time() - 8333;
    
    ddprintf(("TDFXV3: fake_interrupt_thread_func begins\ndi: 0x%08lx\nsi: 0x%08lx\nflags: 0x%08lx\n", (uint32)di, (uint32)si, (uint32)flags));
    
    /* loop until notified */
    
    while(atomic_and(flags, -1) & SKD_HANDLER_INSTALLED) {
        /* see if "interrupts" are enabled */
        
        if((volatile int32)(di->can_interrupt)) {
            /* poll the retrace flag until set */
            
            /* YOUR CODE HERE */
            
            /* get the system_time */
            this_sync = system_time();
            
            /* do our stuff */
            thread_interrupt_work(flags, regs, si);
        } else {
            /* get the system_time */
            this_sync = system_time();
        }
        
        /* find out how long it took */
        diff_sync = this_sync - last_sync;
        
        /* back off a little so we're sure to catch the retrace */
        diff_sync -= diff_sync / 10;
        
        /*
        impose some limits so we can recover from refresh rate changes
        Supported refresh rates are 48 Hz - 120 Hz, so these limits should
        be slightly wider.
        */
        if(diff_sync < 8000) {
            diff_sync = 8000; /* not less than 1/125th of sec */
        }
        
        if(diff_sync > 16666) {
            diff_sync = 20000; /* not more than 1/40th of sec */
        }
        
        if((counter++ & 0x01ff) == 0) {
            diff_sync >>= 2; /* periodically quarter the wait to resync */
        }
        
        /* update for next go-around */
        last_sync = this_sync;
        
        /* snooze until our next retrace */
        
        snooze_until(this_sync + diff_sync, B_SYSTEM_TIMEBASE);
    }
    
    ddprintf(("TDFXV3: fake_interrupt_thread_func ends with flags = 0x%08lx\n", *flags));
    
    /* gotta return something */
    
    return B_OK;
}
#endif

#if DEBUG > 0
static int voodoodump(int argc, char **argv) {
    unsigned int i;

    kprintf("VOODOO Kernel Driver Persistant Data\n\nThere are %ld card(s)\n", pd->count);
    kprintf("Driver wide benahpore: %ld/%ld\n", pd->kernel.ben, pd->kernel.sem);

    kprintf("Total seen interrupts: %ld\n", pd->total_interrupts);
    for (i = 0; i < pd->count; i++) {
        device_info *di = &(pd->di[i]);
        uint16 device_id = di->pcii.device_id;
        shared_info *si = di->si;
        kprintf("  device_id: 0x%04x\n", device_id);
        kprintf("  interrupt count: %ld\n", di->interrupt_count);
        if (si) {
            kprintf("  cursor: %d,%d\n", si->cursor.x, si->cursor.y);
            kprintf("  flags:");
            if (si->flags & SKD_MOVE_CURSOR) kprintf(" SKD_MOVE_CURSOR");
            if (si->flags & SKD_PROGRAM_CLUT) kprintf(" SKD_PROGRAM_CLUT");
            if (si->flags & SKD_SET_START_ADDR) kprintf(" SKD_SET_START_ADDR");
            kprintf("  vblank semaphore id: %ld\n", si->vblank);
        }
        kprintf("\n");
    }
    return 1; /* the magic number for success */
}
#endif

static status_t open_hook (const char* name, uint32 flags, void** cookie)
{
    int32 index = 0;
    device_info *di;
    shared_info *si;
    thread_id    thid;
    thread_info    thinfo;
    status_t    result = B_OK;
    vuint32        *regs;
    char shared_name[B_OS_NAME_LENGTH];

    ddprintf(("TDFXV3: open_hook(%s, %ld, 0x%08lx)\n", name, flags, (uint32)cookie));

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
    sprintf(shared_name, "%04X_%04X_%02X%02X%02X shared",
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

	if (di->pcii.device_id==VOODOO3) {
		si->pix_clk_max8  = 300000;
    	si->pix_clk_max16 = 300000;
	    si->pix_clk_max32 = 300000;
	} else {
		si->pix_clk_max8  = 270000;
    	si->pix_clk_max16 = 270000;
	    si->pix_clk_max32 = 270000;
	}

    /* change the owner of the semaphores to the opener's team */
    /* this is required because apps can't aquire kernel semaphores */
    thid = find_thread(NULL);
    get_thread_info(thid, &thinfo);
    set_sem_owner(si->vblank, thinfo.team);

    /* assign local regs pointer for VOODOOxx() macros */
    regs = di->regs;

    voodoo3_disableirq();
    /* disable and clear any pending interrupts */

    /* if we're faking interrupts */
    if ((di->pcii.u.h0.interrupt_pin == 0x00) || (di->pcii.u.h0.interrupt_line == 0xff)){
		ddprintf (("TDFXV3:faking IRQ\n"));
        /* fake some kind of interrupt with a timer */
        di->can_interrupt = FALSE;
        si->flags = SKD_HANDLER_INSTALLED;
        si->refresh_period = 16666; /* fake 60Hz to start */
        si->blank_period = si->refresh_period / 20;
#if defined(POST_R4_0)
        di->ti_a.di = di;    /* refer to ourself */
        di->ti_b.di = di;
        di->current_timer = &(di->ti_a);
        /* program the first timer interrupt, and it will handle the rest */
        result = add_timer((timer *)(di->current_timer), timer_interrupt_func, si->refresh_period, B_ONE_SHOT_RELATIVE_TIMER);
        /* bail if we can't add the timer */
        if (result != B_OK) goto delete_the_sem;
#else
        /* fake some kind of interrupt with a thread */        
        result = di->tid = spawn_kernel_thread(fake_interrupt_thread_func, "SKD fake interrupt", B_REAL_TIME_DISPLAY_PRIORITY, di);
        /* bail if we can't spawn the thread */
        if(result < 0) goto delete_the_sem;
        /* start up the thread */
        resume_thread(di->tid);
#endif
    } else {
        /* otherwise install our interrupt handler */
        result = install_io_interrupt_handler(di->pcii.u.h0.interrupt_line, voodoo_interrupt, (void *)di, 0);
        /* bail if we couldn't install the handler */
		ddprintf (("TDFXV3:installed IRQ @ %i\n",di->pcii.u.h0.interrupt_line));
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
    ddprintf(("TDFXV3: open_hook returning 0x%08lx\n", result));
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
    ddprintf(("TDFXV3: close_hook(%08lx)\n", (uint32)dev));
    /* we don't do anything on close: there might be dup'd fd */
    return B_NO_ERROR;
}

/* -----------
    free_hook - close down the device
----------- */
static status_t
free_hook (void* dev) {
    device_info *di = (device_info *)dev;
    shared_info    *si = di->si;
    vuint32 *regs = di->regs;

    ddprintf(("TDFXV3: free_hook() begins...\n"));
    /* lock the driver */
    AQUIRE_BEN(pd->kernel);

    /* if opened multiple times, decrement the open count and exit */
    if (di->is_open > 1)
        goto unlock_and_exit;

    voodoo3_disableirq();
    /* disable and clear any pending interrupts */
    *regs = *regs; /* CHANGE ME */
    
    /* if we were faking the interrupts */
    if ((di->pcii.u.h0.interrupt_pin == 0x00) || (di->pcii.u.h0.interrupt_line == 0xff)){
        /* stop our interrupt faking thread */

        si->flags = 0;
        di->can_interrupt = FALSE;
#if defined(POST_R4_0)
        /* cancel the timer */
        /* we don't know which one is current, so cancel them both and ignore any error */
        cancel_timer((timer *)&(di->ti_a));
        cancel_timer((timer *)&(di->ti_b));
#else
        /* we don't do anything here, as the R4 kernel reaps its own threads */
        /* After R4.0 we can do it ourselves, but we'd rather use timers */
#endif
    /* otherwise */
    	
    } else {
        /* remove interrupt handler */
        remove_io_interrupt_handler(di->pcii.u.h0.interrupt_line, voodoo_interrupt, di);
    }

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
    ddprintf(("TDFXV3: free_hook() ends.\n"));
    /* all done */
    return B_OK;
}

/* -----------
    control_hook - where the real work is done
----------- */
static status_t
control_hook (void* dev, uint32 msg, void *buf, size_t len)
{
    device_info *di = (device_info *)dev;
    status_t result = B_DEV_INVALID_IOCTL;

    switch (msg)
    {
        /* the only PUBLIC ioctl */
    case B_GET_ACCELERANT_SIGNATURE:
    {
        char *sig = (char *)buf;
        strcpy(sig, "tdfx.accelerant");
        result = B_OK;
    }
        break;
        
    /* PRIVATE ioctl from here on */
    case VOODOO_GET_PRIVATE_DATA:
    {
        voodoo_get_private_data *gpd = (voodoo_get_private_data *)buf;
        if (gpd->magic == VOODOO_PRIVATE_DATA_MAGIC)
        {
            gpd->shared_info_area = di->shared_area;
            result = B_OK;
        }
    }
        break;
    case VOODOO_GET_PCI:
    {
        voodoo_get_set_pci *gsp = (voodoo_get_set_pci *)buf;
        if (gsp->magic == VOODOO_PRIVATE_DATA_MAGIC) {
            pci_info *pcii = &(di->pcii);
            gsp->value = get_pci(gsp->offset, gsp->size);
            result = B_OK;
        }
    }
        break;
    case VOODOO_SET_PCI:
    {
        voodoo_get_set_pci *gsp = (voodoo_get_set_pci *)buf;
        if (gsp->magic == VOODOO_PRIVATE_DATA_MAGIC) {
            pci_info *pcii = &(di->pcii);
            set_pci(gsp->offset, gsp->size, gsp->value);
            result = B_OK;
        }
    }
        break;
    case VOODOO_RUN_INTERRUPTS:
    {
        voodoo_set_bool_state *ri = (voodoo_set_bool_state *)buf;
        if (ri->magic == VOODOO_PRIVATE_DATA_MAGIC) {
            /* are we faking interrupts? */
            if ((di->pcii.u.h0.interrupt_pin == 0x00) || (di->pcii.u.h0.interrupt_line == 0xff)){
                di->can_interrupt = ri->do_it;
            } else {
                vuint32 *regs = di->regs;
                if (ri->do_it) {
				    voodoo3_enableirq();
                    *regs = *regs; /* CHANGE ME */
                } else {
				    voodoo3_disableirq();
                    *regs = *regs; /* CHANGE ME */
                }
            }
            result = B_OK;
        }
    }
        break;
    case VOODOO_DPRINTF:
        dprintf ((char*)buf);
        break;

    case VOODOO_INB: {
    	voodoo_port_io *io = (voodoo_port_io *)buf;
    	io->data8 = inportb(io->port);
    	} break;
    case VOODOO_INW: {
    	voodoo_port_io *io = (voodoo_port_io *)buf;
    	io->data16=inportw(io->port);
	   	} break;
    case VOODOO_INL: {
    	voodoo_port_io *io = (voodoo_port_io *)buf;
    	io->data32= inportl(io->port);
    	} break;
    case VOODOO_OUTB: {
    	voodoo_port_io *io = (voodoo_port_io *)buf;
    	outportb(io->port, io->data8);
    	} break;
    case VOODOO_OUTW: {
    	voodoo_port_io *io = (voodoo_port_io *)buf;
    	outportw(io->port, io->data16);

    	} break;
    case VOODOO_OUTL: {
    	voodoo_port_io *io = (voodoo_port_io *)buf;
    	outportl(io->port, io->data32);

    	} break;
    }
    return result;
}
