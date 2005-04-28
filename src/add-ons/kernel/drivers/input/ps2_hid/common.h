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
#include <KernelExport.h>
#include <OS.h>


/////////////////////////////////////////////////////////////////////////
// Interface definitions for the Intel 8042, 8741, or 8742 (PS/2)

// I/O addresses
#define PS2_PORT_DATA					0x60
#define PS2_PORT_CTRL					0x64

// data port bits
#define PS2_STATUS_OUTPUT_BUFFER_FULL	0x01
#define PS2_STATUS_INPUT_BUFFER_FULL	0x02
#define PS2_STATUS_MOUSE_DATA			0x20
#define PS2_STATUS_TIMEOUT				0x40

// control words
#define PS2_CTRL_READ_CMD				0x20
#define PS2_CTRL_WRITE_CMD				0x60
#define PS2_CTRL_WRITE_AUX				0xd4
#define PS2_CTRL_MOUSE_DISABLE			0xa7
#define PS2_CTRL_MOUSE_ENABLE			0xa8
#define PS2_CTRL_MOUSE_TEST				0xa9
#define PS2_CTRL_KEYBOARD_SELF_TEST		0xaa
#define PS2_CTRL_KEYBOARD_ACTIVATE		0xae
#define PS2_CTRL_KEYBOARD_DEACTIVATE	0xad

// command bytes
#define PS2_CMD_DEV_INIT				0x43

// command bits
#define PS2_BITS_KEYBOARD_INTERRUPT		0x01
#define PS2_BITS_AUX_INTERRUPT			0x02
#define PS2_BITS_KEYBOARD_DISABLED		0x10
#define PS2_BITS_MOUSE_DISABLED			0x20
#define PS2_BITS_TRANSLATE_SCANCODES	0x40

// data words
#define PS2_CMD_TEST_PASSED				0xaa
#define PS2_CMD_GET_DEVICE_ID			0xf2
#define PS2_CMD_SET_SAMPLE_RATE			0xf3
#define PS2_CMD_ENABLE_MOUSE			0xf4
#define PS2_CMD_DISABLE_MOUSE			0xf5
#define PS2_CMD_RESET_MOUSE				0xff

// reply codes
#define PS2_REPLY_TEST_PASSED			0x55
#define PS2_REPLY_ACK					0xfa
#define PS2_REPLY_RESEND				0xfe
#define PS2_REPLY_ERROR					0xfc

// interrupts
#define INT_PS2_MOUSE					0x0c
#define INT_PS2_KEYBOARD				0x01

// mouse device IDs
#define PS2_DEV_ID_STANDARD				0
#define PS2_DEV_ID_INTELLIMOUSE			3

// packet sizes
#define PS2_PACKET_STANDARD				3
#define PS2_PACKET_INTELLIMOUSE			4
#define PS2_MAX_PACKET_SIZE				4
	// Should be equal to the biggest packet size


// debug defines
#ifdef DEBUG
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

// global variables
extern isa_module_info *gIsa;
extern sem_id gDeviceOpenSemaphore;

// prototypes from common.c
extern status_t ps2_write_ctrl(uint8 data);
extern status_t ps2_write_data(uint8 data);
extern status_t ps2_read_data(uint8 *data);

extern status_t ps2_write_aux_byte(uint8 data);
extern uint8 ps2_get_command_byte(void);
extern status_t ps2_set_command_byte(uint8 command);

extern void ps2_claim_result(uint8 *buffer, size_t bytes);
extern void ps2_unclaim_result(void);
extern status_t ps2_wait_for_result(void);
extern bool ps2_handle_result(uint8 data);

extern void ps2_common_uninitialize(void);
extern status_t ps2_common_initialize(void);

// prototypes from keyboard.c & mouse.c
extern status_t probe_keyboard(void);
extern status_t probe_mouse(void);

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
