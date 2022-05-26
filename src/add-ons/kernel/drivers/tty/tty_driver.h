/*
 * Copyright 2004-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef TTY_DRIVER_H
#define TTY_DRIVER_H


#include "tty_private.h"


extern tty gMasterTTYs[kNumTTYs];
extern tty gSlaveTTYs[kNumTTYs];

extern device_hooks gMasterTTYHooks;
extern device_hooks gSlaveTTYHooks;

extern struct mutex gGlobalTTYLock;

extern int32 get_tty_index(const char *name);

extern void reset_tty(struct tty *tty, int32 index, mutex* lock, bool isMaster);


#endif /* !TTY_DRIVER_H */
