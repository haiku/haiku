/*
 * Copyright 2006-2007, François Revol. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/*
	nbd driver for Haiku

	Maps BEOS/IMAGE.BE files as virtual partitions.
*/

#include <KernelExport.h>
#include <Drivers.h>
#include <Errors.h>
#include <ksocket.h>

#include "nbd.h"

#define MAX_NBDS 4
#define DEVICE_PREFIX "disk/virtual/nbd/"
#define DEVICE_FMT DEVICE_PREFIX "%2d/raw"
#define DEVICE_NAME_MAX 32
#define MAX_REQ_SIZE (32*1024*1024)

struct nbd_request_entry {
	struct nbd_request_entry *next;
	struct nbd_request req;
	bool r; /* is read */
	size_t len;
	void *buffer; /* write: ptr to passed buffer; read: ptr to malloc()ed extra */
};

struct nbd_device {
	//lock
	vint32 refcnt;
	uint64 req; /* next ID for requests */
	int sock;
	thread_id postoffice;
	uint64 size;
	struct nbd_request_entry *reqs;
};

typedef struct cookie {
	struct nbd_device *dev;
	
} cookie_t;

/* data=NULL on read */
status_t nbd_alloc_request(struct nbd_device, struct nbd_request_entry **req, size_t len, const char *data);
status_t nbd_post_request(struct nbd_device, uint64 handle, struct nbd_request_entry **req);
status_t nbd_dequeue_request(struct nbd_device, uint64 handle, struct nbd_request_entry **req);
status_t nbd_free_request(struct nbd_device, struct nbd_request_entry *req);

#pragma mark ==== request manager ====


#pragma mark ==== nbd handler ====

int32 postoffice(void *arg)
{
	struct nbd_device *dev = (struct nbd_device *)arg;
	int sock = dev->sock;
	
	
	
	return 0;
}

#pragma mark ==== device hooks ====

status_t nbd_open(const char *name, uint32 flags, cookie_t **cookie) {
	(void)name; (void)flags;
	*cookie = (void*)malloc(sizeof(cookie_t));
	if (*cookie == NULL) {
		dprintf("nbd_open : error allocating cookie\n");
		goto err0;
	}
	memset(*cookie, 0, sizeof(cookie_t));
	return B_OK;
err0:
	return B_ERROR;
}

status_t nbd_close(void *cookie) {
	(void)cookie;
	return B_OK;
}

status_t nbd_free(cookie_t *cookie) {
	free(cookie);
	return B_OK;
}

status_t nbd_control(cookie_t *cookie, uint32 op, void *data, size_t len) {
	switch (op) {
	case B_GET_DEVICE_SIZE: /* this one is broken anyway... */
		if (data) {
			*(size_t *)data = (size_t)cookie->dev->size;
			return B_OK;
		}
		return EINVAL;
	case B_SET_DEVICE_SIZE: /* broken */
		return EINVAL;
	case B_SET_NONBLOCKING_IO:
		return EINVAL;
	case B_SET_BLOCKING_IO:
		return B_OK;
	case B_GET_READ_STATUS:
	case B_GET_WRITE_STATUS:
		if (data) {
			*(bool *)data = false;
			return B_OK;
		}
		return EINVAL;
	case B_GET_GEOMETRY:
	case B_GET_BIOS_GEOMETRY:
		if (data) {
			device_geometry *geom = (device_geometry *)data;
			geom->bytes_per_sector = 256;
			geom->sectors_per_track = 1;
			geom->cylinder_count = cookie->dev->size / 256;
			geom->head_count = 1;
			geom->device_type = B_DISK;
			geom->removable = false;
			geom->read_only = false; // XXX
			geom->write_once = false;
			return B_OK;
		}
		return EINVAL;
	case B_GET_MEDIA_STATUS:
		if (data) {
			*(status_t *)data = B_OK;
			return B_OK;
		}
		return EINVAL;
		
	case B_EJECT_DEVICE:
	case B_LOAD_MEDIA:
		return ENOSYS;
	case B_FLUSH_DRIVE_CACHE: /* wait for request list to be empty ? */
	default:
		return ENOSYS;
	}
	return B_NOT_ALLOWED;
}

status_t nbd_read(cookie_t *cookie, off_t position, void *data, size_t *numbytes) {
	*numbytes = 0;
	return B_NOT_ALLOWED;
}

status_t nbd_write(cookie_t *cookie, off_t position, const void *data, size_t *numbytes) {
	(void)cookie; (void)position; (void)data; (void)numbytes;
	*numbytes = 0;
	return EIO;
}

device_hooks nbd_hooks={
	(device_open_hook)nbd_open,
	nbd_close,
	(device_free_hook)nbd_free,
	(device_control_hook)nbd_control,
	(device_read_hook)nbd_read,
	(device_write_hook)nbd_write,
	NULL,
	NULL,
	NULL,
	NULL
};


#pragma mark ==== driver hooks ====

static const char *nbd_name[MAX_NBDS+1] = {
	NULL
};

status_t
init_hardware (void)
{
	return B_OK;
}

status_t
init_driver (void)
{
	int i;
	// load settings
	for (i = 0; i < MAX_NBDS; i++) {
		nbd_name[i] = malloc(DEVICE_NAME_MAX);
		if (nbd_name[i] == NULL)
			break;
		sprintf(nbd_name[i], DEVICE_FMT, i);
	}
	nbd_name[i] = NULL;
	return B_OK;
}

void
uninit_driver (void)
{
	int i;
	for (i = 0; i < MAX_NBDS; i++) {
		free(nbd_name[i]);
	}
}

const char**
publish_devices()
{
	return nbd_name;
}

device_hooks*
find_device(const char* name)
{
	return &nbd_hooks;
}
