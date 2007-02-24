// DispatcherDefs.h

#ifndef USERLAND_FS_DISPATCHER_DEFS_H
#define USERLAND_FS_DISPATCHER_DEFS_H

namespace UserlandFSUtil {

extern const char* kUserlandFSDispatcherPortName;
extern const char* kUserlandFSDispatcherReplyPortName;

}	// namespace UserlandFSUtil

using UserlandFSUtil::kUserlandFSDispatcherPortName;
using UserlandFSUtil::kUserlandFSDispatcherReplyPortName;

enum {
	UFS_DISPATCHER_CONNECT		= 'cnct',
	UFS_DISPATCHER_CONNECT_ACK	= 'cack',
};

#endif	// USERLAND_FS_DISPATCHER_DEFS_H
