/*
 * Copyright 2005-2010 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * PS/2 bus manager
 *
 * Authors (in chronological order):
 *		Marcus Overhagen (marcus@overhagen.de)
 */
#ifndef __PS2_SERVICE_H
#define __PS2_SERVICE_H


#include "ps2_common.h"
#include "ps2_dev.h"


#ifdef __cplusplus
extern "C" {
#endif

status_t	ps2_service_init(void);
void		ps2_service_exit(void);

void		ps2_service_notify_device_added(ps2_dev *dev);
void		ps2_service_notify_device_republish(ps2_dev *dev);
void		ps2_service_notify_device_removed(ps2_dev *dev);

#ifdef __cplusplus
}
#endif


#endif	/* __PS2_SERVICE_H */
