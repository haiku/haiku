// GlobalBlockerPool.h

#ifndef NET_FS_GLOBAL_BLOCKER_POOL_H
#define NET_FS_GLOBAL_BLOCKER_POOL_H

#include "BlockerPool.h"

class GlobalBlockerPool {
public:

	static	status_t			CreateDefault();
	static	void				DeleteDefault();
	static	BlockerPool*		GetDefault();

private:
	static	BlockerPool*		sPool;
};

#endif	// NET_FS_GLOBAL_BLOCKER_POOL_H
