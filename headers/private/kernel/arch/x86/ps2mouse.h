/*
 * ps2mouse.c:
 * PS/2 mouse device driver for NewOS and OpenBeOS.
 * Author:     Elad Lahav (elad@eldarshany.com)
 * Created:    21.12.2001
 * Modified:   11.1.2002
 */

/*
 * A PS/2 mouse is connected to the IBM 8042 controller, and gets its
 * name from the IBM PS/2 personal computer, which was the first to
 * use this device. All resources are shared between the keyboard, and
 * the mouse, referred to as the "Auxiliary Device".
 * I/O:
 * ~~~
 * The controller has 3 I/O registers:
 * 1. Status (input), mapped to port 64h
 * 2. Control (output), mapped to port 64h
 * 3. Data (input/output), mapped to port 60h
 * Data:
 * ~~~~
 * Since a mouse is an input only device, data can only be read, and
 * not written. A packet read from the mouse data port is composed of
 * three bytes:
 * byte 0: status byte, where
 * - bit 0: Y overflow (1 = true)
 * - bit 1: X overflow (1 = true)
 * - bit 2: MSB of Y offset
 * - bit 3: MSB of X offset
 * - bit 4: Syncronization bit (always 1)
 * - bit 5: Middle button (1 = down)
 * - bit 6: Right button (1 = down)
 * - bit 7: Left button (1 = down)
 * byte 1: X position change, since last probed (-127 to +127)
 * byte 2: Y position change, since last probed (-127 to +127)
 * Interrupts:
 * ~~~~~~~~~~
 * The PS/2 mouse device is connected to interrupt 12, which means that
 * it uses the second interrupt controller (handles INT8 to INT15). In
 * order for this interrupt to be enabled, both the 5th interrupt of
 * the second controller AND the 3rd interrupt of the first controller
 * (cascade mode) should be unmasked.
 * The controller uses 3 consecutive interrupts to inform the computer
 * that it has new data. On the first the data register holds the status
 * byte, on the second the X offset, and on the 3rd the Y offset.
 */

#ifndef _KERNEL_ARCH_x86_PS2MOUSE_H
#define _KERNEL_ARCH_x86_PS2MOUSE_H

#include <kernel.h>
#include <memheap.h>
#include <int.h>
#include <debug.h>
#include <devfs.h>
#include <arch/int.h>
#include <OS.h>
#include <string.h>

/////////////////////////////////////////////////////////////////////////
// definitions

// I/O addresses
#define PS2_PORT_DATA            0x60
#define PS2_PORT_CTRL            0x64

// control words
#define PS2_CTRL_WRITE_CMD       0x60
#define PS2_CTRL_WRITE_AUX       0xD4

// data words
#define PS2_CMD_DEV_INIT         0x43
#define PS2_CMD_ENABLE_MOUSE     0xF4
#define PS2_CMD_DISABLE_MOUSE    0xF5
#define PS2_CMD_RESET_MOUSE      0xFF

#define PS2_RES_ACK              0xFA
#define PS2_RES_RESEND           0xFE

// interrupts
#define INT_BASE                 0x20
#define INT_PS2_MOUSE            0x0C
#define INT_CASCADE              0x02

#define PACKET_SIZE              3

/*
 * mouse_data:
 * Holds the data read from the PS/2 auxiliary device.
 */
typedef struct {
   char status;
	char delta_x;
	char delta_y;
} mouse_data; // mouse_data

#ifdef _PS2MOUSE_
static mouse_data md_int;
static mouse_data md_read;
static sem_id mouse_sem;
static bool in_read;
#endif

int mouse_dev_init(kernel_args *ka);

#endif /* _KERNEL_ARCH_x86_PS2MOUSE_H */
