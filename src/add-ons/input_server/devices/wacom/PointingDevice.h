/*
 * Copyright 2005-2008 Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT license.
 */
#ifndef POINTING_DEVICE_H
#define POINTING_DEVICE_H

#include <SupportDefs.h>

class DeviceReader;
class MasterServerDevice;

class PointingDevice {
 public:
								PointingDevice(MasterServerDevice* parent,
											   DeviceReader* reader);
	virtual						~PointingDevice();
	
	virtual status_t			InitCheck();
	
	virtual	status_t			Start() = 0;
	virtual	status_t			Stop() = 0;

	virtual	void				SetActive(bool active);
			bool				IsActive() const;

								// forwards the device path of the reader
			const char*			DevicePath() const;

								// hook function to determine if
								// PS/2 Mouse thread should be disabled
	virtual	bool				DisablePS2() const;

			// query the device for information
			uint16				VendorID() const;
			uint16				ProductID() const;

 protected:
	MasterServerDevice*			fParent;
	DeviceReader*				fReader;

	volatile bool				fActive;
};

#endif // POINTING_DEVICE_H
