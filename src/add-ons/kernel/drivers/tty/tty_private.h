/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/
#ifndef TTY_PRIVATE_H
#define TTY_PRIVATE_H


#include <KernelExport.h>
#include <Drivers.h>
#include <lock.h>

#include <termios.h>

#include "line_buffer.h"


#define TTY_BUFFER_SIZE 4096

// The only nice Rico'ism :)
#define CTRL(c)	((c) - 0100)


typedef status_t (*tty_service_func)(struct tty *tty, uint32 op);
	// not yet used...

struct tty {
	int32				open_count;
	int32				index;
	struct mutex		lock;
	sem_id				read_sem;
	sem_id				write_sem;
	pid_t				pgrp_id;
	line_buffer			input_buffer;
	tty_service_func	service_func;
	struct termios		termios;
	struct winsize		window_size;
};

static const uint32 kNumTTYs = 64;

extern tty gMasterTTYs[kNumTTYs];
extern tty gSlaveTTYs[kNumTTYs];

extern device_hooks gMasterTTYHooks;
extern device_hooks gSlaveTTYHooks;


// functions available for master/slave TTYs

extern int32 get_tty_index(const char *name);
extern void reset_tty(struct tty *tty, int32 index);
extern status_t tty_input_putc(struct tty *tty, int c);
extern status_t tty_input_read(struct tty *tty, void *buffer, size_t *_length, uint32 mode);
extern status_t tty_output_getc(struct tty *tty, int *_c);
extern status_t tty_write_to_tty(struct tty *tty, struct tty *dest, const void *buffer,
					size_t *_length, uint32 mode, bool sourceIsMaster);

extern status_t tty_open(struct tty *tty, tty_service_func func);
extern status_t tty_close(struct tty *tty);
extern status_t tty_ioctl(struct tty *tty, uint32 op, void *buffer, size_t length);
extern status_t tty_select(struct tty *tty, uint8 event, uint32 ref, selectsync *sync);
extern status_t tty_deselect(struct tty *tty, uint8 event, selectsync *sync);

#endif	/* TTY_PRIVATE_H */
