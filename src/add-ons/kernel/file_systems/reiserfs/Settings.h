// Settings.h
//
// Copyright (c) 2003, Ingo Weinhold (bonefish@cs.tu-berlin.de)
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can alternatively use *this file* under the terms of the the MIT
// license included in this package.

#ifndef SETTINGS_H
#define SETTINGS_H

#include <driver_settings.h>

#include "List.h"
#include "String.h"

class Settings {
public:
	Settings();
	~Settings();

	status_t SetTo(const char *volumeName = NULL);
	status_t SetTo(off_t volumeOffset, off_t volumeSize);
	void Unset();

	const char *GetDefaultVolumeName() const;
	const char *GetVolumeName() const;
	bool GetHideEsoteric() const;
	const char *HiddenEntryAt(int32 index) const;

	void Dump();

private:
	status_t _Init(const driver_settings *settings,
				   const driver_parameter *volume);
	static const driver_parameter *_FindVolumeParameter(
		const driver_settings *settings, const char *name);
	static const driver_parameter *_FindVolumeParameter(
		const driver_settings *settings, off_t offset, off_t size);

	template<typename container_t>
	static const driver_parameter *_FindNextParameter(
		const container_t *container, const char *name, int32 &cookie);

	template<typename container_t>
	static const char *_GetParameterValue(const container_t *container,
		const char *name, const char *unknownValue, const char *noArgValue);

	template<typename container_t>
	static bool _GetParameterValue(const container_t *container,
		const char *name, bool unknownValue, bool noArgValue);

	template<typename container_t>
	static int64 _GetParameterValue(const container_t *container,
		const char *name, int64 unknownValue, int64 noArgValue);

	static void _CheckVolumeName(String &name);
	static bool _CheckEntryName(const char *name);

private:
	String			fDefaultVolumeName;
	String			fVolumeName;
	bool			fHideEsoteric;
	List<String>	fHiddenEntries;
};

// _FindNextParameter
template<typename container_t>
const driver_parameter *
Settings::_FindNextParameter(const container_t *container,
							 const char *name, int32 &cookie)
{
	const driver_parameter *parameter = NULL;
	if (container) {
		for (; !parameter && cookie < container->parameter_count; cookie++) {
			const driver_parameter &param = container->parameters[cookie];
			if (!strcmp(param.name, name))
				parameter = &param;
		}
	}
	return parameter;
}

// _GetParameterValue
template<typename container_t>
const char *
Settings::_GetParameterValue(const container_t *container, const char *name,
							 const char *unknownValue, const char *noArgValue)
{
	if (container) {
		for (int32 i = container->parameter_count - 1; i >= 0; i--) {
			const driver_parameter &param = container->parameters[i];
			if (!strcmp(param.name, name)) {
				if (param.value_count > 0)
					return param.values[0];
				return noArgValue;
			}
		}
	}
	return unknownValue;
}

// contains
static inline
bool
contains(const char **array, size_t size, const char *value)
{
	for (int32 i = 0; i < (int32)size; i++) {
		if (!strcmp(array[i], value))
			return true;
	}
	return false;
}

// _GetParameterValue
template<typename container_t>
bool
Settings::_GetParameterValue(const container_t *container, const char *name,
							 bool unknownValue, bool noArgValue)
{
	// note: container may be NULL
	const char unknown = 0;
	const char noArg = 0;
	const char *value = _GetParameterValue(container, name, &unknown, &noArg);
	if (value == &unknown)
		return unknownValue;
	if (value == &noArg)
		return noArgValue;
	const char *trueStrings[]
		= { "1", "true", "yes", "on", "enable", "enabled" };
	const char *falseStrings[]
		= { "0", "false", "no", "off", "disable", "disabled" };
	if (contains(trueStrings, sizeof(trueStrings) / sizeof(const char*),
				 value)) {
		return true;
	}
	if (contains(falseStrings, sizeof(falseStrings) / sizeof(const char*),
				 value)) {
		return false;
	}
	return unknownValue;
}

// _GetParameterValue
template<typename container_t>
int64
Settings::_GetParameterValue(const container_t *container, const char *name,
							 int64 unknownValue, int64 noArgValue)
{
	// note: container may be NULL
	const char unknown = 0;
	const char noArg = 0;
	const char *value = _GetParameterValue(container, name, &unknown, &noArg);
	if (value == &unknown)
		return unknownValue;
	if (value == &noArg)
		return noArgValue;
	return atoll(value);
}

#endif	// SETTINGS_H
