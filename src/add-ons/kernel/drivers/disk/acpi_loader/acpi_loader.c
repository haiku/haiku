/*
 * R5 ACPI Loader. This needs to happen as soon as possible in the boot process
 * so we have to break convention and place it here.
 */

#include <KernelExport.h>
#include <Drivers.h>
#include <Errors.h>

#include <ACPI.h>

acpi_module_info *acpi;

status_t
init_hardware (void)
{
	return B_OK;
}

status_t
init_driver (void)
{
	return get_module(B_ACPI_MODULE_NAME, (module_info**)&acpi);
}

void
uninit_driver (void)
{
	put_module(B_ACPI_MODULE_NAME);
}

static const char *acpi_loader_name[] = {
	NULL
};

device_hooks acpi_loader_hooks = {
	NULL, /* open */
	NULL, /* close */
	NULL, /* free */
	NULL, /* control */
	NULL, /* read */
	NULL, /* write */
	NULL, NULL, NULL, NULL
};

const char**
publish_devices()
{
	return acpi_loader_name;
}

device_hooks*
find_device(const char* name)
{
	return &acpi_loader_hooks;
}
