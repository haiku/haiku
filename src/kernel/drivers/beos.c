/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <kernel.h>
#include <elf.h>
#include <sem.h>
#include <arch/cpu.h>
#include <debug.h>
#include <memheap.h>
#include <devfs.h>
#include <beos.h>
#include <string.h>
#include <stdio.h>
#include <errors.h>
#include <atomic.h>

#ifdef 0 //ARCH_x86

// contains a bunch of beos defines
#include "beos_p.h"

// private funcs
static uint8 read8(int port);
static void write8(int port, uint8 data);
static uint16 read16(int port);
static void write16(int port, uint16 data);
static uint32 read32(int port);
static void write32(int port, uint32 data);
static void unhandled_isa_call(void);

/*
static int translation_open(void * ident, void * *cookie);
static int translation_close(void * cookie);
static int translation_freecookie(void * cookie);
static int translation_seek(void * cookie, off_t pos, seek_type st);
static int translation_ioctl(void * cookie, int op, void *buf, size_t len);
static ssize_t translation_read(void * cookie, void *buf, off_t pos, ssize_t len);
static ssize_t translation_write(void * cookie, const void *buf, off_t pos, ssize_t len);
static int translation_canpage(void * ident);
static ssize_t translation_readpage(void * ident, iovecs *vecs, off_t pos);
static ssize_t translation_writepage(void * ident, iovecs *vecs, off_t pos);
*/

static int newos2beos_err(int err);
static int beos2newos_err(int err);

isa_module_info isa = {
	{ /* binfo */
		{ "isa", 0, NULL }, /* minfo */
		(void *)unhandled_isa_call
	},
	read8,
	write8,
	read16,
	write16,
	read32,
	write32,
	(void *)unhandled_isa_call,
	(void *)unhandled_isa_call,
	(void *)unhandled_isa_call,
	(void *)unhandled_isa_call,
	(void *)unhandled_isa_call,
	(void *)unhandled_isa_call
};

//struct module {
//	const char *name;
//	void *data;
//} modules[] = {
//	{ B_ISA_MODULE_NAME, &isa },
//	{ NULL, NULL }
//};

int beos_layer_init(void)
{
	return 0;
}

int _beos_atomic_add(volatile int *val, int incr)
{
	return atomic_add(val, incr);
}

int _beos_atomic_and(volatile int *val, int incr)
{
	return atomic_and(val, incr);
}

int _beos_atomic_or(volatile int *val, int incr)
{
	return atomic_or(val, incr);
}

int _beos_acquire_sem(sem_id id)
{
	return newos2beos_err(sem_acquire(id, 1));
}

int _beos_acquire_sem_etc(sem_id id, uint32 count, uint32 flags, bigtime_t timeout)
{
	int nuflags = 0;
	bigtime_t nutimeout = timeout;

	if(flags & B_CAN_INTERRUPT)
		nuflags |= SEM_FLAG_INTERRUPTABLE;
	if(flags & B_DO_NOT_RESCHEDULE)
		nuflags |= SEM_FLAG_NO_RESCHED;
	if(flags & B_RELATIVE_TIMEOUT)
		nuflags |= SEM_FLAG_TIMEOUT;
	if(flags & B_ABSOLUTE_TIMEOUT) {
		nuflags |= SEM_FLAG_TIMEOUT;
		nutimeout = timeout - system_time();
		if(nutimeout < 0)
			nutimeout = 0;
	}
	return newos2beos_err(sem_acquire_etc(id, count, nuflags, nutimeout, NULL));
}

sem_id _beos_create_sem(uint32 count, const char *name)
{
	return newos2beos_err(sem_create(count, name));
}

int _beos_delete_sem(sem_id id)
{
	return newos2beos_err(sem_delete(id));
}

int _beos_get_sem_count(sem_id id, int32 *count)
{
	panic("_beos_get_sem_count: not supported\n");
	return 0;
}

int _beos_release_sem(sem_id id)
{
	return newos2beos_err(sem_release(id, 1));
}

int _beos_release_sem_etc(sem_id id, int32 count, uint32 flags)
{
	int nuflags = 0;

	if(flags & B_DO_NOT_RESCHEDULE)
		nuflags = SEM_FLAG_NO_RESCHED;

	return newos2beos_err(sem_release_etc(id, count, nuflags));
}

int _beos_strcmp(const char *cs, const char *ct)
{
	return strcmp(cs, ct);
}

void _beos_spin(bigtime_t microseconds)
{
	bigtime_t time = system_time();

	while((system_time() - time) < microseconds)
		;
}

/*
int _beos_get_module(const char *path, module_info **vec)
{
	struct module *m;

	dprintf("get_module: called on '%s'\n", path);
	for(m = modules; m->name; m++) {
		if(!strcmp(path, m->name)) {
			*vec = m->data;
			return NO_ERROR;
		}
	}
	return ERR_GENERAL;
}

int _beos_put_module(const char *path)
{
	dprintf("put_module: called on '%s'\n", path);
	return NO_ERROR;
}
*/

static uint8 read8(int port)
{
	return in8(port);
}

static void write8(int port, uint8 data)
{
//	dprintf("w8 0x%x, 0x%x\n", port, data);
	out8(data, port);
}

static uint16 read16(int port)
{
	return in16(port);
}

static void write16(int port, uint16 data)
{
//	dprintf("w16 0x%x, 0x%x\n", port, data);
	out16(data, port);
}

static uint32 read32(int port)
{
	return in32(port);
}

static void write32(int port, uint32 data)
{
//	dprintf("w32 0x%x, 0x%x\n", port, data);
	out32(data, port);
}

static void unhandled_isa_call(void)
{
	panic("call into unhandled isa function\n");
}

static int beos2newos_err(int err)
{
	if(err >= 0)
		return err;

	switch(err) {
		case B_NO_MEMORY: return ERR_NO_MEMORY;
		case B_IO_ERROR: return ERR_IO_ERROR;
		case B_PERMISSION_DENIED: return ERR_PERMISSION_DENIED;
		case B_NAME_NOT_FOUND: return ERR_NOT_FOUND;
		case B_TIMED_OUT: return ERR_SEM_TIMED_OUT;
		case B_INTERRUPTED: return ERR_SEM_INTERRUPTED;
		case B_NOT_ALLOWED: return ERR_NOT_ALLOWED;
		case B_ERROR: return ERR_GENERAL;
		case B_BAD_SEM_ID: return ERR_INVALID_HANDLE;
		case B_NO_MORE_SEMS: return ERR_SEM_OUT_OF_SLOTS;
		case B_BAD_THREAD_ID: return ERR_INVALID_HANDLE;
		case B_NO_MORE_THREADS: return ERR_NO_MORE_HANDLES;
		case B_BAD_TEAM_ID: return ERR_INVALID_HANDLE;
		case B_NO_MORE_TEAMS: return ERR_NO_MORE_HANDLES;
		case B_BAD_PORT_ID: return ERR_INVALID_HANDLE;
		case B_NO_MORE_PORTS: return ERR_NO_MORE_HANDLES;
		case B_BAD_IMAGE_ID: return ERR_INVALID_HANDLE;
		case B_NOT_AN_EXECUTABLE: return ERR_INVALID_BINARY;
		default: return ERR_GENERAL;
	}
}

static int newos2beos_err(int err)
{
	if(err >= 0)
		return err;

	switch(err) {
		case ERR_NO_MEMORY: return B_NO_MEMORY;
		case ERR_IO_ERROR: return B_IO_ERROR;
		case ERR_TIMED_OUT: return B_TIMED_OUT;
		case ERR_NOT_ALLOWED: return B_NOT_ALLOWED;
		case ERR_PERMISSION_DENIED: return B_PERMISSION_DENIED;
		case ERR_INVALID_BINARY: return B_NOT_AN_EXECUTABLE;

		case ERR_SEM_DELETED: return B_CANCELED;
		case ERR_SEM_TIMED_OUT: return B_TIMED_OUT;
		case ERR_SEM_OUT_OF_SLOTS: return B_NO_MORE_SEMS;
		case ERR_SEM_INTERRUPTED: return B_INTERRUPTED;
		default: return B_ERROR;
	}
}

/*
static int translation_open(void * ident, void * *_cookie)
{
	struct beos_device_node *node = (struct beos_device_node *)ident;
	struct beos_device_cookie *cookie;
	void *beos_cookie;
	int err;

	err = node->beos_hooks->open(node->name, 0, &beos_cookie);
	if(err < 0)
		return beos2newos_err(err);

	cookie = kmalloc(sizeof(struct beos_device_cookie));
	if(!cookie)
		return ERR_NO_MEMORY;

	cookie->node = node;
	cookie->beos_cookie = beos_cookie;
	cookie->pos = 0;

	*_cookie = cookie;

	return NO_ERROR;
}

static int translation_close(void * _cookie)
{
	struct beos_device_cookie *cookie = _cookie;
	int err;

	err = cookie->node->beos_hooks->close(cookie->beos_cookie);
	if(err < 0)
		return beos2newos_err(err);

	return NO_ERROR;
}

static int translation_freecookie(void * _cookie)
{
	struct beos_device_cookie *cookie = _cookie;
	int err;

	err = cookie->node->beos_hooks->free(cookie->beos_cookie);
	if(err < 0)
		return beos2newos_err(err);

	kfree(cookie);

	return NO_ERROR;
}

static int translation_seek(void * _cookie, off_t pos, seek_type st)
{
	struct beos_device_cookie *cookie = _cookie;
	int err = NO_ERROR;
	off_t nupos;

	switch(st) {
	case SEEK_SET:
		if(pos < 0)
			pos = 0;
 		cookie->pos = pos;
		break;
	case SEEK_CUR:
		nupos = cookie->pos + pos;
		if(nupos < 0)
			nupos = 0;
		cookie->pos = nupos;
		break;
	case SEEK_END:
	default:
		err = ERR_INVALID_ARGS;
	}
	return err;
}

static int translation_ioctl(void * _cookie, int op, void *buf, size_t len)
{
	struct beos_device_cookie *cookie = _cookie;
	int err;

	err = cookie->node->beos_hooks->control(cookie->beos_cookie, op, buf, len);
	if(err < 0)
		return beos2newos_err(err);
	return NO_ERROR;
}

static ssize_t translation_read(void * _cookie, void *buf, off_t pos, ssize_t _len)
{
	struct beos_device_cookie *cookie = _cookie;
	int err;
	bool update_cookie = false;
	size_t len;

	if(pos < 0) {
		update_cookie = true;
		pos = cookie->pos;
	}

	if(_len < 0)
		len = 0;
	else
		len = _len;

	err = cookie->node->beos_hooks->read(cookie->beos_cookie, pos, buf, &len);
	if(err < 0)
		return beos2newos_err(err);

	if(update_cookie) {
		cookie->pos += len;
	}
	return len;
}

static ssize_t translation_write(void * _cookie, const void *buf, off_t pos, ssize_t _len)
{
	struct beos_device_cookie *cookie = _cookie;
	int err;
	bool update_cookie = false;
	size_t len;

	if(pos < 0) {
		update_cookie = true;
		pos = cookie->pos;
	}

	if(_len < 0)
		len = 0;
	else
		len = _len;

	err = cookie->node->beos_hooks->write(cookie->beos_cookie, pos, buf, &len);
	if(err < 0)
		return beos2newos_err(err);

	if(update_cookie) {
		cookie->pos += len;
	}
	return len;
}

static int translation_canpage(void * ident)
{
	return 0;
}

static ssize_t translation_readpage(void * ident, iovecs *vecs, off_t pos)
{
	return ERR_NOT_ALLOWED;
}

static ssize_t translation_writepage(void * ident, iovecs *vecs, off_t pos)
{
	return ERR_NOT_ALLOWED;
}
*/
/*
image_id beos_load_beos_driver(const char *name)
{
	image_id id;
	char path[SYS_MAX_PATH_LEN];
	char **names;
	int i;

	int (*init_hardware)(void);
	int (*init_driver)(void);
	char **(*publish_devices)(void);
	beos_device_hooks *(*find_device)(const char *name);
	int *api_version;

	sprintf(path, "/boot/addons/beosdev/%s", name);

	id = elf_load_kspace(path, "_beos_");
	if(id < 0)
		return id;

	api_version = (int *)elf_lookup_symbol(id, "api_version");
	if(!api_version || *api_version != B_CUR_DRIVER_API_VERSION)
		return ERR_INVALID_BINARY;

//	dprintf("calling init_hardware\n");

	init_hardware = (void *)elf_lookup_symbol(id, "init_hardware");
	if(!init_hardware)
		return ERR_INVALID_BINARY;
	init_hardware();

//	dprintf("done calling init_hardware\n");

//	dprintf("calling init_driver\n");

	init_driver = (void *)elf_lookup_symbol(id, "init_driver");
	if(!init_driver)
		return ERR_INVALID_BINARY;
	init_driver();

//	dprintf("done calling init_driver\n");

//	dprintf("calling publish_devices\n");

	publish_devices = (void *)elf_lookup_symbol(id, "publish_devices");
	if(!publish_devices)
		return ERR_INVALID_BINARY;
	names = publish_devices();

//	dprintf("done calling publish_devices\n");

	find_device = (void *)elf_lookup_symbol(id, "find_device");
	if(!publish_devices)
		return ERR_INVALID_BINARY;

	for(i=0; names[i]; i++) {
		struct beos_device_node *node;

//		dprintf("publishing name '%s'\n", names[i]);

		node = kmalloc(sizeof(struct beos_device_node));

		node->name = names[i];
		node->beos_hooks = find_device(names[i]);

		devfs_publish_device(names[i], node, &translation_hooks);

		node->next = nodes;
		nodes = node;
	}

	return id;
}
*/
#else

int beos_layer_init(void)
{
	return 0;
}

image_id beos_load_beos_driver(const char *name)
{
	return ERR_GENERAL;
}

#endif

