/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/
#ifndef TTY_PRIVATE_H
#define TTY_PRIVATE_H


#include <KernelExport.h>
#include <Drivers.h>

#include <termios.h>


struct tty {
	int32	open_count;
};

static const uint32 kNumTTYs = 64;

extern tty gMasterTTYs[kNumTTYs];
extern tty gSlaveTTYs[kNumTTYs];

extern device_hooks gMasterTTYHooks;
extern device_hooks gSlaveTTYHooks;


extern int32 get_tty_index(const char *name);

#endif	/* TTY_PRIVATE_H */
