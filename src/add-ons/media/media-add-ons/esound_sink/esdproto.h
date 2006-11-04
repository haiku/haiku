/*
 * ESounD media addon for BeOS
 *
 * Copyright (c) 2006 François Revol (revol@free.fr)
 * 
 * Based on Multi Audio addon for Haiku,
 * Copyright (c) 2002, 2003 Jerome Duval (jerome.duval@free.fr)
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
#ifndef _ESDPROTO_H
#define _ESDPROTO_H
/* 
 * ESounD protocol
 * cf. http://www.jcraft.com/jesd/EsounD-protocol.txt
 */

#include <inttypes.h>

/* limits */
#define ESD_MAX_BUF (4 * 1024)
#define ESD_MAX_KEY 16
#define ESD_MAX_NAME 128

typedef uint32_t	esd_rate_t;
typedef uint32_t	esd_format_t;
typedef uint32_t	esd_command_t;
typedef char	esd_key_t[ESD_MAX_KEY];
typedef char	esd_name_t[ESD_MAX_NAME];

/* defaults */
//#define ESD_DEFAULT_PORT 5001
#define ESD_DEFAULT_PORT 16001

#define ESD_DEFAULT_RATE 44100 
//#define ESD_DEFAULT_RATE 22050 

#define ESD_ENDIAN_TAG 'ENDN'

enum {
	/* init */
	ESD_PROTO_CONNECT = 0,
	
	/* security  */
	ESD_PROTO_LOCK,
	ESD_PROTO_UNLOCK,
	
	ESD_PROTO_STREAM_PLAY,
	ESD_PROTO_STREAM_REC,
	ESD_PROTO_STREAM_MON,
	
	ESD_PROTO_SAMPLE_CACHE,
	ESD_PROTO_SAMPLE_FREE,
	ESD_PROTO_SAMPLE_PLAY,
	ESD_PROTO_SAMPLE_LOOP,
	ESD_PROTO_SAMPLE_STOP,
	ESD_PROTO_SAMPLE_KILL,
	
	ESD_PROTO_STANDBY,
	ESD_PROTO_RESUME,
	
	ESD_PROTO_SAMPLE_GETID,
	ESD_PROTO_STREAM_FILTER,
	
	ESD_PROTO_SERVER_INFO,
	ESD_PROTO_SERVER_ALL_INFO,
	ESD_PROTO_SUBSCRIBE,
	ESD_PROTO_UNSUBSCRIBE,
	
	ESD_PROTO_STREAM_PAN,
	ESD_PROTO_SAMPLE_PAN,
	
	ESD_PROTO_STANDBY_MODE,
	ESD_PROTO_LATENCY,
	ESD_PROTO_MAX
};

#define ESD_MASK_BITS	0x000F
#define ESD_MASK_CHAN	0x00F0
#define ESD_MASK_MODE	0x0F00
#define ESD_MASK_FUNC	0xF000

/* sample size */
#define ESD_BITS8	0x0000
#define ESD_BITS16	0x0001

/* channel count */
#define ESD_MONO	0x0010
#define ESD_STEREO	0x0020

/* mode */
#define ESD_STREAM	0x0000
#define ESD_SAMPLE	0x0100
#define ESD_ADPCM	0x0200

/* functions */
#define ESD_FUNC_PLAY		0x1000
#define ESD_FUNC_MONITOR	0x0000
#define ESD_FUNC_RECORD		0x2000
#define ESD_FUNC_STOP	0x0000
#define ESD_FUNC_LOOP		0x2000

/* errors */
#define ESD_OK	0
#define ESD_ERROR_STANDBY	1
#define ESD_ERROR_AUTOSTANDBY	2
#define ESD_ERROR_RUNNING	3

#define ESD_FALSE	0
#define ESD_TRUE	1

#endif /* _ESDPROTO_H */
