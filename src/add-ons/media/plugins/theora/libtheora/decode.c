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
  last mod: $Id: decode.c,v 1.1 2004/02/24 13:50:13 shatty Exp $

 ********************************************************************/

#include <string.h>
#include "encoder_internal.h"
#include "block_inline.h"

static const CODING_MODE  ModeAlphabet[MODE_METHODS-2][MAX_MODES] = {

  /* Last motion vector dominates */
  {    CODE_INTER_LAST_MV,    CODE_INTER_PRIOR_LAST,
       CODE_INTER_PLUS_MV,    CODE_INTER_NO_MV,
       CODE_INTRA,            CODE_USING_GOLDEN,
       CODE_GOLDEN_MV,        CODE_INTER_FOURMV },

  {    CODE_INTER_LAST_MV,    CODE_INTER_PRIOR_LAST,
       CODE_INTER_NO_MV,      CODE_INTER_PLUS_MV,
       CODE_INTRA,            CODE_USING_GOLDEN,
       CODE_GOLDEN_MV,        CODE_INTER_FOURMV },

  {    CODE_INTER_LAST_MV,    CODE_INTER_PLUS_MV,
       CODE_INTER_PRIOR_LAST, CODE_INTER_NO_MV,
       CODE_INTRA,            CODE_USING_GOLDEN,
       CODE_GOLDEN_MV,        CODE_INTER_FOURMV },

  {    CODE_INTER_LAST_MV,    CODE_INTER_PLUS_MV,
       CODE_INTER_NO_MV,      CODE_INTER_PRIOR_LAST,
       CODE_INTRA,            CODE_USING_GOLDEN,
       CODE_GOLDEN_MV,        CODE_INTER_FOURMV },

  /* No motion vector dominates */
  {    CODE_INTER_NO_MV,      CODE_INTER_LAST_MV,
       CODE_INTER_PRIOR_LAST, CODE_INTER_PLUS_MV,
       CODE_INTRA,            CODE_USING_GOLDEN,
       CODE_GOLDEN_MV,        CODE_INTER_FOURMV },

  {    CODE_INTER_NO_MV,      CODE_USING_GOLDEN,
       CODE_INTER_LAST_MV,    CODE_INTER_PRIOR_LAST,
       CODE_INTER_PLUS_MV,    CODE_INTRA,
       CODE_GOLDEN_MV,        CODE_INTER_FOURMV },

};

int GetFrameType(PB_INSTANCE *pbi){
  return pbi->FrameType;
}

static int LoadFrameHeader(PB_INSTANCE *pbi){
  long ret;
  unsigned char  DctQMask;
  unsigned char  SpareBits;       /* Spare cfg bits */

  /* Is the frame and inter frame or a key frame */
  theora_read(pbi->opb,1,&ret);
  pbi->FrameType = (unsigned char)ret;

  /* Quality (Q) index */
  theora_read(pbi->opb,6,&ret);
  DctQMask = (unsigned char)ret;

  /* spare bit for possible additional Q indicies - should be 0 */
  theora_read(pbi->opb,1,&ret);
  SpareBits = (unsigned char)ret;

  if ( (pbi->FrameType == BASE_FRAME) ){
    /* Read the type / coding method for the key frame. */
    theora_read(pbi->opb,1,&ret);
    pbi->KeyFrameType = (unsigned char)ret;

    theora_read(pbi->opb,2,&ret);
    SpareBits = (unsigned char)ret;

  }

  /* Set this frame quality value from Q Index */
  pbi->ThisFrameQualityValue = pbi->QThreshTable[DctQMask];

  return 1;
}

void SetFrameType( PB_INSTANCE *pbi,unsigned char FrType ){
  /* Set the appropriate frame type according to the request. */
  switch ( FrType ){

  case BASE_FRAME:
    pbi->FrameType = FrType;
    break;

  default:
    pbi->FrameType = FrType;
    break;
  }
}

static int LoadFrame(PB_INSTANCE *pbi){

  /* Load the frame header (including the frame size). */
  if ( LoadFrameHeader(pbi) ){
    /* Read in the updated block map */
    QuadDecodeDisplayFragments( pbi );
    return 1;
  }

  return 0;
}

static void DecodeModes (PB_INSTANCE *pbi,
                         ogg_uint32_t SBRows,
                         ogg_uint32_t SBCols){
  long ret;
  ogg_int32_t   FragIndex;
  ogg_uint32_t  MB;
  ogg_uint32_t  SBrow;
  ogg_uint32_t  SBcol;
  ogg_uint32_t  SB=0;
  CODING_MODE  CodingMethod;

  ogg_uint32_t  UVRow;
  ogg_uint32_t  UVColumn;
  ogg_uint32_t  UVFragOffset;

  ogg_uint32_t  CodingScheme;

  ogg_uint32_t  MBListIndex = 0;

  ogg_uint32_t  i;

  /* If the frame is an intra frame then all blocks have mode intra. */
  if ( GetFrameType(pbi) == BASE_FRAME ){
    for ( i = 0; i < pbi->UnitFragments; i++ ){
      pbi->FragCodingMethod[i] = CODE_INTRA;
    }
  }else{
    ogg_uint32_t        ModeEntry; /* Mode bits read */
    CODING_MODE         CustomModeAlphabet[MAX_MODES];
    const CODING_MODE  *ModeList;

    /* Read the coding method */
    theora_read(pbi->opb, MODE_METHOD_BITS, &ret);
    CodingScheme=ret;

    /* If the coding method is method 0 then we have to read in a
       custom coding scheme */
    if ( CodingScheme == 0 ){
      /* Read the coding scheme. */
      for ( i = 0; i < MAX_MODES; i++ ){
        theora_read(pbi->opb, MODE_BITS, &ret);
        CustomModeAlphabet[ret]=i;
      }
      ModeList=CustomModeAlphabet;
    }
    else{
      ModeList=ModeAlphabet[CodingScheme-1];
    }

    /* Unravel the quad-tree */
    for ( SBrow=0; SBrow<SBRows; SBrow++ ){
      for ( SBcol=0; SBcol<SBCols; SBcol++ ){
        for ( MB=0; MB<4; MB++ ){
          /* There may be MB's lying out of frame which must be
             ignored. For these MB's top left block will have a negative
             Fragment Index. */
          if ( QuadMapToMBTopLeft(pbi->BlockMap, SB,MB) >= 0){
            /* Is the Macro-Block coded: */
            if ( pbi->MBCodedFlags[MBListIndex++] ){
              /* Upack the block level modes and motion vectors */
              FragIndex = QuadMapToMBTopLeft( pbi->BlockMap, SB, MB );

              /* Unpack the mode. */
              if ( CodingScheme == (MODE_METHODS-1) ){
                /* This is the fall back coding scheme. */
                /* Simply MODE_BITS bits per mode entry. */
                theora_read(pbi->opb, MODE_BITS, &ret);
                CodingMethod = (CODING_MODE)ret;
              }else{
                ModeEntry = FrArrayUnpackMode(pbi);
                CodingMethod = ModeList[ModeEntry];
              }

              /* Note the coding mode for each block in macro block. */
              pbi->FragCodingMethod[FragIndex] = CodingMethod;
              pbi->FragCodingMethod[FragIndex + 1] = CodingMethod;
              pbi->FragCodingMethod[FragIndex + pbi->HFragments] =
                CodingMethod;
              pbi->FragCodingMethod[FragIndex + pbi->HFragments + 1] =
                CodingMethod;

              /* Matching fragments in the U and V planes */
              UVRow = (FragIndex / (pbi->HFragments * 2));
              UVColumn = (FragIndex % pbi->HFragments) / 2;
              UVFragOffset = (UVRow * (pbi->HFragments / 2)) + UVColumn;
              pbi->FragCodingMethod[pbi->YPlaneFragments + UVFragOffset] =
                CodingMethod;
              pbi->FragCodingMethod[pbi->YPlaneFragments +
                                   pbi->UVPlaneFragments + UVFragOffset] =
                CodingMethod;

            }
          }
        }

        /* Next Super-Block */
        SB++;
      }
    }
  }
}

static ogg_int32_t ExtractMVectorComponentA(PB_INSTANCE *pbi){
  long ret;
  ogg_int32_t   MVectComponent;
  ogg_uint32_t  MVCode = 0;
  ogg_uint32_t  ExtraBits = 0;

  /* Get group to which coded component belongs */
  theora_read(pbi->opb, 3, &ret);
  MVCode=ret;

  /*  Now extract the appropriate number of bits to identify the component */
  switch ( MVCode ){
  case 0:
    MVectComponent = 0;
    break;
  case 1:
    MVectComponent = 1;
    break;
  case 2:
    MVectComponent = -1;
    break;
  case 3:
    theora_read(pbi->opb,1,&ret);
    if (ret)
      MVectComponent = -2;
    else
      MVectComponent = 2;
    break;
  case 4:
    theora_read(pbi->opb,1,&ret);
    if (ret)
      MVectComponent = -3;
    else
      MVectComponent = 3;
    break;
  case 5:
    theora_read(pbi->opb,2,&ret);
    ExtraBits=ret;
    MVectComponent = 4 + ExtraBits;
    theora_read(pbi->opb,1,&ret);
    if (ret)
      MVectComponent = -MVectComponent;
    break;
  case 6:
    theora_read(pbi->opb,3,&ret);
    ExtraBits=ret;
    MVectComponent = 8 + ExtraBits;
    theora_read(pbi->opb,1,&ret);
    if (ret)
      MVectComponent = -MVectComponent;
    break;
  case 7:
    theora_read(pbi->opb,4,&ret);
    ExtraBits=ret;
    MVectComponent = 16 + ExtraBits;
    theora_read(pbi->opb,1,&ret);
    if (ret)
      MVectComponent = -MVectComponent;
    break;
  }

  return MVectComponent;
}

static ogg_int32_t ExtractMVectorComponentB(PB_INSTANCE *pbi){
  long ret;
  ogg_int32_t   MVectComponent;

  /* Get group to which coded component belongs */
  theora_read(pbi->opb,5,&ret);
  MVectComponent=ret;
  theora_read(pbi->opb,1,&ret);
  if (ret)
    MVectComponent = -MVectComponent;

  return MVectComponent;
}

static void DecodeMVectors ( PB_INSTANCE *pbi,
                             ogg_uint32_t SBRows,
                             ogg_uint32_t SBCols ){
  long ret;
  ogg_int32_t   FragIndex;
  ogg_uint32_t  MB;
  ogg_uint32_t  SBrow;
  ogg_uint32_t  SBcol;
  ogg_uint32_t  SB=0;
  ogg_uint32_t  CodingMethod;

  MOTION_VECTOR MVect[6];
  MOTION_VECTOR TmpMVect;
  MOTION_VECTOR LastInterMV;
  MOTION_VECTOR PriorLastInterMV;
  ogg_int32_t (*ExtractMVectorComponent)(PB_INSTANCE *pbi);

  ogg_uint32_t  UVRow;
  ogg_uint32_t  UVColumn;
  ogg_uint32_t  UVFragOffset;

  ogg_uint32_t  MBListIndex = 0;

  /* Should not be decoding motion vectors if in INTRA only mode. */
  if ( GetFrameType(pbi) == BASE_FRAME ){
    return;
  }

  /* set the default motion vector to 0,0 */
  MVect[0].x = 0;
  MVect[0].y = 0;
  LastInterMV.x = 0;
  LastInterMV.y = 0;
  PriorLastInterMV.x = 0;
  PriorLastInterMV.y = 0;

  /* Read the entropy method used and set up the appropriate decode option */
  theora_read(pbi->opb, 1, &ret);  
  if ( ret == 0 )
    ExtractMVectorComponent = ExtractMVectorComponentA;
  else
    ExtractMVectorComponent = ExtractMVectorComponentB;

  /* Unravel the quad-tree */
  for ( SBrow=0; SBrow<SBRows; SBrow++ ){

    for ( SBcol=0; SBcol<SBCols; SBcol++ ){
      for ( MB=0; MB<4; MB++ ){
        /* There may be MB's lying out of frame which must be
           ignored. For these MB's the top left block will have a
           negative Fragment. */
        if ( QuadMapToMBTopLeft(pbi->BlockMap, SB,MB) >= 0 ) {
          /* Is the Macro-Block further coded: */
          if ( pbi->MBCodedFlags[MBListIndex++] ){
            /* Upack the block level modes and motion vectors */
            FragIndex = QuadMapToMBTopLeft( pbi->BlockMap, SB, MB );

            /* Clear the motion vector before we start. */
            MVect[0].x = 0;
            MVect[0].y = 0;

            /* Unpack the mode (and motion vectors if necessary). */
            CodingMethod = pbi->FragCodingMethod[FragIndex];

            /* Read the motion vector or vectors if present. */
            if ( (CodingMethod == CODE_INTER_PLUS_MV) ||
                 (CodingMethod == CODE_GOLDEN_MV) ){
              MVect[0].x = ExtractMVectorComponent(pbi);
              MVect[1].x = MVect[0].x;
              MVect[2].x = MVect[0].x;
              MVect[3].x = MVect[0].x;
              MVect[4].x = MVect[0].x;
              MVect[5].x = MVect[0].x;
              MVect[0].y = ExtractMVectorComponent(pbi);
              MVect[1].y = MVect[0].y;
              MVect[2].y = MVect[0].y;
              MVect[3].y = MVect[0].y;
              MVect[4].y = MVect[0].y;
              MVect[5].y = MVect[0].y;
            }else if ( CodingMethod == CODE_INTER_FOURMV ){
              /* Extrac the 4 Y MVs */
              MVect[0].x = ExtractMVectorComponent(pbi);
              MVect[0].y = ExtractMVectorComponent(pbi);

              MVect[1].x = ExtractMVectorComponent(pbi);
              MVect[1].y = ExtractMVectorComponent(pbi);

              MVect[2].x = ExtractMVectorComponent(pbi);
              MVect[2].y = ExtractMVectorComponent(pbi);

              MVect[3].x = ExtractMVectorComponent(pbi);
              MVect[3].y = ExtractMVectorComponent(pbi);

              /* Calculate the U and V plane MVs as the average of the
                 Y plane MVs. */
              /* First .x component */
              MVect[4].x = MVect[0].x + MVect[1].x + MVect[2].x + MVect[3].x;
              if ( MVect[4].x >= 0 )
                MVect[4].x = (MVect[4].x + 2) / 4;
              else
                MVect[4].x = (MVect[4].x - 2) / 4;
              MVect[5].x = MVect[4].x;
              /* Then .y component */
              MVect[4].y = MVect[0].y + MVect[1].y + MVect[2].y + MVect[3].y;
              if ( MVect[4].y >= 0 )
                MVect[4].y = (MVect[4].y + 2) / 4;
              else
                MVect[4].y = (MVect[4].y - 2) / 4;
              MVect[5].y = MVect[4].y;
            }

            /* Keep track of last and prior last inter motion vectors. */
            if ( CodingMethod == CODE_INTER_PLUS_MV ){
              PriorLastInterMV.x = LastInterMV.x;
              PriorLastInterMV.y = LastInterMV.y;
              LastInterMV.x = MVect[0].x;
              LastInterMV.y = MVect[0].y;
            }else if ( CodingMethod == CODE_INTER_LAST_MV ){
              /* Use the last coded Inter motion vector. */
              MVect[0].x = LastInterMV.x;
              MVect[1].x = MVect[0].x;
              MVect[2].x = MVect[0].x;
              MVect[3].x = MVect[0].x;
              MVect[4].x = MVect[0].x;
              MVect[5].x = MVect[0].x;
              MVect[0].y = LastInterMV.y;
              MVect[1].y = MVect[0].y;
              MVect[2].y = MVect[0].y;
              MVect[3].y = MVect[0].y;
              MVect[4].y = MVect[0].y;
              MVect[5].y = MVect[0].y;
            }else if ( CodingMethod == CODE_INTER_PRIOR_LAST ){
              /* Use the next-to-last coded Inter motion vector. */
              MVect[0].x = PriorLastInterMV.x;
              MVect[1].x = MVect[0].x;
              MVect[2].x = MVect[0].x;
              MVect[3].x = MVect[0].x;
              MVect[4].x = MVect[0].x;
              MVect[5].x = MVect[0].x;
              MVect[0].y = PriorLastInterMV.y;
              MVect[1].y = MVect[0].y;
              MVect[2].y = MVect[0].y;
              MVect[3].y = MVect[0].y;
              MVect[4].y = MVect[0].y;
              MVect[5].y = MVect[0].y;

              /* Swap the prior and last MV cases over */
              TmpMVect.x = PriorLastInterMV.x;
              TmpMVect.y = PriorLastInterMV.y;
              PriorLastInterMV.x = LastInterMV.x;
              PriorLastInterMV.y = LastInterMV.y;
              LastInterMV.x = TmpMVect.x;
              LastInterMV.y = TmpMVect.y;
            }else if ( CodingMethod == CODE_INTER_FOURMV ){
              /* Update last MV and prior last mv */
              PriorLastInterMV.x = LastInterMV.x;
              PriorLastInterMV.y = LastInterMV.y;
              LastInterMV.x = MVect[3].x;
              LastInterMV.y = MVect[3].y;
            }

            /* Note the coding mode and vector for each block in the
               current macro block. */
            pbi->FragMVect[FragIndex].x = MVect[0].x;
            pbi->FragMVect[FragIndex].y = MVect[0].y;

            pbi->FragMVect[FragIndex + 1].x = MVect[1].x;
            pbi->FragMVect[FragIndex + 1].y = MVect[1].y;

            pbi->FragMVect[FragIndex + pbi->HFragments].x = MVect[2].x;
            pbi->FragMVect[FragIndex + pbi->HFragments].y = MVect[2].y;

            pbi->FragMVect[FragIndex + pbi->HFragments + 1].x = MVect[3].x;
            pbi->FragMVect[FragIndex + pbi->HFragments + 1].y = MVect[3].y;

            /* Matching fragments in the U and V planes */
            UVRow = (FragIndex / (pbi->HFragments * 2));
            UVColumn = (FragIndex % pbi->HFragments) / 2;
            UVFragOffset = (UVRow * (pbi->HFragments / 2)) + UVColumn;

            pbi->FragMVect[pbi->YPlaneFragments + UVFragOffset].x = MVect[4].x;
            pbi->FragMVect[pbi->YPlaneFragments + UVFragOffset].y = MVect[4].y;

            pbi->FragMVect[pbi->YPlaneFragments + pbi->UVPlaneFragments +
                          UVFragOffset].x = MVect[5].x;
            pbi->FragMVect[pbi->YPlaneFragments + pbi->UVPlaneFragments +
                          UVFragOffset].y = MVect[5].y;
          }
        }
      }

      /* Next Super-Block */
      SB++;
    }
  }
}

static ogg_uint32_t ExtractToken(oggpack_buffer *opb,
                                 HUFF_ENTRY * CurrentRoot){
  long ret;
  ogg_uint32_t Token;
  /* Loop searches down through tree based upon bits read from the
     bitstream */
  /* until it hits a leaf at which point we have decoded a token */
  while ( CurrentRoot->Value < 0 ){

    theora_read(opb, 1, &ret);  
    if (ret)
      CurrentRoot = CurrentRoot->OneChild;
    else
      CurrentRoot = CurrentRoot->ZeroChild;

  }
  Token = CurrentRoot->Value;
  return Token;
}

static void UnpackAndExpandDcToken( PB_INSTANCE *pbi,
                                    Q_LIST_ENTRY *ExpandedBlock,
                                    unsigned char * CoeffIndex ){
  long			ret;
  ogg_int32_t           ExtraBits = 0;
  ogg_uint32_t          Token;

  Token = ExtractToken(pbi->opb, pbi->HuffRoot_VP3x[pbi->DcHuffChoice]);


  /* Now.. if we are using the DCT optimised coding system, extract any
   *  assosciated additional bits token.
   */
  if ( pbi->ExtraBitLengths_VP3x[Token] > 0 ){
    /* Extract the appropriate number of extra bits. */
    theora_read(pbi->opb,pbi->ExtraBitLengths_VP3x[Token], &ret);
    ExtraBits = ret;
  }

  /* Take token dependant action */
  if ( Token >= DCT_SHORT_ZRL_TOKEN ) {
    /* "Value", "zero run" and "zero run value" tokens */
    ExpandToken(ExpandedBlock, CoeffIndex, Token, ExtraBits );
    if ( *CoeffIndex >= BLOCK_SIZE )
      pbi->BlocksToDecode --;
  } else if ( Token == DCT_EOB_TOKEN ){
    *CoeffIndex = BLOCK_SIZE;
    pbi->BlocksToDecode --;
  }else{
    /* Special action and EOB tokens */
    switch ( Token ){
    case DCT_EOB_PAIR_TOKEN:
      pbi->EOB_Run = 1;
      *CoeffIndex = BLOCK_SIZE;
      pbi->BlocksToDecode --;
      break;
    case DCT_EOB_TRIPLE_TOKEN:
      pbi->EOB_Run = 2;
      *CoeffIndex = BLOCK_SIZE;
      pbi->BlocksToDecode --;
      break;
    case DCT_REPEAT_RUN_TOKEN:
      pbi->EOB_Run = ExtraBits + 3;
      *CoeffIndex = BLOCK_SIZE;
      pbi->BlocksToDecode --;
      break;
    case DCT_REPEAT_RUN2_TOKEN:
      pbi->EOB_Run = ExtraBits + 7;
      *CoeffIndex = BLOCK_SIZE;
      pbi->BlocksToDecode --;
      break;
    case DCT_REPEAT_RUN3_TOKEN:
      pbi->EOB_Run = ExtraBits + 15;
      *CoeffIndex = BLOCK_SIZE;
      pbi->BlocksToDecode --;
      break;
    case DCT_REPEAT_RUN4_TOKEN:
      pbi->EOB_Run = ExtraBits - 1;
      *CoeffIndex = BLOCK_SIZE;
      pbi->BlocksToDecode --;
      break;
    }
  }
}

static void UnpackAndExpandAcToken( PB_INSTANCE *pbi,
                                    Q_LIST_ENTRY * ExpandedBlock,
                                    unsigned char * CoeffIndex  ) {
  long                  ret;
  ogg_int32_t           ExtraBits = 0;
  ogg_uint32_t          Token;

  Token = ExtractToken(pbi->opb, pbi->HuffRoot_VP3x[pbi->ACHuffChoice]);

  /* Now.. if we are using the DCT optimised coding system, extract any
   *  assosciated additional bits token.
   */
  if ( pbi->ExtraBitLengths_VP3x[Token] > 0 ){
    /* Extract the appropriate number of extra bits. */
    theora_read(pbi->opb,pbi->ExtraBitLengths_VP3x[Token], &ret);
    ExtraBits = ret;
  }

  /* Take token dependant action */
  if ( Token >= DCT_SHORT_ZRL_TOKEN ){
    /* "Value", "zero run" and "zero run value" tokens */
    ExpandToken(ExpandedBlock, CoeffIndex, Token, ExtraBits );
    if ( *CoeffIndex >= BLOCK_SIZE )
      pbi->BlocksToDecode --;
  } else if ( Token == DCT_EOB_TOKEN ) {
    *CoeffIndex = BLOCK_SIZE;
    pbi->BlocksToDecode --;
  } else {
    /* Special action and EOB tokens */
    switch ( Token ) {
    case DCT_EOB_PAIR_TOKEN:
      pbi->EOB_Run = 1;
      *CoeffIndex = BLOCK_SIZE;
      pbi->BlocksToDecode --;
      break;
    case DCT_EOB_TRIPLE_TOKEN:
      pbi->EOB_Run = 2;
      *CoeffIndex = BLOCK_SIZE;
      pbi->BlocksToDecode --;
      break;
    case DCT_REPEAT_RUN_TOKEN:
      pbi->EOB_Run = ExtraBits + 3;
      *CoeffIndex = BLOCK_SIZE;
      pbi->BlocksToDecode --;
      break;
    case DCT_REPEAT_RUN2_TOKEN:
      pbi->EOB_Run = ExtraBits + 7;
      *CoeffIndex = BLOCK_SIZE;
      pbi->BlocksToDecode --;
      break;
    case DCT_REPEAT_RUN3_TOKEN:
      pbi->EOB_Run = ExtraBits + 15;
      *CoeffIndex = BLOCK_SIZE;
      pbi->BlocksToDecode --;
      break;
    case DCT_REPEAT_RUN4_TOKEN:
      pbi->EOB_Run = ExtraBits - 1;
      *CoeffIndex = BLOCK_SIZE;
      pbi->BlocksToDecode --;
      break;
    }
  }
}

static void UnPackVideo (PB_INSTANCE *pbi){
  long ret;
  ogg_int32_t       EncodedCoeffs = 1;
  ogg_int32_t       FragIndex;
  ogg_int32_t *     CodedBlockListPtr;
  ogg_int32_t *     CodedBlockListEnd;

  unsigned char     AcHuffIndex1;
  unsigned char     AcHuffIndex2;
  unsigned char     AcHuffChoice1;
  unsigned char     AcHuffChoice2;

  unsigned char     DcHuffChoice1;
  unsigned char     DcHuffChoice2;


  /* Bail out immediately if a decode error has already been reported. */
  if ( pbi->DecoderErrorCode ) return;

  /* Clear down the array that indicates the current coefficient index
     for each block. */
  memset(pbi->FragCoeffs, 0, pbi->UnitFragments);
  memset(pbi->FragCoefEOB, 0, pbi->UnitFragments);

  /* Clear down the pbi->QFragData structure for all coded blocks. */
  ClearDownQFragData(pbi);

  /* Note the number of blocks to decode */
  pbi->BlocksToDecode = pbi->CodedBlockIndex;

  /* Get the DC huffman table choice for Y and then UV */
  theora_read(pbi->opb,DC_HUFF_CHOICE_BITS,&ret); 
  DcHuffChoice1 = ret + DC_HUFF_OFFSET;
  theora_read(pbi->opb,DC_HUFF_CHOICE_BITS,&ret); 
  DcHuffChoice2 = ret + DC_HUFF_OFFSET;

  /* UnPack DC coefficients / tokens */
  CodedBlockListPtr = pbi->CodedBlockList;
  CodedBlockListEnd = &pbi->CodedBlockList[pbi->CodedBlockIndex];
  while ( CodedBlockListPtr < CodedBlockListEnd  ) {
    /* Get the block data index */
    FragIndex = *CodedBlockListPtr;
    pbi->FragCoefEOB[FragIndex] = pbi->FragCoeffs[FragIndex];

    /* Select the appropriate huffman table offset according to
       whether the token is from a Y or UV block */
    if ( FragIndex < (ogg_int32_t)pbi->YPlaneFragments )
      pbi->DcHuffChoice = DcHuffChoice1;
    else
      pbi->DcHuffChoice = DcHuffChoice2;

    /* If we are in the middle of an EOB run */
    if ( pbi->EOB_Run ){
      /* Mark the current block as fully expanded and decrement
         EOB_RUN count */
      pbi->FragCoeffs[FragIndex] = BLOCK_SIZE;
      pbi->EOB_Run --;
      pbi->BlocksToDecode --;
    }else{
      /* Else unpack a DC token */
      UnpackAndExpandDcToken( pbi,
                              pbi->QFragData[FragIndex],
                              &pbi->FragCoeffs[FragIndex] );
    }
    CodedBlockListPtr++;
  }

  /* Get the AC huffman table choice for Y and then for UV. */

  theora_read(pbi->opb,AC_HUFF_CHOICE_BITS,&ret); 
  AcHuffIndex1 = ret + AC_HUFF_OFFSET;
  theora_read(pbi->opb,AC_HUFF_CHOICE_BITS,&ret); 
  AcHuffIndex2 = ret + AC_HUFF_OFFSET;

  /* Unpack Lower AC coefficients. */
  while ( EncodedCoeffs < 64 ) {
    /* Repeatedly scan through the list of blocks. */
    CodedBlockListPtr = pbi->CodedBlockList;
    CodedBlockListEnd = &pbi->CodedBlockList[pbi->CodedBlockIndex];

    /* Huffman table selection based upon which AC coefficient we are on */
    if ( EncodedCoeffs <= AC_TABLE_2_THRESH ){
      AcHuffChoice1 = AcHuffIndex1;
      AcHuffChoice2 = AcHuffIndex2;
    }else if ( EncodedCoeffs <= AC_TABLE_3_THRESH ){
      AcHuffChoice1 = AcHuffIndex1 + AC_HUFF_CHOICES;
      AcHuffChoice2 = AcHuffIndex2 + AC_HUFF_CHOICES;
    } else if ( EncodedCoeffs <= AC_TABLE_4_THRESH ){
      AcHuffChoice1 = AcHuffIndex1 + (AC_HUFF_CHOICES * 2);
      AcHuffChoice2 = AcHuffIndex2 + (AC_HUFF_CHOICES * 2);
    } else {
      AcHuffChoice1 = AcHuffIndex1 + (AC_HUFF_CHOICES * 3);
      AcHuffChoice2 = AcHuffIndex2 + (AC_HUFF_CHOICES * 3);
    }

    while( CodedBlockListPtr < CodedBlockListEnd ) {
      /* Get the linear index for the current fragment. */
      FragIndex = *CodedBlockListPtr;

      /* Should we decode a token for this block on this pass. */
      if ( pbi->FragCoeffs[FragIndex] <= EncodedCoeffs ) {
        pbi->FragCoefEOB[FragIndex] = pbi->FragCoeffs[FragIndex];
        /* If we are in the middle of an EOB run */
        if ( pbi->EOB_Run ) {
          /* Mark the current block as fully expanded and decrement
             EOB_RUN count */
          pbi->FragCoeffs[FragIndex] = BLOCK_SIZE;
          pbi->EOB_Run --;
          pbi->BlocksToDecode --;
        }else{
          /* Else unpack an AC token */
          /* Work out which huffman table to use, then decode a token */
          if ( FragIndex < (ogg_int32_t)pbi->YPlaneFragments )
            pbi->ACHuffChoice = AcHuffChoice1;
          else
            pbi->ACHuffChoice = AcHuffChoice2;

          UnpackAndExpandAcToken( pbi, pbi->QFragData[FragIndex],
                                  &pbi->FragCoeffs[FragIndex] );
        }
      }
      CodedBlockListPtr++;
    }

    /* Test for condition where there are no blocks left with any
       tokesn to decode */
    if ( !pbi->BlocksToDecode )
      break;

    EncodedCoeffs ++;
  }
}

static void DecodeData(PB_INSTANCE *pbi){
  ogg_uint32_t i;

  /* Bail out immediately if a decode error has already been reported. */
  if ( pbi->DecoderErrorCode ) return;

  /* Clear down the macro block level mode and MV arrays. */
  for ( i = 0; i < pbi->UnitFragments; i++ ){
    pbi->FragCodingMethod[i] = CODE_INTER_NO_MV; /* Default coding mode */
    pbi->FragMVect[i].x = 0;
    pbi->FragMVect[i].y = 0;
  }

  /* Zero Decoder EOB run count */
  pbi->EOB_Run = 0;

  /* Make a not of the number of coded blocks this frame */
  pbi->CodedBlocksThisFrame = pbi->CodedBlockIndex;

  /* Decode the modes data */
  DecodeModes( pbi, pbi->YSBRows, pbi->YSBCols);

  /* Unpack and decode the motion vectors. */
  DecodeMVectors ( pbi, pbi->YSBRows, pbi->YSBCols);

  /* Unpack and decode the actual video data. */
  UnPackVideo(pbi);

  /* Reconstruct and display the frame */
  ReconRefFrames(pbi);

}


int LoadAndDecode(PB_INSTANCE *pbi){
  int    LoadFrameOK;

  /* Reset the DC predictors. */
  pbi->InvLastIntraDC = 0;
  pbi->InvLastInterDC = 0;

  /* Load the next frame. */
  LoadFrameOK = LoadFrame(pbi);

  if ( LoadFrameOK ){
    if ( (pbi->ThisFrameQualityValue != pbi->LastFrameQualityValue) ){
      /* Initialise DCT tables. */
      UpdateQ( pbi, pbi->ThisFrameQualityValue );
      pbi->LastFrameQualityValue = pbi->ThisFrameQualityValue;
    }


    /* Decode the data into the fragment buffer. */
    DecodeData(pbi);
    return(0);
  }

  return(OC_BADPACKET);
}
