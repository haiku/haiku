/*
 * Copyright 2008, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Fredrik Modeen
 *
 */
#include "JoystickTweaker.h"

#include <new>
#include <stdio.h>
#include <stdlib.h>

#include <Debug.h>
#include <Directory.h>
#include <Joystick.h>
#include <Path.h>
#include <String.h>
#include <UTF8.h>


#define STACK_STRING_BUFFER_SIZE	64


#if DEBUG
inline void
LOG(const char *fmt, ...)
{
	char buf[1024];
	va_list ap;
	va_start(ap, fmt);
	vsprintf(buf, fmt, ap);
	va_end(ap);

	fputs(buf, _BJoystickTweaker::sLogFile);
	fflush(_BJoystickTweaker::sLogFile);
}
#	define LOG_ERR(text...) LOG(text)
FILE *_BJoystickTweaker::sLogFile = NULL;
#else
#	define LOG(text...)
#	define LOG_ERR(text...) fprintf(stderr, text)
#endif

#define CALLED() LOG("%s\n", __PRETTY_FUNCTION__)


_BJoystickTweaker::_BJoystickTweaker()
{
#if DEBUG
	sLogFile = fopen("/var/log/joystick.log", "a");
#endif
	CALLED();
}


_BJoystickTweaker::_BJoystickTweaker(BJoystick &stick)
{
#if DEBUG
	sLogFile = fopen("/var/log/joystick.log", "a");
#endif
	CALLED();

	fJoystick = &stick;
}


_BJoystickTweaker::~_BJoystickTweaker()
{
}


status_t
_BJoystickTweaker::save_config(const entry_ref *ref)
{
	CALLED();
	return B_ERROR;
}


status_t
_BJoystickTweaker::_ScanIncludingDisabled(const char *rootPath, BList *list,
	BEntry *rootEntry)
{
	BDirectory root;

	if (rootEntry != NULL)
		root.SetTo(rootEntry);
	else if (rootPath != NULL)
		root.SetTo(rootPath);
	else
		return B_ERROR;

	BEntry entry;

	ASSERT(list != NULL);
	while (root.GetNextEntry(&entry) == B_OK) {
		if (entry.IsDirectory()) {
			status_t result = _ScanIncludingDisabled(rootPath, list, &entry);
			if (result != B_OK)
				return result;

			continue;
		}

		BPath path;
		status_t result = entry.GetPath(&path);
		if (result != B_OK)
			return result;

		BString *deviceName = new(std::nothrow) BString(path.Path());
		if (deviceName == NULL)
			return B_NO_MEMORY;

		deviceName->RemoveFirst(rootPath);
		if (!list->AddItem(deviceName)) {
			delete deviceName;
			return B_ERROR;
		}
	}

	return B_OK;
}


void
_BJoystickTweaker::scan_including_disabled()
{
	CALLED();
	_EmpyList(fJoystick->fDevices);
	_ScanIncludingDisabled(DEVICE_BASE_PATH, fJoystick->fDevices);
}


void
_BJoystickTweaker::_EmpyList(BList *list)
{
	for (int32 i = 0; i < list->CountItems(); i++)
		delete (BString *)list->ItemAt(i);

	list->MakeEmpty();
}


status_t
_BJoystickTweaker::get_info()
{
	CALLED();
	return B_ERROR;
}


status_t
_BJoystickTweaker::GetInfo(_joystick_info *info, const char *ref)
{
	CALLED();
	BString configFilePath(JOYSTICK_CONFIG_BASE_PATH);
	configFilePath.Append(ref);

	FILE *file = fopen(configFilePath.String(), "r");
	if (file == NULL)
		return B_ERROR;

	char line[STACK_STRING_BUFFER_SIZE];
	while (fgets(line, sizeof(line), file) != NULL) {
		int length = strlen(line);
		if (length > 0 && line[length - 1] == '\n')
			line[length - 1] = '\0';

		_BuildFromJoystickDesc(line, info);
	}

	fclose(file);
	return B_OK;
}


void
_BJoystickTweaker::_BuildFromJoystickDesc(char *string, _joystick_info *info)
{
	BString str(string);
	str.RemoveAll("\"");

	if (str.IFindFirst("module") != -1) {
		str.RemoveFirst("module = ");
		strlcpy(info->module_info.module_name, str.String(),
			STACK_STRING_BUFFER_SIZE);
	} else if (str.IFindFirst("gadget") != -1) {
		str.RemoveFirst("gadget = ");
		strlcpy(info->module_info.device_name, str.String(),
			STACK_STRING_BUFFER_SIZE);
	} else if (str.IFindFirst("num_axes") != -1) {
		str.RemoveFirst("num_axes = ");
		info->module_info.num_axes = atoi(str.String());
	} else if (str.IFindFirst("num_hats") != -1) {
		str.RemoveFirst("num_hats = ");
		info->module_info.num_hats = atoi(str.String());
	} else if (str.IFindFirst("num_buttons") != -1) {
		str.RemoveFirst("num_buttons = ");
		info->module_info.num_buttons = atoi(str.String());
	} else if (str.IFindFirst("num_sticks") != -1) {
		str.RemoveFirst("num_sticks = ");
		info->module_info.num_sticks = atoi(str.String());
	} else {
		LOG("Path = %s\n", str.String());
	}
}


status_t
_BJoystickTweaker::SendIOCT(uint32 op)
{
	switch (op) {
		case B_JOYSTICK_SET_DEVICE_MODULE:
				break;

		case B_JOYSTICK_GET_DEVICE_MODULE:
				break;

		case B_JOYSTICK_GET_SPEED_COMPENSATION:
		case B_JOYSTICK_SET_SPEED_COMPENSATION:
		case B_JOYSTICK_GET_MAX_LATENCY:
		case B_JOYSTICK_SET_MAX_LATENCY:
		case B_JOYSTICK_SET_RAW_MODE:
		default:
				break;
	}

	return B_ERROR;
}
