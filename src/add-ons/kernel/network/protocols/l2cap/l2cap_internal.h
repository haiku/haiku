/* 
 * Copyright 2008 Oliver Ruiz Dorantes. All rights reserved.
 * Copyright 2024, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef L2CAP_INTERNAL_H
#define L2CAP_INTERNAL_H


#include <btCoreData.h>
#include <net_protocol.h>
#include <net_stack.h>


extern bluetooth_core_data_module_info *btCoreData;
extern bt_hci_module_info* btDevices;
extern net_buffer_module_info *gBufferModule;
extern net_stack_module_info *gStackModule;
extern net_socket_module_info *gSocketModule;


#endif	// L2CAP_INTERNAL_H
