/*
    Copyright 1999, Be Incorporated.   All Rights Reserved.
    This file may be used under the terms of the Be Sample Code License.

   - PPC Port: Andreas Drewke (andreas_dr@gmx.de)
   - Voodoo3Driver 0.02 (c) by Carwyn Jones (2002)
   
*/

#include "GlobalData.h"
#include "generic.h"
#include "voodoo3_accelerant.h"
#include "string.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include <sys/ioctl.h>

/* defined in ProposeDisplayMode.c */
extern status_t create_mode_list(void);
/* defined in Cursor.c */
extern void set_cursor_colors(void);

static status_t init_common(int the_fd);

static int tty = 0;
/* Initialization code shared between primary and cloned accelerants */
static status_t init_common(int the_fd)
{
    status_t result;
    voodoo_get_private_data gpd;
	
    /* memorize the file descriptor */
    fd = the_fd;
    ddpf(("TDFXV3:init_common_called: %d times\n", tty));
	tty++;
    
    ddpf (("TDFXV3:init_common begin\n"));
    /* set the magic number so the driver knows we're for real */
    gpd.magic = VOODOO_PRIVATE_DATA_MAGIC;
    /* contact driver and get a pointer to the registers and shared data */
    result = ioctl(fd, VOODOO_GET_PRIVATE_DATA, &gpd, sizeof(gpd));
    if (result != B_OK) goto error0;

    /* clone the shared area for our use */
    shared_info_area = clone_area (
        "VOODOO shared info", (void **)&si, B_ANY_ADDRESS,
        B_READ_AREA | B_WRITE_AREA, gpd.shared_info_area);
    if (shared_info_area < 0)
    {
        result = shared_info_area;
        goto error0;
    }
    /* clone the memory mapped registers for our use */
    regs_area = clone_area (
        "VOODOO regs area", (void **)&regs, B_ANY_ADDRESS,
        B_READ_AREA | B_WRITE_AREA, si->regs_area);
    if (regs_area < 0)
    {
        result = regs_area;
        goto error1;
    }
	// We don't use it here, we use it trough Kernel Driver
	/*    
		iobase_area = clone_area(
			"VOODOO io area", (void **)&iobase, B_ANY_ADDRESS,
				B_READ_AREA | B_WRITE_AREA, si->io_area);
	*/
  
    /* clone the framebuffer buffer for our use */
    fb_area = clone_area (
        "VOODOO fb area", (void **)&framebuffer, B_ANY_ADDRESS,
        B_READ_AREA | B_WRITE_AREA, si->fb_area);

    if (fb_area < 0)
    {
        result = regs_area;
        goto error2;
    }
    ddpf (("TDFXV3:starting voodoo3_init\n"));    

	voodoo3_init((uint8 *)regs, si->iobase);
    /* all done */
	goto error0;

error2:
    delete_area(regs_area);
error1:
    delete_area(shared_info_area);
error0:
    ddpf (("TDFXV3:init_common done\n"));

    return result;
}

/* Clean up code shared between primary and cloned accelrants */
static void uninit_common(void)
{
    /* release framebuffer area */
    delete_area (fb_area);
    /* release the memory mapped registers */
    delete_area(regs_area);
    /* a little cheap paranoia */

	// delete_area(iobase_area);

    /* a little cheap paranoia */
    framebuffer = 0;
    regs = 0;
    /* release our copy of the shared info from the kernel driver */
    delete_area(shared_info_area);
    /* more cheap paranoia */
    si = 0;
}

 
/*
Initialize the accelerant.  the_fd is the file handle of the device (in
/dev/graphics) that has been opened by the app_server (or some test harness).
We need to determine if the kernel driver and the accelerant are compatible.
If they are, get the accelerant ready to handle other hook functions and
report success or failure.
*/

status_t INIT_ACCELERANT(int the_fd)
{
    status_t result;
    uint32 cnt;
    /* note that we're the primary accelerant (accelerantIsClone is global) */
    accelerantIsClone = 0;

    /* do the initialization common to both the primary and the clones */
    result = init_common(the_fd);

    /* bail out if the common initialization failed */
    if (result != B_OK) goto error0;

    /*
    If there is a possiblity that the kernel driver will recognize a card that
    the accelerant can't support, you should check for that here.  Perhaps some
    odd memory configuration or some such.
    */

    /*
    This is a good place to go and initialize your card.  The details are so
    device specific, we're not even going to pretend to provide you with sample
    code.  If this fails, we'll have to bail out, cleaning up the resources
    we've already allocated.
    */
    /* call the device specific init code */
	ddpf (("TDFXV3:pixelclock 8: %li\n", si->pix_clk_max8));
	ddpf (("TDFXV3:pixelclock 16: %li\n", si->pix_clk_max16));
	ddpf (("TDFXV3:pixelclock 32: %li\n", si->pix_clk_max32));	
	
   	voodoo3_set_monitor_defaults();
    si->mem_size = voodoo3_get_memory_size();
    ddpf (("TDFXV3:mem_size = %dMB\n", si->mem_size / 1024  / 1024));    
    if (!si->mem_size) {
    	result=-1;
    	goto error0;
    }
    	
    /*
    Now would be a good time to figure out what video modes your card supports.
    We'll place the list of modes in another shared area so all of the copies
    of the driver can see them.  The primary copy of the accelerant (ie the one
    initialized with this routine) will own the "one true copy" of the list.
    Everybody else get's a read-only clone.
    */
    result = create_mode_list();
    if (result != B_OK) goto error2;

    /*
    Initialize the frame buffer and cursor pointers.  Most newer video cards
    have integrated the DAC into the graphics engine, and so the cursor shape
    is stored in the frame buffer RAM.  Also, newer cards tend not to have as
    many restrictions about the placement of the start of the frame buffer in
    frame buffer RAM.  If you're supporting an older card with frame buffer
    positioning restrictions, or one without an integrated DAC, you'll have to
    change this accordingly.
    */
    /*
    Put the cursor at the start of the frame buffer.  The typical 64x64 4 color
    (black, white, transparent, inverse) takes up 1024 bytes of RAM.
    */

    si->cursor.data = (uint8 *)si->framebuffer;// + 131072;
    /* Initialize the rest of the cursor information while we're here */
    si->cursor.width = 0;
    si->cursor.height = 0;
    si->cursor.hot_x = 0;
    si->cursor.hot_y = 0;
    si->cursor.x = 0;
    si->cursor.y = 0;
    /*
    Put the frame buffer immediately following the cursor data. We store this
    info in a frame_buffer_config structure to make it convienient to return
    to the app_server later.
    */
    si->fbc.frame_buffer = (void *)(((char *)si->framebuffer) + 1024);
    si->fbc.frame_buffer_dma = (void *)(((char *)si->framebuffer_pci) + 1024);
    si->start_addr = 1024;
	
    /* init the shared semaphore */
    INIT_BEN(si->engine.lock);
	INIT_BEN(si->overlay.lock);
	for (cnt = 0; cnt < MAXBUFFERS; cnt++)
	{
		/* make sure overlay buffers are 'marked' as being free */
		si->overlay.myBuffer[cnt].buffer = NULL;
		si->overlay.myBuffer[cnt].buffer_dma = NULL;
	}
	/* make sure overlay unit is 'marked' as being free */
	si->overlay.myToken = NULL;	

    /* initialize the engine synchronization variables */
    
    /* count of issued parameters or commands */
    si->engine.last_idle = si->engine.count = 0;

    /* set the cursor colors.  You may or may not have to do this, depending
    on the device. */
    
    voodoo3_init_cursor_address();
    set_cursor_colors();

    /* ensure cursor state */
    SHOW_CURSOR(false);
    /* a winner! */
    result = B_OK;
    goto error0;

    /* free up the benaphore */
    DELETE_BEN(si->engine.lock);
    DELETE_BEN(si->overlay.lock);    

error2:
    /*
    Clean up any resources allocated in your device specific initialization
    code.
    */

    /*
    Initialization failed after init_common() succeeded, so we need to clean
    up before quiting.
    */
    uninit_common();

error0:
    return result;
}

/*
Return the number of bytes required to hold the information required
to clone the device.
*/
ssize_t ACCELERANT_CLONE_INFO_SIZE(void)
{
    /*
    Since we're passing the name of the device as the only required
    info, return the size of the name buffer
    */
    return MAX_VOODOO_DEVICE_NAME_LENGTH;
}


/*
Return the info required to clone the device.  void *data points to
a buffer at least ACCELERANT_CLONE_INFO_SIZE() bytes in length.
*/
void GET_ACCELERANT_CLONE_INFO(void *data)
{
    voodoo_device_name dn;
    status_t result;

    /* call the kernel driver to get the device name */    
    dn.magic = VOODOO_PRIVATE_DATA_MAGIC;
    /* store the returned info directly into the passed buffer */
    dn.name = (char *)data;
    result = ioctl(fd, VOODOO_DEVICE_NAME, &dn, sizeof(dn));
}

/*
Initialize a copy of the accelerant as a clone.  void *data points to
a copy of the data returned by GET_ACCELERANT_CLONE_INFO().
*/
status_t CLONE_ACCELERANT(void *data)
{
    status_t result;
    char path[MAXPATHLEN];

    /* the data is the device name */
    strcpy(path, "/dev");
    strcat(path, (const char *)data);
    /* open the device, the permissions aren't important */
    fd = open(path, B_READ_WRITE);
    if (fd < 0)
    {
        result = fd;
        goto error0;
    }

    /* note that we're a clone accelerant */
    accelerantIsClone = 1;

    /* call the shared initialization code */
    result = init_common(fd);

    /* bail out if the common initialization failed */
    if (result != B_OK) goto error1;

    /* get shared area for display modes */
    result = my_mode_list_area = clone_area(
        "VOODOO cloned display_modes",
        (void **)&my_mode_list,
        B_ANY_ADDRESS,
        B_READ_AREA,
        si->mode_area
    );
    if (result < B_OK) goto error2;

    /* all done */
    result = B_OK;
    goto error0;

error2:
    /* free up the areas we cloned */
    uninit_common();
error1:
    /* close the device we opened */
    close(fd);
error0:
    return result;
}

void UNINIT_ACCELERANT(void)
{
    /* free our mode list area */
    delete_area(my_mode_list_area);
    /* paranoia */
    my_mode_list = 0;
    /* release our cloned data */
    uninit_common();
    /* close the file handle ONLY if we're the clone */
    if (accelerantIsClone) close(fd);
}

status_t GET_ACCELERANT_DEVICE_INFO(accelerant_device_info *adi)
{
    status_t retval;
    char * names[] = 
        { "Voodoo3" };

    adi->version = 0x10000;
    strcpy (adi->name, names[0]);
    strcpy (adi->chipset, names[0]);
    strcpy (adi->serial_no, "0");
    adi->memory = si->mem_size;
    adi->dac_speed = 300;

    retval = B_OK;
    return retval;
}

