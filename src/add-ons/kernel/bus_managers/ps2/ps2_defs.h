/*
 * Copyright 2004-2005 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * PS/2 interface definitions
 *
 * Authors (in chronological order):
 *		Elad Lahav (elad@eldarshany.com)
 *		Stefano Ceccherini (burton666@libero.it)
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef _PS2_DEFS_H
#define _PS2_DEFS_H


/** Interface definitions for the Intel 8042, 8741, or 8742 (PS/2) */

// I/O addresses
#define PS2_PORT_DATA					0x60
#define PS2_PORT_CTRL					0x64

// data port bits
#define PS2_STATUS_OUTPUT_BUFFER_FULL	0x01
#define PS2_STATUS_INPUT_BUFFER_FULL	0x02
#define PS2_STATUS_AUX_DATA				0x20
#define PS2_STATUS_TIMEOUT				0x40

// control words
#define PS2_CTRL_READ_CMD				0x20
#define PS2_CTRL_WRITE_CMD				0x60
#define PS2_CTRL_WRITE_AUX				0xd4
#define PS2_CTRL_MOUSE_DISABLE			0xa7
#define PS2_CTRL_MOUSE_ENABLE			0xa8
#define PS2_CTRL_MOUSE_TEST				0xa9
#define PS2_CTRL_SELF_TEST				0xaa
#define PS2_CTRL_KEYBOARD_TEST			0xab
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
#define PS2_CMD_KEYBOARD_SET_LEDS		0xed
#define PS2_CMD_KEYBOARD_SET_TYPEMATIC	0xf3
#define PS2_CMD_ECHO					0xee
#define PS2_CMD_TEST_PASSED				0xaa
#define PS2_CMD_GET_DEVICE_ID			0xf2
#define PS2_CMD_SET_SAMPLE_RATE			0xf3
#define PS2_CMD_ENABLE					0xf4
#define PS2_CMD_DISABLE					0xf5
#define PS2_CMD_RESET					0xff
#define PS2_CMD_RESEND					0xfe

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
#define PS2_DEV_ID_TOUCHPAD_RICATECH	4

// packet sizes
#define PS2_PACKET_STANDARD				3
#define PS2_PACKET_INTELLIMOUSE			4
#define PS2_PACKET_SYNAPTICS			6
#define PS2_PACKET_ALPS					6
#define PS2_MAX_PACKET_SIZE				6
	// Should be equal to the biggest packet size

// timeouts
#define PS2_CTRL_WAIT_TIMEOUT			500000

#endif /* _PS2_H */
