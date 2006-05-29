/*
 * Emuxki BeOS Driver for Creative Labs SBLive!/Audigy series
 *
 * Copyright (c) 2002, Jerome Duval (jerome.duval@free.fr)
 *
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice, 
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation 
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR 
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS 
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, 
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * Original code : Copyright 1999, Be Incorporated.   All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License.
*/

#include <string.h>
#include <stdlib.h>
#include <signal.h>

#include "emuxki.h"
#include "io.h"
#include "debug.h"
#include "midi_driver.h"
#include "util.h"

extern generic_mpu401_module * mpu401;

void
midi_interrupt_op(
	int32 op,
	void * data)
{
	cpu_status status;
	// dprint seems to be disabled here, will have to enable it to trace
	 
	midi_dev * port = (midi_dev *)data;

	LOG(("mpu401:midi_interrupt_op %x\n",op));
	LOG(("port = %p\n", port));
	if (op == B_MPU_401_ENABLE_CARD_INT) {
		/* turn on MPU interrupts */
		LOG(("emuxki: B_MPU_401_ENABLE_CARD_INT\n"));
		status = lock();
		emuxki_reg_write_32(&(port->card->config), EMU_INTE,
			  emuxki_reg_read_32(&(port->card->config), EMU_INTE) | EMU_INTE_MIDIRXENABLE );
		unlock(status);
		LOG(("INTE address: %x\n",&port->card->config));
	}
	else if (op == B_MPU_401_DISABLE_CARD_INT) {
		/* turn off MPU interrupts */
		LOG(("emuxki: B_MPU_401_DISABLE_CARD_INT\n"));
		status = lock();
		emuxki_reg_write_32(&port->card->config, EMU_INTE,
			  emuxki_reg_read_32(&port->card->config, EMU_INTE) &  ~ EMU_INTE_MIDIRXENABLE);
		unlock(status);
	}

	LOG(("midi_interrupt_op() done\n"));
}

static status_t midi_open(const char *name, uint32 flags, void **cookie);
static status_t midi_close(void *cookie);
static status_t midi_free(void *cookie);
static status_t midi_control(void *cookie, uint32 op, void *data, size_t len);
static status_t midi_read(void *cookie, off_t pos, void *data, size_t *len);
static status_t midi_write(void *cookie, off_t pos, const void *data, size_t *len);


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

static status_t
midi_open(
	const char * name,
	uint32 flags,
	void ** cookie)
{
	int ix;
	int ret;

	LOG(("midi_open()\n"));

	*cookie = NULL;
	for (ix=0; ix<num_cards; ix++) {
		if (!strcmp(name, cards[ix].midi.name)) {
			break;
		}
	}
	if (ix >= num_cards) {
		LOG(("bad device\n"));
		return ENODEV;
	}

	LOG(("mpu401: %p  open(): %p  driver: %p\n", mpu401, mpu401->open_hook, cards[ix].midi.driver));
	ret = (*mpu401->open_hook)(cards[ix].midi.driver, flags, cookie);
	if (ret >= B_OK) {
		cards[ix].midi.cookie = *cookie;
		atomic_add(&cards[ix].midi.count, 1);
	}
	LOG(("mpu401: open returns %x / %p\n", ret, *cookie));
	return ret;
}


static status_t
midi_close(
	void * cookie)
{
	LOG(("midi_close()\n"));
	return (*mpu401->close_hook)(cookie);
}


static status_t
midi_free(
	void * cookie)
{
	int ix;
	status_t f;
	LOG(("midi_free()\n"));
	f = (*mpu401->free_hook)(cookie);
	for (ix=0; ix<num_cards; ix++) {
		if (cards[ix].midi.cookie == cookie) {
			if (atomic_add(&cards[ix].midi.count, -1) == 1) {
				cards[ix].midi.cookie = NULL;
				LOG(("cleared %p card %d\n", cookie, ix));
			}
			break;
		}
	}
	LOG(("midi_free() done\n"));
	return f;
}


static status_t
midi_control(
	void * cookie,
	uint32 iop,
	void * data,
	size_t len)
{
	return (*mpu401->control_hook)(cookie, iop, data, len);
}


static status_t
midi_read(
	void * cookie,
	off_t pos,
	void * ptr,
	size_t * nread)
{
	return (*mpu401->read_hook)(cookie, pos, ptr, nread);
}


static status_t
midi_write(
	void * cookie, 
	off_t pos,
	const void * ptr, 
	size_t * nwritten)
{
	return (*mpu401->write_hook)(cookie, pos, ptr, nwritten);
}


bool
midi_interrupt(emuxki_dev *card)
{
	TRACE(("midi_interrupt\n"));
	if (!card->midi.driver)  {
		dprintf("aiigh\n");
		return false;
	}
		
	return (*mpu401->interrupt_hook)(card->midi.driver);
}

