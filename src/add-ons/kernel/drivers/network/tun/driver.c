/*
 * /dev/config/tun network tunnel driver for BeOS
 * (c) 2003, mmu_man, revol@free.fr
 * licenced under MIT licence.
 */
#include <Drivers.h>
#include <KernelExport.h>
#include <OS.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <fsproto.h>
#include "bone_tun.h"


const char * device_names[] = {TUN_DRIVER_NAME, NULL};
extern device_hooks tun_hooks;

int32 api_version = B_CUR_DRIVER_API_VERSION;

vint32 if_mod_ref_count = 0;
bone_tun_interface_info_t *gIfaceModule = NULL;
bone_util_info_t *gUtil = NULL;


status_t
init_hardware(void)
{
	dprintf("tun:init_hardware()\n");
	return B_OK;
}


status_t
init_driver(void)
{
	dprintf("tun:init_driver()\n");
	return B_OK;
}


void
uninit_driver(void)
{
	dprintf("tun:uninit_driver()\n");
}


const char**
publish_devices()
{
	return device_names;
}


device_hooks*
find_device(const char *name)
{
	(void)name;
	return &tun_hooks;
}


status_t
tun_open(const char *name, uint32 flags, cookie_t **cookie)
{
	status_t err = B_OK;
	(void)name; (void)flags;
	/* XXX: add O_NONBLOCK + FIONBIO */
#if DEBUG > 1
	dprintf("tun:open(%s, 0x%08lx,)\n", name, flags);
#endif
	err = get_module(BONE_UTIL_MOD_NAME, (struct module_info **)&gUtil);
	if (err < B_OK)
		return err;
	err = get_module(TUN_INTERFACE_MODULE, (struct module_info **)&gIfaceModule);
	if (err < B_OK) {
		put_module(BONE_UTIL_MOD_NAME);
		return err;
	}

	/* XXX: FIXME, still not ok (rescan) */
	if (atomic_add(&if_mod_ref_count, 1) < 1) /* force one more open to keep loaded */
		get_module(TUN_INTERFACE_MODULE, (struct module_info **)&gIfaceModule);
	
	*cookie = (void*)malloc(sizeof(cookie_t));
	if (*cookie == NULL) {
		dprintf("tun_open : error allocating cookie\n");
		goto err0;
	}
	memset(*cookie, 0, sizeof(cookie_t));
	(*cookie)->blocking_io = true;
	return B_OK;
	
err1:
	dprintf("tun_open : cleanup : will free cookie\n");
	free(*cookie);
	*cookie = NULL;
	put_module(TUN_INTERFACE_MODULE);
	put_module(BONE_UTIL_MOD_NAME);
err0:
	return B_ERROR;
}


status_t
tun_close(void *cookie)
{
	(void)cookie;
	return B_OK;
}


status_t
tun_free(cookie_t *cookie)
{
	status_t err = B_OK;
#if DEBUG > 1
	dprintf("tun_close()\n");
#endif
	if (cookie->iface)
		err = gIfaceModule->tun_detach_driver(cookie->iface, true);
	free(cookie);
	atomic_add(&if_mod_ref_count, -1);
	put_module(TUN_INTERFACE_MODULE);
	put_module(BONE_UTIL_MOD_NAME);
	return err;
}


status_t
tun_ioctl(cookie_t *cookie, uint32 op, void *data, size_t len)
{
	ifreq_t *ifr;
	bone_tun_if_interface_t *iface;
	(void)cookie; (void)op; (void)data; (void)len;
	iface = cookie->iface;
#if DEBUG > 1
	dprintf("tun_ioctl(%d(0x%08lx), , %d)\n", op, op, len);
#endif

	switch (op) {
	case B_SET_NONBLOCKING_IO:
		cookie->blocking_io = false;
		return B_OK;
	case B_SET_BLOCKING_IO:
		cookie->blocking_io = true;
		return B_OK;
	case TUNSETNOCSUM:
		return B_OK;//EOPNOTSUPP;
	case TUNSETDEBUG:
		return B_OK;//EOPNOTSUPP;
	case TUNSETIFF:
		if (data == NULL)
			return EINVAL;
		ifr = (ifreq_t *)data;

		iface = gIfaceModule->tun_reuse_or_create(ifr, cookie);
		if (iface != NULL) {
			dprintf("tun: new tunnel created: %s, flags: 0x%08lx\n", ifr->ifr_name, iface->flags);
			return B_OK;
		} else
			dprintf("tun: can't allocate a new tunnel!\n");
		break;
		
	case SIOCGIFHWADDR:
		if (data == NULL)
			return EINVAL;
		ifr = (ifreq_t *)data;
		if (iface == NULL)
			return EINVAL;
		if (strncmp(ifr->ifr_name, iface->ifn->if_name, IFNAMSIZ) != 0)
			return EINVAL;
		memcpy(ifr->ifr_hwaddr, iface->fakemac.octet, 6);
		return B_OK;
	case SIOCSIFHWADDR:
		if (data == NULL)
			return EINVAL;
		ifr = (ifreq_t *)data;
		if (iface == NULL)
			return EINVAL;
		if (strncmp(ifr->ifr_name, iface->ifn->if_name, IFNAMSIZ) != 0)
			return EINVAL;
		memcpy(iface->fakemac.octet, ifr->ifr_hwaddr, 6);
		return B_OK;
	
	}
	return B_ERROR;
}


status_t
tun_read(cookie_t *cookie, off_t position, void *data, size_t *numbytes)
{
	bone_data_t *bdata;
	uint32 got;
	ssize_t pktsize;
	if (cookie->iface == NULL)
		return EBUSY;

	//if ((pktsize = gUtil->dequeue_fifo_data(cookie->iface->wfifo, &bdata, B_INFINITE_TIMEOUT, false)) < 0)
	pktsize = gUtil->dequeue_fifo_data(cookie->iface->wfifo, &bdata, cookie->blocking_io?B_INFINITE_TIMEOUT:0LL, false);

	if (pktsize < 0) {
		*numbytes = 0;
		return pktsize;
	}
#if DEBUG > 2
	dprintf("tun: dequeue = %ld, datalen = %ld, \n", pktsize, bdata?bdata->datalen:0);
#endif
	pktsize = (ssize_t)bdata->datalen;

	got = gUtil->copy_from_data(bdata, 0L, data, MIN((bdata->datalen), (*numbytes)));
	//got = MIN((*numbytes), bdata->datalen);
	if ((pktsize > *numbytes) && !(cookie->iface->flags & TUN_NO_PI))
		((struct tun_pi *)data)->flags |= TUN_PKT_STRIP;
	gUtil->delete_data(bdata);
#if DEBUG > 2
	dprintf("tun: read() got %ld bytes, buf is %ld\n", got, *numbytes);
	dump_data(bdata);
	iovec_dump_data(bdata);
#endif
	*numbytes = got;
	return B_OK;
ERROR_EOF:
	*numbytes = 0;
	return B_OK;
}


status_t
tun_write(cookie_t *cookie, off_t position, const void *data, size_t *numbytes)
{
	bone_data_t *bdata = NULL;
	void *buf;
	status_t err = B_NO_MEMORY;
	(void)position;
	
	/* XXX: max for *numbytes ? */
	
	if (cookie->iface == NULL)
		return EBUSY;
	bdata = gUtil->new_data();
	if (bdata == NULL)
		return B_NO_MEMORY;
	buf = gUtil->falloc(*numbytes);
	if (buf == NULL)
		goto ERROR_1;
	memcpy(buf, data, *numbytes);
	
	if (gUtil->prepend_data(bdata, buf, *numbytes, gUtil->free) < 0) {
		gUtil->free(buf);
		goto ERROR_1;
	}
	if ((err = gUtil->enqueue_fifo_data(cookie->iface->rfifo, bdata)) < B_OK)
		goto ERROR_1;
	return B_OK;

ERROR_1:
	if (bdata)
		gUtil->delete_data(bdata);
	return err;
}


status_t
tun_select(cookie_t *cookie, uint8 event, uint32 ref, selectsync *sync)
{
	status_t err = B_OK;
#if DEBUG > 1
	dprintf("tun: select(, %d, %ld, )\n", event, ref);
#endif
	if (cookie->iface == NULL)
		return B_NO_INIT;
	if (event > B_SELECT_EXCEPTION || !event)
		return EINVAL;
	err = gUtil->lock_benaphore(&cookie->iface->lock);
	if (err < B_OK)
		return err;
	/* iface LOCKED */
	if (cookie->iface->sel[event].sync)
		err = EALREADY;
	else {
		bone_fifo_t *fifo = NULL;
		switch (event) {
		case B_SELECT_READ:
			fifo = cookie->iface->wfifo;
			break;
		case B_SELECT_WRITE:
			fifo = cookie->iface->rfifo;
			break;
		}
		if (fifo) {
			/* XXX: is it safe ??? shouldn't we dequeue(peek=true) ? */
			err = gUtil->lock_benaphore(&fifo->lock);
			if (err >= B_OK) {
				bool avail;
				switch (event) {
				case B_SELECT_READ:
					avail = (fifo->current_bytes > 1);
					break;
				case B_SELECT_WRITE:
					avail = ((fifo->max_bytes - fifo->current_bytes) > cookie->iface->ifn->if_mtu);
					break;
				}
				/* check if we don't already have the event */
				if (avail) {
					notify_select_event(sync, ref);
				}
				else {
					cookie->iface->sel[event].sync = sync;
					cookie->iface->sel[event].ref = ref;
				}
				gUtil->unlock_benaphore(&fifo->lock);
			}
		} else {
			cookie->iface->sel[event].sync = sync;
			cookie->iface->sel[event].ref = ref;
		}
	}
	gUtil->unlock_benaphore(&cookie->iface->lock);
	/* iface UNLOCKED */
	return err;
}


status_t
tun_deselect(cookie_t *cookie, uint8 event, selectsync *sync)
{
	status_t err = B_OK;
#if DEBUG > 1
	dprintf("tun: deselect(, %d, )\n", event);
#endif
	if (cookie->iface == NULL)
		return B_NO_INIT;
	if (event > B_SELECT_EXCEPTION || !event)
		return EINVAL;
	err = gUtil->lock_benaphore(&cookie->iface->lock);
	if (err < B_OK)
		return err;
	cookie->iface->sel[event].sync = NULL;
	cookie->iface->sel[event].ref = 0;
	gUtil->unlock_benaphore(&cookie->iface->lock);
	/* iface LOCKED */
	return B_OK;
}


status_t
tun_readv(cookie_t *cookie, off_t position, const iovec *vec, size_t count, size_t *numBytes)
{
	dprintf("tun: readv(, %Ld, , %ld)\n", position, count);
	return EOPNOTSUPP;
}


status_t
tun_writev(cookie_t *cookie, off_t position, const iovec *vec, size_t count, size_t *numBytes)
{
	dprintf("tun: writev(, %Ld, , %ld)\n", position, count);
	return EOPNOTSUPP;
}


device_hooks tun_hooks = {
	(device_open_hook)tun_open,
	tun_close,
	(device_free_hook)tun_free,
	(device_control_hook)tun_ioctl,
	(device_read_hook)tun_read,
	(device_write_hook)tun_write,
	(device_select_hook)tun_select,
	(device_deselect_hook)tun_deselect,
	(device_readv_hook)tun_readv,
	(device_writev_hook)tun_writev
};
