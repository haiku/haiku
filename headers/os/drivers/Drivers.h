#ifndef _DRIVERS_DRIVERS_H
#define _DRIVERS_DRIVERS_H

#include <sys/types.h>
#include <defines.h>
#include <sys/uio.h>

/* ---
	these hooks are how the kernel accesses the device
--- */

struct selectsync;
typedef struct selectsync selectsync;

typedef status_t (*device_open_hook) (const char *name, uint32 flags, void **cookie);
typedef status_t (*device_close_hook) (void *cookie);
typedef status_t (*device_free_hook) (void *cookie);
typedef status_t (*device_control_hook) (void *cookie, uint32 op, void *data,
                                         size_t len);
typedef ssize_t  (*device_read_hook) (void *cookie, off_t position, void *data,
                                      size_t *numBytes);
typedef ssize_t  (*device_write_hook) (void *cookie, off_t position,
                                       const void *data, size_t *numBytes);
typedef status_t (*device_select_hook) (void *cookie, uint8 event, uint32 ref,
                                        selectsync *sync);
typedef status_t (*device_deselect_hook) (void *cookie, uint8 event,
                                          selectsync *sync);
typedef status_t (*device_readv_hook) (void *cookie, off_t position, const iovec *vec,
					size_t count, size_t *numBytes);
typedef status_t (*device_writev_hook) (void *cookie, off_t position, const iovec *vec,
					size_t count, size_t *numBytes);

#define	B_CUR_DRIVER_API_VERSION	2

/* ---
	the device_hooks structure is a descriptor for the device, giving its
	entry points.
--- */

typedef struct {
	device_open_hook		open;		/* called to open the device */
	device_close_hook		close;		/* called to close the device */
	device_free_hook		free;		/* called to free the cookie */
	device_control_hook		control;	/* called to control the device */
	device_read_hook		read;		/* reads from the device */
	device_write_hook		write;		/* writes to the device */
	device_select_hook		select;		/* start select */
	device_deselect_hook	deselect;	/* stop select */
	device_readv_hook		readv;		/* scatter-gather read from the device */
	device_writev_hook		writev;		/* scatter-gather write to the device */
} device_hooks;

status_t		init_hardware(void);
const char	  **publish_devices(void);
device_hooks	*find_device(const char *name);
status_t		init_driver(void);
void			uninit_driver(void);	

extern int32	api_version;

enum {
	// called on partition device to get info
	IOCTL_DEVFS_GET_PARTITION_INFO = 1000,
	// called on raw device to declare one partition
	IOCTL_DEVFS_SET_PARTITION,
};

typedef struct devfs_partition_info {
	// offset and size relative to raw device in bytes
	off_t offset;
	off_t size;
	
	// logical block size in bytes
	uint32 logical_block_size;
	
	// session/partition id
	uint32 session;
	uint32 partition;
	
	// path of raw device (GET_PARTITION_INFO only)
	char raw_device[SYS_MAX_PATH_LEN];
} devfs_partition_info;

enum {
	B_GET_DEVICE_SIZE = 1,			/* get # bytes */
									/*   returns size_t in *data */

	B_SET_DEVICE_SIZE,				/* set # bytes */
									/*   passed size_t in *data */

	B_SET_NONBLOCKING_IO,			/* set to non-blocking i/o */

	B_SET_BLOCKING_IO,				/* set to blocking i/o */

	B_GET_READ_STATUS,				/* check if can read w/o blocking */
									/*   returns bool in *data */

	B_GET_WRITE_STATUS,				/* check if can write w/o blocking */
									/*   returns bool in *data */

	B_GET_GEOMETRY,					/* get info about device geometry */
									/*   returns struct geometry in *data */

	B_GET_DRIVER_FOR_DEVICE,		/* get the path of the executable serving that device */

	B_GET_PARTITION_INFO,			/* get info about a device partition */
									/*   returns struct partition_info in *data */

	B_SET_PARTITION,				/* create a user-defined partition */

	B_FORMAT_DEVICE,				/* low-level device format */

	B_EJECT_DEVICE,					/* eject the media if supported */

	B_GET_ICON,						/* return device icon (see struct below) */

	B_GET_BIOS_GEOMETRY,			/* get info about device geometry */
									/* as reported by the bios */
									/*   returns struct geometry in *data */

	B_GET_MEDIA_STATUS,				/* get status of media. */
									/* return status_t in *data: */
									/* B_NO_ERROR: media ready */
									/* B_DEV_NO_MEDIA: no media */
									/* B_DEV_NOT_READY: device not ready */
									/* B_DEV_MEDIA_CHANGED: media changed */
									/*  since open or last B_GET_MEDIA_STATUS */
									/* B_DEV_MEDIA_CHANGE_REQUESTED: user */
									/*  pressed button on drive */
									/* B_DEV_DOOR_OPEN: door open */
	
	B_LOAD_MEDIA,					/* load the media if supported */
	
	B_GET_BIOS_DRIVE_ID,			/* get bios id for this device */

	B_SET_UNINTERRUPTABLE_IO,		/* prevent cntl-C from interrupting i/o */
	B_SET_INTERRUPTABLE_IO,			/* allow cntl-C to interrupt i/o */

	B_FLUSH_DRIVE_CACHE,			/* flush drive cache */

	B_GET_NEXT_OPEN_DEVICE = 1000,	/* iterate through open devices */
	B_ADD_FIXED_DRIVER,				/* private */
	B_REMOVE_FIXED_DRIVER,			/* private */

	B_AUDIO_DRIVER_BASE = 8000,		/* base for codes in audio_driver.h */
	B_MIDI_DRIVER_BASE = 8100,		/* base for codes in midi_driver.h */
	B_JOYSTICK_DRIVER_BASE = 8200,	/* base for codes in joystick.h */
	B_GRAPHIC_DRIVER_BASE = 8300,	/* base for codes in graphic_driver.h */

	B_DEVICE_OP_CODES_END = 9999	/* end of Be-defined contol id's */
};

#endif /* _DRIVERS_DRIVERS_H */
