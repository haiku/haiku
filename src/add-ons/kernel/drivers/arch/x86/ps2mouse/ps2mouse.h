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
#include <string.h>

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

// data words
#define PS2_CMD_DEV_INIT         0x43
#define PS2_CMD_ENABLE_MOUSE     0xF4
#define PS2_CMD_DISABLE_MOUSE    0xF5
#define PS2_CMD_RESET_MOUSE      0xFF
#define PS2_CMD_TEST_PASSED		 0xAA

#define PS2_RES_ACK              0xFA
#define PS2_RES_RESEND           0xFE
#define PS2_ERROR				 0xFC

// interrupts
#define INT_PS2_MOUSE            0x0C

// other stuff
#define PS2_PACKET_SIZE              3
#define MOUSE_HISTORY_SIZE		 256


// TODO: Move these to another file, which will be
// included by every mouse driver, and also by the mouse
// input server addon. At least, that's the idea.

// ioctls
enum {
	MOUSE_GET_MOVEMENTS = 0x2773,
	MOUSE_GET_EVENTS_COUNT = 0x2774,
	MOUSE_GET_ACCELERATION = 0x2775,
	MOUSE_SET_ACCELERATION = 0x2776,
	MOUSE_SET_TYPE = 0x2778,
	MOUSE_SET_MAP = 0x277A,
	MOUSE_SET_CLICK_SPEED = 0x277C
} ioctls;

/*
 * mouse_movements:
 * Passed as parameter of the MOUSE_GET_MOVEMENTS ioctl() call.
 * (compatible with the R5 mouse addon/driver)
 */
typedef struct mouse_movement mouse_movement;
struct mouse_movement {
  int32 ser_fd_index;
  int32 buttons;
  int32 xdelta;
  int32 ydelta;
  int32 click_count;
  int32 mouse_mods;
  int64 mouse_time;
};

#endif /* _KERNEL_ARCH_x86_PS2MOUSE_H */
