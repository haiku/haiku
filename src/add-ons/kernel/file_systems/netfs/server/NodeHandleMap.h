// NodeHandleMap.h

#ifndef NET_FS_NODE_HANDLE_MAP_H
#define NET_FS_NODE_HANDLE_MAP_H

#include <HashMap.h>

#include "Locker.h"
#include "NodeHandle.h"


// NodeHandleMap
class NodeHandleMap : HashMap<HashKey32<int32>, NodeHandle*>, Locker {
public:
								NodeHandleMap(const char* name);
								~NodeHandleMap();

			status_t			Init();

			status_t			AddNodeHandle(NodeHandle* handle);
			bool				RemoveNodeHandle(NodeHandle* handle);

			status_t			LockNodeHandle(int32 cookie,
									NodeHandle** _handle);
			void				UnlockNodeHandle(NodeHandle* handle);

private:
			int32				_NextNodeHandleCookie();
			vint32				fNextNodeHandleCookie;
};


#endif	// NET_FS_NODE_HANDLE_MAP_H
