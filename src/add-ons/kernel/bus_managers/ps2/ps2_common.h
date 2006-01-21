/*
 * Copyright 2004-2005 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * PS/2 hid device driver
 *
 * Authors (in chronological order):
 *		Elad Lahav (elad@eldarshany.com)
 *		Stefano Ceccherini (burton666@libero.it)
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef __PS2_COMMON_H
#define __PS2_COMMON_H


#include <ISA.h>
#include <Drivers.h>
#include <KernelExport.h>
#include <OS.h>

#include "ps2_defs.h"
#include "ps2_dev.h"


// debug defines
#ifdef DEBUG
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

// global variables
extern isa_module_info *gIsa;
extern sem_id gDeviceOpenSemaphore;

extern device_hooks sKeyboardDeviceHooks;
extern device_hooks sMouseDeviceHooks;

extern bool gMultiplexingActive;

// prototypes from common.c

status_t ps2_init_driver(void);
void ps2_uninit_driver(void);

extern status_t ps2_wait_read();
extern status_t ps2_wait_write();

extern void ps2_flush();

extern status_t ps2_command(uint8 cmd, const uint8 *out, int out_count, uint8 *in, int in_count);

extern status_t ps2_keyboard_command(uint8 cmd, const uint8 *out, int out_count, uint8 *in, int in_count);

extern status_t ps2_get_command_byte(uint8 *byte);
extern status_t ps2_set_command_byte(uint8 byte);

extern void ps2_claim_result(uint8 *buffer, size_t bytes);
extern void ps2_unclaim_result(void);
extern status_t ps2_wait_for_result(void);
extern bool ps2_handle_result(uint8 data);


// prototypes from keyboard.c & mouse.c
extern status_t probe_keyboard(void);

extern int32 mouse_handle_int(ps2_dev *dev, uint8 data);
extern int32 keyboard_handle_int(uint8 data);


extern status_t keyboard_open(const char *name, uint32 flags, void **cookie);
extern status_t keyboard_close(void *cookie);
extern status_t keyboard_freecookie(void *cookie);
extern status_t keyboard_read(void *cookie, off_t pos, void *buf, size_t *len);
extern status_t keyboard_write(void * cookie, off_t pos, const void *buf, size_t *len);
extern status_t keyboard_ioctl(void *cookie, uint32 op, void *buf, size_t len);

extern status_t mouse_open(const char *name, uint32 flags, void **cookie);
extern status_t mouse_close(void *cookie);
extern status_t mouse_freecookie(void *cookie);
extern status_t mouse_read(void *cookie, off_t pos, void *buf, size_t *len);
extern status_t mouse_write(void * cookie, off_t pos, const void *buf, size_t *len);
extern status_t mouse_ioctl(void *cookie, uint32 op, void *buf, size_t len);

#endif /* __PS2_COMMON_H */
