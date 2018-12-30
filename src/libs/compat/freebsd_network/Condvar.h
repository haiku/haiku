/*
 * Copyright 2009 Colin Günther, coling@gmx.de
 * All Rights Reserved. Distributed under the terms of the MIT License.
 */
#ifndef CONDVAR_H_
#define CONDVAR_H_


#ifdef __cplusplus
extern "C" {
#endif

void conditionInit(struct cv*, const char*);
void conditionPublish(struct cv*, const void*, const char*);
void conditionUnpublish(struct cv*);
int conditionTimedWait(struct cv*, const int);
void conditionWait(struct cv*);
void conditionNotifyOne(struct cv*);
int publishedConditionTimedWait(const void*, const int);
void publishedConditionNotifyAll(const void*);
void publishedConditionNotifyOne(const void*);

#ifdef __cplusplus
}
#endif

#endif /* CONDVAR_H_ */
