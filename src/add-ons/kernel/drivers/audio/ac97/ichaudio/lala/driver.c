#include <KernelExport.h>
#include <Drivers.h>
#include <Errors.h>
#include <OS.h>
#include <malloc.h>

int32 api_version = B_CUR_DRIVER_API_VERSION;

#define MAX_DEVICES 8

char *	device_names[MAX_DEVICES];
int		device_count = 0;

status_t
init_hardware(void)
{
	dprintf("init_hardware\n");
	return B_OK;
}

status_t
init_driver(void)
{
	dprintf("init_driver\n");
	return B_OK;
}

void
uninit_driver(void)
{
	dprintf("uninit_driver\n");
}

static status_t
ich_open(const char *name, uint32 flags, void** cookie)
{
	return B_OK;
}

static status_t
ich_close(void* cookie)
{
	return B_OK;
}

static status_t
ich_free(void* cookie)
{
	return B_OK;
}

static status_t
ich_control(void* cookie, uint32 op, void* arg, size_t len)
{
	return B_OK;
}

static status_t
ich_read(void* cookie, off_t position, void *buf, size_t* num_bytes)
{
	*num_bytes = 0;
	return B_IO_ERROR;
}

static status_t
ich_write(void* cookie, off_t position, const void* buffer, size_t* num_bytes)
{
	*num_bytes = 0;
	return B_IO_ERROR;
}

static const char *ich_name[] = {
	"audio/lala/ichaudio/1",
	NULL
};

device_hooks ich_hooks = {
	ich_open,
	ich_close,
	ich_free,
	ich_control,
	ich_read,
	ich_write
};

const char**
publish_devices(void)
{
	dprintf("publish_devices\n");
	return ich_name;
}

device_hooks*
find_device(const char* name)
{
	dprintf("find_device\n");
	return &ich_hooks;
}

