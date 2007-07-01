/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "cm_private.h"
#include <string.h>

#if !defined(_KERNEL_EXPORT_H)
#include <KernelExport.h>
#endif /* _KERNEL_EXPORT_H */


static status_t mixer_open(const char *name, uint32 flags, void **cookie);
static status_t mixer_close(void *cookie);
static status_t mixer_free(void *cookie);
static status_t mixer_control(void *cookie, uint32 op, void *data, size_t len);
static status_t mixer_read(void *cookie, off_t pos, void *data, size_t *len);
static status_t mixer_write(void *cookie, off_t pos, const void *data, size_t *len);

device_hooks mixer_hooks = {
    &mixer_open,
    &mixer_close,
    &mixer_free,
    &mixer_control,
    &mixer_read,
    &mixer_write,
    NULL,		/* select */
    NULL,		/* deselect */
    NULL,		/* readv */
    NULL		/* writev */
};


typedef struct {
	int selector;
	int port;
	float div;
	float sub;
	int minval;
	int maxval;
	int leftshift;
	int mask;
	int mutemask;
} mixer_info;

/* mute is special -- when it's 0x01, it means enable... */
mixer_info the_mixers[] = {
  {CMEDIA_PCI_LEFT_ADC_INPUT_G,			0,	   1.5,  0.0, 0, 15, 0, 0x0f, 0x00},
  {CMEDIA_PCI_RIGHT_ADC_INPUT_G,		1,	   1.5,  0.0, 0, 15, 0, 0x0f, 0x00},
  {CMEDIA_PCI_LEFT_AUX1_LOOPBACK_GAM,	2,	  -1.5, 12.0, 0, 31, 0, 0x1f, 0x80},
  {CMEDIA_PCI_RIGHT_AUX1_LOOPBACK_GAM,	3,	  -1.5, 12.0, 0, 31, 0, 0x1f, 0x80},
  {CMEDIA_PCI_LEFT_CD_LOOPBACK_GAM,		4,	  -1.5, 12.0, 0, 31, 0, 0x1f, 0x80},
  {CMEDIA_PCI_RIGHT_CD_LOOPBACK_GAM,	5,	  -1.5, 12.0, 0, 31, 0, 0x1f, 0x80},
  {CMEDIA_PCI_LEFT_LINE_LOOPBACK_GAM,	6,	  -1.5, 12.0, 0, 31, 0, 0x1f, 0x80},
  {CMEDIA_PCI_RIGHT_LINE_LOOPBACK_GAM,	7,	  -1.5, 12.0, 0, 31, 0, 0x1f, 0x80},
  {CMEDIA_PCI_MIC_LOOPBACK_GAM,			8,	  -1.5, 12.0, 0, 31, 0, 0x1f, 0x80},
  {CMEDIA_PCI_LEFT_SYNTH_OUTPUT_GAM,	0xa,  -1.5, 12.0, 0, 31, 0, 0x1f, 0x80},
  {CMEDIA_PCI_RIGHT_SYNTH_OUTPUT_GAM,	0xb,  -1.5, 12.0, 0, 31, 0, 0x1f, 0x80},
  {CMEDIA_PCI_LEFT_AUX2_LOOPBACK_GAM,	0xc,  -1.5, 12.0, 0, 31, 0, 0x1f, 0x80},
  {CMEDIA_PCI_RIGHT_AUX2_LOOPBACK_GAM,	0xd,  -1.5, 12.0, 0, 31, 0, 0x1f, 0x80},
  {CMEDIA_PCI_LEFT_MASTER_VOLUME_AM,	0xe,  -1.5,  0.0, 0, 31, 0, 0x1f, 0x80},
  {CMEDIA_PCI_RIGHT_MASTER_VOLUME_AM,	0xf,  -1.5,  0.0, 0, 31, 0, 0x1f, 0x80},
  {CMEDIA_PCI_LEFT_PCM_OUTPUT_GAM,		0x10, -1.5,  0.0, 0, 63, 0, 0x3f, 0x80},
  {CMEDIA_PCI_RIGHT_PCM_OUTPUT_GAM,		0x11, -1.5,  0.0, 0, 63, 0, 0x3f, 0x80},
  {CMEDIA_PCI_DIGITAL_LOOPBACK_AM,		0x16, -1.5,  0.0, 0, 63, 2, 0xfc, 0x01},
};
#define N_MIXERS (sizeof(the_mixers) / sizeof(the_mixers[0]))

static int
map_mixer(int selector) {
  int i;
  for (i = 0; i < N_MIXERS; i++)
	if (the_mixers[i].selector == selector)
	  return i;
  return -1;
}


static status_t
mixer_open(
	const char * name,
	uint32 flags,
	void ** cookie)
{
	int ix;
	/* mixer_dev * it = NULL; */

	ddprintf(("cmedia_pci: mixer_open()\n"));

	*cookie = NULL;
	for (ix=0; ix<num_cards; ix++) {
		if (!strcmp(name, cards[ix].mixer.name)) {
			break;
		}
	}
	if (ix == num_cards) {
		return ENODEV;
	}

	atomic_add(&cards[ix].mixer.open_count, 1);
	cards[ix].mixer.card = &cards[ix];
	*cookie = &cards[ix].mixer;

	return B_OK;
}


static status_t
mixer_close(
	void * cookie)
{
	mixer_dev * it = (mixer_dev *)cookie;

	atomic_add(&it->open_count, -1);

	return B_OK;
}


static status_t
mixer_free(
	void * cookie)
{
	ddprintf(("cmedia_pci: mixer_free()\n"));

	if (((mixer_dev *)cookie)->open_count != 0) {
		dprintf("cmedia_pci: mixer open_count is bad in mixer_free()!\n");
	}
	return B_OK;	/* already done in close */
}


static int
get_mixer_value(
	cmedia_pci_dev * card,
	cmedia_pci_level * lev)
{
	int ix = map_mixer(lev->selector);
	uchar val;
	if (ix < 0) {
		return B_BAD_VALUE;
	}
	val = get_indirect(card, the_mixers[ix].port);
	lev->flags = 0;
	if (!the_mixers[ix].mutemask) {
		/* no change */
	}
	else if (the_mixers[ix].mutemask == 0x01) {
		if (!(val & 0x01)) {
			lev->flags |= CMEDIA_PCI_LEVEL_MUTED;
		}
	}
	else if (val & the_mixers[ix].mutemask) {
		lev->flags |= CMEDIA_PCI_LEVEL_MUTED;
	}
	val &= the_mixers[ix].mask;
	val >>= the_mixers[ix].leftshift;
	lev->value = ((float)val)*the_mixers[ix].div+the_mixers[ix].sub;

	return B_OK;
}


static int
gather_info(
	mixer_dev * mixer,
	cmedia_pci_level * data,
	int count)
{
	int ix;
	cpu_status cp;

	cp = disable_interrupts();
	acquire_spinlock(&mixer->card->hardware);

	for (ix=0; ix<count; ix++) {
		if (get_mixer_value(mixer->card, &data[ix]) < B_OK)
			break;
	}

	release_spinlock(&mixer->card->hardware);
	restore_interrupts(cp);

	return ix;
}


static status_t
set_mixer_value(
	cmedia_pci_dev * card,
	cmedia_pci_level * lev)
{
	int selector = map_mixer(lev->selector);
	int value;
	int mask;
	if (selector < 0) {
		return EINVAL;
	}
	value = (lev->value-the_mixers[selector].sub)/the_mixers[selector].div;
	if (value < the_mixers[selector].minval) {
		value = the_mixers[selector].minval;
	}
	if (value > the_mixers[selector].maxval) {
		value = the_mixers[selector].maxval;
	}
	value <<= the_mixers[selector].leftshift;
	if (the_mixers[selector].mutemask) {
		if (the_mixers[selector].mutemask == 0x01) {
			if (!(lev->flags & CMEDIA_PCI_LEVEL_MUTED)) {
				value |= the_mixers[selector].mutemask;
			}
		} else {
			if (lev->flags & CMEDIA_PCI_LEVEL_MUTED) {
				value |= the_mixers[selector].mutemask;
			}
		}
	}
	mask = the_mixers[selector].mutemask | the_mixers[selector].mask;
	set_indirect(card, the_mixers[selector].port, value, mask);
	return B_OK;
}


static int
disperse_info(
	mixer_dev * mixer,
	cmedia_pci_level * data,
	int count)
{
	int ix;
	cpu_status cp;

	cp = disable_interrupts();
	acquire_spinlock(&mixer->card->hardware);

	for (ix=0; ix<count; ix++) {
		if (set_mixer_value(mixer->card, &data[ix]) < B_OK)
			break;
	}

	release_spinlock(&mixer->card->hardware);
	restore_interrupts(cp);

	return ix;
}


static status_t
mixer_control(
	void * cookie,
	uint32 iop,
	void * data,
	size_t len)
{
	mixer_dev * it = (mixer_dev *)cookie;
	status_t err = B_OK;

	if (!data) {
		return B_BAD_VALUE;
	}

	ddprintf(("cmedia_pci: mixer_control()\n")); /* slow printing */

	switch (iop) {
	case B_MIXER_GET_VALUES:
		((cmedia_pci_level_cmd *)data)->count = 
			gather_info(it, ((cmedia_pci_level_cmd *)data)->data, 
				((cmedia_pci_level_cmd *)data)->count);
		break;
	case B_MIXER_SET_VALUES:
		((cmedia_pci_level_cmd *)data)->count = 
			disperse_info(it, ((cmedia_pci_level_cmd *)data)->data, 
				((cmedia_pci_level_cmd *)data)->count);
		break;
	default:
		err = B_BAD_VALUE;
		break;
	}
	return err;
}


static status_t
mixer_read(
	void * cookie,
	off_t pos,
	void * data,
	size_t * nread)
{
	return EPERM;
}


static status_t
mixer_write(
	void * cookie,
	off_t pos,
	const void * data,
	size_t * nwritten)
{
	return EPERM;
}

