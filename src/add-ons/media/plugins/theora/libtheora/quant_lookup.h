/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggTheora SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE Theora SOURCE CODE IS COPYRIGHT (C) 2002-2003                *
 * by the Xiph.Org Foundation http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

  function:
  last mod: $Id: quant_lookup.h,v 1.1 2004/02/24 13:50:13 shatty Exp $

 ********************************************************************/

#include "encoder_internal.h"

#define MIN16 ((1<<16)-1)
#define SHIFT16 (1<<16)

#define MIN_LEGAL_QUANT_ENTRY 8
#define MIN_DEQUANT_VAL       2
#define IDCT_SCALE_FACTOR     2 /* Shift left bits to improve IDCT precision */
#define OLD_SCHEME            1

static ogg_uint32_t dequant_index[64] = {
  0,  1,  8,  16,  9,  2,  3, 10,
  17, 24, 32, 25, 18, 11,  4,  5,
  12, 19, 26, 33, 40, 48, 41, 34,
  27, 20, 13,  6,  7, 14, 21, 28,
  35, 42, 49, 56, 57, 50, 43, 36,
  29, 22, 15, 23, 30, 37, 44, 51,
  58, 59, 52, 45, 38, 31, 39, 46,
  53, 60, 61, 54, 47, 55, 62, 63
};
