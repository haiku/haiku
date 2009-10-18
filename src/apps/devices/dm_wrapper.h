/*
 * Copyright 2006 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jerome Duval (listdev)
 */
#ifndef DM_WRAPPER_H
#define DM_WRAPPER_H


#include "device_manager_defs.h"

status_t init_dm_wrapper(void);
status_t uninit_dm_wrapper(void);
status_t get_root(device_node_cookie *cookie);
status_t get_child(device_node_cookie *cookie);
status_t get_next_child(device_node_cookie *cookie);
status_t dm_get_next_attr(struct device_attr_info *attr);

#endif /* DM_WRAPPER_H */
