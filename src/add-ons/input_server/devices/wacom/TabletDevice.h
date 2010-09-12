/*
 * Copyright 2003-2008 Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT license.
 */

// This class encapsulates all info and status about a tablet.
// It runs the thread polling from the USB and sends messages
// to/via the InputServer.

#ifndef TABLET_DEVICE_H
#define TABLET_DEVICE_H

#include <OS.h>

#include "PointingDevice.h"

class TabletDevice : public PointingDevice {
 public:
								TabletDevice(MasterServerDevice* parent,
									DeviceReader* reader);
	virtual						~TabletDevice();

	virtual	status_t			InitCheck();

	virtual	status_t			Start();
	virtual	status_t			Stop();

			status_t			DetectDevice(const DeviceReader* reader);

			void				SetDevice(float maxX, float maxY,
									uint32 mode = DEVICE_INTUOS);

			void				ReadData(const uchar* data,
									int dataBytes,
									bool& hasContact,
									uint32& mode,
									uint32& buttons,
									float& x, float& y,
									float& pressure,
									int32& clicks,
									int32& eraser,
									float& wheelX,
									float& wheelY,
									float& tiltX,
									float& tiltY) const;

			void				SetStatus(uint32 mode,
									uint32 buttons,
									float x, float y,
									float pressure,
									int32 clicks,
									uint32 modifiers,
									int32 eraser,
									float wheelX,
									float wheelY,
									float tiltX = 0.0,
									float tiltY = 0.0,
									const uchar* data = NULL);

			void				SetContact(bool contact);

 private:
	static int32				poll_usb_device(void* arg);

			bool				_DeviceSupportsTilt() const;
			void				_GetName(uint16 productID,
									const char** name) const;

	enum {
		DEVICE_UNKOWN,
		DEVICE_PENPARTNER,
		DEVICE_GRAPHIRE,
		DEVICE_INTUOS,
		DEVICE_INTUOS3,
		DEVICE_PL500,
		DEVICE_VOLITO,
		DEVICE_PENSTATION,
		DEVICE_CINTIQ,
		DEVICE_BAMBOO,
		DEVICE_BAMBOO_PT,
	};

	enum {
		MODE_PEN,
		MODE_MOUSE,
	};

			thread_id			fThreadID;

			// device specific settings
			uint32				fDeviceMode;
			float				fMaxX;
			float				fMaxY;
			int					fDataBytes;

			float				fPosX;
			float				fPosY;
			float				fFakeMouseX;
			float				fFakeMouseY;
			uint32				fButtons;
			float				fPressure;
			uint32				fModifiers;
			bool				fEraser;
			float				fTiltX;
			float				fTiltY;
			float				fWheelX;
			float				fWheelY;
			bigtime_t			fLastClickTime;
			int32				fClicks;
			bool				fHasContact;

			float				fJitterX;
			float				fJitterY;
};

#endif	// WACOM_TABLET_H
