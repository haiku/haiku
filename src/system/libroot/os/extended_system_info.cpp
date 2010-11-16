/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <extended_system_info.h>
#include <extended_system_info_defs.h>

#include <syscalls.h>
#include <AutoDeleter.h>

#include <util/KMessage.h>


namespace BPrivate {


status_t
get_extended_team_info(team_id teamID, uint32 flags, KMessage& info)
{
	// initial buffer size
	size_t bufferSize = 4096;
		// TODO: Pick it depending on the set flags.

	while (true) {
		// allocate the buffer
		void* buffer = malloc(bufferSize);
		if (buffer == NULL)
			return B_NO_MEMORY;
		MemoryDeleter bufferDeleter(buffer);

		// get the info
		size_t sizeNeeded;
		status_t error = _kern_get_extended_team_info(teamID, flags, buffer,
			bufferSize, &sizeNeeded);
		if (error == B_OK) {
			return info.SetTo((const void*)buffer, sizeNeeded,
				KMessage::KMESSAGE_CLONE_BUFFER);
				// TODO: Just transfer our buffer, if it isn't much larger.
		}

		if (error != B_BUFFER_OVERFLOW)
			return error;

		// The buffer was too small. Try again with a larger one.
		bufferSize = (sizeNeeded + 1023) / 1024 * 1024;
	}
}


}	// namespace BPrivate
