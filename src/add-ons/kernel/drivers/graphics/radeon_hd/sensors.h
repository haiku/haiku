/*
 * Copyright 2011-2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck IV, kallisti5@unixzen.com
 */
#ifndef SENSORS_H
#define SENSORS_H


#include "driver.h"
#include "device.h"
#include "radeon_hd.h"


int32 radeon_thermal_query(radeon_info &info);


#endif /* SENSORS_H */
