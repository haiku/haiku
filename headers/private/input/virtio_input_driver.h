/*
 * Copyright 2021-26, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _VIRTIO_INPUT_DRIVER_H_
#define _VIRTIO_INPUT_DRIVER_H_


#include <Drivers.h>


enum {
	virtioInputRead = B_DEVICE_OP_CODES_END + 1,
	virtioInputCancelIO = B_DEVICE_OP_CODES_END + 2,
	virtioInputGetType = B_DEVICE_OP_CODES_END + 3,
};


enum VirtioInputType {
	kVirtioInputUnknown = 0,
	kVirtioInputTablet,
	kVirtioInputKeyboard,
};


#endif	// _VIRTIO_INPUT_DRIVER_H_
