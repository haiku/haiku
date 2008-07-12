/*
 * Copyright 2008 Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander Coers		Alexander.Coers@gmx.de
 *		Fredrik Modéen 		fredrik@modeen.se
 */

#include "emuxki.h"
#include "debug.h"

#if !defined(_KERNEL_EXPORT_H)
#include <KernelExport.h>
#endif /* _KERNEL_EXPORT_H */

static status_t joy_open(const char *name, uint32 flags, void **cookie);
static status_t joy_close(void *cookie);
static status_t joy_free(void *cookie);
static status_t joy_control(void *cookie, uint32 op, void *data, size_t len);
static status_t joy_read(void *cookie, off_t pos, void *data, size_t *len);
static status_t joy_write(void *cookie, off_t pos, const void *data, size_t *len);

#define MIN_COMP -7
#define MAX_COMP 8

device_hooks joy_hooks = {
    &joy_open,
    &joy_close,
    &joy_free,
    &joy_control,
    &joy_read,
    &joy_write,
    NULL,		/* select */
    NULL,		/* deselect */
    NULL,		/* readv */
    NULL		/* writev */
};


static status_t
joy_open(const char * name, uint32 flags, void ** cookie)
{
	int ix;
	int offset = -1;

	LOG(("GamePort: joy_open()\n"));

	*cookie = NULL;
	for (ix = 0; ix < num_cards; ix++) {
		if (!strcmp(name, cards[ix].joy.name1)) {
			offset = 0;
			break;
		}
	}
	if (offset < 0) {
		return ENODEV;
	}
	return (*gameport->open_hook)(cards[ix].joy.driver, flags, cookie);
}


static status_t
joy_close(void * cookie)
{
	return (*gameport->close_hook)(cookie);
}


static status_t
joy_free(void * cookie)
{
	return (*gameport->free_hook)(cookie);
}


static status_t
joy_control(void * cookie, uint32 iop, void * data, size_t len)
{
	return (*gameport->control_hook)(cookie, iop, data, len);
}


static status_t
joy_read(void * cookie, off_t pos, void * data, size_t * nread)
{
	return (*gameport->read_hook)(cookie, pos, data, nread);
}


static status_t
joy_write(void * cookie, off_t pos, const void * data, size_t * nwritten)
{
	return (*gameport->write_hook)(cookie, pos, data, nwritten);
}
