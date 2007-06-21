/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.

	Other authors:
	Gerald Zajac 2006-2007
*/

#include "GlobalData.h"
#include "AccelerantPrototypes.h"
#include "savage.h"

#include "errno.h"
#include "fcntl.h"
#include "stdio.h"
#include "string.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include <sys/ioctl.h>


/* Initialization code shared between primary and cloned accelerants */

static status_t 
init_common(int the_fd)
{
	status_t result;
	SavageGetPrivateData gpd;

	/* memorize the file descriptor */
	driverFileDesc = the_fd;

	/* set the magic number so the driver knows we're for real */
	gpd.magic = SAVAGE_PRIVATE_DATA_MAGIC;

	/* contact driver and get a pointer to the registers and shared data */
	result = ioctl(driverFileDesc, SAVAGE_GET_PRIVATE_DATA, &gpd, sizeof(gpd));
	if (result != B_OK)
		goto error0;

	/* clone the shared area for our use */
	sharedInfoArea = clone_area("SAVAGE shared info", (void**)&si, B_ANY_ADDRESS,
		B_READ_AREA | B_WRITE_AREA, gpd.sharedInfoArea);
	if (sharedInfoArea < 0) {
		result = sharedInfoArea;
		goto error0;
	}

	/* clone the memory mapped registers for our use */
	regsArea = clone_area("SAVAGE regs area", (void**)&regs, B_ANY_ADDRESS,
		B_READ_AREA | B_WRITE_AREA, si->regsArea);
	if (regsArea < 0) {
		result = regsArea;
		goto error1;
	}

	/* all done */
	goto error0;

error1:
	delete_area(sharedInfoArea);
error0:
	return result;
}


/* Clean up code shared between primary and cloned accelrants */
static void 
uninit_common(void)
{
	/* release the memory mapped registers */
	delete_area(regsArea);
	regs = 0;
	/* release our copy of the shared info from the kernel driver */
	delete_area(sharedInfoArea);
	si = 0;
}


/*
Initialize the accelerant.	the_fd is the file handle of the device (in
/dev/graphics) that has been opened by the app_server (or some test harness).
We need to determine if the kernel driver and the accelerant are compatible.
If they are, get the accelerant ready to handle other hook functions and
report success or failure.
*/
status_t 
INIT_ACCELERANT(int the_fd)
{
	status_t result;

	TRACE(("Enter INIT_ACCELERANT\n"));

	/* note that we're the primary accelerant (bAccelerantIsClone is global) */
	bAccelerantIsClone = false;

	/* do the initialization common to both the primary and the clones */
	result = init_common(the_fd);
	if (result != B_OK)
		goto error0;		// common initialization failed

	TRACE(("Vendor ID: 0x%X,  Device ID: 0x%X\n", si->vendorID, si->deviceID));

	/* ensure that INIT_ACCELERANT is executed just once (copies should be clones) */
	if (si->bAccelerantInUse) {
		result = B_NOT_ALLOWED;
		goto error1;
	}

	/* call the device specific init code */

	if (!SavagePreInit())
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

	/* Initialize the cursor information */
	si->cursor.width = 0;
	si->cursor.height = 0;
	si->cursor.hot_x = 0;
	si->cursor.hot_y = 0;
	si->cursor.x = 0;
	si->cursor.y = 0;

	si->fbc.frame_buffer = (void *)((addr_t)si->videoMemAddr + si->frameBufferOffset);
	si->fbc.frame_buffer_dma = (void *)((addr_t)si->videoMemPCI + si->frameBufferOffset);

	/* init the shared semaphore */
	INIT_BEN(si->engine.lock);

	/* initialize the engine synchronization variables */
	/* count of issued parameters or commands */
	si->engine.lastIdle = si->engine.count = 0;

	/* set the cursor colors.	You may or may not have to do this, depending
	on the device. */
	SavageSetCursorColors(~0, 0);

	/* ensure cursor state */
	SHOW_CURSOR(false);

	/* ensure that INIT_ACCELERANT won't be executed again (copies should be clones) */
	si->bAccelerantInUse = true;

	result = B_OK;
	goto error0;

error1:
	/*
	Initialization failed after init_common() succeeded, so we need to clean
	up before quiting.
	*/
	uninit_common();

error0:
	TRACE(("Exit INIT_ACCELERANT, result: 0x%X\n", result));
	return result;
}


/*
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
	return B_OS_NAME_LENGTH;
}


/*
Return the info required to clone the device.  void* data points to
a buffer at least ACCELERANT_CLONE_INFO_SIZE() bytes in length.
*/
void 
GET_ACCELERANT_CLONE_INFO(void* data)
{
	SavageDeviceName dn;
	status_t result;

	/* call the kernel driver to get the device name */
	dn.magic = SAVAGE_PRIVATE_DATA_MAGIC;
	/* store the returned info directly into the passed buffer */
	dn.name = (char*)data;
	result = ioctl(driverFileDesc, SAVAGE_DEVICE_NAME, &dn, sizeof(dn));
}

/*
Initialize a copy of the accelerant as a clone.  void* data points to
a copy of the data returned by GET_ACCELERANT_CLONE_INFO().
*/
status_t 
CLONE_ACCELERANT(void* data)
{
	status_t result;
	char path[MAXPATHLEN];

	/* the data is the device name */
	strcpy(path, "/dev/");
	strcat(path, (const char*)data);
	/* open the device, the permissions aren't important */
	driverFileDesc = open(path, B_READ_WRITE);
	if (driverFileDesc < 0)
		return errno;

	/* note that we're a clone accelerant */
	bAccelerantIsClone = true;

	/* call the shared initialization code */
	result = init_common(driverFileDesc);

	/* bail out if the common initialization failed */
	if (result != B_OK)
		goto error1;

	/* get shared area for display modes */
	result = modeListArea = clone_area(
		"SAVAGE cloned display_modes",
		(void**) &modeList,
		B_ANY_ADDRESS,
		B_READ_AREA,
		si->modeArea
	);

	if (result < B_OK)
		goto error2;

	/* all done */
	result = B_OK;
	goto error0;

error2:
	/* free up the areas we cloned */
	uninit_common();
error1:
	/* close the device we opened */
	close(driverFileDesc);
error0:
	return result;
}


void 
UNINIT_ACCELERANT(void)
{
	/* free our mode list area */
	delete_area(modeListArea);
	modeList = 0;
	/* release our cloned data */
	uninit_common();
	/* close file handle ONLY if we're the clone */
	if (bAccelerantIsClone)
		close(driverFileDesc);
}


// Kernel function dprintf() is not available in user space; however,
// _sPrintf performs the same function in user space but is undefined
// in the OS header files.  Thus, it is defined here.
void  _sPrintf(const char *format, ...);

void 
TraceLog(const char* fmt, ...)
{
	char	string[1024];
	va_list	args;

	strcpy(string, "savage: ");
	va_start(args, fmt);
	vsprintf(&string[strlen(string)], fmt, args);
	_sPrintf(string);
}

