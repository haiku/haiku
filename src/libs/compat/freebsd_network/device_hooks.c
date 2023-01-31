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
	status_t status;

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

	IFF_LOCKGIANT(ifp);

	if (ifp->if_init != NULL)
		ifp->if_init(ifp->if_softc);

	if (!HAIKU_DRIVER_REQUIRES(FBSD_WLAN_FEATURE)) {
		ifp->if_flags &= ~IFF_UP;
		ifp->if_ioctl(ifp, SIOCSIFFLAGS, NULL);

		memset(&ifr, 0, sizeof(ifr));
		ifr.ifr_media = IFM_MAKEWORD(IFM_ETHER, IFM_AUTO, 0, 0);
		status = ifp->if_ioctl(ifp, SIOCSIFMEDIA, (caddr_t)&ifr);
		if (status != B_OK) {
			ifr.ifr_media = IFM_MAKEWORD(IFM_ETHER, IFM_10_T, 0, 0);
			status = ifp->if_ioctl(ifp, SIOCSIFMEDIA, (caddr_t)&ifr);
		}
	}

	ifp->if_flags |= IFF_UP;
	ifp->flags &= ~DEVICE_CLOSED;
	ifp->if_ioctl(ifp, SIOCSIFFLAGS, NULL);

	IFF_UNLOCKGIANT(ifp);

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
	size_t length = *numBytes;

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

	if (mb->m_pkthdr.len > length) {
		if_printf(ifp, "error reading packet: too large! (%d > %" B_PRIuSIZE ")\n",
			mb->m_pkthdr.len, length);
		m_freem(mb);
		*numBytes = 0;
		return E2BIG;
	}

	length = min_c(max_c((size_t)mb->m_pkthdr.len, 0), length);

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
	int length = *numBytes;

	//if_printf(ifp, "compat_write(%lld, %p, [%lu])\n", position,
	//	buffer, *numBytes);

	if (length <= MHLEN) {
		mb = m_gethdr(0, MT_DATA);
		if (mb == NULL)
			return ENOBUFS;
	} else {
		mb = m_get2(length, 0, MT_DATA, M_PKTHDR);
		if (mb == NULL)
			return E2BIG;

		length = min_c(length, mb->m_ext.ext_size);
	}

	// if we waited, check after if the ifp is still valid

	mb->m_pkthdr.len = mb->m_len = length;
	memcpy(mtod(mb, void *), buffer, mb->m_len);
	*numBytes = length;

	IFF_LOCKGIANT(ifp);
	int result = ifp->if_output(ifp, mb, NULL, NULL);
	IFF_UNLOCKGIANT(ifp);

	return result;
}


static status_t
compat_control(void *cookie, uint32 op, void *arg, size_t length)
{
	struct ifnet *ifp = cookie;
	status_t status;

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

			IFF_LOCKGIANT(ifp);
			status = ifp->if_ioctl(ifp, SIOCSIFFLAGS, NULL);
			IFF_UNLOCKGIANT(ifp);
			return status;
		}

		case ETHER_GETFRAMESIZE:
		{
			uint32 frameSize;
			if (length < 4)
				return B_BAD_VALUE;

			const int MTUs[] = {
				ETHERMTU_JUMBO,
				PAGESIZE - (ETHER_HDR_LEN + ETHER_CRC_LEN),
				2290, /* IEEE80211_MTU_MAX */
				0
			};

			// This is (usually) only invoked during initialization to get the
			// maximum frame size. Thus we try a few common possible values,
			// as there is no way to determine what is supported (or required).
			for (int i = 0; MTUs[i] != 0; i++) {
				struct ifreq ifr;
				ifr.ifr_mtu = MTUs[i];
				if (compat_control(cookie, SIOCSIFMTU, &ifr, sizeof(ifr)) == 0)
					break;
			}

			frameSize = ifp->if_mtu + (ETHER_HDR_LEN + ETHER_CRC_LEN);
			return user_memcpy(arg, &frameSize, 4);
		}

		case ETHER_ADDMULTI:
		case ETHER_REMMULTI:
		{
			struct sockaddr_dl address;

			if ((ifp->if_flags & IFF_MULTICAST) == 0)
				return B_NOT_SUPPORTED;
			if (length != ETHER_ADDR_LEN)
				return B_BAD_VALUE;

			memset(&address, 0, sizeof(address));
			address.sdl_family = AF_LINK;
			if (user_memcpy(LLADDR(&address), arg, ETHER_ADDR_LEN) < B_OK)
				return B_BAD_ADDRESS;

			IFF_LOCKGIANT(ifp);
			if (op == ETHER_ADDMULTI)
				status = if_addmulti(ifp, (struct sockaddr *)&address, NULL);
			else
				status = if_delmulti(ifp, (struct sockaddr *)&address);
			IFF_UNLOCKGIANT(ifp);
			return status;
		}

		case ETHER_GET_LINK_STATE:
		{
			struct ifmediareq mediareq;
			ether_link_state_t state;

			if (length < sizeof(ether_link_state_t))
				return EINVAL;

			memset(&mediareq, 0, sizeof(mediareq));
			IFF_LOCKGIANT(ifp);
			status = ifp->if_ioctl(ifp, SIOCGIFMEDIA, (caddr_t)&mediareq);
			IFF_UNLOCKGIANT(ifp);
			if (status < B_OK)
				return status;

			state.media = mediareq.ifm_active;
			if ((mediareq.ifm_status & IFM_ACTIVE) != 0)
				state.media |= IFM_ACTIVE;
			state.speed = ifmedia_baudrate(mediareq.ifm_active);
			state.quality = 1000;

			return user_memcpy(arg, &state, sizeof(ether_link_state_t));
		}

		case ETHER_SET_LINK_STATE_SEM:
			if (user_memcpy(&ifp->link_state_sem, arg, sizeof(sem_id)) < B_OK) {
				ifp->link_state_sem = -1;
				return B_BAD_ADDRESS;
			}
			return B_OK;

		case SIOCSIFFLAGS:
		case SIOCSIFMEDIA:
		case SIOCSIFMTU:
		{
			IFF_LOCKGIANT(ifp);
			status = ifp->if_ioctl(ifp, op, (caddr_t)arg);
			IFF_UNLOCKGIANT(ifp);
			return status;
		}
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
