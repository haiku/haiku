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
  last mod: $Id: reconstruct.c,v 1.1 2004/02/24 13:50:13 shatty Exp $

 ********************************************************************/

#include "encoder_internal.h"

void ReconIntra( PB_INSTANCE *pbi, unsigned char * ReconPtr,
                 ogg_int16_t * ChangePtr, ogg_uint32_t LineStep ) {
  ogg_uint32_t i;

  for ( i = 0; i < BLOCK_HEIGHT_WIDTH; i++ ){
    /* Convert the data back to 8 bit unsigned */
    /* Saturate the output to unsigend 8 bit values */
    ReconPtr[0] = clamp255( ChangePtr[0] + 128 );
    ReconPtr[1] = clamp255( ChangePtr[1] + 128 );
    ReconPtr[2] = clamp255( ChangePtr[2] + 128 );
    ReconPtr[3] = clamp255( ChangePtr[3] + 128 );
    ReconPtr[4] = clamp255( ChangePtr[4] + 128 );
    ReconPtr[5] = clamp255( ChangePtr[5] + 128 );
    ReconPtr[6] = clamp255( ChangePtr[6] + 128 );
    ReconPtr[7] = clamp255( ChangePtr[7] + 128 );

    ReconPtr += LineStep;
    ChangePtr += BLOCK_HEIGHT_WIDTH;
  }

}

void ReconInter( PB_INSTANCE *pbi, unsigned char * ReconPtr,
                 unsigned char * RefPtr, ogg_int16_t * ChangePtr,
                 ogg_uint32_t LineStep ) {
  ogg_uint32_t i;

  for ( i = 0; i < BLOCK_HEIGHT_WIDTH; i++) {
    ReconPtr[0] = clamp255(RefPtr[0] + ChangePtr[0]);
    ReconPtr[1] = clamp255(RefPtr[1] + ChangePtr[1]);
    ReconPtr[2] = clamp255(RefPtr[2] + ChangePtr[2]);
    ReconPtr[3] = clamp255(RefPtr[3] + ChangePtr[3]);
    ReconPtr[4] = clamp255(RefPtr[4] + ChangePtr[4]);
    ReconPtr[5] = clamp255(RefPtr[5] + ChangePtr[5]);
    ReconPtr[6] = clamp255(RefPtr[6] + ChangePtr[6]);
    ReconPtr[7] = clamp255(RefPtr[7] + ChangePtr[7]);

    ChangePtr += BLOCK_HEIGHT_WIDTH;
    ReconPtr += LineStep;
    RefPtr += LineStep;
  }

}

void ReconInterHalfPixel2( PB_INSTANCE *pbi, unsigned char * ReconPtr,
                           unsigned char * RefPtr1, unsigned char * RefPtr2,
                           ogg_int16_t * ChangePtr, ogg_uint32_t LineStep ) {
  ogg_uint32_t  i;

  for ( i = 0; i < BLOCK_HEIGHT_WIDTH; i++ ){
    ReconPtr[0] = clamp255((((int)RefPtr1[0] + (int)RefPtr2[0]) >> 1) + ChangePtr[0] );
    ReconPtr[1] = clamp255((((int)RefPtr1[1] + (int)RefPtr2[1]) >> 1) + ChangePtr[1] );
    ReconPtr[2] = clamp255((((int)RefPtr1[2] + (int)RefPtr2[2]) >> 1) + ChangePtr[2] );
    ReconPtr[3] = clamp255((((int)RefPtr1[3] + (int)RefPtr2[3]) >> 1) + ChangePtr[3] );
    ReconPtr[4] = clamp255((((int)RefPtr1[4] + (int)RefPtr2[4]) >> 1) + ChangePtr[4] );
    ReconPtr[5] = clamp255((((int)RefPtr1[5] + (int)RefPtr2[5]) >> 1) + ChangePtr[5] );
    ReconPtr[6] = clamp255((((int)RefPtr1[6] + (int)RefPtr2[6]) >> 1) + ChangePtr[6] );
    ReconPtr[7] = clamp255((((int)RefPtr1[7] + (int)RefPtr2[7]) >> 1) + ChangePtr[7] );

    ChangePtr += BLOCK_HEIGHT_WIDTH;
    ReconPtr += LineStep;
    RefPtr1 += LineStep;
    RefPtr2 += LineStep;
  }

}
