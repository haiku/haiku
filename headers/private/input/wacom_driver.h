/*
 * Copyright 2020, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 */
#ifndef WACOM_DRIVER_H
#define WACOM_DRIVER_H


#include <SupportDefs.h>


typedef struct {
	uint16	vendor_id;
	uint16	product_id;
	size_t	max_packet_size;
} _PACKED wacom_device_header;


#endif // WACOM_DRIVER_H
