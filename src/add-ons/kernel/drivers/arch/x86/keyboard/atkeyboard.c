/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/


#include <KernelExport.h>
#include <Drivers.h>
#include <OS.h>
#include <ISA.h>

#include "cbuf_adapter.h"
#include <string.h>
//#include <lock.h>

#include "kb_mouse_driver.h"

#define DEVICE_NAME "input/keyboard/at/0"

#define TRACE_KEYBOARD 1
#if TRACE_KEYBOARD
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


#define LED_SCROLL	1
#define LED_NUM		2
#define LED_CAPS	4

int32 api_version = B_CUR_DRIVER_API_VERSION;

static sem_id keyboard_sem;
//static mutex keyboard_read_mutex;
static cbuf *keyboard_buf;
static isa_module_info *sIsa;
static at_kbd_io sKeyInfo;

static int32 sOpenMask;

static void
wait_for_output(void)
{
	while (sIsa->read_io_8(0x64) & 0x2)
		;
}


static void 
set_leds(led_info *ledInfo)
{
	int leds = 0;
	if (ledInfo->scroll_lock)
		leds |= LED_SCROLL;
	if (ledInfo->num_lock)
		leds |= LED_NUM;
	if (ledInfo->caps_lock)
		leds |= LED_CAPS;
		
	wait_for_output();
	sIsa->write_io_8(0x60, 0xed);
	wait_for_output();
	sIsa->write_io_8(0x60, leds);
}


static int32 
handle_keyboard_interrupt(void *data)
{
	unsigned char key;
	
	key = sIsa->read_io_8(0x60);
	TRACE(("handle_keyboard_interrupt: key = 0x%x\n", key));

	if (key == 0xE0) {
		// Extended key. Handle it in some way
		return B_HANDLED_INTERRUPT;
	}
	
	if (key & 0x80) {
		sKeyInfo.is_keydown = false;
		
	}

	sKeyInfo.timestamp = system_time();
	sKeyInfo.scancode = key;
	
	// TODO: Check return value
	cbuf_memcpy_to_chain(keyboard_buf, 0, (void *)&sKeyInfo, sizeof(sKeyInfo));
	release_sem_etc(keyboard_sem, 1, B_DO_NOT_RESCHEDULE);
	
	// Reset this so the next event defaults to keydown
	sKeyInfo.is_keydown = true;
	
	return B_INVOKE_SCHEDULER;
}


static status_t
read_keyboard_packet(at_kbd_io *buffer)
{
	status_t status;
	status = acquire_sem_etc(keyboard_sem, 1, B_CAN_INTERRUPT, 0);
	if (status < B_OK)
		return status;
	
	status = cbuf_memcpy_from_chain((void *)buffer, keyboard_buf, 0, sizeof(at_kbd_io));
	if (status < B_OK)
		TRACE(("read_keyboard_packet(): error reading packet: %s\n", strerror(status)));

	TRACE(("scancode: %x, keydown: %s\n", buffer->scancode, buffer->is_keydown ? "true" : "false"));
	return status;
}


//	#pragma mark -


static status_t 
keyboard_open(const char *name, uint32 flags, void **cookie)
{
	TRACE(("keyboard open()\n"));
	if (atomic_or(&sOpenMask, 1) != 0)
		return B_BUSY;

	*cookie = NULL;
	return B_OK;
}


static status_t 
keyboard_close(void *cookie)
{
	atomic_and(&sOpenMask, 0);
	return B_OK;
}


static status_t 
keyboard_freecookie(void *cookie)
{
	return B_OK;
}


static status_t 
keyboard_read(void *cookie, off_t pos, void *buffer, size_t *_length)
{
	TRACE(("keyboard read()\n"));
	*_length = 0;
	return EROFS;
}


static status_t 
keyboard_write(void *cookie, off_t pos, const void *buf,  size_t *len)
{
	TRACE(("keyboard write()\n"));
	*len = 0;
	return EROFS;
}


static status_t 
keyboard_ioctl(void *cookie, uint32 op, void *buf, size_t len)
{
	TRACE(("keyboard ioctl()\n"));
	switch (op) {
		case KB_READ:
			TRACE(("KB_READ\n"));
			return read_keyboard_packet((at_kbd_io *)buf);
		case KB_SET_LEDS:
			set_leds((led_info *)buf);
			TRACE(("KB_SET_LEDS\n"));
			return B_OK;
		case KB_SET_KEY_REPEATING:
		case KB_SET_KEY_NONREPEATING:
			TRACE(("ioctl 0x%x not implemented yet, returning B_OK\n", op));
			return B_OK;
		default:
			TRACE(("invalid ioctl 0x%x\n", op));
			return EINVAL;
	}
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
	TRACE(("keyboard: init_driver()\n"));
	if (get_module(B_ISA_MODULE_NAME, (module_info **)&sIsa) < B_OK)
		panic("could not get ISA module\n");

	keyboard_sem = create_sem(0, "keyboard_sem");
	if (keyboard_sem < 0)
		panic("could not create keyboard sem!\n");

	//if (mutex_init(&keyboard_read_mutex, "keyboard_read_mutex") < 0)
	//	panic("could not create keyboard read mutex!\n");

	keyboard_buf = cbuf_get_chain(1024);
	if (keyboard_buf == NULL)
		panic("could not create keyboard cbuf chain!\n");

	install_io_interrupt_handler(1, &handle_keyboard_interrupt, NULL, 0);
	
	sKeyInfo.is_keydown = true;
	
	sOpenMask = 0;

	return B_OK;
}


void 
uninit_driver()
{
	remove_io_interrupt_handler(1, &handle_keyboard_interrupt, NULL);

	cbuf_free_chain(keyboard_buf);
	delete_sem(keyboard_sem);
	//mutex_destroy(&keyboard_read_mutex);

	put_module(B_ISA_MODULE_NAME);
}
