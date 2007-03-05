// FSCapabilities.h

#ifndef USERLAND_FS_FS_CAPABILITIES_H
#define USERLAND_FS_FS_CAPABILITIES_H

#include <string.h>
#include <stdio.h>

#include "Debug.h"

enum client_fs_type {
	CLIENT_FS_BEOS_KERNEL = 0,
	CLIENT_FS_HAIKU_KERNEL,
};

enum {
	// FS operations
	FS_CAPABILITY_MOUNT	= 0,
	FS_CAPABILITY_UNMOUNT,

	FS_CAPABILITY_READ_FS_INFO,
	FS_CAPABILITY_WRITE_FS_INFO,
	FS_CAPABILITY_SYNC,

	// vnode operations
	FS_CAPABILITY_LOOKUP,
	FS_CAPABILITY_GET_VNODE_NAME,

	FS_CAPABILITY_GET_VNODE,
	FS_CAPABILITY_PUT_VNODE,
	FS_CAPABILITY_REMOVE_VNODE,

	// VM file access
	FS_CAPABILITY_CAN_PAGE,
	FS_CAPABILITY_READ_PAGES,
	FS_CAPABILITY_WRITE_PAGES,

	// cache file access
	FS_CAPABILITY_GET_FILE_MAP,

	// common operations
	FS_CAPABILITY_IOCTL,
	FS_CAPABILITY_SET_FLAGS,
	FS_CAPABILITY_SELECT,
	FS_CAPABILITY_DESELECT,
	FS_CAPABILITY_FSYNC,

	FS_CAPABILITY_READ_SYMLINK,
	FS_CAPABILITY_CREATE_SYMLINK,

	FS_CAPABILITY_LINK,
	FS_CAPABILITY_UNLINK,
	FS_CAPABILITY_RENAME,

	FS_CAPABILITY_ACCESS,
	FS_CAPABILITY_READ_STAT,
	FS_CAPABILITY_WRITE_STAT,

	// file operations
	FS_CAPABILITY_CREATE,
	FS_CAPABILITY_OPEN,
	FS_CAPABILITY_CLOSE,
	FS_CAPABILITY_FREE_COOKIE,
	FS_CAPABILITY_READ,
	FS_CAPABILITY_WRITE,

	// directory operations
	FS_CAPABILITY_CREATE_DIR,
	FS_CAPABILITY_REMOVE_DIR,
	FS_CAPABILITY_OPEN_DIR,
	FS_CAPABILITY_CLOSE_DIR,
	FS_CAPABILITY_FREE_DIR_COOKIE,
	FS_CAPABILITY_READ_DIR,
	FS_CAPABILITY_REWIND_DIR,

	// attribute directory operations
	FS_CAPABILITY_OPEN_ATTR_DIR,
	FS_CAPABILITY_CLOSE_ATTR_DIR,
	FS_CAPABILITY_FREE_ATTR_DIR_COOKIE,
	FS_CAPABILITY_READ_ATTR_DIR,
	FS_CAPABILITY_REWIND_ATTR_DIR,

	// attribute operations
	FS_CAPABILITY_CREATE_ATTR,
	FS_CAPABILITY_OPEN_ATTR,
	FS_CAPABILITY_CLOSE_ATTR,
	FS_CAPABILITY_FREE_ATTR_COOKIE,
	FS_CAPABILITY_READ_ATTR,
	FS_CAPABILITY_WRITE_ATTR,

	FS_CAPABILITY_READ_ATTR_STAT,
	FS_CAPABILITY_WRITE_ATTR_STAT,
	FS_CAPABILITY_RENAME_ATTR,
	FS_CAPABILITY_REMOVE_ATTR,

	// index directory & index operations
	FS_CAPABILITY_OPEN_INDEX_DIR,
	FS_CAPABILITY_CLOSE_INDEX_DIR,
	FS_CAPABILITY_FREE_INDEX_DIR_COOKIE,
	FS_CAPABILITY_READ_INDEX_DIR,
	FS_CAPABILITY_REWIND_INDEX_DIR,

	FS_CAPABILITY_CREATE_INDEX,
	FS_CAPABILITY_REMOVE_INDEX,
	FS_CAPABILITY_READ_INDEX_STAT,

	// query operations
	FS_CAPABILITY_OPEN_QUERY,
	FS_CAPABILITY_CLOSE_QUERY,
	FS_CAPABILITY_FREE_QUERY_COOKIE,
	FS_CAPABILITY_READ_QUERY,
	FS_CAPABILITY_REWIND_QUERY,

	FS_CAPABILITY_COUNT,
};


namespace UserlandFSUtil {

struct FSCapabilities {
			client_fs_type		clientFSType;
			uint8				capabilities[(FS_CAPABILITY_COUNT + 7) / 8];

	inline	void				ClearAll();

	inline	void				Set(uint32 capability, bool set = true);
	inline	void				Clear(uint32 capability);
	inline	bool				Get(uint32 capability) const;

	inline	void				Dump() const;
};

// ClearAll
inline void
FSCapabilities::ClearAll()
{
	memset(capabilities, 0, sizeof(capabilities));
}

// Set
inline void
FSCapabilities::Set(uint32 capability, bool set)
{
	if (capability >= FS_CAPABILITY_COUNT)
		return;

	uint8 flag = uint8(1 << (capability % 8));
	if (set)
		capabilities[capability / 8] |= flag;
	else
		capabilities[capability / 8] &= ~flag;
}

// Clear
inline void
FSCapabilities::Clear(uint32 capability)
{
	Set(capability, false);
}

// Get
inline bool
FSCapabilities::Get(uint32 capability) const
{
	if (capability >= FS_CAPABILITY_COUNT)
		return false;

	uint8 flag = uint8(1 << (capability % 8));
	return (capabilities[capability / 8] & flag);
}

// Dump
inline void
FSCapabilities::Dump() const
{
	D(
		char buffer[128];
		int byteCount = sizeof(capabilities);
		for (int i = 0; i < byteCount; i++)
			sprintf(buffer + 2 * i, "%02x", (int)capabilities[i]);

		PRINT(("FSCapabilities[%d, %s]\n", clientFSType, buffer));
	)
}


}	// namespace UserlandFSUtil

using UserlandFSUtil::FSCapabilities;

#endif	// USERLAND_FS_FS_CAPABILITIES_H
