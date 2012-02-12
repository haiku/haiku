/*
 * Copyright 2006-2007, François Revol. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/*
 * nbd driver for Haiku
 *
 * Maps a Network Block Device as virtual partitions.
 */


#include <ByteOrder.h>
#include <KernelExport.h>
#include <Drivers.h>
#include <driver_settings.h>
#include <Errors.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ksocket.h>
#include <netinet/in.h>

//#define DEBUG 1

/* on the first open(), open ourselves for some seconds, 
 * to avoid trying to reconnect and failing on a 2nd open,
 * as it happens with the python server.
 */
//#define MOUNT_KLUDGE


/* names, ohh names... */
#ifndef SHUT_RDWR
#define SHUT_RDWR SHUTDOWN_BOTH 
#endif

/* locking support */
#ifdef __HAIKU__
#include <kernel/lock.h>
#else
/* wrappers for R5 */
#ifndef _IMPEXP_KERNEL
#define _IMPEXP_KERNEL
#endif
#include "lock.h"
#define mutex lock
#define mutex_init new_lock
#define mutex_destroy free_lock
#define mutex_lock LOCK
#define mutex_unlock UNLOCK
#endif

#define DEBUG 1

#include "nbd.h"

#define DRV "nbd"
#define DP "nbd:"
#define MAX_NBDS 4
#define DEVICE_PREFIX "disk/virtual/nbd/"
#define DEVICE_FMT DEVICE_PREFIX "%d/raw"
#define DEVICE_NAME_MAX 32
#define MAX_REQ_SIZE (32*1024*1024)
#define BLKSIZE 512

/* debugging */
#if DEBUG
#define PRINT(a) dprintf a
#define WHICH(dev) ((int)(dev - nbd_devices))
#else
#define PRINT(a)
#endif

struct nbd_request_entry {
	struct nbd_request_entry *next;
	struct nbd_request req; /* net byte order */
	struct nbd_reply reply; /* net byte order */
	sem_id sem;
	bool replied;
	bool discard;
	uint64 handle;
	uint32 type;
	uint64 from;
	size_t len;
	void *buffer; /* write: ptr to passed buffer; read: ptr to malloc()ed extra */
};

struct nbd_device {
	bool valid;
	bool readonly;
	struct sockaddr_in server;
	mutex ben;
	vint32 refcnt;
	uint64 req; /* next ID for requests */
	int sock;
	thread_id postoffice;
	uint64 size;
	struct nbd_request_entry *reqs;
#ifdef MOUNT_KLUDGE
	int kludge;
#endif
};

typedef struct cookie {
	struct nbd_device *dev;
	
} cookie_t;

/* data=NULL on read */
status_t nbd_alloc_request(struct nbd_device *dev, struct nbd_request_entry **req, uint32 type, off_t from, size_t len, const char *data);
status_t nbd_queue_request(struct nbd_device *dev, struct nbd_request_entry *req);
status_t nbd_dequeue_request(struct nbd_device *dev, uint64 handle, struct nbd_request_entry **req);
status_t nbd_free_request(struct nbd_device *dev, struct nbd_request_entry *req);

struct nbd_device *nbd_find_device(const char* name);

int32 nbd_postoffice(void *arg);
status_t nbd_connect(struct nbd_device *dev);
status_t nbd_teardown(struct nbd_device *dev);
status_t nbd_post_request(struct nbd_device *dev, struct nbd_request_entry *req);

status_t nbd_open(const char *name, uint32 flags, cookie_t **cookie);
status_t nbd_close(cookie_t *cookie);
status_t nbd_free(cookie_t *cookie);
status_t nbd_control(cookie_t *cookie, uint32 op, void *data, size_t len);
status_t nbd_read(cookie_t *cookie, off_t position, void *data, size_t *numbytes);
status_t nbd_write(cookie_t *cookie, off_t position, const void *data, size_t *numbytes);

KSOCKET_MODULE_DECL;

/* HACK:
 * In BONE at least, if connect() fails (EINTR or ETIMEDOUT)
 * keeps locked pages around (likely a bone_data,
 * until TCP gets the last ACK). If that happens, we snooze()
 * in unload_driver() to let TCP timeout before the kernel
 * tries to delete the image. */
bool gDelayUnload = false;
#define BONE_TEARDOWN_DELAY 60000000

#if 0
#pragma mark ==== support ====
#endif

// move that to ksocket inlined
static int kinet_aton(const char *in, struct in_addr *addr)
{
	int i;
	unsigned long a;
	uint32 inaddr = 0L;
	char *p = (char *)in;
	for (i = 0; i < 4; i++) {
		a = strtoul(p, &p, 10);
		if (!p)
			return -1;
		inaddr = (inaddr >> 8) | ((a & 0x0ff) << 24);
		*(uint32 *)addr = inaddr;
		if (!*p)
			return 0;
		p++;
	}
	return 0;
}

#if 0
#pragma mark ==== request manager ====
#endif

status_t nbd_alloc_request(struct nbd_device *dev, struct nbd_request_entry **req, uint32 type, off_t from, size_t len, const char *data)
{
	bool w = (type == NBD_CMD_WRITE);
	struct nbd_request_entry *r;
	status_t err = EINVAL;
	uint64 handle;
	PRINT((DP ">%s(%ld, %Ld, %ld)\n", __FUNCTION__, type, from, len));
	
	if (type != NBD_CMD_READ && type != NBD_CMD_WRITE && type != NBD_CMD_DISC)
		return err;
	if (!dev || !req || from < 0)
		return err;
	
	//LOCK
	err = mutex_lock(&dev->ben);
	if (err)
		return err;

	// atomic
	handle = dev->req++;
	
	
	//UNLOCK
	mutex_unlock(&dev->ben);
	
	err = ENOMEM;
	r = malloc(sizeof(struct nbd_request_entry) + (w ? 0 : len));
	if (r == NULL)
		goto err0;
	r->next = NULL;
	err = r->sem = create_sem(0, "nbd request sem");
	if (err < 0)
		goto err1;
	
	r->replied = false;
	r->discard = false;
	r->handle = handle;
	r->type = type;
	r->from = from;
	r->len = len;
	
	r->req.magic = B_HOST_TO_BENDIAN_INT32(NBD_REQUEST_MAGIC);
	r->req.type = B_HOST_TO_BENDIAN_INT32(type);
	r->req.handle = B_HOST_TO_BENDIAN_INT64(r->handle);
	r->req.from = B_HOST_TO_BENDIAN_INT64(r->from);
	r->req.len = B_HOST_TO_BENDIAN_INT32(len);
	
	r->buffer = (void *)(w ? data : (((char *)r) + sizeof(struct nbd_request_entry)));
	
	*req = r;
	return B_OK;

err1:
	free(r);
err0:
	dprintf(DP " %s: error 0x%08lx\n", __FUNCTION__, err);
	return err;
}


status_t nbd_queue_request(struct nbd_device *dev, struct nbd_request_entry *req)
{
	PRINT((DP ">%s(handle:%Ld)\n", __FUNCTION__, req->handle));
	req->next = dev->reqs;
	dev->reqs = req;
	return B_OK;
}


status_t nbd_dequeue_request(struct nbd_device *dev, uint64 handle, struct nbd_request_entry **req)
{
	struct nbd_request_entry *r, *prev;
	PRINT((DP ">%s(handle:%Ld)\n", __FUNCTION__, handle));
	r = dev->reqs;
	prev = NULL;
	while (r && r->handle != handle) {
		prev = r;
		r = r->next;
	}
	if (!r)
		return ENOENT;
	
	if (prev)
		prev->next = r->next;
	else
		dev->reqs = r->next;
	
	*req = r;
	return B_OK;
}


status_t nbd_free_request(struct nbd_device *dev, struct nbd_request_entry *req)
{
	PRINT((DP ">%s(handle:%Ld)\n", __FUNCTION__, req->handle));
	delete_sem(req->sem);
	free(req);
	return B_OK;
}


#if 0
#pragma mark ==== nbd handler ====
#endif

int32 nbd_postoffice(void *arg)
{
	struct nbd_device *dev = (struct nbd_device *)arg;
	struct nbd_request_entry *req = NULL;
	struct nbd_reply reply;
	status_t err;
	const char *reason;
	PRINT((DP ">%s()\n", __FUNCTION__));
	
	for (;;) {
		reason = "recv";
		err = krecv(dev->sock, &reply, sizeof(reply), 0);
		if (err == -1 && errno < 0)
			err = errno;
		if (err < 0)
			goto err;
		reason = "recv:size";
		if (err < sizeof(reply))
			err = EINVAL;
		if (err < 0)
			goto err;
		reason = "magic";
		err = EINVAL;
		if (B_BENDIAN_TO_HOST_INT32(reply.magic) != NBD_REPLY_MAGIC)
			goto err;
		
		reason = "lock";
		//LOCK
		err = mutex_lock(&dev->ben);
		if (err)
			goto err;
		
		reason = "dequeue_request";
		err = nbd_dequeue_request(dev, B_BENDIAN_TO_HOST_INT64(reply.handle), &req);
		
		//UNLOCK
		mutex_unlock(&dev->ben);
		
		if (!err && !req) {
			dprintf(DP "nbd_dequeue_rquest found NULL!\n");
			err = ENOENT;
		}
		
		if (err == B_OK) {
			memcpy(&req->reply, &reply, sizeof(reply));
			if (req->type == NBD_CMD_READ) {
				err = 0;
				reason = "recv(data)";
				if (reply.error == 0)
					err = krecv(dev->sock, req->buffer, req->len, 0);
				if (err < 0)
					goto err;
				/* tell back how much we've got (?) */
				req->len = err;
			} else {
				if (reply.error)
					req->len = 0;
			}
			
			reason = "lock";
			//LOCK
			err = mutex_lock(&dev->ben);
			if (err)
				goto err;
			
			// this also must be atomic!
			release_sem(req->sem);
			req->replied = true;
			if (req->discard)
				nbd_free_request(dev, req);
			
			//UNLOCK
			mutex_unlock(&dev->ben);
		}
		
	}
	
	PRINT((DP "<%s\n", __FUNCTION__));
	return 0;

err:
	dprintf(DP "%s: %s: error 0x%08lx\n", __FUNCTION__, reason, err);
	return err;
}


status_t nbd_connect(struct nbd_device *dev)
{
	struct nbd_init_packet initpkt;
	status_t err;
	PRINT((DP ">%s()\n", __FUNCTION__));

	PRINT((DP " %s: socket()\n", __FUNCTION__));
	err = dev->sock = ksocket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (err == -1 && errno < 0)
		err = errno;
	if (err < 0)
		goto err0;
	
	PRINT((DP " %s: connect()\n", __FUNCTION__));
	err = kconnect(dev->sock, (struct sockaddr *)&dev->server, sizeof(dev->server));
	//err = ENOSYS;
	if (err == -1 && errno < 0)
		err = errno;
	/* HACK: avoid the kernel unloading us with locked pages from TCP */
	if (err)
		gDelayUnload = true;
	if (err)
		goto err1;
	
	PRINT((DP " %s: recv(initpkt)\n", __FUNCTION__));
	err = krecv(dev->sock, &initpkt, sizeof(initpkt), 0);
	if (err == -1 && errno < 0)
		err = errno;
	if (err < sizeof(initpkt))
		goto err2;
	err = EINVAL;//EPROTO;
	if (memcmp(initpkt.passwd, NBD_INIT_PASSWD, sizeof(initpkt.passwd)))
		goto err3;
	if (B_BENDIAN_TO_HOST_INT64(initpkt.magic) != NBD_INIT_MAGIC)
		goto err3;
	
	dev->size = B_BENDIAN_TO_HOST_INT64(initpkt.device_size);
	
	dprintf(DP " %s: connected, device size %Ld bytes.\n", __FUNCTION__, dev->size);

	err = dev->postoffice = spawn_kernel_thread(nbd_postoffice, "nbd postoffice", B_REAL_TIME_PRIORITY, dev);
	if (err < B_OK)
		goto err4;
	resume_thread(dev->postoffice);
	
	PRINT((DP "<%s\n", __FUNCTION__));
	return B_OK;
	
err4:
	dev->postoffice = -1;
err3:
err2:
err1:
	kclosesocket(dev->sock);
	dev->sock = -1;
err0:
	dprintf(DP "<%s: error 0x%08lx\n", __FUNCTION__, err);
	return err;
}


status_t nbd_teardown(struct nbd_device *dev)
{
	status_t ret;
	PRINT((DP ">%s()\n", __FUNCTION__));
	kshutdown(dev->sock, SHUT_RDWR);
	kclosesocket(dev->sock);
	dev->sock = -1;
	wait_for_thread(dev->postoffice, &ret);
	return B_OK;
}


status_t nbd_post_request(struct nbd_device *dev, struct nbd_request_entry *req)
{
	status_t err;
	PRINT((DP ">%s(handle:%Ld)\n", __FUNCTION__, req->handle));
	
	err = ksend(dev->sock, &req->req, sizeof(req->req), 0);
	if (err < 0)
		return err;

	if (req->type == NBD_CMD_WRITE)
		err = ksend(dev->sock, req->buffer, req->len, 0);
	if (err < 0)
		return err;
	else
		req->len = err;
	
	err = nbd_queue_request(dev, req);
	return err;
}


#if 0
#pragma mark ==== device hooks ====
#endif

static struct nbd_device nbd_devices[MAX_NBDS];

status_t nbd_open(const char *name, uint32 flags, cookie_t **cookie) {
	status_t err;
#ifdef MOUNT_KLUDGE
	int32 refcnt;
	int kfd;
#endif
	struct nbd_device *dev = NULL;
	PRINT((DP ">%s(%s, %lx, )\n", __FUNCTION__, name, flags));
	(void)name; (void)flags;
	dev = nbd_find_device(name);
	if (!dev || !dev->valid)
		return ENOENT;
	err = ENOMEM;
	*cookie = (void*)malloc(sizeof(cookie_t));
	if (*cookie == NULL)
		goto err0;
	memset(*cookie, 0, sizeof(cookie_t));
	(*cookie)->dev = dev;
	err = mutex_lock(&dev->ben);
	if (err)
		goto err1;
	/*  */
	if (dev->sock < 0)
		err = nbd_connect(dev);
	if (err)
		goto err2;
#ifdef MOUNT_KLUDGE
	refcnt = dev->refcnt++;
	kfd = dev->kludge;
	dev->kludge = -1;
#endif
	mutex_unlock(&dev->ben);
	
#ifdef MOUNT_KLUDGE
	if (refcnt == 0) {
		char buf[32];
		sprintf(buf, "/dev/%s", name);
		dev->kludge = open(buf, O_RDONLY);
	} else if (kfd) {
		close(kfd);
	}
#endif
	
	return B_OK;
	
err2:
	mutex_unlock(&dev->ben);
err1:
	free(*cookie);
err0:
	dprintf(DP " %s: error 0x%08lx\n", __FUNCTION__, err);
	return err;
}


status_t nbd_close(cookie_t *cookie) {
	struct nbd_device *dev = cookie->dev;
	status_t err;
#ifdef MOUNT_KLUDGE
	int kfd = -1;
#endif
	PRINT((DP ">%s(%d)\n", __FUNCTION__, WHICH(cookie->dev)));
	
	err = mutex_lock(&dev->ben);
	if (err)
		return err;
	
	// XXX: do something ?
#ifdef MOUNT_KLUDGE
	kfd = dev->kludge;
	dev->kludge = -1;
#endif
	
	mutex_unlock(&dev->ben);

#ifdef MOUNT_KLUDGE
	if (kfd > -1) {
		close(kfd);
	}
#endif
	return B_OK;
}


status_t nbd_free(cookie_t *cookie) {
	struct nbd_device *dev = cookie->dev;
	status_t err;
	PRINT((DP ">%s(%d)\n", __FUNCTION__, WHICH(cookie->dev)));
	
	err = mutex_lock(&dev->ben);
	if (err)
		return err;
	
	if (--dev->refcnt == 0) {
		err = nbd_teardown(dev);
	}
	
	mutex_unlock(&dev->ben);
	
	free(cookie);
	return err;
}


status_t nbd_control(cookie_t *cookie, uint32 op, void *data, size_t len) {
	PRINT((DP ">%s(%d, %lu, , %ld)\n", __FUNCTION__, WHICH(cookie->dev), op, len));
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
			geom->bytes_per_sector = BLKSIZE;
			geom->sectors_per_track = 1;
			geom->cylinder_count = cookie->dev->size / BLKSIZE;
			geom->head_count = 1;
			geom->device_type = B_DISK;
			geom->removable = false;
			geom->read_only = cookie->dev->readonly;
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
		return B_BAD_VALUE;
	case B_FLUSH_DRIVE_CACHE: /* wait for request list to be empty ? */
		return B_OK;
	default:
		return B_BAD_VALUE;
	}
	return B_NOT_ALLOWED;
}


status_t nbd_read(cookie_t *cookie, off_t position, void *data, size_t *numbytes) {
	struct nbd_device *dev = cookie->dev;
	struct nbd_request_entry *req;
	status_t err, semerr;
	PRINT((DP ">%s(%d, %Ld, , )\n", __FUNCTION__, WHICH(cookie->dev), position));
	
	if (position < 0)
		return EINVAL;
	if (!data)
		return EINVAL;
	
	err = nbd_alloc_request(dev, &req, NBD_CMD_READ, position, *numbytes, NULL);
	if (err)
		goto err0;
	
	//LOCK
	err = mutex_lock(&dev->ben);
	if (err)
		goto err1;
	
	err = nbd_post_request(dev, req);
	
	//UNLOCK
	mutex_unlock(&dev->ben);

	if (err)
		goto err2;


	semerr = acquire_sem(req->sem);
	
	//LOCK
	err = mutex_lock(&dev->ben);
	if(err)
		goto err3;
	
	/* bad scenarii */
	if (!req->replied)
		req->discard = true;
	else if (semerr)
		nbd_free_request(dev, req);
	
	//UNLOCK
	mutex_unlock(&dev->ben);

	if (semerr == B_OK) {
		*numbytes = req->len;
		memcpy(data, req->buffer, req->len);
		err = B_OK;
		if (*numbytes == 0 && req->reply.error)
			err = EIO;
		nbd_free_request(dev, req);
		return err;
	}
	
	*numbytes = 0;
	return semerr;
			

err3:
err2:
err1:
	nbd_free_request(dev, req);
err0:
	*numbytes = 0;
	return err;
}


status_t nbd_write(cookie_t *cookie, off_t position, const void *data, size_t *numbytes) {
	struct nbd_device *dev = cookie->dev;
	struct nbd_request_entry *req;
	status_t err, semerr;
	PRINT((DP ">%s(%d, %Ld, %ld, )\n", __FUNCTION__, WHICH(cookie->dev), position, *numbytes));
	
	if (position < 0)
		return EINVAL;
	if (!data)
		return EINVAL;
	err = B_NOT_ALLOWED;
	if (dev->readonly)
		goto err0;
	
	err = nbd_alloc_request(dev, &req, NBD_CMD_WRITE, position, *numbytes, data);
	if (err)
		goto err0;
	
	//LOCK
	err = mutex_lock(&dev->ben);
	if (err)
		goto err1;
	
	/* sending request+data must be atomic */
	err = nbd_post_request(dev, req);
	
	//UNLOCK
	mutex_unlock(&dev->ben);

	if (err)
		goto err2;


	semerr = acquire_sem(req->sem);
	
	//LOCK
	err = mutex_lock(&dev->ben);
	if(err)
		goto err3;
	
	/* bad scenarii */
	if (!req->replied)
		req->discard = true;
	else if (semerr)
		nbd_free_request(dev, req);
	
	//UNLOCK
	mutex_unlock(&dev->ben);

	if (semerr == B_OK) {
		*numbytes = req->len;
		err = B_OK;
		if (*numbytes == 0 && req->reply.error)
			err = EIO;
		nbd_free_request(dev, req);
		return err;
	}
	
	*numbytes = 0;
	return semerr;
			

err3:
err2:
err1:
	nbd_free_request(dev, req);
err0:
	*numbytes = 0;
	return err;
}


device_hooks nbd_hooks={
	(device_open_hook)nbd_open,
	(device_close_hook)nbd_close,
	(device_free_hook)nbd_free,
	(device_control_hook)nbd_control,
	(device_read_hook)nbd_read,
	(device_write_hook)nbd_write,
	NULL,
	NULL,
	NULL,
	NULL
};

#if 0
#pragma mark ==== driver hooks ====
#endif

int32 api_version = B_CUR_DRIVER_API_VERSION;

static char *nbd_name[MAX_NBDS+1] = {
	NULL
};


status_t
init_hardware (void)
{
	PRINT((DP ">%s()\n", __FUNCTION__));
	return B_OK;
}


status_t
init_driver (void)
{
	status_t err;
	int i, j;
	// XXX: load settings
	void *handle;
	char **names = nbd_name;
	PRINT((DP ">%s()\n", __FUNCTION__));

	handle = load_driver_settings(DRV);
	if (handle == NULL)
		return ENOENT;
	// XXX: test for boot args ?
	
	
	err = ksocket_init();
	if (err < B_OK)
		return err;
	
	for (i = 0; i < MAX_NBDS; i++) {
		nbd_devices[i].valid = false;
		nbd_devices[i].readonly = false;
		mutex_init(&nbd_devices[i].ben, "nbd lock");
		nbd_devices[i].refcnt = 0;
		nbd_devices[i].req = 0LL; /* next ID for requests */
		nbd_devices[i].sock = -1;
		nbd_devices[i].postoffice = -1;
		nbd_devices[i].size = 0LL;
		nbd_devices[i].reqs = NULL;
#ifdef MOUNT_KLUDGE
		nbd_devices[i].kludge = -1;
#endif
		nbd_name[i] = NULL;
	}
	
	for (i = 0; i < MAX_NBDS; i++) {
		const driver_settings *settings = get_driver_settings(handle);
		driver_parameter *p = NULL;
		char keyname[10];
		sprintf(keyname, "%d", i);
		for (j = 0; j < settings->parameter_count; j++)
			if (!strcmp(settings->parameters[j].name, keyname))
				p = &settings->parameters[j];
		if (!p)
			continue;
		for (j = 0; j < p->parameter_count; j++) {
			if (!strcmp(p->parameters[j].name, "readonly"))
				nbd_devices[i].readonly = true;
			if (!strcmp(p->parameters[j].name, "server")) {
				if (p->parameters[j].value_count < 2)
					continue;
				nbd_devices[i].server.sin_len = sizeof(struct sockaddr_in);
				nbd_devices[i].server.sin_family = AF_INET;
				kinet_aton(p->parameters[j].values[0], &nbd_devices[i].server.sin_addr);
				nbd_devices[i].server.sin_port = htons(atoi(p->parameters[j].values[1]));
				dprintf(DP " configured [%d]\n", i);
				*(names) = malloc(DEVICE_NAME_MAX);
				if (*(names) == NULL)
					return ENOMEM;
				sprintf(*(names++), DEVICE_FMT, i);
				nbd_devices[i].valid = true;
			}
		}
	}
	*names = NULL;
		
	unload_driver_settings(handle);
	return B_OK;
}


void
uninit_driver (void)
{
	int i;
	PRINT((DP ">%s()\n", __FUNCTION__));
	for (i = 0; i < MAX_NBDS; i++) {
		free(nbd_name[i]);
		mutex_destroy(&nbd_devices[i].ben);
	}
	ksocket_cleanup();
	/* HACK */
	if (gDelayUnload)
		snooze(BONE_TEARDOWN_DELAY);
}


const char**
publish_devices()
{
	PRINT((DP ">%s()\n", __FUNCTION__));
	return (const char **)nbd_name;
}


device_hooks*
find_device(const char* name)
{
	PRINT((DP ">%s(%s)\n", __FUNCTION__, name));
	return &nbd_hooks;
}


struct nbd_device*
nbd_find_device(const char* name)
{
	int i;
	PRINT((DP ">%s(%s)\n", __FUNCTION__, name));
	for (i = 0; i < MAX_NBDS; i++) {
		char buf[DEVICE_NAME_MAX];
		sprintf(buf, DEVICE_FMT, i);
		if (!strcmp(buf, name))
			return &nbd_devices[i];
	}
	return NULL;
}

