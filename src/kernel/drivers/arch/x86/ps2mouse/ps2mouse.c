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

#include <kernel.h>
#include <Errors.h>
#include <memheap.h>
#include <int.h>
#include <debug.h>
#include <devfs.h>
#include <arch/int.h>
#include <string.h>

#define _PS2MOUSE_
#include <arch/x86/ps2mouse.h>

/////////////////////////////////////////////////////////////////////////
// interrupt

/*
 * handle_mouse_interrupt:
 * Interrupt handler for the mouse device. Called whenever the I/O
 * controller generates an interrupt for the PS/2 mouse. Reads mouse
 * information from the data port, and stores it, so it can be accessed
 * by read() operations. The full data is obtained using 3 consecutive
 * calls to the handler, each holds a different byte on the data port.
 * Parameters:
 * void*, ignored
 * Return value:
 * int, ???
 */
static int handle_mouse_interrupt(void* data)
{
   char c;
	static int next_input = 0;

	// read port
   c = in8(PS2_PORT_DATA);

	// put port contents in the appropriate data member, according to
	// current cycle
	switch(next_input) {
   // status byte
	case 0:
	   md_int.status = c;
		break;

	// x-axis change
   case 1:
      md_int.delta_x += c;
		break;

	// y-axis change
	case 2:
      md_int.delta_y += c;

		// check if someone is waiting to read data
		if(in_read) {
		   // copy data to read structure, and release waiting process
         memcpy(&md_read, &md_int, sizeof(mouse_data));
			memset(&md_int, 0, sizeof(mouse_data));
			in_read = false;
	      release_sem_etc(mouse_sem, 1, B_DO_NOT_RESCHEDULE);
		} // if
		break;
	} // switch

   next_input = (next_input + 1) % 3;
	return INT_NO_RESCHEDULE;
} // handle_mouse_interrupt

/////////////////////////////////////////////////////////////////////////
// file operations

/*
 * mouse_open:
 */
static int mouse_open(const char *name, uint32 flags, void **cookie)
{
	*cookie = NULL;
	return 0;
} // mouse_open

/*
 * mouse_close:
 */
static int mouse_close(void * cookie)
{
	return 0;
} // mouse_close

/*
 * mouse_freecookie:
 */
static int mouse_freecookie(void * cookie)
{
	return 0;
} // mouse_freecookie

/*
 * mouse_seek:
 */
static int mouse_seek(void * cookie, off_t pos, int st)
{
	return EPERM;
} // mouse_seek

/*
 * mouse_read:
 * Gets a mouse data packet.
 * Parameters:
 * void *, ignored
 * void*, pointer to a buffer that accepts the data
 * off_t, ignored
 * ssize_t, buffer size, must be at least the size of the data packet
 */
static ssize_t mouse_read(void * cookie, off_t pos, void* buf, size_t *len)
{
   // inform interrupt handler that data is being waited for
   in_read = true;

   // wait until there is data to read
	if(acquire_sem_etc(mouse_sem, 1, B_CAN_INTERRUPT, 0) ==
	   ERR_SEM_INTERRUPTED) {
		return 0;
	} // if

	// verify user's buffer is of the right size
	if(*len < PACKET_SIZE) {
		*len = 0;
		/* XXX - should return an error here */
		return 0;
	}

	// copy data to user's buffer
	((char*)buf)[0] = md_read.status;
	((char*)buf)[1] = md_read.delta_x;
	((char*)buf)[2] = md_read.delta_y;

	*len = PACKET_SIZE;
	return 0;
} // mouse_read

/*
 * mouse_write:
 */
static ssize_t mouse_write(void * cookie, off_t pos, const void *buf, size_t *len)
{
	*len = 0;
	return ERR_VFS_READONLY_FS;
} // mouse_write

/*
 * mouse_ioctl:
 */
static int mouse_ioctl(void * cookie, uint32 op, void *buf, size_t len)
{
	return ERR_INVALID_ARGS;
} // mouse_ioctl

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
//	NULL,
//	NULL
}; // ps2_mouse_hooks

/////////////////////////////////////////////////////////////////////////
// initialization

/*
 * wait_write_ctrl:
 * Wait until the control port is ready to be written. This requires that
 * the "Input buffer full" and "Output buffer full" bits will both be set
 * to 0.
 */
static void wait_write_ctrl()
{
	while(in8(PS2_PORT_CTRL) & 0x3);
} // wait_for_ctrl_output

/*
 * wait_write_data:
 * Wait until the data port is ready to be written. This requires that
 * the "Input buffer full" bit will be set to 0.
 */
static void wait_write_data()
{
	while(in8(PS2_PORT_CTRL) & 0x2);
} // wait_write_data

/*
 * wait_read_data:
 * Wait until the data port can be read from. This requires that the
 * "Output buffer full" bit will be set to 1.
 */
static void wait_read_data()
{
	while((in8(PS2_PORT_CTRL) & 0x1) == 0);
} // wait_read_data

/*
 * write_command_byte:
 * Writes a command byte to the data port of the PS/2 controller.
 * Parameters:
 * unsigned char, byte to write
 */
static void write_command_byte(unsigned char b)
{
	wait_write_ctrl();
	out8(PS2_CTRL_WRITE_CMD, PS2_PORT_CTRL);
	wait_write_data();
	out8(b, PS2_PORT_DATA);
} // write_command_byte

/*
 * write_aux_byte:
 * Writes a byte to the mouse device. Uses the control port to indicate
 * that the byte is sent to the auxiliary device (mouse), instead of the
 * keyboard.
 * Parameters:
 * unsigned char, byte to write
 */
static void write_aux_byte(unsigned char b)
{
   wait_write_ctrl();
	out8(PS2_CTRL_WRITE_AUX, PS2_PORT_CTRL);
	wait_write_data();
	out8(b, PS2_PORT_DATA);
} // write_aux_byte

/*
 * read_data_byte:
 * Reads a single byte from the data port.
 * Return value:
 * unsigned char, byte read
 */
static unsigned char read_data_byte()
{
   wait_read_data();
	return in8(PS2_PORT_DATA);
} // read_data_byte

/*
 * mouse_dev_init:
 * Called by the kernel to setup the device. Initializes the driver.
 * Parameters:
 * kernel_args*, ignored
 * Return value:
 * int, 0 if successful, negative error value otherwise
 */
int mouse_dev_init(kernel_args *ka)
{
   dprintf("Initializing PS/2 mouse\n");

   // init device driver
	memset(&md_int, 0, sizeof(mouse_data));

	// register interrupt handler
	int_set_io_interrupt_handler(INT_BASE + INT_PS2_MOUSE,
	   &handle_mouse_interrupt, NULL);

	// must enable the cascade interrupt
	arch_int_enable_io_interrupt(INT_BASE + INT_CASCADE);

	// enable auxilary device, IRQs and PS/2 mouse
   write_command_byte(PS2_CMD_DEV_INIT);
	write_aux_byte(PS2_CMD_ENABLE_MOUSE);

	// controller should send ACK if mouse was detected
	if(read_data_byte() != PS2_RES_ACK) {
	   dprintf("No PS/2 mouse found\n");
		return -1;
	} // if

	dprintf("A PS/2 mouse has been successfully detected\n");

	// create the mouse semaphore, used for synchronization between
	// the interrupt handler and the read() operation
	mouse_sem = create_sem(0, "ps2_mouse_sem");
	if(mouse_sem < 0)
	   panic("failed to create PS/2 mouse semaphore!\n");

	// register device file-system like operations
	devfs_publish_device("ps2mouse", NULL, &ps2_mouse_hooks);

	return 0;
} // mouse_dev_init
