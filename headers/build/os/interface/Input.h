/******************************************************************************
/
/	File:			Input.h
/
/	Description:	Functions and class to manage input devices.
/
/	Copyright 1998, Be Incorporated, All Rights Reserved.
/
******************************************************************************/

#ifndef _INPUT_H
#define _INPUT_H

#include <BeBuild.h>
#include <Messenger.h>
#include <SupportDefs.h>


enum input_method_op {
	B_INPUT_METHOD_STARTED			= 0,
	B_INPUT_METHOD_STOPPED			= 1,
	B_INPUT_METHOD_CHANGED	 		= 2,
	B_INPUT_METHOD_LOCATION_REQUEST	= 3
};


enum input_device_type {
	B_POINTING_DEVICE	= 0,
	B_KEYBOARD_DEVICE	= 1,
	B_UNDEFINED_DEVICE	= 2
};

/*	
	what == B_INPUT_DEVICES_CHANGED
	FindString("name", name_of_device);
	FindInt32("opcode", input_device_notification);
*/

enum input_device_notification {
	B_INPUT_DEVICE_ADDED 	= 0x0001,
	B_INPUT_DEVICE_STARTED	= 0x0002,
	B_INPUT_DEVICE_STOPPED	= 0x0004,
	B_INPUT_DEVICE_REMOVED	= 0x0008
};


class BInputDevice;


_IMPEXP_BE BInputDevice*	find_input_device(const char *name);
_IMPEXP_BE status_t			get_input_devices(BList *list);
_IMPEXP_BE status_t			watch_input_devices(BMessenger target, bool start);


class BInputDevice {
public:
							~BInputDevice();

	const char*				Name() const;
	input_device_type		Type() const;
	bool					IsRunning() const;

	status_t				Start();
	status_t				Stop();
	status_t				Control(uint32		code, 
									BMessage	*message);

	static status_t			Start(input_device_type type);
	static status_t			Stop(input_device_type type);
	static status_t			Control(input_device_type	type, 
									uint32 				code, 
									BMessage			*message);

private:
	friend BInputDevice*	find_input_device(const char *name);
	friend status_t			get_input_devices(BList *list);

							BInputDevice();
	void					set_name_and_type(const char		*name,
											  input_device_type	type);

	char*					fName;
	input_device_type		fType;
	uint32					_reserved[4];
};


#endif
