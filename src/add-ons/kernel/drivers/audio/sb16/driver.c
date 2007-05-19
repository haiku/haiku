#include "driver.h"

sb16_dev_t device;

const char* devs[2] = {
	NULL, NULL
};

const char** publish_devices(void); /* Just to silence compiler */

static status_t
extract_driver_settings(void* settings, sb16_dev_t* sb16)
{
	const char* port;
	const char* irq;
	const char* dma8;
	const char* dma16;
	const char* midiport;
	status_t rc;

	if ((port=get_driver_parameter(settings, "port", "220", NULL)) != NULL &&
		(irq=get_driver_parameter(settings, "irq", "5", NULL)) != NULL &&
		(dma8=get_driver_parameter(settings, "dma8", "1", NULL)) != NULL &&
		(dma16=get_driver_parameter(settings, "dma16", "5", NULL)) != NULL &&
		(midiport=get_driver_parameter(settings, "midiport", "330", NULL)) != NULL) {
		sb16->port	= strtol(port, NULL, 16);
		sb16->irq	= strtol(irq, NULL, 16);
		sb16->dma8	= strtol(dma8, NULL, 16);
		sb16->dma16	= strtol(dma16, NULL, 16);
		sb16->midiport	= strtol(midiport, NULL, 16);

		rc = B_OK;
	} else {
		rc = B_BAD_VALUE;
	}

	return rc;
}

//#pragma mark --

int32 api_version = B_CUR_DRIVER_API_VERSION;

status_t
init_hardware(void)
{
	return B_OK;
}

status_t
init_driver (void)
{
	void* settings = load_driver_settings("sb16");
	status_t rc;

	if (settings != NULL) {
		rc = extract_driver_settings(settings, &device);
		if (rc == B_OK) {
			devs[0] = DEVFS_PATH "/sb16/0";
			dprintf("%s: publishing %s\n", __func__, devs[0]);
		}
	} else
		rc = ENODEV;

	return rc;
}

void
uninit_driver (void)
{
}

const char**
publish_devices(void)
{
	return devs;
}

device_hooks*
find_device(const char* name)
{
	return &driver_hooks;
}
