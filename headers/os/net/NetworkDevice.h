/*
 * Copyright 2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _NETWORK_DEVICE_H
#define _NETWORK_DEVICE_H


#include <net/if.h>

#include <NetworkAddress.h>


class BNetworkAddress;


struct wireless_network {
	char				name[32];
	BNetworkAddress		address;
	uint8				noise_level;
	uint8				signal_strength;
};

class BNetworkDevice {
public:
								BNetworkDevice();
								BNetworkDevice(const char* name);
								~BNetworkDevice();

			void				Unset();
			void				SetTo(const char* name);

			const char*			Name() const;
			uint32				Index() const;

			uint32				Flags() const;
			bool				HasLink() const;

			int32				CountMedia() const;
			int32				GetMediaAt(int32 index) const;

			int32				Media() const;
			status_t			SetMedia(int32 media);

			status_t			GetHardwareAddress(BNetworkAddress& address);

			bool				IsEthernet();
			bool				IsWireless();

			status_t			Scan(bool wait = true,
									bool forceRescan = true);
			ssize_t				CountScanResults();
			status_t			GetScanResultAt(int32 index,
									wireless_network& network);

			status_t			JoinNetwork(const BNetworkAddress& address,
									const char* password = NULL);
			status_t			LeaveNetwork();
			status_t			GetCurrentNetwork(wireless_network& network);
			status_t			GetCurrentNetwork(BNetworkAddress& address);

private:
			char				fName[IF_NAMESIZE];
};


#endif	// _NETWORK_DEVICE_H
