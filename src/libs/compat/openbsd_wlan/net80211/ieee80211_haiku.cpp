/*
 * Copyright 2022, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <AutoDeleter.h>
#include <util/KMessage.h>

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
#include <net80211/ieee80211_node.h>
#include <net80211/ieee80211_ioctl.h>

#undef _NET80211_IEEE80211_IOCTL_H_
#define IEEE80211_IOCTLS_ABBREVIATED
#include "../../freebsd_wlan/net80211/ieee80211_ioctl.h"
}

#include <shared.h>

#include <ether_driver.h>
#include <NetworkDevice.h>
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

	TRACE("%s: wlan started.\n", __func__);
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
	TRACE("wlan_close(%p)\n", cookie);
	struct ifnet* ifp = (struct ifnet*)cookie;

	ifp->if_flags &= ~IFF_UP;
	ifp->if_ioctl(ifp, SIOCSIFFLAGS, NULL);

	return B_OK;
}


static uint8_t
fbsd_capinfo_from_obsd(uint16_t obsd_capinfo)
{
	// FreeBSD only exposes the first 8 bits of the capinfo,
	// and these are identical in OpenBSD. Makes things easy.
	return uint8_t(obsd_capinfo);
}


static uint32
obsd_ciphers_from_haiku(uint32 ciphers)
{
	uint32 obsd_ciphers = 0;
	if ((ciphers & B_NETWORK_CIPHER_WEP_40) != 0)
		obsd_ciphers |= IEEE80211_WPA_CIPHER_WEP40;
	if ((ciphers & B_NETWORK_CIPHER_WEP_104) != 0)
		obsd_ciphers |= IEEE80211_WPA_CIPHER_WEP104;
	if ((ciphers & B_NETWORK_CIPHER_TKIP) != 0)
		obsd_ciphers |= IEEE80211_WPA_CIPHER_TKIP;
	if ((ciphers & B_NETWORK_CIPHER_CCMP) != 0)
		obsd_ciphers |= IEEE80211_WPA_CIPHER_CCMP;
	return obsd_ciphers;
}


status_t
wlan_control(void* cookie, uint32 op, void* arg, size_t length)
{
	if (op != SIOCG80211 && op != SIOCS80211)
		return B_BAD_VALUE;

	struct ifnet* ifp = (struct ifnet*)cookie;
	struct ieee80211com* ic = (ieee80211com*)ifp;

	struct ieee80211req ireq;
	if (user_memcpy(&ireq, arg, sizeof(struct ieee80211req)) != B_OK)
		return B_BAD_ADDRESS;

	switch (ireq.i_type) {
		case IEEE80211_IOC_SCAN_REQ: {
			if (op != SIOCS80211)
				return B_BAD_VALUE;

			// SIOCS80211SCAN is a no-op, scans cannot actually be initiated.
			// But we can at least check if one is already in progress.
			status_t status = EBUSY;

			IFF_LOCKGIANT(ifp);
			if (ic->ic_state == IEEE80211_S_SCAN || (ic->ic_flags & IEEE80211_F_BGSCAN) != 0)
				status = EINPROGRESS;
			IFF_UNLOCKGIANT(ifp);

			return status;
		}

		case IEEE80211_IOC_SCAN_RESULTS: {
			if (op != SIOCG80211)
				return B_BAD_VALUE;

			struct ieee80211_nodereq nodereq;
			struct ieee80211_nodereq_all nodereq_all = {};

			// We need a scan_result of maximum possible size to work with.
			struct ieee80211req_scan_result* sr = (struct ieee80211req_scan_result*)
				alloca(sizeof(struct ieee80211req_scan_result) + IEEE80211_NWID_LEN + 257);

			uint16 remaining = ireq.i_len, offset = 0;
			for (int i = 0; remaining > 0; i++) {
				nodereq_all.na_node = &nodereq;
				nodereq_all.na_size = sizeof(struct ieee80211_nodereq);
				nodereq_all.na_startnode = i;

				IFF_LOCKGIANT(ifp);
				status_t status = ifp->if_ioctl(ifp, SIOCG80211ALLNODES, (caddr_t)&nodereq_all);
				IFF_UNLOCKGIANT(ifp);
				if (status != B_OK)
					return status;
				if (nodereq_all.na_nodes == 0)
					break;

				int32 size = sizeof(struct ieee80211req_scan_result) + nodereq.nr_nwid_len;
				uint16_t ieLen = 0;
				if (nodereq.nr_rsnie[1] != 0) {
					ieLen = 2 + nodereq.nr_rsnie[1];
					size += ieLen;
				}
				const int32 roundedSize = roundup(size, 4);
				if (remaining < roundedSize)
					break;

				sr->isr_ie_off = sizeof(struct ieee80211req_scan_result);
				sr->isr_ie_len = ieLen;
				sr->isr_freq = ieee80211_ieee2mhz(nodereq.nr_channel, nodereq.nr_chan_flags);
				sr->isr_flags = 0; /* not actually used by userland */
				sr->isr_noise = 0; /* unknown */
				sr->isr_rssi = nodereq.nr_rssi;
				sr->isr_intval = nodereq.nr_intval;
				sr->isr_capinfo = fbsd_capinfo_from_obsd(nodereq.nr_capinfo);
				sr->isr_erp = nodereq.nr_erp;
				memcpy(sr->isr_bssid, nodereq.nr_bssid, IEEE80211_ADDR_LEN);
				sr->isr_nrates = nodereq.nr_nrates;
				memcpy(sr->isr_rates, nodereq.nr_rates, IEEE80211_RATE_MAXSIZE);
				sr->isr_ssid_len = nodereq.nr_nwid_len;
				sr->isr_meshid_len = 0;
				memcpy((uint8*)sr + sr->isr_ie_off, nodereq.nr_nwid, sr->isr_ssid_len);
				memcpy((uint8*)sr + sr->isr_ie_off + sr->isr_ssid_len, nodereq.nr_rsnie, ieLen);

				sr->isr_len = roundedSize;
				if (user_memcpy((uint8*)ireq.i_data + offset, sr, size) != B_OK)
					return B_BAD_ADDRESS;

				offset += sr->isr_len;
				remaining -= sr->isr_len;
			}
			ireq.i_len = offset;

			IFF_LOCKGIANT(ifp);
			const bigtime_t RAISE_INACT_INTERVAL = 5 * 1000 * 1000 /* 5s */;
			if (ic->ic_opmode == IEEE80211_M_STA
					&& system_time() >= (ic->ic_last_raise_inact + RAISE_INACT_INTERVAL)) {
				// net80211 only raises and checks inactivity during active scans, or background
				// scans performed in S_RUN, so we need to do it here so that stale nodes are
				// evicted for S_SCAN, too. (This means "inact" will be raised a bit more often
				// and nodes evicted faster during other modes, but that's acceptable.)
				ieee80211_iterate_nodes(ic, ieee80211_node_raise_inact, NULL);
				ieee80211_clean_inactive_nodes(ic, IEEE80211_INACT_SCAN);
				ic->ic_last_raise_inact = system_time();
			}
			IFF_UNLOCKGIANT(ifp);

			break;
		}

		case IEEE80211_IOC_BSSID: {
			if (ireq.i_len != IEEE80211_ADDR_LEN)
				return EINVAL;

			struct ieee80211_bssid bssid;
			if (op == SIOCS80211) {
				if (user_memcpy(bssid.i_bssid, ireq.i_data, ireq.i_len) != B_OK)
					return B_BAD_ADDRESS;
			}

			IFF_LOCKGIANT(ifp);
			status_t status = ifp->if_ioctl(ifp, op == SIOCG80211 ?
				SIOCG80211BSSID : SIOCS80211BSSID, (caddr_t)&bssid);
			IFF_UNLOCKGIANT(ifp);
			if (status != B_OK)
				return status;

			if (op == SIOCG80211) {
				if (user_memcpy(ireq.i_data, bssid.i_bssid, ireq.i_len) != B_OK)
					return B_BAD_ADDRESS;
			}

			break;
		}

		case IEEE80211_IOC_SSID: {
			struct ifreq ifr;
			struct ieee80211_nwid nwid;
			ifr.ifr_data = (uint8_t*)&nwid;

			if (op == SIOCS80211) {
				nwid.i_len = ireq.i_len;
				if (user_memcpy(nwid.i_nwid, ireq.i_data, ireq.i_len) != B_OK)
					return B_BAD_ADDRESS;
			}

			IFF_LOCKGIANT(ifp);
			status_t status = ifp->if_ioctl(ifp, op == SIOCG80211 ?
				SIOCG80211NWID : SIOCS80211NWID, (caddr_t)&ifr);
			ieee80211_state state = ic->ic_state;
			IFF_UNLOCKGIANT(ifp);
			if (status != B_OK)
				return status;

			if (op == SIOCG80211) {
				if (state == IEEE80211_S_INIT || state == IEEE80211_S_SCAN) {
					// The returned NWID will be the one we want to join, not connected to.
					ireq.i_len = 0;
				} else {
					if (ireq.i_len < nwid.i_len)
						return E2BIG;
					ireq.i_len = nwid.i_len;
					if (user_memcpy(ireq.i_data, nwid.i_nwid, ireq.i_len) != B_OK)
						return B_BAD_ADDRESS;
				}
			}

			break;
		}

		case IEEE80211_IOC_MLME: {
			if (op != SIOCS80211)
				return B_BAD_VALUE;

			struct ieee80211req_mlme mlme;
			if (user_memcpy(&mlme, ireq.i_data, min_c(sizeof(mlme), ireq.i_len)) != B_OK)
				return B_BAD_ADDRESS;

			switch (mlme.im_op) {
				case IEEE80211_MLME_DISASSOC:
				case IEEE80211_MLME_DEAUTH: {
					struct ifreq ifr;
					struct ieee80211_nwid nwid;
					ifr.ifr_data = (uint8_t*)&nwid;
					nwid.i_len = 0;

					IFF_LOCKGIANT(ifp);
					status_t status = ifp->if_ioctl(ifp, SIOCS80211NWID, (caddr_t)&ifr);
					IFF_UNLOCKGIANT(ifp);
					if (status != 0)
						return status;
					break;
				}

				default:
					TRACE("openbsd wlan_control: unsupported MLME operation %" B_PRIu8 "\n",
						mlme.im_op);
					return EOPNOTSUPP;
			}

			break;
		}

		case IEEE80211_IOC_HAIKU_JOIN: {
			if (op != SIOCS80211)
				return B_BAD_VALUE;

			struct ieee80211_haiku_join_req* haiku_join =
				(struct ieee80211_haiku_join_req*)calloc(1, ireq.i_len);
			MemoryDeleter haikuJoinDeleter(haiku_join);
			if (!haikuJoinDeleter.IsSet())
				return B_NO_MEMORY;

			if (user_memcpy(haiku_join, ireq.i_data, ireq.i_len) != B_OK)
				return B_BAD_ADDRESS;

			struct ifreq ifr;
			struct ieee80211_nwid nwid;
			struct ieee80211_wpaparams wpaparams;
			struct ieee80211_wpapsk wpapsk;
			memset(&wpaparams, 0, sizeof(wpaparams));
			memset(&wpapsk, 0, sizeof(wpapsk));

			// Convert join information.
			ifr.ifr_data = (uint8_t*)&nwid;
			nwid.i_len = haiku_join->i_nwid_len;
			memcpy(nwid.i_nwid, haiku_join->i_nwid, nwid.i_len);

			switch (haiku_join->i_authentication_mode) {
				case B_NETWORK_AUTHENTICATION_NONE:
					break;

				case B_NETWORK_AUTHENTICATION_WPA:
				case B_NETWORK_AUTHENTICATION_WPA2:
					wpaparams.i_enabled = 1;
					wpaparams.i_protos |=
						(haiku_join->i_authentication_mode == B_NETWORK_AUTHENTICATION_WPA2) ?
							IEEE80211_WPA_PROTO_WPA2 : IEEE80211_WPA_PROTO_WPA1;

					// NOTE: i_wpaparams.i_groupcipher is not a flags field!
					wpaparams.i_ciphers = obsd_ciphers_from_haiku(haiku_join->i_ciphers);
					wpaparams.i_groupcipher =
						fls(obsd_ciphers_from_haiku(haiku_join->i_group_ciphers));

					if ((haiku_join->i_key_mode & B_KEY_MODE_IEEE802_1X) != 0)
						wpaparams.i_akms |= IEEE80211_WPA_AKM_8021X;
					if ((haiku_join->i_key_mode & B_KEY_MODE_PSK) != 0)
						wpaparams.i_akms |= IEEE80211_WPA_AKM_PSK;
					if ((haiku_join->i_key_mode & B_KEY_MODE_IEEE802_1X_SHA256) != 0)
						wpaparams.i_akms |= IEEE80211_WPA_AKM_SHA256_8021X;
					if ((haiku_join->i_key_mode & B_KEY_MODE_PSK_SHA256) != 0)
						wpaparams.i_akms |= IEEE80211_WPA_AKM_SHA256_PSK;

					if (haiku_join->i_key_len != 0) {
						wpapsk.i_enabled = 1;
						memcpy(wpapsk.i_psk, haiku_join->i_key, haiku_join->i_key_len);
						memset(&wpapsk.i_psk[haiku_join->i_key_len], 0,
							sizeof(wpapsk.i_psk) - haiku_join->i_key_len);
					}
					break;

				default:
					TRACE("openbsd wlan_control: unsupported authentication mode %" B_PRIu32 "\n",
						haiku_join->i_authentication_mode);
					return EOPNOTSUPP;
			}

			IFF_LOCKGIANT(ifp);
			status_t status = ifp->if_ioctl(ifp, SIOCS80211NWID, (caddr_t)&ifr);
			if (status != B_OK) {
				IFF_UNLOCKGIANT(ifp);
				return status;
			}
			if (wpapsk.i_enabled) {
				status = ifp->if_ioctl(ifp, SIOCS80211WPAPSK, (caddr_t)&wpapsk);
				if (status != B_OK) {
					IFF_UNLOCKGIANT(ifp);
					return status;
				}
			}
			if (wpaparams.i_enabled) {
				status = ifp->if_ioctl(ifp, SIOCS80211WPAPARMS, (caddr_t)&wpaparams);
				if (status != B_OK) {
					IFF_UNLOCKGIANT(ifp);
					return status;
				}
			}
			IFF_UNLOCKGIANT(ifp);
			if (status != B_OK)
				return status;

			break;
		}

		default:
			TRACE("openbsd wlan_control: %" B_PRIu32 ", %d (not supported)\n", op, ireq.i_type);
			return EOPNOTSUPP;
	}

	if (op == SIOCG80211 && user_memcpy(arg, &ireq,
			sizeof(struct ieee80211req)) != B_OK)
		return B_BAD_ADDRESS;
	return B_OK;
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
