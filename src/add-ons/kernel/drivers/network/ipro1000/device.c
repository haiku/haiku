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
#include <KernelExport.h>
#include <Errors.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <driver_settings.h>

//#define DEBUG

#include "debug.h"
#include "device.h"
#include "driver.h"
#include "util.h"
#include "if_em.h"
#include "if_compat.h"

#undef malloc
#undef free

static int32 gOpenMask = 0;

int  em_attach(device_t);
int  em_detach(device_t);


static void
ipro1000_read_settings(ipro1000_device *device)
{
	void *handle;
	const char *param;
	int mtu;
	
	handle = load_driver_settings("ipro1000");
	if (!handle)
		return;
	
	param = get_driver_parameter(handle, "mtu", "0", "0");
	mtu = atoi(param);
	if (mtu >= 50 && mtu <= 1500)
		device->maxframesize = mtu + ENET_HEADER_SIZE;
	else
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
	
	TRACE("ipro1000_open()\n");

	for (dev_id = 0; (deviceName = gDevNameList[dev_id]) != NULL; dev_id++) {
		if (!strcmp(name, deviceName))
			break;
	}		
	if (deviceName == NULL) {
		ERROR("invalid device name");
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
	
	ipro1000_read_settings(device);

	if (em_attach(device) != 0) {
		TRACE("em_attach failed\n");
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
	TRACE("ipro1000_close()\n");
	
	device->closed = true;
	release_sem(ifp->if_rcv_sem);

	return B_OK;
}


status_t
ipro1000_free(void* cookie)
{
	ipro1000_device *device = (ipro1000_device *)cookie;
	TRACE("ipro1000_free()\n");

	if (em_detach(device) != 0) {
		TRACE("em_detach failed\n");
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
	
//	TRACE("ipro1000_read() enter\n");
	
	if (device->closed) {
		TRACE("ipro1000_read() interrupted 1\n");
		return B_INTERRUPTED;
	}
retry:
	stat = acquire_sem_etc(ifp->if_rcv_sem, 1, B_CAN_INTERRUPT | (device->nonblocking ? B_TIMEOUT : 0), 0);
	if (device->closed) {
		// TRACE("ipro1000_read() interrupted 2\n"); // net_server will crash if we print this (race condition in net_server?)
		return B_INTERRUPTED;
	}
	if (stat == B_WOULD_BLOCK) {
		TRACE("ipro1000_read() would block (OK 0 bytes)\n");
		*num_bytes = 0;
		return B_OK;
	}
	if (stat != B_OK) {
		TRACE("ipro1000_read() error\n");
		return B_ERROR;
	}
	
	IF_DEQUEUE(&ifp->if_rcv, mb);
	if (!mb) {
		ERROR("ipro1000_read() mbuf not ready\n");
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
	
//	TRACE("ipro1000_read() leave, %d bytes\n", len);
	return B_OK;
}


status_t
ipro1000_write(void* cookie, off_t position, const void* buffer, size_t* num_bytes)
{
//	bigtime_t t = system_time();
	ipro1000_device *device = (ipro1000_device *)cookie;
	struct ifnet *ifp = &device->adapter->interface_data.ac_if;
	struct mbuf *mb;
	
//	TRACE("ipro1000_write() enter\n");

	// allocate mbuf	
	for (;;) {
		MGETHDR(mb, M_DONTWAIT, MT_DATA);
		if (mb)
			break;
		snooze(700);
		if (device->closed)
			return B_INTERRUPTED;
	}

//	TRACE("ipro1000_write() 1\n");

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

//	TRACE("ipro1000_write() 2\n");

	// copy data
	mb->m_len = *num_bytes;
	if (mb->m_len > MCLBYTES)
		mb->m_len = MCLBYTES;
	memcpy(mtod(mb, uint8 *), buffer, mb->m_len);

//	TRACE("ipro1000_write() 3\n");

	// add mbuf to send queue
	IF_APPEND(&ifp->if_snd, mb);

//	TRACE("ipro1000_write() 4\n");

	// wait for output available
	while (ifp->if_flags & IFF_OACTIVE) {
		snooze(700);
		if (device->closed)
			return B_INTERRUPTED;
	}

//	TRACE("ipro1000_write() 5\n");
	
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
//		TRACE("write %Ld\n", t);

//	TRACE("ipro1000_write() finished\n");
	
	return B_OK;
}


status_t
ipro1000_control(void *cookie, uint32 op, void *arg, size_t len)
{
	ipro1000_device *device = (ipro1000_device *)cookie;

	switch (op) {
		case ETHER_INIT:
			TRACE("ipro1000_control() ETHER_INIT\n");
			return B_OK;
		
		case ETHER_GETADDR:
			TRACE("ipro1000_control() ETHER_GETADDR\n");
			memcpy(arg, &device->macaddr, sizeof(device->macaddr));
			return B_OK;
			
		case ETHER_NONBLOCK:
			if (*(int32 *)arg) {
				TRACE("non blocking mode on\n");
				device->nonblocking = true;
			} else {
				TRACE("non blocking mode off\n");
				device->nonblocking = false;
			}
			return B_OK;

		case ETHER_ADDMULTI:
			TRACE("ipro1000_control() ETHER_ADDMULTI not supported\n");
			break;
		
		case ETHER_REMMULTI:
			TRACE("ipro1000_control() ETHER_REMMULTI not supported\n");
			return B_OK;
		
		case ETHER_SETPROMISC:
			if (*(int32 *)arg) {
				TRACE("promiscuous mode on not supported\n");
			} else {
				TRACE("promiscuous mode off\n");
			}
			return B_OK;

		case ETHER_GETFRAMESIZE:
			TRACE("ipro1000_control() ETHER_GETFRAMESIZE, framesize = %d (MTU = %d)\n", device->maxframesize,  device->maxframesize - ENET_HEADER_SIZE);
			*(uint32*)arg = device->maxframesize;
			return B_OK;
			
		default:
			TRACE("ipro1000_control() Invalid command\n");
			break;
	}
	
	return B_ERROR;
}
