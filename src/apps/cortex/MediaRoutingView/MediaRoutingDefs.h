/*
 * Copyright (c) 1999-2000, Eric Moon.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions, and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF TITLE, NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


// MediaRoutingDefs.h
// c.lenz 9oct99
//
// Constants used in MediaRoutingView and friends:
// - Colors
// - Cursors
//
// HISTORY
// 6oct99	c.lenz		Begun
// 

#ifndef __MediaRoutingDefs_H__
#define __MediaRoutingDefs_H__

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

// -------------------------------------------------------- //
// *** cursors
// -------------------------------------------------------- //

const unsigned char M_CABLE_CURSOR [] = {
// PREAMBLE
  16,   1,   0,   0,
// BITMAP
 224,   0, 144,   0, 168,   0,  68,   0,  34,   0,  17, 128,  11,  64,   7, 160,
   7, 208,   3, 232,   1, 244,   0, 250,   0, 125,   0,  63,   0,  30,   0,  12,
// MASK
 224,   0, 240,   0, 248,   0, 124,   0,  62,   0,  31, 128,  15, 192,   7, 224,
   7, 240,   3, 248,   1, 252,   0, 254,   0, 127,   0,  63,   0,  30,   0,  12
};

__END_CORTEX_NAMESPACE
#endif /* __MediaRoutingDefs_H__ */
