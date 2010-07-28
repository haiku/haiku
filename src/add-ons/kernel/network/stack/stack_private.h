/*
 * Copyright 2006-2010, Haiku, Inc. All Rights Reserved.
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
#include <net_stack_interface.h>


// Stack-wide configuration
#define ENABLE_DEBUGGER_COMMANDS	1
#define STACK_DEBUG_PREFIX			"\33[31mnet:\33[0m "


class Interface;


extern net_stack_module_info gNetStackModule;
extern net_buffer_module_info gNetBufferModule;
extern net_socket_module_info gNetSocketModule;
extern net_datalink_module_info gNetDatalinkModule;
extern net_datalink_protocol_module_info gDatalinkInterfaceProtocolModule;
extern net_stack_interface_module_info gNetStackInterfaceModule;

// stack.cpp
status_t register_domain_datalink_protocols(int family, int type, ...);
status_t register_domain_protocols(int family, int type, int protocol, ...);
status_t get_domain_protocols(net_socket* socket);
status_t put_domain_protocols(net_socket* socket);
status_t get_domain_datalink_protocols(Interface* interface,
	net_domain* domain);
status_t put_domain_datalink_protocols(Interface* interface,
	net_domain* domain);

// notifications.cpp
status_t notify_interface_added(net_interface* interface);
status_t notify_interface_removed(net_interface* interface);
status_t notify_interface_changed(net_interface* interface, uint32 oldFlags = 0,
	uint32 newFlags = 0);
status_t notify_link_changed(net_device* device);
status_t init_notifications();
void uninit_notifications();

status_t init_stack();
status_t uninit_stack();


#endif	// STACK_PRIVATE_H
