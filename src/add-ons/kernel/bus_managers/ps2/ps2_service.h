/*
 * Copyright 2005 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * PS/2 hid device driver
 *
 * Authors (in chronological order):
 *		Marcus Overhagen (marcus@overhagen.de)
 */
#ifndef __PS2_SERVICE_H
#define __PS2_SERVICE_H

#include "ps2_common.h"
#include "ps2_dev.h"

status_t	ps2_service_init(void);
void		ps2_service_exit(void);

void		ps2_service_handle_device_added(ps2_dev *dev);
void		ps2_service_handle_device_removed(ps2_dev *dev);

#endif
