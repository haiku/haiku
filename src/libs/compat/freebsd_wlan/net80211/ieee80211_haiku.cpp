/*
 * Copyright 2009, Colin Günther, coling@gmx.de.
 * All rights reserved. Distributed under the terms of the MIT License.
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
#include <bosii_driver.h>
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
	ieee80211_phy_init();
	ieee80211_auth_setup();
	ieee80211_ht_init();

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
	int i;
	struct ifnet* ifp = get_ifnet(device, i);
	if (ifp == NULL)
		return B_BAD_VALUE;

// TODO: review this and find a cleaner solution!
	// This ensures that the cloned device gets
	// the same index assigned as the base device
	// Resulting in the same device name
	// e.g.: /dev/net/atheros/0 instead of
	//       /dev/net/atheros/1
	gDevices[i] = NULL;

	struct ieee80211com* ic = (ieee80211com*)ifp->if_l2com;

	struct ieee80211vap* vap = ic->ic_vap_create(ic, "wlan",
		device_get_unit(device),
		IEEE80211_M_STA,		// mode
		0,						// flags
		NULL,					// BSSID
		IF_LLADDR(ifp));		// MAC address

	if (vap == NULL) {
		gDevices[i] = ifp;
		return B_ERROR;
	}

	// ic_vap_create() established that gDevices[i] links to vap->iv_ifp now
	KASSERT(gDevices[i] == vap->iv_ifp,
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

	if (ifp->if_type == IFT_IEEE80211) {
		// This happens when there was an error in starting the wlan before,
		// resulting in never creating a clone device
		return B_OK;
	}

	delete_sem(ifp->scan_done_sem);

	struct ieee80211vap* vap = (ieee80211vap*)ifp->if_softc;
	struct ieee80211com* ic = vap->iv_ic;

	ic->ic_vap_delete(vap);

	// ic_vap_delete freed gDevices[i]
	KASSERT(gDevices[i] == NULL, ("stop_wlan: gDevices[i] != NULL"));

	// assign the base device ifp again
	gDevices[i] = ic->ic_ifp;

	return B_OK;
}


status_t
wlan_control(void* cookie, uint32 op, void* arg, size_t length)
{
	struct ifnet* ifp = (struct ifnet*)cookie;

	switch (op) {
		case BOSII_DEVICE:
			return B_OK;

		case BOSII_DETECT_NETWORKS:
		{
			struct ieee80211req request;
			struct ieee80211_scan_req scanRequest;

			if_printf(ifp, "%s: BOSII_DETECT_NETWORKS\n", __func__);
			memset(&scanRequest, 0, sizeof(scanRequest));
			scanRequest.sr_flags = IEEE80211_IOC_SCAN_ACTIVE
				| IEEE80211_IOC_SCAN_NOPICK
				| IEEE80211_IOC_SCAN_ONCE;
			scanRequest.sr_duration = 10000; // 10 s
			scanRequest.sr_nssid = 0;

			memset(&request, 0, sizeof(request));
			request.i_type = IEEE80211_IOC_SCAN_REQ;
			request.i_data = &scanRequest;
			request.i_len = sizeof(scanRequest);

			ifp->if_ioctl(ifp, SIOCS80211, (caddr_t)&request);

			acquire_sem_etc(ifp->scan_done_sem, 1, B_RELATIVE_TIMEOUT,
				10000000); // 10 s

			return B_OK;
		}

		case BOSII_GET_DETECTED_NETWORKS:
		{
			struct ieee80211req request;
			struct ifreq ifRequest;
			struct route_entry* networkRequest = &ifRequest.ifr_route;

			if_printf(ifp, "%s: BOSII_GET_DETECTED_NETWORKS\n", __func__);

			if (length < sizeof(struct ieee80211req_scan_result))
				return B_BAD_VALUE;

			if (user_memcpy(&ifRequest, arg, sizeof(ifRequest)) < B_OK)
				return B_BAD_ADDRESS;

			memset(&request, 0, sizeof(request));
			request.i_type = IEEE80211_IOC_SCAN_RESULTS;
			request.i_len = length;
			request.i_data = networkRequest->destination;

			// After return value of request.i_data is copied into user
			// space, already.
			if (ifp->if_ioctl(ifp, SIOCG80211, (caddr_t)&request) < B_OK)
				return B_BAD_ADDRESS;

			// Tell the user space how much data was copied
			networkRequest->mtu = request.i_len;
			if (user_memcpy(&((struct ifreq*)arg)->ifr_route.mtu,
				&networkRequest->mtu, sizeof(networkRequest->mtu)) < B_OK)
				return B_BAD_ADDRESS;

			return B_OK;
		}

		case BOSII_JOIN_NETWORK:
		{
			struct ieee80211req request;
			struct ifreq ifRequest;
			struct route_entry* networkRequest = &ifRequest.ifr_route;
			struct ieee80211req_scan_result network;

			if_printf(ifp, "%s: BOSII_JOIN_NETWORK\n", __func__);

			if (length < sizeof(struct ifreq))
				return B_BAD_VALUE;

			if (user_memcpy(&ifRequest, arg, sizeof(ifRequest)) != B_OK
				|| user_memcpy(&network, networkRequest->source,
						sizeof(ieee80211req_scan_result)) != B_OK)
				return B_BAD_ADDRESS;

			memset(&request, 0, sizeof(ieee80211req));

			request.i_type = IEEE80211_IOC_SSID;
			request.i_val = 0;
			request.i_len = network.isr_ssid_len;
			request.i_data = (uint8*)networkRequest->source
				+ network.isr_ie_off;
			if (ifp->if_ioctl(ifp, SIOCS80211, (caddr_t)&request) < B_OK)
				return B_ERROR;

			// wait for network join

			return B_OK;
		}

		case BOSII_GET_ASSOCIATED_NETWORK:
		{
			struct ieee80211req request;
			struct ifreq ifRequest;
			struct route_entry* networkRequest = &ifRequest.ifr_route;

			if_printf(ifp, "%s: BOSII_GET_ASSOCIATED_NETWORK\n", __func__);

			if (length < sizeof(struct ieee80211req_sta_req))
				return B_BAD_VALUE;

			if (user_memcpy(&ifRequest, arg, sizeof(ifRequest)) < B_OK)
				return B_BAD_ADDRESS;

			// Only want station information about associated network.
			memset(&request, 0, sizeof(request));
			request.i_type = IEEE80211_IOC_BSSID;
			request.i_len = IEEE80211_ADDR_LEN;
			request.i_data = ((struct ieee80211req_sta_req*)networkRequest->
					destination)->is_u.macaddr;
			if (ifp->if_ioctl(ifp, SIOCG80211, (caddr_t)&request) < B_OK)
				return B_BAD_ADDRESS;

			request.i_type = IEEE80211_IOC_STA_INFO;
			request.i_len = length;
			request.i_data = networkRequest->destination;

			// After return value of request.i_data is copied into user
			// space, already.
			if (ifp->if_ioctl(ifp, SIOCG80211, (caddr_t)&request) < B_OK)
				return B_BAD_ADDRESS;

			// Tell the user space how much data was copied
			networkRequest->mtu = request.i_len;
			if (user_memcpy(&((struct ifreq*)arg)->ifr_route.mtu,
					&networkRequest->mtu, sizeof(networkRequest->mtu)) != B_OK)
				return B_BAD_ADDRESS;

			return B_OK;
		}

		case SIOCG80211:
		case SIOCS80211:
		{
			// Allowing FreeBSD based WLAN ioctls to pass, as those will become
			// the future Haiku WLAN ioctls anyway.

			// FreeBSD drivers assume that the request structure has already
			// been copied into kernel space
			struct ieee80211req request;
			if (user_memcpy(&request, arg, sizeof(struct ieee80211req)) != B_OK)
				return B_BAD_ADDRESS;

			TRACE("wlan_control: %ld, %d\n", op, request.i_type);
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


status_t
wlan_close(void* cookie)
{
	struct ifnet* ifp = (struct ifnet*)cookie;

	ifp->if_flags &= ~IFF_UP;
	ifp->if_ioctl(ifp, SIOCSIFFLAGS, NULL);

	return release_sem_etc(ifp->scan_done_sem, 1, B_RELEASE_ALL);
}


status_t
wlan_if_l2com_alloc(void* data)
{
	struct ifnet* ifp = (struct ifnet*)data;

	ifp->if_l2com = _kernel_malloc(sizeof(struct ieee80211com), M_ZERO);
	if (ifp->if_l2com == NULL)
		return B_NO_MEMORY;
	((struct ieee80211com*)(ifp->if_l2com))->ic_ifp = ifp;
	return B_OK;
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


struct mbuf*
ieee80211_getmgtframe(uint8_t** frm, int headroom, int pktlen)
{
	struct mbuf* m;
	u_int len;

	len = roundup2(headroom + pktlen, 4);
	KASSERT(len <= MCLBYTES, ("802.11 mgt frame too large: %u", len));
	if (len < MINCLSIZE) {
		m = m_gethdr(M_NOWAIT, MT_DATA);
		if (m != NULL)
			MH_ALIGN(m, len);
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
 * tests whether it became zero.
 *
 * @return 1 reference-counter became zero
 * @return 0 reference-counter didn't became zero
 */
int
ieee80211_node_dectestref(struct ieee80211_node* ni)
{
	// atomic_add returns old value
	return atomic_add((vint32*)&ni->ni_refcnt, -1) == 1;
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


void
ieee80211_sysctl_vattach(struct ieee80211vap* vap)
{
	vap->iv_debug = IEEE80211_MSG_XRATE
		| IEEE80211_MSG_NODE
		| IEEE80211_MSG_ASSOC
		| IEEE80211_MSG_AUTH
		| IEEE80211_MSG_STATE
		| IEEE80211_MSG_POWER
		| IEEE80211_MSG_WME
		| IEEE80211_MSG_DOTH
		| IEEE80211_MSG_INACT
		| IEEE80211_MSG_ROAM
		| IEEE80211_MSG_RATECTL;
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
	dprintf("%s not implemented, yet: modname %s\n", __func__, modname);
}


void
ieee80211_notify_node_join(struct ieee80211_node* ni, int newassoc)
{
	struct ieee80211vap* vap = ni->ni_vap;
	struct ifnet* ifp = vap->iv_ifp;

	if (ni == vap->iv_bss)
		if_link_state_change(ifp, LINK_STATE_UP);

	TRACE("%s\n", __FUNCTION__);

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
