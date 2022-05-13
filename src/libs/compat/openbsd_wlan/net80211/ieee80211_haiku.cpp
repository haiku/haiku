/*
 * Copyright 2022, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

extern "C" {
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/kernel.h>
#include <sys/bus.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/endian.h>
#include <sys/errno.h>
#include <sys/task.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>

#include <netinet/in.h>
#include <netinet/if_ether.h>

#include <net80211/ieee80211_var.h>
}

#include <shared.h>

#include <util/KMessage.h>

#include <ether_driver.h>
#include <net_notifications.h>


#define TRACE_WLAN
#ifdef TRACE_WLAN
#	define TRACE(x...) dprintf(x);
#else
#	define TRACE(x...) ;
#endif


static net_notifications_module_info* sNotificationModule;


static struct ifnet*
get_ifnet(device_t device, int& i)
{
	void* softc = device_get_softc(device);

	for (i = 0; i < MAX_DEVICES; i++) {
		if (gDevices[i] != NULL && gDevices[i]->if_softc == softc)
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


static void
ieee80211_init()
{
}


status_t
start_wlan(device_t device)
{
	int i;
	struct ifnet* ifp = get_ifnet(device, i);
	if (ifp == NULL)
		return B_BAD_VALUE;

	struct ieee80211com* ic = (ieee80211com*)ifp;

	if (ifp->if_init == NULL)
		ifp->if_init = ieee80211_init;
	ifp->if_flags |= IFF_NEEDSGIANT;

	if_initname(ifp, device_get_name(device), i);

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

	struct ieee80211com* ic = (ieee80211com*)ifp;

	return B_OK;
}


status_t
wlan_close(void* cookie)
{
	dprintf("wlan_close(%p)\n", cookie);
	struct ifnet* ifp = (struct ifnet*)cookie;

	ifp->if_flags &= ~IFF_UP;
	ifp->if_ioctl(ifp, SIOCSIFFLAGS, NULL);

	return B_OK;
}


status_t
wlan_control(void* cookie, uint32 op, void* arg, size_t length)
{
	struct ifnet* ifp = (struct ifnet*)cookie;
#if 0
	switch (op) {
		case SIOCG80211:
		case SIOCS80211:
		{
			// FreeBSD drivers assume that the request structure has already
			// been copied into kernel space
			struct ieee80211req request;
			if (user_memcpy(&request, arg, sizeof(struct ieee80211req)) != B_OK)
				return B_BAD_ADDRESS;

			TRACE("wlan_control: %" B_PRIu32 ", %d\n", op, request.i_type);
			status_t status = ifp->if_ioctl(ifp, op, (caddr_t)&request);
			if (status != B_OK)
				return status;

			if (op == SIOCG80211 && user_memcpy(arg, &request,
					sizeof(struct ieee80211req)) != B_OK)
				return B_BAD_ADDRESS;
			return B_OK;
		}
	}
#endif

	return B_BAD_VALUE;
}


void
ieee80211_rtm_80211info_task(void* arg)
{
	struct ieee80211com *ic = arg;
	struct ifnet *ifp = &ic->ic_if;

	if (sNotificationModule != NULL) {
		char messageBuffer[512];
		KMessage message;
		message.SetTo(messageBuffer, sizeof(messageBuffer), B_NETWORK_MONITOR);
		if (ifp->if_link_state == LINK_STATE_UP)
			message.AddInt32("opcode", B_NETWORK_WLAN_JOINED);
		else
			message.AddInt32("opcode", B_NETWORK_WLAN_LEFT);
		message.AddString("interface", ifp->device_name);
		// TODO: add data about the node

		sNotificationModule->send_notification(&message);
	}
}


#if 0
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
#endif
