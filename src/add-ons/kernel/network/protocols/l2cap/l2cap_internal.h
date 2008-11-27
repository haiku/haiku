/* 
 * Copyright 2008 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef L2CAP_INTERNAL_H
#define L2CAP_INTERNAL_H

#include <btCoreData.h>
#include <net_protocol.h>
#include <net_stack.h>

extern l2cap_flow_t default_qos;

extern bluetooth_core_data_module_info *btCoreData;
extern net_buffer_module_info *gBufferModule;
extern net_stack_module_info *gStackModule;
extern net_socket_module_info *gSocketModule;

#endif	// L2CAP_ADDRESS_H
