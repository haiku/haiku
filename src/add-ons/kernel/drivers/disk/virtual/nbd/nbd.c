/*
 * Copyright 2006-2007, François Revol. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/*
 * nbd driver for Haiku
 *
 * Maps a Network Block Device as virtual partitions.
 */

#include <KernelExport.h>
#include <Drivers.h>
#include <Errors.h>
#include <driver_settings.h>
#include <ksocket.h>
#include <netinet/in.h>

#ifdef __HAIKU__
#include <kernel/lock.h>
#else
/* wrappers */
#ifndef _IMPEXP_KERNEL
#define _IMPEXP_KERNEL
#endif
#include "lock.h"
#define benaphore lock
#define benaphore_init new_lock
#define benaphore_destroy free_lock
#define benaphore_lock LOCK
#define benaphore_unlock UNLOCK
#endif

#include "nbd.h"

#define DRV "nbd"
#define D "nbd:"
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
	char target[64]; // "ip:port"
	struct sockaddr_in server;
	benaphore ben;
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

struct nbd_device *nbd_find_device(const char* name);

KSOCKET_MODULE_DECL;

#pragma mark ==== request manager ====


#pragma mark ==== nbd handler ====

int32 postoffice(void *arg)
{
	struct nbd_device *dev = (struct nbd_device *)arg;
	int sock = dev->sock;
	
	
	
	return 0;
}

status_t nbd_connect(struct nbd_device *dev)
{
	
	return B_OK;
}

status_t nbd_teardown(struct nbd_device *dev)
{
	close(dev->sock);
	return B_OK;
}

#pragma mark ==== device hooks ====

static struct nbd_device nbd_devices[MAX_NBDS];

status_t nbd_open(const char *name, uint32 flags, cookie_t **cookie) {
	status_t err;
	struct nbd_device *dev = NULL;
	(void)name; (void)flags;
	dev = nbd_find_device(name);
	if (!dev)
		return ENOENT;
	err = ENOMEM;
	*cookie = (void*)malloc(sizeof(cookie_t));
	if (*cookie == NULL)
		goto err0;
	memset(*cookie, 0, sizeof(cookie_t));
	(*cookie)->dev = dev;
	err = benaphore_lock(&dev->ben);
	if (err)
		goto err1;
	/*  */
	if (dev->sock < 0)
		err = nbd_connect(dev);
	if (err)
		goto err2;
	dev->refcnt++;
	benaphore_unlock(&dev->ben);
	return B_OK;
	
err2:
	benaphore_unlock(&dev->ben);
err1:
	free(*cookie);
err0:
	dprintf("nbd_open : error 0x%08lx\n", err);
	return err;
}

status_t nbd_close(cookie_t *cookie) {
	struct nbd_device *dev = cookie->dev;
	status_t err;
	
	err = benaphore_lock(&dev->ben);
	if (err)
		return err;
	
	// XXX: do something ?
	
	benaphore_unlock(&dev->ben);
	return B_OK;
}

status_t nbd_free(cookie_t *cookie) {
	struct nbd_device *dev = cookie->dev;
	status_t err;
	
	err = benaphore_lock(&dev->ben);
	if (err)
		return err;
	
	if (--dev->refcnt == 0) {
		err = nbd_teardown(dev);
	}
	
	benaphore_unlock(&dev->ben);
	
	free(cookie);
	return err;
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
	status_t err;
	int i, j;
	// XXX: load settings
	void *handle;

	err = ksocket_init();
	if (err < B_OK)
		return err;
	
	for (i = 0; i < MAX_NBDS; i++) {
		memset(nbd_devices[i].target, 0, 64);
		err = benaphore_init(&nbd_devices[i].ben, "nbd lock");
		if (err < B_OK)
			return err; // XXX
		nbd_devices[i].refcnt = 0;
		nbd_devices[i].req = 0LL; /* next ID for requests */
		nbd_devices[i].sock = -1;
		nbd_devices[i].postoffice = -1;
		nbd_devices[i].size = 0LL;
		nbd_devices[i].reqs = NULL;
		nbd_name[i] = malloc(DEVICE_NAME_MAX);
		if (nbd_name[i] == NULL)
			break;
		sprintf(nbd_name[i], DEVICE_FMT, i);
		// XXX: init nbd_devices[i]
	}
	nbd_name[i] = NULL;
	
	handle = load_driver_settings(DRV);
	if (handle) {
		for (i = 0; i < MAX_NBDS; i++) {
			char keyname[10];
			char *v;
			sprintf(keyname, "nbd%d", i);
			v = get_driver_parameter(handle, keyname, NULL, NULL);
			/* should be "ip:port" */
			// XXX: test
			if (v || 0) {
				//strncpy(nbd_devices[i].target, v, 64);
				//XXX:TEST
				//strncpy(nbd_devices[i].target, "127.0.0.1:1337", 64);
				//XXX:parse it
				nbd_devices[i].server.sin_len = sizeof(struct sockaddr_in);
				nbd_devices[i].server.sin_family = AF_INET;
				nbd_devices[i].server.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
				nbd_devices[i].server.sin_port = htons(1337 + i);
			}
		}
		/*should parse as a tree:
		  settings = get_driver_settings(handle);
		  for (i = 0; i < settings->parameter_count; i++) {
		    
		  }
		*/
		
		unload_driver_settings(handle);
	}

	return B_OK;
}

void
uninit_driver (void)
{
	status_t err;
	int i;
	for (i = 0; i < MAX_NBDS; i++) {
		free(nbd_name[i]);
		err = benaphore_destroy(&nbd_devices[i].ben);
	}
	err = ksocket_cleanup();
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

struct nbd_device*
nbd_find_device(const char* name)
{
	int i;
	for (i = 0; i < MAX_NBDS; i++) {
		if (!strcmp(nbd_name[i], name))
			return &nbd_devices[i];
	}
	return NULL;
}
