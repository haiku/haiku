/*
 * Copyright 2005, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 */
#ifndef _KERNEL_PORT_H
#define _KERNEL_PORT_H

#include <thread.h>

struct kernel_args;


#define PORT_FLAG_USE_USER_MEMCPY 0x80000000

#ifdef __cplusplus
extern "C" {
#endif

status_t port_init(struct kernel_args *args);
int delete_owned_ports(team_id owner);
int32 port_max_ports(void);
int32 port_used_ports(void);

// temp: test
void port_test(void);

// user syscalls
port_id		_user_create_port(int32 queue_length, const char *name);
status_t	_user_close_port(port_id id);
status_t	_user_delete_port(port_id id);
port_id		_user_find_port(const char *port_name);
status_t	_user_get_port_info(port_id id, struct port_info *info);
status_t 	_user_get_next_port_info(team_id team, int32 *cookie,
				struct port_info *info);
ssize_t		_user_port_buffer_size_etc(port_id port, uint32 flags,
				bigtime_t timeout);
ssize_t		_user_port_count(port_id port);
ssize_t		_user_read_port_etc(port_id port, int32 *msg_code,
				void *msg_buffer, size_t buffer_size, uint32 flags,
				bigtime_t timeout);
status_t	_user_set_port_owner(port_id port, team_id team);
status_t	_user_write_port_etc(port_id port, int32 msg_code,
				const void *msg_buffer, size_t buffer_size,
				uint32 flags, bigtime_t timeout);

#ifdef __cplusplus
}
#endif

#endif	/* _KERNEL_PORT_H */
