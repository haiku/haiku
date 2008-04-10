/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef UNIX_H
#define UNIX_H


#define UNIX_MAX_TRANSFER_UNIT	65536

struct net_address_module_info;
struct net_buffer_module_info;
struct net_socket_module_info;
struct net_stack_module_info;
class UnixAddressManager;

extern net_address_module_info gAddressModule;
extern net_buffer_module_info *gBufferModule;
extern net_socket_module_info *gSocketModule;
extern net_stack_module_info *gStackModule;
extern UnixAddressManager gAddressManager;


#endif	// UNIX_H
