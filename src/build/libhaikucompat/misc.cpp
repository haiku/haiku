
#include <stdarg.h>
#include <stdio.h>

#include <fs_volume.h>

extern "C" void
debug_printf(const char *format, ...)
{
	va_list list;

	va_start(list, format);
	vprintf(format, list);
	va_end(list);
}


dev_t
fs_mount_volume(const char *where, const char *device, const char *filesystem,
	uint32 flags, const char *parameters)
{
	return B_ERROR;
}


status_t
fs_unmount_volume(const char *path, uint32 flags)
{
	return B_ERROR;
}
