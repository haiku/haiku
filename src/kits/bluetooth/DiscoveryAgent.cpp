/*
 * Copyright 2007-2008 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 */

#include <bluetooth/DiscoveryAgent.h>
#include <bluetooth/DiscoveryListener.h>

#include <bluetooth/RemoteDevice.h>

namespace Bluetooth {


RemoteDevice**
DiscoveryAgent::RetrieveDevices(int option)
{
	return NULL;

}


bool
DiscoveryAgent::StartInquiry(int accessCode, DiscoveryListener listener)
{
	return false;

}


bool
DiscoveryAgent::CancelInquiry(DiscoveryListener listener)
{
	return false;
}
        

DiscoveryAgent::DiscoveryAgent()
{

}

}
