/*****************************************************************************/
// OpenBeOS InputServer
//
// Version: [0.0.5] [Development Stage]
//
// [Description]
//
//
// This application and all source files used in its construction, except 
// where noted, are licensed under the MIT License, and have been written 
// and are:
//
// Copyright (c) 2002 OpenBeOS Project
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


#include "InputServerDevice.h"
#include "InputServer.h"

/**
 *  Method: BInputServerDevice::BInputServerDevice()
 *   Descr: 
 */
BInputServerDevice::BInputServerDevice()
{
}


/**
 *  Method: BInputServerDevice::~BInputServerDevice()
 *   Descr: 
 */
BInputServerDevice::~BInputServerDevice()
{
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
/*	
	// Lock this section of code so that access to the device list
	// is serialized.  Be sure to release the lock before exitting.
	//
	InputServer::gInputDeviceListLocker.Lock();
	
	bool has_pointing_device = false;
	bool has_keyboard_device = false;
	
	input_device_ref *device = NULL;
	for (int i = 0; NULL != (device = devices[i]); i++)
	{
		// :TODO: Check to make sure that each device is valid and
		// that it is not already registered.
		// getDeviceIndex()
		
		InputServer::gInputDeviceList.AddItem(new InputDeviceListItem(this, device) );
		
		has_pointing_device = has_pointing_device || (device->type == B_POINTING_DEVICE);
		has_keyboard_device = has_keyboard_device || (device->type == B_KEYBOARD_DEVICE);
	}

	// If the InputServer event loop is running, signal the InputServer
	// to start all devices of the type managed by this InputServerDevice.
	//
	if (InputServer::EventLoopRunning() )
	{
		if (has_pointing_device)
		{
			InputServer::StartStopDevices(NULL, B_POINTING_DEVICE, true);
		}
		if (has_keyboard_device)
		{
			InputServer::StartStopDevices(NULL, B_KEYBOARD_DEVICE, true);
		}
	}
	
	InputServer::gInputDeviceListLocker.Unlock();
	*/
	return B_OK;
}


/**
 *  Method: BInputServerDevice::UnregisterDevices()
 *   Descr: 
 */
status_t
BInputServerDevice::UnregisterDevices(input_device_ref **devices)
{
    status_t dummy;

    return dummy;
}


/**
 *  Method: BInputServerDevice::EnqueueMessage()
 *   Descr: 
 */
status_t
BInputServerDevice::EnqueueMessage(BMessage *message)
{
/*	status_t  	err;
	
	ssize_t length = message->FlattenedSize();
	char* buffer = new char[length];
	if ((err = message->Flatten(buffer,length)) < 0) {
	} else {
		if((write_port(mEventPort, 0, buffer, length)) < 0) {
		
		}else {
		free(buffer);
		err = B_OK;
		}
	}
	return err;
	*/
	return B_OK;
}


/**
 *  Method: BInputServerDevice::StartMonitoringDevice()
 *   Descr: 
 */
status_t
BInputServerDevice::StartMonitoringDevice(const char *device)
{
    status_t dummy;

    return dummy;
}


/**
 *  Method: BInputServerDevice::StopMonitoringDevice()
 *   Descr: 
 */
status_t
BInputServerDevice::StopMonitoringDevice(const char *device)
{
    status_t dummy;

    return dummy;
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


