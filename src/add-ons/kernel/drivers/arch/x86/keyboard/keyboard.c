/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/


#include <KernelExport.h>
#include <Drivers.h>
#include <OS.h>
#include <drivers/ISA.h>

#include <string.h>
#include <lock.h>

#define DEVICE_NAME "keyboard"

#define TRACE_KEYBOARD 0
#if TRACE_KEYBOARD
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


enum keycodes {
	ESCAPE		= 1,

	LSHIFT		= 42,
	RSHIFT		= 54,
	CAPS_LOCK	= 58,
	NUM_LOCK	= 69,
	SCR_LOCK	= 70,
	SYS_REQ		= 55,

	F1			= 0x3b,
	F2, F3, F4, F5, F6,
	F7, F8, F9, F10,

	F11			= 87,
	F12			= 88,
};

#define LED_SCROLL	1
#define LED_NUM		2
#define LED_CAPS	4

int32 api_version = B_CUR_DRIVER_API_VERSION;

static bool shift;
static int leds;
static sem_id keyboard_sem;
static mutex keyboard_read_mutex;
static char keyboard_buf[1024];
static unsigned int head, tail;
static isa_module_info *gISA;

// stolen from nujeffos
const char unshifted_keymap[128] = {
	0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 8, '\t',
	'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0, 'a', 's',
	'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\', 'z', 'x', 'c', 'v',
	'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	'\\', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

const char shifted_keymap[128] = {
	0, 27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 8, '\t',
	'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0, 'A', 'S',
	'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0, '|', 'Z', 'X', 'C', 'V',
	'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

const char caps_keymap[128] = {
	0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 8, '\t',
	'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[', ']', '\n', 0, 'A', 'S',
	'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', '\'', '`', 0, '\\', 'Z', 'X', 'C', 'V',
	'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	'\\', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};


static void
wait_for_output(void)
{
	while (gISA->read_io_8(0x64) & 0x2)
		;
}


static void 
set_leds(void)
{
	wait_for_output();
	gISA->write_io_8(0x60, 0xed);
	wait_for_output();
	gISA->write_io_8(0x60, leds);
}


static void 
insert_in_buf(char c)
{
	unsigned int temp_tail = tail;

 	// see if the next char will collide with the head
	temp_tail++;
	temp_tail %= sizeof(keyboard_buf);
	if (temp_tail == head) {
		// buffer overflow, ditch this char
		return;
	}
	keyboard_buf[tail] = c;
	tail = temp_tail;
	release_sem_etc(keyboard_sem, 1, B_DO_NOT_RESCHEDULE);
}


static int32 
handle_keyboard_interrupt(void *data)
{
	unsigned char key;
	int32 retval = B_HANDLED_INTERRUPT;

	key = gISA->read_io_8(0x60);
	TRACE(("handle_keyboard_interrupt: key = 0x%x\n", key));

	if (key & 0x80) {
		// key up
		if (key == LSHIFT + 0x80 || key == RSHIFT + 0x80)
			shift = false;

		return B_HANDLED_INTERRUPT;
	}

	switch (key) {
		case LSHIFT:
		case RSHIFT:
			shift = true;
			break;
		case CAPS_LOCK:
			if (leds & LED_CAPS)
				leds &= ~LED_CAPS;
			else
				leds |= LED_CAPS;
			set_leds();
			break;
		case SCR_LOCK:
			if (leds & LED_SCROLL) {
				leds &= ~LED_SCROLL;
				//set_dprintf_enabled(false);
			} else {
				leds |= LED_SCROLL;
				//set_dprintf_enabled(true);
			}
			set_leds();
			break;
		case NUM_LOCK:
			if (leds & LED_NUM)
				leds &= ~LED_NUM;
			else
				leds |= LED_NUM;
			set_leds();
			break;

		/* the following code has two possibilities because of issues
		 * with BeBochs & BeOS (all special keys don't map to anything
		 * useful, and SYS_REQ does a screen dump in BeOS).
		 * ToDo: remove these key functions some day...
		 */
		case ESCAPE:
		case SYS_REQ:
			panic("Keyboard Requested Halt\n");
			break;
#if 0
		case F11:
			dbg_set_serial_debug(dbg_get_serial_debug() ? false : true);
			break;
#endif
		case F12:
			reboot();
			break;

		default: {
			char ascii;

			if (shift)
				ascii = shifted_keymap[key];
			else {
				if (leds & LED_CAPS)
					ascii = caps_keymap[key];
				else
					ascii = unshifted_keymap[key];
			}

			TRACE(("ascii = 0x%x, '%c'\n", ascii, ascii));

			if (ascii != 0) {
				insert_in_buf(ascii);
				retval = B_INVOKE_SCHEDULER;
			} else
				TRACE(("keyboard: unknown scan-code 0x%x\n", key));
		}
	}

	return retval;
}


//	#pragma mark -


static status_t 
keyboard_open(const char *name, uint32 flags, void **cookie)
{
	*cookie = NULL;
	return 0;
}


static status_t 
keyboard_close(void *cookie)
{
	return 0;
}


static status_t 
keyboard_freecookie(void *cookie)
{
	return 0;
}


static status_t 
keyboard_read(void *cookie, off_t pos, void *buffer, size_t *_length)
{
	unsigned int savedTail;
	size_t length = *_length;
	size_t copiedBytes = 0;
	int status;

	if (length < 0) {
		*_length = 0;
		return B_OK;
	}

	if (length > sizeof(keyboard_buf) - 1)
		length = sizeof(keyboard_buf) - 1;

retry:
	// block here until data is ready
	status = acquire_sem_etc(keyboard_sem, 1, B_CAN_INTERRUPT, 0);
	if (status == B_INTERRUPTED) {
		*_length = 0;
		return B_OK;
	}

	// critical section
	mutex_lock(&keyboard_read_mutex);

	savedTail = tail;
	if (head == savedTail) {
		// buffer queue is empty
		mutex_unlock(&keyboard_read_mutex);
		goto retry;
	} else {
		size_t copyLength;

		// copy out of the buffer
		if (head < savedTail)
			copyLength = min(length, savedTail - head);
		else
			copyLength = min(length, sizeof(keyboard_buf) - head);
		memcpy(buffer, &keyboard_buf[head], copyLength);
		copiedBytes = copyLength;

		head = (head + copyLength) % sizeof(keyboard_buf);
		if (head == 0 && savedTail > 0 && copiedBytes < length) {
			// we wrapped around and have more bytes to read
			// copy the first part of the buffer
			copyLength = min(savedTail, length - copiedBytes);
			memcpy((uint8 *)buffer + length, keyboard_buf, copyLength);
			copiedBytes += copyLength;
			head = copyLength;
		}
	}
	if (head != savedTail) {
		// we did not empty the keyboard queue
		release_sem_etc(keyboard_sem, 1, B_DO_NOT_RESCHEDULE);
	}

	mutex_unlock(&keyboard_read_mutex);

	*_length = copiedBytes;
	return B_OK;
}


static status_t 
keyboard_write(void *cookie, off_t pos, const void *buf,  size_t *len)
{
	return EROFS;
}


static status_t 
keyboard_ioctl(void *cookie, uint32 op, void *buf, size_t len)
{
	return EINVAL;
}


device_hooks keyboard_hooks = {
	&keyboard_open,
	&keyboard_close,
	&keyboard_freecookie,
	&keyboard_ioctl,
	&keyboard_read,
	&keyboard_write,
	NULL,
	NULL,
	NULL,
	NULL
};


//	#pragma mark -
/***** driver hooks *****/


status_t 
init_hardware()
{
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
		return &keyboard_hooks;

	return NULL;
}


status_t 
init_driver()
{
	if (get_module(B_ISA_MODULE_NAME, (module_info **)&gISA) < B_OK)
		panic("could not get ISA module\n");

	keyboard_sem = create_sem(0, "keyboard_sem");
	if (keyboard_sem < 0)
		panic("could not create keyboard sem!\n");

	if (mutex_init(&keyboard_read_mutex, "keyboard_read_mutex") < 0)
		panic("could not create keyboard read mutex!\n");

	shift = 0;
	leds = 0;

	// have the scroll lock reflect the state of serial debugging
#if 0
	if (dbg_get_serial_debug())
		leds |= LED_SCROLL;
#endif
	set_leds();

	head = tail = 0;

	install_io_interrupt_handler(0x01, &handle_keyboard_interrupt, NULL, 0);
	
	return B_OK;
}


void 
uninit_driver()
{
	remove_io_interrupt_handler(0x01, &handle_keyboard_interrupt, NULL);

	delete_sem(keyboard_sem);
	mutex_destroy(&keyboard_read_mutex);

	put_module(B_ISA_MODULE_NAME);
}
