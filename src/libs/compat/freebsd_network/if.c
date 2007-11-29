/*
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Copyright 2007, Axel Dörfler, axeld@pinc-software.de. All Rights Reserved.
 * Copyright 2004, Marcus Overhagen. All Rights Reserved.
 *
 * Distributed under the terms of the MIT License.
 */


#include "device.h"

#include <stdio.h>
#include <net/if_types.h>
#include <sys/sockio.h>

#include <compat/sys/bus.h>
#include <compat/sys/kernel.h>

#include <compat/net/if.h>
#include <compat/net/if_arp.h>
#include <compat/net/if_var.h>
#include <compat/sys/malloc.h>

#include <compat/net/ethernet.h>


struct ifnet *
if_alloc(u_char type)
{
	char semName[64];

	struct ifnet *ifp = _kernel_malloc(sizeof(struct ifnet), M_ZERO);
	if (ifp == NULL)
		return NULL;

	snprintf(semName, sizeof(semName), "%s receive", gDriverName);

	ifp->receive_sem = create_sem(0, semName);
	if (ifp->receive_sem < B_OK)
		goto err1;

	if (type == IFT_ETHER) {
		ifp->if_l2com = _kernel_malloc(sizeof(struct arpcom), M_ZERO);
		if (ifp->if_l2com == NULL)
			goto err2;

		IFP2AC(ifp)->ac_ifp = ifp;
	}

	ifp->link_state_sem = -1;
	ifp->flags = 0;
	ifq_init(&ifp->receive_queue, semName);

	ifp->if_type = type;
	IF_ADDR_LOCK_INIT(ifp);
	return ifp;

err2:
	delete_sem(ifp->receive_sem);
err1:
	_kernel_free(ifp);
	return NULL;
}


void
if_free(struct ifnet *ifp)
{
	IF_ADDR_LOCK_DESTROY(ifp);
	if (ifp->if_type == IFT_ETHER)
		_kernel_free(ifp->if_l2com);

	delete_sem(ifp->receive_sem);
	ifq_uninit(&ifp->receive_queue);

	_kernel_free(ifp);
}


void
if_initname(struct ifnet *ifp, const char *name, int unit)
{
	dprintf("if_initname(%p, %s, %d)\n", ifp, name, unit);

	if (name == NULL)
		panic("interface goes unamed");

	if (gDeviceCount >= MAX_DEVICES)
		panic("unit too large");

	ifp->if_dname = name;
	ifp->if_dunit = unit;
	ifp->if_index = gDeviceCount++;

	strlcpy(ifp->if_xname, name, sizeof(ifp->if_xname));

	snprintf(ifp->device_name, sizeof(ifp->device_name), "net/%s/%i",
		gDriverName, ifp->if_index);

	driver_printf("%s: /dev/%s\n", gDriverName, ifp->device_name);

	gDeviceNameList[ifp->if_index] = ifp->device_name;
	gDevices[ifp->if_index] = ifp;

	ifp->root_device = find_root_device(unit);
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

	IF_ADDR_LOCK_INIT(ifp);

	ifq_init((struct ifqueue *)&ifp->if_snd, ifp->if_xname);
}


void
if_detach(struct ifnet *ifp)
{
	IF_ADDR_LOCK_DESTROY(ifp);
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

	dprintf("[%s] %s", ifp->device_name, buf);
	return 0;
}


void
if_link_state_change(struct ifnet *ifp, int linkState)
{
	if (ifp->if_link_state == linkState)
		return;

	ifp->if_link_state = linkState;
	release_sem_etc(ifp->link_state_sem, 1, B_DO_NOT_RESCHEDULE);
}


static struct ifmultiaddr *
if_findmulti(struct ifnet *ifp, struct sockaddr *_address)
{
	struct sockaddr_dl *address = (struct sockaddr_dl *)_address;
	struct ifmultiaddr *ifma;

	TAILQ_FOREACH (ifma, &ifp->if_multiaddrs, ifma_link) {
		if (memcmp(LLADDR(address),
				LLADDR((struct sockaddr_dl *)ifma->ifma_addr),
				ETHER_ADDR_LEN) == 0)
			return ifma;
	}

	return NULL;
}


static struct ifmultiaddr *
_if_addmulti(struct ifnet *ifp, struct sockaddr *address)
{
	struct ifmultiaddr *addr = if_findmulti(ifp, address);

	if (addr != NULL) {
		addr->ifma_refcount++;
		return addr;
	}

	addr = (struct ifmultiaddr *)malloc(sizeof(struct ifmultiaddr));
	if (addr == NULL)
		return NULL;

	addr->ifma_lladdr = NULL;
	addr->ifma_ifp = ifp;
	addr->ifma_protospec = NULL;

	memcpy(&addr->ifma_addr_storage, address, sizeof(struct sockaddr_dl));
	addr->ifma_addr = (struct sockaddr *)&addr->ifma_addr_storage;

	addr->ifma_refcount = 1;

	TAILQ_INSERT_HEAD(&ifp->if_multiaddrs, addr, ifma_link);

	return addr;
}


int
if_addmulti(struct ifnet *ifp, struct sockaddr *address,
	struct ifmultiaddr **out)
{
	struct ifmultiaddr *result;
	int refcount = 0;

	IF_ADDR_LOCK(ifp);
	result = _if_addmulti(ifp, address);
	if (result)
		refcount = result->ifma_refcount;
	IF_ADDR_UNLOCK(ifp);

	if (result == NULL)
		return ENOBUFS;

	if (refcount == 1)
		ifp->if_ioctl(ifp, SIOCADDMULTI, NULL);

	if (out)
		(*out) = result;

	return 0;
}


static void
if_delete_multiaddr(struct ifnet *ifp, struct ifmultiaddr *ifma)
{
	TAILQ_REMOVE(&ifp->if_multiaddrs, ifma, ifma_link);
	free(ifma);
}


int
if_delmulti(struct ifnet *ifp, struct sockaddr *address)
{
	struct ifmultiaddr *addr;
	int deleted = 0;

	IF_ADDR_LOCK(ifp);
	addr = if_findmulti(ifp, address);
	if (addr != NULL) {
		addr->ifma_refcount--;
		if (addr->ifma_refcount == 0) {
			if_delete_multiaddr(ifp, addr);
			deleted = 1;
		}
	}
	IF_ADDR_UNLOCK(ifp);

	if (deleted)
		ifp->if_ioctl(ifp, SIOCDELMULTI, NULL);

	return 0;
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
	IF_ENQUEUE(&ifp->receive_queue, m);
	release_sem_etc(ifp->receive_sem, 1, B_DO_NOT_RESCHEDULE);
}


void
ether_ifattach(struct ifnet *ifp, const uint8_t *macAddress)
{
	ifp->if_addrlen = ETHER_ADDR_LEN;
	ifp->if_hdrlen = ETHER_HDR_LEN;
	if_attach(ifp);
	ifp->if_mtu = ETHERMTU;
	ifp->if_output = ether_output;
	ifp->if_input = ether_input;
	ifp->if_resolvemulti = NULL; /* done in the stack */

	ifp->if_lladdr.sdl_family = AF_LINK;
	memcpy(IF_LLADDR(ifp), macAddress, ETHER_ADDR_LEN);

	// TODO: according to FreeBSD's if_ethersubr.c, this should be removed
	//		once all drivers are cleaned up.
	if (macAddress != IFP2ENADDR(ifp))
		memcpy(IFP2ENADDR(ifp), macAddress, ETHER_ADDR_LEN);
}


void
ether_ifdetach(struct ifnet *ifp)
{
	if_detach(ifp);
}


int
ether_ioctl(struct ifnet *ifp, u_long command, caddr_t data)
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

