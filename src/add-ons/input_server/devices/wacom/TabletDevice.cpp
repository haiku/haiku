/*
 * Copyright 2003-2008 Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Copyright 2000-2002  Olaf van Es. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 *
 * These people have added and tested device ids:
 *
 *		Frans van Nispen	<frans@xentronix.com>
 *		Stefan Werner		<stefan@keindesign.de>
 *		Hiroyuki Tsutsumi	<???>
 *		Jeroen Oortwijn		<oortwijn@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <File.h>
#include <InterfaceDefs.h>
#include <Screen.h>
#include <View.h>

#include "DeviceReader.h"
#include "MasterServerDevice.h"

#include "TabletDevice.h"

#define SNOOZE_AMOUNT 2500
#define JITTER_X .0007
#define JITTER_Y .0007
#define ACCELERATION_KICK_IN 2.3

// constructor
TabletDevice::TabletDevice(MasterServerDevice* parent, DeviceReader* reader)
	: PointingDevice(parent, reader),
	  fThreadID(B_ERROR),
	  fDeviceMode(DEVICE_UNKOWN),
	  fMaxX(1.0),
	  fMaxY(1.0),
	  fPosX(0.5),
	  fPosY(0.5),
	  fFakeMouseX(0.5),
	  fFakeMouseY(0.5),
	  fButtons(0),
	  fPressure(0.0),
	  fModifiers(0),
	  fEraser(0),
	  fTiltX(0.0),
	  fTiltY(0.0),
	  fClicks(0),
	  fHasContact(false)
{
}

// destructor
TabletDevice::~TabletDevice()
{
	// cleanup
	Stop();
}

// InitCheck
status_t
TabletDevice::InitCheck()
{
	status_t status = PointingDevice::InitCheck();
	if (status >= B_OK)
		status = DetectDevice(fReader);
	return status;
}

// Start
status_t
TabletDevice::Start()
{
	status_t status = B_NO_INIT;
	if (fReader) {
		fActive = true;
		// get a nice name for our polling thread
		const char* name;
		_GetName(fReader->ProductID(), &name);
		// start generating events
		fThreadID = spawn_thread(poll_usb_device, name, 104, this);
		if (fThreadID >= B_OK) {
			resume_thread(fThreadID);
			status = B_OK;
		} else
			status = fThreadID;
	}
	return status;
}

// Stop
status_t
TabletDevice::Stop()
{
	status_t err = B_OK;

	fActive = false;
	if (fThreadID >= B_OK)
		wait_for_thread(fThreadID, &err);
	fThreadID = B_ERROR;

	return err;
}

// DetectDevice
status_t
TabletDevice::DetectDevice(const DeviceReader* reader)
{
	status_t status = B_OK;
	switch (reader->ProductID()) {
		case 0x00:
			SetDevice(5040.0, 3780.0, DEVICE_PENPARTNER);
			break;
		case 0x03:
			SetDevice(2048.0, 15360.0, DEVICE_PL500);
			break;
		case 0x10:
		case 0x11:
		case 0x13:
			SetDevice(10206.0, 7422.0, DEVICE_GRAPHIRE);
			break;
		case 0x12:	// Graphire 3 4x5
			SetDevice(13918.0, 10206.0, DEVICE_GRAPHIRE);
			break;
		case 0x14:	// Graphire 3 6x8
			SetDevice(16704.0, 12064.0, DEVICE_GRAPHIRE);
			break;
		case 0x15:	// Graphire 4 4x5 (tested)
			SetDevice(10208.0, 7024.0, DEVICE_GRAPHIRE);
			break;
		case 0x16:	// Graphire 4 6x8 (tested)
			SetDevice(16704.0, 12064.0, DEVICE_GRAPHIRE);
			break;
		case 0x17:	// BambooFun 4x5 (from Linux Wacom Project)
			SetDevice(14760.0, 9225.0, DEVICE_BAMBOO);
			break;
		case 0x18:	// BambooFun 6x8 (from Linux Wacom Project)
			SetDevice(21648.0, 13530.0, DEVICE_BAMBOO);
			break;
		case 0x20:
			SetDevice(12700.0, 10600.0, DEVICE_INTUOS);
			break;
		case 0x21:
			SetDevice(20320.0, 16240.0);
			break;
		case 0x22:
			SetDevice(30480.0, 24060.0);
			break;
		case 0x23:
			SetDevice(30480.0, 31680.0);
			break;
		case 0x24:
			SetDevice(45720.0, 31680.0);
			break;
		case 0x30:
			SetDevice(5408.0, 4056.0, DEVICE_PL500);
			break;
		case 0x31:
			SetDevice(6144.0, 4608.0, DEVICE_PL500);
			break;
		case 0x32:
			SetDevice(6126.0, 4604.0, DEVICE_PL500);
			break;
		case 0x33:
			SetDevice(6260.0, 5016.0, DEVICE_PL500);
			break;
		case 0x34:
			SetDevice(6144.0, 4608.0, DEVICE_PL500);
			break;
		case 0x35:
			SetDevice(7220.0, 5780.0, DEVICE_PL500);
			break;
		case 0x3F:
			SetDevice(87200.0, 65600.0, DEVICE_CINTIQ);
			break;
		case 0x41:
			SetDevice(12700.0, 10600.0);
			break;
		case 0x42:
			SetDevice(20320.0, 16240.0);
			break;
		case 0x43:
			SetDevice(30480.0, 24060.0);
			break;
		case 0x44:
			SetDevice(30480.0, 31680.0);
			break;
		case 0x45:
			SetDevice(45720.0, 31680.0);
			break;
		case 0x47:	// some I2 6x8 report as 0x47
			SetDevice(20320.0, 16240.0);
			break;
		case 0x60:
			SetDevice(5104.0, 3712.0, DEVICE_GRAPHIRE);
			break;
		case 0x61: // PenStation
//			SetDevice(3403.0, 2475.0, DEVICE_GRAPHIRE); // this version was untested
			SetDevice(3248.0, 2320.0, DEVICE_PENSTATION); // this version came from "beer"
			break;
		case 0x62: // Volito
			SetDevice(5040.0, 3712.0, DEVICE_VOLITO);
			break;
		case 0x64:	// PenPartner.1
//			SetDevice(3450.0, 2100.0, DEVICE_PENSTATION);
			SetDevice(3248.0, 2320.0, DEVICE_PENSTATION);
			break;
		case 0x65:	// Bamboo (from Linux Wacom Project)
			SetDevice(14760.0, 9225.0, DEVICE_BAMBOO);
			break;
		case 0x69:	// Bamboo1 (from Linux Wacom Project)
			SetDevice(5104.0,  3712.0, DEVICE_BAMBOO);
			break;
		case 0xB0:
			SetDevice(25400.0, 20320.0, DEVICE_INTUOS3);
			break;
		case 0xB1:
			SetDevice(40640.0, 30480.0, DEVICE_INTUOS3);
			break;
		case 0xB2:
			SetDevice(60960.0, 45720.0, DEVICE_INTUOS3);
			break;
		case 0xD0:	// Wacom Bamboo 2FG (from Linux Wacom Project)
			SetDevice(14720.0, 9200.0, DEVICE_BAMBOO_PT);
			break;
		case 0xD1:	// Wacom BambooFun 2FG 4x5 (from Linux Wacom Project)
			SetDevice(14720.0, 9200.0, DEVICE_BAMBOO_PT);
			break;
		case 0xD2:	// Wacom Bamboo Craft (from Linux Wacom Project)
			SetDevice(14720.0, 9200.0, DEVICE_BAMBOO_PT);
			break;
		case 0xD3:	// Wacom BambooFun 2FG 6x8 (from Linux Wacom Project)
			SetDevice(21648.0, 13530.0, DEVICE_BAMBOO_PT);
			break;
		case 0xD4:	// Wacom Bamboo 4x5 (from Linux Wacom Project)
			SetDevice(14720.0, 9200.0, DEVICE_BAMBOO_PT);
			break;
		case 0xD6:	// Wacom Bamboo CTH-460/K (from Linux Wacom Project)
			SetDevice(14720.0, 9200.0, DEVICE_BAMBOO_PT);
			break;
		case 0xD7:	// Wacom Bamboo CTH-461/S (from Linux Wacom Project)
			SetDevice(14720.0, 9200.0, DEVICE_BAMBOO_PT);
			break;
		case 0xD8:	// Wacom Bamboo CTH-661/S1 (from Linux Wacom Project)
			SetDevice(21648.0, 13530.0, DEVICE_BAMBOO_PT);
			break;
		case 0xDA:	// Wacom Bamboo CTH-461/L (from Linux Wacom Project)
			SetDevice(14720.0, 9200.0, DEVICE_BAMBOO_PT);
			break;
		case 0xDB:	// Wacom Bamboo CTH-661 (from Linux Wacom Project)
			SetDevice(21648.0, 13530.0, DEVICE_BAMBOO_PT);
			break;
		default:
			status = B_BAD_VALUE;
			break;
	}
	return status;
}

// SetDevice
void
TabletDevice::SetDevice(float maxX, float maxY, uint32 mode)
{
	fDeviceMode = mode;
	fMaxX = maxX;
	fMaxY = maxY;
	fJitterX = JITTER_X;
	fJitterY = JITTER_Y;
}

// ReadData
void
TabletDevice::ReadData(const uchar* data, int dataBytes, bool& hasContact,
	uint32& mode, uint32& buttons, float& x, float& y, float& pressure,
	int32& clicks, int32& eraser, float& wheelX, float& wheelY,
	float& tiltX, float& tiltY) const
{
	hasContact = false;
	buttons = 0;
	mode = MODE_PEN;
	bool firstButton = false;
	bool secondButton = false;
	bool thirdButton = false;
	uint16 xPos = 0;
	uint16 yPos = 0;

	switch (fDeviceMode) {
		case DEVICE_PENPARTNER: {
			xPos = data[2] << 8 | data[1];
			yPos = data[4] << 8 | data[3];

			eraser = (data[5] & 0x20);

			int8 pressureData = data[6];
			pressure = (float)(pressureData + 120) / 240.0;

			firstButton = ((pressureData > -80) && !(data[5] & 0x20));
			secondButton = (data[5] & 0x40);

			hasContact = true;
			break;
		}
		case DEVICE_GRAPHIRE:
		case DEVICE_BAMBOO:
		{
			xPos = data[3] << 8 | data[2];
			yPos = data[5] << 8 | data[4];

			hasContact = (data[1] & 0x80);

			uint16 pressureData = data[7] << 8 | data[6];
			pressure = (float)pressureData / 511.0;
			eraser = (data[1] & 0x20);

			// mouse wheel support
			if (data[1] & 0x40) {	// mouse is on tablet!
				wheelY = (float)(int8)data[6];
				mode = MODE_MOUSE;
				// override contact to loose it as soon as possible
				// when mouse is lifted from tablet
				hasContact = (uint8)data[7] >= 30;
				pressure = 0.0;
				eraser = 0;
			}

			firstButton = pressure > 0.0 ? true : (data[1] & 0x01);
			secondButton = (data[1] & 0x02);
			thirdButton = (data[1] & 0x04);

			break;
		}
		case DEVICE_BAMBOO_PT:
		{
			if (dataBytes < 20) {	// ignore touch-packets
				xPos = data[3] << 8 | data[2];
				yPos = data[5] << 8 | data[4];

				hasContact = (data[1] & 0x20);

				uint16 pressureData = data[7] << 8 | data[6];
				pressure = (float)pressureData / 1023.0;
				eraser = (data[1] & 0x08);

				firstButton = (data[1] & 0x01);
				secondButton = (data[1] & 0x02);
				thirdButton = (data[1] & 0x04);

				break;
			}
		}
		case DEVICE_INTUOS:
		case DEVICE_INTUOS3:
		case DEVICE_CINTIQ:
			if ((data[0] == 0x02) && !(((data[1] >> 5) & 0x03) == 0x02)) {
				if (fDeviceMode == DEVICE_INTUOS3) {
					xPos = (data[2] << 9) | (data[3] << 1)
						| ((data[9] >> 1) & 1);
					yPos = (data[4] << 9) | (data[5] << 1) | (data[9] & 1);
				} else {
					xPos = (data[2] << 8) | data[3];
					yPos = (data[4] << 8) | data[5];
				}
				uint16 pressureData = data[6] << 2 | ((data[7] >> 6) & 0x03);
				pressure = (float)pressureData / 1023.0;

				// mouse and wheel support
				if (data[1] == 0xf0) {	// mouse is on tablet!
					mode = MODE_MOUSE;

					if (data[8] == 0x02)
						wheelY = 1.0;
					else if (data[8] == 0x01)
						wheelY = -1.0;

					firstButton = (data[8] & 0x04);
					secondButton = (data[8] & 0x10);
					thirdButton = (data[8] & 0x08);

					// override contact to loose it as soon as possible
					// when mouse is lifted from tablet
					hasContact = data[9] <= 0x68;
					pressure = 0.0;
					eraser = 0;
				} else {
//					eraser = (data[1] & 0x20); // eraser is een tool-id
//					firstButton = (pressureData > 0) && data[9] <= 0x68;// > 180);
//					firstButton = (pressureData > 180);
					firstButton = (data[6] > 0);
					secondButton = (data[1] & 0x02);
					thirdButton = (data[1] & 0x04);
					hasContact = (data[1] & 0x40);
					// convert tilt (-128 ... 127)
//					int8 tiltDataX = ((data[7] & 0x3f) << 2) | ((data[8] & 0x80) >> 6);
					int8 tiltDataX = ((data[7] & 0x3f) << 1) | ((data[8] & 0x80) >> 7);
					int8 tiltDataY = data[8] & 0x7f;
//					int8 tiltDataY = 0;
					// convert to floats
					tiltX = (float)(tiltDataX - 64) / 64.0;
					tiltY = (float)(tiltDataY - 64) / 64.0;
				}
			}
			break;
		case DEVICE_PL500: {
			hasContact = ( data[1] & 0x20);
			xPos = data[2] << 8 | data[3];
			yPos = data[5] << 8 | data[6];
			firstButton = (data[4] & 0x08);
			secondButton = (data[4] & 0x10);
			thirdButton = (data[4] & 0x20);
			uint16 pressureData = (data[4] & 0x04) >> 2 | (data[7] & 0x7f) << 1;
			pressure = (float)pressureData / 511.0;
			break;
		}
		case DEVICE_VOLITO: {
			eraser = 0;
			thirdButton = 0;

			xPos = data[3] << 8 | data[2];
			yPos = data[5] << 8 | data[4];

			hasContact = (data[1] & 0x80);

			firstButton = (data[1] & 0x01) == 1;
			secondButton = data[1] & 0x04;

			uint16 pressureData = data[7] << 8 | data[6];
			pressure = (float)pressureData / 511.0;

			if (data[1] & 0x40) {	// mouse is on tablet
				wheelY = 0;
				mode = MODE_MOUSE;
				hasContact = (uint8)data[7] >= 30;
				pressure = 0.0;
				secondButton = data[1] & 0x02;
			}

			break;
		}
		case DEVICE_PENSTATION: {
			xPos = data[3] << 8 | data[2];
			yPos = data[5] << 8 | data[4];
			hasContact = (data[1] & 0x10);
			uint16 pressureData = data[7] << 8 | data[6];
			pressure = (float)pressureData / 511.0;
			firstButton = (data[1] & 0x01);
			secondButton = (data[1] & 0x02);
			thirdButton = (data[1] & 0x04);
			break;
		}
	}
	if (pressure > 1.0)
		pressure = 1.0;
	else if (pressure < 0.0)
		pressure = 0.0;
	buttons = (firstButton ? B_PRIMARY_MOUSE_BUTTON : 0)
			   | (secondButton ? B_SECONDARY_MOUSE_BUTTON : 0)
			   | (thirdButton ? B_TERTIARY_MOUSE_BUTTON : 0);
	x = (float)xPos;
	y = (float)yPos;
}

// SetStatus
void
TabletDevice::SetStatus(uint32 mode, uint32 buttons, float x, float y,
	float pressure, int32 clicks, uint32 modifiers, int32 eraser,
	float wheelX, float wheelY, float tiltX, float tiltY, const uchar* data)
{
	if (fActive) {
		uint32 what = B_MOUSE_MOVED;
		if (buttons > fButtons)
			what = B_MOUSE_DOWN;
		else if (buttons < fButtons)
			what = B_MOUSE_UP;


#if DEBUG
	float tabletX = x;
	float tabletY = y;
#endif
		x /= fMaxX;
		y /= fMaxY;

		float deltaX = 0.0;
		float deltaY = 0.0;

		float absDeltaX = 0.0;
		float absDeltaY = 0.0;

		float unfilteredX = x;
		float unfilteredY = y;

		if (fHasContact) {
			deltaX = x - fPosX;
			deltaY = y - fPosY;

			absDeltaX = fabsf(deltaX);
			absDeltaY = fabsf(deltaY);

#if 0 //DEBUG
fParent->LogString() << "x: " << x << ", y: " << y << ", pressure: " << pressure << "\n";
fParent->LogString() << "tilt x: " << tiltX << ", tilt y: " << tiltY << "\n\n";
#endif
			// apply a bit of filtering
			if (absDeltaX < fJitterX)
				x = fPosX;
			if (absDeltaY < fJitterY)
				y = fPosY;
		}

		// only do send message if something changed
		if (x != fPosX || y != fPosY || fButtons != buttons || pressure != fPressure
			|| fEraser != eraser || fTiltX != tiltX || fTiltY != tiltY) {

			bigtime_t now = system_time();

			// common fields for any mouse message
			BMessage* event = new BMessage(what);
			event->AddInt64("when", now);
			event->AddInt32("buttons", buttons);
			if (mode == MODE_PEN) {
				event->AddFloat("x", x);
				event->AddFloat("y", y);
				event->AddFloat("be:tablet_x", unfilteredX);
				event->AddFloat("be:tablet_y", unfilteredY);
				event->AddFloat("be:tablet_pressure", pressure);
				event->AddInt32("be:tablet_eraser", eraser);
				if (_DeviceSupportsTilt()) {
					event->AddFloat("be:tablet_tilt_x", tiltX);
					event->AddFloat("be:tablet_tilt_y", tiltY);
				}
				// adjust mouse coordinates as well
				// to have the mouse appear at the pens
				// last position when switching
				fFakeMouseX = unfilteredX;
				fFakeMouseY = unfilteredY;
			} else if (mode == MODE_MOUSE) {
				// apply acceleration
				float accelerationX = fJitterX * ACCELERATION_KICK_IN;
//				if (absDeltaX > accelerationX)
					deltaX *= absDeltaX / accelerationX;
				float accelerationY = fJitterY * ACCELERATION_KICK_IN;
//				if (absDeltaY > accelerationY)
					deltaY *= absDeltaY / accelerationY;
				// calculate screen coordinates
				fFakeMouseX = min_c(1.0, max_c(0.0, fFakeMouseX + deltaX));
				fFakeMouseY = min_c(1.0, max_c(0.0, fFakeMouseY + deltaY));
				event->AddFloat("x", fFakeMouseX);
				event->AddFloat("y", fFakeMouseY);
				event->AddFloat("be:tablet_x", fFakeMouseX);
				event->AddFloat("be:tablet_y", fFakeMouseY);
			}
			event->AddInt32("modifiers", modifiers);

#if DEBUG
if (data) {
	event->AddData("raw usb data", B_RAW_TYPE, data, 12);
}
event->AddFloat("tablet x", tabletX);
event->AddFloat("tablet y", tabletY);
#endif
			// additional fields for mouse down or up
			if (what == B_MOUSE_DOWN) {
				if (now - fLastClickTime < fParent->DoubleClickSpeed()) {
					fClicks++;
					if (fClicks > 3)
						fClicks = 1;
				} else {
					fClicks = 1;
				}
				fLastClickTime = now;
				event->AddInt32("clicks", fClicks);
			} else if (what == B_MOUSE_UP)
				event->AddInt32("clicks", 0);

			status_t ret = fParent->EnqueueMessage(event);
			if (ret < B_OK)
				PRINT(("EnqueueMessage(): %s\n", strerror(ret)));

			// apply values to members
			fPosX = x;
			fPosY = y;
			fButtons = buttons;
			fPressure = pressure;
			fModifiers = modifiers;
			fEraser = eraser;
			fTiltX = tiltX;
			fTiltY = tiltY;
		}

		// separate wheel changed message
		if (fWheelX != wheelX || fWheelY != wheelY) {
			BMessage* event = new BMessage(B_MOUSE_WHEEL_CHANGED);
			event->AddInt64("when", system_time());
			event->AddFloat("be:wheel_delta_x", wheelX);
			event->AddFloat("be:wheel_delta_y", wheelY);
			fParent->EnqueueMessage(event);

			// apply values to members
			fWheelX = wheelX;
			fWheelY = wheelY;
		}
	}
}

// SetContact
void
TabletDevice::SetContact(bool contact)
{
	fHasContact = contact;
}

// poll_usb_device
int32
TabletDevice::poll_usb_device(void* arg)
{
	TabletDevice* tabletDevice = (TabletDevice*)arg;
	DeviceReader* reader = tabletDevice->fReader;

	if (!reader || reader->InitCheck() < B_OK)
		return B_BAD_VALUE;

	int dataBytes = reader->MaxPacketSize();
	if (dataBytes > 128)
		return B_BAD_VALUE;

	uchar data[max_c(12, dataBytes)];

	while (tabletDevice->IsActive()) {

		status_t ret = reader->ReadData(data, dataBytes);

		if (ret == dataBytes) {
			// data we read from the wacom device
			uint32 mode;
			bool hasContact = false;
			uint32 buttons = 0;
			float x = 0.0;
			float y = 0.0;
			float pressure = 0.0;
			int32 clicks = 0;
			int32 eraser = 0;
			float wheelX = 0.0;
			float wheelY = 0.0;
			float tiltX = 0.0;
			float tiltY = 0.0;
			// let the device extract all information from the data
			tabletDevice->ReadData(data, dataBytes, hasContact, mode, buttons,
								   x, y, pressure, clicks, eraser,
								   wheelX, wheelY, tiltX, tiltY);
			if (hasContact) {
				// apply the changes to the device
				tabletDevice->SetStatus(mode, buttons, x, y, pressure,
										clicks, modifiers(), eraser,
										wheelX, wheelY, tiltX, tiltY, data);
			} else
				PRINT(("device has no contact\n"));
			tabletDevice->SetContact(hasContact);
		} else {
			PRINT(("failed to read %ld bytes, read: %ld or %s\n",
				dataBytes, ret, strerror(ret)));

			if (ret < B_OK) {
				if (ret == B_TIMED_OUT)
					snooze(SNOOZE_AMOUNT);
				else if (ret == B_INTERRUPTED)
					snooze(SNOOZE_AMOUNT);
				else {
					return ret;
				}
			}
		}
	}

	return B_OK;
}

// _DeviceSupportsTilt
bool
TabletDevice::_DeviceSupportsTilt() const
{
	bool tilt = false;
	switch (fDeviceMode) {
		case DEVICE_INTUOS:
		case DEVICE_INTUOS3:
		case DEVICE_CINTIQ:
			tilt = true;
			break;
	}
	return tilt;
}

// _GetName
void
TabletDevice::_GetName(uint16 productID, const char** name) const
{
	switch (productID) {
		case 0x00:
			*name = "Wacom USB";
			break;
		case 0x03:	// driver does not support this yet
			*name = "Wacom Cintiq Partner USB";
			break;
		case 0x10:
			*name = "Wacom Graphire USB";
			break;
		case 0x11:
			*name = "Wacom Graphire2 4x5\" USB";
			break;
		case 0x12:
			*name = "Wacom Graphire2 5x7\" USB";
			break;
		case 0x13:
			*name = "Wacom Graphire3 4x5\" USB";
			break;
		case 0x14:
			*name = "Wacom Graphire3 6x8\" USB";
			break;
		case 0x15:
			*name = "Wacom Graphire4 4x5\" USB";
			break;
		case 0x16:
			*name = "Wacom Graphire4 6x8\" USB";
			break;
		case 0x17:
			*name = "Wacom BambooFun 4x5\" USB";
			break;
		case 0x18:
			*name = "Wacom BambooFun 6x8\" USB";
			break;
		case 0x20:
			*name = "Wacom Intuos 4x5\" USB";
			break;
		case 0x21:
			*name = "Wacom Intuos 6x8\" USB";
			break;
		case 0x22:
			*name = "Wacom Intuos 9x12\" USB";
			break;
		case 0x23:
			*name = "Wacom Intuos 12x12\" USB";
			break;
		case 0x24:
			*name = "Wacom Intuos 12x18\" USB";
			break;
		case 0x30:
			*name = "Wacom PL400 USB";
			break;
		case 0x31:
			*name = "Wacom PL500 USB";
			break;
		case 0x32:
			*name = "Wacom PL600 USB";
			break;
		case 0x33:
			*name = "Wacom PL600SX USB";
			break;
		case 0x34:
			*name = "Wacom PL550 USB";
			break;
		case 0x35:
			*name = "Wacom PL800 USB";
			break;

		case 0x3F:
			*name = "Wacom Cintiq 21UX USB";
			break;

		case 0x41:
			*name = "Wacom Intuos2 4x5\" USB";
			break;
		case 0x42:
			*name = "Wacom Intuos2 6x8\" USB";
			break;
		case 0x43:
			*name = "Wacom Intuos2 9x12\" USB";
			break;
		case 0x44:
			*name = "Wacom Intuos2 12x12\" USB";
			break;
		case 0x45:
			*name = "Wacom Intuos2 12x18\" USB";
			break;
		case 0x47:	// some I2 6x8s seem to report as 0x47
			*name = "Wacom Intuos2 6x8\" USB";
			break;

		case 0x60:
			*name = "Wacom Volito USB";
			break;
		case 0x61:
			*name = "Wacom PenStation USB";
			break;
		case 0x62:
			*name = "Wacom Volito2 USB";
			break;
		case 0x64:
			*name = "Wacom PenPartner.1 USB";
			break;
		case 0x65:
			*name = "Wacom Bamboo USB";
			break;
		case 0x69:
			*name = "Wacom Bamboo1 USB";
			break;

		case 0xB0:
			*name = "Wacom Intuos3 4x5 USB";
			break;
		case 0xB1:
			*name = "Wacom Intuos3 6x8 USB";
			break;
		case 0xB2:
			*name = "Wacom Intuos3 9x12 USB";
			break;

		case 0xD0:
			*name = "Wacom Bamboo 2FG USB";
			break;
		case 0xD1:
			*name = "Wacom BambooFun 2FG 4x5\" USB";
			break;
		case 0xD2:
			*name = "Wacom Bamboo Craft USB";
			break;
		case 0xD3:
			*name = "Wacom BambooFun 2FG 6x8\" USB";
			break;
		case 0xD4:
			*name = "Wacom Bamboo 4x5\" USB";
			break;
		case 0xD6:
			*name = "Wacom Bamboo (CTH-460/K)";
			break;
		case 0xD7:
			*name = "Wacom Bamboo (CTH-461/S)";
			break;
		case 0xD8:
			*name = "Wacom Bamboo (CTH-661/S1)";
			break;
		case 0xDA:
			*name = "Wacom Bamboo (CTH-461/L)";
			break;
		case 0xDB:
			*name = "Wacom Bamboo (CTH-661)";
			break;

		default:
			*name = "<unkown wacom tablet>";
			break;
	}
}

