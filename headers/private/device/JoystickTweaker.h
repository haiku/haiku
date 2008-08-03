/*
 * Copyright 2008, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Fredrik Modeen 
 *
 */
 
#include "joystick_driver.h"
 
#include <List.h>
#include <Entry.h>
#include <Path.h>
#include <Directory.h>
#include <File.h>

#include <String.h>
#include <Debug.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>

#define DEBUG

#define DEVICEPATH "/dev/joystick/"
#define JOYSTICKPATH "/boot/home/config/settings/joysticks/"

class BJoystick;

struct _joystick_info;

class _BJoystickTweaker {

public:
					_BJoystickTweaker();
					_BJoystickTweaker(BJoystick &stick);
virtual				~_BJoystickTweaker();
		status_t	SendIOCT(uint32 op);
		void		scan_including_disabled();
		status_t	get_info(_joystick_info* info, const char * ref);
protected:
private:
		void 		BuildFromJoystickDesc(char *string, _joystick_info* info);
		status_t	scan_including_disabled(const char* rootPath, BList *list, BEntry *rootEntry = NULL);
		status_t	save_config(const entry_ref * ref = NULL);
		status_t	get_info();
		BJoystick* fJoystick;
#ifdef DEBUG
public:
	static FILE *sLogFile;
#endif		
};
