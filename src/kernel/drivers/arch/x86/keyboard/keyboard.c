/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <kernel.h>
#include <debug.h>
#include <memheap.h>
#include <int.h>
#include <OS.h>
#include <string.h>
#include <lock.h>
#include <devfs.h>
#include <arch/cpu.h>
#include <Errors.h>

#include <arch/x86/keyboard.h>


#define LSHIFT  42
#define RSHIFT  54
#define SCRLOCK 70
#define NUMLOCK 69
#define CAPS    58
#define SYSREQ  55
#define F11     87
#define F12     88

#define LED_SCROLL 1
#define LED_NUM    2
#define LED_CAPS   4

static bool shift;
static int  leds;
static sem_id keyboard_sem;
static mutex keyboard_read_mutex;
static char keyboard_buf[1024];
static unsigned int head, tail;

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
	'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0, '\\', 'Z', 'X', 'C', 'V',
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

static void wait_for_output(void)
{
	while(in8(0x64) & 0x2)
		;
}

static void set_leds(void)
{
	wait_for_output();
	out8(0xed, 0x60);
	wait_for_output();
	out8(leds, 0x60);
}

static ssize_t _keyboard_read(void *_buf, size_t len)
{
	unsigned int saved_tail;
	char *buf = _buf;
	size_t copied_bytes = 0;
	size_t copy_len;
	int rc;

	if(len > sizeof(keyboard_buf) - 1)
		len = sizeof(keyboard_buf) - 1;

retry:
	// block here until data is ready
	rc = acquire_sem_etc(keyboard_sem, 1, B_CAN_INTERRUPT, 0);
	if(rc == ERR_SEM_INTERRUPTED) {
		return 0;
	}

	// critical section
	mutex_lock(&keyboard_read_mutex);

	saved_tail = tail;
	if(head == saved_tail) {
		mutex_unlock(&keyboard_read_mutex);
		goto retry;
	} else {
		// copy out of the buffer
		if(head < saved_tail)
			copy_len = min(len, saved_tail - head);
		else
			copy_len = min(len, sizeof(keyboard_buf) - head);
		memcpy(buf, &keyboard_buf[head], copy_len);
		copied_bytes = copy_len;
		head = (head + copy_len) % sizeof(keyboard_buf);
		if(head == 0 && saved_tail > 0 && copied_bytes < len) {
			// we wrapped around and have more bytes to read
			// copy the first part of the buffer
			copy_len = min(saved_tail, len - copied_bytes);
			memcpy(&buf[len], &keyboard_buf[0], copy_len);
			copied_bytes += copy_len;
			head = copy_len;
		}
	}
	if(head != saved_tail) {
		// we did not empty the keyboard queue
		release_sem_etc(keyboard_sem, 1, B_DO_NOT_RESCHEDULE);
	}

	mutex_unlock(&keyboard_read_mutex);

	return copied_bytes;
}

static void insert_in_buf(char c)
{
	unsigned int temp_tail = tail;

 	// see if the next char will collide with the head
	temp_tail++;
	temp_tail %= sizeof(keyboard_buf);
	if(temp_tail == head) {
		// buffer overflow, ditch this char
		return;
	}
	keyboard_buf[tail] = c;
	tail = temp_tail;
	release_sem_etc(keyboard_sem, 1, B_DO_NOT_RESCHEDULE);
}

static int handle_keyboard_interrupt(void* data)
{
	unsigned char key;
	int retval = INT_NO_RESCHEDULE;

	key = in8(0x60);
//	dprintf("handle_keyboard_interrupt: key = 0x%x\n", key);

	if(key & 0x80) {
		// keyup
		if(key == LSHIFT + 0x80 || key == RSHIFT + 0x80)
			shift = false;
	} else {
		switch(key) {
			case LSHIFT:
			case RSHIFT:
				shift = true;
				break;
			case CAPS:
				if(leds & LED_CAPS)
					leds &= ~LED_CAPS;
				else
					leds |= LED_CAPS;
				set_leds();
				break;
			case SCRLOCK:
				if(leds & LED_SCROLL) {
					leds &= ~LED_SCROLL;
					dbg_set_serial_debug(false);
				} else {
					leds |= LED_SCROLL;
					dbg_set_serial_debug(true);
				}
				set_leds();
				break;
			case NUMLOCK:
				if(leds & LED_NUM)
					leds &= ~LED_NUM;
				else
					leds |= LED_NUM;
				set_leds();
				break;
			case SYSREQ:
				panic("Keyboard Requested Halt\n");
				break;
			case F11:
				dbg_set_serial_debug(dbg_get_serial_debug()?false:true);
				break;
			case F12:
				reboot();
				break;
			default: {
				char ascii;

				if(shift)
					ascii = shifted_keymap[key];
				else
					if (leds & LED_CAPS)
						ascii = caps_keymap[key];
					else
						ascii = unshifted_keymap[key];

//					dprintf("ascii = 0x%x, '%c'\n", ascii, ascii);
				if(ascii != 0) {
					insert_in_buf(ascii);
					retval = INT_RESCHEDULE;
				} else {
//					dprintf("keyboard: unknown scan-code 0x%x\n",key);
				}
			}
		}
	}
	return retval;
}

static int keyboard_open(const char *name, uint32 flags, void * *cookie)
{
	*cookie = NULL;
	return 0;
}

static int keyboard_close(void * cookie)
{
	return 0;
}

static int keyboard_freecookie(void * cookie)
{
	return 0;
}

static int keyboard_seek(void * cookie, off_t pos, int st)
{
	return EPERM;
}

static ssize_t keyboard_read(void * cookie, off_t pos, void *buf, size_t *len)
{
	int rv;
	if (*len < 0)
		return 0;

	rv = _keyboard_read(buf, *len);
	if (rv < 0)
		return rv;
	*len = rv;
	return 0;
}

static ssize_t keyboard_write(void * cookie, off_t pos, const void *buf,  size_t *len)
{
	return ERR_VFS_READONLY_FS;
}

static int keyboard_ioctl(void * cookie, uint32 op, void *buf, size_t len)
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
//	NULL,
//	NULL
};

static int setup_keyboard(void)
{
	keyboard_sem = create_sem(0, "keyboard_sem");
	if(keyboard_sem < 0)
		panic("could not create keyboard sem!\n");

	if(mutex_init(&keyboard_read_mutex, "keyboard_read_mutex") < 0)
		panic("could not create keyboard read mutex!\n");

	shift = 0;
	leds = 0;
	// have the scroll lock reflect the state of serial debugging
	if(dbg_get_serial_debug())
		leds |= LED_SCROLL;
	set_leds();

	head = tail = 0;

	return 0;
}

int	keyboard_dev_init(kernel_args *ka)
{
	setup_keyboard();
	int_set_io_interrupt_handler(0x21,&handle_keyboard_interrupt, NULL);

	devfs_publish_device("keyboard", NULL, &keyboard_hooks);

	return 0;
}
