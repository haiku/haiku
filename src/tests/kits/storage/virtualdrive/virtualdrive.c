// ----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  File Name:		virtualdrive.c
//
//	Description:	Driver that emulates virtual drives.
//
//	Author:			Marcus Overhagen <Marcus@Overhagen.de>
//					Ingo Weinhold <bonefish@users.sf.net>
// ----------------------------------------------------------------------

#include <KernelExport.h>
#include <Drivers.h>
#include <Errors.h>
#include <malloc.h>
#include "lock.h"

/*
[2:07] <geist> when you open the file in the driver, use stat() to see if it's a file. if it is, call ioctl 10000 on the underlying file
[2:07] <geist> that's the disable-cache ioctl
[2:08] <geist> bfs is probably doing the same algorithm, and seeing that you are a device and not a file, and so it doesn't call 10000 on you
[2:08] <marcus_o> thanks, I will try calling it
[2:08] <geist> and dont bother using dosfs as a host fs, it wont work
[2:09] <geist> bfs is the only fs that's reasonably safe being reentered like that, but only if the underlying one is opened with the cache disabled on that file
[2:09] <geist> that ioctl is used on the swap file as well
[2:10] <marcus_o> I'm currently allocating memory in the driver's write() function hook
[2:10] <geist> cant do that
*/

#define TRACE dprintf
//#define TRACE (void)
#define MB (1024LL * 1024LL)

static int dev_num_for_path(const char *path);

int32 api_version = B_CUR_DRIVER_API_VERSION;

lock driverlock;

typedef struct device_inf {
	int32 opencount;
	uint8 *buffer;
	int	fd;
	int64 size;
	bool hidden;
	char file[1024];
	const char device_path[1024];
} device_inf;

struct device_inf device_info[] = 
{
	{ 0, 0, 0, 64 * MB, false, "/tmp/virtualdrive64", "disk/virtual/0/raw" },
	{ 0, 0, 0, 32 * MB, false, "/tmp/virtualdrive32", "disk/virtual/1/raw" },
};

const int kDeviceCount = sizeof(device_info) / sizeof(device_inf);

/* ----------
	init_hardware - called once the first time the driver is loaded
----- */
status_t
init_hardware (void)
{
	TRACE("virtualdrive: init_hardware\n");
	return B_OK;
}


/* ----------
	init_driver - optional function - called every time the driver
	is loaded.
----- */
status_t
init_driver (void)
{
	TRACE("virtualdrive: init\n");

	new_lock(&driverlock, "virtualdrive lock");

	return B_OK;
}


/* ----------
	uninit_driver - optional function - called every time the driver
	is unloaded
----- */
void
uninit_driver (void)
{
	TRACE("virtualdrive: uninit\n");
	free_lock(&driverlock);
}

	
/* ----------
	virtualdrive_open - handle open() calls
----- */

static status_t
virtualdrive_open (const char *name, uint32 flags, void** cookie)
{
	int dev_num;
	TRACE("virtualdrive: open %s\n",name);

	*cookie = (void *) -1;
	
	if (0 == strcmp(name,"disk/virtual/virtualdrive")) {
		TRACE("virtualdrive: rejected!\n");
		return B_ERROR;
	}
	
	LOCK(driverlock);
	
	dev_num = dev_num_for_path(name);

	TRACE("virtualdrive: dev_num %d!\n",dev_num);
	
	if (dev_num < 0 || dev_num >= kDeviceCount) {
		TRACE("virtualdrive: wrong index!\n");
		UNLOCK(driverlock);
		return B_ERROR;
	}
	
	if (device_info[dev_num].hidden) {
		TRACE("virtualdrive: device is hidden!\n");
		UNLOCK(driverlock);
		return B_ERROR;
	}

	// store index in cookie
	*cookie = (void *) dev_num;

	device_info[dev_num].opencount++;
	
	if (device_info[dev_num].opencount == 1) {
		char c;
		const char *file;
		file = device_info[dev_num].file;
		TRACE("virtualdrive: opening %s\n",file);
		device_info[dev_num].fd = open(file,O_RDWR | O_CREAT,0666);
		if (device_info[dev_num].fd < 0) {
			device_info[dev_num].opencount--;
			TRACE("virtualdrive: file open failed!\n");
			// remove index from cookie
			*cookie = (void *) -1;
			UNLOCK(driverlock);
			return B_ERROR;
		}

		// disable caching for underlying file! (else this driver will deadlock)
		if (0 != ioctl(device_info[dev_num].fd,10000)) {
			TRACE("virtualdrive: disable caching ioctl failed\n");
		}
		// set file size
		lseek(device_info[dev_num].fd,device_info[dev_num].size - 1,0);
		read(device_info[dev_num].fd,&c,1);
		lseek(device_info[dev_num].fd,device_info[dev_num].size - 1,0);
		write(device_info[dev_num].fd,&c,1);
		
//		device_info[dev_num].buffer = (uint8*) malloc(65536);
		TRACE("virtualdrive: file opened\n");
	}

	UNLOCK(driverlock);
	return B_OK;
}


/* ----------
	virtualdrive_close - handle close() calls
----- */

static status_t
virtualdrive_close (void* cookie)
{
	int dev_num;
	TRACE("virtualdrive: close\n");
	
	dev_num = (int)cookie;

	TRACE("virtualdrive: dev_num = %d\n",dev_num);
	if (dev_num < 0 || dev_num >= kDeviceCount)
		return B_OK;

	LOCK(driverlock);
	
	device_info[dev_num].opencount--;
	
	if (device_info[dev_num].opencount == 0) {
		close(device_info[dev_num].fd);
//		free(device_info[dev_num].buffer);
	}
	
	UNLOCK(driverlock);

	return B_OK;
}


/* ----------
	virtualdrive_read - handle read() calls
----- */

static status_t
virtualdrive_read (void* cookie, off_t position, void *buffer, size_t* num_bytes)
{
	int fd;
	TRACE("virtualdrive: read pos = 0x%08Lx, bytes = 0x%08lx\n",position,*num_bytes);
	fd = device_info[(int) cookie].fd;
	if (fd < 0)
		return B_ERROR;
	if (position & 511)
		return B_ERROR;
	if (*num_bytes & 511)
		return B_ERROR;
	
	LOCK(driverlock);
	lseek(fd,position,0);
	read(fd,buffer,*num_bytes);
	UNLOCK(driverlock);

	return B_OK;
}


/* ----------
	virtualdrive_write - handle write() calls
----- */

static status_t
virtualdrive_write (void* cookie, off_t position, const void* buffer, size_t* num_bytes)
{
	int fd;
	TRACE("virtualdrive: write pos = 0x%08Lx, bytes = 0x%08lx\n",position,*num_bytes);
	fd = device_info[(int) cookie].fd;
	if (fd < 0)
		return B_ERROR;
	if (position & 511)
		return B_ERROR;
	if (*num_bytes & 511)
		return B_ERROR;

	LOCK(driverlock);
	lseek(fd,position,0);
	write(fd,buffer,*num_bytes);
	UNLOCK(driverlock);

	return B_OK;
}


/* ----------
	virtualdrive_control - handle ioctl calls
----- */

static status_t
virtualdrive_control (void* cookie, uint32 op, void* arg, size_t len)
{
	device_geometry *dg;
	partition_info *pi;
	struct device_inf *dev;
	
	TRACE("virtualdrive: ioctl\n");

	dev = &device_info[(int)cookie];
	
	switch (op) {
		case B_GET_DEVICE_SIZE:
			TRACE("virtualdrive: free\n");
			*(size_t*)arg = dev->size;
			return B_OK;

		case B_SET_NONBLOCKING_IO:
			TRACE("virtualdrive: B_SET_NONBLOCKING_IO\n");
			return B_OK;

		case B_SET_BLOCKING_IO:
			TRACE("virtualdrive: B_SET_BLOCKING_IO\n");
			return B_OK;

		case B_GET_READ_STATUS:
			TRACE("virtualdrive: B_GET_READ_STATUS\n");
			*(bool*)arg = true;
			return B_OK;

		case B_GET_WRITE_STATUS:		
			TRACE("virtualdrive: B_GET_WRITE_STATUS\n");
			*(bool*)arg = true;
			return B_OK;

		case B_GET_ICON:
			TRACE("virtualdrive: B_GET_ICON\n");
			return B_OK;

		case B_GET_GEOMETRY:
			TRACE("virtualdrive: B_GET_GEOMETRY\n");
			dg = (device_geometry *)arg;
			dg->bytes_per_sector = 512;
			dg->sectors_per_track = dev->size / 512;
			dg->cylinder_count = 1;
			dg->head_count = 1;
			dg->device_type = B_DISK;
			dg->removable = false;
			dg->read_only = false;
			dg->write_once = false;
			return B_OK;

		case B_GET_BIOS_GEOMETRY:
			TRACE("virtualdrive: B_GET_BIOS_GEOMETRY\n");
			dg = (device_geometry *)arg;
			dg->bytes_per_sector = 512;
			dg->sectors_per_track = dev->size / (512 * 1024);
			dg->cylinder_count = 1024;
			dg->head_count = 1;
			dg->device_type = B_DISK;
			dg->removable = false;
			dg->read_only = false;
			dg->write_once = false;
			return B_OK;

		case B_GET_PARTITION_INFO:
			// this one never gets called
			TRACE("virtualdrive: B_GET_PARTITION_INFO\n");
			pi = (partition_info *)arg;
			pi->offset = 0;
			pi->size = dev->size;
			pi->logical_block_size = 512;
			pi->session = 1;
			pi->partition = 1;
			strcpy(pi->device,"/boot/home/dummy");
			return B_OK;

		case B_GET_MEDIA_STATUS:
			TRACE("virtualdrive: B_GET_MEDIA_STATUS\n");
			*(status_t*)arg = B_NO_ERROR;
			return B_OK;
	
		case B_SET_UNINTERRUPTABLE_IO:
			TRACE("virtualdrive: B_SET_UNINTERRUPTABLE_IO\n");
			return B_OK;

		case B_SET_INTERRUPTABLE_IO:
			TRACE("virtualdrive: B_SET_INTERRUPTABLE_IO\n");
			return B_OK;

		case B_FLUSH_DRIVE_CACHE:
			TRACE("virtualdrive: B_FLUSH_DRIVE_CACHE\n");
			return B_OK;

		case B_GET_BIOS_DRIVE_ID:
			TRACE("virtualdrive: B_GET_BIOS_DRIVE_ID\n");
			*(uint8*)arg = 0xF8;
			return B_OK;

		case B_GET_DRIVER_FOR_DEVICE:
		case B_SET_DEVICE_SIZE:
		case B_SET_PARTITION:
		case B_FORMAT_DEVICE:
		case B_EJECT_DEVICE:
		case B_LOAD_MEDIA:
		case B_GET_NEXT_OPEN_DEVICE:
			TRACE("virtualdrive: another ioctl: %lx (%lu)\n", op, op);
			return B_BAD_VALUE;
		default:
			TRACE("virtualdrive: unknown ioctl: %lx (%lu)\n", op, op);
			return B_BAD_VALUE;
	}
}


/* -----
	virtualdrive_free - called after the last device is closed, and after
	all i/o is complete.
----- */
static status_t
virtualdrive_free (void* cookie)
{
	TRACE("virtualdrive: free\n");
	return B_OK;
}


/* -----
	null-terminated array of device names supported by this driver
----- */

static const char *virtualdrive_name[] = {
//	"disk/virtual/virtualdrive",
	"disk/virtual/0/raw",
	"disk/virtual/1/raw",
	NULL
};

/* -----
	function pointers for the device hooks entry points
----- */

device_hooks virtualdrive_hooks = {
	virtualdrive_open, 			/* -> open entry point */
	virtualdrive_close, 		/* -> close entry point */
	virtualdrive_free,			/* -> free cookie */
	virtualdrive_control, 		/* -> control entry point */
	virtualdrive_read,			/* -> read entry point */
	virtualdrive_write			/* -> write entry point */
};

/* ----------
	publish_devices - return a null-terminated array of devices
	supported by this driver.
----- */

const char**
publish_devices()
{
	TRACE("virtualdrive: publish_devices\n");
	return virtualdrive_name;
}

/* ----------
	find_device - return ptr to device hooks structure for a
	given device name
----- */

device_hooks*
find_device(const char* name)
{
	return &virtualdrive_hooks;
}

// dev_num_for_path
static
int
dev_num_for_path(const char *path)
{
	int i;
	for (i = 0; i < kDeviceCount; i++) {
		if (!strcmp(path, device_info[i].device_path))
			return i;
	}
	return -1;
}

