/*****************************************************************************/
// Haiku InputServer
//
// This is the InputServerDevice implementation
//
// This application and all source files used in its construction, except 
// where noted, are licensed under the MIT License, and have been written 
// and are:
//
// Copyright (c) 2002-2004 Haiku Project
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included 
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/

#include <Autolock.h>
#include <InputServerDevice.h>
#include "InputServer.h"

/**
 *  Method: BInputServerDevice::BInputServerDevice()
 *   Descr: 
 */
BInputServerDevice::BInputServerDevice()
{
	fOwner = new _BDeviceAddOn_(this);
}


/**
 *  Method: BInputServerDevice::~BInputServerDevice()
 *   Descr: 
 */
BInputServerDevice::~BInputServerDevice()
{
	CALLED();
	 // Lock this section of code so that access to the device list
	// is serialized.  Be sure to release the lock before exitting.
	//
	InputServer::gInputDeviceListLocker.Lock();
	
	for (int i = InputServer::gInputDeviceList.CountItems() - 1; 
		i >= 0 && i < InputServer::gInputDeviceList.CountItems(); i--) {
	
		PRINT(("%s Device #%d\n", __PRETTY_FUNCTION__, i));
		InputDeviceListItem* item = (InputDeviceListItem*) InputServer::gInputDeviceList.ItemAt(i);
		if (NULL != item && this == item->mIsd) {
			if (item->mStarted) {
				input_device_ref   dev = item->mDev;
				PRINT(("  Stopping: %s\n", dev.name)); 
				Stop(dev.name, dev.cookie);
			}
			InputServer::gInputDeviceList.RemoveItem(i++);
			delete item;
		}
	}
	
	InputServer::gInputDeviceListLocker.Unlock();
	
	delete fOwner;
}


/**
 *  Method: BInputServerDevice::InitCheck()
 *   Descr: 
 */
status_t
BInputServerDevice::InitCheck()
{
    return B_OK;
}


/**
 *  Method: BInputServerDevice::SystemShuttingDown()
 *   Descr: 
 */
status_t
BInputServerDevice::SystemShuttingDown()
{
    return B_OK;
}


/**
 *  Method: BInputServerDevice::Start()
 *   Descr: 
 */
status_t
BInputServerDevice::Start(const char *device,
                          void *cookie)
{
    return B_OK;
}


/**
 *  Method: BInputServerDevice::Stop()
 *   Descr: 
 */
status_t
BInputServerDevice::Stop(const char *device,
                         void *cookie)
{
    return B_OK;
}


/**
 *  Method: BInputServerDevice::Control()
 *   Descr: 
 */
status_t
BInputServerDevice::Control(const char *device,
                            void *cookie,
                            uint32 code,
                            BMessage *message)
{
    return B_OK;
}


/**
 *  Method: BInputServerDevice::RegisterDevices()
 *   Descr: 
 */
status_t
BInputServerDevice::RegisterDevices(input_device_ref **devices)
{
	CALLED();

	BAutolock lock(InputServer::gInputDeviceListLocker);

	input_device_ref *device = NULL;
	for (int i = 0; NULL != (device = devices[i]); i++) {
		if (device->type != B_POINTING_DEVICE 
			&& device->type != B_KEYBOARD_DEVICE 
			&& device->type != B_UNDEFINED_DEVICE)
			continue;
			
		bool found = false;
		for (int j = InputServer::gInputDeviceList.CountItems() - 1; j >= 0; j--) {
			InputDeviceListItem* item = (InputDeviceListItem*)InputServer::gInputDeviceList.ItemAt(j);
			if (!item)
				continue;
				
			if (strcmp(device->name, item->mDev.name) == 0) {
				found = true;
				break;
			}
		}
		
		if (!found) {
			PRINT(("RegisterDevices not found %s\n", device->name));
			InputServer::gInputDeviceList.AddItem(new InputDeviceListItem(this, *device) );
			InputServer::StartStopDevices(this, true);
		} else {
			PRINT(("RegisterDevices found %s\n", device->name));
		}
	}

	return B_OK;
}


/**
 *  Method: BInputServerDevice::UnregisterDevices()
 *   Descr: 
 */
status_t
BInputServerDevice::UnregisterDevices(input_device_ref **devices)
{
    CALLED();
    
    BAutolock lock(InputServer::gInputDeviceListLocker);

	input_device_ref *device = NULL;
	for (int i = 0; NULL != (device = devices[i]); i++) {
		
		for (int j = InputServer::gInputDeviceList.CountItems() - 1; j >= 0; j--) {
			InputDeviceListItem* item = (InputDeviceListItem*)InputServer::gInputDeviceList.ItemAt(j);
			if (!item)
				continue;
				
			if (strcmp(device->name, item->mDev.name) == 0) {
				Stop(item->mDev.name, item->mDev.cookie);
				InputServer::gInputDeviceList.RemoveItem(j);
				break;
			}
		}
	}
	
    return B_OK;
}


/**
 *  Method: BInputServerDevice::EnqueueMessage()
 *   Descr: 
 */
status_t
BInputServerDevice::EnqueueMessage(BMessage *message)
{
	return ((InputServer*)be_app)->EnqueueDeviceMessage(message);
}


/**
 *  Method: BInputServerDevice::StartMonitoringDevice()
 *   Descr: 
 */
status_t
BInputServerDevice::StartMonitoringDevice(const char *device)
{
	CALLED();
	PRINT(("StartMonitoringDevice %s\n", device));

    return InputServer::gDeviceManager.StartMonitoringDevice(fOwner, device);
}


/**
 *  Method: BInputServerDevice::StopMonitoringDevice()
 *   Descr: 
 */
status_t
BInputServerDevice::StopMonitoringDevice(const char *device)
{
    CALLED();
    return InputServer::gDeviceManager.StopMonitoringDevice(fOwner, device);
}

/**
///////////////////////// Below this line is reserved functions /////////////////////////////
*/


/**
 *  Method: BInputServerDevice::_ReservedInputServerDevice1()
 *   Descr: 
 */
void
BInputServerDevice::_ReservedInputServerDevice1()
{
}


/**
 *  Method: BInputServerDevice::_ReservedInputServerDevice2()
 *   Descr: 
 */
void
BInputServerDevice::_ReservedInputServerDevice2()
{
}


/**
 *  Method: BInputServerDevice::_ReservedInputServerDevice3()
 *   Descr: 
 */
void
BInputServerDevice::_ReservedInputServerDevice3()
{
}


/**
 *  Method: BInputServerDevice::_ReservedInputServerDevice4()
 *   Descr: 
 */
void
BInputServerDevice::_ReservedInputServerDevice4()
{
}


