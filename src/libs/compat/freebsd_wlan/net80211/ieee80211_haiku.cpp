/*
 * Copyright 2009, Colin Günther, coling@gmx.de. All rights reserved.
 * Copyright 2018, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


/*-
 * Copyright (c) 2003-2009 Sam Leffler, Errno Consulting
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


/*
 * IEEE 802.11 support (Haiku-specific code)
 */


#include "ieee80211_haiku.h"

extern "C" {
#	include <sys/kernel.h>
#	include <sys/mbuf.h>
#	include <sys/bus.h>
#	include <sys/sockio.h>

#	include <net/if.h>
#	include <net/if_media.h>
#	include <net/if_types.h>
#	include <net/if_var.h>

#	include "ieee80211_var.h"
};

#include <SupportDefs.h>

#include <util/KMessage.h>

#include <ether_driver.h>
#include <net_notifications.h>

#include <shared.h>


#define TRACE_WLAN
#ifdef TRACE_WLAN
#	define TRACE(x...) dprintf(x);
#else
#	define TRACE(x...) ;
#endif


#define	MC_ALIGN(m, len)									\
do {														\
	(m)->m_data += (MCLBYTES - (len)) &~ (sizeof(long) - 1);\
} while (/* CONSTCOND */ 0)


static net_notifications_module_info* sNotificationModule;


static struct ifnet*
get_ifnet(device_t device, int& i)
{
	int unit = device_get_unit(device);

	for (i = 0; i < MAX_DEVICES; i++) {
		if (gDevices[i] != NULL && gDevices[i]->if_dunit == unit)
			return gDevices[i];
	}

	return NULL;
}


status_t
init_wlan_stack(void)
{
	get_module(NET_NOTIFICATIONS_MODULE_NAME,
		(module_info**)&sNotificationModule);

	return B_OK;
}


void
uninit_wlan_stack(void)
{
	if (sNotificationModule != NULL)
		put_module(NET_NOTIFICATIONS_MODULE_NAME);
}


status_t
start_wlan(device_t device)
{
	struct ieee80211com* ic = ieee80211_find_com(device->nameunit);
	if (ic == NULL)
		return B_BAD_VALUE;

	struct ieee80211vap* vap = ic->ic_vap_create(ic, "wlan",
		device_get_unit(device),
		IEEE80211_M_STA,		// mode
		0,						// flags
		NULL,					// BSSID
		ic->ic_macaddr);		// MAC address

	if (vap == NULL)
		return B_ERROR;

	// ic_vap_create() established that gDevices[i] links to vap->iv_ifp now
	KASSERT(gDevices[gDeviceCount - 1] == vap->iv_ifp,
		("start_wlan: gDevices[i] != vap->iv_ifp"));

	vap->iv_ifp->scan_done_sem = create_sem(0, "wlan scan done");

	// We aren't connected to a WLAN, yet.
	if_link_state_change(vap->iv_ifp, LINK_STATE_DOWN);

	dprintf("%s: wlan started.\n", __func__);

	return B_OK;
}


status_t
stop_wlan(device_t device)
{
	int i;
	struct ifnet* ifp = get_ifnet(device, i);
	if (ifp == NULL)
		return B_BAD_VALUE;

	delete_sem(ifp->scan_done_sem);

	struct ieee80211vap* vap = (ieee80211vap*)ifp->if_softc;
	struct ieee80211com* ic = vap->iv_ic;

	ic->ic_vap_delete(vap);

	// ic_vap_delete freed gDevices[i]
	KASSERT(gDevices[i] == NULL, ("stop_wlan: gDevices[i] != NULL"));

	return B_OK;
}


status_t
wlan_open(void* cookie)
{
	dprintf("wlan_open(%p)\n", cookie);
	struct ifnet* ifp = (struct ifnet*)cookie;

	ifp->if_init(ifp->if_softc);

	ifp->if_flags |= IFF_UP;
	ifp->if_ioctl(ifp, SIOCSIFFLAGS, NULL);

	return B_OK;
}


status_t
wlan_close(void* cookie)
{
	dprintf("wlan_close(%p)\n", cookie);
	struct ifnet* ifp = (struct ifnet*)cookie;

	ifp->if_flags &= ~IFF_UP;
	ifp->if_ioctl(ifp, SIOCSIFFLAGS, NULL);

	return release_sem_etc(ifp->scan_done_sem, 1, B_RELEASE_ALL);
}


status_t
wlan_control(void* cookie, uint32 op, void* arg, size_t length)
{
	struct ifnet* ifp = (struct ifnet*)cookie;

	switch (op) {
		case SIOCG80211:
		case SIOCS80211:
		{
			// FreeBSD drivers assume that the request structure has already
			// been copied into kernel space
			struct ieee80211req request;
			if (user_memcpy(&request, arg, sizeof(struct ieee80211req)) != B_OK)
				return B_BAD_ADDRESS;

			if (request.i_type == IEEE80211_IOC_HAIKU_COMPAT_WLAN_UP)
				return wlan_open(cookie);
			else if (request.i_type == IEEE80211_IOC_HAIKU_COMPAT_WLAN_DOWN)
				return wlan_close(cookie);

			TRACE("wlan_control: %" B_PRIu32 ", %d\n", op, request.i_type);
			status_t status = ifp->if_ioctl(ifp, op, (caddr_t)&request);
			if (status != B_OK)
				return status;

			if (op == SIOCG80211 && user_memcpy(arg, &request,
					sizeof(struct ieee80211req)) != B_OK)
				return B_BAD_ADDRESS;
			return B_OK;
		}

		case SIOCSIFFLAGS:
		case SIOCSIFMEDIA:
		case SIOCGIFMEDIA:
		case SIOCSIFMTU:
			// Requests that make it here always come from the kernel
			return ifp->if_ioctl(ifp, op, (caddr_t)arg);
	}

	return B_BAD_VALUE;
}


void
get_random_bytes(void* p, size_t n)
{
	uint8_t* dp = (uint8_t*)p;

	while (n > 0) {
		uint32_t v = arc4random();
		size_t nb = n > sizeof(uint32_t) ? sizeof(uint32_t) : n;
		bcopy(&v, dp, n > sizeof(uint32_t) ? sizeof(uint32_t) : n);
		dp += sizeof(uint32_t), n -= nb;
	}
}


struct mbuf *
ieee80211_getmgtframe(uint8_t **frm, int headroom, int pktlen)
{
	struct mbuf *m;
	u_int len;

	/*
	 * NB: we know the mbuf routines will align the data area
	 *     so we don't need to do anything special.
	 */
	len = roundup2(headroom + pktlen, 4);
	KASSERT(len <= MCLBYTES, ("802.11 mgt frame too large: %u", len));
	if (len < MINCLSIZE) {
		m = m_gethdr(M_NOWAIT, MT_DATA);
		/*
		 * Align the data in case additional headers are added.
		 * This should only happen when a WEP header is added
		 * which only happens for shared key authentication mgt
		 * frames which all fit in MHLEN.
		 */
		if (m != NULL)
			M_ALIGN(m, len);
	} else {
		m = m_getcl(M_NOWAIT, MT_DATA, M_PKTHDR);
		if (m != NULL)
			MC_ALIGN(m, len);
	}
	if (m != NULL) {
		m->m_data += headroom;
		*frm = (uint8_t*)m->m_data;
	}
	return m;
}


/*
 * Decrements the reference-counter and
 * tests whether it became zero. If so, sets it to one.
 *
 * @return 1 reference-counter became zero
 * @return 0 reference-counter didn't became zero
 */
int
ieee80211_node_dectestref(struct ieee80211_node* ni)
{
	atomic_subtract_int(&ni->ni_refcnt, 1);
	return atomic_cmpset_int(&ni->ni_refcnt, 0, 1);
}


void
ieee80211_drain_ifq(struct ifqueue* ifq)
{
	struct ieee80211_node* ni;
	struct mbuf* m;

	for (;;) {
		IF_DEQUEUE(ifq, m);
		if (m == NULL)
			break;

		ni = (struct ieee80211_node*)m->m_pkthdr.rcvif;
		KASSERT(ni != NULL, ("frame w/o node"));
		ieee80211_free_node(ni);
		m->m_pkthdr.rcvif = NULL;

		m_freem(m);
	}
}


void
ieee80211_flush_ifq(struct ifqueue* ifq, struct ieee80211vap* vap)
{
	struct ieee80211_node* ni;
	struct mbuf* m;
	struct mbuf** mprev;

	IF_LOCK(ifq);
	mprev = &ifq->ifq_head;
	while ((m = *mprev) != NULL) {
		ni = (struct ieee80211_node*)m->m_pkthdr.rcvif;
		if (ni != NULL && ni->ni_vap == vap) {
			*mprev = m->m_nextpkt;
				// remove from list
			ifq->ifq_len--;

			m_freem(m);
			ieee80211_free_node(ni);
				// reclaim ref
		} else
			mprev = &m->m_nextpkt;
	}
	// recalculate tail ptr
	m = ifq->ifq_head;
	for (; m != NULL && m->m_nextpkt != NULL; m = m->m_nextpkt);
	ifq->ifq_tail = m;
	IF_UNLOCK(ifq);
}


#ifndef __NO_STRICT_ALIGNMENT
/*
 * Re-align the payload in the mbuf.  This is mainly used (right now)
 * to handle IP header alignment requirements on certain architectures.
 */
extern "C" struct mbuf *
ieee80211_realign(struct ieee80211vap *vap, struct mbuf *m, size_t align)
{
	int pktlen, space;
	struct mbuf *n;

	pktlen = m->m_pkthdr.len;
	space = pktlen + align;
	if (space < MINCLSIZE)
		n = m_gethdr(M_NOWAIT, MT_DATA);
	else {
		n = m_getjcl(M_NOWAIT, MT_DATA, M_PKTHDR,
		    space <= MCLBYTES ?     MCLBYTES :
#if MJUMPAGESIZE != MCLBYTES
		    space <= MJUMPAGESIZE ? MJUMPAGESIZE :
#endif
		    space <= MJUM9BYTES ?   MJUM9BYTES : MJUM16BYTES);
	}
	if (__predict_true(n != NULL)) {
		m_move_pkthdr(n, m);
		n->m_data = (caddr_t)(ALIGN(n->m_data + align) - align);
		m_copydata(m, 0, pktlen, mtod(n, caddr_t));
		n->m_len = pktlen;
	} else {
		IEEE80211_DISCARD(vap, IEEE80211_MSG_ANY,
		    mtod(m, const struct ieee80211_frame *), NULL,
		    "%s", "no mbuf to realign");
		vap->iv_stats.is_rx_badalign++;
	}
	m_freem(m);
	return n;
}
#endif /* !__NO_STRICT_ALIGNMENT */


int
ieee80211_add_callback(struct mbuf* m,
	void (*func)(struct ieee80211_node*, void*, int), void* arg)
{
	struct m_tag* mtag;
	struct ieee80211_cb* cb;

	mtag = m_tag_alloc(MTAG_ABI_NET80211, NET80211_TAG_CALLBACK,
		sizeof(struct ieee80211_cb), M_NOWAIT);
	if (mtag == NULL)
		return 0;

	cb = (struct ieee80211_cb*)(mtag+1);
	cb->func = func;
	cb->arg = arg;
	m_tag_prepend(m, mtag);
	m->m_flags |= M_TXCB;
	return 1;
}


void
ieee80211_process_callback(struct ieee80211_node* ni, struct mbuf* m,
	int status)
{
	struct m_tag* mtag;

	mtag = m_tag_locate(m, MTAG_ABI_NET80211, NET80211_TAG_CALLBACK, NULL);
	if (mtag != NULL) {
		struct ieee80211_cb* cb = (struct ieee80211_cb*)(mtag+1);
		cb->func(ni, cb->arg, status);
	}
}


int
ieee80211_add_xmit_params(struct mbuf *m,
	const struct ieee80211_bpf_params *params)
{
	struct m_tag *mtag;
	struct ieee80211_tx_params *tx;

	mtag = m_tag_alloc(MTAG_ABI_NET80211, NET80211_TAG_XMIT_PARAMS,
		sizeof(struct ieee80211_tx_params), M_NOWAIT);
	if (mtag == NULL)
		return (0);

	tx = (struct ieee80211_tx_params *)(mtag+1);
	memcpy(&tx->params, params, sizeof(struct ieee80211_bpf_params));
	m_tag_prepend(m, mtag);
	return (1);
}


int
ieee80211_get_xmit_params(struct mbuf *m,
	struct ieee80211_bpf_params *params)
{
	struct m_tag *mtag;
	struct ieee80211_tx_params *tx;

	mtag = m_tag_locate(m, MTAG_ABI_NET80211, NET80211_TAG_XMIT_PARAMS,
		NULL);
	if (mtag == NULL)
		return (-1);
	tx = (struct ieee80211_tx_params *)(mtag + 1);
	memcpy(params, &tx->params, sizeof(struct ieee80211_bpf_params));
	return (0);
}


/*
 * Add RX parameters to the given mbuf.
 *
 * Returns 1 if OK, 0 on error.
 */
int
ieee80211_add_rx_params(struct mbuf *m, const struct ieee80211_rx_stats *rxs)
{
	struct m_tag *mtag;
	struct ieee80211_rx_params *rx;

	mtag = m_tag_alloc(MTAG_ABI_NET80211, NET80211_TAG_RECV_PARAMS,
		sizeof(struct ieee80211_rx_stats), M_NOWAIT);
	if (mtag == NULL)
		return (0);

	rx = (struct ieee80211_rx_params *)(mtag + 1);
	memcpy(&rx->params, rxs, sizeof(*rxs));
	m_tag_prepend(m, mtag);
	return (1);
}


int
ieee80211_get_rx_params(struct mbuf *m, struct ieee80211_rx_stats *rxs)
{
	struct m_tag *mtag;
	struct ieee80211_rx_params *rx;

	mtag = m_tag_locate(m, MTAG_ABI_NET80211, NET80211_TAG_RECV_PARAMS,
		NULL);
	if (mtag == NULL)
		return (-1);
	rx = (struct ieee80211_rx_params *)(mtag + 1);
	memcpy(rxs, &rx->params, sizeof(*rxs));
	return (0);
}


const struct ieee80211_rx_stats *
ieee80211_get_rx_params_ptr(struct mbuf *m)
{
	struct m_tag *mtag;
	struct ieee80211_rx_params *rx;

	mtag = m_tag_locate(m, MTAG_ABI_NET80211, NET80211_TAG_RECV_PARAMS,
	    NULL);
	if (mtag == NULL)
		return (NULL);
	rx = (struct ieee80211_rx_params *)(mtag + 1);
	return (&rx->params);
}


/*
 * Add TOA parameters to the given mbuf.
 */
int
ieee80211_add_toa_params(struct mbuf *m, const struct ieee80211_toa_params *p)
{
	struct m_tag *mtag;
	struct ieee80211_toa_params *rp;

	mtag = m_tag_alloc(MTAG_ABI_NET80211, NET80211_TAG_TOA_PARAMS,
	    sizeof(struct ieee80211_toa_params), M_NOWAIT);
	if (mtag == NULL)
		return (0);

	rp = (struct ieee80211_toa_params *)(mtag + 1);
	memcpy(rp, p, sizeof(*rp));
	m_tag_prepend(m, mtag);
	return (1);
}


int
ieee80211_get_toa_params(struct mbuf *m, struct ieee80211_toa_params *p)
{
	struct m_tag *mtag;
	struct ieee80211_toa_params *rp;

	mtag = m_tag_locate(m, MTAG_ABI_NET80211, NET80211_TAG_TOA_PARAMS,
	    NULL);
	if (mtag == NULL)
		return (0);
	rp = (struct ieee80211_toa_params *)(mtag + 1);
	if (p != NULL)
		memcpy(p, rp, sizeof(*p));
	return (1);
}


/*
 * Transmit a frame to the parent interface.
 */
int
ieee80211_parent_xmitpkt(struct ieee80211com *ic, struct mbuf *m)
{
	int error;

	/*
	 * Assert the IC TX lock is held - this enforces the
	 * processing -> queuing order is maintained
	 */
	IEEE80211_TX_LOCK_ASSERT(ic);
	error = ic->ic_transmit(ic, m);
	if (error) {
		struct ieee80211_node *ni;

		ni = (struct ieee80211_node *)m->m_pkthdr.rcvif;

		/* XXX number of fragments */
		if_inc_counter(ni->ni_vap->iv_ifp, IFCOUNTER_OERRORS, 1);
		ieee80211_free_node(ni);
		ieee80211_free_mbuf(m);
	}
	return (error);
}


/*
 * Transmit a frame to the VAP interface.
 */
int
ieee80211_vap_xmitpkt(struct ieee80211vap *vap, struct mbuf *m)
{
	struct ifnet *ifp = vap->iv_ifp;

	/*
	 * When transmitting via the VAP, we shouldn't hold
	 * any IC TX lock as the VAP TX path will acquire it.
	 */
	IEEE80211_TX_UNLOCK_ASSERT(vap->iv_ic);

	return (ifp->if_transmit(ifp, m));

}


void
ieee80211_sysctl_vattach(struct ieee80211vap* vap)
{
	vap->iv_debug = IEEE80211_MSG_XRATE
		| IEEE80211_MSG_NODE
		| IEEE80211_MSG_ASSOC
		| IEEE80211_MSG_AUTH
		| IEEE80211_MSG_STATE
		| IEEE80211_MSG_WME
		| IEEE80211_MSG_DOTH
		| IEEE80211_MSG_INACT
		| IEEE80211_MSG_ROAM;
}


void
ieee80211_sysctl_vdetach(struct ieee80211vap* vap)
{
	dprintf("%s not implemented, yet.\n", __func__);
}


void
ieee80211_vap_destroy(struct ieee80211vap* vap)
{
	struct ieee80211com* ic = vap->iv_ic;

	ic->ic_vap_delete(vap);
	dprintf("%s: done.\n", __func__);
}


void
ieee80211_load_module(const char* modname)
{
#if 0
	dprintf("%s not implemented, yet: modname %s\n", __func__, modname);
#endif
}


void
ieee80211_notify_node_join(struct ieee80211_node* ni, int newassoc)
{
	struct ieee80211vap* vap = ni->ni_vap;
	struct ifnet* ifp = vap->iv_ifp;

	TRACE("%s\n", __FUNCTION__);

	if (ni == vap->iv_bss)
		if_link_state_change(ifp, LINK_STATE_UP);

	if (sNotificationModule != NULL) {
		char messageBuffer[512];
		KMessage message;
		message.SetTo(messageBuffer, sizeof(messageBuffer), B_NETWORK_MONITOR);
		message.AddInt32("opcode", B_NETWORK_WLAN_JOINED);
		message.AddString("interface", ifp->device_name);
		// TODO: add data about the node

		sNotificationModule->send_notification(&message);
	}
}


void
ieee80211_notify_node_leave(struct ieee80211_node* ni)
{
	struct ieee80211vap* vap = ni->ni_vap;
	struct ifnet* ifp = vap->iv_ifp;

	if (ni == vap->iv_bss)
		if_link_state_change(ifp, LINK_STATE_DOWN);

	TRACE("%s\n", __FUNCTION__);

	if (sNotificationModule != NULL) {
		char messageBuffer[512];
		KMessage message;
		message.SetTo(messageBuffer, sizeof(messageBuffer), B_NETWORK_MONITOR);
		message.AddInt32("opcode", B_NETWORK_WLAN_LEFT);
		message.AddString("interface", ifp->device_name);
		// TODO: add data about the node

		sNotificationModule->send_notification(&message);
	}
}


void
ieee80211_notify_scan_done(struct ieee80211vap* vap)
{
	release_sem_etc(vap->iv_ifp->scan_done_sem, 1,
		B_DO_NOT_RESCHEDULE | B_RELEASE_ALL);

	TRACE("%s\n", __FUNCTION__);

	if (sNotificationModule != NULL) {
		char messageBuffer[512];
		KMessage message;
		message.SetTo(messageBuffer, sizeof(messageBuffer), B_NETWORK_MONITOR);
		message.AddInt32("opcode", B_NETWORK_WLAN_SCANNED);
		message.AddString("interface", vap->iv_ifp->device_name);

		sNotificationModule->send_notification(&message);
	}
}


void
ieee80211_notify_replay_failure(struct ieee80211vap* vap,
	const struct ieee80211_frame* wh, const struct ieee80211_key* k,
	u_int64_t rsc, int tid)
{
	dprintf("%s not implemented, yet.\n", __func__);
}


void
ieee80211_notify_michael_failure(struct ieee80211vap* vap,
	const struct ieee80211_frame* wh, u_int keyix)
{
	dprintf("%s not implemented, yet.\n", __func__);
}


void
ieee80211_notify_wds_discover(struct ieee80211_node* ni)
{
	dprintf("%s not implemented, yet.\n", __func__);
}


void
ieee80211_notify_csa(struct ieee80211com* ic,
	const struct ieee80211_channel* c, int mode, int count)
{
	dprintf("%s not implemented, yet.\n", __func__);
}


void
ieee80211_notify_radar(struct ieee80211com* ic,
	const struct ieee80211_channel* c)
{
	dprintf("%s not implemented, yet.\n", __func__);
}


void
ieee80211_notify_cac(struct ieee80211com* ic,
	const struct ieee80211_channel* c, enum ieee80211_notify_cac_event type)
{
	dprintf("%s not implemented, yet.\n", __func__);
}


void
ieee80211_notify_node_deauth(struct ieee80211_node* ni)
{
	dprintf("%s not implemented, yet.\n", __func__);
}


void
ieee80211_notify_node_auth(struct ieee80211_node* ni)
{
	dprintf("%s not implemented, yet.\n", __func__);
}


void
ieee80211_notify_country(struct ieee80211vap* vap,
	const uint8_t bssid[IEEE80211_ADDR_LEN], const uint8_t cc[2])
{
	dprintf("%s not implemented, yet.\n", __func__);
}


void
ieee80211_notify_radio(struct ieee80211com* ic, int state)
{
	dprintf("%s not implemented, yet.\n", __func__);
}


void
ieee80211_sysctl_attach(struct ieee80211com* ic)
{
	dprintf("%s not implemented, yet.\n", __func__);
}


void
ieee80211_sysctl_detach(struct ieee80211com* ic)
{
	dprintf("%s not implemented, yet.\n", __func__);
}
