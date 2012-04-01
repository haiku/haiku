/*
 * Copyright 2006-2012, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _DM_WRAPPER_H_
#define _DM_WRAPPER_H_


#include "device_manager_defs.h"


status_t init_dm_wrapper(void);
status_t uninit_dm_wrapper(void);

status_t get_root(device_node_cookie *cookie);
status_t get_child(device_node_cookie *cookie);
status_t get_next_child(device_node_cookie *cookie);
status_t dm_get_next_attr(struct device_attr_info *attr);


#endif
