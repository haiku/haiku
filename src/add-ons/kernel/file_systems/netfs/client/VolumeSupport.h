// VolumeSupport.h

#ifndef NET_FS_VOLUME_SUPPORT_H
#define NET_FS_VOLUME_SUPPORT_H

#include <dirent.h>

#include "DebugSupport.h"

// set_dirent_name
static inline
status_t
set_dirent_name(struct dirent* buffer, size_t bufferSize, const char* name,
	int32 nameLen)
{
	size_t length = (buffer->d_name + nameLen + 1) - (char*)buffer;
	if (length <= bufferSize) {
		memcpy(buffer->d_name, name, nameLen);
		buffer->d_name[nameLen] = '\0';
		buffer->d_reclen = length;
		return B_OK;
	} else {
		RETURN_ERROR(B_BAD_VALUE);
	}
}

// next_dirent
static inline
bool
next_dirent(struct dirent*& buffer, size_t& bufferSize)
{
	// align
	char* nextBuffer = (char*)buffer + buffer->d_reclen;
	nextBuffer = (char*)(((uint32)nextBuffer + 3) & ~0x3);

	// check, if the buffer is at least large enough to align the current entry
	int32 len = nextBuffer - (char*)buffer;
	if (len > (int32)bufferSize)
		return false;

	buffer->d_reclen = len;
	buffer = (dirent*)nextBuffer;
	bufferSize -= len;

	return true;
}

#endif	// NET_FS_VOLUME_SUPPORT_H
