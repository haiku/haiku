/* ports.h
 *
 * Definitions here are for kernel use only. For the actual
 * definitions of port functions, please look at OS.h
 */

#ifndef _KERNEL_PORT_H
#define _KERNEL_PORT_H

#include <thread.h>

#define PORT_FLAG_USE_USER_MEMCPY 0x80000000

int port_init(kernel_args *ka);
int delete_owned_ports(proc_id owner);

// temp: test
void port_test(void);
int	 port_test_thread_func(void* arg);

// user-level API
port_id		user_create_port(int32 queue_length, const char *name);
int			user_close_port(port_id id);
int			user_delete_port(port_id id);
port_id		user_find_port(const char *port_name);
int			user_get_port_info(port_id id, struct port_info *info);
int		 	user_get_next_port_info(proc_id proc,
				uint32 *cookie,
				struct port_info *info);
ssize_t		user_port_buffer_size_etc(port_id port,
				uint32 flags,
				bigtime_t timeout);
int32		user_port_count(port_id port);
ssize_t		user_read_port_etc(port_id port,
				int32 *msg_code,
				void *msg_buffer,
				size_t buffer_size,
				uint32 flags,
				bigtime_t timeout);
int			user_set_port_owner(port_id port, proc_id proc);
int			user_write_port_etc(port_id port,
				int32 msg_code,
				void *msg_buffer,
				size_t buffer_size,
				uint32 flags,
				bigtime_t timeout);

#endif
