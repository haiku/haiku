/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef STACK_PRIVATE_H
#define STACK_PRIVATE_H


#include <net_buffer.h>
#include <net_datalink.h>
#include <net_datalink_protocol.h>
#include <net_protocol.h>
#include <net_socket.h>
#include <net_stack.h>


#define NET_STARTER_MODULE_NAME "network/stack/starter/v1"

extern net_stack_module_info gNetStackModule;
extern net_buffer_module_info gNetBufferModule;
extern net_socket_module_info gNetSocketModule;
extern net_datalink_module_info gNetDatalinkModule;
extern net_datalink_protocol_module_info gDatalinkInterfaceProtocolModule;

// stack.cpp
status_t register_domain_datalink_protocols(int family, int type, ...);
status_t register_domain_protocols(int family, int type, int protocol, ...);
status_t get_domain_protocols(net_socket *socket);
status_t put_domain_protocols(net_socket *socket);
status_t get_domain_datalink_protocols(net_interface *interface);
status_t put_domain_datalink_protocols(net_interface *interface);

#endif	// STACK_PRIVATE_H
