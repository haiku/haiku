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
  last mod: $Id: block_inline.h,v 1.1 2004/02/24 13:50:13 shatty Exp $

 ********************************************************************/

#include "encoder_internal.h"

static ogg_int32_t MBOrderMap[4] = { 0, 2, 3, 1 };
static ogg_int32_t BlockOrderMap1[4][4] = {
  { 0, 1, 3, 2 },
  { 0, 2, 3, 1 },
  { 0, 2, 3, 1 },
  { 3, 2, 0, 1 }
};

static ogg_int32_t QuadMapToIndex1( ogg_int32_t (*BlockMap)[4][4],
                                    ogg_uint32_t SB, ogg_uint32_t MB,
                                    ogg_uint32_t B ){
  return BlockMap[SB][MBOrderMap[MB]][BlockOrderMap1[MB][B]];
}


static ogg_int32_t QuadMapToMBTopLeft( ogg_int32_t (*BlockMap)[4][4],
                                       ogg_uint32_t SB, ogg_uint32_t MB ){
  return BlockMap[SB][MBOrderMap[MB]][0];
}
