/*
 * Copyright 2002-2008, Marcus Overhagen, Stefano Ceccherini, Fredrik Modéen.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <Joystick.h>
#include <List.h>
#include <Entry.h>
#include <Path.h>
#include <Directory.h>

#include <String.h>
#include <Debug.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>

#define DEVICEPATH "/dev/joystick/"

typedef struct _joystick_info {
	char			module_name[64];
	char			device_name[64];
	int16			num_axes;
	int16			num_buttons;
	int16			num_hats;
	uint16			num_sticks;
} joystick_info;

//Scans a directory and adds the entries it founds as strings to the given list
static status_t
scan_directory(const char* rootPath, BList *list, BEntry *rootEntry = NULL)
{	
	BDirectory root;

	if (rootEntry != NULL)
		root.SetTo( rootEntry);
	else if (rootPath != NULL)
		root.SetTo(rootPath);
	else
		return B_ERROR;
		
	BEntry entry;
	
	ASSERT(list != NULL);
	while ((root.GetNextEntry(&entry)) > B_ERROR ) {
		if (entry.IsDirectory()) {
			scan_directory(rootPath, list, &entry);
		} else {
			BPath path;
			entry.GetPath(&path);
			BString str(path.Path());
			str.RemoveFirst(rootPath);
			list->AddItem(strdup(str.String()));
		}
	}
	return B_OK;
}

class _BJoystickTweaker {

public:
					_BJoystickTweaker();
					_BJoystickTweaker(BJoystick &stick);
virtual				~_BJoystickTweaker();

protected:
private:
		status_t	scan_including_disabled();
		status_t	save_config(const entry_ref * ref = NULL);
		status_t	get_info();
		BJoystick* fJoystick;
};
//-------------------------------------------------------------------//


BJoystick::BJoystick()
	: _mBeBoxMode(false)
	, _fDevices(new BList)
{
	ScanDevices();
}


BJoystick::~BJoystick()
{
	if (ffd >= 0)
		close(ffd);
	
	for (int32 count = _fDevices->CountItems() - 1; count >= 0; count--)
		free(_fDevices->RemoveItem(count));
	
	delete _fDevices;
}


status_t
BJoystick::Open(const char *portName)
{
	return Open(portName, true);
}


status_t
BJoystick::Open(const char *portName,
				bool enter_enhanced)
{
	char buf[64];
	
	//We don't want to use enhanced mode but _mBeBoxMode sugest if we use 
	//BeBoxMode
	if(!enter_enhanced)
		_mBeBoxMode = !enter_enhanced;
	
	if (portName == NULL)
		return B_BAD_VALUE; // Heheee, we won't crash
		
	if (portName[0] != '/') 
		snprintf(buf, 64, DEVICEPATH"/%s", portName);
	else
		// A name like "/dev/joystick/usb/0" was passed
		snprintf(buf, 64, "%s", portName);
		
	if (ffd >= 0) //If this port is already open, close it
		close(ffd);
 
	// TODO: BeOS don't use O_EXCL, and this seems to lead to some issues. I
	// added this flag having read some comments by Marco Nelissen on the 
	// annotated BeBook. I think BeOS uses O_RDWR|O_NONBLOCK here.
	ffd = open(buf, O_RDWR | O_NONBLOCK | O_EXCL); 
	
	if (ffd >= 0) {
		// we used open() with O_NONBLOCK flag to let it return immediately, 
		// but we want read/write operations to block if needed, so we clear 
		// that bit here.
		int flags = fcntl(ffd, F_GETFL);
		fcntl(ffd, F_SETFL, flags & ~O_NONBLOCK);
				
		//DriverControl();
	}
	// TODO: I wonder why the return type is a status_t, 
	// since we (as BeOS does) return the descriptor number for the device... 
	return (ffd >= 0) ? ffd : errno;
}


void
BJoystick::Close(void)
{
	if (ffd >= 0)
		close(ffd);
	ffd = -1;
}


void
BJoystick::ScanDevices(bool use_disabled)
{
	// First, we empty the list
	for (int32 count = _fDevices->CountItems() - 1; count >= 0; count--)
		free(_fDevices->RemoveItem(count));
	
	// Add devices to the list
	scan_directory(DEVICEPATH, _fDevices);		
}


int32
BJoystick::CountDevices()
{
	int32 count = 0;
	
	// Refresh devices list
	ScanDevices();
	
	if (_fDevices != NULL)
		count = _fDevices->CountItems();
	
	return count;
}


status_t
BJoystick::GetDeviceName(int32 n, char *name, size_t bufSize)
{
	status_t result = B_ERROR;
	const char *dev = NULL;
	if (_fDevices != NULL)
		dev = static_cast<char*>(_fDevices->ItemAt(n));

	if (dev != NULL && name != NULL) {
		strncpy(name, dev, bufSize);
		name[bufSize - 1] = '\0';
		result = B_OK;
	}
	return result;
}


status_t
BJoystick::Update(void)
{
	return B_ERROR;
}


status_t
BJoystick::SetMaxLatency(bigtime_t max_latency)
{
	return B_ERROR;
}


bool
BJoystick::EnterEnhancedMode(const entry_ref *ref)
{
	_mBeBoxMode = false;
	return !_mBeBoxMode;
}


int32
BJoystick::CountSticks()
{
	return 0;
}


int32
BJoystick::CountAxes()
{
	return 0;
}


int32
BJoystick::CountHats()
{
	return 0;
}


int32
BJoystick::CountButtons()
{
	return 0;
}


status_t
BJoystick::GetAxisValues(int16 *out_values,
						 int32 for_stick)
{
	return B_ERROR;
}


status_t
BJoystick::GetHatValues(uint8 *out_hats,
						int32 for_stick)
{
	return B_ERROR;
}


uint32
BJoystick::ButtonValues(int32 for_stick)
{
	return 0;
}


status_t
BJoystick::GetAxisNameAt(int32 index,
						 BString *out_name)
{
	return B_ERROR;
}


status_t
BJoystick::GetHatNameAt(int32 index,
						BString *out_name)
{
	return B_ERROR;
}


status_t
BJoystick::GetButtonNameAt(int32 index,
						   BString *out_name)
{
	return B_ERROR;
}


status_t
BJoystick::GetControllerModule(BString *out_name)
{
	return B_ERROR;
}


status_t
BJoystick::GetControllerName(BString *out_name)
{
	return B_ERROR;
}


bool
BJoystick::IsCalibrationEnabled()
{
	return false;
}


status_t
BJoystick::EnableCalibration(bool calibrates)
{
	return false;
}


void
BJoystick::Calibrate(struct _extended_joystick *reading)
{
}


status_t
BJoystick::gather_enhanced_info(const entry_ref *ref)
{
	return B_ERROR;
}


status_t
BJoystick::save_config(const entry_ref *ref)
{
	return B_ERROR;
}


/* These functions are here to maintain Binary Compatibility */ 
void BJoystick::_ReservedJoystick1() {}
void BJoystick::_ReservedJoystick2() {}
void BJoystick::_ReservedJoystick3() {}
status_t BJoystick::_Reserved_Joystick_4(void *, ...) {return B_ERROR;}
status_t BJoystick::_Reserved_Joystick_5(void *, ...) {return B_ERROR;}
status_t BJoystick::_Reserved_Joystick_6(void *, ...) {return B_ERROR;}


//-------------------------------------------------------------------//


_BJoystickTweaker::_BJoystickTweaker()
{
}


_BJoystickTweaker::_BJoystickTweaker(BJoystick &stick)
{
	fJoystick = &stick;
}


_BJoystickTweaker::~_BJoystickTweaker()
{
}


status_t
_BJoystickTweaker::save_config(const entry_ref *ref)
{
	return B_ERROR;
}


status_t
_BJoystickTweaker::scan_including_disabled()
{
	return B_ERROR;
}


status_t
_BJoystickTweaker::get_info()
{
	return B_ERROR;
}
