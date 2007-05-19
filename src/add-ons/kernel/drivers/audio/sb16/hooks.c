#include "driver.h"

extern sb16_dev_t device;

static status_t
sb16_open (const char *name, uint32 flags, void** cookie)
{
	sb16_dev_t* dev = &device;
	status_t rc = B_OK;

	if (dev->opened)
		return B_BUSY;

	rc = sb16_hw_init(dev);
	if (rc != B_OK)
		return rc;

	dev->opened++;
		
	*cookie = dev;
	return B_OK;
}

static status_t
sb16_read (void* cookie, off_t position, void *buf, size_t* num_bytes)
{
	*num_bytes = 0;				/* tell caller nothing was read */
	return B_IO_ERROR;
}

static status_t
sb16_write (void* cookie, off_t position, const void* buffer, size_t* num_bytes)
{
	*num_bytes = 0;				/* tell caller nothing was written */
	return B_IO_ERROR;
}

static status_t
sb16_control (void* cookie, uint32 op, void* arg, size_t len)
{
	if (cookie)
		return multi_audio_control(cookie, op, arg, len);
		
	return B_BAD_VALUE;
}

static status_t
sb16_close (void* cookie)
{
	sb16_dev_t* dev = (sb16_dev_t*)cookie;
	sb16_hw_stop(dev);

	return B_OK;
}

static status_t
sb16_free (void* cookie)
{
	sb16_dev_t* dev = (sb16_dev_t*)cookie;
	sb16_hw_uninit(dev);
	--dev->opened;

	return B_OK;
}

device_hooks driver_hooks = {
	sb16_open, 			/* -> open entry point */
	sb16_close, 			/* -> close entry point */
	sb16_free,			/* -> free cookie */
	sb16_control, 		/* -> control entry point */
	sb16_read,			/* -> read entry point */
	sb16_write			/* -> write entry point */
};
