/*
 * Copyright 2009 Colin Günther, coling@gmx.de
 * All Rights Reserved. Distributed under the terms of the MIT License.
 */
#ifndef CONDVAR_H_
#define CONDVAR_H_


#ifdef __cplusplus
extern "C" {
#endif

void conditionPublish(struct cv*, const void*, const char*);
void conditionUnpublish(const struct cv*);
void conditionNotifyOne(const void*);
void conditionNotifyAll(const void*);
int conditionTimedWait(const struct cv*, const int);
void conditionWait(const struct cv*);

#ifdef __cplusplus
}
#endif

#endif /* CONDVAR_H_ */
