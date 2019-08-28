/*
 * Copyright 2006-2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval
 *
 * References:
 *   Google search "technic doc genius" , http://www.bebits.com/app/2152
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <Debug.h>
#include <InterfaceDefs.h>
#include <SerialPort.h>

#include "EasyPenInputDevice.h"
#include "keyboard_mouse_driver.h"

/*
IN ABSOLUTE MODE (MM Series)

                      Least                          Most

  Bytes       Bit to  Significant                    Significant   Bit to

Transmitted   Start   Bit                            Bit           Stop

                      0   1   2   3    4    5    6    7   8

-----------   ----- ---------------------------------------------  ------

1st Byte       0     Fa  Fb  Fc  Sy   Sx   T    PR   PH  P          1

2nd Byte       0     X0  X1  X2  X3   X4   X5   X6   0   P          1

3rd Byte       0     X7  X8  X9  X10  X11  X12  X13  0   P          1

4th Byte       0     Y0  Y1  Y2  Y3   Y4   Y5   Y6   0   P          1

5th Byte       0     Y7  Y8  Y9  Y10  Y11  Y12  Y13  0   P          1

*/

#if DEBUG
        inline void LOG(const char *fmt, ...) { char buf[1024]; va_list ap; va_start(ap, fmt); vsprintf(buf, fmt, ap); va_end(ap); \
                fputs(buf, EasyPenInputDevice::sLogFile); fflush(EasyPenInputDevice::sLogFile); }
        #define LOG_ERR(text...) LOG(text)
FILE *EasyPenInputDevice::sLogFile = NULL;
#else
        #define LOG(text...)
        #define LOG_ERR(text...) fprintf(stderr, text)
#endif

#define CALLED() LOG("%s\n", __PRETTY_FUNCTION__)

const static uint32 kTabletThreadPriority = B_FIRST_REAL_TIME_PRIORITY + 4;

struct tablet_device {
	tablet_device(BSerialPort *port);
	~tablet_device();

	input_device_ref device_ref;
	thread_id device_watcher;
	bool active;
	BSerialPort *serial;
	EasyPenInputDevice *owner;
};


extern "C"
BInputServerDevice *
instantiate_input_device()
{
	return new EasyPenInputDevice();
}


EasyPenInputDevice::EasyPenInputDevice()
{
#if DEBUG
	sLogFile = fopen("/var/log/easypen_device_log.log", "a");
#endif
}


EasyPenInputDevice::~EasyPenInputDevice()
{
	CALLED();
	for (int32 i = 0; i < fDevices.CountItems(); i++)
		delete (tablet_device *)fDevices.ItemAt(i);

#if DEBUG
	fclose(sLogFile);
#endif
}


status_t
EasyPenInputDevice::InitCheck()
{
	CALLED();

	BSerialPort *serial = new BSerialPort();
	int count = serial->CountDevices();
	char devName[B_OS_NAME_LENGTH];
	char byte;

	for (int i=0; i<count; i++) {
		serial->GetDeviceName(i, devName);
		serial->SetDataRate(B_9600_BPS);
		serial->SetDataBits(B_DATA_BITS_8);
		serial->SetStopBits(B_STOP_BITS_1);
		serial->SetParityMode(B_ODD_PARITY);
		serial->SetFlowControl(B_HARDWARE_CONTROL+B_SOFTWARE_CONTROL);
		serial->SetBlocking(true);
		serial->SetTimeout(800000);

		if (serial->Open(devName) < B_OK) {
			LOG("Fail to open %s\n", devName);
			continue;
		}
		snooze(250000);
		serial->Write("z9", 2); // 8 data bits,odd  parity <command>                     z 9
		serial->Write("", 1); //	Reset                <command>                NUL
		serial->Write("DP", 2);		// mode command    D   trigger command    P
		snooze(250000);
		serial->ClearInput();
		serial->Write("twP", 3);	// Self-Test, Send Test Results, trigger command    P
		serial->Read(&byte, 1);
		if ((byte & 0xf7) != 0x87) {
			LOG("Self test failed for %s %x\n", devName, byte & 0xf7);
			continue;
		}
		tablet_device *device = new tablet_device(serial);
		device->owner = this;

		input_device_ref *devices[2];
		devices[0] = &device->device_ref;
		devices[1] = NULL;

		fDevices.AddItem(device);
		RegisterDevices(devices);

		serial = new BSerialPort();
	}

	delete serial;

	LOG("Found %ld devices\n", fDevices.CountItems());

	get_click_speed(&fClickSpeed);

	return fDevices.CountItems() > 0 ? B_OK : B_ERROR;
}


status_t
EasyPenInputDevice::Start(const char *name, void *cookie)
{
	tablet_device *device = (tablet_device *)cookie;

	LOG("%s(%s)\n", __PRETTY_FUNCTION__, name);

	char threadName[B_OS_NAME_LENGTH];
	snprintf(threadName, B_OS_NAME_LENGTH, "%s watcher", name);

	device->active = true;
	device->device_watcher = spawn_thread(DeviceWatcher, threadName,
		kTabletThreadPriority, device);

	if (device->device_watcher < B_OK)
		return device->device_watcher;
	resume_thread(device->device_watcher);

	return B_OK;
}


status_t
EasyPenInputDevice::Stop(const char *name, void *cookie)
{
	tablet_device *device = (tablet_device *)cookie;

	LOG("%s(%s)\n", __PRETTY_FUNCTION__, name);

	device->active = false;
	if (device->device_watcher >= 0) {
		suspend_thread(device->device_watcher);
		resume_thread(device->device_watcher);
		status_t dummy;
		wait_for_thread(device->device_watcher, &dummy);
	}

	return B_OK;
}


status_t
EasyPenInputDevice::Control(const char *name, void *cookie,
						  uint32 command, BMessage *message)
{
	LOG("%s(%s, code: %lu)\n", __PRETTY_FUNCTION__, name, command);

	if (command == B_CLICK_SPEED_CHANGED)
		get_click_speed(&fClickSpeed);

	return B_OK;
}


int32
EasyPenInputDevice::DeviceWatcher(void *arg)
{
	tablet_device *dev = (tablet_device *)arg;
	EasyPenInputDevice *owner = dev->owner;

	tablet_movement movements;
	tablet_movement old_movements;
	BMessage *message;
	uint8 byte;
	uint8 bytes[4];
	bigtime_t lastClickTimeStamp = 0LL;

	memset(&movements, 0, sizeof(movements));
	memset(&old_movements, 0, sizeof(old_movements));

	dev->serial->Write(":@FRb", 5);

	while (dev->active) {
		byte = 0;
		while(!byte)
			dev->serial->Read(&byte, 1);
		if (byte & 0x40 || !(byte & 0x80)) { // 7th bit on or 8th bit off
			snooze(10000); // this is a realtime thread, and something is wrong...
			continue;
		}

		if (dev->serial->Read(bytes, 4) != 4)
			continue;

		if (*(uint32*)bytes & 0x80808080)
			continue;

		movements.buttons = byte & 0x7;
		movements.timestamp = system_time();
		movements.xpos = (bytes[1] << 7) | bytes[0];
		movements.ypos = (bytes[3] << 7) | bytes[2];

		uint32 buttons = old_movements.buttons ^ movements.buttons;

		LOG("%s: buttons: 0x%lx, x: %f, y: %f, clicks:%ld, wheel_x:%ld, wheel_y:%ld\n", dev->device_ref.name, movements.buttons,
			movements.xpos, movements.ypos, movements.clicks, movements.wheel_xdelta, movements.wheel_ydelta);

		movements.xpos /= 1950.0;
		movements.ypos /= 1500.0;

		LOG("%s: x: %f, y: %f, \n", dev->device_ref.name, movements.xpos, movements.ypos);

		if (buttons != 0) {
			message = new BMessage(B_MOUSE_UP);
			if ((buttons & movements.buttons) > 0) {
				message->what = B_MOUSE_DOWN;
				if(lastClickTimeStamp + owner->fClickSpeed <= movements.timestamp)
					movements.clicks = 1;
				else
					movements.clicks++;
				lastClickTimeStamp = movements.timestamp;
				message->AddInt32("clicks", movements.clicks);
				LOG("B_MOUSE_DOWN\n");
			} else {
				LOG("B_MOUSE_UP\n");
			}

			message->AddInt64("when", movements.timestamp);
			message->AddInt32("buttons", movements.buttons);
			message->AddFloat("x", movements.xpos);
			message->AddFloat("y", movements.ypos);
			owner->EnqueueMessage(message);
		}

		if (movements.xpos != 0.0 || movements.ypos != 0.0) {
			message = new BMessage(B_MOUSE_MOVED);
			if (message) {
				message->AddInt64("when", movements.timestamp);
				message->AddInt32("buttons", movements.buttons);
				message->AddFloat("x", movements.xpos);
				message->AddFloat("y", movements.ypos);
				message->AddFloat("be:tablet_x", movements.xpos);
				message->AddFloat("be:tablet_y", movements.ypos);
				message->AddFloat("be:tablet_pressure", movements.pressure);
				message->AddInt32("be:tablet_eraser", movements.eraser);
				if (movements.tilt_x != 0.0 || movements.tilt_y != 0.0) {
					message->AddFloat("be:tablet_tilt_x", movements.tilt_x);
					message->AddFloat("be:tablet_tilt_y", movements.tilt_y);
				}

				owner->EnqueueMessage(message);
			}
		}

		if ((movements.wheel_ydelta != 0) || (movements.wheel_xdelta != 0)) {
			message = new BMessage(B_MOUSE_WHEEL_CHANGED);
			if (message) {
				message->AddInt64("when", movements.timestamp);
				message->AddFloat("be:wheel_delta_x", movements.wheel_xdelta);
				message->AddFloat("be:wheel_delta_y", movements.wheel_ydelta);

				owner->EnqueueMessage(message);
			}
		}

		old_movements = movements;

	}

	return 0;
}


// tablet_device
tablet_device::tablet_device(BSerialPort *port)
{
	serial = port;
	device_watcher = -1;
	active = false;
	device_ref.name = strdup("Genius EasyPen");
	device_ref.type = B_POINTING_DEVICE;
	device_ref.cookie = this;
};


tablet_device::~tablet_device()
{
	free(device_ref.name);
}

