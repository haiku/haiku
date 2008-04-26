
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


extern "C" void
ktrace_printf(const char *format, ...)
{
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


int32
atomic_get(vint32 *value)
{
	return *value;
}


status_t
_get_port_message_info_etc(port_id id, port_message_info *info,
	size_t infoSize, uint32 flags, bigtime_t timeout)
{
	return B_ERROR;
}

