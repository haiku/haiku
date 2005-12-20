#ifndef PTHREAD_EMU_H
#define PTHREAD_EMU_H

#ifdef __cplusplus
extern "C" {
#endif

#include <TLS.h>
#include <pthread.h>
#include <OS.h>


typedef int32 pthread_key_t;


extern pthread_mutex_t pthread_mutex_static_initializer(void);

extern int pthread_key_create(pthread_key_t *key, void (*destructor)(void *));
extern int pthread_setspecific(pthread_key_t key, void *value);
extern void *pthread_getspecific(pthread_key_t key);


#ifdef __cplusplus
}
#endif

#endif	/* PTHREAD_EMU_H */
