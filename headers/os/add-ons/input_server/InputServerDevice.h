/******************************************************************************
/
/	File:			InputServerDevice.h
/
/	Description:	Add-on class for input_server devices.
/
/	Copyright 1998, Be Incorporated, All Rights Reserved.
/
******************************************************************************/

#ifndef _INPUTSERVERDEVICE_H
#define _INPUTSERVERDEVICE_H

#include <BeBuild.h>
#include <Input.h>
#include <SupportDefs.h>


struct input_device_ref {
	char				*name;
	input_device_type	type;
	void				*cookie;
};


enum {
	// B_KEYBOARD_DEVICE notifications
	B_KEY_MAP_CHANGED = 1,
	B_KEY_LOCKS_CHANGED,
	B_KEY_REPEAT_DELAY_CHANGED,	
	B_KEY_REPEAT_RATE_CHANGED,

	// B_POINTING_DEVICE notifications
	B_MOUSE_TYPE_CHANGED,	
	B_MOUSE_MAP_CHANGED,
	B_MOUSE_SPEED_CHANGED,
	B_CLICK_SPEED_CHANGED,
	B_MOUSE_ACCELERATION_CHANGED
};


class _BDeviceAddOn_;


class BInputServerDevice {
public:
						BInputServerDevice();
	virtual				~BInputServerDevice();

	virtual status_t	InitCheck();
	virtual status_t	SystemShuttingDown();

	virtual status_t	Start(const char *device, void *cookie);
	virtual	status_t	Stop(const char *device, void *cookie);
	virtual status_t	Control(const char	*device, 
								void		*cookie, 
								uint32		code, 
								BMessage	*message);

	status_t			RegisterDevices(input_device_ref **devices);
	status_t			UnregisterDevices(input_device_ref **devices);

	status_t			EnqueueMessage(BMessage *message);

	status_t			StartMonitoringDevice(const char *device);
	status_t			StopMonitoringDevice(const char *device);	

private:
	_BDeviceAddOn_*		fOwner;

	virtual void		_ReservedInputServerDevice1();
	virtual void		_ReservedInputServerDevice2();
	virtual void		_ReservedInputServerDevice3();
	virtual void		_ReservedInputServerDevice4();
	uint32				_reserved[4];
};


#endif
