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
  last mod: $Id: frarray.c,v 1.1 2004/02/24 13:50:13 shatty Exp $

 ********************************************************************/

#include <string.h>
#include "encoder_internal.h"
#include "block_inline.h"

static ogg_uint32_t FrArrayCodeSBRun( CP_INSTANCE *cpi, ogg_uint32_t value ){
  ogg_uint32_t CodedVal = 0;
  ogg_uint32_t CodedBits = 0;

  /* Coding scheme:
        Codeword              RunLength
      0                       1
      10x                     2-3
      110x                    4-5
      1110xx                  6-9
      11110xxx                10-17
      111110xxxx              18-33
      111111xxxxxxxxxxxx      34-4129 */

  if ( value == 1 ){
    CodedVal = 0;
    CodedBits = 1;
  } else if ( value <= 3 ) {
    CodedVal = 0x0004 + (value - 2);
    CodedBits = 3;
  } else if ( value <= 5 ) {
    CodedVal = 0x000C + (value - 4);
    CodedBits = 4;
  } else if ( value <= 9 ) {
    CodedVal = 0x0038 + (value - 6);
    CodedBits = 6;
  } else if ( value <= 17 ) {
    CodedVal = 0x00F0 + (value - 10);
    CodedBits = 8;
  } else if ( value <= 33 ) {
    CodedVal = 0x03E0 + (value - 18);
    CodedBits = 10;
  } else {
    CodedVal = 0x3F000 + (value - 34);
    CodedBits = 18;
  }

  /* Add the bits to the encode holding buffer. */
  oggpackB_write( cpi->oggbuffer, CodedVal, (ogg_uint32_t)CodedBits );

  return CodedBits;
}

static ogg_uint32_t FrArrayCodeBlockRun( CP_INSTANCE *cpi,
                                         ogg_uint32_t value ) {
  ogg_uint32_t CodedVal = 0;
  ogg_uint32_t CodedBits = 0;

  /* Coding scheme:
        Codeword                                RunLength
        0x                                      1-2
        10x                                     3-4
        110x                                    5-6
        1110xx                                  7-10
        11110xx                                 11-14
        11111xxxx                               15-30 */

  if ( value <= 2 ) {
    CodedVal = value - 1;
    CodedBits = 2;
  } else if ( value <= 4 ) {
    CodedVal = 0x0004 + (value - 3);
    CodedBits = 3;

  } else if ( value <= 6 ) {
    CodedVal = 0x000C + (value - 5);
    CodedBits = 4;

  } else if ( value <= 10 ) {
    CodedVal = 0x0038 + (value - 7);
    CodedBits = 6;

  } else if ( value <= 14 ) {
    CodedVal = 0x0078 + (value - 11);
    CodedBits = 7;
  } else {
    CodedVal = 0x01F0 + (value - 15);
    CodedBits = 9;
 }

  /* Add the bits to the encode holding buffer. */
  oggpackB_write( cpi->oggbuffer, CodedVal, (ogg_uint32_t)CodedBits );

  return CodedBits;
}

void PackAndWriteDFArray( CP_INSTANCE *cpi ){
  ogg_uint32_t  i;
  unsigned char val;
  ogg_uint32_t  run_count;

  ogg_uint32_t  SB, MB, B;   /* Block, MB and SB loop variables */
  ogg_uint32_t  BListIndex = 0;
  ogg_uint32_t  LastSbBIndex = 0;
  ogg_int32_t   DfBlockIndex;  /* Block index in display_fragments */

  /* Initialise workspaces */
  memset( cpi->pb.SBFullyFlags, 1, cpi->pb.SuperBlocks);
  memset( cpi->pb.SBCodedFlags, 0, cpi->pb.SuperBlocks );
  memset( cpi->PartiallyCodedFlags, 0, cpi->pb.SuperBlocks );
  memset( cpi->BlockCodedFlags, 0, cpi->pb.UnitFragments);

  for( SB = 0; SB < cpi->pb.SuperBlocks; SB++ ) {
    /* Check for coded blocks and macro-blocks */
    for ( MB=0; MB<4; MB++ ) {
      /* If MB in frame */
      if ( QuadMapToMBTopLeft(cpi->pb.BlockMap,SB,MB) >= 0 ) {
        for ( B=0; B<4; B++ ) {
          DfBlockIndex = QuadMapToIndex1( cpi->pb.BlockMap,SB, MB, B );

          /* Does Block lie in frame: */
          if ( DfBlockIndex >= 0 ) {
            /* In Frame: If it is not coded then this SB is only
               partly coded.: */
            if ( cpi->pb.display_fragments[DfBlockIndex] ) {
              cpi->pb.SBCodedFlags[SB] = 1; /* SB at least partly coded */
              cpi->BlockCodedFlags[BListIndex] = 1; /* Block is coded */
            }else{
              cpi->pb.SBFullyFlags[SB] = 0; /* SB not fully coded */
              cpi->BlockCodedFlags[BListIndex] = 0; /* Block is not coded */
            }

            BListIndex++;
          }
        }
      }
    }

    /* Is the SB fully coded or uncoded.
       If so then backup BListIndex and MBListIndex */
    if ( cpi->pb.SBFullyFlags[SB] || !cpi->pb.SBCodedFlags[SB] ) {
      BListIndex = LastSbBIndex; /* Reset to values from previous SB */
    }else{
      cpi->PartiallyCodedFlags[SB] = 1; /* Set up list of partially
                                           coded SBs */
      LastSbBIndex = BListIndex;
    }
  }

  /* Code list of partially coded Super-Block.  */
  val = cpi->PartiallyCodedFlags[0];
  oggpackB_write( cpi->oggbuffer, (ogg_uint32_t)val, 1);
  i = 0;
  while ( i < cpi->pb.SuperBlocks ) {
    run_count = 0;
    while ( (i<cpi->pb.SuperBlocks) && (cpi->PartiallyCodedFlags[i]==val) ) {
      i++;
      run_count++;
    }

    /* Code the run */
    FrArrayCodeSBRun( cpi, run_count );
    val = ( val == 0 ) ? 1 : 0;
  }

  /* RLC Super-Block fully/not coded. */
  i = 0;

  /* Skip partially coded blocks */
  while( (i < cpi->pb.SuperBlocks) && cpi->PartiallyCodedFlags[i] )
    i++;

  if ( i < cpi->pb.SuperBlocks ) {
    val = cpi->pb.SBFullyFlags[i];
    oggpackB_write( cpi->oggbuffer, (ogg_uint32_t)val, 1);

    while ( i < cpi->pb.SuperBlocks ) {
      run_count = 0;
      while ( (i < cpi->pb.SuperBlocks) && (cpi->pb.SBFullyFlags[i] == val) ) {
        i++;
        /* Skip partially coded blocks */
        while( (i < cpi->pb.SuperBlocks) && cpi->PartiallyCodedFlags[i] )
          i++;
        run_count++;
      }

      /* Code the run */
      FrArrayCodeSBRun( cpi, run_count );
      val = ( val == 0 ) ? 1 : 0;
    }
  }

  /*  Now code the block flags */
  if ( BListIndex > 0 ) {
    /* Code the block flags start value */
    val = cpi->BlockCodedFlags[0];
    oggpackB_write( cpi->oggbuffer, (ogg_uint32_t)val, 1);

    /* Now code the block flags. */
    for ( i = 0; i < BListIndex; ) {
      run_count = 0;
      while ( (cpi->BlockCodedFlags[i] == val) && (i < BListIndex) ) {
        i++;
        run_count++;
      }

      FrArrayCodeBlockRun( cpi, run_count );
      val = ( val == 0 ) ? 1 : 0;

    }
  }
}

static void FrArrayDeCodeInit(PB_INSTANCE *pbi){
  /* Initialise the decoding of a run.  */
  pbi->bit_pattern = 0;
  pbi->bits_so_far = 0;
}

static int FrArrayDeCodeBlockRun(  PB_INSTANCE *pbi, ogg_uint32_t bit_value,
                            ogg_int32_t * run_value ){
  int  ret_val = 0;

  /* Add in the new bit value. */
  pbi->bits_so_far++;
  pbi->bit_pattern = (pbi->bit_pattern << 1) + (bit_value & 1);

  /* Coding scheme:
     Codeword           RunLength
     0x                    1-2
     10x                   3-4
     110x                  5-6
     1110xx                7-10
     11110xx              11-14
     11111xxxx            15-30
  */

  switch ( pbi->bits_so_far ){
  case 2:
    /* If bit 1 is clear */
    if ( !(pbi->bit_pattern & 0x0002) ){
      ret_val = 1;
      *run_value = (pbi->bit_pattern & 0x0001) + 1;
    }
    break;

  case 3:
    /* If bit 1 is clear */
    if ( !(pbi->bit_pattern & 0x0002) ){
      ret_val = 1;
      *run_value = (pbi->bit_pattern & 0x0001) + 3;
    }
    break;

  case 4:
    /* If bit 1 is clear */
    if ( !(pbi->bit_pattern & 0x0002) ){
      ret_val = 1;
      *run_value = (pbi->bit_pattern & 0x0001) + 5;
    }
    break;

  case 6:
    /* If bit 2 is clear */
    if ( !(pbi->bit_pattern & 0x0004) ){
      ret_val = 1;
      *run_value = (pbi->bit_pattern & 0x0003) + 7;
    }
    break;

  case 7:
    /* If bit 2 is clear */
    if ( !(pbi->bit_pattern & 0x0004) ){
      ret_val = 1;
      *run_value = (pbi->bit_pattern & 0x0003) + 11;
    }
    break;

  case 9:
    ret_val = 1;
    *run_value = (pbi->bit_pattern & 0x000F) + 15;
    break;
  }

  return ret_val;
}

static int FrArrayDeCodeSBRun (PB_INSTANCE *pbi, ogg_uint32_t bit_value,
                        ogg_int32_t * run_value ){
  int ret_val = 0;

  /* Add in the new bit value. */
  pbi->bits_so_far++;
  pbi->bit_pattern = (pbi->bit_pattern << 1) + (bit_value & 1);

  /* Coding scheme:
     Codeword            RunLength
     0                       1
     10x                    2-3
     110x                   4-5
     1110xx                 6-9
     11110xxx              10-17
     111110xxxx            18-33
     111111xxxxxxxxxxxx    34-4129
  */

  switch ( pbi->bits_so_far ){
  case 1:
    if ( pbi->bit_pattern == 0 ){
      ret_val = 1;
      *run_value = 1;
    }
    break;

  case 3:
    /* Bit 1 clear */
    if ( !(pbi->bit_pattern & 0x0002) ){
      ret_val = 1;
      *run_value = (pbi->bit_pattern & 0x0001) + 2;
    }
    break;

  case 4:
    /* Bit 1 clear */
    if ( !(pbi->bit_pattern & 0x0002) ){
      ret_val = 1;
      *run_value = (pbi->bit_pattern & 0x0001) + 4;
    }
    break;

  case 6:
    /* Bit 2 clear */
    if ( !(pbi->bit_pattern & 0x0004) ){
      ret_val = 1;
      *run_value = (pbi->bit_pattern & 0x0003) + 6;
    }
    break;

  case 8:
    /* Bit 3 clear */
    if ( !(pbi->bit_pattern & 0x0008) ){
      ret_val = 1;
      *run_value = (pbi->bit_pattern & 0x0007) + 10;
    }
    break;

  case 10:
    /* Bit 4 clear */
    if ( !(pbi->bit_pattern & 0x0010) ){
      ret_val = 1;
      *run_value = (pbi->bit_pattern & 0x000F) + 18;
    }
    break;

  case 18:
    ret_val = 1;
    *run_value = (pbi->bit_pattern & 0x0FFF) + 34;
    break;

  default:
    ret_val = 0;
    break;
  }

  return ret_val;
}

static void GetNextBInit(PB_INSTANCE *pbi){
  long ret;

  theora_read(pbi->opb,1,&ret);
  pbi->NextBit = (unsigned char)ret;

  /* Read run length */
  FrArrayDeCodeInit(pbi);
  do theora_read(pbi->opb,1,&ret);
  while (FrArrayDeCodeBlockRun(pbi,ret,&pbi->BitsLeft)==0);

}

static unsigned char GetNextBBit (PB_INSTANCE *pbi){
  long ret;
  if ( !pbi->BitsLeft ){
    /* Toggle the value.   */
    pbi->NextBit = ( pbi->NextBit == 1 ) ? 0 : 1;

    /* Read next run */
    FrArrayDeCodeInit(pbi);
    do theora_read(pbi->opb,1,&ret);
    while (FrArrayDeCodeBlockRun(pbi,ret,&pbi->BitsLeft)==0);

  }

  /* Have  read a bit */
  pbi->BitsLeft--;

  /* Return next bit value */
  return pbi->NextBit;
}

static void GetNextSbInit(PB_INSTANCE *pbi){
  long ret;

  theora_read(pbi->opb,1,&ret);
  pbi->NextBit = (unsigned char)ret;

  /* Read run length */
  FrArrayDeCodeInit(pbi);
  do theora_read(pbi->opb,1,&ret);
  while (FrArrayDeCodeSBRun(pbi,ret,&pbi->BitsLeft)==0);

}

static unsigned char GetNextSbBit (PB_INSTANCE *pbi){
  long ret;

  if ( !pbi->BitsLeft ){
    /* Toggle the value.   */
    pbi->NextBit = ( pbi->NextBit == 1 ) ? 0 : 1;

    /* Read next run */
    FrArrayDeCodeInit(pbi);
    do theora_read(pbi->opb,1,&ret);
    while (FrArrayDeCodeSBRun(pbi,ret,&pbi->BitsLeft)==0);

  }

  /* Have  read a bit */
  pbi->BitsLeft--;

  /* Return next bit value */
  return pbi->NextBit;
}

void QuadDecodeDisplayFragments ( PB_INSTANCE *pbi ){
  ogg_uint32_t  SB, MB, B;
  int    DataToDecode;

  ogg_int32_t   dfIndex;
  ogg_uint32_t  MBIndex = 0;

  /* Reset various data structures common to key frames and inter frames. */
  pbi->CodedBlockIndex = 0;
  memset ( pbi->display_fragments, 0, pbi->UnitFragments );

  /* For "Key frames" mark all blocks as coded and return. */
  /* Else initialise the ArrayPtr array to 0 (all blocks uncoded by default) */
  if ( GetFrameType(pbi) == BASE_FRAME ) {
    memset( pbi->SBFullyFlags, 1, pbi->SuperBlocks );
    memset( pbi->SBCodedFlags, 1, pbi->SuperBlocks );
        memset( pbi->MBCodedFlags, 0, pbi->MacroBlocks );
  }else{
    memset( pbi->SBFullyFlags, 0, pbi->SuperBlocks );
    memset( pbi->MBCodedFlags, 0, pbi->MacroBlocks );

    /* Un-pack the list of partially coded Super-Blocks */
    GetNextSbInit(pbi);
    for( SB = 0; SB < pbi->SuperBlocks; SB++){
      pbi->SBCodedFlags[SB] = GetNextSbBit (pbi);
    }

    /* Scan through the list of super blocks.  Unless all are marked
       as partially coded we have more to do. */
    DataToDecode = 0;
    for ( SB=0; SB<pbi->SuperBlocks; SB++ ) {
      if ( !pbi->SBCodedFlags[SB] ) {
        DataToDecode = 1;
        break;
      }
    }

    /* Are there further block map bits to decode ? */
    if ( DataToDecode ) {
      /* Un-pack the Super-Block fully coded flags. */
      GetNextSbInit(pbi);
      for( SB = 0; SB < pbi->SuperBlocks; SB++) {
        /* Skip blocks already marked as partially coded */
        while( (SB < pbi->SuperBlocks) && pbi->SBCodedFlags[SB] )
          SB++;

        if ( SB < pbi->SuperBlocks ) {
          pbi->SBFullyFlags[SB] = GetNextSbBit (pbi);

          if ( pbi->SBFullyFlags[SB] )       /* If SB is fully coded. */
            pbi->SBCodedFlags[SB] = 1;       /* Mark the SB as coded */
        }
      }
    }

    /* Scan through the list of coded super blocks.  If at least one
       is marked as partially coded then we have a block list to
       decode. */
    for ( SB=0; SB<pbi->SuperBlocks; SB++ ) {
      if ( pbi->SBCodedFlags[SB] && !pbi->SBFullyFlags[SB] ) {
        /* Initialise the block list decoder. */
        GetNextBInit(pbi);
        break;
      }
    }
  }

  /* Decode the block data from the bit stream. */
  for ( SB=0; SB<pbi->SuperBlocks; SB++ ){
    for ( MB=0; MB<4; MB++ ){
      /* If MB is in the frame */
      if ( QuadMapToMBTopLeft(pbi->BlockMap, SB,MB) >= 0 ){
        /* Only read block level data if SB was fully or partially coded */
        if ( pbi->SBCodedFlags[SB] ) {
          for ( B=0; B<4; B++ ){
            /* If block is valid (in frame)... */
            dfIndex = QuadMapToIndex1( pbi->BlockMap, SB, MB, B );
            if ( dfIndex >= 0 ){
              if ( pbi->SBFullyFlags[SB] )
                pbi->display_fragments[dfIndex] = 1;
              else
                pbi->display_fragments[dfIndex] = GetNextBBit(pbi);

              /* Create linear list of coded block indices */
              if ( pbi->display_fragments[dfIndex] ) {
                pbi->MBCodedFlags[MBIndex] = 1;
                pbi->CodedBlockList[pbi->CodedBlockIndex] = dfIndex;
                pbi->CodedBlockIndex++;
              }
            }
          }
        }
        MBIndex++;

      }
    }
  }
}

CODING_MODE FrArrayUnpackMode(PB_INSTANCE *pbi){
  long ret;
  /* Coding scheme:
     Token                      Codeword           Bits
     Entry   0 (most frequent)  0                   1
     Entry   1                  10                  2
     Entry   2                  110                 3
     Entry   3                  1110                4
     Entry   4                  11110               5
     Entry   5                  111110              6
     Entry   6                  1111110             7
     Entry   7                  1111111             7
  */

  /* Initialise the decoding. */
  pbi->bits_so_far = 0;

  theora_read(pbi->opb,1,&ret);
  pbi->bit_pattern = ret;

  /* Do we have a match */
  if ( pbi->bit_pattern == 0 )
    return (CODING_MODE)0;

  /* Get the next bit */
  theora_read(pbi->opb,1,&ret);
  pbi->bit_pattern = (pbi->bit_pattern << 1) | ret;

  /* Do we have a match */
  if ( pbi->bit_pattern == 0x0002 )
    return (CODING_MODE)1;

  theora_read(pbi->opb,1,&ret);
  pbi->bit_pattern = (pbi->bit_pattern << 1) | ret;

  /* Do we have a match  */
  if ( pbi->bit_pattern == 0x0006 )
    return (CODING_MODE)2;

  theora_read(pbi->opb,1,&ret);
  pbi->bit_pattern = (pbi->bit_pattern << 1) | ret;

  /* Do we have a match */
  if ( pbi->bit_pattern == 0x000E )
    return (CODING_MODE)3;

  theora_read(pbi->opb,1,&ret);
  pbi->bit_pattern = (pbi->bit_pattern << 1) | ret;

  /* Do we have a match */
  if ( pbi->bit_pattern == 0x001E )
    return (CODING_MODE)4;

  theora_read(pbi->opb,1,&ret);
  pbi->bit_pattern = (pbi->bit_pattern << 1) | ret;

  /* Do we have a match */
  if ( pbi->bit_pattern == 0x003E )
    return (CODING_MODE)5;

  theora_read(pbi->opb,1,&ret);
  pbi->bit_pattern = (pbi->bit_pattern << 1) | ret;

  /* Do we have a match */
  if ( pbi->bit_pattern == 0x007E )
    return (CODING_MODE)6;
  else
    return (CODING_MODE)7;
}

