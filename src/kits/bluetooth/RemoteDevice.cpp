#include <bluetooth/DiscoveryAgent.h>
#include <bluetooth/DiscoveryListener.h>
#include <bluetooth/bdaddrUtils.h>

#include <bluetooth/RemoteDevice.h>

namespace Bluetooth {


bool
RemoteDevice::IsTrustedDevice(void)
{
    return true;
}


BString
RemoteDevice::GetFriendlyName(bool alwaysAsk)
{
    return BString("Not implemented");
}


BString
RemoteDevice::GetBluetoothAddress()
{
    return BString(bdaddrUtils::ToString(fBdaddr));
}


bool
RemoteDevice::Equals(RemoteDevice* obj)
{
    return true;
}

//  static RemoteDevice* GetRemoteDevice(Connection conn);

bool
RemoteDevice::Authenticate()
{
    return true;
}


//  bool Authorize(Connection conn);
//  bool Encrypt(Connection conn, bool on);


bool
RemoteDevice::IsAuthenticated()
{
    return true;
}


//  bool IsAuthorized(Connection conn);


bool
RemoteDevice::IsEncrypted()
{
    return true;
}

/* Private */
void
RemoteDevice::SetLocalDeviceOwner(LocalDevice* ld)
{
    fDiscovererLocalDevice = ld;
}

/* Constructor */
RemoteDevice::RemoteDevice(bdaddr_t address)
{
	fBdaddr = address;
}

RemoteDevice::RemoteDevice(BString address)
{
   /* TODO */
   
}

}