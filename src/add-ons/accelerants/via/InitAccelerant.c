/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.

	Other authors:
	Mark Watson,
	Rudolf Cornelissen 10/2002-7/2005.
*/

#define MODULE_BIT 0x00800000

#include <string.h>
#include <unistd.h>
#include "acc_std.h"

static status_t init_common(int the_fd);

/* Initialization code shared between primary and cloned accelerants */
static status_t init_common(int the_fd) {
	status_t result;
	eng_get_private_data gpd;
	
	// LOG not available from here to next LOG: NULL si

	/* memorize the file descriptor */
	fd = the_fd;
	/* set the magic number so the driver knows we're for real */
	gpd.magic = VIA_PRIVATE_DATA_MAGIC;
	/* contact driver and get a pointer to the registers and shared data */
	result = ioctl(fd, ENG_GET_PRIVATE_DATA, &gpd, sizeof(gpd));
	if (result != B_OK)
		goto error0;

	/* clone the shared area for our use */
	shared_info_area = clone_area(DRIVER_PREFIX " shared", (void **)&si, B_ANY_ADDRESS,
		B_READ_AREA | B_WRITE_AREA, gpd.shared_info_area);
	if (shared_info_area < 0) {
			result = shared_info_area;
			goto error0;
	}
	// LOG is now available, si !NULL
	LOG(4,("init_common: logmask 0x%08x, memory %dMB, hardcursor %d, usebios %d, switchhead %d, force_pci %d\n",
		si->settings.logmask, si->settings.memory, si->settings.hardcursor, si->settings.usebios, si->settings.switchhead, si->settings.force_pci));
	LOG(4,("init_common: dumprom %d, unhide_fw %d, pgm_panel %d\n",
		si->settings.dumprom, si->settings.unhide_fw, si->settings.pgm_panel));

 	/*Check for R4.5.0 and if it is running, use work around*/
 	{
 		if (si->use_clone_bugfix)
 		{
 			/*check for R4.5.0 bug and attempt to work around*/
 			LOG(2,("InitACC: Found R4.5.0 bug - attempting to work around\n"));
 			regs = si->clone_bugfix_regs;
 		}
 		else
 		{
			/* clone the memory mapped registers for our use  - does not work on <4.5.2 (but is better this way)*/
			regs_area = clone_area(DRIVER_PREFIX " regs", (void **)&regs, B_ANY_ADDRESS,
				B_READ_AREA | B_WRITE_AREA, si->regs_area);
			if (regs_area < 0) {
				result = regs_area;
				goto error1;
			}
 		}
 	}

	/*FIXME - print dma addresses*/
	//LOG(4,("DMA_virtual:%x\tDMA_physical:%x\tDMA_area:%x\n",si->dma_buffer,si->dma_buffer_pci,si->dma_buffer_area));

	/* all done */
	goto error0;

error1:
	delete_area(shared_info_area);
error0:
	return result;
}

/* Clean up code shared between primary and cloned accelrants */
static void uninit_common(void) {
	/* release the memory mapped registers */
	delete_area(regs_area);
	/* a little cheap paranoia */
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
status_t INIT_ACCELERANT(int the_fd) {
	status_t result;
	int pointer_reservation; //mem reserved for pointer
	int cnt; 				 //used for iteration through the overlay buffers

	if (0) {
		time_t now = time (NULL);
		// LOG not available from here to next LOG: NULL si
		MSG(("INIT_ACCELERANT: %s", ctime (&now)));
	}

	/* note that we're the primary accelerant (accelerantIsClone is global) */
	accelerantIsClone = 0;

	/* do the initialization common to both the primary and the clones */
	result = init_common(the_fd);

	/* bail out if the common initialization failed */
	if (result != B_OK)
		goto error0;
	// LOG now available: !NULL si
	
	/* call the device specific init code */
	result = eng_general_powerup();

	/* bail out if it failed */
	if (result != B_OK)
		goto error1;

	/*
	Now would be a good time to figure out what video modes your card supports.
	We'll place the list of modes in another shared area so all of the copies
	of the driver can see them.  The primary copy of the accelerant (ie the one
	initialized with this routine) will own the "one true copy" of the list.
	Everybody else get's a read-only clone.
	*/
	result = create_mode_list();
	if (result != B_OK)
		goto error1;

	/*
	Put the cursor at the start of the frame buffer.
	Nvidia cursor is 32x32 16 color? takes up 4096 bytes of RAM.
	*/
	/* Initialize the rest of the cursor information while we're here */
	si->cursor.width = 16;
	si->cursor.height = 16;
	si->cursor.hot_x = 0;
	si->cursor.hot_y = 0;
	si->cursor.x = 0;
	si->cursor.y = 0;
	si->cursor.dh_right = false;

	/*
	Put the frame buffer immediately following the cursor data. We store this
	info in a frame_buffer_config structure to make it convienient to return
	to the app_server later.
	*/
	pointer_reservation = 0;
	/* VIA hardcursor needs 4kB space */
	if (si->settings.hardcursor) pointer_reservation = 4096;

	si->fbc.frame_buffer = (void *)((char *)si->framebuffer+pointer_reservation);
	si->fbc.frame_buffer_dma = (void *)((char *)si->framebuffer_pci+pointer_reservation);

	/* count of issued parameters or commands */
	si->engine.last_idle = si->engine.count = 0;
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

	/* note that overlay is not in use (for eng_bes_move_overlay()) */
	si->overlay.active = false;

	/* initialise various cursor stuff */
	head1_cursor_init();
	if (si->ps.secondary_head) head2_cursor_init();

	/* ensure cursor state */
	head1_cursor_hide();
	if (si->ps.secondary_head) head2_cursor_hide();

	/* a winner! */
	result = B_OK;
	goto error0;

error1:
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
ssize_t ACCELERANT_CLONE_INFO_SIZE(void) {
	/*
	Since we're passing the name of the device as the only required
	info, return the size of the name buffer
	*/
	return B_OS_NAME_LENGTH; // apsed, was MAX_ENG_DEVICE_NAME_LENGTH;
}


/*
Return the info required to clone the device.  void *data points to
a buffer at least ACCELERANT_CLONE_INFO_SIZE() bytes in length.
*/
void GET_ACCELERANT_CLONE_INFO(void *data) {
	eng_device_name dn;
	status_t result;

	/* call the kernel driver to get the device name */	
	dn.magic = VIA_PRIVATE_DATA_MAGIC;
	/* store the returned info directly into the passed buffer */
	dn.name = (char *)data;
	result = ioctl(fd, ENG_DEVICE_NAME, &dn, sizeof(dn));
}

/*
Initialize a copy of the accelerant as a clone.  void *data points to
a copy of the data returned by GET_ACCELERANT_CLONE_INFO().
*/
status_t CLONE_ACCELERANT(void *data) {
	status_t result;
	char path[MAXPATHLEN];

	/* the data is the device name */
	/* Note: the R4 graphics driver kit is in error here (missing trailing '/') */
	strcpy(path, "/dev/");
	strcat(path, (const char *)data);
	/* open the device, the permissions aren't important */
	fd = open(path, B_READ_WRITE);
	if (fd < 0)
	{
		/* we can't use LOG because we didn't get the shared_info struct.. */
		char     fname[64];
		FILE    *myhand = NULL;

		sprintf (fname, "/boot/home/" DRIVER_PREFIX ".accelerant.0.log");
		myhand=fopen(fname,"a+");
		fprintf(myhand, "CLONE_ACCELERANT: couldn't open kerneldriver %s! Aborting.\n", path);
		fclose(myhand);

		/* abort with resultcode from open attempt on kerneldriver */
		result = fd;
		goto error0;
	}

	/* note that we're a clone accelerant */
	accelerantIsClone = 1;

	/* call the shared initialization code */
	result = init_common(fd);

	/* setup CRTC and DAC functions access */
	setup_virtualized_heads(si->crtc_switch_mode);

	/* bail out if the common initialization failed */
	if (result != B_OK) goto error1;

	/* get shared area for display modes */
	result = my_mode_list_area = clone_area(
		DRIVER_PREFIX " cloned display_modes",
		(void **)&my_mode_list,
		B_ANY_ADDRESS,
		B_READ_AREA,
		si->mode_area
	);
	if (result < B_OK) goto error2;

	/* all done */
	LOG(4,("CLONE_ACCELERANT: cloning was succesfull.\n"));

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
	if (accelerantIsClone)
	{
		LOG(4,("UNINIT_ACCELERANT: shutting down clone accelerant.\n"));
	}
	else
	{
		LOG(4,("UNINIT_ACCELERANT: shutting down primary accelerant.\n"));

		/* delete benaphores ONLY if we are the primary accelerant */
		DELETE_BEN(si->engine.lock);
		DELETE_BEN(si->overlay.lock);
	}

	/* free our mode list area */
	delete_area(my_mode_list_area);
	/* paranoia */
	my_mode_list = 0;
	/* release our cloned data */
	uninit_common();
	/* close the file handle ONLY if we're the clone */
	if (accelerantIsClone) close(fd);
}
