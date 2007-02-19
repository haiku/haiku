// Settings.cpp

#include <new>

#include <driver_settings.h>

#include "Debug.h"
#include "HashMap.h"
#include "IOCtlInfo.h"
#include "Settings.h"

static const char *kFSName = "userlandfs";

// IOCtlInfoMap
struct Settings::IOCtlInfoMap : public HashMap<HashKey32<int>, IOCtlInfo*> {
};


// _FindNextParameter
template<typename container_t>
static
const driver_parameter *
_FindNextParameter(const container_t *container, const char *name,
	int32 &cookie)
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
static
const char *
_GetParameterValue(const container_t *container, const char *name,
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
static
bool
_GetParameterValue(const container_t *container, const char *name,
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
static
int
_GetParameterValue(const container_t *container, const char *name,
	int unknownValue, int noArgValue)
{
	// note: container may be NULL
	const char unknown = 0;
	const char noArg = 0;
	const char *value = _GetParameterValue(container, name, &unknown, &noArg);
	if (value == &unknown)
		return unknownValue;
	if (value == &noArg)
		return noArgValue;
	return atoi(value);
}

// _FindFSParameter
static
const driver_parameter *
_FindFSParameter(const driver_settings *settings, const char *name)
{
	if (settings) {
		int32 cookie = 0;
		while (const driver_parameter *parameter
				= _FindNextParameter(settings, "file_system", cookie)) {
PRINT(("  found file_system parameter\n"));
if (parameter->value_count > 0)
PRINT(("    value: `%s'\n", parameter->values[0]));
			if (parameter->value_count == 1
				&& !strcmp(parameter->values[0], name)) {
				return parameter;
			}
		}
	}
	return NULL;
}

// constructor
Settings::Settings()
	: fIOCtlInfos(NULL)
{
}

// destructor
Settings::~Settings()
{
	Unset();
}

// SetTo
status_t
Settings::SetTo(const char* fsName)
{
	if (!fsName)
		RETURN_ERROR(B_BAD_VALUE);
	// unset
	Unset();
	// create the ioctl info map
	fIOCtlInfos = new(nothrow) IOCtlInfoMap;
	if (!fIOCtlInfos)
		RETURN_ERROR(B_NO_MEMORY);
	// load the driver settings and find the entry for the FS
	void *settings = load_driver_settings(kFSName);
	const driver_parameter *fsParameter = NULL;
	const driver_settings *ds = get_driver_settings(settings);
	if (!ds)
		RETURN_ERROR(B_ENTRY_NOT_FOUND);
	fsParameter = _FindFSParameter(ds, fsName);
	// init the object and unload the settings
	status_t error = B_OK;
	if (fsParameter)
		_Init(ds, fsParameter);
	else
		error = B_ENTRY_NOT_FOUND;
	unload_driver_settings(settings);
	return B_OK;
}

// Unset
void
Settings::Unset()
{
	if (fIOCtlInfos) {
		for (IOCtlInfoMap::Iterator it = fIOCtlInfos->GetIterator();
			 it.HasNext();) {
			IOCtlInfoMap::Entry entry = it.Next();
			delete entry.value;
		}
		delete fIOCtlInfos;
		fIOCtlInfos = NULL;
	}
}

// GetIOCtlInfo
const IOCtlInfo*
Settings::GetIOCtlInfo(int command) const
{
	return (fIOCtlInfos ? fIOCtlInfos->Get(command) : NULL);
}

// Dump
void
Settings::Dump() const
{
	PRINT(("Settings:\n"));
	if (fIOCtlInfos) {
		for (IOCtlInfoMap::Iterator it = fIOCtlInfos->GetIterator();
			 it.HasNext();) {
			IOCtlInfoMap::Entry entry = it.Next();
			IOCtlInfo* info = entry.value;
			PRINT(("  ioctl %d: buffer size: %ld, write buffer size: %ld\n",
				info->command, info->bufferSize, info->writeBufferSize));
		}
	}
}

// _Init
status_t
Settings::_Init(const driver_settings *settings,
				const driver_parameter *fsParams)
{
PRINT(("Settings::_Init(%p, %p)\n", settings, fsParams));
	status_t error = B_OK;
	int32 cookie = 0;
	while (const driver_parameter *parameter
			= _FindNextParameter(fsParams, "ioctl", cookie)) {
		if (parameter->value_count == 1) {
			int command = atoi(parameter->values[0]);
			if (command > 0) {
				IOCtlInfo* info = fIOCtlInfos->Remove(command);
				if (!info) {
					info = new(nothrow) IOCtlInfo;
					if (!info)
						RETURN_ERROR(B_NO_MEMORY);
				}
				info->command = command;
				info->bufferSize
					= _GetParameterValue(parameter, "buffer_size", 0, 0);
				info->writeBufferSize
					= _GetParameterValue(parameter, "write_buffer_size", 0, 0);
				info->isBuffer = _GetParameterValue(parameter, "is_buffer",
					false, false);
				error = fIOCtlInfos->Put(command, info);
				if (error != B_OK) {
					delete info;
					return error;
				}
			}
		}
	}
PRINT(("Settings::_Init() done: %s\n", strerror(error)));
	return error;
}

