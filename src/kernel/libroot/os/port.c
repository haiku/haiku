/* 
** Copyright 2002, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <OS.h>
#include "syscalls.h"


port_id
create_port(int32 capacity, const char *name)
{
	return sys_port_create(capacity, name);
}


port_id
find_port(const char *name)
{
	return sys_port_find(name);
}


status_t
write_port(port_id port, int32 code, const void *buffer, size_t bufferSize)
{
	return sys_port_write(port, code, buffer, bufferSize);
}


status_t
read_port(port_id port, int32 *code, void *buffer, size_t bufferSize)
{
	return sys_port_read(port, code, buffer, bufferSize);
}


status_t
write_port_etc(port_id port, int32 code, const void *buffer, size_t bufferSize,
	uint32 flags, bigtime_t timeout)
{
	return sys_port_write_etc(port, code, buffer, bufferSize, flags, timeout);
}


status_t
read_port_etc(port_id port, int32 *code, void *buffer, size_t bufferSize,
	uint32 flags, bigtime_t timeout)
{
	return sys_port_read_etc(port, code, buffer, bufferSize, flags, timeout);
}


ssize_t
port_buffer_size(port_id port)
{
	return sys_port_buffer_size(port);
}


ssize_t
port_buffer_size_etc(port_id port, uint32 flags, bigtime_t timeout)
{
	return sys_port_buffer_size_etc(port, flags, timeout);
}


ssize_t
port_count(port_id port)
{
	return sys_port_count(port);
}


status_t
set_port_owner(port_id port, team_id team)
{
	return sys_port_set_owner(port, team);
}


status_t
close_port(port_id port)
{
	return sys_port_close(port);
}


status_t
delete_port(port_id port)
{
	return sys_port_delete(port);
}


status_t
_get_next_port_info(team_id team, int32 *cookie, port_info *info, size_t size)
{
	// size is not yet used, but may, if port_info changes
	(void)size;

	return sys_port_get_next_port_info(team, cookie, info);
}


status_t
_get_port_info(port_id port, port_info *info, size_t size)
{
	return sys_port_get_info(port, info);
}

