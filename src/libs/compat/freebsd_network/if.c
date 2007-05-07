/*
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Hugo Santos, hugosantos@gmail.com
 *
 * Some of this code is based on previous work by Marcus Overhagen.
 */

#include "device.h"

#include <stdio.h>
#include <sys/sockio.h>

#include <compat/sys/bus.h>
#include <compat/sys/kernel.h>

#include <compat/net/if.h>
#include <compat/net/if_var.h>
#include <compat/sys/malloc.h>

#include <compat/net/ethernet.h>

struct ifnet *
if_alloc(u_char type)
{
	struct ifnet *ifp = _kernel_malloc(sizeof(struct ifnet), M_ZERO);
	if (ifp == NULL)
		return NULL;

	ifp->if_type = type;

	IF_ADDR_LOCK_INIT(ifp);
	return ifp;
}


void
if_free(struct ifnet *ifp)
{
	IF_ADDR_LOCK_DESTROY(ifp);
	_kernel_free(ifp);
}


void
if_initname(struct ifnet *ifp, const char *name, int unit)
{
	int i;

	dprintf("if_initname(%p, %s, %d)\n", ifp, name, unit);

	if (name == NULL)
		panic("interface goes unamed");

	ifp->if_dname = name;
	ifp->if_dunit = unit;

	strlcpy(ifp->if_xname, name, sizeof(ifp->if_xname));

	for (i = 0; gDevNameList[i] != NULL; i++) {
		if (strcmp(gDevNameList[i], name) == 0)
			break;
	}

	if (gDevNameList[i] == NULL)
		panic("unknown interface");

	ifp->if_dev = gDevices[i];
	ifp->if_dev->ifp = ifp;
}


void
ifq_init(struct ifqueue *ifq, const char *name)
{
	ifq->ifq_head = NULL;
	ifq->ifq_tail = NULL;
	ifq->ifq_len = 0;
	ifq->ifq_maxlen = IFQ_MAXLEN;
	ifq->ifq_drops = 0;

	mtx_init(&ifq->ifq_mtx, name, NULL, MTX_DEF);
}


void
ifq_uninit(struct ifqueue *ifq)
{
	mtx_destroy(&ifq->ifq_mtx);
}


void
if_attach(struct ifnet *ifp)
{
	TAILQ_INIT(&ifp->if_addrhead);
	TAILQ_INIT(&ifp->if_prefixhead);
	TAILQ_INIT(&ifp->if_multiaddrs);

	ifq_init((struct ifqueue *)&ifp->if_snd, ifp->if_xname);
}


void
if_detach(struct ifnet *ifp)
{
	ifq_uninit((struct ifqueue *)&ifp->if_snd);
}


void
if_start(struct ifnet *ifp)
{
#ifdef IFF_NEEDSGIANT
	if (ifp->if_flags & IFF_NEEDSGIANT)
		panic("freebsd compat.: unsupported giant requirement");
#endif
	ifp->if_start(ifp);
}


int
if_printf(struct ifnet *ifp, const char *format, ...)
{
	char buf[256];
	va_list vl;
	va_start(vl, format);
	vsnprintf(buf, sizeof(buf), format, vl);
	va_end(vl);

	dprintf("[%s] %s", ifp->if_xname, buf);
	return 0;
}


void
if_link_state_change(struct ifnet *ifp, int link_state)
{
	if (ifp->if_link_state == link_state)
		return;

	ifp->if_link_state = link_state;

	/* XXX */
	/* wake network stack's link state sem */
}


int
ether_output(struct ifnet *ifp, struct mbuf *m, struct sockaddr *dst,
	struct rtentry *rt0)
{
	int error = 0;
	IFQ_HANDOFF(ifp, m, error);
	return error;
}


static void
ether_input(struct ifnet *ifp, struct mbuf *m)
{
	device_t dev = ifp->if_dev;

	IF_ENQUEUE(&dev->receive_queue, m);
	release_sem_etc(dev->receive_sem, 1, B_DO_NOT_RESCHEDULE);
}


void
ether_ifattach(struct ifnet *ifp, const uint8_t *mac_address)
{
	ifp->if_addrlen = ETHER_ADDR_LEN;
	ifp->if_hdrlen = ETHER_HDR_LEN;
	if_attach(ifp);
	ifp->if_mtu = ETHERMTU;
	ifp->if_output = ether_output;
	ifp->if_input = ether_input;
	ifp->if_resolvemulti = NULL; /* done in the stack */

	ifp->if_lladdr.sdl_family = AF_LINK;
	memcpy(IF_LLADDR(ifp), mac_address, ETHER_ADDR_LEN);

	ifp->if_init(ifp->if_softc);
}


void
ether_ifdetach(struct ifnet *ifp)
{
	if_detach(ifp);
}


#if 0
/*
 * This is for reference.  We have a table-driven version
 * of the little-endian crc32 generator, which is faster
 * than the double-loop.
 */
uint32_t
ether_crc32_le(const uint8_t *buf, size_t len)
{
	size_t i;
	uint32_t crc;
	int bit;
	uint8_t data;

	crc = 0xffffffff;	/* initial value */

	for (i = 0; i < len; i++) {
		for (data = *buf++, bit = 0; bit < 8; bit++, data >>= 1)
			carry = (crc ^ data) & 1;
			crc >>= 1;
			if (carry)
				crc = (crc ^ ETHER_CRC_POLY_LE);
	}

	return (crc);
}
#else
uint32_t
ether_crc32_le(const uint8_t *buf, size_t len)
{
	static const uint32_t crctab[] = {
		0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
		0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
		0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
		0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
	};
	size_t i;
	uint32_t crc;

	crc = 0xffffffff;	/* initial value */

	for (i = 0; i < len; i++) {
		crc ^= buf[i];
		crc = (crc >> 4) ^ crctab[crc & 0xf];
		crc = (crc >> 4) ^ crctab[crc & 0xf];
	}

	return (crc);
}
#endif

uint32_t
ether_crc32_be(const uint8_t *buf, size_t len)
{
	size_t i;
	uint32_t crc, carry;
	int bit;
	uint8_t data;

	crc = 0xffffffff;	/* initial value */

	for (i = 0; i < len; i++) {
		for (data = *buf++, bit = 0; bit < 8; bit++, data >>= 1) {
			carry = ((crc & 0x80000000) ? 1 : 0) ^ (data & 0x01);
			crc <<= 1;
			if (carry)
				crc = (crc ^ ETHER_CRC_POLY_BE) | carry;
		}
	}

	return (crc);
}


int
ether_ioctl(struct ifnet *ifp, int command, caddr_t data)
{
	struct ifreq *ifr = (struct ifreq *)data;

	switch (command) {
	case SIOCSIFMTU:
		if (ifr->ifr_mtu > ETHERMTU)
			return EINVAL;
		else
			;
			/* need to fix our ifreq to work with C... */
			/* ifp->ifr_mtu = ifr->ifr_mtu; */
		break;

	default:
		return EINVAL;
	}

	return 0;
}


char *
ether_sprintf(const u_char *ap)
{
	static char etherbuf[18];
	snprintf(etherbuf, sizeof (etherbuf),
		"%02lx:%02lx:%02lx:%02lx:%02lx:%02lx",
		(uint32)ap[0], (uint32)ap[1], (uint32)ap[2], (uint32)ap[3],
		(uint32)ap[4], (uint32)ap[5]);
	return (etherbuf);
}
