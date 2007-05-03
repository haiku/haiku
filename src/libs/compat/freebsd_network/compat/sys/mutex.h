#ifndef _FBSD_COMPAT_SYS_MUTEX_H_
#define _FBSD_COMPAT_SYS_MUTEX_H_

#include <lock.h>

struct mtx {
	int type;
	union {
		mutex mutex;
		int32 spinlock;
		recursive_lock recursive;
	} u;
};

#define MA_OWNED		0x1
#define MA_NOTOWNED		0x2
#define MA_RECURSED		0x4
#define MA_NOTRECURSED	0x8

#define mtx_assert(mtx, what)

#define MTX_DEF				0x0000
#define MTX_RECURSE			0x0004

#define MTX_NETWORK_LOCK	"network driver"


static inline void mtx_lock(struct mtx *mtx)
{
	if (mtx->type == MTX_DEF)
		mutex_lock(&mtx->u.mutex);
	else if (mtx->type == MTX_RECURSE)
		recursive_lock_lock(&mtx->u.recursive);
}


static inline void mtx_unlock(struct mtx *mtx)
{
	if (mtx->type == MTX_DEF)
		mutex_unlock(&mtx->u.mutex);
	else if (mtx->type == MTX_RECURSE)
		recursive_lock_unlock(&mtx->u.recursive);
}


void mtx_init(struct mtx *m, const char *name, const char *type, int opts);
void mtx_destroy(struct mtx *m);

extern struct mtx Giant;

#endif
