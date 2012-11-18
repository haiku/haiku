/*
 * Copyright 2007-2009, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Copyright 2004, Marcus Overhagen. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include "device.h"

#include <stdlib.h>
#include <sys/sockio.h>

#include <Drivers.h>
#include <ether_driver.h>

#include <compat/sys/haiku-module.h>

#include <compat/sys/bus.h>
#include <compat/sys/mbuf.h>
#include <compat/net/ethernet.h>
#include <compat/net/if_media.h>


static status_t
compat_open(const char *name, uint32 flags, void **cookie)
{
	struct ifnet *ifp;
	struct ifreq ifr;
	int i;

	for (i = 0; i < MAX_DEVICES; i++) {
		if (gDevices[i] != NULL && !strcmp(gDevices[i]->device_name, name))
			break;
	}

	if (i == MAX_DEVICES)
		return B_ERROR;

	if (get_module(NET_STACK_MODULE_NAME, (module_info **)&gStack) != B_OK)
		return B_ERROR;

	ifp = gDevices[i];
	if_printf(ifp, "compat_open(0x%" B_PRIx32 ")\n", flags);

	if (atomic_or(&ifp->open_count, 1)) {
		put_module(NET_STACK_MODULE_NAME);
		return B_BUSY;
	}

	ifp->if_init(ifp->if_softc);

	if (!HAIKU_DRIVER_REQUIRES(FBSD_WLAN)) {
		ifp->if_flags &= ~IFF_UP;
		ifp->if_ioctl(ifp, SIOCSIFFLAGS, NULL);
	}

	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_media = IFM_MAKEWORD(IFM_ETHER, IFM_AUTO, 0, 0);
	ifp->if_ioctl(ifp, SIOCSIFMEDIA, (caddr_t)&ifr);

	ifp->if_flags |= IFF_UP;
	ifp->if_ioctl(ifp, SIOCSIFFLAGS, NULL);

	*cookie = ifp;
	return B_OK;
}


static status_t
compat_close(void *cookie)
{
	struct ifnet *ifp = cookie;

	if_printf(ifp, "compat_close()\n");

	atomic_or(&ifp->flags, DEVICE_CLOSED);

	wlan_close(cookie);

	release_sem_etc(ifp->receive_sem, 1, B_RELEASE_ALL);

	return B_OK;
}


static status_t
compat_free(void *cookie)
{
	struct ifnet *ifp = cookie;

	if_printf(ifp, "compat_free()\n");

	// TODO: empty out the send queue

	atomic_and(&ifp->open_count, 0);
	put_module(NET_STACK_MODULE_NAME);
	return B_OK;
}


static status_t
compat_read(void *cookie, off_t position, void *buffer, size_t *numBytes)
{
	struct ifnet *ifp = cookie;
	uint32 semFlags = B_CAN_INTERRUPT;
	status_t status;
	struct mbuf *mb;
	size_t length;

	//if_printf(ifp, "compat_read(%lld, %p, [%lu])\n", position,
	//	buffer, *numBytes);

	if (ifp->flags & DEVICE_CLOSED)
		return B_INTERRUPTED;

	if (ifp->flags & DEVICE_NON_BLOCK)
		semFlags |= B_RELATIVE_TIMEOUT;

	do {
		status = acquire_sem_etc(ifp->receive_sem, 1, semFlags, 0);
		if (ifp->flags & DEVICE_CLOSED)
			return B_INTERRUPTED;

		if (status == B_WOULD_BLOCK) {
			*numBytes = 0;
			return B_OK;
		} else if (status < B_OK)
			return status;

		IF_DEQUEUE(&ifp->receive_queue, mb);
	} while (mb == NULL);

	length = min_c(max_c((size_t)mb->m_pkthdr.len, 0), *numBytes);

#if 0
	mb = m_defrag(mb, 0);
	if (mb == NULL) {
		*numBytes = 0;
		return B_NO_MEMORY;
	}
#endif

	m_copydata(mb, 0, length, buffer);
	*numBytes = length;

	m_freem(mb);
	return B_OK;
}


static status_t
compat_write(void *cookie, off_t position, const void *buffer,
	size_t *numBytes)
{
	struct ifnet *ifp = cookie;
	struct mbuf *mb;

	//if_printf(ifp, "compat_write(%lld, %p, [%lu])\n", position,
	//	buffer, *numBytes);

	if (*numBytes > MHLEN)
		mb = m_getcl(0, MT_DATA, M_PKTHDR);
	else
		mb = m_gethdr(0, MT_DATA);

	if (mb == NULL)
		return ENOBUFS;

	// if we waited, check after if the ifp is still valid

	mb->m_pkthdr.len = mb->m_len = min_c(*numBytes, (size_t)MCLBYTES);
	memcpy(mtod(mb, void *), buffer, mb->m_len);

	return ifp->if_output(ifp, mb, NULL, NULL);
}


static status_t
compat_control(void *cookie, uint32 op, void *arg, size_t length)
{
	struct ifnet *ifp = cookie;

	//if_printf(ifp, "compat_control(op %lu, %p, [%lu])\n", op,
	//	arg, length);

	switch (op) {
		case ETHER_INIT:
			return B_OK;

		case ETHER_GETADDR:
			return user_memcpy(arg, IF_LLADDR(ifp), ETHER_ADDR_LEN);

		case ETHER_NONBLOCK:
		{
			int32 value;
			if (length < 4)
				return B_BAD_VALUE;
			if (user_memcpy(&value, arg, sizeof(int32)) < B_OK)
				return B_BAD_ADDRESS;
			if (value)
				ifp->flags |= DEVICE_NON_BLOCK;
			else
				ifp->flags &= ~DEVICE_NON_BLOCK;
			return B_OK;
		}

		case ETHER_SETPROMISC:
		{
			int32 value;
			if (length < 4)
				return B_BAD_VALUE;
			if (user_memcpy(&value, arg, sizeof(int32)) < B_OK)
				return B_BAD_ADDRESS;
			if (value)
				ifp->if_flags |= IFF_PROMISC;
			else
				ifp->if_flags &= ~IFF_PROMISC;
			return ifp->if_ioctl(ifp, SIOCSIFFLAGS, NULL);
		}

		case ETHER_GETFRAMESIZE:
		{
			uint32 frameSize;
			if (length < 4)
				return B_BAD_VALUE;

			frameSize = ifp->if_mtu + ETHER_HDR_LEN;
			return user_memcpy(arg, &frameSize, 4);
		}

		case ETHER_ADDMULTI:
		case ETHER_REMMULTI:
		{
			struct sockaddr_dl address;

			if ((ifp->if_flags & IFF_MULTICAST) == 0)
				return B_NOT_SUPPORTED;

			memset(&address, 0, sizeof(address));
			address.sdl_family = AF_LINK;
			memcpy(LLADDR(&address), arg, ETHER_ADDR_LEN);

			if (op == ETHER_ADDMULTI)
				return if_addmulti(ifp, (struct sockaddr *)&address, NULL);

			return if_delmulti(ifp, (struct sockaddr *)&address);
		}

		case ETHER_GET_LINK_STATE:
		{
			struct ifmediareq mediareq;
			ether_link_state_t state;
			status_t status;

			if (length < sizeof(ether_link_state_t))
				return EINVAL;

			memset(&mediareq, 0, sizeof(mediareq));
			status = ifp->if_ioctl(ifp, SIOCGIFMEDIA, (caddr_t)&mediareq);
			if (status < B_OK)
				return status;

			state.media = mediareq.ifm_active;
			if ((mediareq.ifm_status & IFM_ACTIVE) != 0)
				state.media |= IFM_ACTIVE;
			if ((mediareq.ifm_active & IFM_10_T) != 0)
				state.speed = 10000000;
			else if ((mediareq.ifm_active & IFM_100_TX) != 0)
				state.speed = 100000000;
			else
				state.speed = 1000000000;
			state.quality = 1000;

			return user_memcpy(arg, &state, sizeof(ether_link_state_t));
		}

		case ETHER_SET_LINK_STATE_SEM:
			if (user_memcpy(&ifp->link_state_sem, arg, sizeof(sem_id)) < B_OK) {
				ifp->link_state_sem = -1;
				return B_BAD_ADDRESS;
			}
			return B_OK;
	}

	return wlan_control(cookie, op, arg, length);
}


device_hooks gDeviceHooks = {
	compat_open,
	compat_close,
	compat_free,
	compat_control,
	compat_read,
	compat_write,
};
