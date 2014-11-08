#include "display_adapter.h"


typedef struct acpi_ns_device_info {
	device_node *node;
	acpi_handle acpi_device;
} display_device_info;
	

extern "C" {


/*

TODO: ACPI Spec 5 Appendix B: Implement:
_BCL Brightness control levels
_BCM Brightness control method
_BQC Brightness Query Current Level
_DCS Get current hardware status
_DDC Return the EDID for this device
_DGS Query desired hardware active \ inactive state
_DSS Set hardware active \ inactive state

Brightness notifications

*/

	
static status_t
display_open(void *_cookie, const char* path, int flags, void** cookie)
{
	display_device_info *device = (display_device_info *)_cookie;
	*cookie = device;
	return B_OK;
}


static status_t
display_read(void *_cookie, off_t position, void *buf, size_t* num_bytes)
{
	return B_ERROR;
}


static status_t
display_write(void* cookie, off_t position, const void* buffer,
	size_t* num_bytes)
{
	*num_bytes = 0;
	return B_ERROR;
}


static status_t
display_control(void* cookie, uint32 op, void* arg, size_t len)
{
	return B_ERROR;
}


static status_t
display_close(void* cookie)
{
	return B_OK;
}


static status_t
display_free(void* cookie)
{
	display_device_info *device = (display_device_info *)cookie;
	return B_OK;
}


//	#pragma mark - device module API


static status_t
display_init(void *_cookie, void **cookie)
{
	device_node *node = (device_node *)_cookie;

	display_device_info *device = 
		(display_device_info *)calloc(1, sizeof(*device));

	if (device == NULL)
		return B_NO_MEMORY;

	device->node = node;

	const char *path;
	if (gDeviceManager->get_attr_string(node, ACPI_DEVICE_PATH_ITEM, &path,
			false) != B_OK
		|| gAcpi->get_handle(NULL, path, &device->acpi_device) != B_OK) {
		dprintf("%s: failed to get acpi node.\n", __func__);
		free(device);
		return B_ERROR;
	}

	*cookie = device;
	return B_OK;
}


static void
display_uninit(void *_cookie)
{
	display_device_info *device = (display_device_info *)_cookie;
	free(device);
}

} //extern c

device_module_info display_device_module = {
	{
		DISPLAY_DEVICE_MODULE_NAME,
		0,
		NULL
	},

	display_init,
	display_uninit,
	NULL,

	display_open,
	display_close,
	display_free,
	display_read,
	display_write,
	NULL,
	display_control,

	NULL,
	NULL
};




