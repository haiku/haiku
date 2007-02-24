// IOCtlInfo.h

#ifndef USERLAND_FS_IOCTL_INFO_H
#define USERLAND_FS_IOCTL_INFO_H

#include <SupportDefs.h>

// IOCtlInfo
struct IOCtlInfo {
	int		command;
	bool	isBuffer;
	int32	bufferSize;
	int32	writeBufferSize;
};

#endif	// USERLAND_FS_IOCTL_INFO_H
