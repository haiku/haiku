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

// other stuff
#define MOUSE_HISTORY_SIZE		 256

// mouse device IDs
#define PS2_DEV_ID_STANDARD		 0
#define PS2_DEV_ID_INTELLIMOUSE	 3

// packet sizes
#define PS2_PACKET_STANDARD		 3
#define PS2_PACKET_INTELLIMOUSE	 4
#define PS2_MAX_PACKET_SIZE		 4 // Should be equal to the biggest packet size

#endif /* _KERNEL_ARCH_x86_PS2MOUSE_H */
