/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "cm_private.h"
#include <string.h>

#if !defined(_KERNEL_EXPORT_H)
#include <KernelExport.h>
#endif /* _KERNEL_EXPORT_H */


static status_t mux_open(const char *name, uint32 flags, void **cookie);
static status_t mux_close(void *cookie);
static status_t mux_free(void *cookie);
static status_t mux_control(void *cookie, uint32 op, void *data, size_t len);
static status_t mux_read(void *cookie, off_t pos, void *data, size_t *len);
static status_t mux_write(void *cookie, off_t pos, const void *data, size_t *len);

device_hooks mux_hooks = {
    &mux_open,
    &mux_close,
    &mux_free,
    &mux_control,
    &mux_read,
    &mux_write,
    NULL,		/* select */
    NULL,		/* deselect */
    NULL,		/* readv */
    NULL		/* writev */
};


typedef struct {
	int port_l;
	int port_r;
	int minval;
	int maxval;
	int leftshift;
	int mask;
	char name[B_OS_NAME_LENGTH];
} mux_info;

const mux_info the_muxes[] = {
	{ 0, 1, 1, 7, 5, 0xe0, "Sampling input" },
	{ 0, -1, 0, 1, 4, 0x10, "Mic +20dB selection" },
	{ 0x2a, -1, 0, 1, 0, 0x01, "MIDI output to synth" },
	{ 0x2a, -1, 0, 1, 1, 0x02, "MIDI input to synth" },
	{ 0x2a, -1, 0, 1, 2, 0x04, "MIDI output to port" },
};

static const uchar unmap_input[] = {
  0,
  CMEDIA_PCI_INPUT_CD,
  CMEDIA_PCI_INPUT_DAC,
  CMEDIA_PCI_INPUT_AUX2,
  CMEDIA_PCI_INPUT_LINE,
  CMEDIA_PCI_INPUT_AUX1,
  CMEDIA_PCI_INPUT_MIC,
  CMEDIA_PCI_INPUT_MIX_OUT
};

static uchar
map_input(uchar val) {
  int i;
  for (i = 0; i < 8; i++)
	if (unmap_input[i] == val)
	  return i;
  return 0;
}


static status_t
mux_open(
	const char * name,
	uint32 flags,
	void ** cookie)
{
	int ix;
	/* mux_dev * plex = NULL; */

	ddprintf(("cmedia_pci: mux_open()\n"));

	*cookie = NULL;
	for (ix=0; ix<num_cards; ix++) {
		if (!strcmp(name, cards[ix].mux.name)) {
			break;
		}
	}
	if (ix == num_cards) {
		return ENODEV;
	}

	atomic_add(&cards[ix].mux.open_count, 1);
	cards[ix].mux.card = &cards[ix];
	*cookie = &cards[ix].mux;

	return B_OK;
}


static status_t
mux_close(
	void * cookie)
{
	mux_dev * plex = (mux_dev *)cookie;

	atomic_add(&plex->open_count, -1);

	return B_OK;
}


static status_t
mux_free(
	void * cookie)
{
	ddprintf(("cmedia_pci: mux_free()\n"));

	if (((mux_dev *)cookie)->open_count != 0) {
		dprintf("cmedia_pci: mux open_count is bad in mux_free()!\n");
	}
	return B_OK;	/* already done in close */
}


static int
get_mux_value(
	cmedia_pci_dev * card,
	int ix)
{
	uchar val;
	if (ix < 0) {
		return -1;
	}
	if (ix > 4) {
		return -1;
	}
	val = get_indirect(card, the_muxes[ix].port_l);
	val &= the_muxes[ix].mask;
	val >>= the_muxes[ix].leftshift;
	if (ix == CMEDIA_PCI_INPUT_MUX)
	  return unmap_input[val];
	return val;
}


static int
gather_info(
	mux_dev * mux,
	cmedia_pci_routing * data,
	int count)
{
	int ix;
	cpu_status cp;

	cp = disable_interrupts();
	acquire_spinlock(&mux->card->hardware);

	for (ix=0; ix<count; ix++) {
		data[ix].value = get_mux_value(mux->card, data[ix].selector);
		if (data[ix].value < 0) {
			break;
		}
	}

	release_spinlock(&mux->card->hardware);
	restore_interrupts(cp);

	return ix;
}


static status_t
set_mux_value(
	cmedia_pci_dev * card,
	int selector,
	int value)
{
	ddprintf(("set_mux_value(%d,%d)\n", selector, value));
	if (selector < 0 || selector > 4) {
		ddprintf(("selector EINVAL\n"));
		return EINVAL;
	}
	if (selector == CMEDIA_PCI_INPUT_MUX)
	    value = map_input(value);
	if (value < the_muxes[selector].minval ||
		value > the_muxes[selector].maxval) {
		ddprintf(("value EINVAL\n"));
		return EINVAL;
	}
	set_indirect(card, the_muxes[selector].port_l, 
		(value << the_muxes[selector].leftshift),
		the_muxes[selector].mask);
	if (the_muxes[selector].port_r > -1) {
		set_indirect(card, the_muxes[selector].port_r, 
			(value << the_muxes[selector].leftshift),
			the_muxes[selector].mask);
	}
	return B_OK;
}


static int
disperse_info(
	mux_dev * mux,
	cmedia_pci_routing * data,
	int count)
{
	int ix;
	cpu_status cp;

	cp = disable_interrupts();
	acquire_spinlock(&mux->card->hardware);

	for (ix=0; ix<count; ix++) {
		if (set_mux_value(mux->card, data[ix].selector, data[ix].value) < B_OK) {
			break;
		}
	}

	release_spinlock(&mux->card->hardware);
	restore_interrupts(cp);

	return ix;
}


static status_t
mux_control(
	void * cookie,
	uint32 iop,
	void * data,
	size_t len)
{
	mux_dev * plex = (mux_dev *)cookie;
	status_t err = B_OK;

	ddprintf(("cmedia_pci: mux_control()\n")); /* slow printing */

	switch (iop) {
	case B_ROUTING_GET_VALUES:
		((cmedia_pci_routing_cmd *)data)->count = 
			gather_info(plex, ((cmedia_pci_routing_cmd *)data)->data, 
				((cmedia_pci_routing_cmd *)data)->count);
		break;
	case B_ROUTING_SET_VALUES:
		((cmedia_pci_routing_cmd *)data)->count = 
			disperse_info(plex, ((cmedia_pci_routing_cmd *)data)->data, 
				((cmedia_pci_routing_cmd *)data)->count);
		break;
	default:
		err = B_BAD_VALUE;
		break;
	}
	return err;
}


static status_t
mux_read(
	void * cookie,
	off_t pos,
	void * data,
	size_t * nread)
{
	return EPERM;
}


static status_t
mux_write(
	void * cookie,
	off_t pos,
	const void * data,
	size_t * nwritten)
{
	return EPERM;
}

