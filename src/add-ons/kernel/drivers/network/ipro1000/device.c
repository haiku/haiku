/* Intel PRO/1000 Family Driver
 * Copyright (C) 2004 Marcus Overhagen <marcus@overhagen.de>. All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies, and that both the
 * copyright notice and this permission notice appear in supporting documentation.
 *
 * Marcus Overhagen makes no representations about the suitability of this software
 * for any purpose. It is provided "as is" without express or implied warranty.
 *
 * MARCUS OVERHAGEN DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL MARCUS
 * OVERHAGEN BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */


#include "debug.h"
#include "device.h"
#include "driver.h"
#include "util.h"
#include "if_em.h"
#include "if_compat.h"

#include <KernelExport.h>
#include <driver_settings.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAIKU_TARGET_PLATFORM_HAIKU
#	include <net/if_media.h>
#endif

#undef malloc
#undef free

static int32 gOpenMask = 0;

int  em_attach(device_t);
int  em_detach(device_t);
void em_media_status(struct ifnet *, struct ifmediareq *);


static void
ipro1000_read_settings(ipro1000_device *device)
{
	void *handle;
	const char *param;
	int mtu;

	handle = load_driver_settings("ipro1000");
	if (!handle)
		return;

	param = get_driver_parameter(handle, "mtu", "-1", "-1");
	mtu = atoi(param);
	if (mtu >= 50 && mtu <= 1500)
		device->maxframesize = mtu + ENET_HEADER_SIZE;
	else if (mtu != -1)
		dprintf("ipro1000: unsupported mtu setting '%s' ignored\n", param);

	unload_driver_settings(handle);
}


status_t
ipro1000_open(const char *name, uint32 flags, void** cookie)
{
	ipro1000_device *device;
	char *deviceName;
	int dev_id;
	int mask;

	DEVICE_DEBUGOUT("ipro1000_open()");

	for (dev_id = 0; (deviceName = gDevNameList[dev_id]) != NULL; dev_id++) {
		if (!strcmp(name, deviceName))
			break;
	}
	if (deviceName == NULL) {
		ERROROUT("invalid device name");
		return B_ERROR;
	}

	// allow only one concurrent access
	mask = 1 << dev_id;
	if (atomic_or(&gOpenMask, mask) & mask)
		return B_BUSY;

	*cookie = device = (ipro1000_device *)malloc(sizeof(ipro1000_device));
	if (!device) {
		atomic_and(&gOpenMask, ~(1 << dev_id));
		return B_NO_MEMORY;
	}

	memset(device, 0, sizeof(*device));

	device->devId = dev_id;
	device->pciInfo = gDevList[dev_id];
	device->nonblocking = (flags & O_NONBLOCK) ? true : false;
	device->closed = false;

	device->pciBus 	= device->pciInfo->bus;
	device->pciDev	= device->pciInfo->device;
	device->pciFunc	= device->pciInfo->function;
	device->adapter = 0;
	device->maxframesize = 1514; // XXX is MAXIMUM_ETHERNET_FRAME_SIZE = 1518 too much?

#ifdef HAIKU_TARGET_PLATFORM_HAIKU
	device->linkChangeSem = -1;
#endif

	ipro1000_read_settings(device);

	if (em_attach(device) != 0) {
		DEVICE_DEBUGOUT("em_attach failed");
		goto err;
	}

	return B_OK;

err:
	free(device);
	atomic_and(&gOpenMask, ~(1 << dev_id));
	return B_ERROR;
}


status_t
ipro1000_close(void* cookie)
{
	ipro1000_device *device = (ipro1000_device *)cookie;
	struct ifnet *ifp = &device->adapter->interface_data.ac_if;
	DEVICE_DEBUGOUT("ipro1000_close()");

	device->closed = true;
	release_sem(ifp->if_rcv_sem);

	return B_OK;
}


status_t
ipro1000_free(void* cookie)
{
	ipro1000_device *device = (ipro1000_device *)cookie;
	DEVICE_DEBUGOUT("ipro1000_free()");

	if (em_detach(device) != 0) {
		DEVICE_DEBUGOUT("em_detach failed");
	}

	free(device);
	atomic_and(&gOpenMask, ~(1 << device->devId));
	return B_OK;
}


status_t
ipro1000_read(void* cookie, off_t position, void *buf, size_t* num_bytes)
{
	ipro1000_device *device = (ipro1000_device *)cookie;
	struct ifnet *ifp = &device->adapter->interface_data.ac_if;
	struct mbuf *mb;
	status_t stat;
	int len;

//	DEVICE_DEBUGOUT("ipro1000_read() enter");

	if (device->closed) {
		DEVICE_DEBUGOUT("ipro1000_read() interrupted 1");
		return B_INTERRUPTED;
	}
retry:
	stat = acquire_sem_etc(ifp->if_rcv_sem, 1, B_CAN_INTERRUPT | (device->nonblocking ? B_TIMEOUT : 0), 0);
	if (device->closed) {
		// DEVICE_DEBUGOUT("ipro1000_read() interrupted 2"); // net_server will crash if we print this (race condition in net_server?)
		return B_INTERRUPTED;
	}
	if (stat == B_WOULD_BLOCK) {
		DEVICE_DEBUGOUT("ipro1000_read() would block (OK 0 bytes)");
		*num_bytes = 0;
		return B_OK;
	}
	if (stat != B_OK) {
		DEVICE_DEBUGOUT("ipro1000_read() error");
		return B_ERROR;
	}

	IF_DEQUEUE(&ifp->if_rcv, mb);
	if (!mb) {
		ERROROUT("ipro1000_read() mbuf not ready");
		goto retry;
	}

	len = mb->m_len;
	if (len < 0)
		len = 0;
	if (len > (int)*num_bytes)
		len = *num_bytes;

	memcpy(buf, mtod(mb, uint8 *), len); // XXX this is broken for jumbo frames
	*num_bytes = len;

	m_freem(mb);

//	DEVICE_DEBUGOUT1("ipro1000_read() leave, %d bytes", len);
	return B_OK;
}


status_t
ipro1000_write(void* cookie, off_t position, const void* buffer, size_t* num_bytes)
{
//	bigtime_t t = system_time();
	ipro1000_device *device = (ipro1000_device *)cookie;
	struct ifnet *ifp = &device->adapter->interface_data.ac_if;
	struct mbuf *mb;

//	DEVICE_DEBUGOUT("ipro1000_write() enter");

	// allocate mbuf
	for (;;) {
		MGETHDR(mb, M_DONTWAIT, MT_DATA);
		if (mb)
			break;
		snooze(700);
		if (device->closed)
			return B_INTERRUPTED;
	}

//	DEVICE_DEBUGOUT("ipro1000_write() 1");

	// allocate memory
	for (;;) {
		MCLGET(mb, M_DONTWAIT);
		if (mb->m_flags & M_EXT)
			break;
		snooze(700);
		if (device->closed) {
			m_freem(mb);
			return B_INTERRUPTED;
		}
	}

//	DEVICE_DEBUGOUT("ipro1000_write() 2");

	// copy data
	mb->m_len = *num_bytes;
	if (mb->m_len > MCLBYTES)
		mb->m_len = MCLBYTES;
	memcpy(mtod(mb, uint8 *), buffer, mb->m_len);

//	DEVICE_DEBUGOUT("ipro1000_write() 3");

	// add mbuf to send queue
	IF_APPEND(&ifp->if_snd, mb);

//	DEVICE_DEBUGOUT("ipro1000_write() 4");

	// wait for output available
	while (ifp->if_flags & IFF_OACTIVE) {
		snooze(700);
		if (device->closed)
			return B_INTERRUPTED;
	}

//	DEVICE_DEBUGOUT("ipro1000_write() 5");

	// send everything (if still required)
	if (ifp->if_snd.ifq_head != NULL)
		ifp->if_start(ifp);

//	while (ifp->if_snd.ifq_head != NULL) {
//		ifp->if_start(ifp);
//		if (ifp->if_snd.ifq_head != NULL) {
//			snooze(1000);
//			if (device->closed)
//				return B_INTERRUPTED;
//		}
//	}

//	t = system_time() - t;

//	if (t > 20)
//		DEVICE_DEBUGOUT("write %Ld", t);

//	DEVICE_DEBUGOUT("ipro1000_write() finished");

	return B_OK;
}


static struct ifnet *
device_ifp(ipro1000_device *device)
{
	return &device->adapter->interface_data.ac_if;
}


status_t
ipro1000_control(void *cookie, uint32 op, void *arg, size_t len)
{
	ipro1000_device *device = (ipro1000_device *)cookie;

	switch (op) {
		case ETHER_INIT:
			DEVICE_DEBUGOUT("ipro1000_control() ETHER_INIT");
			return B_OK;

		case ETHER_GETADDR:
			DEVICE_DEBUGOUT("ipro1000_control() ETHER_GETADDR");
			memcpy(arg, &device->macaddr, sizeof(device->macaddr));
			return B_OK;

		case ETHER_NONBLOCK:
			if (*(int32 *)arg) {
				DEVICE_DEBUGOUT("non blocking mode on");
				device->nonblocking = true;
			} else {
				DEVICE_DEBUGOUT("non blocking mode off");
				device->nonblocking = false;
			}
			return B_OK;

		case ETHER_ADDMULTI:
		case ETHER_REMMULTI:
		{
			struct ifnet *ifp = device_ifp(device);
			struct sockaddr_dl address;

			if (len != ETHER_ADDR_LEN)
				return EINVAL;

			memset(&address, 0, sizeof(address));
			address.sdl_family = AF_LINK;
			memcpy(LLADDR(&address), arg, ETHER_ADDR_LEN);

			if (op == ETHER_ADDMULTI)
				return ether_add_multi(ifp, (struct sockaddr *)&address);
			else
				return ether_rem_multi(ifp, (struct sockaddr *)&address);
		}

		case ETHER_SETPROMISC:
			if (*(int32 *)arg) {
				DEVICE_DEBUGOUT("promiscuous mode on not supported");
			} else {
				DEVICE_DEBUGOUT("promiscuous mode off");
			}
			return B_OK;

		case ETHER_GETFRAMESIZE:
			DEVICE_DEBUGOUT2("ipro1000_control() ETHER_GETFRAMESIZE, framesize = %d (MTU = %d)", device->maxframesize,  device->maxframesize - ENET_HEADER_SIZE);
			*(uint32*)arg = device->maxframesize;
			return B_OK;

#ifdef HAIKU_TARGET_PLATFORM_HAIKU
		case ETHER_GET_LINK_STATE:
		{
			struct ifmediareq mediareq;
			ether_link_state_t state;

			if (len < sizeof(ether_link_state_t))
				return ENOBUFS;

			em_media_status(device_ifp(device), &mediareq);

			state.media = mediareq.ifm_active;
			if (mediareq.ifm_status & IFM_ACTIVE)
				state.media |= IFM_ACTIVE;
			if (mediareq.ifm_active & IFM_10_T)
				state.speed = 10000000;
			else if (mediareq.ifm_active & IFM_100_TX)
				state.speed = 100000000;
			else
				state.speed = 1000000000;
			state.quality = 1000;

			return user_memcpy(arg, &state, sizeof(ether_link_state_t));
		}

		case ETHER_SET_LINK_STATE_SEM:
		{
			if (user_memcpy(&device->linkChangeSem, arg, sizeof(sem_id)) < B_OK) {
				device->linkChangeSem = -1;
				return B_BAD_ADDRESS;
			}
			return B_OK;
		}
#endif

		default:
			DEVICE_DEBUGOUT("ipro1000_control() Invalid command");
			break;
	}

	return B_BAD_VALUE;
}
