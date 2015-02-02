/*
 * Copyright 2004-2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval, jerome.duval@free.fr
 *		Marcus Overhagen, marcus@overhagen.de
 *		Jérôme Lévêque, leveque.jerome@gmail.com
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
extern int32 num_cards;
extern ice1712 cards[NUM_CARDS];

void ice1712Midi_interrupt(int32 op, void *data);


void
ice1712Midi_interrupt(int32 op, void *data)
{
	cpu_status status;
	uint8 int_status = 0;
	ice1712Midi *midi = (ice1712Midi*)data;

	if (op == B_MPU_401_ENABLE_CARD_INT) {
		status = lock();

		int_status = read_ccs_uint8(midi->card, CCS_INTERRUPT_MASK);
		int_status &= ~(midi->int_mask);
		write_ccs_uint8(midi->card, CCS_INTERRUPT_MASK, int_status);

		ITRACE("B_MPU_401_ENABLE_CARD_INT: %s\n", midi->name);

		unlock(status);
	} else if (op == B_MPU_401_DISABLE_CARD_INT) {
		status = lock();

		int_status = read_ccs_uint8(midi->card, CCS_INTERRUPT_MASK);
		int_status |= midi->int_mask;
		write_ccs_uint8(midi->card, CCS_INTERRUPT_MASK, int_status);

		ITRACE("B_MPU_401_DISABLE_CARD_INT: %s\n", midi->name);

		unlock(status);
	}

	ITRACE("New mask status 0x%x\n", int_status);
}


static status_t
ice1712Midi_open(const char *name, uint32 flags, void **cookie)
{
	int midi, card;
	status_t ret = ENODEV;

	ITRACE("**midi_open()\n");
	*cookie = NULL;

	for (card = 0; card < num_cards; card++) {
		for (midi = 0; midi < cards[card].config.nb_MPU401; midi++) {
			if (!strcmp(name, cards[card].midiItf[midi].name)) {
				ice1712Midi *dev = &(cards[card].midiItf[midi]);
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


static status_t
ice1712Midi_close(void* cookie)
{
	ITRACE("**midi_close()\n");
	return (*mpu401->close_hook)(cookie);
}


static status_t
ice1712Midi_free(void* cookie)
{
	int midi, card;
	status_t ret;

	ITRACE("**midi_free()\n");

	ret = (*mpu401->free_hook)(cookie);

	for (card = 0; card < num_cards; card++) {
		for (midi = 0; midi < cards[card].config.nb_MPU401; midi++) {
			if (cookie == cards[card].midiItf[midi].mpu401device) {
				cards[card].midiItf[midi].mpu401device = NULL;
				ITRACE("Cleared %p card %d, midi %d\n", cookie, card, midi);
				break;
			}
		}
	}

	return ret;
}


static status_t
ice1712Midi_control(void* cookie,
	uint32 iop, void* data, size_t len)
{
	ITRACE("**midi_control()\n");
	return (*mpu401->control_hook)(cookie, iop, data, len);
}


static status_t
ice1712Midi_read(void * cookie, off_t pos, void * ptr, size_t * nread)
{
	status_t ret = B_ERROR;

	ret = (*mpu401->read_hook)(cookie, pos, ptr, nread);
	ITRACE_VV("**midi_read: %" B_PRIi32 "\n", ret);

	return ret;
}


static status_t
ice1712Midi_write(void * cookie, off_t pos, const void * ptr,
		size_t * nwritten)
{
	status_t ret = B_ERROR;

	ret = (*mpu401->write_hook)(cookie, pos, ptr, nwritten);
	ITRACE_VV("**midi_write: %" B_PRIi32 "\n", ret);

	return ret;
}


device_hooks ice1712Midi_hooks =
{
	ice1712Midi_open,
	ice1712Midi_close,
	ice1712Midi_free,
	ice1712Midi_control,
	ice1712Midi_read,
	ice1712Midi_write,
	NULL,
	NULL,
	NULL,
	NULL
};
