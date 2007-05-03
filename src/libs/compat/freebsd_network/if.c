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
	ifp->if_dname = name;
	ifp->if_dunit = unit;

	/* XXX use a more Haiku friendly name here? */
	snprintf(ifp->if_xname, IFNAMSIZ, "%s%d", name, unit);
}


void
if_attach(struct ifnet *ifp)
{
	TAILQ_INIT(&ifp->if_addrhead);
	TAILQ_INIT(&ifp->if_prefixhead);
	TAILQ_INIT(&ifp->if_multiaddrs);

	mtx_init(&ifp->if_snd.ifq_mtx, ifp->if_xname, NULL, MTX_DEF);
}


void
if_detach(struct ifnet *ifp)
{
	mtx_destroy(&ifp->if_snd.ifq_mtx);
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
ether_output(struct ifnet *ifp, struct mbuf *m, struct sockaddr *dst,
	struct rtentry *rt0)
{
	int error;
	IFQ_HANDOFF(ifp, m, error);
	return error;
}


static void
ether_input(struct ifnet *ifp, struct mbuf *m)
{
	device_t dev = NULL; /* XXX */

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

	ifp->if_init(ifp->if_softc);
}


void
ether_ifdetach(struct ifnet *ifp)
{
	if_detach(ifp);
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

