/*
 * Copyright 2008, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Fredrik Modeen
 */

#include "joystick_driver.h"

#include <List.h>
#include <Entry.h>

#define DEVICEPATH "/dev/joystick/"
#define JOYSTICKPATH "/boot/home/config/settings/joysticks/"

class BJoystick;

struct _joystick_info;

class _BJoystickTweaker {

public:
					_BJoystickTweaker();
					_BJoystickTweaker(BJoystick &stick);
		virtual		~_BJoystickTweaker();
		status_t	SendIOCT(uint32 op);
		status_t	GetInfo(_joystick_info* info, const char * ref);
		
		// BeOS R5's joystick pref need these
		status_t	save_config(const entry_ref * ref = NULL);
		void		scan_including_disabled();
		status_t	get_info();
		
private:
		void 		_BuildFromJoystickDesc(char *string, _joystick_info* info);
		status_t	_ScanIncludingDisabled(const char* rootPath, BList *list, 
						BEntry *rootEntry = NULL);
						
		void		_EmpyList(BList *list);		
		BJoystick* 	fJoystick;
#if DEBUG
public:
	static FILE *sLogFile;
#endif
};
