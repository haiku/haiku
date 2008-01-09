#include "driver.h"

static status_t
hda_open (const char *name, uint32 flags, void** cookie)
{
	hda_controller* hc = NULL;
	status_t rc = B_OK;
	long i;

	for(i=0; i < num_cards; i++) {
		if (strcmp(cards[i].devfs_path, name) == 0) {
			hc = &cards[i];
		}
	}
	
	if (hc == NULL)
		return ENODEV;
		
	if (hc->opened)
		return B_BUSY;

	rc = hda_hw_init(hc);
	if (rc != B_OK)
		return rc;

	hc->opened++;

	*cookie = hc;
	return B_OK;
}

static status_t
hda_read (void* cookie, off_t position, void *buf, size_t* num_bytes)
{
	*num_bytes = 0;				/* tell caller nothing was read */
	return B_IO_ERROR;
}

static status_t
hda_write (void* cookie, off_t position, const void* buffer, size_t* num_bytes)
{
	*num_bytes = 0;				/* tell caller nothing was written */
	return B_IO_ERROR;
}

static status_t
hda_control (void* cookie, uint32 op, void* arg, size_t len)
{
	hda_controller* hc = (hda_controller*)cookie;
	if (hc->active_codec)
		return multi_audio_control(hc->active_codec, op, arg, len);

	return B_BAD_VALUE;
}

static status_t
hda_close (void* cookie)
{
	hda_controller* hc = (hda_controller*)cookie;
	hda_hw_stop(hc);
	--hc->opened;

	return B_OK;
}

static status_t
hda_free (void* cookie)
{
	hda_controller* hc = (hda_controller*)cookie;
	hda_hw_uninit(hc);

	return B_OK;
}

device_hooks driver_hooks = {
	hda_open, 		/* -> open entry point */
	hda_close, 		/* -> close entry point */
	hda_free,		/* -> free cookie */
	hda_control, 		/* -> control entry point */
	hda_read,		/* -> read entry point */
	hda_write		/* -> write entry point */
};
