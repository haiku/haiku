/* ++++++++++
	ACPI Button Driver, used to get info on power buttons, etc.
+++++ */

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
	get_module(B_ACPI_MODULE_NAME,(module_info **)&acpi);
	return B_OK;
}

void
uninit_driver (void)
{
	put_module(B_ACPI_MODULE_NAME);
}

	
/* ----------
	acpi_button_open - handle open() calls
----- */

static status_t
acpi_button_open (const char *name, uint32 flags, void** cookie)
{
	if (strcmp(name,"power/button/power") == 0) {
		*cookie = (void *)ACPI_BITREG_POWER_BUTTON_STATUS;
		acpi->write_acpi_reg(ACPI_BITREG_POWER_BUTTON_ENABLE,0);
	} else if (strcmp(name,"power/button/sleep") == 0) {
		*cookie = (void *)ACPI_BITREG_SLEEP_BUTTON_STATUS;
		acpi->write_acpi_reg(ACPI_BITREG_SLEEP_BUTTON_ENABLE,0);
	} else {
		return B_ERROR;
	}
	return B_OK;
}


/* ----------
	acpi_button_read - handle read() calls
----- */

static status_t
acpi_button_read (void* cookie, off_t position, void *buf, size_t* num_bytes)
{
	if (*num_bytes < 1)
		return B_IO_ERROR;
		
	*((uint8 *)(buf)) = acpi->read_acpi_reg((uint32)(cookie));
	acpi->write_acpi_reg((uint32)(cookie),0); //--- Clear sticky status reg
	*num_bytes = 1;
	return B_OK;
}


/* ----------
	acpi_button_write - handle write() calls
----- */

static status_t
acpi_button_write (void* cookie, off_t position, const void* buffer, size_t* num_bytes)
{
	if (*num_bytes < 1)
		return B_IO_ERROR;
		
	acpi->write_acpi_reg((uint32)(cookie),*((uint8 *)(buffer)));
	*num_bytes = 1;
	return B_OK;
}


/* ----------
	acpi_button_control - handle ioctl calls
----- */

static status_t
acpi_button_control (void* cookie, uint32 op, void* arg, size_t len)
{	
	return B_ERROR;
}


/* ----------
	acpi_button_close - handle close() calls
----- */

static status_t
acpi_button_close (void* cookie)
{
	if ((uint32)(cookie) == ACPI_BITREG_POWER_BUTTON_STATUS)
		acpi->write_acpi_reg(ACPI_BITREG_POWER_BUTTON_ENABLE,1);
	else if ((uint32)(cookie) == ACPI_BITREG_SLEEP_BUTTON_STATUS)
		acpi->write_acpi_reg(ACPI_BITREG_SLEEP_BUTTON_ENABLE,1);
	return B_OK;
}


/* -----
	acpi_button_free - called after the last device is closed, and after
	all i/o is complete.
----- */
static status_t
acpi_button_free (void* cookie)
{
	return B_OK;
}


/* -----
	null-terminated array of device names supported by this driver
----- */

static const char *acpi_button_name[] = {
	"power/button/power",
	"power/button/sleep",
	NULL
};

/* -----
	function pointers for the device hooks entry points
----- */

device_hooks acpi_button_hooks = {
	acpi_button_open, 			/* -> open entry point */
	acpi_button_close, 			/* -> close entry point */
	acpi_button_free,			/* -> free cookie */
	acpi_button_control, 		/* -> control entry point */
	acpi_button_read,			/* -> read entry point */
	acpi_button_write,			/* -> write entry point */
	NULL, NULL, NULL, NULL
};

/* ----------
	publish_devices - return a null-terminated array of devices
	supported by this driver.
----- */

const char**
publish_devices()
{
	return acpi_button_name;
}

/* ----------
	find_device - return ptr to device hooks structure for a
	given device name
----- */

device_hooks*
find_device(const char* name)
{
	return &acpi_button_hooks;
}

