/*
** Copyright 2002, Thomas Kurschel. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#ifndef _DRIVERS_H
#define _DRIVERS_H

#include <types.h>
#include <defines.h>

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
/* XXX - no iovec support yet
typedef status_t (*device_readv_hook) (void *cookie, off_t position, const iovec *vec,
					size_t count, size_t *numBytes);
typedef status_t (*device_writev_hook) (void *cookie, off_t position, const iovec *vec,
					size_t count, size_t *numBytes);
*/

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
/* XXX - no iovec support yet */
//	device_readv_hook		readv;		/* scatter-gather read from the device */
//	device_writev_hook		writev;		/* scatter-gather write to the device */
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

#endif /* _DRIVERS_H */
