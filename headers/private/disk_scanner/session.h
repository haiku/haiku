// session.h

#ifndef _PARTSCAN_SESSION_H
#define _PARTSCAN_SESSION_H

#include <module.h>

struct session_info;

typedef bool (*session_identify_hook)(int deviceFD, off_t deviceSize,
	int32 blockSize);
typedef status_t (*session_get_nth_info_hook)(int deviceFD, int32 index,
	off_t deviceSize, int32 blockSize, struct session_info *sessionInfo);

typedef struct session_module_info {
	module_info					module;

	session_identify_hook		identify;
	session_get_nth_info_hook	get_nth_info;
} session_module_info;

/*
	identify():
	----------

	Checks whether the module knows about sessions on the given device.
	Returns true, if it does, false otherwise.

	params:
	deviceFD: a device FD
	deviceSize: size of the device in bytes
	blockSize: the logical block size


	get_nth_info():
	--------------

	Fills in all fields of sessionInfo with information about
	the indexth session on the specified device.

	params:
	deviceFD: a device FD
	index: the session index
	deviceSize: size of the device in bytes
	blockSize: the logical block size
	sessionInfo: the session info

	The functions is only called, when a call to identify() returned
	true.

	Returns B_OK, if successful, B_ENTRY_NOT_FOUND, if the index is out of
	range.
*/

#endif	// _PARTSCAN_SESSION_H
