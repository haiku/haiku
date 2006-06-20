/* 
** Distributed under the terms of the Haiku License.
*/
#ifndef _PTHREAD_H_
#define _PTHREAD_H_

#include <time.h>
#include <OS.h>

typedef thread_id 					pthread_t;
typedef struct  _pthread_attr		*pthread_attr_t;
typedef struct  _pthread_mutex		*pthread_mutex_t;
typedef struct  _pthread_mutexattr	*pthread_mutexattr_t;
typedef struct  _pthread_cond		*pthread_cond_t;
typedef struct  _pthread_cond_attr	*pthread_condattr_t;
typedef int							pthread_key_t;
typedef struct  _pthread_once		pthread_once_t;
typedef struct  _pthread_rwlock		*pthread_rwlock_t;
typedef struct  _pthread_rwlockattr	*pthread_rwlockattr_t;
typedef struct  _pthread_barrier		*pthread_barrier_t;
typedef struct  _pthread_barrierattr	*pthread_barrierattr_t;
typedef struct  _pthread_spinlock	*pthread_spinlock_t;

struct pthread_once {
	int	state;
	pthread_mutex_t mutex;
};


enum pthread_mutex_type {
	PTHREAD_MUTEX_DEFAULT,
	PTHREAD_MUTEX_NORMAL,
	PTHREAD_MUTEX_ERRORCHECK,
	PTHREAD_MUTEX_RECURSIVE,
};

enum pthread_process_shared {
	PTHREAD_PROCESS_PRIVATE,
	PTHREAD_PROCESS_SHARED,
};

/*
 * Flags for threads and thread attributes.
 */
#define PTHREAD_DETACHED		0x1
#define PTHREAD_SCOPE_SYSTEM		0x2
#define PTHREAD_INHERIT_SCHED		0x4
#define PTHREAD_NOFLOAT			0x8

#define PTHREAD_CREATE_DETACHED		PTHREAD_DETACHED
#define PTHREAD_CREATE_JOINABLE		0
#define PTHREAD_SCOPE_PROCESS		0
#define PTHREAD_EXPLICIT_SCHED		0

/*
 * Flags for cancelling threads
 */
#define PTHREAD_CANCEL_ENABLE		0
#define PTHREAD_CANCEL_DISABLE		1
#define PTHREAD_CANCEL_DEFERRED		0
#define PTHREAD_CANCEL_ASYNCHRONOUS	2
#define PTHREAD_CANCELED		((void *) 1)

#define PTHREAD_COND_INITIALIZER	NULL

#define PTHREAD_NEEDS_INIT	0
#define PTHREAD_DONE_INIT	1
#define PTHREAD_ONCE_INIT 	{ PTHREAD_NEEDS_INIT, NULL }

#define PTHREAD_BARRIER_SERIAL_THREAD	-1
#define PTHREAD_PRIO_NONE		0
#define PTHREAD_PRIO_INHERIT		1
#define PTHREAD_PRIO_PROTECT		2

//extern pthread_mutexattr_t pthread_mutexattr_default;

#ifdef __cplusplus
extern "C" {
#endif

extern pthread_mutex_t _pthread_mutex_static_initializer(void);
extern pthread_mutex_t _pthread_recursive_mutex_static_initializer(void);
#define PTHREAD_MUTEX_INITIALIZER \
	pthread_mutex_static_initializer();
#define PTHREAD_RECURSIVE_MUTEX_INITIALIZER \
	pthread_recursive_mutex_static_initializer();

/* mutex functions */
extern int pthread_mutex_destroy(pthread_mutex_t *mutex);
extern int pthread_mutex_getprioceiling(pthread_mutex_t *mutex, int *_priorityCeiling);
extern int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr);
extern int pthread_mutex_lock(pthread_mutex_t *mutex);
extern int pthread_mutex_setprioceiling(pthread_mutex_t *mutex, int newPriorityCeiling,
				int *_oldPriorityCeiling);
extern int pthread_mutex_timedlock(pthread_mutex_t *mutex, const struct timespec *spec);
extern int pthread_mutex_trylock(pthread_mutex_t *mutex);
extern int pthread_mutex_unlock(pthread_mutex_t *mutex);

/* mutex attribute functions */
extern int pthread_mutexattr_destroy(pthread_mutexattr_t *mutexAttr);
extern int pthread_mutexattr_getprioceiling(pthread_mutexattr_t *mutexAttr, int *_priorityCeiling);
extern int pthread_mutexattr_getprotocol(pthread_mutexattr_t *mutexAttr, int *_protocol);
extern int pthread_mutexattr_getpshared(pthread_mutexattr_t *mutexAttr, int *_processShared);
extern int pthread_mutexattr_gettype(pthread_mutexattr_t *mutexAttr, int *_type);
extern int pthread_mutexattr_init(pthread_mutexattr_t *mutexAttr);
extern int pthread_mutexattr_setprioceiling(pthread_mutexattr_t *mutexAttr, int priorityCeiling);
extern int pthread_mutexattr_setprotocol(pthread_mutexattr_t *mutexAttr, int protocol);
extern int pthread_mutexattr_setpshared(pthread_mutexattr_t *mutexAttr, int processShared);
extern int pthread_mutexattr_settype(pthread_mutexattr_t *mutexAttr, int type);

/* misc. functions */
extern int pthread_atfork(void (*prepare)(void), void (*parent)(void), void (*child)(void));

/* thread attributes functions */
extern int pthread_attr_destroy(pthread_attr_t *attr);
extern int pthread_attr_init(pthread_attr_t *attr);
extern int pthread_attr_getdetachstate(const pthread_attr_t *attr, int *detachstate);
extern int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate);

extern int pthread_create(pthread_t *thread, const pthread_attr_t *attr, 
	void *(*start_routine)(void*), void *arg);
extern int pthread_detach(pthread_t thread);
extern int pthread_equal(pthread_t t1, pthread_t t2);
extern void pthread_exit(void *value_ptr);
extern int pthread_join(pthread_t thread, void **value_ptr);
extern pthread_t pthread_self(void);

extern int pthread_kill(pthread_t thread, int sig);

#ifdef __cplusplus
}
#endif

#endif	/* _PTHREAD_ */
