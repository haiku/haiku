/* 
** Distributed under the terms of the Haiku License.
*/
#ifndef _PTHREAD_H_
#define _PTHREAD_H_


#include <time.h>


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


struct _pthread_mutex;
struct _pthread_mutexattr;

typedef struct _pthread_mutex *pthread_mutex_t;
typedef struct _pthread_mutexattr *pthread_mutexattr_t;

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

#ifdef __cplusplus
}
#endif

#endif	/* _PTHREAD_ */
