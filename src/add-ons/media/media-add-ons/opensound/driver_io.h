/*
 * multiaudio replacement media addon for BeOS
 *
 * Copyright (c) 2002, Jerome Duval (jerome.duval@free.fr)
 *
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice, 
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation 
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR 
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS 
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, 
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#ifndef _DRIVER_IO_H
#define _DRIVER_IO_H

#include <unistd.h>
#include <fcntl.h>
#include <Drivers.h>
#include "soundcard.h"

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
