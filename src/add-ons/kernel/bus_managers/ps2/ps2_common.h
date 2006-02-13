/*
 * Copyright 2004-2006 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * PS/2 hid device driver
 *
 * Authors (in chronological order):
 *		Elad Lahav (elad@eldarshany.com)
 *		Stefano Ceccherini (burton666@libero.it)
 *		Axel Dörfler, axeld@pinc-software.de
 *      Marcus Overhagen <marcus@overhagen.de>
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

extern device_hooks gKeyboardDeviceHooks;
extern device_hooks gMouseDeviceHooks;

extern bool gMultiplexingActive;
extern sem_id gControllerSem;

// prototypes from common.c

status_t		ps2_init(void);
void			ps2_uninit(void);

uint8			ps2_read_ctrl(void);
uint8			ps2_read_data(void);
void			ps2_write_ctrl(uint8 ctrl);
void			ps2_write_data(uint8 data);
status_t		ps2_wait_read(void);
status_t		ps2_wait_write(void);

void			ps2_flush(void);

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

#endif /* __PS2_COMMON_H */
