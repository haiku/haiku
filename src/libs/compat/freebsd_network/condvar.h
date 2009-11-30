/*
 * Copyright 2009 Colin Günther, coling@gmx.de
 * All Rights Reserved. Distributed under the terms of the MIT License.
 */
#ifndef CONDVAR_H_
#define CONDVAR_H_


#ifdef __cplusplus
extern "C" {
#endif

void _cv_init(const void*, const char*);
void _cv_destroy(const void*);
void _cv_wait_unlocked(const void*);
int _cv_timedwait_unlocked(const void*, int);
void _cv_signal(const void*);
void _cv_broadcast(const void*);

#ifdef __cplusplus
}
#endif

#endif /* CONDVAR_H_ */
