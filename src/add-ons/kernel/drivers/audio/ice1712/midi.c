/*
 * ice1712 BeOS/Haiku Driver for VIA - VT1712 Multi Channel Audio Controller
 *
 * Copyright (c) 2002, Jerome Duval		(jerome.duval@free.fr)
 * Copyright (c) 2003, Marcus Overhagen	(marcus@overhagen.de)
 * Copyright (c) 2007, Jerome Leveque	(leveque.jerome@neuf.fr)
 *
 * All rights reserved
 * Distributed under the terms of the MIT license.
 */

#include <string.h>
#include <stdlib.h>
#include <signal.h>

#include "ice1712.h"
#include "io.h"
#include "midi_driver.h"
#include "util.h"

#include "debug.h"

extern generic_mpu401_module * mpu401;

void midi_interrupt_op(int32 op, void * data)
{
//	midi_dev * port = (midi_dev *)data;
	if (op == B_MPU_401_ENABLE_CARD_INT)
	{
		// sample code
		cpu_status cp;
//		ddprintf(("sonic_vibes: B_MPU_401_ENABLE_CARD_INT\n"));
		cp = disable_interrupts();
//		acquire_spinlock(&port->card->hardware);
//		increment_interrupt_handler(port->card);
//		set_direct(port->card, 0x01, 0x00, 0x80);
//		set_indirect(port->card, 0x2A, 0x04, 0xff);
//		release_spinlock(&port->card->hardware);
		restore_interrupts(cp);

		//real code
		cp = lock();
//		emuxki_reg_write_32(&(port->card->config), EMU_INTE,
//			  emuxki_reg_read_32(&(port->card->config), EMU_INTE) |
//			  EMU_INTE_MIDITXENABLE | EMU_INTE_MIDIRXENABLE );
		unlock(cp);
	}
	else if (op == B_MPU_401_DISABLE_CARD_INT)
	{
		// sample code
		/* turn off MPU interrupts */
		cpu_status cp;
//		ddprintf(("sonic_vibes: B_MPU_401_DISABLE_CARD_INT\n"));
		cp = disable_interrupts();
//		acquire_spinlock(&port->card->hardware);
//		set_direct(port->card, 0x01, 0x80, 0x80);*/
		/* remove interrupt handler if necessary */
//		decrement_interrupt_handler(port->card);
//		release_spinlock(&port->card->hardware);
		restore_interrupts(cp);

		//real code
//		cpu_status status;
		cp = lock();
//		emuxki_reg_write_32(&port->card->config, EMU_INTE,
//			  emuxki_reg_read_32(&port->card->config, EMU_INTE) &
//			  ~ (EMU_INTE_MIDITXENABLE | EMU_INTE_MIDIRXENABLE ) );
		unlock(cp);
	}
}

static status_t midi_open(const char *name, uint32 flags, void **cookie);
static status_t midi_close(void *cookie);
static status_t midi_free(void *cookie);
static status_t midi_control(void *cookie, uint32 op, void *data, size_t len);
static status_t midi_read(void *cookie, off_t pos, void *data, size_t *len);
static status_t midi_write(void *cookie, off_t pos, const void *data,
                        size_t *len);

device_hooks midi_hooks = {
    &midi_open,
    &midi_close,
    &midi_free,
    &midi_control,
    &midi_read,
    &midi_write,
    NULL,		/* select */
    NULL,		/* deselect */
    NULL,		/* readv */
    NULL		/* writev */
};

static status_t midi_open(const char * name, uint32 flags, void ** cookie)
{
	int i, ix, used_midi = -1;
	int ret;

	TRACE("midi_open()\n");

	*cookie = NULL;
	for (ix = 0; ix < num_cards; ix++) {
		for (i = 0; i < cards[ix].config.nb_MPU401; i++)
			if (!strcmp(name, cards[ix].midi_interf[i].name)) {
				used_midi = i;
				break;
			}
	}

	if (ix >= num_cards) {
		TRACE("bad device\n");
		return ENODEV;
	}

	TRACE("mpu401: %p  open(): %p  driver: %p\n", mpu401,
            mpu401->open_hook, cards[ix].midi_interf[used_midi].driver);
	ret = (*mpu401->open_hook)(cards[ix].midi_interf[used_midi].driver,
            flags, cookie);
	if (ret >= B_OK) {
		cards[ix].midi_interf[used_midi].cookie = *cookie;
		atomic_add(&cards[ix].midi_interf[used_midi].count, 1);
	}
	TRACE("mpu401: open returns %x / %p\n", ret, *cookie);
	return ret;

}


static status_t midi_close(void * cookie)
{
	TRACE("midi_close()\n");
	return (*mpu401->close_hook)(cookie);
}


static status_t midi_free(void * cookie)
{
	int i, ix;
	status_t f;
	TRACE("midi_free()\n");
	f = (*mpu401->free_hook)(cookie);
	for (ix = 0; ix < num_cards; ix++) {
		for (i = 0; i < cards[ix].config.nb_MPU401; i++)
			if (cards[ix].midi_interf[i].cookie == cookie) {
				if (atomic_add(&cards[ix].midi_interf[i].count, -1) == 1) {
					cards[ix].midi_interf[i].cookie = NULL;
					TRACE("cleared %p card %d\n", cookie, ix);
				}
				break;
			}
	}
	TRACE("midi_free() done\n");
	return f;
}


static status_t midi_control(void * cookie, uint32 iop, void * data, size_t len)
{
	return (*mpu401->control_hook)(cookie, iop, data, len);
}


static status_t midi_read(void * cookie, off_t pos, void * ptr, size_t * nread)
{
	return (*mpu401->read_hook)(cookie, pos, ptr, nread);
}


static status_t midi_write(void * cookie, off_t pos, const void * ptr,
        size_t * nwritten)
{
	return (*mpu401->write_hook)(cookie, pos, ptr, nwritten);
}


bool midi_interrupt(ice1712 *card)
{
	TRACE("midi_interrupt\n");
	if (!card->midi_interf[0].driver) {
//		kprintf("aiigh\n");
		return false;
	}
	return (*mpu401->interrupt_hook)(card->midi_interf[0].driver);
}
