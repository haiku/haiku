// Settings.cpp
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

#include <new>

#include "Settings.h"
#include "Debug.h"

using std::nothrow;

/*!
	\class Settings
	\brief Manages the ReiserFS settings.
*/

static const char *kFSName = "reiserfs";

// defaults
static const char *kDefaultDefaultVolumeName = "ReiserFS untitled";
static const bool kDefaultHideEsoteric	= true;

// constructor
Settings::Settings()
	:
	fDefaultVolumeName(),
	fVolumeName(),
	fHideEsoteric(kDefaultHideEsoteric),
	fHiddenEntries(5)
{
}

// destructor
Settings::~Settings()
{
	Unset();
}

// SetTo
status_t
Settings::SetTo(const char *volumeName)
{
	// unset
	Unset();
	// load the driver settings and find the entry for the volume
	void *settings = load_driver_settings(kFSName);
	const driver_parameter *volume = NULL;
	const driver_settings *ds = get_driver_settings(settings);
	if (ds && volumeName)
		volume = _FindVolumeParameter(ds, volumeName);
	// init the object and unload the settings
	_Init(ds, volume);
	unload_driver_settings(settings);
	return B_OK;
}

// SetTo
status_t
Settings::SetTo(off_t volumeOffset, off_t volumeSize)
{
	PRINT(("Settings::SetTo(%" B_PRIdOFF ", %" B_PRIdOFF ")\n", volumeOffset,
		volumeSize));
	// unset
	Unset();
	// load the driver settings and find the entry for the volume
	void *settings = load_driver_settings(kFSName);
	const driver_parameter *volume = NULL;
	const driver_settings *ds = get_driver_settings(settings);
	if (ds)
		volume = _FindVolumeParameter(ds, volumeOffset, volumeSize);
	// init the object and unload the settings
	_Init(ds, volume);
	unload_driver_settings(settings);
	PRINT(("Settings::SetTo(%" B_PRIdOFF ", %" B_PRIdOFF ") done: B_OK\n",
		volumeOffset, volumeSize));
	return B_OK;
}

// Unset
void
Settings::Unset()
{
	fDefaultVolumeName.Unset();
	fVolumeName.Unset();
	fHideEsoteric = kDefaultHideEsoteric;
	fHiddenEntries.MakeEmpty();
}

// GetDefaultVolumeName
const char *
Settings::GetDefaultVolumeName() const
{
	if (fDefaultVolumeName.GetLength() > 0)
		return fDefaultVolumeName.GetString();
	return kDefaultDefaultVolumeName;
}

// GetVolumeName
const char *
Settings::GetVolumeName() const
{
	if (fVolumeName.GetLength() > 0)
		return fVolumeName.GetString();
	return GetDefaultVolumeName();
}

// GetHideEsoteric
bool
Settings::GetHideEsoteric() const
{
	return fHideEsoteric;
}

// HiddenEntryAt
const char *
Settings::HiddenEntryAt(int32 index) const
{
	const char *entry = NULL;
	if (index >= 0 && index < fHiddenEntries.CountItems())
		entry = fHiddenEntries.ItemAt(index).GetString();
	return entry;
}

// Dump
void
Settings::Dump()
{
	PRINT(("Settings:\n"));
	PRINT(("  default volume name:   `%s'\n", GetDefaultVolumeName()));
	PRINT(("  volume name:           `%s'\n", GetVolumeName()));
	PRINT(("  hide esoteric entries: %d\n", GetHideEsoteric()));
	PRINT(("  %" B_PRId32 " hidden entries:\n", fHiddenEntries.CountItems()));
	for (int32 i = 0; const char *entry = HiddenEntryAt(i); i++)
		PRINT(("    `%s'\n", entry));
}

// _Init
status_t
Settings::_Init(const driver_settings *settings,
				const driver_parameter *volume)
{
PRINT(("Settings::_Init(%p, %p)\n", settings, volume));
	status_t error = B_OK;
	// get the global values
	fDefaultVolumeName.SetTo(_GetParameterValue(settings,
		"default_volume_name", (const char*)NULL, NULL));
PRINT(("  default_volume_name is: `%s'\n", fDefaultVolumeName.GetString()));
	fHideEsoteric = _GetParameterValue(settings, "hide_esoteric_entries",
										kDefaultHideEsoteric,
										kDefaultHideEsoteric);
PRINT(("  hide_esoteric_entries is: %d\n", fHideEsoteric));
	// get the per volume settings
	if (volume) {
PRINT(("  getting volume parameters:\n"));
		fVolumeName.SetTo(_GetParameterValue(volume, "name", (const char*)NULL,
												NULL));
PRINT(("    name is: `%s'\n", fVolumeName.GetString()));
		fHideEsoteric = _GetParameterValue(volume, "hide_esoteric_entries",
											fHideEsoteric, fHideEsoteric);
PRINT(("    hide_esoteric_entries is: %d\n", fHideEsoteric));
		int32 cookie = 0;
		while (const driver_parameter *parameter
				= _FindNextParameter(volume, "hide_entries", cookie)) {
			for (int32 i = 0; i < parameter->value_count; i++)
{
				fHiddenEntries.AddItem(parameter->values[i]);
PRINT(("    hidden entry: `%s'\n", parameter->values[i]));
}
		}
	}
	// check the values
PRINT(("  checking volume names...'\n"));
	_CheckVolumeName(fDefaultVolumeName);
	_CheckVolumeName(fVolumeName);
PRINT(("  checking hidden entry names...'\n"));
	for (int32 i = fHiddenEntries.CountItems(); i >= 0; i--) {
		String &entry = fHiddenEntries.ItemAt(i);
		if (!_CheckEntryName(entry.GetString()))
{
PRINT(("    ignoring: `%s'\n", entry.GetString()));
			fHiddenEntries.RemoveItem(i);
}
	}
PRINT(("Settings::_Init() done: %s\n", strerror(error)));
	return error;
}

// _FindVolumeParameter
const driver_parameter *
Settings::_FindVolumeParameter(const driver_settings *settings,
								const char *name)
{
	if (settings) {
		int32 cookie = 0;
		while (const driver_parameter *parameter
				= _FindNextParameter(settings, "volume", cookie)) {
			if (parameter->value_count == 1
				&& !strcmp(parameter->values[0], name)) {
				return parameter;
			}
		}
	}
	return NULL;
}

// _FindVolumeParameter
const driver_parameter *
Settings::_FindVolumeParameter(const driver_settings *settings,
								off_t offset, off_t size)
{
	PRINT(("Settings::_FindVolumeParameter(%" B_PRIdOFF ", %" B_PRIdOFF ")\n",
		offset, size));
	if (settings) {
		int32 cookie = 0;
		while (const driver_parameter *parameter
				= _FindNextParameter(settings, "volume", cookie)) {
			if (_GetParameterValue(parameter, "offset", offset + 1, offset + 1)
					== offset
				&& _GetParameterValue(parameter, "size", size + 1, size + 1)
					== size) {
				PRINT(("Settings::_FindVolumeParameter() done: found parameter:"
					" index: %" B_PRId32 ", (%p)\n", cookie - 1, parameter));
				return parameter;
			}
		}
	}
	PRINT(("Settings::_FindVolumeParameter() done: failed\n"));
	return NULL;
}

// _CheckVolumeName
void
Settings::_CheckVolumeName(String &name)
{
	// truncate, if it is too long
	if (name.GetLength() >= B_FILE_NAME_LENGTH) {
		char buffer[B_FILE_NAME_LENGTH];
		memcpy(buffer, name.GetString(), B_FILE_NAME_LENGTH - 1);
		name.SetTo(buffer, B_FILE_NAME_LENGTH - 1);
	}
	// check for bad characters
	bool invalid = false;
	const char *string = name.GetString();
	for (int32 i = 0; !invalid && i < name.GetLength(); i++) {
		if (string[i] == '/')	// others?
			invalid = true;
	}
	if (invalid)
		name.Unset();
}

// _CheckEntryName
bool
Settings::_CheckEntryName(const char *name)
{
	int32 len = (name ? strlen(name) : 0);
	// any further restictions?
	return (len > 0);
}

