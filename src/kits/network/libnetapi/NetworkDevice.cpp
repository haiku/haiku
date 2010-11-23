/*
 * Copyright 2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <NetworkDevice.h>

#include <errno.h>
#include <net/if.h>
#include <sys/sockio.h>

#include <AutoDeleter.h>

extern "C" {
#	include <net80211/ieee80211_ioctl.h>
}


status_t
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
	return !IsWireless();
}


bool
BNetworkDevice::IsWireless()
{
	// TODO: fix me!
	return strstr(Name(), "wifi") != NULL;
}


ssize_t
BNetworkDevice::CountScanResults()
{
	// TODO: this needs some caching!
	const size_t kBufferSize = 32768;
	uint8* buffer = (uint8*)malloc(kBufferSize);
	if (buffer == NULL)
		return B_NO_MEMORY;

	MemoryDeleter deleter(buffer);

	int32 length = kBufferSize;
	status_t status = get_80211(Name(), IEEE80211_IOC_SCAN_RESULTS,
		buffer, length);
	if (status < B_OK)
		return status;

	int32 bytesLeft = length;
	uint8* entry = buffer;
	int32 count = 0;

	while (bytesLeft > (int32)sizeof(struct ieee80211req_scan_result)) {
		ieee80211req_scan_result* result
			= (ieee80211req_scan_result*)entry;

		entry += result->isr_len;
		bytesLeft -= result->isr_len;
		count++;
	}

	return count;
}


status_t
BNetworkDevice::GetScanResultAt(int32 index, wireless_network& network)
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

	const size_t kBufferSize = 32768;
	uint8* buffer = (uint8*)malloc(kBufferSize);
	if (buffer == NULL)
		return B_NO_MEMORY;

	MemoryDeleter deleter(buffer);

	int32 length = kBufferSize;
	status_t status = get_80211(Name(), IEEE80211_IOC_SCAN_RESULTS,
		buffer, length);
	if (status != B_OK)
		return status;

	int32 bytesLeft = length;
	uint8* entry = buffer;
	int32 count = 0;

	while (bytesLeft > (int32)sizeof(struct ieee80211req_scan_result)) {
		ieee80211req_scan_result* result
			= (ieee80211req_scan_result*)entry;

		if (count == index) {
			// Fill wireless_network with scan result data
			// TODO: add more fields
			strlcpy(network.name, (char*)(result + 1),
				min_c((int)sizeof(network.name), result->isr_ssid_len + 1));
			network.address.SetToLinkLevel(result->isr_bssid,
				IEEE80211_ADDR_LEN);
			network.signal_strength = result->isr_rssi;
			network.noise_level = result->isr_noise;

			return B_OK;
		}

		entry += result->isr_len;
		bytesLeft -= result->isr_len;
		count++;
	}

	return B_BAD_INDEX;
}
