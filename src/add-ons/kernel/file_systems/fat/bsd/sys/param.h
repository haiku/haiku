/*-
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 1982, 1986, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#ifndef FAT_PARAM_H
#define FAT_PARAM_H


// Modified to support the Haiku FAT driver.

#ifndef FS_SHELL
#	include <errno.h>
#	include <sys/time.h>
#	include_next <sys/param.h>
#endif

#include "sys/types.h"


#define DEV_BSHIFT 9 /* log2(DEV_BSIZE) */
#define DEV_BSIZE (1 << DEV_BSHIFT)

#ifndef MAXPHYS				/* max raw I/O transfer size */
#ifdef __ILP32__
#define MAXPHYS		(128 * 1024)
#else
#define MAXPHYS		(1024 * 1024)
#endif
#endif

/*
 * Machine-independent constants (some used in following include files).
 */
#define SPECNAMELEN 255 /* max length of devicename */

// BSD error codes used by the driver
#define EINTEGRITY 97 /* Integrity check failed */
/* pseudo-errors returned inside kernel to modify return to process */
#define EJUSTRETURN (-2) /* don't modify regs, just return */

/* Macros for counting and rounding. */
#ifndef rounddown
#define rounddown(x, y) (((x) / (y)) * (y))
#endif
#ifndef roundup
#define roundup(x, y) ((((x) + ((y) -1)) / (y)) * (y)) /* to any y */
#endif
#ifndef powerof2
#define powerof2(x) ((((x) -1) & (x)) == 0)
#endif


#endif // FAT_PARAM_H
