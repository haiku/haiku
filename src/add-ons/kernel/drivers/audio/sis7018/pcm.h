/*
 * This file is a part of SiS 7018 BeOS driver project.
 * Copyright (c) 2002-2003 by Siarzuk Zharski <imker@gmx.li>
 *
 * This file may be used under the terms of the BSD License
 * See the file "License" for details.
 */
#ifndef _PCM_H_
 #define _PCM_H_

#ifndef _DRIVERS_H
 #include <Drivers.h>
#endif //_DRIVERS_H

#ifndef _SIS7018_H_
 #include "sis7018.h"
#endif //_SIS7018_H_

extern device_hooks pcm_hooks;

bool playback_interrupt(sis7018_ch *ch, bool bHalf);

/* secret handshake ioctl()s */
/*#define SV_SECRET_HANDSHAKE 10100
typedef struct {
  bigtime_t  wr_time;
  bigtime_t  rd_time;
  uint32    wr_skipped;
  uint32    rd_skipped;
  uint64    wr_total;
  uint64    rd_total;
  uint32    _reserved_[6];
} sv_handshake;
#define SV_RD_TIME_WAIT 10101
#define SV_WR_TIME_WAIT 10102
typedef struct {
  bigtime_t  time;
  bigtime_t  bytes;
  uint32    skipped;
  uint32    _reserved_[3];
} sv_timing;
*/
#endif //_PCM_H_
