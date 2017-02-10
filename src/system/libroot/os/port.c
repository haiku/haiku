/* 
** Copyright 2002, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/


#include <OS.h>
#include "syscalls.h"


port_id
create_port(int32 capacity, const char *name)
{
	return _kern_create_port(capacity, name);
}


port_id
find_port(const char *name)
{
	return _kern_find_port(name);
}


status_t
write_port(port_id port, int32 code, const void *buffer, size_t bufferSize)
{
	return _kern_write_port_etc(port, code, buffer, bufferSize, 0, 0);
}


ssize_t
read_port(port_id port, int32 *code, void *buffer, size_t bufferSize)
{
	return _kern_read_port_etc(port, code, buffer, bufferSize, 0, 0);
}


status_t
write_port_etc(port_id port, int32 code, const void *buffer, size_t bufferSize,
	uint32 flags, bigtime_t timeout)
{
	return _kern_write_port_etc(port, code, buffer, bufferSize, flags, timeout);
}


ssize_t
read_port_etc(port_id port, int32 *code, void *buffer, size_t bufferSize,
	uint32 flags, bigtime_t timeout)
{
	return _kern_read_port_etc(port, code, buffer, bufferSize, flags, timeout);
}


ssize_t
port_buffer_size(port_id port)
{
	return _kern_port_buffer_size_etc(port, 0, 0);
}


ssize_t
port_buffer_size_etc(port_id port, uint32 flags, bigtime_t timeout)
{
	return _kern_port_buffer_size_etc(port, flags, timeout);
}


ssize_t
port_count(port_id port)
{
	return _kern_port_count(port);
}


status_t
set_port_owner(port_id port, team_id team)
{
	return _kern_set_port_owner(port, team);
}


status_t
close_port(port_id port)
{
	return _kern_close_port(port);
}


status_t
delete_port(port_id port)
{
	return _kern_delete_port(port);
}


status_t
_get_next_port_info(team_id team, int32 *cookie, port_info *info, size_t size)
{
	// size is not yet used, but may, if port_info changes
	(void)size;

	return _kern_get_next_port_info(team, cookie, info);
}


status_t
_get_port_info(port_id port, port_info *info, size_t size)
{
	return _kern_get_port_info(port, info);
}


status_t
_get_port_message_info_etc(port_id port, port_message_info *info,
	size_t infoSize, uint32 flags, bigtime_t timeout)
{
	return _kern_get_port_message_info_etc(port, info, infoSize, flags,
		timeout);
}
