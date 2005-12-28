/*****************************************************************************\
 * Tseng Labs ET6000, ET6100 and ET6300 graphics driver for BeOS 5.
 * Copyright (c) 2003-2004, Evgeniy Vladimirovich Bobkov.
\*****************************************************************************/

#include "GlobalData.h"
#include "generic.h"

#include "string.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include <sys/ioctl.h>


/*****************************************************************************/
/*
 * Initialization code shared between primary and cloned accelerants.
 */
static status_t initCommon(int the_fd) {
status_t result;
ET6000GetPrivateData gpd;

    /* memorize the file descriptor */
    fd = the_fd;
    /* set the magic number so the driver knows we're for real */
    gpd.magic = ET6000_PRIVATE_DATA_MAGIC;
    /* contact driver and get a pointer to the registers and shared data */
    result = ioctl(fd, ET6000_GET_PRIVATE_DATA, &gpd, sizeof(gpd));
    if (result != B_OK) goto error0;

    /* clone the shared area for our use */
    sharedInfoArea = clone_area("ET6000 shared info", (void **)&si,
        B_ANY_ADDRESS, B_READ_AREA | B_WRITE_AREA, gpd.sharedInfoArea);
    if (sharedInfoArea < 0) {
        result = sharedInfoArea;
        goto error0;
    }

    mmRegs = si->mmRegs;

error0:
    return result;
}
/*****************************************************************************/
/*
 * Clean up code shared between primary and cloned accelrants.
 */
static void uninitCommon(void) {
    /* release our copy of the shared info from the kernel driver */
    delete_area(sharedInfoArea);
    si = 0;
}
/*****************************************************************************/
/*
 * Initialize the accelerant.  the_fd is the file handle of the device
 * (in /dev/graphics) that has been opened by the app_server (or some test
 * harness). We need to determine if the kernel driver and the accelerant
 * are compatible. If they are, get the accelerant ready to handle other
 * hook functions and report success or failure.
 */
status_t INIT_ACCELERANT(int the_fd) {
status_t result;

    /* note that we're the primary accelerant (accelerantIsClone is global) */
    accelerantIsClone = 0;

    /* do the initialization common to both the primary and the clones */
    result = initCommon(the_fd);

    /* bail out if the common initialization failed */
    if (result != B_OK) goto error0;

    /* Call the device specific initialization code here, bail out if it failed */
    if (result != B_OK) goto error1;

    /*
     * Now is a good time to figure out what video modes the card supports.
     * We'll place the list of modes in another shared area so all
     * of the copies of the driver can see them. The primary copy of the
     * accelerant (ie the one initialized with this routine) will own the
     * "one true copy" of the list. Everybody else get's a read-only clone.
     */
    result = createModesList();
    if (result != B_OK) goto error2;

    /*
     * We store this info in a frame_buffer_config structure to
     * make it convienient to return to the app_server later.
     */
    si->fbc.frame_buffer = si->framebuffer;
    si->fbc.frame_buffer_dma = si->physFramebuffer;

    /* init the shared semaphore */
    INIT_BEN(si->engine.lock);

    /* initialize the engine synchronization variables */

    /* count of issued parameters or commands */
    si->engine.lastIdle = si->engine.count = 0;

    /* bail out if something failed */
    if (result != B_OK) goto error3;

    /* a winner! */
    result = B_OK;
    goto error0;

error3:
    /* free up the benaphore */
    DELETE_BEN(si->engine.lock);

error2:
    /*
     * Clean up any resources allocated in your device
     * specific initialization code.
     */

error1:
    /*
     * Initialization failed after initCommon() succeeded, so we need to
     * clean up before quiting.
     */
    uninitCommon();

error0:
    return result;
}
/*****************************************************************************/
/*
 * Return the number of bytes required to hold the information required
 * to clone the device.
 */
ssize_t ACCELERANT_CLONE_INFO_SIZE(void) {
    /*
     * Since we're passing the name of the device as the only required
     * info, return the size of the name buffer
     */
    return B_OS_NAME_LENGTH;
}
/*****************************************************************************/
/*
 * Return the info required to clone the device.  void *data points to
 * a buffer at least ACCELERANT_CLONE_INFO_SIZE() bytes in length.
 */
void GET_ACCELERANT_CLONE_INFO(void *data) {
ET6000DeviceName dn;
status_t result;

    /* call the kernel driver to get the device name */
    dn.magic = ET6000_PRIVATE_DATA_MAGIC;
    /* store the returned info directly into the passed buffer */
    dn.name = (char *)data;
    result = ioctl(fd, ET6000_DEVICE_NAME, &dn, sizeof(dn));
}
/*****************************************************************************/
/*
 * Initialize a copy of the accelerant as a clone.  void *data points
 * to a copy of the data returned by GET_ACCELERANT_CLONE_INFO().
 */
status_t CLONE_ACCELERANT(void *data) {
status_t result;
char path[MAXPATHLEN];

    /* the data is the device name */
    strcpy(path, "/dev/");
    strcat(path, (const char *)data);
    /* open the device, the permissions aren't important */
    fd = open(path, B_READ_WRITE);
    if (fd < 0) {
        result = fd;
        goto error0;
    }

    /* note that we're a clone accelerant */
    accelerantIsClone = 1;

    /* call the shared initialization code */
    result = initCommon(fd);

    /* bail out if the common initialization failed */
    if (result != B_OK) goto error1;

    /* get shared area for display modes */
    result = et6000ModesListArea = clone_area("ET6000 cloned display_modes",
        (void **)&et6000ModesList, B_ANY_ADDRESS, B_READ_AREA, si->modesArea);
    if (result < B_OK) goto error2;

    /* all done */
    result = B_OK;
    goto error0;

error2:
    /* free up the areas we cloned */
    uninitCommon();
error1:
    /* close the device we opened */
    close(fd);
error0:
    return result;
}
/*****************************************************************************/
void UNINIT_ACCELERANT(void) {
    /* free our mode list area */
    delete_area(et6000ModesListArea);
    et6000ModesList = 0;

    /* release our cloned data */
    uninitCommon();

    /* close the file handle ONLY if we're the clone */
    if (accelerantIsClone) close(fd);
}
/*****************************************************************************/
status_t GET_ACCELERANT_DEVICE_INFO(accelerant_device_info *adi)
{
    adi->version = B_ACCELERANT_VERSION;
    strcpy(adi->name, "Tseng Labs ET6x00");
    strcpy(adi->chipset, "Tseng Labs ET6x00");
    strcpy(adi->serial_no, "");
    adi->memory = si->memSize;
    adi->dac_speed = si->pixelClockMax16;

    return B_OK;
}
/*****************************************************************************/
