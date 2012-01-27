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

#include <midi_driver.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

#include "ice1712.h"
#include "ice1712_reg.h"
#include "io.h"
#include "util.h"
#include "debug.h"

extern generic_mpu401_module * mpu401;


void
ice_1712_midi_interrupt_op(int32 op, void *data)
{
	cpu_status status;
	uint8 int_status = 0;
	midi_dev *midi = (midi_dev *)data;

	if (op == B_MPU_401_ENABLE_CARD_INT) {
		status = lock();

		int_status = read_ccs_uint8(midi->card, CCS_INTERRUPT_MASK);
		int_status &= ~(midi->int_mask);
		write_ccs_uint8(midi->card, CCS_INTERRUPT_MASK, int_status);

		TRACE("B_MPU_401_ENABLE_CARD_INT: %s\n", midi->name);

		unlock(status);
	} else if (op == B_MPU_401_DISABLE_CARD_INT) {
		status = lock();

		int_status = read_ccs_uint8(midi->card, CCS_INTERRUPT_MASK);
		int_status |= midi->int_mask;
		write_ccs_uint8(midi->card, CCS_INTERRUPT_MASK, int_status);

		TRACE("B_MPU_401_DISABLE_CARD_INT: %s\n", midi->name);

		unlock(status);
	}

	TRACE("New mask status 0x%x\n", int_status);
}


status_t
ice_1712_midi_open(const char *name, uint32 flags, void **cookie)
{
	int midi, card;
	status_t ret = ENODEV;

	TRACE("**midi_open()\n");
	*cookie = NULL;

	for (card = 0; card < num_cards; card++) {
		for (midi = 0; midi < cards[card].config.nb_MPU401; midi++) {
			if (!strcmp(name, cards[card].midi_interf[midi].name)) {
				midi_dev *dev = &(cards[card].midi_interf[midi]);
				ret = (*mpu401->open_hook)(dev->mpu401device, flags, cookie);
				if (ret >= B_OK) {
					*cookie = dev->mpu401device;
				}
				break;
			}
		}
	}

	return ret;
}


status_t
ice_1712_midi_close(void* cookie)
{
	TRACE("**midi_close()\n");
	return (*mpu401->close_hook)(cookie);
}


status_t
ice_1712_midi_free(void* cookie)
{
	int midi, card;
	status_t ret;

	TRACE("**midi_free()\n");

	ret = (*mpu401->free_hook)(cookie);

	for (card = 0; card < num_cards; card++) {
		for (midi = 0; midi < cards[card].config.nb_MPU401; midi++) {
			if (cookie == cards[card].midi_interf[midi].mpu401device) {
				cards[card].midi_interf[midi].mpu401device = NULL;
				TRACE("Cleared %p card %d, midi %d\n", cookie, card, midi);
				break;
			}
		}
	}

	return ret;
}


status_t
ice_1712_midi_control(void* cookie,
	uint32 iop, void* data, size_t len)
{
	TRACE("**midi_control()\n");
	return (*mpu401->control_hook)(cookie, iop, data, len);
}


status_t
ice_1712_midi_read(void * cookie, off_t pos, void * ptr, size_t * nread)
{
	status_t ret = B_ERROR;

	ret = (*mpu401->read_hook)(cookie, pos, ptr, nread);
	//TRACE("**midi_read(%ld)\n", ret);

	return ret;
}


status_t
ice_1712_midi_write(void * cookie, off_t pos, const void * ptr,
        size_t * nwritten)
{
	status_t ret = B_ERROR;

	ret = (*mpu401->write_hook)(cookie, pos, ptr, nwritten);
	//TRACE("**midi_write(%ld)\n", ret);

	return ret;
}
