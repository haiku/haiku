/*
 * Copyright 2009 Colin Günther, coling@gmx.de
 * All Rights Reserved. Distributed under the terms of the MIT License.
 */
#ifndef CONDVAR_H_
#define CONDVAR_H_


#ifdef __cplusplus
extern "C" {
#endif

void _cv_init(struct cv*, const char*);
void _cv_wait_unlocked(struct cv *);
int _cv_timedwait_unlocked(struct cv*, int);
void _cv_signal(struct cv*);

#ifdef __cplusplus
}
#endif

#endif /* CONDVAR_H_ */
