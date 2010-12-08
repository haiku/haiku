/*
 * Copyright 2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <NetworkDevice.h>

#include <errno.h>
#include <net/if.h>
#include <net/if_media.h>
#include <stdio.h>
#include <sys/sockio.h>

#include <Messenger.h>

#include <AutoDeleter.h>
#include <NetServer.h>

extern "C" {
#	include <net80211/ieee80211_ioctl.h>
}


namespace {


struct ie_data {
	uint8	type;
	uint8	length;
	uint8	data[1];
};


static status_t
get_80211(const char* name, int32 type, void* data, int32& length)
{
	int socket = ::socket(AF_INET, SOCK_DGRAM, 0);
	if (socket < 0)
		return errno;

	FileDescriptorCloser closer(socket);

	struct ieee80211req ireq;
	strlcpy(ireq.i_name, name, IF_NAMESIZE);
	ireq.i_type = type;
	ireq.i_val = 0;
	ireq.i_len = length;
	ireq.i_data = data;

	if (ioctl(socket, SIOCG80211, &ireq, sizeof(struct ieee80211req)) < 0)
		return errno;

	length = ireq.i_len;
	return B_OK;
}


#if 0
static status_t
set_80211(const char* name, int32 type, void* data,
	int32 length = 0, int32 value = 0)
{
	int socket = ::socket(AF_INET, SOCK_DGRAM, 0);
	if (socket < 0)
		return errno;

	FileDescriptorCloser closer(socket);

	struct ieee80211req ireq;
	strlcpy(ireq.i_name, name, IF_NAMESIZE);
	ireq.i_type = type;
	ireq.i_val = value;
	ireq.i_len = length;
	ireq.i_data = data;

	if (ioctl(socket, SIOCS80211, &ireq, sizeof(struct ieee80211req)) < 0)
		return errno;

	return B_OK;
}


template<typename T> status_t
set_80211(const char* name, int32 type, T& data, int32 length = 0,
	int32 value = 0)
{
	if (length == 0)
		length = sizeof(T);

	return set_80211(name, &data, length, value);
}
#endif


template<typename T> status_t
do_request(T& request, const char* name, int option)
{
	int socket = ::socket(AF_LINK, SOCK_DGRAM, 0);
	if (socket < 0)
		return errno;

	FileDescriptorCloser closer(socket);

	strlcpy(((struct ifreq&)request).ifr_name, name, IF_NAMESIZE);

	if (ioctl(socket, option, &request, sizeof(T)) < 0)
		return errno;

	return B_OK;
}


//! Parse information elements.
static void
parse_ie(wireless_network& network, uint8* _ie, int32 ieLength)
{
	struct ie_data* ie = (ie_data*)_ie;

	while (ieLength > 1) {
		switch (ie->type) {
			case IEEE80211_ELEMID_SSID:
				strlcpy(network.name, (char*)ie->data,
					min_c(ie->length + 1, (int)sizeof(network.name)));
				break;
			case IEEE80211_ELEMID_RSN:
				// TODO: we might need to parse those in order to find out
				// the authentication mode (WPA info in vendor?)
				break;
		}

		ieLength -= 2 + ie->length;
		ie = (ie_data*)((uint8*)ie + 2 + ie->length);
	}
}


static void
parse_ie(wireless_network& network, struct ieee80211req_sta_info& info)
{
	parse_ie(network, (uint8*)&info + info.isi_ie_off, info.isi_ie_len);
}


static void
parse_ie(wireless_network& network, struct ieee80211req_scan_result& result)
{
	parse_ie(network, (uint8*)&result + result.isr_ie_off + result.isr_ssid_len
			+ result.isr_meshid_len, result.isr_ie_len);
}


static bool
get_ssid_from_ie(char* name, uint8* _ie, int32 ieLength)
{
	struct ie_data* ie = (ie_data*)_ie;

	while (ieLength > 1) {
		switch (ie->type) {
			case IEEE80211_ELEMID_SSID:
				strlcpy(name, (char*)ie->data, min_c(ie->length + 1, 32));
				return true;
		}

		ieLength -= 2 + ie->length;
		ie = (ie_data*)((uint8*)ie + 2 + ie->length);
	}
	return false;
}


static bool
get_ssid_from_ie(char* name, struct ieee80211req_sta_info& info)
{
	return get_ssid_from_ie(name, (uint8*)&info + info.isi_ie_off,
		info.isi_ie_len);
}


static void
fill_wireless_network(wireless_network& network,
	struct ieee80211req_sta_info& info)
{
	network.name[0] = '\0';
	network.address.SetToLinkLevel(info.isi_macaddr,
		IEEE80211_ADDR_LEN);
	network.signal_strength = info.isi_rssi;
	network.noise_level = info.isi_noise;
	network.flags |= (info.isi_capinfo & IEEE80211_CAPINFO_PRIVACY) != 0
		? B_NETWORK_IS_ENCRYPTED : 0;
	network.authentication_mode = 0;
		// TODO: build from IE if possible

	parse_ie(network, info);
}


static void
fill_wireless_network(wireless_network& network, const char* networkName,
	struct ieee80211req_scan_result& result)
{
	strlcpy(network.name, networkName, sizeof(network.name));
	network.address.SetToLinkLevel(result.isr_bssid,
		IEEE80211_ADDR_LEN);
	network.signal_strength = result.isr_rssi;
	network.noise_level = result.isr_noise;
	network.flags = (result.isr_capinfo & IEEE80211_CAPINFO_PRIVACY)
		!= 0 ? B_NETWORK_IS_ENCRYPTED : 0;
	network.authentication_mode = 0;
		// TODO: build from IE if possible

	parse_ie(network, result);
}


static status_t
get_scan_result(const char* device, wireless_network& network, uint32 index,
	const BNetworkAddress* address, const char* name)
{
	if (address != NULL && address->Family() != AF_LINK)
		return B_BAD_VALUE;

	const size_t kBufferSize = 65535;
	uint8* buffer = (uint8*)malloc(kBufferSize);
	if (buffer == NULL)
		return B_NO_MEMORY;

	MemoryDeleter deleter(buffer);

	int32 length = kBufferSize;
	status_t status = get_80211(device, IEEE80211_IOC_SCAN_RESULTS, buffer,
		length);
	if (status != B_OK)
		return status;

	int32 bytesLeft = length;
	uint8* entry = buffer;
	uint32 count = 0;

	while (bytesLeft > (int32)sizeof(struct ieee80211req_scan_result)) {
		ieee80211req_scan_result* result
			= (ieee80211req_scan_result*)entry;

		char networkName[32];
		strlcpy(networkName, (char*)(result + 1),
			min_c((int)sizeof(networkName), result->isr_ssid_len + 1));

		if (index == count || (address != NULL && !memcmp(
				address->LinkLevelAddress(), result->isr_bssid,
				IEEE80211_ADDR_LEN))
			|| (name != NULL && !strcmp(networkName, name))) {
			// Fill wireless_network with scan result data
			fill_wireless_network(network, networkName, *result);
			return B_OK;
		}

		entry += result->isr_len;
		bytesLeft -= result->isr_len;
		count++;
	}

	return B_ENTRY_NOT_FOUND;
}


static status_t
get_station(const char* device, wireless_network& network, uint32 index,
	const BNetworkAddress* address, const char* name)
{
	if (address != NULL && address->Family() != AF_LINK)
		return B_BAD_VALUE;

	const size_t kBufferSize = 65535;
	uint8* buffer = (uint8*)malloc(kBufferSize);
	if (buffer == NULL)
		return B_NO_MEMORY;

	MemoryDeleter deleter(buffer);

	struct ieee80211req_sta_req& request = *(ieee80211req_sta_req*)buffer;
	if (address != NULL) {
		memcpy(request.is_u.macaddr, address->LinkLevelAddress(),
			IEEE80211_ADDR_LEN);
	} else
		memset(request.is_u.macaddr, 0xff, IEEE80211_ADDR_LEN);

	int32 length = kBufferSize;
	status_t status = get_80211(device, IEEE80211_IOC_STA_INFO, &request,
		length);
	if (status != B_OK)
		return status;

	int32 bytesLeft = length;
	uint8* entry = (uint8*)&request.info[0];
	uint32 count = 0;

	while (bytesLeft > (int32)sizeof(struct ieee80211req_sta_info)) {
		ieee80211req_sta_info* info = (ieee80211req_sta_info*)entry;

		char networkName[32];
		get_ssid_from_ie(networkName, *info);
		if (index == count || address != NULL
			|| (name != NULL && !strcmp(networkName, name))) {
			fill_wireless_network(network, *info);
			return B_OK;
		}

		entry += info->isi_len;
		bytesLeft -= info->isi_len;
		count++;
	}

	return B_ENTRY_NOT_FOUND;
}


static status_t
get_network(const char* device, wireless_network& network, uint32 index,
	const BNetworkAddress* address, const char* name)
{
	status_t status = get_station(device, network, index, address, name);
	if (status != B_OK)
		return get_scan_result(device, network, index, address, name);

	return B_OK;
}


}	// namespace


// #pragma mark -


BNetworkDevice::BNetworkDevice()
{
	Unset();
}


BNetworkDevice::BNetworkDevice(const char* name)
{
	SetTo(name);
}


BNetworkDevice::~BNetworkDevice()
{
}


void
BNetworkDevice::Unset()
{
	fName[0] = '\0';
}


void
BNetworkDevice::SetTo(const char* name)
{
	strlcpy(fName, name, IF_NAMESIZE);
}


const char*
BNetworkDevice::Name() const
{
	return fName;
}


bool
BNetworkDevice::Exists() const
{
	ifreq request;
	return do_request(request, Name(), SIOCGIFINDEX) == B_OK;
}


uint32
BNetworkDevice::Index() const
{
	ifreq request;
	if (do_request(request, Name(), SIOCGIFINDEX) != B_OK)
		return 0;

	return request.ifr_index;
}


uint32
BNetworkDevice::Flags() const
{
	ifreq request;
	if (do_request(request, Name(), SIOCGIFFLAGS) != B_OK)
		return 0;

	return request.ifr_flags;
}


bool
BNetworkDevice::HasLink() const
{
	return (Flags() & IFF_LINK) != 0;
}


int32
BNetworkDevice::CountMedia() const
{
	ifmediareq request;
	request.ifm_count = 0;
	request.ifm_ulist = NULL;

	if (do_request(request, Name(), SIOCGIFMEDIA) != B_OK)
		return -1;

	return request.ifm_count;
}


int32
BNetworkDevice::Media() const
{
	ifmediareq request;
	request.ifm_count = 0;
	request.ifm_ulist = NULL;

	if (do_request(request, Name(), SIOCGIFMEDIA) != B_OK)
		return -1;

	return request.ifm_current;
}


int32
BNetworkDevice::GetMediaAt(int32 index) const
{
	// TODO: this could do some caching
	return 0;
}


status_t
BNetworkDevice::SetMedia(int32 media)
{
	ifreq request;
	request.ifr_media = media;
	return do_request(request, Name(), SIOCSIFMEDIA);
}


status_t
BNetworkDevice::GetHardwareAddress(BNetworkAddress& address)
{
	ifreq request;
	status_t status = do_request(request, Name(), SIOCSIFMEDIA);
	if (status != B_OK)
		return status;

	address.SetTo(request.ifr_addr);
	return B_OK;
}


bool
BNetworkDevice::IsEthernet()
{
	return IFM_TYPE(Media()) == IFM_ETHER;
}


bool
BNetworkDevice::IsWireless()
{
	return IFM_TYPE(Media()) == IFM_IEEE80211;
}


status_t
BNetworkDevice::Scan(bool wait, bool forceRescan)
{
#if 0
	if (index == 0) {
		struct ieee80211_scan_req request;
		memset(&request, 0, sizeof(request));
		request.sr_flags = IEEE80211_IOC_SCAN_ACTIVE | IEEE80211_IOC_SCAN_ONCE
			| IEEE80211_IOC_SCAN_NOJOIN;
		request.sr_duration = IEEE80211_IOC_SCAN_FOREVER;
		set_80211(Name(), IEEE80211_IOC_SCAN_REQ, NULL);
	}
#endif
	return B_ERROR;
}


status_t
BNetworkDevice::GetNextNetwork(uint32& cookie, wireless_network& network)
{
	status_t status = get_scan_result(Name(), network, cookie, NULL, NULL);
	if (status != B_OK)
		return status;

	cookie++;
	return B_OK;
}


status_t
BNetworkDevice::GetNetwork(const char* name, wireless_network& network)
{
	if (name == NULL || name[0] == '\0')
		return B_BAD_VALUE;

	return get_network(Name(), network, UINT32_MAX, NULL, name);
}


status_t
BNetworkDevice::GetNetwork(const BNetworkAddress& address,
	wireless_network& network)
{
	if (address.Family() != AF_LINK)
		return B_BAD_VALUE;

	return get_network(Name(), network, UINT32_MAX, &address, NULL);
}


status_t
BNetworkDevice::JoinNetwork(const char* name, const char* password)
{
	if (name == NULL || name[0] == '\0')
		return B_BAD_VALUE;

	BMessage message(kMsgJoinNetwork);
	status_t status = message.AddString("device", Name());

	if (status == B_OK)
		status = message.AddString("name", name);
	if (status == B_OK && password != NULL)
		status = message.AddString("password", password);
	if (status != B_OK)
		return status;

	// Send message to the net_server

	BMessenger networkServer(kNetServerSignature);
	BMessage reply;
	status = networkServer.SendMessage(&message, &reply);
	if (status == B_OK)
		reply.FindInt32("status", &status);

	return status;
}


status_t
BNetworkDevice::JoinNetwork(const wireless_network& network,
	const char* password)
{
	return JoinNetwork(network.address, password);
}


status_t
BNetworkDevice::JoinNetwork(const BNetworkAddress& address,
	const char* password)
{
	if (address.InitCheck() != B_OK)
		return B_BAD_VALUE;

	BMessage message(kMsgJoinNetwork);
	status_t status = message.AddString("device", Name());

	if (status == B_OK) {
		status = message.AddFlat("address",
			const_cast<BNetworkAddress*>(&address));
	}
	if (status == B_OK && password != NULL)
		status = message.AddString("password", password);
	if (status != B_OK)
		return status;

	// Send message to the net_server

	BMessenger networkServer(kNetServerSignature);
	BMessage reply;
	status = networkServer.SendMessage(&message, &reply);
	if (status == B_OK)
		reply.FindInt32("status", &status);

	return status;
}


status_t
BNetworkDevice::LeaveNetwork(const char* name)
{
	BMessage message(kMsgLeaveNetwork);
	status_t status = message.AddString("device", Name());
	if (status == B_OK)
		status = message.AddString("name", name);
	if (status == B_OK)
		status = message.AddInt32("reason", IEEE80211_REASON_UNSPECIFIED);
	if (status != B_OK)
		return status;

	BMessenger networkServer(kNetServerSignature);
	BMessage reply;
	status = networkServer.SendMessage(&message, &reply);
	if (status == B_OK)
		reply.FindInt32("status", &status);

	return status;
}


status_t
BNetworkDevice::LeaveNetwork(const wireless_network& network)
{
	return LeaveNetwork(network.address);
}


status_t
BNetworkDevice::LeaveNetwork(const BNetworkAddress& address)
{
	BMessage message(kMsgLeaveNetwork);
	status_t status = message.AddString("device", Name());
	if (status == B_OK) {
		status = message.AddFlat("address",
			const_cast<BNetworkAddress*>(&address));
	}
	if (status == B_OK)
		status = message.AddInt32("reason", IEEE80211_REASON_UNSPECIFIED);
	if (status != B_OK)
		return status;

	BMessenger networkServer(kNetServerSignature);
	BMessage reply;
	status = networkServer.SendMessage(&message, &reply);
	if (status == B_OK)
		reply.FindInt32("status", &status);

	return status;
}


status_t
BNetworkDevice::GetNextAssociatedNetwork(uint32& cookie,
	wireless_network& network)
{
	BNetworkAddress address;
	status_t status = GetNextAssociatedNetwork(cookie, address);
	if (status != B_OK)
		return status;

	return GetNetwork(address, network);
}


status_t
BNetworkDevice::GetNextAssociatedNetwork(uint32& cookie,
	BNetworkAddress& address)
{
	// We currently support only a single associated network
	if (cookie != 0)
		return B_ENTRY_NOT_FOUND;

	uint8 mac[IEEE80211_ADDR_LEN];
	int32 length = IEEE80211_ADDR_LEN;
	status_t status = get_80211(Name(), IEEE80211_IOC_BSSID, mac, length);
	if (status != B_OK)
		return status;

	if (mac[0] == 0 && mac[1] == 0 && mac[2] == 0 && mac[3] == 0 && mac[4] == 0
		&& mac[5] == 0)
		return B_ENTRY_NOT_FOUND;

	address.SetToLinkLevel(mac, IEEE80211_ADDR_LEN);
	cookie++;
	return B_OK;
}
