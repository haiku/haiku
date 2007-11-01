/*
 * multiaudio replacement media addon for BeOS
 *
 * Copyright (c) 2002, Jerome Duval (jerome.duval@free.fr)
 * Distributed under the terms of the MIT License.
 */
#ifndef _DRIVER_IO_H
#define _DRIVER_IO_H

#include <unistd.h>
#include <fcntl.h>
#include <Drivers.h>
#include "hmulti_audio.h"

#define DRIVER_GET_DESCRIPTION(x...)				ioctl( fd, B_MULTI_GET_DESCRIPTION, x )
#define DRIVER_GET_ENABLED_CHANNELS(x...)			ioctl( fd, B_MULTI_GET_ENABLED_CHANNELS, x )
#define DRIVER_SET_ENABLED_CHANNELS(x...)			ioctl( fd, B_MULTI_SET_ENABLED_CHANNELS, x )
#define DRIVER_SET_GLOBAL_FORMAT(x...)				ioctl( fd, B_MULTI_SET_GLOBAL_FORMAT, x )
#define DRIVER_GET_BUFFERS(x...)					ioctl( fd, B_MULTI_GET_BUFFERS, x )
#define DRIVER_BUFFER_EXCHANGE(x...)				ioctl( fd, B_MULTI_BUFFER_EXCHANGE, x )

#define DRIVER_LIST_MIX_CONTROLS(x...)				ioctl( fd, B_MULTI_LIST_MIX_CONTROLS, x )
#define DRIVER_SET_MIX(x...)						ioctl( fd, B_MULTI_SET_MIX, x )
#define DRIVER_GET_MIX(x...)						ioctl( fd, B_MULTI_GET_MIX, x )

#endif
