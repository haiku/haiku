// userlandfs_ioctl.h

#ifndef USERLAND_FS_IOCTL_H
#define USERLAND_FS_IOCTL_H

#include <Drivers.h>

// the ioctl command we use for tunnelling our commands
enum {
	USERLANDFS_IOCTL	= B_DEVICE_OP_CODES_END + 666,
};

// the supported commands
enum {
	USERLAND_IOCTL_PUT_ALL_PENDING_VNODES	= 1,
};

// the length of the magic we use
enum {
	USERLAND_IOCTL_MAGIC_LENGTH	= 20,
};

// the version of the ioctl protocol
enum {
	USERLAND_IOCTL_CURRENT_VERSION	= 1,
};

// the errors
enum {
	USERLAND_IOCTL_STILL_CONNECTED				= B_ERRORS_END + 666,
	USERLAND_IOCTL_VNODE_COUNTING_DISABLED,
	USERLAND_IOCTL_OPEN_FILES,
	USERLAND_IOCTL_OPEN_DIRECTORIES,
	USERLAND_IOCTL_OPEN_ATTRIBUTE_DIRECTORIES,
	USERLAND_IOCTL_OPEN_INDEX_DIRECTORIES,
	USERLAND_IOCTL_OPEN_QUERIES,
};

namespace UserlandFSUtil {

struct userlandfs_ioctl {
	char		magic[USERLAND_IOCTL_MAGIC_LENGTH];
	int			version;
	int			command;
	status_t	error;
};

extern const char kUserlandFSIOCtlMagic[USERLAND_IOCTL_MAGIC_LENGTH];

}	// namespace UserlandFSUtil

using UserlandFSUtil::userlandfs_ioctl;
using UserlandFSUtil::kUserlandFSIOCtlMagic;

#endif	// USERLAND_FS_IOCTL_H
