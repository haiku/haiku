/*
 * Copyright 2001-2004 Haiku, Inc.
 * ps2mouse.c:
 * PS/2 mouse device driver
 * Authors (in chronological order):
 * 		Elad Lahav (elad@eldarshany.com)
 *		Stefano Ceccherini (burton666@libero.it)
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
 * This is all done inside install_io_interrupt_handler(), no need to
 * worry about it anymore
 * The controller uses 3 consecutive interrupts to inform the computer
 * that it has new data. On the first the data register holds the status
 * byte, on the second the X offset, and on the 3rd the Y offset.
 */

#include <Drivers.h>
#include <Errors.h>
#include <ISA.h>
#include <KernelExport.h>

#include <string.h>

#include <kb_mouse_driver.h>

#include "ps2mouse.h"

#define DEVICE_NAME "input/mouse/ps2/0"

#define TRACE_PS2MOUSE
#ifdef TRACE_PS2MOUSE
	#define TRACE(x) dprintf x
#else
	#define TRACE(x)
#endif

#ifdef COMPILE_FOR_R5
	#include "cbuf_adapter.h"
#else
	#include <cbuf.h>
#endif

int32 api_version = B_CUR_DRIVER_API_VERSION;

static isa_module_info *sIsa = NULL;

static sem_id sMouseSem;
static int32 sSync;
static cbuf *sMouseChain;

static bigtime_t sLastClickTime;
static bigtime_t sClickSpeed;
static int32 sClickCount;
static int sButtonsState;
static int32 sOpenMask;

/////////////////////////////////////////////////////////////////////////
// ps2 protocol stuff

/** Wait until the control port is ready to be written. This requires that
 *	the "Input buffer full" and "Output buffer full" bits will both be set
 *	to 0. Returns true if the control port is ready to be written, false
 *	if 10ms have passed since the function has been called, and the control
 *	port is still busy.
 */

static bool 
wait_write_ctrl()
{
	int8 read;
	int32 tries = 100;
	TRACE(("wait_write_ctrl()\n"));
	do {
		read = sIsa->read_io_8(PS2_PORT_CTRL);
		spin(100);
	} while ((read & (PS2_IBUF_FULL | PS2_OBUF_FULL)) && tries-- > 0);
		
	return tries > 0;
}


/** Wait until the data port is ready to be written. This requires that
 *	the "Input buffer full" bit will be set to 0.
 */

static bool 
wait_write_data()
{
	int8 read;
	int32 tries = 100;
	TRACE(("wait_write_data()\n"));
	do {
		read = sIsa->read_io_8(PS2_PORT_CTRL);
		spin(100);
	} while ((read & PS2_IBUF_FULL) && tries-- > 0);
		
	return tries > 0;
}


/** Wait until the data port can be read from. This requires that the
 *	"Output buffer full" bit will be set to 1.
 */

static bool 
wait_read_data()
{
	int8 read;
	int32 tries = 100;
	TRACE(("wait_read_data()\n"));
	do {
		read = sIsa->read_io_8(PS2_PORT_CTRL);
		spin(100);
	} while (!(read & PS2_OBUF_FULL) && tries-- > 0);
			
	return tries > 0;
}


/** Writes a command byte to the data port of the PS/2 controller.
 *	Parameters:
 *	unsigned char, byte to write
 */

static void 
write_command_byte(unsigned char cmd)
{
	TRACE(("write_command_byte()\n"));
	if (wait_write_ctrl())
		sIsa->write_io_8(PS2_PORT_CTRL, PS2_CTRL_WRITE_CMD);
	if (wait_write_data())
		sIsa->write_io_8(PS2_PORT_DATA, cmd);
}


/** Writes a byte to the mouse device. Uses the control port to indicate
 *	that the byte is sent to the auxiliary device (mouse), instead of the
 *	keyboard.
 *	Parameters:
 *	unsigned char, byte to write
 */

static void 
write_aux_byte(unsigned char cmd)
{
	TRACE(("write_aux_byte()\n"));
	if (wait_write_ctrl())
		sIsa->write_io_8(PS2_PORT_CTRL, PS2_CTRL_WRITE_AUX);
	if (wait_write_data())
		sIsa->write_io_8(PS2_PORT_DATA, cmd);
}


/** Reads a single byte from the data port.
 *	Return value:
 *	unsigned char, byte read
 */

static uint8
read_data_byte()
{
	TRACE(("read_data_byte()\n"));
	if (wait_read_data()) {
		TRACE(("read_data_byte(): ok\n"));
		return sIsa->read_io_8(PS2_PORT_DATA);
	} 
	
	TRACE(("read_data_byte(): timeout\n"));
	return 0;
}


/////////////////////////////////////////////////////////////////////////
// mouse functions

/*
static status_t
ps2_reset_mouse()
{
	int8 read;
	
	TRACE(("ps2_reset_mouse()\n"));
	read = read_data_byte();
	
	write_aux_byte(PS2_CMD_RESET_MOUSE);
	read = read_data_byte();
	
	return B_OK;	
}
*/


/** Enables or disables mouse reporting for the ps2 port.
 */

static status_t
ps2_enable_mouse(bool enable)
{
	int32 tries = 2;
	uint8 read;
	
	do {
		write_aux_byte(enable ? PS2_CMD_ENABLE_MOUSE : PS2_CMD_DISABLE_MOUSE);
		read = read_data_byte();
		if (read == PS2_RES_ACK)
			break;
		spin(100);
	} while (read == PS2_RES_RESEND && tries-- > 0);
	
	if (read != PS2_RES_ACK)
		return B_ERROR;
	
	return B_OK;
}


/** Converts a packet received by the mouse to a "movement".
 */  
static void
packet_to_movement(uint8 packet[], mouse_pos *pos)
{
	int buttons = packet[0] & 7;
	int xDelta = ((packet[0] & 0x10) ? 0xFFFFFF00 : 0) | packet[1];
	int yDelta = ((packet[0] & 0x20) ? 0xFFFFFF00 : 0) | packet[2];
  	bigtime_t currentTime = system_time();
	
  	if (buttons != 0) {
  		if (sButtonsState == 0) {
  			if (sLastClickTime + sClickSpeed > currentTime)
  				sClickCount++;
  			else
  				sClickCount = 1;
  		}
  	}
  	
  	sLastClickTime = currentTime;
  	sButtonsState = buttons;
  	
  	if (pos) {
		pos->xdelta = xDelta;
		pos->ydelta = yDelta;
		pos->buttons = buttons;
		pos->clicks = sClickCount;
		pos->modifiers = 0;
		pos->time = currentTime;
		pos->wheel_delta = 0;
	}
}


/** Read a mouse event from the mouse events chain buffer.
 */
static status_t
ps2_mouse_read(mouse_pos *pos)
{
	status_t status;
	uint8 packet[PS2_PACKET_SIZE];
		
	TRACE(("ps2_mouse_read()\n"));
	status = acquire_sem_etc(sMouseSem, 1, B_CAN_INTERRUPT, 0);
	if (status < B_OK)
		return status;
	
	status = cbuf_memcpy_from_chain(packet, sMouseChain, 0, PS2_PACKET_SIZE);	
	if (status < B_OK) {
		TRACE(("error copying buffer\n"));
		return status;
	}
	
  	packet_to_movement(packet, pos);
  		
	return B_OK;		
}


/////////////////////////////////////////////////////////////////////////
// interrupt


/** Interrupt handler for the mouse device. Called whenever the I/O
 *	controller generates an interrupt for the PS/2 mouse. Reads mouse
 *	information from the data port, and stores it, so it can be accessed
 *	by read() operations. The full data is obtained using 3 consecutive
 *	calls to the handler, each holds a different byte on the data port.
 */
static int32 
handle_mouse_interrupt(void* data)
{
	int8 read;
	TRACE(("mouse interrupt occurred!!!\n"));
	
	read = sIsa->read_io_8(PS2_PORT_CTRL);
	TRACE(("PS2_PORT_CTRL: %02x\n", read));
	
	if (read < 0) {
		TRACE(("Interrupt was not generated by the ps2 mouse\n"));
		return B_UNHANDLED_INTERRUPT;
	}
	
	read = read_data_byte();
	cbuf_memcpy_to_chain(sMouseChain, 0, &read, sizeof(read));
	
	if (sSync == 0 && !(read & 8)) {
		TRACE(("mouse resynched, bad data\n"));
		return B_HANDLED_INTERRUPT;
	}
		
	if (sSync++ == 2) {
		TRACE(("mouse synched\n"));
		sSync = 0;
		release_sem_etc(sMouseSem, 1, B_DO_NOT_RESCHEDULE);
		
		return B_INVOKE_SCHEDULER;
	}
	
	return B_HANDLED_INTERRUPT;
}


/////////////////////////////////////////////////////////////////////////
// file operations


static status_t 
mouse_open(const char *name, uint32 flags, void **cookie)
{
	status_t status;	
	TRACE(("mouse_open()\n"));	
	
	if (atomic_or(&sOpenMask, 1) != 0)
		return B_BUSY;
		
	*cookie = NULL;
		
	write_command_byte(PS2_CMD_DEV_INIT);	
	status = ps2_enable_mouse(true);
	if (status < B_OK) {
		TRACE(("mouse_open(): cannot enable PS/2 mouse\n"));	
		return B_ERROR;
	}
		
	TRACE(("mouse_open(): mouse succesfully enabled\n"));
		
	// register interrupt handler
	status = install_io_interrupt_handler(INT_PS2_MOUSE, handle_mouse_interrupt, NULL, 0);

	return status;
}


static status_t 
mouse_close(void * cookie)
{
	TRACE(("mouse_close()\n"));
	ps2_enable_mouse(false);
	
	remove_io_interrupt_handler(INT_PS2_MOUSE, &handle_mouse_interrupt, NULL);
	
	atomic_and(&sOpenMask, 0);
	
	return B_OK;
}


static status_t
mouse_freecookie(void * cookie)
{
	return B_OK;
}


static status_t
mouse_read(void *cookie, off_t pos, void *buf, size_t *len)
{
	*len = 0;
	return EROFS;
}


static status_t 
mouse_write(void * cookie, off_t pos, const void *buf, size_t *len)
{
	*len = 0;
	return EROFS;
}


static status_t 
mouse_ioctl(void *cookie, uint32 op, void *buf, size_t len)
{
	mouse_pos *pos = (mouse_pos *)buf;
	switch (op) {
		case MS_NUM_EVENTS:
		{
			int32 count;
			TRACE(("PS2_GET_EVENT_COUNT\n"));
			get_sem_count(sMouseSem, &count);
			return count;
		}
		case MS_READ:
			TRACE(("PS2_GET_MOUSE_MOVEMENTS\n"));	
			return ps2_mouse_read(pos);
		
		default:
			TRACE(("unknown opcode: %ld\n", op));
			return EINVAL;
	}
}

/*
 * function structure used for file-op registration
 */
device_hooks ps2_mouse_hooks = {
	&mouse_open,
	&mouse_close,
	&mouse_freecookie,
	&mouse_ioctl,
	&mouse_read,
	&mouse_write,
	NULL,
	NULL,
	NULL,
	NULL
};


/////////////////////////////////////////////////////////////////////////
// initialization


status_t 
init_hardware()
{
	/* Mouse is detected in init_hardware */
	return B_OK;
}


const char **
publish_devices(void)
{
	static const char *devices[] = {
		DEVICE_NAME, 
		NULL
	};

	return devices;
}


device_hooks *
find_device(const char *name)
{
	if (!strcmp(name, DEVICE_NAME))
		return &ps2_mouse_hooks;

	return NULL;
}


status_t 
init_driver()
{
	status_t status;
	
	status = get_module(B_ISA_MODULE_NAME, (module_info **)&sIsa);
	if (status < B_OK) {
		TRACE(("Failed getting isa module: %s\n", strerror(status)));	
		return status;
	}
	
	// Check if there's a mouse, and disable it
	status = ps2_enable_mouse(false);
	if (status < B_OK) {
		TRACE(("can't find a ps2 mouse\n"));
		put_module(B_ISA_MODULE_NAME);
		return B_ERROR;
	}
		
	TRACE(("A PS/2 mouse has been successfully detected\n"));

	sMouseChain = cbuf_get_chain(MOUSE_HISTORY_SIZE);
	if (sMouseChain == NULL) {
		TRACE(("can't allocate cbuf chain\n"));
		put_module(B_ISA_MODULE_NAME);
		return B_ERROR;
	}
	
	// create the mouse semaphore, used for synchronization between
	// the interrupt handler and the read operation
	sMouseSem = create_sem(0, "ps2_mouse_sem");
	if (sMouseSem < 0) {
		TRACE(("failed creating PS/2 mouse semaphore!\n"));
		cbuf_free_chain(sMouseChain);
		put_module(B_ISA_MODULE_NAME);
		return sMouseSem;
	}
	
	set_sem_owner(sMouseSem, B_SYSTEM_TEAM);
	
	sOpenMask = 0;
	
	return B_OK;
}


void 
uninit_driver()
{
	cbuf_free_chain(sMouseChain);
	delete_sem(sMouseSem);
	put_module(B_ISA_MODULE_NAME);	
}
