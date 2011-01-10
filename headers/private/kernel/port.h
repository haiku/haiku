/*
 * Copyright 2005-2009, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 */
#ifndef _KERNEL_PORT_H
#define _KERNEL_PORT_H


#include <thread.h>
#include <iovec.h>

struct kernel_args;
struct select_info;


#define PORT_FLAG_USE_USER_MEMCPY 0x80000000

// port flags
enum {
	// read_port_etc() flags
	B_PEEK_PORT_MESSAGE		= 0x100	// read the message, but don't remove it;
									// kernel-only; memory must be locked
};

// port notifications
#define PORT_MONITOR	'_Pm_'
#define PORT_ADDED		0x01
#define PORT_REMOVED	0x02

#ifdef __cplusplus
extern "C" {
#endif

status_t port_init(struct kernel_args *args);
void delete_owned_ports(Team* team);
int32 port_max_ports(void);
int32 port_used_ports(void);

status_t select_port(int32 object, struct select_info *info, bool kernel);
status_t deselect_port(int32 object, struct select_info *info, bool kernel);

// currently private API
status_t writev_port_etc(port_id id, int32 msgCode, const iovec *msgVecs,
				size_t vecCount, size_t bufferSize, uint32 flags,
				bigtime_t timeout);

// user syscalls
port_id		_user_create_port(int32 queueLength, const char *name);
status_t	_user_close_port(port_id id);
status_t	_user_delete_port(port_id id);
port_id		_user_find_port(const char *portName);
status_t	_user_get_port_info(port_id id, struct port_info *info);
status_t 	_user_get_next_port_info(team_id team, int32 *cookie,
				struct port_info *info);
ssize_t		_user_port_buffer_size_etc(port_id port, uint32 flags,
				bigtime_t timeout);
ssize_t		_user_port_count(port_id port);
ssize_t		_user_read_port_etc(port_id port, int32 *msgCode,
				void *msgBuffer, size_t bufferSize, uint32 flags,
				bigtime_t timeout);
status_t	_user_set_port_owner(port_id port, team_id team);
status_t	_user_write_port_etc(port_id port, int32 msgCode,
				const void *msgBuffer, size_t bufferSize,
				uint32 flags, bigtime_t timeout);
status_t	_user_writev_port_etc(port_id id, int32 msgCode,
				const iovec *msgVecs, size_t vecCount,
				size_t bufferSize, uint32 flags, bigtime_t timeout);
status_t	_user_get_port_message_info_etc(port_id port,
				port_message_info *info, size_t infoSize, uint32 flags,
				bigtime_t timeout);

#ifdef __cplusplus
}
#endif

#endif	/* _KERNEL_PORT_H */
