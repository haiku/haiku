#ifndef _FBSD_COMPAT_SYS_CALLOUT_H_
#define _FBSD_COMPAT_SYS_CALLOUT_H_

#include <sys/haiku-module.h>

#include <sys/mutex.h>
#include <sys/queue.h>


struct callout {
	struct net_timer	c_timer;
	void *				c_arg;
	void				(*c_func)(void *);
	struct mtx *		c_mtx;
	int					c_flags;
};


void callout_init_mtx(struct callout *, struct mtx *, int);
int	callout_reset(struct callout *, int, void (*)(void *), void *);

#define	callout_drain(c)	_callout_stop_safe(c, 1)
#define	callout_stop(c)		_callout_stop_safe(c, 0)
int	_callout_stop_safe(struct callout *, int);


static inline void
callout_init(struct callout *c, int mpsafe)
{
	if (mpsafe)
		callout_init_mtx(c, NULL, 0);
	else
		callout_init_mtx(c, &Giant, 0);
}

#endif
