/*
 * Copyright 2008-2012 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SEMAPHORE_H_
#define _SEMAPHORE_H_


#include <fcntl.h>
#include <stdint.h>
#include <sys/cdefs.h>
#include <time.h>


typedef struct _sem_t {
	int32_t	id;
	int32_t	_padding[3];
} sem_t;

#define SEM_FAILED	((sem_t*)(long)-1)

__BEGIN_DECLS

sem_t*	sem_open(const char* name, int openFlags,...);
int		sem_close(sem_t* semaphore);
int		sem_unlink(const char* name);

int		sem_init(sem_t* semaphore, int shared, unsigned value);
int		sem_destroy(sem_t* semaphore);

int		sem_post(sem_t* semaphore);
int		sem_timedwait(sem_t* semaphore, const struct timespec* timeout);
int		sem_trywait(sem_t* semaphore);
int		sem_wait(sem_t* semaphore);
int		sem_getvalue(sem_t* semaphore, int* value);

__END_DECLS


#endif	/* _SEMAPHORE_H_ */
