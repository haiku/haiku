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
  last mod: $Id: mcomp.c,v 1.1 2004/02/24 13:50:13 shatty Exp $

 ********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include "encoder_internal.h"

/* Initialises motion compentsation. */
void InitMotionCompensation ( CP_INSTANCE *cpi ){
  int i;
  int SearchSite=0;
  int Len;
  int LineStepY = (ogg_int32_t)cpi->pb.YStride;

  Len=((MAX_MV_EXTENT/2)+1)/2;


  /* How many search stages are there. */
  cpi->MVSearchSteps = 0;

  /* Set up offsets arrays used in half pixel correction. */
  cpi->HalfPixelRef2Offset[0] = -LineStepY - 1;
  cpi->HalfPixelRef2Offset[1] = -LineStepY;
  cpi->HalfPixelRef2Offset[2] = -LineStepY + 1;
  cpi->HalfPixelRef2Offset[3] = - 1;
  cpi->HalfPixelRef2Offset[4] = 0;
  cpi->HalfPixelRef2Offset[5] = 1;
  cpi->HalfPixelRef2Offset[6] = LineStepY - 1;
  cpi->HalfPixelRef2Offset[7] = LineStepY;
  cpi->HalfPixelRef2Offset[8] = LineStepY + 1;

  cpi->HalfPixelXOffset[0] = -1;
  cpi->HalfPixelXOffset[1] = 0;
  cpi->HalfPixelXOffset[2] = 1;
  cpi->HalfPixelXOffset[3] = -1;
  cpi->HalfPixelXOffset[4] = 0;
  cpi->HalfPixelXOffset[5] = 1;
  cpi->HalfPixelXOffset[6] = -1;
  cpi->HalfPixelXOffset[7] = 0;
  cpi->HalfPixelXOffset[8] = 1;

  cpi->HalfPixelYOffset[0] = -1;
  cpi->HalfPixelYOffset[1] = -1;
  cpi->HalfPixelYOffset[2] = -1;
  cpi->HalfPixelYOffset[3] = 0;
  cpi->HalfPixelYOffset[4] = 0;
  cpi->HalfPixelYOffset[5] = 0;
  cpi->HalfPixelYOffset[6] = 1;
  cpi->HalfPixelYOffset[7] = 1;
  cpi->HalfPixelYOffset[8] = 1;


  /* Generate offsets for 8 search sites per step. */
  while ( Len>0 ) {
    /* Another step. */
    cpi->MVSearchSteps += 1;

    /* Compute offsets for search sites. */
    cpi->MVOffsetX[SearchSite] = -Len;
    cpi->MVOffsetY[SearchSite++] = -Len;
    cpi->MVOffsetX[SearchSite] = 0;
    cpi->MVOffsetY[SearchSite++] = -Len;
    cpi->MVOffsetX[SearchSite] = Len;
    cpi->MVOffsetY[SearchSite++] = -Len;
    cpi->MVOffsetX[SearchSite] = -Len;
    cpi->MVOffsetY[SearchSite++] = 0;
    cpi->MVOffsetX[SearchSite] = Len;
    cpi->MVOffsetY[SearchSite++] = 0;
    cpi->MVOffsetX[SearchSite] = -Len;
    cpi->MVOffsetY[SearchSite++] = Len;
    cpi->MVOffsetX[SearchSite] = 0;
    cpi->MVOffsetY[SearchSite++] = Len;
    cpi->MVOffsetX[SearchSite] = Len;
    cpi->MVOffsetY[SearchSite++] = Len;

    /* Contract. */
    Len /= 2;
  }

  /* Compute pixel index offsets. */
  for ( i=SearchSite-1; i>=0; i-- )
    cpi->MVPixelOffsetY[i] = (cpi->MVOffsetY[i]*LineStepY) + cpi->MVOffsetX[i];
}

static ogg_uint32_t GetInterErr (unsigned char * NewDataPtr,
                          unsigned char * RefDataPtr1,
                          unsigned char * RefDataPtr2,
                          ogg_uint32_t PixelsPerLine ) {
  ogg_uint32_t  i;
  ogg_int32_t   XSum=0;
  ogg_int32_t   XXSum=0;
  ogg_int32_t   DiffVal;
  ogg_int32_t   AbsRefOffset = abs((int)(RefDataPtr1 - RefDataPtr2));

  /* Mode of interpolation chosen based upon on the offset of the
     second reference pointer */
  if ( AbsRefOffset == 0 ) {
    for ( i=0; i<BLOCK_HEIGHT_WIDTH; i++ ) {
      DiffVal = ((int)NewDataPtr[0]) - (int)RefDataPtr1[0];
      XSum += DiffVal;

      /* negative array indexes are strictly forbidden by ANSI C and C99 */
      XXSum += DiffVal*DiffVal;

      DiffVal = ((int)NewDataPtr[1]) - (int)RefDataPtr1[1];
      XSum += DiffVal;
      XXSum += DiffVal*DiffVal;

      DiffVal = ((int)NewDataPtr[2]) - (int)RefDataPtr1[2];
      XSum += DiffVal;
      XXSum += DiffVal*DiffVal;

      DiffVal = ((int)NewDataPtr[3]) - (int)RefDataPtr1[3];
      XSum += DiffVal;
      XXSum += DiffVal*DiffVal;

      DiffVal = ((int)NewDataPtr[4]) - (int)RefDataPtr1[4];
      XSum += DiffVal;
      XXSum += DiffVal*DiffVal;

      DiffVal = ((int)NewDataPtr[5]) - (int)RefDataPtr1[5];
      XSum += DiffVal;
      XXSum += DiffVal*DiffVal;

      DiffVal = ((int)NewDataPtr[6]) - (int)RefDataPtr1[6];
      XSum += DiffVal;
      XXSum += DiffVal*DiffVal;

      DiffVal = ((int)NewDataPtr[7]) - (int)RefDataPtr1[7];
      XSum += DiffVal;
      XXSum += DiffVal*DiffVal;

      /* Step to next row of block. */
      NewDataPtr += PixelsPerLine;
      RefDataPtr1 += STRIDE_EXTRA + PixelsPerLine;
    }

  }else{

    /* Simple two reference interpolation */
    for ( i=0; i<BLOCK_HEIGHT_WIDTH; i++ ) {
      DiffVal = ((int)NewDataPtr[0]) -
        (((int)RefDataPtr1[0] + (int)RefDataPtr2[0]) / 2);
      XSum += DiffVal;
      XXSum += DiffVal*DiffVal;

      DiffVal = ((int)NewDataPtr[1]) -
        (((int)RefDataPtr1[1] + (int)RefDataPtr2[1]) / 2);
      XSum += DiffVal;
      XXSum += DiffVal*DiffVal;

      DiffVal = ((int)NewDataPtr[2]) -
        (((int)RefDataPtr1[2] + (int)RefDataPtr2[2]) / 2);
      XSum += DiffVal;
      XXSum += DiffVal*DiffVal;

      DiffVal = ((int)NewDataPtr[3]) -
        (((int)RefDataPtr1[3] + (int)RefDataPtr2[3]) / 2);
      XSum += DiffVal;
      XXSum += DiffVal*DiffVal;

      DiffVal = ((int)NewDataPtr[4]) -
        (((int)RefDataPtr1[4] + (int)RefDataPtr2[4]) / 2);
      XSum += DiffVal;
      XXSum += DiffVal*DiffVal;

      DiffVal = ((int)NewDataPtr[5]) -
        (((int)RefDataPtr1[5] + (int)RefDataPtr2[5]) / 2);
      XSum += DiffVal;
      XXSum += DiffVal*DiffVal;

      DiffVal = ((int)NewDataPtr[6]) -
        (((int)RefDataPtr1[6] + (int)RefDataPtr2[6]) / 2);
      XSum += DiffVal;
      XXSum += DiffVal*DiffVal;

      DiffVal = ((int)NewDataPtr[7]) -
        (((int)RefDataPtr1[7] + (int)RefDataPtr2[7]) / 2);
      XSum += DiffVal;
      XXSum += DiffVal*DiffVal;

      /* Step to next row of block. */
      NewDataPtr += PixelsPerLine;
      RefDataPtr1 += STRIDE_EXTRA+PixelsPerLine;
      RefDataPtr2 += STRIDE_EXTRA+PixelsPerLine;
    }
  }

  /* Compute and return population variance as mis-match metric. */
  return (( (XXSum<<6) - XSum*XSum ));
}

static ogg_uint32_t GetSumAbsDiffs  (unsigned char * NewDataPtr,
                              unsigned char  * RefDataPtr,
                              ogg_uint32_t PixelsPerLine,
                              ogg_uint32_t ErrorSoFar) {
  ogg_uint32_t  i;
  ogg_uint32_t  DiffVal = ErrorSoFar;

  /* Decide on standard or MMX implementation */
  for ( i=0; i < BLOCK_HEIGHT_WIDTH; i++ ) {
    DiffVal += abs( ((int)NewDataPtr[0]) - ((int)RefDataPtr[0]) );
    DiffVal += abs( ((int)NewDataPtr[1]) - ((int)RefDataPtr[1]) );
    DiffVal += abs( ((int)NewDataPtr[2]) - ((int)RefDataPtr[2]) );
    DiffVal += abs( ((int)NewDataPtr[3]) - ((int)RefDataPtr[3]) );
    DiffVal += abs( ((int)NewDataPtr[4]) - ((int)RefDataPtr[4]) );
    DiffVal += abs( ((int)NewDataPtr[5]) - ((int)RefDataPtr[5]) );
    DiffVal += abs( ((int)NewDataPtr[6]) - ((int)RefDataPtr[6]) );
    DiffVal += abs( ((int)NewDataPtr[7]) - ((int)RefDataPtr[7]) );

    /* Step to next row of block. */
    NewDataPtr += PixelsPerLine;
    RefDataPtr += STRIDE_EXTRA+PixelsPerLine;
  }

  return DiffVal;
}

static ogg_uint32_t GetNextSumAbsDiffs (unsigned char * NewDataPtr,
                                 unsigned char * RefDataPtr,
                                 ogg_uint32_t PixelsPerLine,
                                 ogg_uint32_t ErrorSoFar,
                                 ogg_uint32_t BestSoFar ) {
  ogg_uint32_t  i;
  ogg_uint32_t  DiffVal = ErrorSoFar;

  for ( i=0; i < BLOCK_HEIGHT_WIDTH; i++ ) {
    DiffVal += abs( ((int)NewDataPtr[0]) - ((int)RefDataPtr[0]) );
    DiffVal += abs( ((int)NewDataPtr[1]) - ((int)RefDataPtr[1]) );
    DiffVal += abs( ((int)NewDataPtr[2]) - ((int)RefDataPtr[2]) );
    DiffVal += abs( ((int)NewDataPtr[3]) - ((int)RefDataPtr[3]) );
    DiffVal += abs( ((int)NewDataPtr[4]) - ((int)RefDataPtr[4]) );
    DiffVal += abs( ((int)NewDataPtr[5]) - ((int)RefDataPtr[5]) );
    DiffVal += abs( ((int)NewDataPtr[6]) - ((int)RefDataPtr[6]) );
    DiffVal += abs( ((int)NewDataPtr[7]) - ((int)RefDataPtr[7]) );

    if ( DiffVal > BestSoFar )break;

    /* Step to next row of block. */
    NewDataPtr += PixelsPerLine;
    RefDataPtr += STRIDE_EXTRA+PixelsPerLine;
  }

  return DiffVal;
}

static ogg_uint32_t GetHalfPixelSumAbsDiffs (unsigned char * SrcData,
                                      unsigned char * RefDataPtr1,
                                      unsigned char * RefDataPtr2,
                                      ogg_uint32_t PixelsPerLine,
                                      ogg_uint32_t ErrorSoFar,
                                      ogg_uint32_t BestSoFar ) {

  ogg_uint32_t  i;
  ogg_uint32_t  DiffVal = ErrorSoFar;
  ogg_int32_t   RefOffset = (int)(RefDataPtr1 - RefDataPtr2);
  ogg_uint32_t  RefPixelsPerLine = PixelsPerLine + STRIDE_EXTRA;

  if ( RefOffset == 0 ) {
    /* Simple case as for non 0.5 pixel */
    DiffVal += GetSumAbsDiffs( SrcData, RefDataPtr1, PixelsPerLine,
                               ErrorSoFar);
  } else  {
    for ( i=0; i < BLOCK_HEIGHT_WIDTH; i++ ) {
      DiffVal += abs( ((int)SrcData[0]) - (((int)RefDataPtr1[0] +
                                            (int)RefDataPtr2[0]) / 2) );
      DiffVal += abs( ((int)SrcData[1]) - (((int)RefDataPtr1[1] +
                                            (int)RefDataPtr2[1]) / 2) );
      DiffVal += abs( ((int)SrcData[2]) - (((int)RefDataPtr1[2] +
                                            (int)RefDataPtr2[2]) / 2) );
      DiffVal += abs( ((int)SrcData[3]) - (((int)RefDataPtr1[3] +
                                            (int)RefDataPtr2[3]) / 2) );
      DiffVal += abs( ((int)SrcData[4]) - (((int)RefDataPtr1[4] +
                                            (int)RefDataPtr2[4]) / 2) );
      DiffVal += abs( ((int)SrcData[5]) - (((int)RefDataPtr1[5] +
                                            (int)RefDataPtr2[5]) / 2) );
      DiffVal += abs( ((int)SrcData[6]) - (((int)RefDataPtr1[6] +
                                            (int)RefDataPtr2[6]) / 2) );
      DiffVal += abs( ((int)SrcData[7]) - (((int)RefDataPtr1[7] +
                                            (int)RefDataPtr2[7]) / 2) );

      if ( DiffVal > BestSoFar ) break;

      /* Step to next row of block. */
      SrcData += PixelsPerLine;
      RefDataPtr1 += RefPixelsPerLine;
      RefDataPtr2 += RefPixelsPerLine;
    }
  }

  return DiffVal;
}

static ogg_uint32_t GetIntraError (unsigned char * DataPtr,
                            ogg_uint32_t PixelsPerLine ) {
  ogg_uint32_t  i;
  ogg_uint32_t  XSum=0;
  ogg_uint32_t  XXSum=0;
  unsigned char *DiffPtr;

  /* Loop expanded out for speed. */
  DiffPtr = DataPtr;

  for ( i=0; i<BLOCK_HEIGHT_WIDTH; i++ ) {

    /* Examine alternate pixel locations. */
    XSum += DiffPtr[0];
    XXSum += DiffPtr[0]*DiffPtr[0];
    XSum += DiffPtr[1];
    XXSum += DiffPtr[1]*DiffPtr[1];
    XSum += DiffPtr[2];
    XXSum += DiffPtr[2]*DiffPtr[2];
    XSum += DiffPtr[3];
    XXSum += DiffPtr[3]*DiffPtr[3];
    XSum += DiffPtr[4];
    XXSum += DiffPtr[4]*DiffPtr[4];
    XSum += DiffPtr[5];
    XXSum += DiffPtr[5]*DiffPtr[5];
    XSum += DiffPtr[6];
    XXSum += DiffPtr[6]*DiffPtr[6];
    XSum += DiffPtr[7];
    XXSum += DiffPtr[7]*DiffPtr[7];

    /* Step to next row of block. */
    DiffPtr += PixelsPerLine;
  }

  /* Compute population variance as mis-match metric. */
  return (( (XXSum<<6) - XSum*XSum ) );
}

ogg_uint32_t GetMBIntraError (CP_INSTANCE *cpi, ogg_uint32_t FragIndex,
                              ogg_uint32_t PixelsPerLine ) {
  ogg_uint32_t  LocalFragIndex = FragIndex;
  ogg_uint32_t  IntraError = 0;

  /* Add together the intra errors for those blocks in the macro block
     that are coded (Y only) */
  if ( cpi->pb.display_fragments[LocalFragIndex] )
    IntraError +=
      GetIntraError(&cpi->
                    ConvDestBuffer[cpi->pb.pixel_index_table[LocalFragIndex]],
                    PixelsPerLine );


  LocalFragIndex++;
  if ( cpi->pb.display_fragments[LocalFragIndex] )
    IntraError +=
      GetIntraError(&cpi->
                    ConvDestBuffer[cpi->pb.pixel_index_table[LocalFragIndex]],
                    PixelsPerLine );

  LocalFragIndex = FragIndex + cpi->pb.HFragments;
  if ( cpi->pb.display_fragments[LocalFragIndex] )
    IntraError +=
      GetIntraError(&cpi->
                     ConvDestBuffer[cpi->pb.pixel_index_table[LocalFragIndex]],
                     PixelsPerLine );

  LocalFragIndex++;
  if ( cpi->pb.display_fragments[LocalFragIndex] )
    IntraError +=
      GetIntraError(&cpi->
                    ConvDestBuffer[cpi->pb.pixel_index_table[LocalFragIndex]],
                    PixelsPerLine );

  return IntraError;
}

ogg_uint32_t GetMBInterError (CP_INSTANCE *cpi,
                              unsigned char * SrcPtr,
                              unsigned char * RefPtr,
                              ogg_uint32_t FragIndex,
                              ogg_int32_t LastXMV,
                              ogg_int32_t LastYMV,
                              ogg_uint32_t PixelsPerLine ) {
  ogg_uint32_t  RefPixelsPerLine = cpi->pb.YStride;
  ogg_uint32_t  LocalFragIndex = FragIndex;
  ogg_int32_t   PixelIndex;
  ogg_int32_t   RefPixelIndex;
  ogg_int32_t   RefPixelOffset;
  ogg_int32_t   RefPtr2Offset;

  ogg_uint32_t  InterError = 0;

  unsigned char * SrcPtr1;
  unsigned char * RefPtr1;

  /* Work out pixel offset into source buffer. */
  PixelIndex = cpi->pb.pixel_index_table[LocalFragIndex];

  /* Work out the pixel offset in reference buffer for the default
     motion vector */
  RefPixelIndex = cpi->pb.recon_pixel_index_table[LocalFragIndex];
  RefPixelOffset = ((LastYMV/2) * RefPixelsPerLine) + (LastXMV/2);

  /* Work out the second reference pointer offset. */
  RefPtr2Offset = 0;
  if ( LastXMV % 2 ) {
    if ( LastXMV > 0 )
      RefPtr2Offset += 1;
    else
      RefPtr2Offset -= 1;
  }
  if ( LastYMV % 2 ) {
    if ( LastYMV > 0 )
      RefPtr2Offset += RefPixelsPerLine;
    else
      RefPtr2Offset -= RefPixelsPerLine;
  }

  /* Add together the errors for those blocks in the macro block that
     are coded (Y only) */
  if ( cpi->pb.display_fragments[LocalFragIndex] ) {
    SrcPtr1 = &SrcPtr[PixelIndex];
    RefPtr1 = &RefPtr[RefPixelIndex + RefPixelOffset];
    InterError += GetInterErr( SrcPtr1, RefPtr1,
                                 &RefPtr1[RefPtr2Offset], PixelsPerLine );
  }

  LocalFragIndex++;
  if ( cpi->pb.display_fragments[LocalFragIndex] ) {
    PixelIndex = cpi->pb.pixel_index_table[LocalFragIndex];
    RefPixelIndex = cpi->pb.recon_pixel_index_table[LocalFragIndex];
    SrcPtr1 = &SrcPtr[PixelIndex];
    RefPtr1 = &RefPtr[RefPixelIndex + RefPixelOffset];
    InterError += GetInterErr( SrcPtr1, RefPtr1,
                                 &RefPtr1[RefPtr2Offset], PixelsPerLine );

  }

  LocalFragIndex = FragIndex + cpi->pb.HFragments;
  if ( cpi->pb.display_fragments[LocalFragIndex] ) {
    PixelIndex = cpi->pb.pixel_index_table[LocalFragIndex];
    RefPixelIndex = cpi->pb.recon_pixel_index_table[LocalFragIndex];
    SrcPtr1 = &SrcPtr[PixelIndex];
    RefPtr1 = &RefPtr[RefPixelIndex + RefPixelOffset];
    InterError += GetInterErr( SrcPtr1, RefPtr1,
                                 &RefPtr1[RefPtr2Offset], PixelsPerLine );
  }

  LocalFragIndex++;
  if ( cpi->pb.display_fragments[LocalFragIndex] ) {
    PixelIndex = cpi->pb.pixel_index_table[LocalFragIndex];
    RefPixelIndex = cpi->pb.recon_pixel_index_table[LocalFragIndex];
    SrcPtr1 = &SrcPtr[PixelIndex];
    RefPtr1 = &RefPtr[RefPixelIndex + RefPixelOffset];
    InterError += GetInterErr( SrcPtr1, RefPtr1,
                                 &RefPtr1[RefPtr2Offset], PixelsPerLine );
  }
  return InterError;
}

ogg_uint32_t GetMBMVInterError (CP_INSTANCE *cpi,
                                unsigned char * RefFramePtr,
                                ogg_uint32_t FragIndex,
                                ogg_uint32_t PixelsPerLine,
                                ogg_int32_t *MVPixelOffset,
                                MOTION_VECTOR *MV ) {
  ogg_uint32_t  Error = 0;
  ogg_uint32_t  MinError;
  ogg_uint32_t  InterMVError = 0;

  ogg_int32_t   i;
  ogg_int32_t   x=0, y=0;
  ogg_int32_t   step;
  ogg_int32_t   SearchSite=0;

  unsigned char *SrcPtr[4] = {NULL,NULL,NULL,NULL};
  unsigned char *RefPtr=NULL;
  unsigned char *CandidateBlockPtr=NULL;
  unsigned char *BestBlockPtr=NULL;

  ogg_uint32_t  RefRow2Offset = cpi->pb.YStride * 8;

  int    MBlockDispFrags[4];

  /* Half pixel variables */
  ogg_int32_t   HalfPixelError;
  ogg_int32_t   BestHalfPixelError;
  unsigned char   BestHalfOffset;
  unsigned char * RefDataPtr1;
  unsigned char * RefDataPtr2;

  /* Note which of the four blocks in the macro block are to be
     included in the search. */
  MBlockDispFrags[0] =
    cpi->pb.display_fragments[FragIndex];
  MBlockDispFrags[1] =
    cpi->pb.display_fragments[FragIndex + 1];
  MBlockDispFrags[2] =
    cpi->pb.display_fragments[FragIndex + cpi->pb.HFragments];
  MBlockDispFrags[3] =
    cpi->pb.display_fragments[FragIndex + cpi->pb.HFragments + 1];

  /* Set up the source pointers for the four source blocks.  */
  SrcPtr[0] = &cpi->ConvDestBuffer[cpi->pb.pixel_index_table[FragIndex]];
  SrcPtr[1] = SrcPtr[0] + 8;
  SrcPtr[2] = SrcPtr[0] + (PixelsPerLine * 8);
  SrcPtr[3] = SrcPtr[2] + 8;

  /* Set starting reference point for search. */
  RefPtr = &RefFramePtr[cpi->pb.recon_pixel_index_table[FragIndex]];

  /* Check the 0,0 candidate. */
  if ( MBlockDispFrags[0] ) {
    Error = GetSumAbsDiffs( SrcPtr[0], RefPtr,
                         PixelsPerLine, Error);
  }
  if ( MBlockDispFrags[1] ) {
    Error = GetSumAbsDiffs( SrcPtr[1], RefPtr + 8,
                         PixelsPerLine, Error);
  }
  if ( MBlockDispFrags[2] ) {
    Error = GetSumAbsDiffs( SrcPtr[2], RefPtr + RefRow2Offset,
                         PixelsPerLine, Error);
  }
  if ( MBlockDispFrags[3] ) {
    Error = GetSumAbsDiffs( SrcPtr[3], RefPtr + RefRow2Offset + 8,
                         PixelsPerLine, Error);
  }

  /* Set starting values to results of 0, 0 vector. */
  MinError = Error;
  BestBlockPtr = RefPtr;
  x = 0;
  y = 0;
  MV->x = 0;
  MV->y = 0;

  /* Proceed through N-steps. */
  for (  step=0; step<cpi->MVSearchSteps; step++ ) {
    /* Search the 8-neighbours at distance pertinent to current step.*/
    for ( i=0; i<8; i++ ) {
      /* Set pointer to next candidate matching block. */
      CandidateBlockPtr = RefPtr + MVPixelOffset[SearchSite];

      /* Reset error */
      Error = 0;

      /* Get the score for the current offset */
      if ( MBlockDispFrags[0] ) {
        Error = GetSumAbsDiffs( SrcPtr[0], CandidateBlockPtr,
                             PixelsPerLine, Error);
      }

      if ( MBlockDispFrags[1] && (Error < MinError) ) {
        Error = GetNextSumAbsDiffs( SrcPtr[1], CandidateBlockPtr + 8,
                                 PixelsPerLine, Error, MinError );
      }

      if ( MBlockDispFrags[2] && (Error < MinError) ) {
        Error = GetNextSumAbsDiffs( SrcPtr[2], CandidateBlockPtr + RefRow2Offset,
                                 PixelsPerLine, Error, MinError );
      }

      if ( MBlockDispFrags[3] && (Error < MinError) ) {
        Error = GetNextSumAbsDiffs( SrcPtr[3],
                                 CandidateBlockPtr + RefRow2Offset + 8,
                                 PixelsPerLine, Error, MinError );
      }

      if ( Error < MinError ) {
        /* Remember best match. */
        MinError = Error;
        BestBlockPtr = CandidateBlockPtr;

                                /* Where is it. */
        x = MV->x + cpi->MVOffsetX[SearchSite];
        y = MV->y + cpi->MVOffsetY[SearchSite];
      }

      /* Move to next search location. */
      SearchSite += 1;
    }

    /* Move to best location this step. */
    RefPtr = BestBlockPtr;
    MV->x = x;
    MV->y = y;
  }

  /* Factor vectors to 1/2 pixel resoultion. */
  MV->x = (MV->x * 2);
  MV->y = (MV->y * 2);

  /* Now do the half pixel pass */
  BestHalfOffset = 4;     /* Default to the no offset case. */
  BestHalfPixelError = MinError;

  /* Get the half pixel error for each half pixel offset */
  for ( i=0; i < 9; i++ ) {
    HalfPixelError = 0;

    if ( MBlockDispFrags[0] ) {
      RefDataPtr1 = BestBlockPtr;
      RefDataPtr2 = RefDataPtr1 + cpi->HalfPixelRef2Offset[i];
      HalfPixelError =
        GetHalfPixelSumAbsDiffs( SrcPtr[0], RefDataPtr1, RefDataPtr2,
                         PixelsPerLine, HalfPixelError, BestHalfPixelError );
    }

    if ( MBlockDispFrags[1]  && (HalfPixelError < BestHalfPixelError) ) {
      RefDataPtr1 = BestBlockPtr + 8;
      RefDataPtr2 = RefDataPtr1 + cpi->HalfPixelRef2Offset[i];
      HalfPixelError =
        GetHalfPixelSumAbsDiffs( SrcPtr[1], RefDataPtr1, RefDataPtr2,
                         PixelsPerLine, HalfPixelError, BestHalfPixelError );
    }

    if ( MBlockDispFrags[2] && (HalfPixelError < BestHalfPixelError) ) {
      RefDataPtr1 = BestBlockPtr + RefRow2Offset;
      RefDataPtr2 = RefDataPtr1 + cpi->HalfPixelRef2Offset[i];
      HalfPixelError =
        GetHalfPixelSumAbsDiffs( SrcPtr[2], RefDataPtr1, RefDataPtr2,
                         PixelsPerLine, HalfPixelError, BestHalfPixelError );
    }

    if ( MBlockDispFrags[3] && (HalfPixelError < BestHalfPixelError) ) {
      RefDataPtr1 = BestBlockPtr + RefRow2Offset + 8;
      RefDataPtr2 = RefDataPtr1 + cpi->HalfPixelRef2Offset[i];
      HalfPixelError =
        GetHalfPixelSumAbsDiffs( SrcPtr[3], RefDataPtr1, RefDataPtr2,
                         PixelsPerLine, HalfPixelError, BestHalfPixelError );
    }

    if ( HalfPixelError < BestHalfPixelError ) {
      BestHalfOffset = (unsigned char)i;
      BestHalfPixelError = HalfPixelError;
    }
  }

  /* Half pixel adjust the MV */
  MV->x += cpi->HalfPixelXOffset[BestHalfOffset];
  MV->y += cpi->HalfPixelYOffset[BestHalfOffset];

  /* Get the error score for the chosen 1/2 pixel offset as a variance. */
  InterMVError = GetMBInterError( cpi, cpi->ConvDestBuffer, RefFramePtr,
                                  FragIndex, MV->x, MV->y, PixelsPerLine );

  /* Return score of best matching block. */
  return InterMVError;
}

ogg_uint32_t GetMBMVExhaustiveSearch (CP_INSTANCE *cpi,
                                      unsigned char * RefFramePtr,
                                      ogg_uint32_t FragIndex,
                                      ogg_uint32_t PixelsPerLine,
                                      MOTION_VECTOR *MV ) {
  ogg_uint32_t  Error = 0;
  ogg_uint32_t  MinError = HUGE_ERROR;
  ogg_uint32_t  InterMVError = 0;

  ogg_int32_t   i, j;
  ogg_int32_t   x=0, y=0;

  unsigned char *SrcPtr[4] = {NULL,NULL,NULL,NULL};
  unsigned char *RefPtr;
  unsigned char *CandidateBlockPtr=NULL;
  unsigned char *BestBlockPtr=NULL;

  ogg_uint32_t  RefRow2Offset = cpi->pb.YStride * 8;

  int    MBlockDispFrags[4];

  /* Half pixel variables */
  ogg_int32_t   HalfPixelError;
  ogg_int32_t   BestHalfPixelError;
  unsigned char   BestHalfOffset;
  unsigned char * RefDataPtr1;
  unsigned char * RefDataPtr2;

  /* Note which of the four blocks in the macro block are to be
     included in the search. */
  MBlockDispFrags[0] = cpi->
    pb.display_fragments[FragIndex];
  MBlockDispFrags[1] = cpi->
    pb.display_fragments[FragIndex + 1];
  MBlockDispFrags[2] = cpi->
    pb.display_fragments[FragIndex + cpi->pb.HFragments];
  MBlockDispFrags[3] = cpi->
    pb.display_fragments[FragIndex + cpi->pb.HFragments + 1];

  /* Set up the source pointers for the four source blocks. */
  SrcPtr[0] = &cpi->
    ConvDestBuffer[cpi->pb.pixel_index_table[FragIndex]];
  SrcPtr[1] = SrcPtr[0] + 8;
  SrcPtr[2] = SrcPtr[0] + (PixelsPerLine * 8);
  SrcPtr[3] = SrcPtr[2] + 8;

  RefPtr = &RefFramePtr[cpi->pb.recon_pixel_index_table[FragIndex]];
  RefPtr = RefPtr - ((MAX_MV_EXTENT/2) * cpi->
                     pb.YStride) - (MAX_MV_EXTENT/2);

  /* Search each pixel alligned site */
  for ( i = 0; i < (ogg_int32_t)MAX_MV_EXTENT; i ++ ) {
    /* Starting position in row */
    CandidateBlockPtr = RefPtr;

    for ( j = 0; j < (ogg_int32_t)MAX_MV_EXTENT; j++ ) {
      /* Reset error */
      Error = 0;

      /* Summ errors for each block. */
      if ( MBlockDispFrags[0] ) {
        Error = GetSumAbsDiffs( SrcPtr[0], CandidateBlockPtr,
                             PixelsPerLine, Error);
      }
      if ( MBlockDispFrags[1] ){
        Error = GetSumAbsDiffs( SrcPtr[1], CandidateBlockPtr + 8,
                             PixelsPerLine, Error);
      }
      if ( MBlockDispFrags[2] ){
        Error = GetSumAbsDiffs( SrcPtr[2], CandidateBlockPtr + RefRow2Offset,
                             PixelsPerLine, Error);
      }
      if ( MBlockDispFrags[3] ){
        Error = GetSumAbsDiffs( SrcPtr[3], CandidateBlockPtr + RefRow2Offset + 8,
                             PixelsPerLine, Error);
      }

      /* Was this the best so far */
      if ( Error < MinError ) {
        MinError = Error;
        BestBlockPtr = CandidateBlockPtr;
        x = 16 + j - MAX_MV_EXTENT;
        y = 16 + i - MAX_MV_EXTENT;
      }

      /* Move the the next site */
      CandidateBlockPtr ++;
    }

    /* Move on to the next row. */
    RefPtr += cpi->pb.YStride;

  }

  /* Factor vectors to 1/2 pixel resoultion. */
  MV->x = (x * 2);
  MV->y = (y * 2);

  /* Now do the half pixel pass */
  BestHalfOffset = 4;     /* Default to the no offset case. */
  BestHalfPixelError = MinError;

  /* Get the half pixel error for each half pixel offset */
  for ( i=0; i < 9; i++ ) {
    HalfPixelError = 0;

    if ( MBlockDispFrags[0] ) {
      RefDataPtr1 = BestBlockPtr;
      RefDataPtr2 = RefDataPtr1 + cpi->HalfPixelRef2Offset[i];
      HalfPixelError =
        GetHalfPixelSumAbsDiffs( SrcPtr[0], RefDataPtr1, RefDataPtr2,
                         PixelsPerLine, HalfPixelError, BestHalfPixelError );
    }

    if ( MBlockDispFrags[1]  && (HalfPixelError < BestHalfPixelError) ) {
      RefDataPtr1 = BestBlockPtr + 8;
      RefDataPtr2 = RefDataPtr1 + cpi->HalfPixelRef2Offset[i];
      HalfPixelError =
        GetHalfPixelSumAbsDiffs( SrcPtr[1], RefDataPtr1, RefDataPtr2,
                         PixelsPerLine, HalfPixelError, BestHalfPixelError );
    }

    if ( MBlockDispFrags[2] && (HalfPixelError < BestHalfPixelError) ) {
      RefDataPtr1 = BestBlockPtr + RefRow2Offset;
      RefDataPtr2 = RefDataPtr1 + cpi->HalfPixelRef2Offset[i];
      HalfPixelError =
        GetHalfPixelSumAbsDiffs( SrcPtr[2], RefDataPtr1, RefDataPtr2,
                         PixelsPerLine, HalfPixelError, BestHalfPixelError );
    }

    if ( MBlockDispFrags[3] && (HalfPixelError < BestHalfPixelError) ) {
      RefDataPtr1 = BestBlockPtr + RefRow2Offset + 8;
      RefDataPtr2 = RefDataPtr1 + cpi->HalfPixelRef2Offset[i];
      HalfPixelError =
        GetHalfPixelSumAbsDiffs( SrcPtr[3], RefDataPtr1, RefDataPtr2,
                         PixelsPerLine, HalfPixelError, BestHalfPixelError );
    }

    if ( HalfPixelError < BestHalfPixelError ){
      BestHalfOffset = (unsigned char)i;
      BestHalfPixelError = HalfPixelError;
    }
  }

  /* Half pixel adjust the MV */
  MV->x += cpi->HalfPixelXOffset[BestHalfOffset];
  MV->y += cpi->HalfPixelYOffset[BestHalfOffset];

  /* Get the error score for the chosen 1/2 pixel offset as a variance. */
  InterMVError = GetMBInterError( cpi, cpi->ConvDestBuffer, RefFramePtr,
                                  FragIndex, MV->x, MV->y, PixelsPerLine );

  /* Return score of best matching block. */
  return InterMVError;
}

static ogg_uint32_t GetBMVExhaustiveSearch (CP_INSTANCE *cpi,
                                            unsigned char * RefFramePtr,
                                            ogg_uint32_t FragIndex,
                                            ogg_uint32_t PixelsPerLine,
                                            MOTION_VECTOR *MV ) {
  ogg_uint32_t  Error = 0;
  ogg_uint32_t  MinError = HUGE_ERROR;
  ogg_uint32_t  InterMVError = 0;

  ogg_int32_t   i, j;
  ogg_int32_t   x=0, y=0;

  unsigned char *SrcPtr = NULL;
  unsigned char *RefPtr;
  unsigned char *CandidateBlockPtr=NULL;
  unsigned char *BestBlockPtr=NULL;

  /* Half pixel variables */
  ogg_int32_t   HalfPixelError;
  ogg_int32_t   BestHalfPixelError;
  unsigned char   BestHalfOffset;
  unsigned char * RefDataPtr2;

  /* Set up the source pointer for the block. */
  SrcPtr = &cpi->
    ConvDestBuffer[cpi->pb.pixel_index_table[FragIndex]];

  RefPtr = &RefFramePtr[cpi->pb.recon_pixel_index_table[FragIndex]];
  RefPtr = RefPtr - ((MAX_MV_EXTENT/2) *
                     cpi->pb.YStride) - (MAX_MV_EXTENT/2);

  /* Search each pixel alligned site */
  for ( i = 0; i < (ogg_int32_t)MAX_MV_EXTENT; i ++ ) {
    /* Starting position in row */
    CandidateBlockPtr = RefPtr;

    for ( j = 0; j < (ogg_int32_t)MAX_MV_EXTENT; j++ ){
      /* Get the block error score. */
      Error = GetSumAbsDiffs( SrcPtr, CandidateBlockPtr,
                           PixelsPerLine, 0);

      /* Was this the best so far */
      if ( Error < MinError ) {
        MinError = Error;
        BestBlockPtr = CandidateBlockPtr;
        x = 16 + j - MAX_MV_EXTENT;
        y = 16 + i - MAX_MV_EXTENT;
      }

      /* Move the the next site */
      CandidateBlockPtr ++;
    }

    /* Move on to the next row. */
    RefPtr += cpi->pb.YStride;
  }

  /* Factor vectors to 1/2 pixel resoultion. */
  MV->x = (x * 2);
  MV->y = (y * 2);

  /* Now do the half pixel pass */
  BestHalfOffset = 4;     /* Default to the no offset case. */
  BestHalfPixelError = MinError;

  /* Get the half pixel error for each half pixel offset */
  for ( i=0; i < 9; i++ ) {
    RefDataPtr2 = BestBlockPtr + cpi->HalfPixelRef2Offset[i];
    HalfPixelError =
      GetHalfPixelSumAbsDiffs( SrcPtr, BestBlockPtr, RefDataPtr2,
                            PixelsPerLine, 0, BestHalfPixelError );

    if ( HalfPixelError < BestHalfPixelError ){
      BestHalfOffset = (unsigned char)i;
      BestHalfPixelError = HalfPixelError;
    }
  }

  /* Half pixel adjust the MV */
  MV->x += cpi->HalfPixelXOffset[BestHalfOffset];
  MV->y += cpi->HalfPixelYOffset[BestHalfOffset];

  /* Get the variance score at the chosen offset */
  RefDataPtr2 = BestBlockPtr + cpi->HalfPixelRef2Offset[BestHalfOffset];

  InterMVError =
    GetInterErr( SrcPtr, BestBlockPtr, RefDataPtr2, PixelsPerLine );

  /* Return score of best matching block. */
  return InterMVError;
}

ogg_uint32_t GetFOURMVExhaustiveSearch (CP_INSTANCE *cpi,
                                        unsigned char * RefFramePtr,
                                        ogg_uint32_t FragIndex,
                                        ogg_uint32_t PixelsPerLine,
                                        MOTION_VECTOR *MV ) {
  ogg_uint32_t  InterMVError;

  /* For the moment the 4MV mode is only deemd to be valid if all four
     Y blocks are to be updated */
  /* This May be adapted later. */
  if ( cpi->pb.display_fragments[FragIndex] &&
       cpi->pb.display_fragments[FragIndex + 1] &&
       cpi->pb.display_fragments[FragIndex + cpi->pb.HFragments] &&
       cpi->pb.display_fragments[FragIndex + cpi->pb.HFragments + 1] ) {

    /* Reset the error score. */
    InterMVError = 0;

    /* Get the error component from each coded block */
    InterMVError +=
      GetBMVExhaustiveSearch(cpi, RefFramePtr, FragIndex,
                             PixelsPerLine, &(MV[0]) );
    InterMVError +=
      GetBMVExhaustiveSearch(cpi, RefFramePtr, (FragIndex + 1),
                             PixelsPerLine, &(MV[1]) );
    InterMVError +=
      GetBMVExhaustiveSearch(cpi, RefFramePtr,
                             (FragIndex + cpi->pb.HFragments),
                             PixelsPerLine, &(MV[2]) );
    InterMVError +=
      GetBMVExhaustiveSearch(cpi, RefFramePtr,
                             (FragIndex + cpi->pb.HFragments + 1),
                             PixelsPerLine, &(MV[3]) );
  }else{
    InterMVError = HUGE_ERROR;
  }

  /* Return score of best matching block. */
  return InterMVError;
}

