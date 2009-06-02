#ifndef _LOCK_H
#define _LOCK_H

#include <OS.h>

#include <lock.h>

typedef recursive_lock lock;

#define	LOCK(l)		recursive_lock_lock(&l);
#define	UNLOCK(l)	recursive_lock_unlock(&l);

#endif
