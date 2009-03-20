/*
 * Copyright 2001-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef USERLAND_FS_FS_CAPABILITIES_H
#define USERLAND_FS_FS_CAPABILITIES_H

#include <string.h>
#include <stdio.h>

#include "Debug.h"

enum client_fs_type {
	CLIENT_FS_BEOS_KERNEL = 0,
	CLIENT_FS_HAIKU_KERNEL,
	CLIENT_FS_FUSE
};

// FS capabilities
enum {
	// FS operations
	FS_CAPABILITY_MOUNT	= 0,

	FS_CAPABILITY_COUNT
};

// Volume capabilities
enum {
	// general operations
	FS_VOLUME_CAPABILITY_UNMOUNT,

	FS_VOLUME_CAPABILITY_READ_FS_INFO,
	FS_VOLUME_CAPABILITY_WRITE_FS_INFO,
	FS_VOLUME_CAPABILITY_SYNC,

	FS_VOLUME_CAPABILITY_GET_VNODE,

	// index directory & index operations
	FS_VOLUME_CAPABILITY_OPEN_INDEX_DIR,
	FS_VOLUME_CAPABILITY_CLOSE_INDEX_DIR,
	FS_VOLUME_CAPABILITY_FREE_INDEX_DIR_COOKIE,
	FS_VOLUME_CAPABILITY_READ_INDEX_DIR,
	FS_VOLUME_CAPABILITY_REWIND_INDEX_DIR,

	FS_VOLUME_CAPABILITY_CREATE_INDEX,
	FS_VOLUME_CAPABILITY_REMOVE_INDEX,
	FS_VOLUME_CAPABILITY_READ_INDEX_STAT,

	// query operations
	FS_VOLUME_CAPABILITY_OPEN_QUERY,
	FS_VOLUME_CAPABILITY_CLOSE_QUERY,
	FS_VOLUME_CAPABILITY_FREE_QUERY_COOKIE,
	FS_VOLUME_CAPABILITY_READ_QUERY,
	FS_VOLUME_CAPABILITY_REWIND_QUERY,

	// support for FS layers
	FS_VOLUME_CAPABILITY_ALL_LAYERS_MOUNTED,
	FS_VOLUME_CAPABILITY_CREATE_SUB_VNODE,
	FS_VOLUME_CAPABILITY_DELETE_SUB_VNODE,

	FS_VOLUME_CAPABILITY_COUNT
};

// VNode capabilities
enum {
	// vnode operations
	FS_VNODE_CAPABILITY_LOOKUP,
	FS_VNODE_CAPABILITY_GET_VNODE_NAME,

	FS_VNODE_CAPABILITY_PUT_VNODE,
	FS_VNODE_CAPABILITY_REMOVE_VNODE,

	// VM file access
	FS_VNODE_CAPABILITY_CAN_PAGE,
	FS_VNODE_CAPABILITY_READ_PAGES,
	FS_VNODE_CAPABILITY_WRITE_PAGES,

	// asynchronous I/O
	FS_VNODE_CAPABILITY_IO,
	FS_VNODE_CAPABILITY_CANCEL_IO,

	// cache file access
	FS_VNODE_CAPABILITY_GET_FILE_MAP,

	// common operations
	FS_VNODE_CAPABILITY_IOCTL,
	FS_VNODE_CAPABILITY_SET_FLAGS,
	FS_VNODE_CAPABILITY_SELECT,
	FS_VNODE_CAPABILITY_DESELECT,
	FS_VNODE_CAPABILITY_FSYNC,

	FS_VNODE_CAPABILITY_READ_SYMLINK,
	FS_VNODE_CAPABILITY_CREATE_SYMLINK,

	FS_VNODE_CAPABILITY_LINK,
	FS_VNODE_CAPABILITY_UNLINK,
	FS_VNODE_CAPABILITY_RENAME,

	FS_VNODE_CAPABILITY_ACCESS,
	FS_VNODE_CAPABILITY_READ_STAT,
	FS_VNODE_CAPABILITY_WRITE_STAT,

	// file operations
	FS_VNODE_CAPABILITY_CREATE,
	FS_VNODE_CAPABILITY_OPEN,
	FS_VNODE_CAPABILITY_CLOSE,
	FS_VNODE_CAPABILITY_FREE_COOKIE,
	FS_VNODE_CAPABILITY_READ,
	FS_VNODE_CAPABILITY_WRITE,

	// directory operations
	FS_VNODE_CAPABILITY_CREATE_DIR,
	FS_VNODE_CAPABILITY_REMOVE_DIR,
	FS_VNODE_CAPABILITY_OPEN_DIR,
	FS_VNODE_CAPABILITY_CLOSE_DIR,
	FS_VNODE_CAPABILITY_FREE_DIR_COOKIE,
	FS_VNODE_CAPABILITY_READ_DIR,
	FS_VNODE_CAPABILITY_REWIND_DIR,

	// attribute directory operations
	FS_VNODE_CAPABILITY_OPEN_ATTR_DIR,
	FS_VNODE_CAPABILITY_CLOSE_ATTR_DIR,
	FS_VNODE_CAPABILITY_FREE_ATTR_DIR_COOKIE,
	FS_VNODE_CAPABILITY_READ_ATTR_DIR,
	FS_VNODE_CAPABILITY_REWIND_ATTR_DIR,

	// attribute operations
	FS_VNODE_CAPABILITY_CREATE_ATTR,
	FS_VNODE_CAPABILITY_OPEN_ATTR,
	FS_VNODE_CAPABILITY_CLOSE_ATTR,
	FS_VNODE_CAPABILITY_FREE_ATTR_COOKIE,
	FS_VNODE_CAPABILITY_READ_ATTR,
	FS_VNODE_CAPABILITY_WRITE_ATTR,

	FS_VNODE_CAPABILITY_READ_ATTR_STAT,
	FS_VNODE_CAPABILITY_WRITE_ATTR_STAT,
	FS_VNODE_CAPABILITY_RENAME_ATTR,
	FS_VNODE_CAPABILITY_REMOVE_ATTR,

	// support for node and FS layers
	FS_VNODE_CAPABILITY_CREATE_SPECIAL_NODE,
	FS_VNODE_CAPABILITY_GET_SUPER_VNODE,

	FS_VNODE_CAPABILITY_COUNT
};

namespace UserlandFSUtil {

template<const int CapabilityCount>
struct FSCapabilitiesBase {
			uint8				capabilities[(CapabilityCount + 7) / 8];

	inline	void				ClearAll();

	inline	void				Set(uint32 capability, bool set = true);
	inline	void				Clear(uint32 capability);
	inline	bool				Get(uint32 capability) const;

	inline	uint32				GetHashCode() const;

	inline	bool				operator==(
									const FSCapabilitiesBase<CapabilityCount>&
										other) const;

	inline	void				Dump() const;
};


// ClearAll
template<const int CapabilityCount>
inline void
FSCapabilitiesBase<CapabilityCount>::ClearAll()
{
	memset(capabilities, 0, sizeof(capabilities));
}


// Set
template<const int CapabilityCount>
inline void
FSCapabilitiesBase<CapabilityCount>::Set(uint32 capability, bool set)
{
	if (capability >= CapabilityCount)
		return;

	uint8 flag = uint8(1 << (capability % 8));
	if (set)
		capabilities[capability / 8] |= flag;
	else
		capabilities[capability / 8] &= ~flag;
}


// Clear
template<const int CapabilityCount>
inline void
FSCapabilitiesBase<CapabilityCount>::Clear(uint32 capability)
{
	Set(capability, false);
}


// Get
template<const int CapabilityCount>
inline bool
FSCapabilitiesBase<CapabilityCount>::Get(uint32 capability) const
{
	if (capability >= CapabilityCount)
		return false;

	uint8 flag = uint8(1 << (capability % 8));
	return (capabilities[capability / 8] & flag);
}


// GetHashCode
template<const int CapabilityCount>
inline uint32
FSCapabilitiesBase<CapabilityCount>::GetHashCode() const
{
	uint32 hashCode = 0;
	int byteCount = sizeof(capabilities);
	for (int i = 0; i < byteCount; i++)
		hashCode = hashCode * 37 + capabilities[i];

	return hashCode;
}


// ==
template<const int CapabilityCount>
inline bool
FSCapabilitiesBase<CapabilityCount>::operator==(
	const FSCapabilitiesBase<CapabilityCount>& other) const
{
	int byteCount = sizeof(capabilities);
	for (int i = 0; i < byteCount; i++) {
		if (capabilities[i] != other.capabilities[i])
			return false;
	}

	return true;
}


// Dump
template<const int CapabilityCount>
inline void
FSCapabilitiesBase<CapabilityCount>::Dump() const
{
	D(
		char buffer[128];
		int byteCount = sizeof(capabilities);
		for (int i = 0; i < byteCount; i++)
			sprintf(buffer + 2 * i, "%02x", (int)capabilities[i]);

		PRINT(("FSCapabilities[%s]\n", buffer));
	)
}


typedef FSCapabilitiesBase<FS_CAPABILITY_COUNT>			FSCapabilities;
typedef FSCapabilitiesBase<FS_VOLUME_CAPABILITY_COUNT>	FSVolumeCapabilities;
typedef FSCapabilitiesBase<FS_VNODE_CAPABILITY_COUNT>	FSVNodeCapabilities;

}	// namespace UserlandFSUtil

using UserlandFSUtil::FSCapabilities;
using UserlandFSUtil::FSVolumeCapabilities;
using UserlandFSUtil::FSVNodeCapabilities;

#endif	// USERLAND_FS_FS_CAPABILITIES_H
