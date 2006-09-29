#ifndef _MOUNT_H

#define _MOUNT_H

#include <SupportDefs.h>

const int32 MOUNT_VERSION=1;
const int32 MOUNT_PROGRAM=100005;

enum
{
	MOUNTPROC_NULL		= 0,
	MOUNTPROC_MNT		= 1,
	MOUNTPROC_DUMP		= 2,
	MOUNTPROC_UMNT		= 3,
	MOUNTPROC_UMNTALL	= 4,
	MOUNTPROC_EXPORT	= 5
};

#endif
