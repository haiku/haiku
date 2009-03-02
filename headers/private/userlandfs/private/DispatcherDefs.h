// DispatcherDefs.h

#ifndef USERLAND_FS_DISPATCHER_DEFS_H
#define USERLAND_FS_DISPATCHER_DEFS_H

#include "FSCapabilities.h"
#include "Port.h"


namespace UserlandFSUtil {

extern const char* kUserlandFSDispatcherPortName;
extern const char* kUserlandFSDispatcherReplyPortName;

struct fs_init_info {
	FSCapabilities	capabilities;
	client_fs_type	clientFSType;
	int32			portInfoCount;
	Port::Info		portInfos[0];
};

}	// namespace UserlandFSUtil

using UserlandFSUtil::kUserlandFSDispatcherPortName;
using UserlandFSUtil::kUserlandFSDispatcherReplyPortName;
using UserlandFSUtil::fs_init_info;

enum {
	UFS_DISPATCHER_CONNECT		= 'cnct',
	UFS_DISPATCHER_CONNECT_ACK	= 'cack',
};

#endif	// USERLAND_FS_DISPATCHER_DEFS_H
