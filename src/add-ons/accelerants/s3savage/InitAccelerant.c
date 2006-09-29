/*
    Copyright 1999, Be Incorporated.   All Rights Reserved.
    This file may be used under the terms of the Be Sample Code License.
*/
/*
 * Copyright 1999  Erdi Chen
 */

#include "GlobalData.h"
#include "generic.h"
#include "s3drv.h"
#include "s3mmio.h"
#include "s3accel.h"

#include "errno.h"
#include "fcntl.h"
#include "string.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include <sys/ioctl.h>


/*!
	Get the linear address of an area.  Workaround for BeOS bug that 
	returns invalid pointer from clone_area.
*/
static void *
get_area_address(area_id area)
{
	area_info ai;
	get_area_info(area, &ai);
	return ai.address;
}


#if 0
// unused!
// Determines the amount of card memory available by seeing how far up 
// the frame buffer data can be written and read back reliably. Does a
// paranoia check to make sure that it isn't just wrapping, either.
unsigned long
Get_Card_Mem_Size()
{
    // Allowed sizes actually go up to 16 megs, but clip at the
    // register window for now.
    const unsigned long AllowedSizes[] =
    { 0x00080000, 0x00100000, 0x00180000, 0x00200000,
      0x00280000, 0x00300000, 0x00380000, 0x00400000,
      0x00500000, 0x00600000, 0x00700000, 0x007FF800,
      0x0 };

    unsigned long MaxMem;
    unsigned long RWIndex;
    int iMaxIndex, iTestIndex, iX;
    unsigned long LTemp;
    int IsOk;
    uint32    *VramBase;
    
    VramBase = (uint32 *)framebuffer;
    MaxMem = 0; // Default.
    IsOk = 1;
    // Step through ever-larger memory sizes, recording size if passes test and
    // ignoring otherwise.
    for (iMaxIndex = 0; (AllowedSizes[iMaxIndex] != 0) && IsOk; iMaxIndex++)
    {
        // Write test values to the linear aperature.
        // Only need to do this for the farthest location, as previous 
        // locations already have been written to in previous passes.
        RWIndex = AllowedSizes[iMaxIndex];
        RWIndex = (RWIndex - 16384) >> 2;
        for (iX = 0; iX < 4096; iX++)
        {
            LTemp = RWIndex;
            // Hash LTemp. As the parameters for the hash are prime, it should
            // be extremely unlikely to get these values through a glitch, and
            // the pattern only repeats at prime intervals, so aliasing
            // shouldn't fool the test either.
            LTemp = (263 * (LTemp % 65521) + 29) % 65521;
            // Extend this to 32 bits.
            LTemp |= (LTemp ^ 0x0000FFFFul) << 16;
            VramBase[RWIndex] = LTemp;
            RWIndex++;
        }

        // Verify that all test patterns are still intact. If values written
        // past the end of memory drop off the face of the frame buffer, the
        // farthest pattern(s) will not be what they should be. If values
        // written past the end of memory wrap, then previous patterns will
        // be overwritten (or partly overwritten, as the test location at
        // 8 megs is actually at 8 megs - 2k).
        // As soon as an invalid value is detected, IsOk is set to 0, which
        // should quickly terminate the test loops.
        for (iTestIndex = 0; (iTestIndex <= iMaxIndex) && IsOk; iTestIndex++)
        {
            RWIndex = AllowedSizes[iTestIndex];
            RWIndex = (RWIndex - 16384) >> 2;
            for (iX = 0; (iX < 4096) && IsOk; iX++)
            {
                LTemp = RWIndex;
                // Hash LTemp. As the parameters for the hash are prime, it
                // should be extremely unlikely to get these values through
                // a glitch, and the pattern only repeats at prime intervals,
                // so aliasing shouldn't fool the test either.
                LTemp = (263 * (LTemp % 65521) + 29) % 65521;
                // Extend this to 32 bits.
                LTemp |= (LTemp ^ 0x0000FFFFul) << 16;
                // Test against the value read from the frame buffer.
                if (VramBase[RWIndex] != LTemp)
                    IsOk = 0;
                RWIndex++;
            }
        }

        // If the test patterns check out, update MaxMem accordingly.
        if (IsOk)
            MaxMem = AllowedSizes[iMaxIndex];
    }

    return MaxMem;
}
#endif


/* Initialization code shared between primary and cloned accelerants */
static status_t
init_common(int the_fd)
{
	status_t result;
	savage_get_private_data gpd;

	/* memorize the file descriptor */
	fd = the_fd;
	dpf ("init_common begin\n");
	/* set the magic number so the driver knows we're for real */
	gpd.magic = SAVAGE_PRIVATE_DATA_MAGIC;
	/* contact driver and get a pointer to the registers and shared data */
	if (ioctl(fd, SAVAGE_GET_PRIVATE_DATA, &gpd, sizeof(gpd)) != 0)
		return errno;

    /* clone the shared area for our use */
	shared_info_area = clone_area("SAVAGE shared info", (void **)&si,
		B_ANY_ADDRESS, B_READ_AREA | B_WRITE_AREA, gpd.shared_info_area);
	if (shared_info_area < 0)
		return shared_info_area;

	/* clone the memory mapped registers for our use */
	regs_area = clone_area("SAVAGE regs area", (void **)&regs, B_ANY_ADDRESS,
		B_READ_AREA | B_WRITE_AREA, si->regs_area);
	if (regs_area < 0) {
		result = regs_area;
		goto error1;
	}
	regs = get_area_address(regs_area);

	dpf("PCI VENDOR_ID = 0x%4X, DEVICE_ID = 0x%4X\n",
		read16((byte_t*)regs + 0x8000),
		read16((byte_t*)regs + 0x8002));

	/* clone the framebuffer buffer for our use */
	fb_area = clone_area("SAVAGE fb area", (void **)&framebuffer,
		B_ANY_ADDRESS, B_READ_AREA | B_WRITE_AREA, si->fb_area);
	if (fb_area < 0) {
		result = regs_area;
		goto error2;
	}
	framebuffer = get_area_address(fb_area);

	s3drv_init((byte_t*)regs);
	dpf ("s3drv_init called\n");
	s3accel_init(s3drv_get_context());
	dpf ("s3accel_init called\n");
	s3drv_unlock_regs();

	/* all done */
	return B_OK;

error2:
	delete_area(regs_area);
error1:
	delete_area(shared_info_area);
	return result;
}


/*! Clean up code shared between primary and cloned accelerants */
static void
uninit_common(void)
{
    /* release framebuffer area */
    delete_area (fb_area);
    /* release the memory mapped registers */
    delete_area(regs_area);
    /* a little cheap paranoia */
    regs = 0;
    /* release our copy of the shared info from the kernel driver */
    delete_area(shared_info_area);
    /* more cheap paranoia */
    si = 0;
}

 
/*!
	Initialize the accelerant.  the_fd is the file handle of the device (in
	/dev/graphics) that has been opened by the app_server (or some test harness).
	We need to determine if the kernel driver and the accelerant are compatible.
	If they are, get the accelerant ready to handle other hook functions and
	report success or failure.
*/
status_t
INIT_ACCELERANT(int the_fd)
{
    status_t result;
    /* note that we're the primary accelerant (accelerantIsClone is global) */
    accelerantIsClone = 0;

    /* do the initialization common to both the primary and the clones */
    result = init_common(the_fd);
    if (result != B_OK)
    	goto error0;

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
    {
        SVGAMode orig, newmode;
        PixelTiming timing =
            { 0, { 640, 656, 752, 800, 0 }, { 480, 490, 492, 525, 0 } };
        s3drv_unlock_regs ();
        s3drv_get_mode (&timing, 32, (byte_t*)&newmode, sizeof (newmode));
        s3drv_save_mode ((byte_t*)&orig, sizeof (orig));
        s3drv_restore_mode ((byte_t*)&newmode, sizeof (newmode), 1);
        s3accel_init (s3drv_get_context ());
        //si->mem_size = s3drv_detect_vram_mb_size ((byte_t*)framebuffer, 32);
        //dpf ("si->mem_size = %dMB\n", si->mem_size);
        si->mem_size = s3drv_detect_vram_mb_size (NULL, 2);
        dpf ("si->mem_size = %dMB\n", si->mem_size);
        si->mem_size *= 1024*1024;
        s3drv_restore_mode ((byte_t*)&orig, sizeof (orig), 1);
        s3drv_lock_regs ();
    }

    si->pix_clk_max8  = 250000;
    si->pix_clk_max16 = 250000;
    si->pix_clk_max32 = 250000;

    /* bail out if it failed */
    if (result != B_OK) goto error1;

    /*
    Now would be a good time to figure out what video modes your card supports.
    We'll place the list of modes in another shared area so all of the copies
    of the driver can see them.  The primary copy of the accelerant (ie the one
    initialized with this routine) will own the "one true copy" of the list.
    Everybody else get's a read-only clone.
    */
	result = create_mode_list();
	if (result != B_OK)
		goto error2;

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
    si->cursor.data = (uint8 *)si->framebuffer;
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

    /* initialize the engine synchronization variables */
    
    /* count of issued parameters or commands */
    si->engine.last_idle = si->engine.count = 0;

    /* bail out if something failed */
    if (result != B_OK) goto error3;

    /* set the cursor colors.  You may or may not have to do this, depending
    on the device. */
    set_cursor_colors();

    /* ensure cursor state */
    SHOW_CURSOR(false);
    /* a winner! */
    result = B_OK;
    goto error0;

error3:
    /* free up the benaphore */
    DELETE_BEN(si->engine.lock);

error2:
    /*
    Clean up any resources allocated in your device specific initialization
    code.
    */

error1:
    /*
    Initialization failed after init_common() succeeded, so we need to clean
    up before quiting.
    */
    uninit_common();

error0:
    return result;
}


/*!
	Return the number of bytes required to hold the information required
	to clone the device.
*/
ssize_t
ACCELERANT_CLONE_INFO_SIZE(void)
{
    /*
    Since we're passing the name of the device as the only required
    info, return the size of the name buffer
    */
    return MAX_SAVAGE_DEVICE_NAME_LENGTH;
}


/*!
	Return the info required to clone the device.  void *data points to
	a buffer at least ACCELERANT_CLONE_INFO_SIZE() bytes in length.
*/
void
GET_ACCELERANT_CLONE_INFO(void *data)
{
    savage_device_name dn;
    status_t result;

    /* call the kernel driver to get the device name */    
    dn.magic = SAVAGE_PRIVATE_DATA_MAGIC;
    /* store the returned info directly into the passed buffer */
    dn.name = (char *)data;
    result = ioctl(fd, SAVAGE_DEVICE_NAME, &dn, sizeof(dn));
}


/*!
	Initialize a copy of the accelerant as a clone.  void *data points to
	a copy of the data returned by GET_ACCELERANT_CLONE_INFO().
*/
status_t
CLONE_ACCELERANT(void *data)
{
	status_t result;
	char path[MAXPATHLEN];

	/* the data is the device name */
	strcpy(path, "/dev/");
	strcat(path, (const char *)data);
	/* open the device, the permissions aren't important */
	fd = open(path, B_READ_WRITE);
	if (fd < 0)
		return errno;

	/* note that we're a clone accelerant */
	accelerantIsClone = 1;

	/* call the shared initialization code */
	result = init_common(fd);
	if (result != B_OK)
		goto error1;

	/* get shared area for display modes */
	result = my_mode_list_area = clone_area("SAVAGE cloned display_modes",
		(void **)&my_mode_list, B_ANY_ADDRESS, B_READ_AREA, si->mode_area);
	if (result < B_OK)
		goto error2;

	/* all done */
	return B_OK;

error2:
	/* free up the areas we cloned */
	uninit_common();
error1:
	/* close the device we opened */
	close(fd);
	return result;
}


void
UNINIT_ACCELERANT(void)
{
    /* free our mode list area */
    delete_area(my_mode_list_area);
    /* paranoia */
    my_mode_list = 0;
    /* release our cloned data */
    uninit_common();
    /* close the file handle ONLY if we're the clone */
    if (accelerantIsClone)
    	close(fd);
}


status_t
GET_ACCELERANT_DEVICE_INFO(accelerant_device_info *adi)
{
    status_t retval;
    static const char * names1[] = 
        { "Savage3D", "Savage3D/MV", "Savage4", "Savage4",  
          "Savage4", "ProSavage PM133", "ProSavage KM133" };
    static const char * names2[] =
        { "Savage/MX-MV", "Savage/MX", "Savage/IX-MV", "Savage/IX" };
    static const char * names3[] =
        { "ProSavage PN133", "ProSavage KN133", "ProSavage P4M266", "ProSavage8 KM266" };

    adi->version = 0x10000;
    if (si->device_id == PCI_PID_SAVAGE2000) {
        strcpy (adi->name, "Savage2000");
        strcpy (adi->chipset, "Savage2000");
    } else {
        const char ** names = NULL;
        int offset = 0;
        switch (si->device_id & 0xFFF0) {
			case 0x8a20:
				names = names1;
				offset = si->device_id - PCI_PID_SAVAGE3D;
				break;
			case 0x8c10:
				names = names2;
				offset = si->device_id - PCI_PID_SAVAGEMXMV;
				break;
			case 0x8d00:
				names = names3;
				offset = si->device_id - PCI_PID_PN133;
				break;
        }
		if (names != NULL) {
			strcpy (adi->name, names[offset]);
			strcpy (adi->chipset, names[offset]);
		} else {
			strcpy (adi->name, "not supported");
			strcpy (adi->chipset, "Savage");
		}
	}
    strcpy (adi->serial_no, "0");
    adi->memory = si->mem_size;
    adi->dac_speed = 250;

    retval = B_OK;
    return retval;
}
