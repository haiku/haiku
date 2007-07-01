/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <string.h>
#include <stdlib.h>
#include <signal.h>

#include "cm_private.h"


extern void dump_card(cmedia_pci_dev * card);

#if !defined(_KERNEL_EXPORT_H)
#include <KernelExport.h>
#endif /* _KERNEL_EXPORT_H */

#define MIDI_ACTIVE_SENSE 0xfe
#define SNOOZE_GRANULARITY 500

extern generic_mpu401_module * mpu401;

void
midi_interrupt_op(
	int32 op,
	void * data)
{
	midi_dev * port = (midi_dev *)data;
	ddprintf(("port = %p\n", port));
	if (op == B_MPU_401_ENABLE_CARD_INT) {
		cpu_status cp;
		ddprintf(("cmedia_pci: B_MPU_401_ENABLE_CARD_INT\n"));
		cp = disable_interrupts();
		acquire_spinlock(&port->card->hardware);
		increment_interrupt_handler(port->card);
		set_direct(port->card, 0x01, 0x00, 0x80);
		set_indirect(port->card, 0x2A, 0x04, 0xff);
		release_spinlock(&port->card->hardware);
		restore_interrupts(cp);
	}
	else if (op == B_MPU_401_DISABLE_CARD_INT) {
		/* turn off MPU interrupts */
		cpu_status cp;
		ddprintf(("cmedia_pci: B_MPU_401_DISABLE_CARD_INT\n"));
		cp = disable_interrupts();
		acquire_spinlock(&port->card->hardware);
		set_direct(port->card, 0x01, 0x80, 0x80);
		/* remove interrupt handler if necessary */
		decrement_interrupt_handler(port->card);
		release_spinlock(&port->card->hardware);
		restore_interrupts(cp);
	}
	ddprintf(("cmedia_pci: midi_interrupt_op() done\n"));
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

	ddprintf(("cmedia_pci: midi_open()\n"));

	*cookie = NULL;
	for (ix=0; ix<num_cards; ix++) {
		if (!strcmp(name, cards[ix].midi.name)) {
			break;
		}
	}
	if (ix >= num_cards) {
		ddprintf(("bad device\n"));
		return ENODEV;
	}

	ddprintf(("cmedia_pci: mpu401: %p  open(): %p  driver: %p\n", mpu401, mpu401->open_hook, cards[ix].midi.driver));
	ret = (*mpu401->open_hook)(cards[ix].midi.driver, flags, cookie);
	if (ret >= B_OK) {
		cards[ix].midi.cookie = *cookie;
		atomic_add(&cards[ix].midi.count, 1);
	}
	ddprintf(("cmedia_pci: mpu401: open returns %x / %p\n", ret, *cookie));
	return ret;
}


static status_t
midi_close(
	void * cookie)
{
	ddprintf(("cmedia_pci: midi_close()\n"));
	return (*mpu401->close_hook)(cookie);
}


static status_t
midi_free(
	void * cookie)
{
	int ix;
	status_t f;
	ddprintf(("cmedia_pci: midi_free()\n"));
	f = (*mpu401->free_hook)(cookie);
	for (ix=0; ix<num_cards; ix++) {
		if (cards[ix].midi.cookie == cookie) {
			if (atomic_add(&cards[ix].midi.count, -1) == 1) {
				cards[ix].midi.cookie = NULL;
				ddprintf(("cleared %p card %d\n", cookie, ix));
			}
			break;
		}
	}
	ddprintf(("cmedia_pci: midi_free() done\n"));
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
midi_interrupt(
	cmedia_pci_dev * dev)
{
	if (!dev->midi.driver)  {
//		kprintf("aiigh\n");
		return false;
	}
	
	return (*mpu401->interrupt_hook)(dev->midi.driver);
}

