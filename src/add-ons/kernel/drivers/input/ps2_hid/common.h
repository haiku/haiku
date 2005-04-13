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
// definitions

// I/O addresses
#define PS2_PORT_DATA            0x60
#define PS2_PORT_CTRL            0x64

// data port bits
#define PS2_OBUF_FULL			 0x01
#define PS2_IBUF_FULL			 0x02
#define PS2_MOUSE_OBUF_FULL 	 0x20
#define PS2_TIMEOUT				 0x40

// control words
#define PS2_CTRL_READ_CMD		 0x20
#define PS2_CTRL_WRITE_CMD       0x60
#define PS2_CTRL_WRITE_AUX       0xD4
#define PS2_CTRL_MOUSE_DISABLE	 0xA7
#define PS2_CTRL_MOUSE_ENABLE	 0xA8
#define PS2_CTRL_MOUSE_TEST		 0xA9

// command bytes
#define PS2_CMD_DEV_INIT         0x43

// command bits
#define PS2_BITS_AUX_INTERRUPT	 0x02
#define PS2_BITS_MOUSE_DISABLED	 0x20

// data words
#define PS2_CMD_TEST_PASSED		 0xAA
#define PS2_CMD_GET_DEVICE_ID	 0xF2
#define PS2_CMD_SET_SAMPLE_RATE	 0xF3
#define PS2_CMD_ENABLE_MOUSE     0xF4
#define PS2_CMD_DISABLE_MOUSE    0xF5
#define PS2_CMD_RESET_MOUSE      0xFF

// reply codes
#define PS2_RES_TEST_PASSED		 0x55
#define PS2_RES_ACK              0xFA
#define PS2_RES_RESEND           0xFE
#define PS2_ERROR				 0xFC

// interrupts
#define INT_PS2_MOUSE            0x0C
#define INT_PS2_KEYBOARD		 0x01

// mouse device IDs
#define PS2_DEV_ID_STANDARD		 0
#define PS2_DEV_ID_INTELLIMOUSE	 3

// packet sizes
#define PS2_PACKET_STANDARD		 3
#define PS2_PACKET_INTELLIMOUSE	 4
#define PS2_MAX_PACKET_SIZE		 4 // Should be equal to the biggest packet size

// debug defines
#ifdef DEBUG
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

// global variables
extern isa_module_info *gIsa;

// prototypes
bool wait_write_ctrl(void);
bool wait_write_data(void);
bool wait_read_data(void);
void write_aux_byte(unsigned char cmd);
uint8 read_data_byte(void);

int8 get_command_byte(void);
void set_command_byte(unsigned char cmd);

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
