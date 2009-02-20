/*
 * Copyright 2007-2008 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _DISCOVERY_AGENT_H
#define _DISCOVERY_AGENT_H

#include <bluetooth/bluetooth.h>
#include <bluetooth/DiscoveryListener.h>
#include <bluetooth/RemoteDevice.h>


#define BT_CACHED 0x00
#define BT_PREKNOWN 0x01
#define BT_NOT_DISCOVERABLE 0x01

#define BT_GIAC 0x9E8B33
#define BT_LIAC 0x9E8B00

#define BT_MAX_RESPONSES		(32)

#define BT_BASE_INQUIRY_TIME	(1.28)
#define BT_DEFAULT_INQUIRY_TIME	(0x0A)
#define BT_MIN_INQUIRY_TIME	(0x01) //  1.18 secs
#define BT_MAX_INQUIRY_TIME	(0x30) // 61.44 secs

namespace Bluetooth {

class LocalDevice;

class DiscoveryAgent {

public:

	static const int GIAC = BT_GIAC;
	static const int LIAC = BT_LIAC;

	static const int PREKNOWN = BT_PREKNOWN;
	static const int CACHED = BT_CACHED;
	static const int NOT_DISCOVERABLE = BT_NOT_DISCOVERABLE;

	RemoteDevicesList RetrieveDevices(int option); /* TODO */
	status_t StartInquiry(int accessCode, DiscoveryListener* listener); /* Throwing */
	status_t StartInquiry(uint32 accessCode, DiscoveryListener* listener, bigtime_t secs);
	status_t CancelInquiry(DiscoveryListener* listener);

	/*
	int searchServices(int[] attrSet, UUID[] uuidSet, RemoteDevice btDev,
					DiscoveryListener discListener);

	bool cancelServiceSearch(int transID);
	BString selectService(UUID uuid, int security, boolean master);
	*/

private:
	DiscoveryAgent(LocalDevice* ld);
	~DiscoveryAgent();
	void SetLocalDeviceOwner(LocalDevice* ld);

	DiscoveryListener* fLastUsedListener;
	LocalDevice*       fLocalDevice;
	BMessenger*        fMessenger;

	friend class LocalDevice;
};

}

#ifndef _BT_USE_EXPLICIT_NAMESPACE
using Bluetooth::DiscoveryAgent;
#endif

#endif // _DISCOVERY_AGENT_H
