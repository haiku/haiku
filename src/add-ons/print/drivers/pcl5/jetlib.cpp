 /**************************************************************************
 *
 *   (c) Copyright Hewlett-Packard Company, 1994, 1995.  All rights are
 *   reserved.  Copying or other reproduction of this program except
 *   for archival purposes is prohibited without the prior written
 *   consent of Hewlett-Packard Company.
 *
 *                   RESTRICTED RIGHTS LEGEND
 *
 *   Use, duplication, or disclosure by the Government is subject to
 *   restrictions as set forth in paragraph (b) (3) (B) of the Rights
 *   in Technical Data and Computer Software clause in DAR 7-104.9(a).
 *
 *   HEWLETT-PACKARD COMPANY
 *   Boise, Idaho, USA
 ***************************************************************************/

/*
******************************* NOTICE ***************************************

HEWLETT-PACKARD COMPANY MAKES NO WARRANTIES, EXPRESS OR IMPLIED,  
INCLUDING WITHOUT LIMITATION THE IMPLIED WARRANTIES OF 
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, REGARDING THE
SOFTWARE OR TECHNICAL INFORMATION.   HEWLETT-PACKARD COMPANY DOES
NOT WARRANT, GUARANTEE OR MAKE ANY REPRESENTATIONS REGARDING THE 
USE OR THE RESULTS OF THE USE OF THE SOFTWARE OR TECHNICAL 
INFORMATION IN TERMS OF ITS CORRECTNESS, ACCURACY, RELIABILITY, 
CURRENTNESS, OR OTHERWISE.   THE ENTIRE RISK AS TO THE RESULTS AND 
PERFORMANCE OF THE SOFTWARE OR TECHNICAL INFORMATION IS ASSUMED
BY YOU.  The exclusion of implied warranties is not permitted by some
jurisdictions.  The above exclusion may not apply to you.

IN NO EVENT WILL HEWLETT-PACKARD COMPANY BE LIABLE TO YOU FOR ANY 
CONSEQUENTIAL, INCIDENTAL OR INDIRECT DAMAGES (INCLUDING DAMAGES 
FOR LOSS OF BUSINESS PROFITS, BUSINESS INTERRUPTION, LOSS OF BUSINESS 
INFORMATION AND THE LIKE)  ARISING OUT OF THE USE OR INABILITY TO USE 
THE SOFTWARE OR TECHNICAL INFORMATION EVEN IF HEWLETT-PACKARD HAS 
BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.  Because some jurisdictions 
do not allow the exclusion or limitation of liability for consequential
or incidental damages, the above limitations may not apply to you.  
Hewlett-Packard liability to you for actual damages from any cause whatsoever,
 and regardless of the form of the action (whether in contract, tort 
including negligence, product liability or otherwise), will be limited 
to US $50.

Copyright (c) 1995 Hewlett-Packard Company.  All rights reserved.

******************************************************************************
*/


static char *LibRev = "PCL XL Stream Creator Library.  Revision p$Revision: 1.1 $";
static char *LibCopyright = "(c) Copyright Hewlett-Packard Company, 1995, 1996, 1997.  All rights are reserved.";

/* 
 *  file: jetlib.c
 *
 *  PCL XL Stream Creator Library 
 *                                               
 */
#include <math.h>
#ifndef HP_WindowsAPI
#include <stdlib.h> /* for malloc */
#endif

#include "jetlib.h"  /* library header file */
#include "stdarg.h"

#include "string.h" /* standard C string routines */

#include "stdio.h" /* standard C I/O routines (used for sprintf) */

/* The following variables are not used by the Windows driver API: */



#define     HP_ByteLen          1   /* one byte needed */
#define     HP_Int16Len         2   /* two bytes needed */
#define     HP_Int32Len         4   /* four bytes needed */
#define     HP_Real32Len        4   /* four bytes needed */
#define     HP_XyMultiplier     2   /* x,y value length multiplier */
#define     HP_BoxMultiplier    4   /* box value multiplier */

#define HP_MaxNumberOfOps     256   /* number of operators */
#define HP_MaxNumberOfAttrIds 256   /* maximum number of attribute ids */

#define HP_Mask8(value) (HP_UByte) (255 & (value))

#ifdef HP_WindowsAPI
#include "jetlibcw.h"  /* Special macros used by the Windows driver */
#endif

#ifdef JETASM_BUILD
	#include "jetasmf.h"
#else
	#define HP_MEMALLOC(size) malloc(size)
	#define HP_MEMDEALLOC(ptr) if (ptr) free(ptr)
#endif

#ifndef HP_WindowsAPI

/* the following macros are for LITTLE_ENDIAN binary stream devices */

#ifndef OO_INTERFACE
#define HP_FlushFunction pFlushOutBuffer
#else
#define HP_FlushFunction pClient->FlushOutBuffer
#endif


/* following macro adds an 8-bit value to the attribute list */
#define HP_AddToListByte(pStream, value) \
	{\
		if (HP_OutBufferRemaining(pStream) < HP_ByteLen) \
		{\
			pStream->HP_FlushFunction(pStream, pStream->cookie, pStream->HP_OutBuffer, pStream->HP_CurrentBufferLen);\
			pStream->HP_CurrentBufferLen = 0;\
		} \
		if (HP_NoErrors(pStream)) \
		{\
			pStream->HP_OutBuffer[pStream->HP_CurrentBufferLen++]= \
				HP_Mask8(value);\
			pStream->HP_CurrentStreamLen++; \
		}\
	}

/* following macro adds a 16-bit value to the attribute list */
#define HP_AddToListInt16(pStream, value) \
	{\
		HP_UInt16 localVal = (HP_UInt16) value;\
        if (HP_OutBufferRemaining(pStream) < HP_Int16Len) \
        {\
            pStream->HP_FlushFunction(pStream, pStream->cookie, pStream->HP_OutBuffer, pStream->HP_CurrentBufferLen);\
            pStream->HP_CurrentBufferLen = 0;\
        }\
        if (HP_NoErrors(pStream)) \
        {\
            pStream->HP_OutBuffer[pStream->HP_CurrentBufferLen]  = \
                HP_Mask8(localVal);\
            pStream->HP_OutBuffer[pStream->HP_CurrentBufferLen+1]= \
				HP_Mask8(localVal=(localVal>>8));\
            pStream->HP_CurrentBufferLen+=HP_Int16Len;\
            pStream->HP_CurrentStreamLen+=HP_Int16Len;\
        }\
    }
    
/* following macro adds a 32-bit value to the attribute list */
#define HP_AddToListInt32(pStream, value) \
    {\
        HP_UInt32 localVal = value;\
        if (HP_OutBufferRemaining(pStream) < HP_Int32Len) \
        {\
            pStream->HP_FlushFunction(pStream, pStream->cookie, pStream->HP_OutBuffer, pStream->HP_CurrentBufferLen);\
			pStream->HP_CurrentBufferLen = 0;\
		}\
        if (HP_NoErrors(pStream)) \
        {\
			pStream->HP_OutBuffer[pStream->HP_CurrentBufferLen]  = \
                    HP_Mask8(localVal);\
            pStream->HP_OutBuffer[pStream->HP_CurrentBufferLen+1]= \
                    HP_Mask8(localVal=(localVal>>8));\
			pStream->HP_OutBuffer[pStream->HP_CurrentBufferLen+2]= \
                    HP_Mask8(localVal=(localVal>>8));\
            pStream->HP_OutBuffer[pStream->HP_CurrentBufferLen+3]= \
                    HP_Mask8(localVal>>8);\
            pStream->HP_CurrentBufferLen+=HP_Int32Len;\
            pStream->HP_CurrentStreamLen+=HP_Int32Len;\
        }\
    }
                             
/* the following adds a 32-bit real value to the attribute list */
#define HP_AddToListReal32(pStream, value) \
    {\
        HP_UInt32 localVal = (HP_UInt32) HP_Real32ToUInt32(value);\
        if (HP_OutBufferRemaining(pStream) < HP_Real32Len) \
        {\
            pStream->HP_FlushFunction(pStream, pStream->cookie, pStream->HP_OutBuffer, pStream->HP_CurrentBufferLen);\
            pStream->HP_CurrentBufferLen = 0;\
        }\
        if (HP_NoErrors(pStream)) \
        {\
            pStream->HP_OutBuffer[pStream->HP_CurrentBufferLen] \
					 = HP_Mask8(localVal);\
            pStream->HP_OutBuffer[pStream->HP_CurrentBufferLen+1]= \
                     HP_Mask8(localVal=(localVal>>8));\
            pStream->HP_OutBuffer[pStream->HP_CurrentBufferLen+2]= \
                     HP_Mask8(localVal=(localVal>>8));\
            pStream->HP_OutBuffer[pStream->HP_CurrentBufferLen+3]= \
                    HP_Mask8(localVal>>8);\
            pStream->HP_CurrentBufferLen+=HP_Real32Len;\
            pStream->HP_CurrentStreamLen+=HP_Real32Len;\
        }\
    }   

/* the following macros are for BIG_ENDIAN native machines */
/* (most significant byte first)                           */

/* following macro adds a 16-bit value to the attribute list */
#define HP_AddToListInt16MSB(pStream, value) \
    {\
        HP_UInt16 localVal = value;\
        if (HP_OutBufferRemaining(pStream) < HP_Int16Len) \
        {\
            pStream->HP_FlushFunction(pStream, pStream->cookie, pStream->HP_OutBuffer, pStream->HP_CurrentBufferLen);\
			pStream->HP_CurrentBufferLen = 0;\
		}\
        if (HP_NoErrors(pStream)) \
        {\
            pStream->HP_OutBuffer[pStream->HP_CurrentBufferLen+1]  = HP_Mask8(localVal);\
            pStream->HP_OutBuffer[pStream->HP_CurrentBufferLen]= HP_Mask8(localVal=(localVal>>8));\
            pStream->HP_CurrentBufferLen+=HP_Int16Len;\
            pStream->HP_CurrentStreamLen+=HP_Int16Len;\
        }\
    }
    
/* following macro adds a 32-bit value to the attribute list */
#define HP_AddToListInt32MSB(pStream, value) \
    {\
        HP_UInt32 localVal = value;\
        if (HP_OutBufferRemaining(pStream) < HP_Int32Len) \
        {\
            pStream->HP_FlushFunction(pStream, pStream->cookie, pStream->HP_OutBuffer, pStream->HP_CurrentBufferLen);\
            pStream->HP_CurrentBufferLen = 0;\
        }\
		if (HP_NoErrors(pStream)) \
		{\
            pStream->HP_OutBuffer[pStream->HP_CurrentBufferLen+3]  = HP_Mask8(localVal);\
			pStream->HP_OutBuffer[pStream->HP_CurrentBufferLen+2]= HP_Mask8(localVal=(localVal>>8));\
            pStream->HP_OutBuffer[pStream->HP_CurrentBufferLen+1]= HP_Mask8(localVal=(localVal>>8));\
            pStream->HP_OutBuffer[pStream->HP_CurrentBufferLen]= HP_Mask8(localVal>>8);\
            pStream->HP_CurrentBufferLen+=HP_Int32Len;\
            pStream->HP_CurrentStreamLen+=HP_Int32Len;\
        }\
    }
                             
/* the following adds a 32-bit real value to the attribute list */
#define HP_AddToListReal32MSB(pStream, value) \
    {\
        HP_UInt32 localVal = (HP_UInt32) HP_Real32ToUInt32(value);\
        if (HP_OutBufferRemaining(pStream) < HP_Real32Len) \
        {\
            pStream->HP_FlushFunction(pStream, pStream->cookie, pStream->HP_OutBuffer, pStream->HP_CurrentBufferLen);\
            pStream->HP_CurrentBufferLen = 0;\
        }\
        if (HP_NoErrors(pStream)) \
        {\
            pStream->HP_OutBuffer[pStream->HP_CurrentBufferLen+3]  = HP_Mask8(localVal);\
            pStream->HP_OutBuffer[pStream->HP_CurrentBufferLen+2]= HP_Mask8(localVal=(localVal>>8));\
            pStream->HP_OutBuffer[pStream->HP_CurrentBufferLen+1]= HP_Mask8(localVal=(localVal>>8));\
            pStream->HP_OutBuffer[pStream->HP_CurrentBufferLen]= HP_Mask8(localVal>>8);\
            pStream->HP_CurrentBufferLen+=HP_Real32Len;\
			pStream->HP_CurrentStreamLen+=HP_Real32Len;\
        }\
    }   

#endif /* for all translation macros */


#define HP_OpTagLen     0   /* byte length of operator value tag */
#define HP_AttrTagLen   1   /* byte length of attribute identifier tag */
#define HP_DataTagLen   1   /* byte length of data type tag */
  
const HP_UByte HP_OpValueMap[HP_MaxNumberOfOps]=
{
    /* 0x00 nul */  0x00,
    /* 0x01 soh */  0x00,
	/* 0x02 stx */  0x00,
    /* 0x03 etx */  0x00,
    /* 0x04 eot */  0x00,
    /* 0x05 enq */  0x00,
    /* 0x06 ack */  0x00,
    /* 0x07 bel */  0x00,
    /* 0x08 bs  */  0x00,
    /* 0x09 ht  */  0x00,
    /* 0x0a nl  */  0x00,
    /* 0x0b vt  */  0x00,
    /* 0x0c np  */  0x00,
	/* 0x0d cr  */  0x00,
    /* 0x0e so  */  0x00,
    /* 0x0f si  */  0x00,
    /* 0x10 dle */  0x00,
    /* 0x11 dc1 */  0x00,
	/* 0x12 dc2 */  0x00,
    /* 0x13 dc3 */  0x00,
    /* 0x14 dc4 */  0x00,
    /* 0x15 nak */  0x00,
	/* 0x16 syn */  0x00,
    /* 0x17 etb */  0x00,
	/* 0x18 can */  0x00,
    /* 0x19 em  */  0x00,
    /* 0x1a sub */  0x00,
    /* 0x1b esc */  0x00,
	/* 0x1c fs  */  0x00,
    /* 0x1d gs  */  0x00,
    /* 0x1e rs  */  0x00,
    /* 0x1f us  */  0x00,
    /* 0x20 sp  */  0x00,
    /* 0x21  !  */  0x00,
    /* 0x22  "  */  0x00,
    /* 0x23  #  */  0x00,   
    /* 0x24  $  */  0x00,
    /* 0x25  %  */  0x00,
    /* 0x26  &  */  0x00,
    /* 0x27  '  */  0x00,
    /* 0x28  (  */  0x00,
	/* 0x29  )  */  0x00,
	/* 0x2a  *  */  0x00,
    /* 0x2b  +  */  0x00,
	/* 0x2c  ,  */  0x00,
    /* 0x2d  -  */  0x00,
    /* 0x2e  .  */  0x00,
    /* 0x2f  /  */  0x00,
    /* 0x30  0  */  0x00,
    /* 0x31  1  */  0x00,
    /* 0x32  2  */  0x00,     /* 0x32 - 0x40 not used */
    /* 0x33  3  */  0x00,
    /* 0x34  4  */  0x00,
    /* 0x35  5  */  0x00,
    /* 0x36  6  */  0x00,
    /* 0x37  7  */  0x00,
	/* 0x38  8  */  0x00,
    /* 0x39  9  */  0x00,
    /* 0x3a  :  */  0x00,
    /* 0x3b  ;  */  0x00,
	/* 0x3c  <  */  0x00,
    /* 0x3d  =  */  0x00,
    /* 0x3e  >  */  0x00,
    /* 0x3f  ?  */  0x00,
	/* 0x40  @  */  0x00,
    /* 0x41  A  */  HP_BeginSession,
    /* 0x42  B  */  HP_EndSession,
    /* 0x43  C  */  HP_BeginPage,
    /* 0x44  D  */  HP_EndPage,
    /* 0x45  E  */  0x00, 
	/* 0x46  F  */  HP_VendorUnique, /* HP_SelfTest */
    /* 0x47  G  */  HP_Comment,  
    /* 0x48  H  */  HP_OpenDataSource,
    /* 0x49  I  */  HP_CloseDataSource,
    /* 0x4a  J  */  HP_EchoComment,
    /* 0x4b  K  */  HP_Query,
    /* 0x4c  L  */  HP_Diagnostic3,
    /* 0x4d  M  */  0x00, 
    /* 0x4e  N  */  0x00, 
    /* 0x4f  O  */  HP_BeginFontHeader,
    /* 0x50  P  */  HP_ReadFontHeader,
    /* 0x51  Q  */  HP_EndFontHeader,
    /* 0x52  R  */  HP_BeginChar,
    /* 0x53  S  */  HP_ReadChar,
    /* 0x54  T  */  HP_EndChar,
    /* 0x55  U  */  HP_RemoveFont,
	/* 0x56  V  */  HP_SetCharAttribute,
	/* 0x57  W  */  HP_SetDefaultGS,
	/* 0x58  X  */  HP_SetColorTreatment,
    /* 0x59  Y  */  HP_SetGlobalAttributes,    
    /* 0x5a  Z  */  HP_ClearGlobalAttributes,     
    /* 0x5b  [  */  HP_BeginStream,
	/* 0x5c  \  */  HP_ReadStream,
	/* 0x5d  ]  */  HP_EndStream,
    /* 0x5e  ^  */  HP_ExecStream,
    /* 0x5f  _  */  HP_RemoveStream,
    /* 0x60  `  */  HP_PopGS,
    /* 0x61  a  */  HP_PushGS,
    /* 0x62  b  */  HP_SetClipReplace,
    /* 0x63  c  */  HP_SetBrushSource,
	/* 0x64  d  */  HP_SetCharAngle,
	/* 0x65  e  */  HP_SetCharScale,
	/* 0x66  f  */  HP_SetCharShear,
	/* 0x67  g  */  HP_SetClipIntersect,
	/* 0x68  h  */  HP_SetClipRectangle,
	/* 0x69  i  */  HP_SetClipToPage,
	/* 0x6a  j  */  HP_SetColorSpace,
    /* 0x6b  k  */  HP_SetCursor,
    /* 0x6c  l  */  HP_SetCursorRel,
    /* 0x6d  m  */  HP_SetHalftoneMethod,
    /* 0x6e  n  */  HP_SetFillMode,
    /* 0x6f  o  */  HP_SetFont,
	/* 0x70  p  */  HP_SetLineDash,
    /* 0x71  q  */  HP_SetLineCap,
    /* 0x72  r  */  HP_SetLineJoin,
	/* 0x73  s  */  HP_SetMiterLimit,
    /* 0x74  t  */  HP_SetPageDefaultCTM,
    /* 0x75  u  */  HP_SetPageOrigin,
    /* 0x76  v  */  HP_SetPageRotation,
    /* 0x77  w  */  HP_SetPageScale,
	/* 0x78  x  */  HP_SetPaintTxMode,
    /* 0x79  y  */  HP_SetPenSource,
    /* 0x7a  z  */  HP_SetPenWidth,
    /* 0x7b  {  */  HP_SetROP,
	/* 0x7c  |  */  HP_SetSourceTxMode,
	/* 0x7d  }  */  HP_SetCharBoldValue,
	/* 0x7e  ~  */  0x00,
	/* 0x7f del */  HP_SetClipMode,
	/* 0x80     */  HP_SetPathToClip,
	/* 0x81     */  HP_SetCharSubMode,
	/* 0x82     */  HP_BeginUserDefinedLineCap,
	/* 0x83     */  HP_EndUserDefinedLineCap,
	/* 0x84     */  HP_CloseSubPath,
	/* 0x85     */  HP_NewPath,
	/* 0x86     */  HP_PaintPath,
	/* 0x87     */  HP_BeginBackground,
	/* 0x88     */  HP_EndBackground,
	/* 0x89     */  HP_DrawBackground,
	/* 0x8a     */  HP_RemoveBackground,
	/* 0x8b     */  HP_BeginForm,
	/* 0x8c     */  HP_EndForm,
	/* 0x8d     */  HP_DrawForm,
	/* 0x8e     */  HP_RemoveForm,
	/* 0x8f     */  HP_RegisterFormAsPattern,
	/* 0x90     */  0x00,
    /* 0x91     */  HP_ArcPath,
    /* 0x92     */  0x00, 
    /* 0x93     */  HP_BezierPath,
	/* 0x94     */  0x00,
    /* 0x95     */  HP_BezierRelPath,
    /* 0x96     */  HP_Chord,
    /* 0x97     */  HP_ChordPath,
	/* 0x98     */  HP_Ellipse,
    /* 0x99     */  HP_EllipsePath,
	/* 0x9a     */  0x00,
    /* 0x9b     */  HP_LinePath,
    /* 0x9c     */  0x00, 
    /* 0x9d     */  HP_LineRelPath,
    /* 0x9e     */  HP_Pie,
    /* 0x9f     */  HP_PiePath,
    /* 0xa0     */  HP_Rectangle,
    /* 0xa1     */  HP_RectanglePath,
    /* 0xa2     */  HP_RoundRectangle,
    /* 0xa3     */  HP_RoundRectanglePath,
    /* 0xa4     */  0x00, 
    /* 0xa5     */  0x00, 
	/* 0xa6     */  0x00,
    /* 0xa7     */  0x00, 
    /* 0xa8     */  HP_Text,
    /* 0xa9     */  HP_TextPath,
	/* 0xaa     */  HP_SystemText,
    /* 0xab     */  0x00, 
    /* 0xac     */  0x00,
    /* 0xad     */  0x00,
    /* 0xae     */  0x00,
    /* 0xaf     */  0x00,
    /* 0xb0     */  HP_BeginImage,
    /* 0xb1     */  HP_ReadImage,
    /* 0xb2     */  HP_EndImage,
    /* 0xb3     */  HP_BeginRastPattern,
    /* 0xb4     */  HP_ReadRastPattern,
    /* 0xb5     */  HP_EndRastPattern,
    /* 0xb6     */  HP_BeginScan,
    /* 0xb7     */  0x00,
	/* 0xb8     */  HP_EndScan,
	/* 0xb9     */  HP_ScanLineRel,
	/* 0xba     */  0x00,
    /* 0xbb     */  0x00,
    /* 0xbc     */  0x00,
    /* 0xbd     */  0x00,
	/* 0xbe     */  0x00,
    /* 0xbf     */  HP_PassThrough,
    /* 0xc0     */  0x00,
    /* 0xc1     */  0x00,
	/* 0xc2     */  0x00,
	/* 0xc3     */  0x00,
	/* 0xc4     */  0x00,
    /* 0xc5     */  0x00,
    /* 0xc6     */  0x00, 
    /* 0xc7     */  0x00,
    /* 0xc8     */  0x00,
    /* 0xc9     */  0x00,
    /* 0xca     */  0x00,
    /* 0xcb     */  0x00,
    /* 0xcc     */  0x00,
    /* 0xcd     */  0x00,
    /* 0xce     */  0x00,
    /* 0xcf     */  0x00,
    /* 0xd0     */  0x00,
    /* 0xd1     */  0x00,
    /* 0xd2     */  0x00,
    /* 0xd3     */  0x00,
	/* 0xd4     */  0x00,
    /* 0xd5     */  0x00,
    /* 0xd6     */  0x00,
    /* 0xd7     */  0x00,
	/* 0xd8     */  0x00,
	/* 0xd9     */  0x00,
    /* 0xda     */  0x00,
    /* 0xdb     */  0x00,
    /* 0xdc     */  0x00,
    /* 0xdd     */  0x00,
    /* 0xde     */  0x00,
    /* 0xdf     */  0x00,
    /* 0xe0     */  0x00,
    /* 0xe1     */  0x00,
    /* 0xe2     */  0x00,
	/* 0xe3     */  0x00,
	/* 0xe4     */  0x00,
    /* 0xe5     */  0x00,
    /* 0xe6     */  0x00,
    /* 0xe7     */  0x00,
	/* 0xe8     */  0x00,
    /* 0xe9     */  0x00,
    /* 0xea     */  0x00,
	/* 0xeb     */  0x00,
    /* 0xec     */  0x00,
    /* 0xed     */  0x00,
	/* 0xee     */  0x00,
    /* 0xef     */  0x00,
    /* 0xf0     */  0x00,
    /* 0xf1     */  0x00,
    /* 0x2f     */  0x00,
    /* 0xf3     */  0x00,
    /* 0xf4     */  0x00,
    /* 0xf5     */  0x00,
	/* 0xf6     */  0x00,
	/* 0xf7     */  0x00,
    /* 0xf8     */  0x00,
	/* 0xf9     */  0x00,
    /* 0xfa     */  0x00,
    /* 0xfb     */  0x00,
    /* 0xfc     */  0x00, /* Reserved */
    /* 0xfd     */  0x00, /* Reserved */
	/* 0xfe     */  0x00, /* Reserved */
    /* 0xff     */  0x00  /* Reserved */
    };

const HP_UByte HP_AttrIdValueMap[HP_MaxNumberOfAttrIds]=
{
	/*     0 */ 0x00,
	/*     1 */ HP_CMYColor,
	/*     2 */ HP_PaletteDepth,
	/*     3 */ HP_ColorSpace,
	/*     4 */ HP_NullBrush,
	/*     5 */ HP_NullPen,
	/*     6 */ HP_PaletteData,
	/*     7 */ HP_PaletteIndex,
	/*     8 */ HP_PatternSelectID,
	/*     9 */ HP_GrayLevel,
	/*    10 */ 0x00,
	/*    11 */ HP_RGBColor,
	/*    12 */ HP_PatternOrigin,
	/*    13 */ HP_NewDestinationSize,
	/*    14 */ HP_PrimaryArray,
	/*    15 */ HP_PrimaryDepth,
	/*    16 */ 0x00,
	/*    17 */ HP_ColorimetricColorSpace,
	/*    18 */ HP_XYChromaticities,
	/*    19 */ HP_WhitePointReference,
	/*    20 */ HP_CRGBMinMax,
	/*    21 */ HP_GammaGain,
	/*    22 */ 0x00,
	/*    23 */ 0x00,
	/*    24 */ 0x00,
	/*    25 */ 0x00,
	/*    26 */ 0x00,
	/*    27 */ 0x00,
	/*    28 */ 0x00,
	/*    29 */ 0x00,
	/*    30 */ 0x00,
	/*    31 */ 0x00,
	/*    32 */ 0x00,
	/*    33 */ HP_DeviceMatrix,
	/*    34 */ HP_DitherMatrixDataType,
	/*    35 */ HP_DitherOrigin,
	/*    36 */ HP_MediaDest,
	/*    37 */ HP_MediaSize,
	/*    38 */ HP_MediaSource,
    /*    39 */ HP_MediaType,
    /*    40 */ HP_Orientation,
    /*    41 */ HP_PageAngle,
    /*    42 */ HP_PageOrigin,
    /*    43 */ HP_PageScale,
    /*    44 */ HP_ROP3,
    /*    45 */ HP_TxMode,
    /*    46 */ 0x00,
    /*    47 */ HP_CustomMediaSize,
    /*    48 */ HP_CustomMediaSizeUnits,
    /*    49 */ HP_PageCopies,
    /*    50 */ HP_DitherMatrixSize,
	/*    51 */ HP_DitherMatrixDepth,
    /*    52 */ HP_SimplexPageMode,
	/*    53 */ HP_DuplexPageMode,
    /*    54 */ HP_DuplexPageSide,
    /*    55 */ 0x00,
    /*    56 */ 0x00,
    /*    57 */ 0x00,
    /*    58 */ 0x00,
    /*    59 */ 0x00,
	/*    60 */ 0x00,
    /*    61 */ 0x00,
    /*    62 */ 0x00,
	/*    63 */ HP_LineStartCapStyle,
    /*    64 */ HP_LineEndCapStyle,
    /*    65 */ HP_ArcDirection,
    /*    66 */ HP_BoundingBox,
    /*    67 */ HP_DashOffset,
    /*    68 */ HP_EllipseDimension,
    /*    69 */ HP_EndPoint,
    /*    70 */ HP_FillMode,
    /*    71 */ HP_LineCapStyle,
    /*    72 */ HP_LineJoinStyle,
    /*    73 */ HP_MiterLength,
    /*    74 */ HP_LineDashStyle,
    /*    75 */ HP_PenWidth,
    /*    76 */ HP_Point,
    /*    77 */ HP_NumberOfPoints,
	/*    78 */ HP_SolidLine,
    /*    79 */ HP_StartPoint,
    /*    80 */ HP_PointType,
    /*    81 */ HP_ControlPoint1,
    /*    82 */ HP_ControlPoint2,
    /*    83 */ HP_ClipRegion,
    /*    84 */ HP_ClipMode,
	/*    85 */ 0x00,
    /*    86 */ 0x00,
    /*    87 */ 0x00,
	/*    88 */ 0x00,
	/*    89 */ 0x00,
    /*    90 */ 0x00,
    /*    91 */ 0x00,
    /*    92 */ 0x00,
	/*    93 */ 0x00,
	/*    94 */ 0x00,
	/*    95 */ 0x00,
	/*    96 */ 0x00,
	/*    97 */ HP_ColorDepthArray,
	/*    98 */ HP_ColorDepth,
	/*    99 */ HP_BlockHeight,
	/*   100 */ HP_ColorMapping,
	/*   101 */ HP_CompressMode,
	/*   102 */ HP_DestinationBox,
	/*   103 */ HP_DestinationSize,
	/*   104 */ HP_PatternPersistence,
	/*   105 */ HP_PatternDefineID,
	/*   106 */ 0x00,
	/*   107 */ HP_SourceHeight,
	/*   108 */ HP_SourceWidth,
	/*   109 */ HP_StartLine,
	/*   110 */ HP_PadBytesMultiple,
	/*   111 */ HP_BlockByteLength,
	/*   112 */ 0,
	/*   113 */ 0,
	/*   114 */ 0,
	/*   115 */ HP_NumberOfScanLines,
	/*   116 */ 0x00,
	/*   117 */ 0x00,
	/*   118 */ 0x00,
	/*   119 */ 0x00,
	/*   120 */ HP_ColorTreatment,
	/*   121 */ HP_FileName,
	/*   122 */ HP_BackgroundName,
	/*   123 */ HP_FormName,
	/*   124 */ HP_FormType,
	/*   125 */ HP_FormSize,
	/*   126 */ HP_UDLCName,
	/*   127 */ 0x00,
	/*   128 */ 0x00,
	/*   129 */ HP_CommentData,
	/*   130 */ HP_DataOrg,
	/*   131 */ 0x00,
	/*   132 */ 0x00,
    /*   133 */ 0x00,
    /*   134 */ HP_Measure,
    /*   135 */ 0x00,
    /*   136 */ HP_SourceType,
    /*   137 */ HP_UnitsPerMeasure,
    /*   138 */ HP_QueryKey,
	/*   139 */ HP_StreamName,
	/*   140 */ HP_StreamDataLength,
    /*   141 */ 0x00, 
    /*   142 */ 0x00,
    /*   143 */ HP_ErrorReport,
	/*   144 */ 0x00,
	/*   145 */ HP_VUExtension,
	/*   146 */ HP_VUDataLength,
	/*   147 */ HP_VUAttr1,
	/*   148 */ HP_VUAttr2,
	/*   149 */ HP_VUAttr3,
	/*   150 */ HP_VUAttr4,
	/*   151 */ HP_VUAttr5,
	/*   152 */ HP_VUAttr6,
	/*   153 */ HP_VUAttr7,
	/*   154 */ HP_VUAttr8,
	/*   155 */ HP_VUAttr9,
	/*   156 */ HP_VUAttr10,
	/*   157 */ HP_VUAttr11,
	/*   158 */ HP_VUAttr12, /* HP_PassThroughCommand, */
	/*   159 */ HP_PassThroughArray,
	/*   160 */ HP_Diagnostics,
	/*   161 */ HP_CharAngle,
	/*   162 */ HP_CharCode,
	/*   163 */ HP_CharDataSize,
	/*   164 */ HP_CharScale,
	/*   165 */ HP_CharShear,
	/*   166 */ HP_CharSize,
	/*   167 */ HP_FontHeaderLength,
	/*   168 */ HP_FontName,
	/*   169 */ HP_FontFormat,
	/*   170 */ HP_SymbolSet,
	/*   171 */ HP_TextData,
	/*   172 */ HP_CharSubModeArray,
	/*   173 */ HP_WritingMode,
	/*   174 */ HP_BitmapCharScale,
	/*   175 */ HP_XSpacingData,
    /*   176 */ HP_YSpacingData,
    /*   177 */ HP_CharBoldValue,
    /*   178 */ 0x00,
    /*   179 */ 0x00,
	/*   180 */ 0x00,
	/*   181 */ 0x00,
    /*   182 */ 0x00,
    /*   183 */ 0x00,
    /*   184 */ 0x00,
    /*   185 */ 0x00,
    /*   186 */ 0x00,
    /*   187 */ 0x00,
    /*   188 */ 0x00,
	/*   189 */ 0x00,
	/*   190 */ 0x00,
	/*   191 */ 0x00,
    /*   192 */ 0x00,
    /*   193 */ 0x00,
    /*   194 */ 0x00,
    /*   195 */ 0x00,
    /*   196 */ 0x00,
    /*   197 */ 0x00,
    /*   198 */ 0x00,
    /*   199 */ 0x00,
    /*   200 */ 0x00,
    /*   201 */ 0x00,
    /*   202 */ 0x00,
    /*   203 */ 0x00,
	/*   204 */ 0x00,
    /*   205 */ 0x00,
    /*   206 */ 0x00,
    /*   207 */ 0x00,
    /*   208 */ 0x00,
    /*   209 */ 0x00,
    /*   210 */ 0x00,
    /*   211 */ 0x00,
    /*   212 */ 0x00,
	/*   213 */ 0x00,
    /*   214 */ 0x00,
    /*   215 */ 0x00,
    /*   216 */ 0x00,
    /*   217 */ 0x00,
    /*   218 */ 0x00,
    /*   219 */ 0x00,
    /*   220 */ 0x00,
    /*   221 */ 0x00,
    /*   222 */ 0x00,
    /*   223 */ 0x00,
    /*   224 */ 0x00,
    /*   225 */ 0x00,
    /*   226 */ 0x00,
    /*   227 */ 0x00,
    /*   228 */ 0x00,
    /*   229 */ 0x00,
    /*   230 */ 0x00,
	/*   231 */ 0x00,
    /*   232 */ 0x00,
    /*   233 */ 0x00,
    /*   234 */ 0x00,
    /*   235 */ 0x00,
    /*   236 */ 0x00,
    /*   237 */ 0x00,
    /*   238 */ 0x00,
    /*   239 */ 0x00,
    /*   240 */ 0x00,
	/*   241 */ 0x00,
	/*   242 */ 0x00,
    /*   243 */ 0x00,
    /*   244 */ 0x00,
	/*   245 */ 0x00,
	/*   246 */ 0x00,
    /*   247 */ 0x00,
    /*   248 */ 0x00,
    /*   249 */ 0x00,
    /*   250 */ 0x00,
    /*   251 */ 0x00,
    /*   252 */ 0x00,
    /*   253 */ 0x00,
    /*   254 */ 0x00,
    /*   255 */ 0x00
};

/*****************************************************************************

    function:   HP_Real32toUInt32

    purpose:    Take a HP_Real32 value and convert it to a UInt32 value
                where the most significant byte is the left-most byte
				and the least significant byte is the right-most byte.

    parameters: name                data type / description
                -----------------   -------------------------------------
                realNumber          HP_Real32 / A HP library 32-bit real
                                    number which is typically a float.

    comments:   This routine is only used internal to the library in the
                macros defined to output a Real32 value.

*****************************************************************************/
#ifdef HP_WindowsAPI
HP_UInt32 FAR PASCAL HP_Real32ToUInt32(
#else
HP_UInt32 HP_Real32ToUInt32(
#endif
/* HP_Real32ToUInt32(*/ HP_Real32 realNumber)
{

	union {
		HP_Real32 real32Num;
        HP_UInt32 uint32Num;
    } numUnion;

	numUnion.real32Num = realNumber;

    return(numUnion.uint32Num);

}

/*****************************************************************************

    function:   HP_GetLibErrorText

    purpose:    Return a pointer to a constant byte string containing
                text which explains the error found.  If no text is found
                for the error number passed in, a null pointer is returned.

    parameters: name                data type / description
                -----------------   -------------------------------------
                errorNumber         HP_SInt16 / A library error code number
                                    #defined in the jetlib.h file.  

    comments:   

*****************************************************************************/
HP_pUByte HP_GetLibErrorText(HP_SInt16 errorNumber)
{
    
    HP_pUByte pText;
    
	switch (errorNumber)
    {
        case HP_NoError:
			pText = (HP_pUByte) "XL Library: No Errors Detected";
            break;
        case HPERR_InitError:
            pText = (HP_pUByte) "XL Library: Initialization Failed";
            break;
        case HPERR_UndefinedOp:
            pText = (HP_pUByte) "XL Library: Operator Number Undefined";
            break;
        case HPERR_OutOfMemory:
            pText = (HP_pUByte) "XL Library: Out of Memory";
            break;
        case HPERR_ZeroBufferSize:
			pText = (HP_pUByte) "XL Library: A Stream Buffer Size of Zero Was Requested";
            break;
        case HPERR_OutBufferOverflow:
			pText = (HP_pUByte) "XL Library: Stream Output Buffer Overflowed";
            break;
        case HPERR_FlushBufferFailed:
            pText = (HP_pUByte) "XL Library: Attempt to Flush Output Buffer Failed";
            break;
        default:
            pText = (HP_pUByte) "XL Library: Number of Error Reported is Undefined";
            break;
    }
    
	return(pText);
}
                                                        

#ifndef HP_WindowsAPI                                                       
   
/*****************************************************************************

    function:   HP_NewStream

    purpose:    Initialize a new stream.  This must be called prior
                to accessing any library routines.

    parameters: 
                HP_SInt32 outBufferMaxSize :: max size of output buffer 
                                              maintained by stream library
                                              
                flushFunctionType pFlushFunction :: pointer to flush output buffer
                                                    callback function
                                                    
                unsigned long cookie :: magic cookie for callers use included in
                                        callback of flush output buffer function

*****************************************************************************/
#ifndef OO_INTERFACE
HP_InitFuncPrefix HP_NewStream(HP_SInt32 outBufferMaxSize, flushFunctionType pFlushFunction,
							   unsigned long cookie)
#else
HP_InitFuncPrefix HP_NewStream(HP_SInt32 outBufferMaxSize, MJetlibClient *pClient,
							   unsigned long cookie)
#endif
{
     
    HP_StreamHandleType pStream, returnVal = NULL;

    pStream = (HP_StreamHandleType) HP_MEMALLOC ((size_t) sizeof(HP_StreamStructType));
    if (pStream)
    {
#ifndef OO_INTERFACE
        if (pFlushFunction)
#else
		if (pClient)
#endif
        {
            pStream->HP_OutBufferMaxSize = outBufferMaxSize;
            if (pStream->HP_OutBufferMaxSize)
            {                                                         
                /* enhance checking here to insure that buffer size meets some
                 * reasonable minimum value
                 */
                pStream->HP_ErrorCode = HP_NoError;
				pStream->HP_ErrorCount = 0;
				pStream->HP_OutBuffer =
                        (HP_UByte *) HP_MEMALLOC ((size_t) pStream->HP_OutBufferMaxSize);
                if (pStream->HP_OutBuffer)
                {
                    pStream->HP_CurrentBufferLen = 0; 
                    pStream->HP_CurrentStreamLen = 0; 
                    pStream->cookie = cookie;
#ifndef OO_INTERFACE                    
                    pStream->pFlushOutBuffer = (void (*)(void *,unsigned long,unsigned char *,long))pFlushFunction;
#else
                    pStream->pClient = pClient;
#endif
                    returnVal = pStream;
                    return(returnVal);
                }
                else
                {
                    HP_SetErrorCode(pStream, HPERR_OutOfMemory);
                }
			}
            else
            {
				HP_SetErrorCode(pStream, HPERR_ZeroBufferSize);
            }
        }
		else
        {
           HP_SetErrorCode(pStream, HPERR_NullFlushFunction);
        }
    }                                                  
    else
    {
        HP_SetErrorCode(pStream, HPERR_OutOfMemory);
    }                             
    
    return(returnVal);
            
}   

/*****************************************************************************

    function:   HP_FinishStream

    purpose:    Finish the stream.  This must be called to do a final
                flush on the stream output buffer and release all memory 
                allocated by the stream library.

    parameters: None 
    
    return:     returns the total length of the stream

	comments:

*****************************************************************************/
HP_UInt32FuncPrefix HP_FinishStream(HP_StreamHandleType pStream)
{
    HP_UInt32 streamLength = pStream->HP_CurrentStreamLen;
    
#ifndef OO_INTERFACE
    pStream->pFlushOutBuffer(pStream, pStream->cookie, pStream->HP_OutBuffer, pStream->HP_CurrentBufferLen);
#else
    pStream->pClient->FlushOutBuffer(pStream, pStream->cookie, pStream->HP_OutBuffer, pStream->HP_CurrentBufferLen);
#endif
    HP_MEMDEALLOC(pStream->HP_OutBuffer);
    HP_MEMDEALLOC(pStream); 
    
    return(streamLength);
            
}   

#endif

/*****************************************************************************

    function:   HP_FlushStreamBuffer

	purpose:    Flushes the stream buffer to the output stream.

    parameters: None 
    
    return:     None
    
    comments:   

*****************************************************************************/
HP_StdFuncPrefix HP_FlushStreamBuffer(HP_StreamHandleType pStream)
{
#ifndef OO_INTERFACE
    if (pStream->pFlushOutBuffer)
    {
        pStream->pFlushOutBuffer(pStream, 
#else
    if (pStream->pClient)
    {
        pStream->pClient->FlushOutBuffer(pStream,
#endif
                                 pStream->cookie,
                                 pStream->HP_OutBuffer,
                                 pStream->HP_CurrentBufferLen
                                 );
    }
}



/*****************************************************************************


    function:   HP_Operator

    purpose:    Execute or schedule execution of an Operator corresponding to 
                the operatorID

    parameters: name                data type / description
                ----------------    -------------------------------------
                operatorId          HP_UInt16 / An operator identifier from 
                                    the jetlib.h header file signifying the
                                    operator to be executed 

    comments:   The behavior of this function is binding-dependent.  This 
                implementation places a binding-dependent operator 
                identifier in the protocol byte stream.
                
    errors:     HPERR_OutOfMemory, HPERR_UndefinedOp

*****************************************************************************/
HP_StdFuncPrefix HP_Operator(HP_StreamHandleType pStream, HP_UInt16 operatorId)
{
    
    HP_UInt16 bindingOperatorId;
    
    /* Note: all HP_AddToList macros can set the errorCode */
    
    if (HP_NoErrors(pStream))
    {
    
        bindingOperatorId = HP_OpValueMap[operatorId];
        if ((operatorId < HP_MaxNumberOfOps) && bindingOperatorId)
        {
			HP_AddToListByte(pStream, (HP_UByte) bindingOperatorId);
#ifndef HP_NoAutoFlush
			pStream->HP_FlushFunction(pStream, pStream->cookie, pStream->HP_OutBuffer, pStream->HP_CurrentBufferLen);
			pStream->HP_CurrentBufferLen = 0;
#endif
		}
		else
		{
			HP_SetErrorCode(pStream, HPERR_UndefinedOp);
		}

	}

}


/*****************************************************************************


	function:   HP_AttrId

	purpose:    Add an attribute identifier to the attribute list

	parameters: name                data type / description
				-----------------   -------------------------------------
				attrId              HP_UInt16 / An attribute identifier from
									the jetlib.h header file signifying the
									attribute identifier to be added

	comments:   The behavior of this function is binding-dependent.  This
				implementation places an attribute identifier in the protocol
				byte stream.


	errors:     HPERR_OutOfMemory, HPERR_UndefinedAttr

*****************************************************************************/
HP_StdFuncPrefix HP_AttrId(HP_StreamHandleType pStream, HP_UInt16 attrId)
{

	/* Note: all HP_AddToList macros can set the errorCode */

	HP_UInt16 bindingAttributeId;

	if (HP_NoErrors(pStream))
	{

		bindingAttributeId = HP_AttrIdValueMap[attrId];
		if ((attrId < HP_MaxNumberOfAttrIds) && bindingAttributeId)
		{

			HP_AddToListByte(pStream, (HP_UByte) HP_8BitAttrId);
			if (HP_NoErrors(pStream))
			{
				HP_AddToListByte(pStream, (HP_UByte) bindingAttributeId);
			}
		}
		else
		{
			HP_SetErrorCode(pStream, HPERR_UndefinedAttrId);
		}

	}


}

HP_UInt16 HP_strlen(HP_pUByte array)
{

	HP_UInt16 length = 0;
	while (array[length]) length++;
	return(length);
}

HP_StdFuncPrefix HP_AddStreamHeader_1( /* AddStreamHeader signature 1 */
		HP_StreamHandleType pStream,   /* PCL XL Stream library pointer */
		HP_UInt16 protocolClass,       /* protocol class number */
		HP_UInt16 protocolRev,         /* protocol class revision */
		HP_pUByte commentArray,        /* any comment desired */
		HP_UInt32 commentArrayLen)     /* length of comment array */
{

#define HP_MAX_NUM_BUFF 10
#define HP_LINEFEED 0x0a

	HP_UInt32 index = 0;
	HP_UByte numberBuff[HP_MAX_NUM_BUFF];
	static HP_pUByte streamClassName = (HP_pUByte) ") HP-PCL XL";
	static HP_pUByte commentFieldId = (HP_pUByte) ";Comment";

	numberBuff[0] = '\0';

    /* Note: all HP_AddToList macros can set the errorCode */

    if (HP_NoErrors(pStream))
    {
    /* Add stream class name field to stream header: */

        unsigned int nameLen = HP_strlen(streamClassName);

        index = 0;
        while ((HP_NoErrors(pStream)) && (index < nameLen))
        {
           HP_AddToListByte(pStream, (HP_UByte) streamClassName[index++]);
        }
	}

    if (HP_NoErrors(pStream))
    {
    /* Add protocol class number field to stream header: */

#ifndef HP_WindowsAPI
    sprintf((char *) numberBuff, ";%d", protocolClass);
#else
    wsprintf((char *) numberBuff, ";%d", protocolClass);
#endif
	{
        unsigned int numberLen = HP_strlen(numberBuff);

            index = 0;
            while ((HP_NoErrors(pStream)) && (index < numberLen))
            {
               HP_AddToListByte(pStream, (HP_UByte) numberBuff[index++]);
            }
    }
    }

	if (HP_NoErrors(pStream))
	{
	/* Add protocol class revision field to stream header: */

#ifndef HP_WindowsAPI
		sprintf((char *) numberBuff, ";%d", protocolRev);
#else
		wsprintf((char *) numberBuff, ";%d", protocolRev);
#endif
		{
			unsigned int numberLen = HP_strlen(numberBuff);

			index = 0;
			while ((HP_NoErrors(pStream)) && (index < numberLen))
			{
			   HP_AddToListByte(pStream, (HP_UByte) numberBuff[index++]);
			}

		}
	}

	if (HP_NoErrors(pStream))
	{
		/* Add Comment field prefix to stream header: */

		unsigned int commentFieldLen = HP_strlen(commentFieldId);

		index = 0;
		while ((HP_NoErrors(pStream)) && (index < commentFieldLen))
		{
		   HP_AddToListByte(pStream, (HP_UByte) commentFieldId[index++]);
		}
	}

	if (HP_NoErrors(pStream))
	{
		/* Add user comment to stream header: */

		index = 0;
		while ((HP_NoErrors(pStream)) && (index < commentArrayLen))
		{
		   HP_AddToListByte(pStream, (HP_UByte) commentArray[index++]);
		}
	}

	if (HP_NoErrors(pStream))
	{
	/* Add line feed to end the stream header: */

		HP_AddToListByte(pStream, (HP_UByte) HP_LINEFEED);
	}

#undef HP_LINEFEED
#undef HP_MAX_NUM_BUFF
}


/*****************************************************************************


	function:   HP_AttrUByte

	purpose:    Add an 8-bit unsigned value to the
				attribute list

	parameters: name                data type / description
				-----------------   -------------------------------------
				intValue            HP_UByte / The 8-bit integer value to be
									added to the attribute list

	comments:   The behavior of this function is binding-dependent.  This
				implementation places the value in the protocol byte stream.

*****************************************************************************/
HP_StdFuncPrefix HP_AttrUByte(HP_StreamHandleType pStream, HP_UByte intValue)
{



	/* Note: all HP_AddToList macros can set the errorCode */

	if (HP_NoErrors(pStream))
	{

		HP_AddToListByte(pStream, (HP_UByte) HP_UByteData);
		if (HP_NoErrors(pStream))
		{
			HP_AddToListByte(pStream, (HP_UByte) intValue);
		}

	}

}


/*****************************************************************************


	function:   HP_AttrUInt16

	purpose:    Add a 16-bit unsigned integer value to the
				attribute list

	parameters: name                data type / description
				-----------------   -------------------------------------
				intValue            HP_UInt16 / The 16-bit integer value to be
									added to the attribute list

	comments:   The behavior of this function is binding-dependent.  This
				implementation places the value in the protocol byte stream.

*****************************************************************************/
HP_StdFuncPrefix HP_AttrUInt16(HP_StreamHandleType pStream, HP_UInt16 intValue)
{


	/* Note: all HP_AddToList macros can set the errorCode */

	if (HP_NoErrors(pStream))
	{

		HP_AddToListByte(pStream, (HP_UByte) HP_UInt16Data);
		if (HP_NoErrors(pStream))
		{
			HP_AddToListInt16(pStream, (HP_UInt16) intValue);
		}
	}


}

/*****************************************************************************


	function:   HP_AttrSInt16

	purpose:    Add a 16-bit signed integer value to the
				attribute list

	parameters: name                data type / description
				-----------------   -------------------------------------
				intValue            HP_SInt16 / The 16-bit integer value to be
									added to the attribute list

	comments:   The behavior of this function is binding-dependent.  This
				implementation places the value in the protocol byte stream.

*****************************************************************************/
HP_StdFuncPrefix HP_AttrSInt16(HP_StreamHandleType pStream, HP_SInt16 intValue)
{


	/* Note: all HP_AddToList macros can set the errorCode */

	if (HP_NoErrors(pStream))
	{

		HP_AddToListByte(pStream, (HP_UByte) HP_SInt16Data);
		if (HP_NoErrors(pStream))
		{
			HP_AddToListInt16(pStream, (HP_UInt16) intValue);
		}
	}

}


/*****************************************************************************


	function:   HP_AttrUInt32

	purpose:    Add a 32-bit unsigned integer value to the
				attribute list

	parameters: name                data type / description
				-----------------   -------------------------------------
				intValue            HP_UInt32 / The 32-bit integer value to be
									added to the attribute list

	comments:   The behavior of this function is binding-dependent.  This
				implementation places the value in the protocol byte stream.

*****************************************************************************/
HP_StdFuncPrefix HP_AttrUInt32(HP_StreamHandleType pStream, HP_UInt32 intValue)
{

	/* Note: all HP_AddToList macros can set the errorCode */

	if (HP_NoErrors(pStream))
	{

		HP_AddToListByte(pStream, (HP_UByte) HP_UInt32Data);
		if (HP_NoErrors(pStream))
		{
			HP_AddToListInt32(pStream, (HP_UInt32) intValue);
		}
	}

}

/*****************************************************************************


	function:   HP_AttrSInt32

	purpose:    Add a 32-bit signed integer value to the
				attribute list

	parameters: name                data type / description
				-----------------   -------------------------------------
				intValue            HP_SInt32 / The 32-bit integer value to be
									added to the attribute list

	comments:   The behavior of this function is binding-dependent.  This
				implementation places the value in the protocol byte stream.

*****************************************************************************/
HP_StdFuncPrefix HP_AttrSInt32(HP_StreamHandleType pStream, HP_SInt32 intValue)
{

	/* Note: all HP_AddToList macros can set the errorCode */

	if (HP_NoErrors(pStream))
	{

		HP_AddToListByte(pStream, (HP_UByte) HP_SInt32Data);
		if (HP_NoErrors(pStream))
		{
			HP_AddToListInt32(pStream, (HP_UInt32) intValue);
		}
	}

}


/*****************************************************************************


	function:   HP_AttrReal32

	purpose:    Add a 32-bit real (floating point) value to the
				attribute list

	parameters: name                data type / description
				-----------------   -------------------------------------
				realValue           HP_Real32 / The 32-bit real value to be
									added to the attribute list

	comments:   The behavior of this function is binding-dependent.  This
				implementation places the value in the protocol byte stream.

*****************************************************************************/
HP_StdFuncPrefix HP_AttrReal32(HP_StreamHandleType pStream, HP_Real32 realValue)
{

	/* Note: all HP_AddToList macros can set the errorCode */

	if (HP_NoErrors(pStream))
	{

		HP_AddToListByte(pStream, (HP_UByte) HP_Real32Data);
		if (HP_NoErrors(pStream))
		{
			HP_AddToListReal32(pStream, (HP_Real32) realValue);
		}
	}

}


/*****************************************************************************


	function:   HP_AttrXyUByte

	purpose:    Add two 8-bit unsigned values to the
				attribute list.  One represents a value corresponding to
				the x-axis and the other corresponds to the y-axis.

	parameters: name                data type / description
				-----------------   -------------------------------------
				xValue              HP_UByte / The 8-bit integer x value to
									be added to the attribute list

				yValue              HP_UByte / The 8-bit integer y value to
									be added to the attribute list

	comments:   The behavior of this function is binding-dependent.  This
				implementation places the values in the protocol byte stream.

*****************************************************************************/
HP_StdFuncPrefix HP_AttrXyUByte(HP_StreamHandleType pStream, HP_UByte xValue, HP_UByte yValue)
{

	/* Note: all HP_AddToList macros can set the errorCode */

	if (HP_NoErrors(pStream))
	{

		HP_AddToListByte(pStream, (HP_UByte) HP_UByteXy);
		if (HP_NoErrors(pStream))
		{
			HP_AddToListByte(pStream, (HP_UByte) xValue);
		}
		if (HP_NoErrors(pStream))
		{
			HP_AddToListByte(pStream, (HP_UByte) yValue);
		}
	}

}


/*****************************************************************************


	function:   HP_AttrXyUInt16

	purpose:    Add two 16-bit unsigned integer values to the
				attribute list.  One represents a value corresponding to
				the x-axis and the other corresponds to the y-axis.

	parameters: name                data type / description
				-----------------   -------------------------------------
				xValue              HP_UInt16 / The 16-bit integer x value to
									be added to the attribute list

				yValue              HP_UInt16 / The 16-bit integer y value to
									be added to the attribute list

	comments:   The behavior of this function is binding-dependent.  This
				implementation places the values in the protocol byte stream.

*****************************************************************************/
HP_StdFuncPrefix HP_AttrXyUInt16(HP_StreamHandleType pStream, HP_UInt16 xValue, HP_UInt16 yValue)
{


	if (HP_NoErrors(pStream))
	{

		HP_AddToListByte(pStream, (HP_UByte) HP_UInt16Xy);
		if (HP_NoErrors(pStream))
		{
			HP_AddToListInt16(pStream, (HP_UInt16) xValue);
		}
		if (HP_NoErrors(pStream))
		{
			HP_AddToListInt16(pStream, (HP_UInt16) yValue);
		}
	}

}

/*****************************************************************************


	function:   HP_AttrXySInt16

	purpose:    Add two 16-bit signed integer values to the
				attribute list.  One represents a value corresponding to
				the x-axis and the other corresponds to the y-axis.

	parameters: name                data type / description
				-----------------   -------------------------------------
				xValue              HP_SInt16 / The 16-bit integer x value to
									be added to the attribute list

				yValue              HP_SInt16 / The 16-bit integer y value to
									be added to the attribute list

	comments:   The behavior of this function is binding-dependent.  This
				implementation places the values in the protocol byte stream.

*****************************************************************************/
HP_StdFuncPrefix HP_AttrXySInt16(HP_StreamHandleType pStream, HP_SInt16 xValue, HP_SInt16 yValue)
{


	if (HP_NoErrors(pStream))
	{

		HP_AddToListByte(pStream, (HP_UByte) HP_SInt16Xy);
		if (HP_NoErrors(pStream))
		{
			HP_AddToListInt16(pStream, (HP_UInt16) xValue);
		}
		if (HP_NoErrors(pStream))
		{
			HP_AddToListInt16(pStream, (HP_UInt16) yValue);
		}

	}

}


/*****************************************************************************


	function:   HP_AttrXyUInt32

	purpose:    Add two 32-bit unsigned integer values to the
				attribute list.  One represents a value corresponding to
				the x-axis and the other corresponds to the y-axis.

	parameters: name                data type / description
				-----------------   -------------------------------------
				xValue              HP_UInt32 / The 32-bit integer x value to
									be added to the attribute list

				yValue              HP_UInt32 / The 32-bit integer y value to
									be added to the attribute list

	comments:   The behavior of this function is binding-dependent.  This
				implementation places the values in the protocol byte stream.

*****************************************************************************/
HP_StdFuncPrefix HP_AttrXyUInt32(HP_StreamHandleType pStream, HP_UInt32 xValue, HP_UInt32 yValue)
{


	/* Note: all HP_AddToList macros can set the errorCode */

	if (HP_NoErrors(pStream))
	{

		HP_AddToListByte(pStream, (HP_UByte) HP_UInt32Xy);
		if (HP_NoErrors(pStream))
		{
			HP_AddToListInt32(pStream, (HP_UInt32) xValue);
		}
		if (HP_NoErrors(pStream))
		{
			HP_AddToListInt32(pStream, (HP_UInt32) yValue);
		}
	}

}


/*****************************************************************************


	function:   HP_AttrXyUInt32

	purpose:    Add two 32-bit signed integer values to the
				attribute list.  One represents a value corresponding to
				the x-axis and the other corresponds to the y-axis.

	parameters: name                data type / description
				-----------------   -------------------------------------
				xValue              HP_SInt32 / The 32-bit integer x value to
									be added to the attribute list

				yValue              HP_SInt32 / The 32-bit integer y value to
									be added to the attribute list

	comments:   The behavior of this function is binding-dependent.  This
				implementation places the values in the protocol byte stream.

*****************************************************************************/
HP_StdFuncPrefix HP_AttrXySInt32(HP_StreamHandleType pStream, HP_SInt32 xValue, HP_SInt32 yValue)
{

	/* Note: all HP_AddToList macros can set the errorCode */

	if (HP_NoErrors(pStream))
	{

		HP_AddToListByte(pStream, (HP_UByte) HP_SInt32Xy);
		if (HP_NoErrors(pStream))
		{
			HP_AddToListInt32(pStream, (HP_UInt32) xValue);
		}
		if (HP_NoErrors(pStream))
		{
			HP_AddToListInt32(pStream, (HP_UInt32) yValue);
		}

	}

}


/*****************************************************************************


	function:   HP_AttrXyReal32

	purpose:    Add two 32-bit real (floating point) values to the
				attribute list.  One represents a value corresponding to
				the x-axis and the other corresponds to the y-axis.

	parameters: name                data type / description
				-----------------   -------------------------------------
				xValue              HP_Real32 / The 32-bit real x value to
									be added to the attribute list

				yValue              HP_Real32 / The 32-bit real y value to
									be added to the attribute list

	comments:   The behavior of this function is binding-dependent.  This
				implementation places the values in the protocol byte stream.

*****************************************************************************/
HP_StdFuncPrefix HP_AttrXyReal32(HP_StreamHandleType pStream, HP_Real32 xValue, HP_Real32 yValue)
{

	/* Note: all HP_AddToList macros can set the errorCode */

	if (HP_NoErrors(pStream))
	{

		HP_AddToListByte(pStream, (HP_UByte) HP_Real32Xy);
		if (HP_NoErrors(pStream))
		{
			HP_AddToListReal32(pStream, (HP_Real32) xValue);
			if (HP_NoErrors(pStream))
			{
				HP_AddToListReal32(pStream, (HP_Real32) yValue);
			}
		}

	}

}


/*****************************************************************************


	function:   HP_AttrBoxUByte

	purpose:    Add four 8-bit unsigned values to the
				attribute list.  The first two values represent the upper-
				left point of a bounding box (x1, y1).  The last two values
				represent the lower-right point of a bounding box (x2, y2).

	parameters: name                data type / description
				-----------------   -------------------------------------
				x1Value             HP_UByte / The 8-bit integer x1 value to
									be added to the attribute list

				y1Value             HP_UByte / The 8-bit integer y1 value to
									be added to the attribute list

				x2Value             HP_UByte / The 8-bit integer x2 value to
									be added to the attribute list

				y2Value             HP_UByte / The 8-bit integer y2 value to
									be added to the attribute list

	comments:   The behavior of this function is binding-dependent.  This
				implementation places the values in the protocol byte stream.

*****************************************************************************/
HP_StdFuncPrefix HP_AttrBoxUByte(HP_StreamHandleType pStream, HP_UByte x1Value, HP_UByte y1Value,
									HP_UByte x2Value, HP_UByte y2Value)
{

	/* Note: all HP_AddToList macros can set the errorCode */

	if (HP_NoErrors(pStream))
	{

		HP_AddToListByte(pStream, (HP_UByte) HP_UByteBox);
		if (HP_NoErrors(pStream))
		{
			HP_AddToListByte(pStream, (HP_UByte) x1Value);
		}
		if (HP_NoErrors(pStream))
		{
			HP_AddToListByte(pStream, (HP_UByte) y1Value);
		}
		if (HP_NoErrors(pStream))
		{
			HP_AddToListByte(pStream, (HP_UByte) x2Value);
		}
		if (HP_NoErrors(pStream))
		{
			HP_AddToListByte(pStream, (HP_UByte) y2Value);
		}

	}

}


/*****************************************************************************


	function:   HP_AttrBoxUInt16

	purpose:    Add four 16-bit unsigned integer values to the
				attribute list.  The first two values represent the upper-
				left point of a bounding box (x1, y1).  The last two values
				represent the lower-right point of a bounding box (x2, y2).

	parameters: name                data type / description
				-----------------   -------------------------------------
				x1Value             HP_UInt16 / The 16-bit integer x1 value to
									be added to the attribute list

				y1Value             HP_UInt16 / The 16-bit integer y1 value to
									be added to the attribute list

				x2Value             HP_UInt16 / The 16-bit integer x2 value to
									be added to the attribute list

				y2Value             HP_UInt16 / The 16-bit integer y2 value to
									be added to the attribute list

	comments:   The behavior of this function is binding-dependent.  This
				implementation places the values in the protocol byte stream.

*****************************************************************************/
HP_StdFuncPrefix HP_AttrBoxUInt16(HP_StreamHandleType pStream, HP_UInt16 x1Value, HP_UInt16 y1Value,
									HP_UInt16 x2Value, HP_UInt16 y2Value)
{


	/* Note: all HP_AddToList macros can set the errorCode */

	if (HP_NoErrors(pStream))
	{

		HP_AddToListByte(pStream, (HP_UByte) HP_UInt16Box);
		if (HP_NoErrors(pStream))
		{
			HP_AddToListInt16(pStream, (HP_UInt16) x1Value);
		}
		if (HP_NoErrors(pStream))
		{
			HP_AddToListInt16(pStream, (HP_UInt16) y1Value);
		}
		if (HP_NoErrors(pStream))
		{
			HP_AddToListInt16(pStream, (HP_UInt16) x2Value);
		}
		if (HP_NoErrors(pStream))
		{
			HP_AddToListInt16(pStream, (HP_UInt16) y2Value);
		}

	}

}

/*****************************************************************************


	function:   HP_AttrBoxSInt16

	purpose:    Add four 16-bit signed integer values to the
				attribute list.  The first two values represent the upper-
				left point of a bounding box (x1, y1).  The last two values
				represent the lower-right point of a bounding box (x2, y2).

	parameters: name                data type / description
				-----------------   -------------------------------------
				x1Value             HP_SInt16 / The 16-bit integer x1 value to
									be added to the attribute list

				y1Value             HP_SInt16 / The 16-bit integer y1 value to
									be added to the attribute list

				x2Value             HP_SInt16 / The 16-bit integer x2 value to
									be added to the attribute list

				y2Value             HP_SInt16 / The 16-bit integer y2 value to
									be added to the attribute list

	comments:   The behavior of this function is binding-dependent.  This
				implementation places the values in the protocol byte stream.

*****************************************************************************/
HP_StdFuncPrefix HP_AttrBoxSInt16(HP_StreamHandleType pStream, HP_SInt16 x1Value, HP_SInt16 y1Value,
									HP_SInt16 x2Value, HP_SInt16 y2Value)
{


	/* Note: all HP_AddToList macros can set the errorCode */

	if (HP_NoErrors(pStream))
	{

		HP_AddToListByte(pStream, (HP_UByte) HP_SInt16Box);
		if (HP_NoErrors(pStream))
		{
			HP_AddToListInt16(pStream, (HP_UInt16) x1Value);
		}
		if (HP_NoErrors(pStream))
		{
			HP_AddToListInt16(pStream, (HP_UInt16) y1Value);
		}
		if (HP_NoErrors(pStream))
		{
			HP_AddToListInt16(pStream, (HP_UInt16) x2Value);
		}
		if (HP_NoErrors(pStream))
		{
			HP_AddToListInt16(pStream, (HP_UInt16) y2Value);
		}

	}

}


/*****************************************************************************


	function:   HP_AttrBoxUInt32

	purpose:    Add four 32-bit unsigned integer values to the
				attribute list.  The first two values represent the upper-
				left point of a bounding box (x1, y1).  The last two values
				represent the lower-right point of a bounding box (x2, y2).

	parameters: name                data type / description
				-----------------   -------------------------------------
				x1Value             HP_UInt32 / The 32-bit integer x1 value to
									be added to the attribute list

				y1Value             HP_UInt32 / The 32-bit integer y1 value to
									be added to the attribute list

				x2Value             HP_UInt32 / The 32-bit integer x2 value to
									be added to the attribute list

				y2Value             HP_UInt32 / The 32-bit integer y2 value to
									be added to the attribute list

	comments:   The behavior of this function is binding-dependent.  This
				implementation places the values in the protocol byte stream.

*****************************************************************************/
HP_StdFuncPrefix HP_AttrBoxUInt32(HP_StreamHandleType pStream, HP_UInt32 x1Value, HP_UInt32 y1Value,
									HP_UInt32 x2Value, HP_UInt32 y2Value)
{

	/* Note: all HP_AddToList macros can set the errorCode */

	if (HP_NoErrors(pStream))
	{

		HP_AddToListByte(pStream, (HP_UByte) HP_UInt32Box);
		if (HP_NoErrors(pStream))
		{
			HP_AddToListInt32(pStream, (HP_UInt32) x1Value);
		}
		if (HP_NoErrors(pStream))
		{
			HP_AddToListInt32(pStream, (HP_UInt32) y1Value);
		}
		if (HP_NoErrors(pStream))
		{
			HP_AddToListInt32(pStream, (HP_UInt32) x2Value);
		}
		if (HP_NoErrors(pStream))
		{
			HP_AddToListInt32(pStream, (HP_UInt32) y2Value);
		}

	}

}

/*****************************************************************************


	function:   HP_AttrBoxSInt32

	purpose:    Add four 32-bit signed integer values to the
				attribute list.  The first two values represent the upper-
				left point of a bounding box (x1, y1).  The last two values
				represent the lower-right point of a bounding box (x2, y2).

	parameters: name                data type / description
				-----------------   -------------------------------------
				x1Value             HP_SInt32 / The 32-bit integer x1 value to
									be added to the attribute list

				y1Value             HP_SInt32 / The 32-bit integer y1 value to
									be added to the attribute list

				x2Value             HP_SInt32 / The 32-bit integer x2 value to
									be added to the attribute list

				y2Value             HP_SInt32 / The 32-bit integer y2 value to
									be added to the attribute list

	comments:   The behavior of this function is binding-dependent.  This
				implementation places the values in the protocol byte stream.

*****************************************************************************/
HP_StdFuncPrefix HP_AttrBoxSInt32(HP_StreamHandleType pStream, HP_SInt32 x1Value, HP_SInt32 y1Value,
									HP_SInt32 x2Value, HP_SInt32 y2Value)
{


	/* Note: all HP_AddToList macros can set the errorCode */

	if (HP_NoErrors(pStream))
	{

		HP_AddToListByte(pStream, (HP_UByte) HP_SInt32Box);
		if (HP_NoErrors(pStream))
		{
			HP_AddToListInt32(pStream, (HP_UInt32) x1Value);
		}
		if (HP_NoErrors(pStream))
		{
			HP_AddToListInt32(pStream, (HP_UInt32) y1Value);
		}
		if (HP_NoErrors(pStream))
		{
			HP_AddToListInt32(pStream, (HP_UInt32) x2Value);
		}
		if (HP_NoErrors(pStream))
		{
			HP_AddToListInt32(pStream, (HP_UInt32) y2Value);
		}

	}
}


/*****************************************************************************


	function:   HP_AttrBoxReal32

	purpose:    Add four 32-bit real (floating point) values to the
				attribute list.  The first two values represent the upper-
				left point of a bounding box (x1, y1).  The last two values
				represent the lower-right point of a bounding box (x2, y2).

	parameters: name                data type / description
				-----------------   -------------------------------------
				x1Value             HP_Real32 / The 32-bit integer x1 value
									to be added to the attribute list

				y1Value             HP_Real32 / The 32-bit integer y1 value
									to be added to the attribute list

				x2Value             HP_Real32 / The 32-bit integer x2 value
									to be added to the attribute list

				y2Value             HP_Real32 / The 32-bit integer y2 value
									to be added to the attribute list

	comments:   The behavior of this function is binding-dependent.  This
				implementation places the values in the protocol byte stream.

*****************************************************************************/
HP_StdFuncPrefix HP_AttrBoxReal32(HP_StreamHandleType pStream, HP_Real32 x1Value, HP_Real32 y1Value,
						HP_Real32 x2Value, HP_Real32 y2Value)
{

	/* Note: all HP_AddToList macros can set the errorCode */

	if (HP_NoErrors(pStream))
	{

		HP_AddToListByte(pStream, (HP_UByte) HP_Real32Box);

		if (HP_NoErrors(pStream))
		{
			HP_AddToListReal32(pStream, (HP_Real32) x1Value);
		}
		if (HP_NoErrors(pStream))
		{
			HP_AddToListReal32(pStream, (HP_Real32) y1Value);
		}
		if (HP_NoErrors(pStream))
		{
			HP_AddToListReal32(pStream, (HP_Real32) x2Value);
		}
		if (HP_NoErrors(pStream))
		{
			HP_AddToListReal32(pStream, (HP_Real32) y2Value);
		}

	}

}


/*****************************************************************************


	function:   HP_AttrUByteArray

	purpose:    Add 8-bit unsigned values to the
				attribute list in the form of an array

	parameters: name                data type / description
				-----------------   -------------------------------------
				array               HP_UByte* / Pointer to the array of
										8-bit values.

				arrayLen            HP_UInt32 / The number of elements in
										the array.

	comments:   The behavior of this function is binding-dependent.  This
				implementation places the values in the protocol byte stream.

*****************************************************************************/
HP_StdFuncPrefix HP_AttrUByteArray(HP_StreamHandleType pStream, HP_pUByte array, HP_UInt32 arrayLen)
{

	/* Note: all HP_AddToList macros can set the errorCode */

	if (HP_NoErrors(pStream))
	{

		HP_AddToListByte(pStream, (HP_UByte) HP_UByteArray);
		if (HP_NoErrors(pStream))
		{
			HP_UInt32 index = 0;

			HP_AddToListByte(pStream, (HP_UByte) HP_UInt16Data);
			if (HP_NoErrors(pStream)) HP_AddToListInt16(pStream, (HP_UInt16) arrayLen);
			while ((HP_NoErrors(pStream)) && (index < arrayLen))
			{
				HP_AddToListByte(pStream, (HP_UByte) array[index++]);
			}
		}

	}

}

/*****************************************************************************


	function:   HP_AttrString

	purpose:    Add 8-bit unsigned values from a string to the
				attribute list in the form of an array

	parameters: name                data type / description
				-----------------   -------------------------------------
				array               HP_UByte* / Pointer to the string.

				arrayLen            HP_UInt32 / The number of characters in
										the string.

	comments:   The behavior of this function is binding-dependent.  This
				implementation places the values in the protocol byte stream.

*****************************************************************************/
HP_StdFuncPrefix HP_AttrString(HP_StreamHandleType pStream, HP_pUByte array, HP_UInt32 arrayLen)
{

	/* Note: all HP_AddToList macros can set the errorCode */

    if (HP_NoErrors(pStream))
        {
        HP_AddToListByte(pStream, (HP_UByte) HP_String);
        if (HP_NoErrors(pStream))
            {
            HP_UInt32 index = 0;

            HP_AddToListByte(pStream, (HP_UByte) HP_UInt16Data);
            if (HP_NoErrors(pStream))
	            HP_AddToListInt16(pStream, (HP_UInt16) arrayLen);
            while ((HP_NoErrors(pStream)) && (index < arrayLen))
                {
                HP_AddToListByte(pStream, (HP_UByte) array[index++]);
                }
            }
        }

}

/*****************************************************************************


	function:   HP_AttrUInt16Array

	purpose:    Add 16-bit unsigned integer values to the
				attribute list in the form of an array

	parameters: name                data type / description
				-----------------   -------------------------------------
				array               HP_UInt16* / Pointer to the array of
										16-bit values.

                arrayLen            HP_UInt32/ The number of elements in
                                        the array.

    comments:   The behavior of this function is binding-dependent.  This 
                implementation places the values in the protocol byte stream.
                
*****************************************************************************/
HP_StdFuncPrefix HP_AttrUInt16Array(HP_StreamHandleType pStream, HP_pUInt16 array, HP_UInt32 arrayLen)
{

    /* Note: all HP_AddToList macros can set the errorCode */
    
    if (HP_NoErrors(pStream))
    {
    
        HP_AddToListByte(pStream, (HP_UByte) HP_UInt16Array);
        if (HP_NoErrors(pStream))                       
        {
            HP_UInt32 index = 0; 
            
            HP_AddToListByte(pStream, (HP_UByte) HP_UInt16Data);
            if (HP_NoErrors(pStream)) HP_AddToListInt16(pStream, (HP_UInt16) arrayLen);
            while ((HP_NoErrors(pStream)) && (index < arrayLen))
            {
                HP_AddToListInt16(pStream, (HP_UInt16) array[index++]);
            }
        }

    }   
    
}


/*****************************************************************************


    function:   HP_AttrUInt16ToByteArray

    purpose:    Add 16-bit unsigned integer values to the
                attribute list in the form of a byte array

    parameters: name                data type / description
                -----------------   -------------------------------------
                array               HP_UInt16* / Pointer to the array of
                                        16-bit values.
                                        
                arrayLen            HP_UInt32/ The number of elements in
                                        the array.  
                                    
	comments:   The behavior of this function is binding-dependent.  This
                implementation places the values in the protocol byte stream.
                
*****************************************************************************/
HP_StdFuncPrefix HP_AttrUInt16ToByteArray(HP_StreamHandleType pStream, HP_pUInt16 array, HP_UInt32 arrayLen)
{
    
    /* Note: all HP_AddToList macros can set the errorCode */
    
    if (HP_NoErrors(pStream))
	{

        HP_AddToListByte(pStream, (HP_UByte) HP_UByteArray);
		if (HP_NoErrors(pStream))
        {
            HP_UInt32 index = 0; 
            
            HP_AddToListByte(pStream, (HP_UByte) HP_UInt16Data);
            if (HP_NoErrors(pStream)) HP_AddToListInt16(pStream, (HP_UInt16) arrayLen);
            while ((HP_NoErrors(pStream)) && (index < arrayLen))
            {
                HP_AddToListByte(pStream, (HP_UByte) array[index++]);
            }
        }

    }   
    
}

/*****************************************************************************


    function:   HP_AttrSInt16Array

    purpose:    Add 16-bit signed integer values to the
                attribute list in the form of an array

    parameters: name                data type / description
                -----------------   -------------------------------------
                array               HP_UInt16* / Pointer to the array of
                                        16-bit values.
                                        
                arrayLen            HP_UInt32/ The number of elements in
                                        the array.  
                                    
    comments:   The behavior of this function is binding-dependent.  This 
                implementation places the values in the protocol byte stream.
                
*****************************************************************************/
HP_StdFuncPrefix HP_AttrSInt16Array(HP_StreamHandleType pStream, HP_pSInt16 array, HP_UInt32 arrayLen)
{

    /* Note: all HP_AddToList macros can set the errorCode */
    
    if (HP_NoErrors(pStream))
    {
    
        HP_AddToListByte(pStream, (HP_UByte) HP_SInt16Array);
        if (HP_NoErrors(pStream))                       
        {
            HP_UInt32 index = 0; 

			HP_AddToListByte(pStream, (HP_UByte) HP_UInt16Data);
            if (HP_NoErrors(pStream)) HP_AddToListInt16(pStream, (HP_UInt16) arrayLen);
            while ((HP_NoErrors(pStream)) && (index < arrayLen))
            {
                HP_AddToListInt16(pStream, (HP_UInt16) array[index++]);
            }
        }

    }   
    
}

/*****************************************************************************


    function:   HP_AttrSInt16ToByteArray

    purpose:    Add 16-bit signed integer values to the
                attribute list in the form of a byte array

    parameters: name                data type / description
                -----------------   -------------------------------------
				array               HP_UInt16* / Pointer to the array of
                                        16-bit values.
                                        
                arrayLen            HP_UInt32/ The number of elements in
                                        the array.  
                                    
    comments:   The behavior of this function is binding-dependent.  This 
                implementation places the values in the protocol byte stream.
                
*****************************************************************************/
HP_StdFuncPrefix HP_AttrSInt16ToByteArray(HP_StreamHandleType pStream, HP_pSInt16 array, HP_UInt32 arrayLen)
{
    
    /* Note: all HP_AddToList macros can set the errorCode */
    
    if (HP_NoErrors(pStream))
    {
    
		HP_AddToListByte(pStream, (HP_UByte) HP_UByteArray);
        if (HP_NoErrors(pStream))                       
        {
            HP_UInt32 index = 0; 
            
            HP_AddToListByte(pStream, (HP_UByte) HP_UInt16Data);
            if (HP_NoErrors(pStream)) HP_AddToListInt16(pStream, (HP_UInt16) arrayLen);
            while ((HP_NoErrors(pStream)) && (index < arrayLen))
            {
                HP_AddToListByte(pStream, (HP_UByte) array[index++]);
			}
		}

    }   
    
}

/*****************************************************************************


    function:   HP_AttrUInt32Array

    purpose:    Add 32-bit unsigned integer values to the
                attribute list in the form of an array

    parameters: name                data type / description
                -----------------   -------------------------------------
                array               HP_UInt32* / Pointer to the array of
                                        32-bit values.
                                        
                arrayLen            HP_UInt32 / The number of elements in
                                        the array.  
                                    
	comments:   The behavior of this function is binding-dependent.  This
                implementation places the values in the protocol byte stream.
                
*****************************************************************************/
HP_StdFuncPrefix HP_AttrUInt32Array(HP_StreamHandleType pStream, HP_pUInt32 array, HP_UInt32 arrayLen)
{
    
    /* Note: all HP_AddToList macros can set the errorCode */
    
    if (HP_NoErrors(pStream))
    {
    
        HP_AddToListByte(pStream, (HP_UByte) HP_UInt32Array);
        if (HP_NoErrors(pStream))                       
        {
            HP_UInt32 index = 0; 
            
            HP_AddToListByte(pStream, (HP_UByte) HP_UInt16Data);
			if (HP_NoErrors(pStream)) HP_AddToListInt16(pStream, (HP_UInt16) arrayLen);
            while ((HP_NoErrors(pStream)) && (index < arrayLen))
            {
                HP_AddToListInt32(pStream, (HP_UInt32) array[index++]);
            }
        }

	}
    
}

/*****************************************************************************


    function:   HP_AttrSInt32Array

    purpose:    Add 32-bit signed integer values to the
                attribute list in the form of an array

    parameters: name                data type / description
                -----------------   -------------------------------------
                array               HP_UInt32* / Pointer to the array of
                                        32-bit values.
                                        
                arrayLen            HP_UInt32 / The number of elements in
                                        the array.  
                                    
    comments:   The behavior of this function is binding-dependent.  This 
                implementation places the values in the protocol byte stream.
                
*****************************************************************************/
HP_StdFuncPrefix HP_AttrSInt32Array(HP_StreamHandleType pStream, HP_pSInt32 array, HP_UInt32 arrayLen)
{

    /* Note: all HP_AddToList macros can set the errorCode */
    
    if (HP_NoErrors(pStream))
    {
    
		HP_AddToListByte(pStream, (HP_UByte) HP_SInt32Array);
        if (HP_NoErrors(pStream))                       
        {
            HP_UInt32 index = 0; 
        
            HP_AddToListByte(pStream, (HP_UByte) HP_UInt16Data);
            if (HP_NoErrors(pStream)) HP_AddToListInt16(pStream, (HP_UInt16) arrayLen);
            while ((HP_NoErrors(pStream)) && (index < arrayLen))
            {
                HP_AddToListInt32(pStream, (HP_UInt32) array[index++]);
            }
        }

    }   
    
}
 
/*****************************************************************************


	function:   HP_AttrReal32Array

	purpose:    Add 32-bit real (floating point) values to the
				attribute list in the form of an array

	parameters: name                data type / description
				-----------------   -------------------------------------
				array               HP_Real32* / Pointer to the array of
										32-bit real values.

				arrayLen            HP_UInt32 / The number of elements in
										the array.

	comments:   The behavior of this function is binding-dependent.  This
				implementation places the values in the protocol byte stream.

*****************************************************************************/
HP_StdFuncPrefix HP_AttrReal32Array(HP_StreamHandleType pStream, HP_pReal32 array, HP_UInt32 arrayLen)
{

	/* Note: all HP_AddToList macros can set the errorCode */

	if (HP_NoErrors(pStream))
	{

		HP_AddToListByte(pStream, (HP_UByte) HP_Real32Array);

		if (HP_NoErrors(pStream))
		{
			HP_UInt32 index = 0;

			HP_AddToListByte(pStream, (HP_UByte) HP_UInt16Data);
			if (HP_NoErrors(pStream)) HP_AddToListInt16(pStream, (HP_UInt16) arrayLen);
			while ((HP_NoErrors(pStream)) && (index < arrayLen))
			{
				HP_AddToListReal32(pStream, (HP_Real32) array[index++]);
			}
		}

	}

}


/*****************************************************************************


	function:   HP_EmbeddedDataPrefix

	purpose:    Add length of embedded data to data stream.  Raw
				data should follow. Automatically choose a byte or
				32-bit embedded data tag based on the dataLength passed
				in by the user.

	parameters: name                data type / description
				-----------------   -------------------------------------
				dataLength          HP_UInt32 / The number of bytes of
										the data that follows.

	comments:   The behavior of this function is binding-dependent.  This
				implementation places the values in the protocol byte stream.

*****************************************************************************/
HP_StdFuncPrefix HP_EmbeddedDataPrefix(HP_StreamHandleType pStream, HP_UInt32 dataLength)
{
	/* Note: all HP_AddToList macros can set the errorCode */

	if (HP_NoErrors(pStream))
	{
		if (dataLength < 256)
		{
			HP_AddToListByte(pStream, (HP_UByte) HP_EmbeddedDataByte); /* embedded data byte tag */
			if (HP_NoErrors(pStream)) HP_AddToListByte(pStream, (HP_UByte) dataLength);
		}
		else
		{
			HP_AddToListByte(pStream, (HP_UByte) HP_EmbeddedData); /* embedded data tag */
			if (HP_NoErrors(pStream)) HP_AddToListInt32(pStream, (HP_UInt32) dataLength);
		}
	}
}
/*****************************************************************************
	function:   HP_EmbeddedDataPrefix32

	purpose:    Add length of embedded data to data stream.  Raw
				data should follow.  Use a 32-bit datalength tag.

	parameters: name                data type / description
				-----------------   -------------------------------------
				dataLength          HP_UInt32 / The number of bytes of
										the data that follows.

	comments:   The behavior of this function is binding-dependent.  This
				implementation places the values in the protocol byte stream.
*****************************************************************************/
HP_StdFuncPrefix HP_EmbeddedDataPrefix32(HP_StreamHandleType pStream, HP_UInt32 dataLength)
{
	/* Note: all HP_AddToList macros can set the errorCode */
	if (HP_NoErrors(pStream))
	{
		HP_AddToListByte(pStream, (HP_UByte) HP_EmbeddedData); /* embedded data tag */
		if (HP_NoErrors(pStream)) HP_AddToListInt32(pStream, (HP_UInt32) dataLength);
	}
}

/*****************************************************************************


	function:   HP_DataUByteArray

	purpose:    Add 8-bit unsigned values to the data source
				from an array as embedded data

    parameters: name                data type / description
                -----------------   -------------------------------------
                array               HP_UByte* / Pointer to the array of
                                        8-bit values.

                arrayLen            HP_UInt32 / The number of bytes in
                                        the array.  
                                        
    comments:   The behavior of this function is binding-dependent.  This 
                implementation places the values in the protocol byte stream.
                
*****************************************************************************/
HP_StdFuncPrefix HP_DataUByteArray(HP_StreamHandleType pStream, HP_pUByte array, HP_UInt32 arrayLen)
{


	HP_UInt32 lengthInBytes, index = 0;

	/* Note: all HP_AddToList macros can set the errorCode */

	if (HP_NoErrors(pStream))
	{

		lengthInBytes = arrayLen;

		HP_EmbeddedDataPrefix32(pStream, (HP_UInt32) lengthInBytes);
		while ((HP_NoErrors(pStream)) && (index < arrayLen))
		{
			HP_AddToListByte(pStream, (HP_UByte) array[index++]);
		}

	}

}


/*****************************************************************************


	function:   HP_DataUInt16Array

	purpose:    Add 16-bit unsigned integer values to the data source
				from an array as embedded data

    parameters: name                data type / description
                -----------------   -------------------------------------
                array               HP_UInt16* / Pointer to the array of
                                        16-bit values.
                                        
                arrayLen            HP_UInt32 / The number of integers in
                                        the array.  
                                        
    comments:   The behavior of this function is binding-dependent.  This 
                implementation places the values in the protocol byte stream.
                
*****************************************************************************/
HP_StdFuncPrefix HP_DataUInt16Array(HP_StreamHandleType pStream, HP_pUInt16 array, HP_UInt32 arrayLen)
{
    
    
    HP_UInt32 lengthInBytes, index = 0; 
        
    /* Note: all HP_AddToList macros can set the errorCode */
    
    if (HP_NoErrors(pStream))
    {
    
        lengthInBytes = arrayLen * 2;

		HP_EmbeddedDataPrefix32(pStream, (HP_UInt32) lengthInBytes);
		while ((HP_NoErrors(pStream)) && (index < arrayLen))
		{
			HP_AddToListInt16(pStream, (HP_UInt16) array[index++]);
		}

	}

}



/*****************************************************************************


	function:   HP_DataUInt16ArrayMSB

    purpose:    Add 16-bit unsigned integer values to the data source 
                from an array as embedded data.  Most significant byte
                first.

    parameters: name                data type / description
                -----------------   -------------------------------------
                array               HP_UInt16* / Pointer to the array of
										16-bit values.

                arrayLen            HP_UInt32 / The number of integers in
                                        the array.  
                                        
    comments:   The behavior of this function is binding-dependent.  This 
                implementation places the values in the protocol byte stream.
                
*****************************************************************************/
HP_StdFuncPrefix HP_DataUInt16ArrayMSB(HP_StreamHandleType pStream, HP_pUInt16 array, HP_UInt32 arrayLen)
{
    
    
	HP_UInt32 lengthInBytes, index = 0;
        
    /* Note: all HP_AddToList macros can set the errorCode */
    
    if (HP_NoErrors(pStream))
    {
    
        lengthInBytes = arrayLen * 2;
        
		HP_EmbeddedDataPrefix32(pStream, (HP_UInt32) lengthInBytes);
		while ((HP_NoErrors(pStream)) && (index < arrayLen))
		{
			HP_AddToListInt16MSB(pStream, (HP_UInt16) array[index++]);
		}

    }   
    
}

/*****************************************************************************


    function:   HP_DataUInt32Array

    purpose:    Add 32-bit unsigned integer values to the data source 
                from an array as embedded data

    parameters: name                data type / description
				-----------------   -------------------------------------
				array               HP_UInt32* / Pointer to the array of
                                        32-bit values.
                                        
                arrayLen            HP_UInt32 / The number of integers in
                                        the array.  
                                        
    comments:   The behavior of this function is binding-dependent.  This 
                implementation places the values in the protocol byte stream.
                
*****************************************************************************/
HP_StdFuncPrefix HP_DataUInt32Array(HP_StreamHandleType pStream, HP_pUInt32 array, HP_UInt32 arrayLen)
{
    
    
    HP_UInt32 lengthInBytes, index = 0; 
        
    /* Note: all HP_AddToList macros can set the errorCode */
    
    if (HP_NoErrors(pStream))
    {
    
        lengthInBytes = arrayLen * 4;

		HP_EmbeddedDataPrefix32(pStream, (HP_UInt32) lengthInBytes);
		while ((HP_NoErrors(pStream)) && (index < arrayLen))
		{
			HP_AddToListInt32(pStream, (HP_UInt32) array[index++]);
		}

	}


}

/*****************************************************************************


    function:   HP_DataUInt32ArrayMSB

    purpose:    Add 32-bit unsigned integer values to the data source 
                from an array as embedded data.  Most significant byte
                first.

    parameters: name                data type / description
                -----------------   -------------------------------------
                array               HP_UInt32* / Pointer to the array of
                                        32-bit values.
                                        
                arrayLen            HP_UInt32 / The number of integers in
                                        the array.  

	comments:   The behavior of this function is binding-dependent.  This
                implementation places the values in the protocol byte stream.
                
*****************************************************************************/
HP_StdFuncPrefix HP_DataUInt32ArrayMSB(HP_StreamHandleType pStream, HP_pUInt32 array, HP_UInt32 arrayLen)
{
    
    
    HP_UInt32 lengthInBytes, index = 0; 

	/* Note: all HP_AddToList macros can set the errorCode */
    
    if (HP_NoErrors(pStream))
    {
    
        lengthInBytes = arrayLen * 4;

		HP_EmbeddedDataPrefix32(pStream, (HP_UInt32) lengthInBytes);
		while ((HP_NoErrors(pStream)) && (index < arrayLen))
		{
			HP_AddToListInt32MSB(pStream, (HP_UInt32) array[index++]);
		}

	}


}

/*****************************************************************************


    function:   HP_DataReal32Array

    purpose:    Add 32-bit real values to the data source 
                from an array as embedded data

    parameters: name                data type / description
                -----------------   -------------------------------------
                array               HP_Real32* / Pointer to the array of
                                        real 32-bit values.
                                        
                arrayLen            HP_UInt32 / The number of reals in
                                        the array.  
                                        
    comments:   The behavior of this function is binding-dependent.  This 
                implementation places the values in the protocol byte stream.
                
*****************************************************************************/
HP_StdFuncPrefix HP_DataReal32Array(HP_StreamHandleType pStream, HP_pReal32 array, HP_UInt32 arrayLen)
{


    HP_UInt32 lengthInBytes, index = 0; 
        
    /* Note: all HP_AddToList macros can set the errorCode */
    
    if (HP_NoErrors(pStream))
    {
    
        lengthInBytes = arrayLen * 4;

		HP_EmbeddedDataPrefix32(pStream, (HP_UInt32) lengthInBytes);
		while ((HP_NoErrors(pStream)) && (index < arrayLen))
		{
			HP_AddToListReal32(pStream, (HP_Real32) array[index++]);
		}

	}


}
                        
/*****************************************************************************


    function:   HP_DataReal32ArrayMSB

    purpose:    Add 32-bit real values to the data source 
                from an array as embedded data in Most Significant
                Byte First format

    parameters: name                data type / description
                -----------------   -------------------------------------
                array               HP_Real32* / Pointer to the array of
                                        real 32-bit values.
                                        
                arrayLen            HP_UInt32 / The number of reals in
                                        the array.  
                                        
    comments:   The behavior of this function is binding-dependent.  This 
                implementation places the values in the protocol byte stream.
                
*****************************************************************************/
HP_StdFuncPrefix HP_DataReal32ArrayMSB(HP_StreamHandleType pStream, HP_pReal32 array, HP_UInt32 arrayLen)
{
    
    
    HP_UInt32 lengthInBytes, index = 0; 
        
    /* Note: all HP_AddToList macros can set the errorCode */
    
	if (HP_NoErrors(pStream))
	{
    
        lengthInBytes = arrayLen * 4;

		HP_EmbeddedDataPrefix32(pStream, (HP_UInt32) lengthInBytes);
		while ((HP_NoErrors(pStream)) && (index < arrayLen))
		{
			HP_AddToListReal32MSB(pStream, (HP_Real32) array[index++]);
		}

	}


}


/*****************************************************************************


	function:   HP_RawUByte

	purpose:    Add an raw 8-bit unsigned value to the data stream

	parameters: name                data type / description
				-----------------   -------------------------------------
				intValue            HP_UByte / The 8-bit integer value to be
									added to the data source

	comments:   The behavior of this function is binding-dependent.  This
				implementation places the value in the protocol byte stream.

*****************************************************************************/
HP_StdFuncPrefix HP_RawUByte(HP_StreamHandleType pStream, HP_UByte intValue)
{

	/* Note: all HP_AddToList macros can set the errorCode */

	if (HP_NoErrors(pStream))
	{

		HP_AddToListByte(pStream, (HP_UByte) intValue);

	}

}


/*****************************************************************************


	function:   HP_RawUInt16

	purpose:    Add a raw 16-bit unsigned integer value to the data stream

	parameters: name                data type / description
                -----------------   -------------------------------------
                intValue            HP_UInt16 / The 16-bit integer value to be
                                    added to the data stream

    comments:   The behavior of this function is binding-dependent.  This 
				implementation places the value in the protocol byte stream.

*****************************************************************************/
HP_StdFuncPrefix HP_RawUInt16(HP_StreamHandleType pStream, HP_UInt16 intValue)
{
    
    /* Note: all HP_AddToList macros can set the errorCode */
    
    if (HP_NoErrors(pStream))
    {
    
        HP_AddToListInt16(pStream, (HP_UInt16) intValue);
    
	}
    
}
 
/*****************************************************************************


    function:   HP_RawUInt16MSB

    purpose:    Add a raw 16-bit unsigned integer value to the data stream

    parameters: name                data type / description
                -----------------   -------------------------------------
                intValue            HP_UInt16 / The 16-bit integer value to be
                                    added to the data stream in Most 
                                    Byte First format

    comments:   The behavior of this function is binding-dependent.  This 
                implementation places the value in the protocol byte stream.
                
*****************************************************************************/
HP_StdFuncPrefix HP_RawUInt16MSB(HP_StreamHandleType pStream, HP_UInt16 intValue)
{
    
    /* Note: all HP_AddToList macros can set the errorCode */
    
    if (HP_NoErrors(pStream))
    {

		HP_AddToListInt16MSB(pStream, (HP_UInt16) intValue);
    
    }
    
}
 

/*****************************************************************************


	function:   HP_RawUInt32

    purpose:    Add a raw 32-bit unsigned integer value to the data stream

    parameters: name                data type / description
                -----------------   -------------------------------------
                intValue            HP_UInt32 / The 32-bit integer value to be
                                    added to the data stream

    comments:   The behavior of this function is binding-dependent.  This 
                implementation places the value in the protocol byte stream.
                
*****************************************************************************/
HP_StdFuncPrefix HP_RawUInt32(HP_StreamHandleType pStream, HP_UInt32 intValue)
{
    
    /* Note: all HP_AddToList macros can set the errorCode */
    
    if (HP_NoErrors(pStream))
    {
    
        HP_AddToListInt32(pStream, (HP_UInt32) intValue);
    
    }
    
}
 
/*****************************************************************************


    function:   HP_RawUInt32MSB

    purpose:    Add a raw 32-bit unsigned integer value to the data stream

    parameters: name                data type / description
                -----------------   -------------------------------------
                intValue            HP_UInt32 / The 32-bit integer value to be
                                    added to the data stream in Most 
                                    Significant Byte First format

	comments:   The behavior of this function is binding-dependent.  This
				implementation places the value in the protocol byte stream.
                
*****************************************************************************/
HP_StdFuncPrefix HP_RawUInt32MSB(HP_StreamHandleType pStream, HP_UInt32 intValue)
{
    
    /* Note: all HP_AddToList macros can set the errorCode */
    
    if (HP_NoErrors(pStream))
	{

        HP_AddToListInt32MSB(pStream, (HP_UInt32) intValue);
    
    }
    
}
 
/*****************************************************************************


    function:   HP_RawReal32

	purpose:    Add a raw 32-bit real value to the data stream

    parameters: name                data type / description
                -----------------   -------------------------------------
                intValue            HP_Real32 / The 32-bit real value to be
                                    added to the data stream
                                    
    comments:   The behavior of this function is binding-dependent.  This
                implementation places the value in the protocol byte stream.

*****************************************************************************/
HP_StdFuncPrefix HP_RawReal32(HP_StreamHandleType pStream, HP_Real32 realValue)
{

    /* Note: all HP_AddToList macros can set the errorCode */

    if (HP_NoErrors(pStream))
    {

        HP_AddToListReal32(pStream, (HP_Real32) realValue);

    }

}

/*****************************************************************************


	function:   HP_RawReal32MSB

    purpose:    Add a raw 32-bit real value to the data stream

    parameters: name                data type / description
                -----------------   -------------------------------------
                intValue            HP_Real32 / The 32-bit real value to be
                                    added to the data stream in Most
                                    Significant Byte First format

	comments:   The behavior of this function is binding-dependent.  This
				implementation places the value in the protocol byte stream.

*****************************************************************************/
HP_StdFuncPrefix HP_RawReal32MSB(HP_StreamHandleType pStream, HP_Real32 realValue)
{

    /* Note: all HP_AddToList macros can set the errorCode */

    if (HP_NoErrors(pStream))
    {

        HP_AddToListReal32MSB(pStream, (HP_Real32) realValue);

    }

}

/*****************************************************************************


	function:   HP_RawUByteArray

	purpose:    Add raw 8-bit unsigned values to the data stream
				from an array

	parameters: name                data type / description
				-----------------   -------------------------------------
				array               HP_UByte* / Pointer to the array of
										8-bit values.

				arrayLen            HP_UInt32 / The number of bytes in
										the array.

	comments:   The behavior of this function is binding-dependent.  This
				implementation places the values in the protocol byte stream.

*****************************************************************************/
HP_StdFuncPrefix HP_RawUByteArray(HP_StreamHandleType pStream, HP_pUByte array, HP_UInt32 arrayLen)
{


	HP_UInt32 index = 0;

	/* Note: all HP_AddToList macros can set the errorCode */

	if (HP_NoErrors(pStream))
	{

		while ((HP_NoErrors(pStream)) && (index < arrayLen))
		{
			HP_AddToListByte(pStream, (HP_UByte) array[index++]);
		}

	}

}

/*****************************************************************************


    function:   HP_RawUInt16ToByteArray

    purpose:    Add raw 8-bit unsigned integer values to the data stream 
				from a unsigned 16-bit array

    parameters: name                data type / description
                -----------------   -------------------------------------
                array               HP_UInt16* / Pointer to the array of
                                        16-bit values.
                                        
                arrayLen            HP_UInt32 / The number of integers in
                                        the array.  
                                        
    comments:   The behavior of this function is binding-dependent.  This 
                implementation places the values in the protocol byte stream.
                
*****************************************************************************/
HP_StdFuncPrefix HP_RawUInt16ToByteArray(HP_StreamHandleType pStream, HP_pUInt16 array, HP_UInt32 arrayLen)
{
    
    
    HP_UInt32 index = 0; 
        
    /* Note: all HP_AddToList macros can set the errorCode */
    
    if (HP_NoErrors(pStream))
    {
    
        while ((HP_NoErrors(pStream)) && (index < arrayLen))
        {
            HP_AddToListByte(pStream, (HP_UByte) array[index++]);
		}

    }   
    
}

/*****************************************************************************


    function:   HP_RawHugeUByteArray

	purpose:    Add raw 8-bit unsigned values to the data stream
                from an array using a huge pointer for DOS or
                Windows.

    parameters: name                data type / description
                -----------------   -------------------------------------
				array               HP_pCharHuge / Pointer to the array of
                                        8-bit values.
                                        
                arrayLen            HP_UInt32 / The number of bytes in
                                        the array.  
                                        
	comments:   The behavior of this function is binding-dependent.  This
                implementation places the values in the protocol byte stream.
                
*****************************************************************************/
HP_StdFuncPrefix HP_RawHugeUByteArray(HP_StreamHandleType pStream, HP_pCharHuge array, HP_UInt32 arrayLen)
{
    
    
    HP_UInt32 index = 0; 
        
    /* Note: all HP_AddToList macros can set the errorCode */
    
    if (HP_NoErrors(pStream))
    {
    
        while ((HP_NoErrors(pStream)) && (index < arrayLen))
        {
            HP_AddToListByte(pStream, (HP_UByte) array[index++]);
        }

    }
    
}

                        
/*****************************************************************************


	function:   HP_RawUInt16Array

    purpose:    Add raw 16-bit unsigned integer values to the data stream 
                from an array

    parameters: name                data type / description
                -----------------   -------------------------------------
                array               HP_UInt16* / Pointer to the array of
                                        16-bit values.
                                        
				arrayLen            HP_UInt32 / The number of integers in
										the array.
                                        
    comments:   The behavior of this function is binding-dependent.  This 
                implementation places the values in the protocol byte stream.
                
*****************************************************************************/
HP_StdFuncPrefix HP_RawUInt16Array(HP_StreamHandleType pStream, HP_pUInt16 array, HP_UInt32 arrayLen)
{
    
    
    HP_UInt32 index = 0; 
        
	/* Note: all HP_AddToList macros can set the errorCode */
    
    if (HP_NoErrors(pStream))
    {
    
        while ((HP_NoErrors(pStream)) && (index < arrayLen))
        {
            HP_AddToListInt16(pStream, (HP_UInt16) array[index++]);
        }

    }   
    
}

/*****************************************************************************


    function:   HP_RawUInt16ArrayMSB

    purpose:    Add raw 16-bit unsigned integer values to the data stream 
                from an array in Most Significant Byte First format

    parameters: name                data type / description
                -----------------   -------------------------------------
                array               HP_UInt16* / Pointer to the array of
                                        16-bit values.
                                        
                arrayLen            HP_UInt32 / The number of integers in
										the array.

    comments:   The behavior of this function is binding-dependent.  This 
                implementation places the values in the protocol byte stream.
                
*****************************************************************************/
HP_StdFuncPrefix HP_RawUInt16ArrayMSB(HP_StreamHandleType pStream, HP_pUInt16 array, HP_UInt32 arrayLen)
{
    
    
	HP_UInt32 index = 0;

    /* Note: all HP_AddToList macros can set the errorCode */
    
    if (HP_NoErrors(pStream))
    {
    
        while ((HP_NoErrors(pStream)) && (index < arrayLen))
        {
            HP_AddToListInt16MSB(pStream, (HP_UInt16) array[index++]);
        }

    }   

}

                        
/*****************************************************************************


    function:   HP_RawUInt32Array

    purpose:    Add raw 32-bit unsigned integer values to the data stream 
                from an array

    parameters: name                data type / description
                -----------------   -------------------------------------
                array               HP_UInt32* / Pointer to the array of
                                        32-bit values.
                                        
                arrayLen            HP_UInt32 / The number of integers in
                                        the array.  
                                        
    comments:   The behavior of this function is binding-dependent.  This 
                implementation places the values in the protocol byte stream.
                
*****************************************************************************/
HP_StdFuncPrefix HP_RawUInt32Array(HP_StreamHandleType pStream, HP_pUInt32 array, HP_UInt32 arrayLen)
{
    
    
	HP_UInt32 index = 0;

    /* Note: all HP_AddToList macros can set the errorCode */
    
    if (HP_NoErrors(pStream))
    {
    
        while ((HP_NoErrors(pStream)) && (index < arrayLen))
        {
            HP_AddToListInt32(pStream, (HP_UInt32) array[index++]);
		}

    }

    
}
  
/*****************************************************************************


    function:   HP_RawUInt32ArrayMSB

    purpose:    Add raw 32-bit unsigned integer values to the data stream 
				from an array in Most Significant Byte First format

    parameters: name                data type / description
                -----------------   -------------------------------------
                array               HP_UInt32* / Pointer to the array of
                                        32-bit values.
                                        
                arrayLen            HP_UInt32 / The number of integers in
                                        the array.  
                                        
    comments:   The behavior of this function is binding-dependent.  This 
                implementation places the values in the protocol byte stream.
                
*****************************************************************************/
HP_StdFuncPrefix HP_RawUInt32ArrayMSB(HP_StreamHandleType pStream, HP_pUInt32 array, HP_UInt32 arrayLen)
{
    
    
    HP_UInt32 index = 0; 
        
    /* Note: all HP_AddToList macros can set the errorCode */
    
    if (HP_NoErrors(pStream))
    {
    
        while ((HP_NoErrors(pStream)) && (index < arrayLen))
        {
            HP_AddToListInt32MSB(pStream, (HP_UInt32) array[index++]);
		}

    }

    
}
  
/*****************************************************************************


	function:   HP_RawReal32Array

    purpose:    Add raw 32-bit real values to the data stream
                from an array

    parameters: name                data type / description
                -----------------   -------------------------------------
                array               HP_Real32* / Pointer to the array of
                                        32-bit values.

                arrayLen            HP_Real32 / The number of reals in
                                        the array.

	comments:   The behavior of this function is binding-dependent.  This
                implementation places the values in the protocol byte stream.

*****************************************************************************/
HP_StdFuncPrefix HP_RawReal32Array(HP_StreamHandleType pStream, HP_pReal32 array, HP_UInt32 arrayLen)
{


    HP_UInt32 index = 0;

    /* Note: all HP_AddToList macros can set the errorCode */

    if (HP_NoErrors(pStream))
    {

        while ((HP_NoErrors(pStream)) && (index < arrayLen))
        {
            HP_AddToListReal32(pStream, (HP_Real32) array[index++]);
        }

    }


}


/*****************************************************************************


	function:   HP_RawReal32ArrayMSB

    purpose:    Add raw 32-bit real values to the data stream
                from an array in Most Significant Byte First format

    parameters: name                data type / description
                -----------------   -------------------------------------
                array               HP_Real32* / Pointer to the array of
                                        32-bit values.

				arrayLen            HP_Real32 / The number of reals in
                                        the array.

    comments:   The behavior of this function is binding-dependent.  This
                implementation places the values in the protocol byte stream.

*****************************************************************************/
HP_StdFuncPrefix HP_RawReal32ArrayMSB(HP_StreamHandleType pStream, HP_pReal32 array, HP_UInt32 arrayLen)
{


    HP_UInt32 index = 0;

    /* Note: all HP_AddToList macros can set the errorCode */

    if (HP_NoErrors(pStream))
    {

        while ((HP_NoErrors(pStream)) && (index < arrayLen))
        {
            HP_AddToListReal32MSB(pStream, (HP_Real32) array[index++]);
        }

    }


}

/********************************************************************************************
 *
 * function:   HP_DataStream()
 *
 * Purpose:    Used to pass unlimited numbers of randomly placed attributes. This call
 *             can be used to replace ALL other operator calls
 *
 * Parameters: name               dataType / Description
 *             -----------------------------------------------------------------------------
 *             pStream            HP_StreamHandleType  / Pointer to the current data stream
 *             Operator           HP_UByte             / XL Operator ID, defined in JETLIB.H
 *             ...                struct ATTRIBUTE *   / Pointer to attribute class defined
 *                                                       in the callers space. There can be
 *                                                       any number of attribute classes
 *                                                       passed in the call, in any order.
 * NOTE: Because a variable number of
 *
 *******************************************************************************************/

HP_StdFuncPrefix  HP_DataStream(HP_StreamHandleType pStream, HP_UByte Operator, ...)
{
va_list ap;
struct ATTRIBUTE *att = NULL;
HP_UByte buff[256];

	va_start(ap, Operator);
   	while ((att = va_arg(ap, struct ATTRIBUTE *)) != NULL)
    	{
        switch (att->Type)
        	{
            case HP_UByteData:
        		HP_AttrUByte(pStream, att->val.ubyte);
                break;
            case HP_UInt16Data:
        		HP_AttrUInt16(pStream, att->val.uint16);
                break;
            case HP_UInt32Data:
        		HP_AttrUInt32(pStream, att->val.uint32);
                break;
            case HP_SInt16Data:
        		HP_AttrSInt16(pStream, att->val.sint16);
                break;
            case HP_SInt32Data:
        		HP_AttrSInt32(pStream, att->val.sint32);
                break;
            case HP_Real32Data:
        		HP_AttrReal32(pStream, att->val.real32);
                break;
            case HP_UByteArray:
            	HP_AttrUByteArray(pStream, att->val.ubyte_array, att->arrayLen);
                break;
            case HP_UInt16Array:
            	HP_AttrUInt16Array(pStream, att->val.uint16_array, att->arrayLen);
                break;
            case HP_UInt32Array:
            	HP_AttrUInt32Array(pStream, att->val.uint32_array, att->arrayLen);
                break;
            case HP_SInt16Array:
            	HP_AttrSInt16Array(pStream, att->val.sint16_array, att->arrayLen);
                break;
            case HP_SInt32Array:
            	HP_AttrSInt32Array(pStream, att->val.sint32_array, att->arrayLen);
                break;
            case HP_Real32Array:
            	HP_AttrReal32Array(pStream, att->val.real32_array, att->arrayLen);
                break;
            case HP_UByteXy:
				HP_AttrXyUByte(pStream, att->val.UByte_XY.x,
                						 att->val.UByte_XY.y);
            	break;
            case HP_UInt16Xy:
				HP_AttrXyUInt16(pStream, att->val.UInt16_XY.x,
                						 att->val.UInt16_XY.y);
            	break;
            case HP_UInt32Xy:
				HP_AttrXyUInt32(pStream, att->val.UInt32_XY.x,
                						 att->val.UInt32_XY.y);
            	break;
            case HP_SInt16Xy:
				HP_AttrXySInt16(pStream, att->val.SInt16_XY.x,
                						 att->val.SInt16_XY.y);
				break;
            case HP_SInt32Xy:
				HP_AttrXySInt32(pStream, att->val.SInt32_XY.x,
                						 att->val.SInt32_XY.y);
				break;
            case HP_Real32Xy:
				HP_AttrXyReal32(pStream, att->val.Real32_XY.x,
                						 att->val.Real32_XY.y);
				break;
            case HP_UByteBox:
				HP_AttrBoxUByte(pStream, att->val.UByte_BOX.x1,
                						 att->val.UByte_BOX.y1,
                                         att->val.UByte_BOX.x2,
                                         att->val.UByte_BOX.y2);
            case HP_UInt16Box:
				HP_AttrBoxUInt16(pStream, att->val.UInt16_BOX.x1,
                						  att->val.UInt16_BOX.y1,
                                          att->val.UInt16_BOX.x2,
                                          att->val.UInt16_BOX.y2);
            	break;
            case HP_UInt32Box:
				HP_AttrBoxUInt32(pStream, att->val.UInt32_BOX.x1,
                						  att->val.UInt32_BOX.y1,
                                          att->val.UInt32_BOX.x2,
                                          att->val.UInt32_BOX.y2);
            	break;
            case HP_SInt16Box:
				HP_AttrBoxSInt16(pStream, att->val.SInt16_BOX.x1,
                						  att->val.SInt16_BOX.y1,
                                          att->val.SInt16_BOX.x2,
                                          att->val.SInt16_BOX.y2);
            case HP_SInt32Box:
				HP_AttrBoxSInt32(pStream, att->val.SInt32_BOX.x1,
                						  att->val.SInt32_BOX.y1,
                                          att->val.SInt32_BOX.x2,
                                          att->val.SInt32_BOX.y2);
            case HP_Real32Box:
				HP_AttrBoxReal32(pStream, att->val.Real32_BOX.x1,
                						  att->val.Real32_BOX.y1,
                                          att->val.Real32_BOX.x2,
                                          att->val.Real32_BOX.y2);
            	break;
			}
	   	HP_AttrId(pStream, att->Tag);
    	}
	HP_Operator(pStream, Operator);

   va_end(ap);
}
#ifndef __cplusplus
HP_StdFuncPrefix  HP_InitAttribute(struct ATTRIBUTE *att, HP_UByte aTag, HP_UByte aType )
	{
    att->Tag = aTag;
    att->Type = aType;
    att->arrayLen = 0;
    att->cksum = (HP_UInt16)aTag * (HP_UInt16)aType;
    }
#endif

/********************************************************************************************
 *
 * End Function:   HP_DataStream()
 *
 *******************************************************************************************/

#ifndef JETASM_BUILD
#ifdef DOS_MEM_MODEL
#pragma code_seg ( "PCLXL_STREAM_API" )
#endif

/*------------------------------------------------------------------
 * <PCL XL Stream API Definitions>
 *
 * Descriptions:
 *   Each of the following functions correspond to one of the Cheetah
 *   operators in the Feature Reference Manual.  All binary binding
 *   protocol translations are hidden behind those APIs.
 *
 * NOTE:
 *   If there are optional sets of parameters for an operator
 *   there will be multiple functions for the each operator.
 *
 *--------------------------------------------------------------------*/


HP_StdFuncPrefix HP_ArcPath_1(             /* ArcPath signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
	HP_SInt16 x1Val,                       /* bounding box top left x */
	HP_SInt16 y1Val,                       /* bounding box top left y */
    HP_SInt16 x2Val,                       /* bounding box bottom right x */
    HP_SInt16 y2Val,                       /* bounding box bottom right y */
    HP_SInt16 StartPointX,                 /* starting point x */
    HP_SInt16 StartPointY,                 /* starting point y */
    HP_SInt16 EndPointX,                   /* ending point x */
    HP_SInt16 EndPointY,                   /* ending point y */
    HP_UByte arcDirectionEnum)             /* arc direction enumeration */
{
   HP_AttrBoxSInt16(pStream, x1Val, y1Val,x2Val,y2Val);
   HP_AttrId(pStream, HP_BoundingBox);
   HP_AttrXySInt16(pStream, StartPointX, StartPointY);
   HP_AttrId(pStream, HP_StartPoint);
   HP_AttrXySInt16(pStream, EndPointX, EndPointY);
   HP_AttrId(pStream, HP_EndPoint);
   HP_AttrUByte(pStream, arcDirectionEnum);
   HP_AttrId(pStream, HP_ArcDirection);
   HP_Operator(pStream, HP_ArcPath);
}


HP_StdFuncPrefix HP_ArcPath_2(             /* ArcPath signature 2 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UInt16 x1Val,                       /* bounding box top left x */
    HP_UInt16 y1Val,                       /* bounding box top left y */
    HP_UInt16 x2Val,                       /* bounding box bottom right x */
    HP_UInt16 y2Val,                       /* bounding box bottom right y */
    HP_UInt16 StartPointX,                 /* starting point x */
    HP_UInt16 StartPointY,                 /* starting point y */
    HP_UInt16 EndPointX,                   /* ending point x */
    HP_UInt16 EndPointY,                   /* ending point y */
    HP_UByte arcDirectionEnum)             /* arc direction enumeration */
{
   HP_AttrBoxUInt16(pStream, x1Val, y1Val,x2Val,y2Val);
   HP_AttrId(pStream, HP_BoundingBox);
   HP_AttrXyUInt16(pStream, StartPointX, StartPointY);
   HP_AttrId(pStream, HP_StartPoint);
   HP_AttrXyUInt16(pStream, EndPointX, EndPointY);
   HP_AttrId(pStream, HP_EndPoint);
   HP_AttrUByte(pStream, arcDirectionEnum);
   HP_AttrId(pStream, HP_ArcDirection);
   HP_Operator(pStream, HP_ArcPath);
}


HP_StdFuncPrefix HP_ArcPath_3(             /* ArcPath signature 3 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_SInt32 x1Val,                       /* bounding box top left x */
    HP_SInt32 y1Val,                       /* bounding box top left y */
    HP_SInt32 x2Val,                       /* bounding box bottom right x */
    HP_SInt32 y2Val,                       /* bounding box bottom right y */
    HP_SInt32 StartPointX,                 /* starting point x */
    HP_SInt32 StartPointY,                 /* starting point y */
    HP_SInt32 EndPointX,                   /* ending point x */
    HP_SInt32 EndPointY,                   /* ending point y */
    HP_UByte arcDirectionEnum)             /* arc direction enumeration */
{
   HP_AttrBoxSInt32(pStream, x1Val, y1Val,x2Val,y2Val);
   HP_AttrId(pStream, HP_BoundingBox);
   HP_AttrXySInt32(pStream, StartPointX, StartPointY);
   HP_AttrId(pStream, HP_StartPoint);
   HP_AttrXySInt32(pStream, EndPointX, EndPointY);
   HP_AttrId(pStream, HP_EndPoint);
   HP_AttrUByte(pStream, arcDirectionEnum);
   HP_AttrId(pStream, HP_ArcDirection);
   HP_Operator(pStream, HP_ArcPath);
}



HP_StdFuncPrefix HP_ArcPath_4(             /* ArcPath signature 4 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_Real32 x1Val,                       /* bounding box top left x */
    HP_Real32 y1Val,                       /* bounding box top left y */
    HP_Real32 x2Val,                       /* bounding box bottom right x */
    HP_Real32 y2Val,                       /* bounding box bottom right y */
    HP_Real32 StartPointX,                 /* starting point x */
    HP_Real32 StartPointY,                 /* starting point y */
    HP_Real32 EndPointX,                   /* ending point x */
    HP_Real32 EndPointY,                   /* ending point y */
    HP_UByte arcDirectionEnum)             /* arc direction enumeration */
{
   HP_AttrBoxReal32(pStream, x1Val, y1Val,x2Val,y2Val);
   HP_AttrId(pStream, HP_BoundingBox);
   HP_AttrXyReal32(pStream, StartPointX, StartPointY);
   HP_AttrId(pStream, HP_StartPoint);
   HP_AttrXyReal32(pStream, EndPointX, EndPointY);
   HP_AttrId(pStream, HP_EndPoint);
   HP_AttrUByte(pStream, arcDirectionEnum);
   HP_AttrId(pStream, HP_ArcDirection);
   HP_Operator(pStream, HP_ArcPath);
}




HP_StdFuncPrefix HP_BeginChar_1(           /* BeginChar signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
	HP_pUByte fontName,                    /* array for character's font name */
	HP_UInt16 fontNameLen)                 /* length of font name array */
{
	HP_AttrUByteArray(pStream, fontName, (HP_UInt32) fontNameLen);
	HP_AttrId(pStream,HP_FontName);
	HP_Operator(pStream,HP_BeginChar);
}



HP_StdFuncPrefix HP_BeginFontHeader_1(     /* BeginFontHeader signature 1 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_pUByte fontName,                    /* array for font name */
	HP_UInt16 fontNameLen,                 /* length of font name array */
	HP_UByte fontFormat)                   /* number of font format */
{
	HP_AttrUByteArray(pStream, fontName, (HP_UInt32) fontNameLen);
	HP_AttrId(pStream,HP_FontName);
    HP_AttrUByte(pStream, fontFormat);
    HP_AttrId(pStream, HP_FontFormat);
    HP_Operator(pStream,HP_BeginFontHeader);
}

          


HP_StdFuncPrefix HP_BeginImage_1(          /* BeginImage signature 1 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_UByte colorMappingEnum,             /* color mapping enumeration */
	HP_UByte  colorDepthEnum,              /* color depth enumeration */
	HP_UInt16 sourceWidth,                 /* source image width */
	HP_UInt16 sourceHeight,                /* source image height */
	HP_UInt16 destWidth,                   /* destination height in user units */
	HP_UInt16 destHeight)                  /* destination width in user units */

{
   HP_AttrUByte(pStream, colorMappingEnum);
   HP_AttrId(pStream, HP_ColorMapping);
   HP_AttrUByte(pStream, colorDepthEnum);
   HP_AttrId(pStream, HP_ColorDepth);
   HP_AttrUInt16(pStream, sourceWidth);
   HP_AttrId(pStream,HP_SourceWidth);
   HP_AttrUInt16(pStream, sourceHeight);
   HP_AttrId(pStream, HP_SourceHeight);
   HP_AttrXyUInt16(pStream, destWidth, destHeight);
   HP_AttrId(pStream, HP_DestinationSize);
   HP_Operator(pStream, HP_BeginImage);
}



HP_StdFuncPrefix HP_BeginImage_2(          /* BeginImage signature 2 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_UByte colorMappingEnum,             /* color mapping enumeration */
	HP_UByte  colorDepthEnum,              /* color depth enumeration */
	HP_UInt16 sourceWidth,                 /* source image width */
	HP_UInt16 sourceHeight,                /* source image height */
	HP_UInt16 destWidth,                   /* destination height in user units */
	HP_UInt16 destHeight)                  /* destination width in user units */

{
   HP_AttrUByte(pStream, colorMappingEnum);
   HP_AttrId(pStream, HP_ColorMapping);
   HP_AttrUByte(pStream, colorDepthEnum);
   HP_AttrId(pStream, HP_ColorDepth);
   HP_AttrUInt16(pStream, sourceWidth);
   HP_AttrId(pStream,HP_SourceWidth);
   HP_AttrUInt16(pStream, sourceHeight);
   HP_AttrId(pStream, HP_SourceHeight);
   HP_AttrXyUInt16(pStream, destWidth, destHeight);
   HP_AttrId(pStream, HP_DestinationSize);
   HP_Operator(pStream, HP_BeginImage);
}




HP_StdFuncPrefix HP_BeginImage_3(          /* BeginImage signature 3 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_UByte colorMappingEnum,             /* color mapping enumeration */
	HP_UByte  colorDepthEnum,              /* color depth enumeration */
	HP_UInt32 sourceWidth,                 /* source image width */
	HP_UInt32 sourceHeight,                /* source image height */
	HP_UInt16 destWidth,                   /* destination height in user units */
	HP_UInt16 destHeight)                  /* destination width in user units */

{
   HP_AttrUByte(pStream, colorMappingEnum);
   HP_AttrId(pStream, HP_ColorMapping);
   HP_AttrUByte(pStream, colorDepthEnum);
   HP_AttrId(pStream, HP_ColorDepth);
   HP_AttrUInt32(pStream, sourceWidth);
   HP_AttrId(pStream,HP_SourceWidth);
   HP_AttrUInt32(pStream, sourceHeight);
   HP_AttrId(pStream, HP_SourceHeight);
   HP_AttrXyUInt16(pStream, destWidth, destHeight);
   HP_AttrId(pStream, HP_DestinationSize);
   HP_Operator(pStream, HP_BeginImage);
}




HP_StdFuncPrefix HP_BeginPage_1(           /* BeginPage signature 1 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_UByte orientationEnum,              /* page orientation enumeration */
	HP_UByte mediaSizeEnum)                /* media size enumeration */
{
  HP_AttrUByte(pStream, orientationEnum);
  HP_AttrId(pStream, HP_Orientation);
  HP_AttrUByte(pStream, mediaSizeEnum);
  HP_AttrId(pStream, HP_MediaSize);
  HP_Operator(pStream, HP_BeginPage);
}


HP_StdFuncPrefix HP_BeginPage_2(           /* BeginPage signature 2 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_UByte orientationEnum,              /* page orientation enumeration */
	HP_UInt16 xSize,                       /* custom media x-axis size */
	HP_UInt16 ySize,                       /* custom media y-axis size */
	HP_UByte unitOfMeasureEnum)            /* unit of measure enumeration */
{
  HP_AttrUByte(pStream, orientationEnum);
  HP_AttrId(pStream, HP_Orientation);
  HP_AttrXyUInt16(pStream, xSize, ySize);
  HP_AttrId(pStream, HP_CustomMediaSize);
  HP_AttrUByte(pStream, unitOfMeasureEnum);
  HP_AttrId(pStream, HP_CustomMediaSizeUnits);
  HP_Operator(pStream, HP_BeginPage);
}


HP_StdFuncPrefix HP_BeginPage_3(           /* BeginPage signature 3 */

	HP_StreamHandleType pStream,           /* stream handle */
    HP_UByte orientationEnum,              /* page orientation enumeration */
    HP_UByte mediaSizeEnum,                /* media size enumeration */
    HP_UByte mediaSourceEnum)              /* media source enumeration */
{
  HP_AttrUByte(pStream, orientationEnum);
  HP_AttrId(pStream, HP_Orientation);
  HP_AttrUByte(pStream, mediaSizeEnum);
  HP_AttrId(pStream, HP_MediaSize);
  HP_AttrUByte(pStream, mediaSourceEnum);
  HP_AttrId(pStream, HP_MediaSource);
  HP_Operator(pStream, HP_BeginPage);
}


HP_StdFuncPrefix HP_BeginPage_4(           /* BeginPage signature 4 */
   
    HP_StreamHandleType pStream,           /* stream handle */
    HP_UByte orientationEnum,              /* page orientation enumeration */ 
    HP_UInt16 xSize,                       /* custom media x-axis size */
    HP_UInt16 ySize,                       /* custom media y-axis size */
    HP_UByte unitOfMeasureEnum,            /* unit of measure enumeration */
	HP_UByte mediaSourceEnum)              /* media Source enumeration */
{
  HP_AttrUByte(pStream, orientationEnum);
  HP_AttrId(pStream, HP_Orientation);
  HP_AttrXyUInt16(pStream, xSize, ySize);
  HP_AttrId(pStream, HP_CustomMediaSize);
  HP_AttrUByte(pStream, unitOfMeasureEnum);
  HP_AttrId(pStream, HP_CustomMediaSizeUnits);
  HP_AttrUByte(pStream, mediaSourceEnum);
  HP_AttrId(pStream, HP_MediaSource);
  HP_Operator(pStream, HP_BeginPage);
}

HP_StdFuncPrefix HP_BeginPage_5(           /* BeginPage signature 1 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_UByte orientationEnum,              /* page orientation enumeration */
	HP_UByte mediaSizeEnum,                /* media size enumeration */
    HP_UByte DuplexModeEnum,               /* Set/Clear duplex mode */
    HP_UByte DuplexPageSide)               /* Front/Back to start */
{
  HP_AttrUByte(pStream, orientationEnum);
  HP_AttrId(pStream, HP_Orientation);
  HP_AttrUByte(pStream, mediaSizeEnum);
  HP_AttrId(pStream, HP_MediaSize);
  HP_AttrUByte(pStream, DuplexModeEnum);
  HP_AttrId(pStream, HP_DuplexPageMode);
  HP_AttrUByte(pStream, DuplexPageSide);
  HP_AttrId(pStream, HP_DuplexPageSide);
  HP_Operator(pStream, HP_BeginPage);
}

HP_StdFuncPrefix HP_BeginRastPattern_1(    /* BeginRastPattern signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UByte colorMappingEnum,             /* color mapping enumeration */
    HP_UByte  colorDepthEnum,              /* color depth enumeration */
    HP_UInt16 sourceWidth,                 /* source image width */
    HP_UInt16 sourceHeight,                /* source image height */
    HP_UInt16 xHeight,                     /* destination size x-axis */
    HP_UInt16 yHeight,                     /* destination size y-axis */
    HP_SInt16 patternDefineID,             /* pattern identifier */
    HP_UByte patternPersistence)           /* pattern persistence */
{
   HP_AttrUByte(pStream, colorMappingEnum);
   HP_AttrId(pStream, HP_ColorMapping);
   HP_AttrUByte(pStream, colorDepthEnum);
   HP_AttrId(pStream, HP_ColorDepth);
   HP_AttrUInt16(pStream, sourceWidth);
   HP_AttrId(pStream,HP_SourceWidth);    
   HP_AttrUInt16(pStream, sourceHeight);
   HP_AttrId(pStream, HP_SourceHeight);    
   HP_AttrXyUInt16(pStream, xHeight,yHeight );
   HP_AttrId(pStream, HP_DestinationSize);
   HP_AttrSInt16(pStream, patternDefineID);
   HP_AttrId(pStream, HP_PatternDefineID);
   HP_AttrUByte(pStream, patternPersistence);
   HP_AttrId(pStream, HP_PatternPersistence);
   HP_Operator(pStream, HP_BeginRastPattern);
}




HP_StdFuncPrefix HP_BeginRastPattern_2(    /* BeginRastPattern signature 3 */

    HP_StreamHandleType pStream,           /* stream handle */
	HP_UByte colorMappingEnum,             /* color mapping enumeration */
    HP_UByte  colorDepthEnum,              /* color depth enumeration */
    HP_UInt32 sourceWidth,                 /* source image width */
    HP_UInt32 sourceHeight,                /* source image height */
    HP_UInt32 xHeight,                     /* destination size x-axis */
    HP_UInt32 yHeight,                     /* destination size y-axis */
    HP_SInt16 patternDefineID)             /* pattern identifier */
{
   HP_AttrUByte(pStream, colorMappingEnum);
   HP_AttrId(pStream, HP_ColorMapping);
   HP_AttrUByte(pStream, colorDepthEnum);
   HP_AttrId(pStream, HP_ColorDepth);    
   HP_AttrUInt32(pStream, sourceWidth);
   HP_AttrId(pStream,HP_SourceWidth);    
   HP_AttrUInt32(pStream, sourceHeight);
   HP_AttrId(pStream, HP_SourceHeight);    
   HP_AttrXyUInt32(pStream, xHeight,yHeight );
   HP_AttrId(pStream, HP_DestinationSize);
   HP_AttrSInt16(pStream, patternDefineID);
   HP_AttrId(pStream, HP_PatternDefineID);    
   HP_Operator(pStream, HP_BeginRastPattern);
}





HP_StdFuncPrefix HP_BeginScan_1(           /* BeginScan signature 1 */

	HP_StreamHandleType pStream)           /* stream handle */

{
    HP_Operator(pStream, HP_BeginScan);
}

HP_StdFuncPrefix HP_BeginSession_1(        /* BeginSession signature 1 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_UInt16     xValue,                  /* x units of measure */
	HP_UInt16     yValue,                  /* y units of measure */
	HP_UByte      MeasureEnum)             /* measure enumeration */
{

	HP_AttrXyUInt16(pStream,xValue,yValue);
	HP_AttrId(pStream,HP_UnitsPerMeasure);
	HP_AttrUByte(pStream, MeasureEnum);
	HP_AttrId(pStream,HP_Measure);
	HP_Operator(pStream,HP_BeginSession);
}

HP_StdFuncPrefix HP_BeginSession_2(        /* BeginSession signature 2 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_UInt16     xValue,                  /* x units of measure */
	HP_UInt16     yValue,                  /* y units of measure */
	HP_UByte      MeasureEnum,             /* measure enumeration */
	HP_UByte      errorReportingEnum)      /* error reporting enumeration */
{

	HP_AttrXyUInt16(pStream,xValue,yValue);
	HP_AttrId(pStream,HP_UnitsPerMeasure);
	HP_AttrUByte(pStream, MeasureEnum);
	HP_AttrId(pStream,HP_Measure);
	HP_AttrUByte(pStream, errorReportingEnum);
	HP_AttrId(pStream,HP_ErrorReport);
	HP_Operator(pStream,HP_BeginSession);

}

HP_StdFuncPrefix HP_BeginStream_1(         /* BeginStream signature 1 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_pUByte streamName,                  /* array for stream name */
	HP_UInt16 streamNameLen)               /* length of stream name array */
{

	HP_AttrUByteArray(pStream, streamName, (HP_UInt32) streamNameLen);
	HP_AttrId(pStream,HP_StreamName);
	HP_Operator(pStream,HP_BeginStream);

}


HP_StdFuncPrefix HP_BezierPath_1(          /* BezierPath signature 1 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_UInt16 numberOfPoints,              /* number of points */
	HP_UByte pointTypeEnum)                /* enum for data type of points */

{
   HP_AttrUInt16(pStream, numberOfPoints);
   HP_AttrId(pStream, HP_NumberOfPoints);
   HP_AttrUByte(pStream, pointTypeEnum);
   HP_AttrId(pStream, HP_PointType);
   HP_Operator(pStream, HP_BezierPath);
}




HP_StdFuncPrefix HP_BezierPath_2(          /* BezierPath signature 2 */

	HP_StreamHandleType pStream,           /* stream handle */
    HP_SInt16 ControlPointX1,              /* control point x1 */
    HP_SInt16 ControlPointY1,              /* control point y1 */
    HP_SInt16 ControlPointX2,              /* control point x2 */
    HP_SInt16 ControlPointY2,              /* control point y2 */
    HP_SInt16 EndPointX,                   /* ending point x */
    HP_SInt16 EndPointY)                   /* ending point y */
{
   HP_AttrXySInt16(pStream, ControlPointX1, ControlPointY1);
   HP_AttrId(pStream, HP_ControlPoint1);
   HP_AttrXySInt16(pStream, ControlPointX2, ControlPointY2);
   HP_AttrId(pStream, HP_ControlPoint2);
   HP_AttrXySInt16(pStream, EndPointX, EndPointY);
   HP_AttrId(pStream, HP_EndPoint);
   HP_Operator(pStream, HP_BezierPath);
}




HP_StdFuncPrefix HP_BezierPath_3(          /* BezierPath signature 3 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UInt16 ControlPointX1,              /* control point x1 */
	HP_UInt16 ControlPointY1,              /* control point y1 */
    HP_UInt16 ControlPointX2,              /* control point x2 */
    HP_UInt16 ControlPointY2,              /* control point y2 */
    HP_UInt16 EndPointX,                   /* ending point x */
	HP_UInt16 EndPointY)                   /* ending point y */
{
   HP_AttrXyUInt16(pStream, ControlPointX1, ControlPointY1);
   HP_AttrId(pStream, HP_ControlPoint1);
   HP_AttrXyUInt16(pStream, ControlPointX2, ControlPointY2);
   HP_AttrId(pStream, HP_ControlPoint2);
   HP_AttrXyUInt16(pStream, EndPointX, EndPointY);
   HP_AttrId(pStream, HP_EndPoint);
   HP_Operator(pStream, HP_BezierPath);
}



HP_StdFuncPrefix HP_BezierPath_4(          /* BezierPath signature 4 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_SInt32 ControlPointX1,              /* control point x1 */
    HP_SInt32 ControlPointY1,              /* control point y1 */
	HP_SInt32 ControlPointX2,              /* control point x2 */
    HP_SInt32 ControlPointY2,              /* control point y2 */
    HP_SInt32 EndPointX,                   /* ending point x */
    HP_SInt32 EndPointY)                   /* ending point y */
{
   HP_AttrXySInt32(pStream, ControlPointX1, ControlPointY1);
   HP_AttrId(pStream, HP_ControlPoint1);
   HP_AttrXySInt32(pStream, ControlPointX2, ControlPointY2);
   HP_AttrId(pStream, HP_ControlPoint2);
   HP_AttrXySInt32(pStream, EndPointX, EndPointY);
   HP_AttrId(pStream, HP_EndPoint);
   HP_Operator(pStream, HP_BezierPath);
}



HP_StdFuncPrefix HP_BezierPath_5(          /* BezierPath signature 5 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_Real32 ControlPointX1,              /* control point x1 */
    HP_Real32 ControlPointY1,              /* control point y1 */
    HP_Real32 ControlPointX2,              /* control point x2 */
    HP_Real32 ControlPointY2,              /* control point y2 */
    HP_Real32 EndPointX,                   /* ending point x */
    HP_Real32 EndPointY)                   /* ending point y */
{
   HP_AttrXyReal32(pStream, ControlPointX1, ControlPointY1);
   HP_AttrId(pStream, HP_ControlPoint1);
   HP_AttrXyReal32(pStream, ControlPointX2, ControlPointY2);
   HP_AttrId(pStream, HP_ControlPoint2);
   HP_AttrXyReal32(pStream, EndPointX, EndPointY);
   HP_AttrId(pStream, HP_EndPoint);
   HP_Operator(pStream, HP_BezierPath);
}



HP_StdFuncPrefix HP_BezierRelPath_1(       /* BezierRelPath signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UInt16 numberOfPoints,              /* number of points */
    HP_UByte pointTypeEnum)                /* enum for data type of points */

{
   HP_AttrUInt16(pStream, numberOfPoints);
   HP_AttrId(pStream, HP_NumberOfPoints);
   HP_AttrUByte(pStream, pointTypeEnum);
   HP_AttrId(pStream, HP_PointType);
   HP_Operator(pStream, HP_BezierRelPath);
}




HP_StdFuncPrefix HP_BezierRelPath_2(       /* BezierRelPath signature 2 */

	HP_StreamHandleType pStream,           /* stream handle */
    HP_SInt16 ControlPointX1,              /* control point x1 */
    HP_SInt16 ControlPointY1,              /* control point y1 */
    HP_SInt16 ControlPointX2,              /* control point x2 */
    HP_SInt16 ControlPointY2,              /* control point y2 */
    HP_SInt16 EndPointX,                   /* ending point x */
    HP_SInt16 EndPointY)                   /* ending point y */
{
   HP_AttrXySInt16(pStream, ControlPointX1, ControlPointY1);
   HP_AttrId(pStream, HP_ControlPoint1);
   HP_AttrXySInt16(pStream, ControlPointX2, ControlPointY2);
   HP_AttrId(pStream, HP_ControlPoint2);
   HP_AttrXySInt16(pStream, EndPointX, EndPointY);
   HP_AttrId(pStream, HP_EndPoint);
   HP_Operator(pStream, HP_BezierRelPath);
}




HP_StdFuncPrefix HP_BezierRelPath_3(       /* BezierRelPath signature 3 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UInt16 ControlPointX1,              /* control point x1 */
	HP_UInt16 ControlPointY1,              /* control point y1 */
    HP_UInt16 ControlPointX2,              /* control point x2 */
    HP_UInt16 ControlPointY2,              /* control point y2 */
    HP_UInt16 EndPointX,                   /* ending point x */
	HP_UInt16 EndPointY)                   /* ending point y */
{
   HP_AttrXyUInt16(pStream, ControlPointX1, ControlPointY1);
   HP_AttrId(pStream, HP_ControlPoint1);
   HP_AttrXyUInt16(pStream, ControlPointX2, ControlPointY2);
   HP_AttrId(pStream, HP_ControlPoint2);
   HP_AttrXyUInt16(pStream, EndPointX, EndPointY);
   HP_AttrId(pStream, HP_EndPoint);
   HP_Operator(pStream, HP_BezierRelPath);
}



HP_StdFuncPrefix HP_BezierRelPath_4(       /* BezierRelPath signature 4 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_SInt32 ControlPointX1,              /* control point x1 */
    HP_SInt32 ControlPointY1,              /* control point y1 */
	HP_SInt32 ControlPointX2,              /* control point x2 */
    HP_SInt32 ControlPointY2,              /* control point y2 */
    HP_SInt32 EndPointX,                   /* ending point x */
    HP_SInt32 EndPointY)                   /* ending point y */
{
   HP_AttrXySInt32(pStream, ControlPointX1, ControlPointY1);
   HP_AttrId(pStream, HP_ControlPoint1);
   HP_AttrXySInt32(pStream, ControlPointX2, ControlPointY2);
   HP_AttrId(pStream, HP_ControlPoint2);
   HP_AttrXySInt32(pStream, EndPointX, EndPointY);
   HP_AttrId(pStream, HP_EndPoint);
   HP_Operator(pStream, HP_BezierRelPath);
}



HP_StdFuncPrefix HP_BezierRelPath_5(       /* BezierRelPath signature 5 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_Real32 ControlPointX1,              /* control point x1 */
    HP_Real32 ControlPointY1,              /* control point y1 */
    HP_Real32 ControlPointX2,              /* control point x2 */
    HP_Real32 ControlPointY2,              /* control point y2 */
    HP_Real32 EndPointX,                   /* ending point x */
    HP_Real32 EndPointY)                   /* ending point y */
{
   HP_AttrXyReal32(pStream, ControlPointX1, ControlPointY1);
   HP_AttrId(pStream, HP_ControlPoint1);
   HP_AttrXyReal32(pStream, ControlPointX2, ControlPointY2);
   HP_AttrId(pStream, HP_ControlPoint2);
   HP_AttrXyReal32(pStream, EndPointX, EndPointY);
   HP_AttrId(pStream, HP_EndPoint);
   HP_Operator(pStream, HP_BezierRelPath);
}




HP_StdFuncPrefix HP_Chord_1(               /* Chord signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_SInt16 x1Val,                       /* bounding box top left x */
    HP_SInt16 y1Val,                       /* bounding box top left y */
	HP_SInt16 x2Val,                       /* bounding box bottom right x */
	HP_SInt16 y2Val,                       /* bounding box bottom right y */
    HP_SInt16 StartPointX,                 /* starting point x */
    HP_SInt16 StartPointY,                 /* starting point y */
    HP_SInt16 EndPointX,                   /* ending point x */
    HP_SInt16 EndPointY)                   /* ending point y */
{
   HP_AttrBoxSInt16(pStream, x1Val, y1Val,x2Val,y2Val);
   HP_AttrId(pStream, HP_BoundingBox);
   HP_AttrXySInt16(pStream, StartPointX, StartPointY);
   HP_AttrId(pStream, HP_StartPoint);
   HP_AttrXySInt16(pStream, EndPointX, EndPointY);
   HP_AttrId(pStream, HP_EndPoint);
   HP_Operator(pStream, HP_Chord);
}




HP_StdFuncPrefix HP_Chord_2(               /* Chord signature 2 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UInt16 x1Val,                       /* bounding box top left x */
    HP_UInt16 y1Val,                       /* bounding box top left y */
    HP_UInt16 x2Val,                       /* bounding box bottom right x */
    HP_UInt16 y2Val,                       /* bounding box bottom right y */
    HP_UInt16 StartPointX,                 /* starting point x */
    HP_UInt16 StartPointY,                 /* starting point y */
    HP_UInt16 EndPointX,                   /* ending point x */
    HP_UInt16 EndPointY)                   /* ending point y */
{
   HP_AttrBoxUInt16(pStream, x1Val, y1Val,x2Val,y2Val);
   HP_AttrId(pStream, HP_BoundingBox);
   HP_AttrXyUInt16(pStream, StartPointX, StartPointY);
   HP_AttrId(pStream, HP_StartPoint);
   HP_AttrXyUInt16(pStream, EndPointX, EndPointY);
   HP_AttrId(pStream, HP_EndPoint);
   HP_Operator(pStream, HP_Chord);
}



HP_StdFuncPrefix HP_Chord_3(               /* Chord signature 3 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_SInt32 x1Val,                       /* bounding box top left x */
    HP_SInt32 y1Val,                       /* bounding box top left y */
    HP_SInt32 x2Val,                       /* bounding box bottom right x */
    HP_SInt32 y2Val,                       /* bounding box bottom right y */
    HP_SInt32 StartPointX,                 /* starting point x */
    HP_SInt32 StartPointY,                 /* starting point y */
	HP_SInt32 EndPointX,                   /* ending point x */
	HP_SInt32 EndPointY)                   /* ending point y */
{
   HP_AttrBoxSInt32(pStream, x1Val, y1Val,x2Val,y2Val);
   HP_AttrId(pStream, HP_BoundingBox);
   HP_AttrXySInt32(pStream, StartPointX, StartPointY);
   HP_AttrId(pStream, HP_StartPoint);
   HP_AttrXySInt32(pStream, EndPointX, EndPointY);
   HP_AttrId(pStream, HP_EndPoint);
   HP_Operator(pStream, HP_Chord);
}




HP_StdFuncPrefix HP_Chord_4(               /* Chord signature 4 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_Real32 x1Val,                       /* bounding box top left x */
    HP_Real32 y1Val,                       /* bounding box top left y */
    HP_Real32 x2Val,                       /* bounding box bottom right x */
    HP_Real32 y2Val,                       /* bounding box bottom right y */
    HP_Real32 StartPointX,                 /* starting point x */
    HP_Real32 StartPointY,                 /* starting point y */
    HP_Real32 EndPointX,                   /* ending point x */
    HP_Real32 EndPointY)                   /* ending point y */
{
   HP_AttrBoxReal32(pStream, x1Val, y1Val,x2Val,y2Val);
   HP_AttrId(pStream, HP_BoundingBox);
   HP_AttrXyReal32(pStream, StartPointX, StartPointY);
   HP_AttrId(pStream, HP_StartPoint);
   HP_AttrXyReal32(pStream, EndPointX, EndPointY);
   HP_AttrId(pStream, HP_EndPoint);
   HP_Operator(pStream, HP_Chord);
}




HP_StdFuncPrefix HP_ChordPath_1(           /* ChordPath signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_SInt16 x1Val,                       /* bounding box top left x */
    HP_SInt16 y1Val,                       /* bounding box top left y */
    HP_SInt16 x2Val,                       /* bounding box bottom right x */
    HP_SInt16 y2Val,                       /* bounding box bottom right y */
    HP_SInt16 StartPointX,                 /* starting point x */
    HP_SInt16 StartPointY,                 /* starting point y */
    HP_SInt16 EndPointX,                   /* ending point x */
    HP_SInt16 EndPointY)                   /* ending point y */
{
   HP_AttrBoxSInt16(pStream, x1Val, y1Val,x2Val,y2Val);
   HP_AttrId(pStream, HP_BoundingBox);
   HP_AttrXySInt16(pStream, StartPointX, StartPointY);
   HP_AttrId(pStream, HP_StartPoint);
   HP_AttrXySInt16(pStream, EndPointX, EndPointY);
   HP_AttrId(pStream, HP_EndPoint);
   HP_Operator(pStream, HP_ChordPath);
}




HP_StdFuncPrefix HP_ChordPath_2(           /* ChordPath signature 2 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UInt16 x1Val,                       /* bounding box top left x */
    HP_UInt16 y1Val,                       /* bounding box top left y */
    HP_UInt16 x2Val,                       /* bounding box bottom right x */
    HP_UInt16 y2Val,                       /* bounding box bottom right y */
    HP_UInt16 StartPointX,                 /* starting point x */
    HP_UInt16 StartPointY,                 /* starting point y */
    HP_UInt16 EndPointX,                   /* ending point x */
    HP_UInt16 EndPointY)                   /* ending point y */
{
   HP_AttrBoxUInt16(pStream, x1Val, y1Val,x2Val,y2Val);
   HP_AttrId(pStream, HP_BoundingBox);
   HP_AttrXyUInt16(pStream, StartPointX, StartPointY);
   HP_AttrId(pStream, HP_StartPoint);
   HP_AttrXyUInt16(pStream, EndPointX, EndPointY);
   HP_AttrId(pStream, HP_EndPoint);
   HP_Operator(pStream, HP_ChordPath);
}



HP_StdFuncPrefix HP_ChordPath_3(           /* ChordPath signature 3 */

	HP_StreamHandleType pStream,           /* stream handle */
    HP_SInt32 x1Val,                       /* bounding box top left x */
    HP_SInt32 y1Val,                       /* bounding box top left y */
    HP_SInt32 x2Val,                       /* bounding box bottom right x */
    HP_SInt32 y2Val,                       /* bounding box bottom right y */
    HP_SInt32 StartPointX,                 /* starting point x */
    HP_SInt32 StartPointY,                 /* starting point y */
    HP_SInt32 EndPointX,                   /* ending point x */
    HP_SInt32 EndPointY)                   /* ending point y */
{
   HP_AttrBoxSInt32(pStream, x1Val, y1Val,x2Val,y2Val);
   HP_AttrId(pStream, HP_BoundingBox);
   HP_AttrXySInt32(pStream, StartPointX, StartPointY);
   HP_AttrId(pStream, HP_StartPoint);
   HP_AttrXySInt32(pStream, EndPointX, EndPointY);
   HP_AttrId(pStream, HP_EndPoint);
   HP_Operator(pStream, HP_ChordPath);
}




HP_StdFuncPrefix HP_ChordPath_4(           /* ChordPath signature 4 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_Real32 x1Val,                       /* bounding box top left x */
    HP_Real32 y1Val,                       /* bounding box top left y */
	HP_Real32 x2Val,                       /* bounding box bottom right x */
    HP_Real32 y2Val,                       /* bounding box bottom right y */
    HP_Real32 StartPointX,                 /* starting point x */
    HP_Real32 StartPointY,                 /* starting point y */
    HP_Real32 EndPointX,                   /* ending point x */
    HP_Real32 EndPointY)                   /* ending point y */
{
   HP_AttrBoxReal32(pStream, x1Val, y1Val,x2Val,y2Val);
   HP_AttrId(pStream, HP_BoundingBox);
   HP_AttrXyReal32(pStream, StartPointX, StartPointY);
   HP_AttrId(pStream, HP_StartPoint);
   HP_AttrXyReal32(pStream, EndPointX, EndPointY);
   HP_AttrId(pStream, HP_EndPoint);
   HP_Operator(pStream, HP_ChordPath);
}




HP_StdFuncPrefix HP_CloseDataSource_1(     /* CloseDataSource signature 1 */

	HP_StreamHandleType pStream)           /* stream handle */
{
    HP_Operator(pStream,HP_CloseDataSource);
}




HP_StdFuncPrefix HP_CloseSubPath_1(        /* CloseSubPath signature 1 */

    HP_StreamHandleType pStream)           /* stream handle */
{
   HP_Operator(pStream, HP_CloseSubPath);
}
 



HP_StdFuncPrefix HP_Comment_1(             /* Comment signature 1 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_pUByte array,                       /* array for comment data/text */
	HP_UInt32 arrayLen)                    /* length of comment data array */
{
	HP_AttrUByteArray(pStream, array, arrayLen);
	HP_AttrId(pStream, HP_CommentData);
	HP_Operator(pStream,HP_Comment);
}

HP_StdFuncPrefix HP_Comment_2(             /* Comment signature 1 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_pCharHuge array)                    /* length of comment data array */
{
HP_UInt32 arrayLen = strlen((const char*)array);

	HP_AttrUByteArray(pStream, array, arrayLen);
	HP_AttrId(pStream, HP_CommentData);
	HP_Operator(pStream,HP_Comment);
}



HP_StdFuncPrefix HP_EchoComment_1(         /* EchoComment signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_pUByte array,                       /* array for comment data/text */
    HP_UInt32 arrayLen)                    /* length of comment data array */
{
    HP_AttrUByteArray(pStream, array, arrayLen);
    HP_AttrId(pStream, HP_CommentData);
    HP_Operator(pStream,HP_EchoComment);
}



HP_StdFuncPrefix HP_Ellipse_1(             /* Ellipse signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_SInt16 x1Val,                       /* bounding box top left x */
    HP_SInt16 y1Val,                       /* bounding box top left y */
    HP_SInt16 x2Val,                       /* bounding box bottom right x */
    HP_SInt16 y2Val)                       /* bounding box bottom right y */
{
   HP_AttrBoxSInt16(pStream, x1Val, y1Val, x2Val, y2Val);
   HP_AttrId(pStream, HP_BoundingBox);
   HP_Operator(pStream, HP_Ellipse);
}




HP_StdFuncPrefix HP_Ellipse_2(             /* Ellipse signature 2 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UInt16 x1Val,                       /* bounding box top left x */
    HP_UInt16 y1Val,                       /* bounding box top left y */
    HP_UInt16 x2Val,                       /* bounding box bottom right x */
    HP_UInt16 y2Val)                       /* bounding box bottom right y */
{
   HP_AttrBoxUInt16(pStream, x1Val, y1Val, x2Val, y2Val);
   HP_AttrId(pStream, HP_BoundingBox);
   HP_Operator(pStream, HP_Ellipse);
}



HP_StdFuncPrefix HP_Ellipse_3(             /* Ellipse signature 3 */

	HP_StreamHandleType pStream,           /* stream handle */
    HP_SInt32 x1Val,                       /* bounding box top left x */
    HP_SInt32 y1Val,                       /* bounding box top left y */
    HP_SInt32 x2Val,                       /* bounding box bottom right x */
    HP_SInt32 y2Val)                       /* bounding box bottom right y */
{
   HP_AttrBoxSInt32(pStream, x1Val, y1Val, x2Val, y2Val);
   HP_AttrId(pStream, HP_BoundingBox);
   HP_Operator(pStream, HP_Ellipse);
}



HP_StdFuncPrefix HP_Ellipse_4(             /* Ellipse signature 4 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_Real32 x1Val,                       /* bounding box top left x */
    HP_Real32 y1Val,                       /* bounding box top left y */
    HP_Real32 x2Val,                       /* bounding box bottom right x */
    HP_Real32 y2Val)                       /* bounding box bottom right y */
{
   HP_AttrBoxReal32(pStream, x1Val, y1Val, x2Val, y2Val);
   HP_AttrId(pStream, HP_BoundingBox);
   HP_Operator(pStream, HP_Ellipse);
}




HP_StdFuncPrefix HP_EllipsePath_1(         /* EllipsePath signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_SInt16 x1Val,                       /* bounding box top left x */
    HP_SInt16 y1Val,                       /* bounding box top left y */
    HP_SInt16 x2Val,                       /* bounding box bottom right x */
    HP_SInt16 y2Val)                       /* bounding box bottom right y */
{
   HP_AttrBoxSInt16(pStream, x1Val, y1Val, x2Val, y2Val);
   HP_AttrId(pStream, HP_BoundingBox);
   HP_Operator(pStream, HP_EllipsePath);
}




HP_StdFuncPrefix HP_EllipsePath_2(         /* EllipsePath signature 2 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UInt16 x1Val,                       /* bounding box top left x */
    HP_UInt16 y1Val,                       /* bounding box top left y */
    HP_UInt16 x2Val,                       /* bounding box bottom right x */
	HP_UInt16 y2Val)                       /* bounding box bottom right y */
{
   HP_AttrBoxUInt16(pStream, x1Val, y1Val, x2Val, y2Val);
   HP_AttrId(pStream, HP_BoundingBox);
   HP_Operator(pStream, HP_EllipsePath);
}



HP_StdFuncPrefix HP_EllipsePath_3(         /* EllipsePath signature 3 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_SInt32 x1Val,                       /* bounding box top left x */
    HP_SInt32 y1Val,                       /* bounding box top left y */
    HP_SInt32 x2Val,                       /* bounding box bottom right x */
    HP_SInt32 y2Val)                       /* bounding box bottom right y */
{
   HP_AttrBoxSInt32(pStream, x1Val, y1Val, x2Val, y2Val);
   HP_AttrId(pStream, HP_BoundingBox);
   HP_Operator(pStream, HP_EllipsePath);
}



HP_StdFuncPrefix HP_EllipsePath_4(         /* EllipsePath signature 4 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_Real32 x1Val,                       /* bounding box top left x */
    HP_Real32 y1Val,                       /* bounding box top left y */
	HP_Real32 x2Val,                       /* bounding box bottom right x */
    HP_Real32 y2Val)                       /* bounding box bottom right y */
{
   HP_AttrBoxReal32(pStream, x1Val, y1Val, x2Val, y2Val);
   HP_AttrId(pStream, HP_BoundingBox);
   HP_Operator(pStream, HP_EllipsePath);
}



HP_StdFuncPrefix HP_EndChar_1(             /* EndChar signature 1 */

    HP_StreamHandleType pStream)           /* stream handle */
{
	HP_Operator(pStream,HP_EndChar);
}

         


HP_StdFuncPrefix HP_EndFontHeader_1(       /* EndFontHeader signature 1 */

	HP_StreamHandleType pStream)           /* stream handle */
{                                       
    HP_Operator(pStream,HP_EndFontHeader);
}




HP_StdFuncPrefix HP_EndImage_1(            /* EndImage signature 1 */

    HP_StreamHandleType pStream)           /* stream handle */
{
    HP_Operator(pStream, HP_EndImage);
}



HP_StdFuncPrefix HP_EndPage_1(             /* EndPage signature 1 */

    HP_StreamHandleType pStream)           /* stream handle */
{
    HP_Operator(pStream,HP_EndPage);
}


HP_StdFuncPrefix HP_EndPage_2(             /* EndPage signature 2 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UInt16 pageCopies)                  /* number of copies */
{
    HP_AttrUInt16(pStream, pageCopies);
    HP_AttrId(pStream, HP_PageCopies);
    HP_Operator(pStream,HP_EndPage);
}


HP_StdFuncPrefix HP_EndPage_3(             /* EndPage signature 3 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UInt16 pageCopies,                  /* number of copies */
    HP_UByte mediaSourceEnum)              /* media source enumeration */
{
    HP_AttrUInt16(pStream, pageCopies);
	HP_AttrId(pStream, HP_PageCopies);
	HP_AttrUByte(pStream, mediaSourceEnum);
    HP_AttrId(pStream, HP_MediaSource);
    HP_Operator(pStream,HP_EndPage);
}

HP_StdFuncPrefix HP_EndPage_4(             /* EndPage signature 4 */

	HP_StreamHandleType pStream,           /* stream handle */
    HP_UByte mediaSourceEnum)              /* media source enumeration */
{
    HP_AttrUByte(pStream, mediaSourceEnum);
    HP_AttrId(pStream, HP_MediaSource);
	HP_Operator(pStream,HP_EndPage);
}


HP_StdFuncPrefix HP_EndRastPattern_1(      /* EndRastPattern signature 1 */

    HP_StreamHandleType pStream)           /* stream handle */
{
    HP_Operator(pStream, HP_EndRastPattern);
}




HP_StdFuncPrefix HP_EndScan_1(             /* EndScan signature 1 */

    HP_StreamHandleType pStream)           /* stream handle */
{
    HP_Operator(pStream, HP_EndScan);

}


HP_StdFuncPrefix HP_EndStream_1(           /* EndStream signature 1 */

    HP_StreamHandleType pStream)           /* stream handle */
{
    HP_Operator(pStream, HP_EndStream);

}


HP_StdFuncPrefix HP_EndSession_1(          /* EndSession signature 1 */

    HP_StreamHandleType pStream)           /* stream handle */
{
    HP_Operator(pStream,HP_EndSession);
}


HP_StdFuncPrefix HP_LinePath_1(            /* LinePath signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UInt16 numberOfPoints,              /* number of points */
    HP_UByte pointTypeEnum)                /* enum for data type of points */

{
   HP_AttrUInt16(pStream, numberOfPoints);
   HP_AttrId(pStream, HP_NumberOfPoints);
   HP_AttrUByte(pStream, pointTypeEnum);
   HP_AttrId(pStream, HP_PointType);
   HP_Operator(pStream, HP_LinePath);
}




HP_StdFuncPrefix HP_LinePath_2(            /* LinePath signature 2 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_SInt16 EndPointX,                   /* ending point x */
    HP_SInt16 EndPointY)                   /* ending point y */

{
   HP_AttrXySInt16(pStream, EndPointX, EndPointY);
   HP_AttrId(pStream, HP_EndPoint);
   HP_Operator(pStream, HP_LinePath);
}




HP_StdFuncPrefix HP_LinePath_3(            /* LinePath signature 3 */

    HP_StreamHandleType pStream,           /* stream handle */
	HP_UInt16 EndPointX,                   /* ending point x */
    HP_UInt16 EndPointY)                   /* ending point y */

{
   HP_AttrXyUInt16(pStream, EndPointX, EndPointY);
   HP_AttrId(pStream, HP_EndPoint);
   HP_Operator(pStream, HP_LinePath);
}




HP_StdFuncPrefix HP_LinePath_4(            /* LinePath signature 4 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_SInt32 EndPointX,                   /* ending point x */
    HP_SInt32 EndPointY)                   /* ending point y */

{
   HP_AttrXySInt32(pStream, EndPointX, EndPointY);
   HP_AttrId(pStream, HP_EndPoint);
   HP_Operator(pStream, HP_LinePath);
}




HP_StdFuncPrefix HP_LinePath_5(            /* LinePath signature 5 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_Real32 EndPointX,                   /* ending point x */
	HP_Real32 EndPointY)                   /* ending point y */

{
   HP_AttrXyReal32(pStream, EndPointX, EndPointY);
   HP_AttrId(pStream, HP_EndPoint);
   HP_Operator(pStream, HP_LinePath);
}




HP_StdFuncPrefix HP_LineRelPath_1(         /* LineRelPath signature 1 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_UInt16 numberOfPoints,              /* number of points */
	HP_UByte pointTypeEnum)                /* enum for data type of points */

{
   HP_AttrUInt16(pStream, numberOfPoints);
   HP_AttrId(pStream, HP_NumberOfPoints);
   HP_AttrUByte(pStream, pointTypeEnum);
   HP_AttrId(pStream, HP_PointType);
   HP_Operator(pStream, HP_LineRelPath);
}




HP_StdFuncPrefix HP_LineRelPath_2(         /* LineRelPath signature 2 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_SInt16 EndPointX,                   /* ending point x */
	HP_SInt16 EndPointY)                   /* ending point y */

{
   HP_AttrXySInt16(pStream, EndPointX, EndPointY);
   HP_AttrId(pStream, HP_EndPoint);
   HP_Operator(pStream, HP_LineRelPath);
}




HP_StdFuncPrefix HP_LineRelPath_3(         /* LineRelPath signature 3 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_UInt16 EndPointX,                   /* ending point x */
	HP_UInt16 EndPointY)                   /* ending point y */

{
   HP_AttrXyUInt16(pStream, EndPointX, EndPointY);
   HP_AttrId(pStream, HP_EndPoint);
   HP_Operator(pStream, HP_LineRelPath);
}




HP_StdFuncPrefix HP_LineRelPath_4(         /* LineRelPath signature 4 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_SInt32 EndPointX,                   /* ending point x */
	HP_SInt32 EndPointY)                   /* ending point y */

{
   HP_AttrXySInt32(pStream, EndPointX, EndPointY);
   HP_AttrId(pStream, HP_EndPoint);
   HP_Operator(pStream, HP_LineRelPath);
}




HP_StdFuncPrefix HP_LineRelPath_5(         /* LineRelPath signature 5 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_Real32 EndPointX,                   /* ending point x */
	HP_Real32 EndPointY)                   /* ending point y */

{
   HP_AttrXyReal32(pStream, EndPointX, EndPointY);
   HP_AttrId(pStream, HP_EndPoint);
   HP_Operator(pStream, HP_LineRelPath);
}




HP_StdFuncPrefix HP_NewPath_1(             /* NewPath signature 1 */

    HP_StreamHandleType pStream)           /* stream handle */
{
   HP_Operator(pStream, HP_NewPath);
}
 

/* This is a temporary API until the firmware catches up to the Vol. 2 spec */

HP_StdFuncPrefix HP_OpenDataSource_Temp1(  /* OpenDataSource signature Temp1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UByte dataSourceEnum)               /* dataSourceEnumeration */
{                                          
    HP_AttrUByte(pStream, dataSourceEnum);
    HP_AttrId(pStream, HP_SourceType); 
    HP_Operator(pStream,HP_OpenDataSource);
}


HP_StdFuncPrefix HP_OpenDataSource_1(      /* OpenDataSource signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UByte dataSourceEnum,               /* dataSourceEnumeration */
    HP_UByte dataOrgEnum)                  /* dataOrgEnumeration */
{
    HP_AttrUByte(pStream, dataSourceEnum);
    HP_AttrId(pStream, HP_SourceType); 
    HP_AttrUByte(pStream, dataOrgEnum);
    HP_AttrId(pStream, HP_DataOrg);
    HP_Operator(pStream,HP_OpenDataSource);
}



HP_StdFuncPrefix HP_PaintPath_1(           /* PaintPath signature 1 */
    
    HP_StreamHandleType pStream)           /* stream handle */
{
   HP_Operator(pStream, HP_PaintPath);
}




HP_StdFuncPrefix HP_Pie_1(                 /* Pie signature 1 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_SInt16 x1Val,                       /* bounding box top left x */
    HP_SInt16 y1Val,                       /* bounding box top left y */
    HP_SInt16 x2Val,                       /* bounding box bottom right x */
    HP_SInt16 y2Val,                       /* bounding box bottom right y */
    HP_SInt16 StartPointX,                 /* starting point x */
    HP_SInt16 StartPointY,                 /* starting point y */
    HP_SInt16 EndPointX,                   /* ending point x */
    HP_SInt16 EndPointY)                   /* ending point y */
{
   HP_AttrBoxSInt16(pStream, x1Val, y1Val,x2Val,y2Val);
   HP_AttrId(pStream, HP_BoundingBox);
   HP_AttrXySInt16(pStream, StartPointX, StartPointY);
   HP_AttrId(pStream, HP_StartPoint);
   HP_AttrXySInt16(pStream, EndPointX, EndPointY);
   HP_AttrId(pStream, HP_EndPoint);
   HP_Operator(pStream, HP_Pie);
}




HP_StdFuncPrefix HP_Pie_2(                 /* Pie signature 2 */

	HP_StreamHandleType pStream,           /* stream handle */
    HP_UInt16 x1Val,                       /* bounding box top left x */
    HP_UInt16 y1Val,                       /* bounding box top left y */
    HP_UInt16 x2Val,                       /* bounding box bottom right x */
    HP_UInt16 y2Val,                       /* bounding box bottom right y */
    HP_UInt16 StartPointX,                 /* starting point x */
    HP_UInt16 StartPointY,                 /* starting point y */
    HP_UInt16 EndPointX,                   /* ending point x */
    HP_UInt16 EndPointY)                   /* ending point y */
{
   HP_AttrBoxUInt16(pStream, x1Val, y1Val,x2Val,y2Val);
   HP_AttrId(pStream, HP_BoundingBox);
   HP_AttrXyUInt16(pStream, StartPointX, StartPointY);
   HP_AttrId(pStream, HP_StartPoint);
   HP_AttrXyUInt16(pStream, EndPointX, EndPointY);
   HP_AttrId(pStream, HP_EndPoint);
   HP_Operator(pStream, HP_Pie);
}



HP_StdFuncPrefix HP_Pie_3(                 /* Pie signature 3 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_SInt32 x1Val,                       /* bounding box top left x */
    HP_SInt32 y1Val,                       /* bounding box top left y */
    HP_SInt32 x2Val,                       /* bounding box bottom right x */
	HP_SInt32 y2Val,                       /* bounding box bottom right y */
	HP_SInt32 StartPointX,                 /* starting point x */
    HP_SInt32 StartPointY,                 /* starting point y */
    HP_SInt32 EndPointX,                   /* ending point x */
    HP_SInt32 EndPointY)                   /* ending point y */
{
   HP_AttrBoxSInt32(pStream, x1Val, y1Val,x2Val,y2Val);
   HP_AttrId(pStream, HP_BoundingBox);
   HP_AttrXySInt32(pStream, StartPointX, StartPointY);
   HP_AttrId(pStream, HP_StartPoint);
   HP_AttrXySInt32(pStream, EndPointX, EndPointY);
   HP_AttrId(pStream, HP_EndPoint);
   HP_Operator(pStream, HP_Pie);
}




HP_StdFuncPrefix HP_Pie_4(                 /* Pie signature 4 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_Real32 x1Val,                       /* bounding box top left x */
    HP_Real32 y1Val,                       /* bounding box top left y */
    HP_Real32 x2Val,                       /* bounding box bottom right x */
    HP_Real32 y2Val,                       /* bounding box bottom right y */
    HP_Real32 StartPointX,                 /* starting point x */
    HP_Real32 StartPointY,                 /* starting point y */
    HP_Real32 EndPointX,                   /* ending point x */
    HP_Real32 EndPointY)                   /* ending point y */
{
   HP_AttrBoxReal32(pStream, x1Val, y1Val,x2Val,y2Val);
   HP_AttrId(pStream, HP_BoundingBox);
   HP_AttrXyReal32(pStream, StartPointX, StartPointY);
   HP_AttrId(pStream, HP_StartPoint);
   HP_AttrXyReal32(pStream, EndPointX, EndPointY);
   HP_AttrId(pStream, HP_EndPoint);
   HP_Operator(pStream, HP_Pie);
}




HP_StdFuncPrefix HP_PiePath_1(             /* PiePath signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_SInt16 x1Val,                       /* bounding box top left x */
    HP_SInt16 y1Val,                       /* bounding box top left y */
    HP_SInt16 x2Val,                       /* bounding box bottom right x */
    HP_SInt16 y2Val,                       /* bounding box bottom right y */
    HP_SInt16 StartPointX,                 /* starting point x */
    HP_SInt16 StartPointY,                 /* starting point y */
	HP_SInt16 EndPointX,                   /* ending point x */
	HP_SInt16 EndPointY)                   /* ending point y */
{
   HP_AttrBoxSInt16(pStream, x1Val, y1Val,x2Val,y2Val);
   HP_AttrId(pStream, HP_BoundingBox);
   HP_AttrXySInt16(pStream, StartPointX, StartPointY);
   HP_AttrId(pStream, HP_StartPoint);
   HP_AttrXySInt16(pStream, EndPointX, EndPointY);
   HP_AttrId(pStream, HP_EndPoint);
   HP_Operator(pStream, HP_PiePath);
}




HP_StdFuncPrefix HP_PiePath_2(             /* PiePath signature 2 */

    HP_StreamHandleType pStream,           /* stream handle */
	HP_UInt16 x1Val,                       /* bounding box top left x */
    HP_UInt16 y1Val,                       /* bounding box top left y */
    HP_UInt16 x2Val,                       /* bounding box bottom right x */
    HP_UInt16 y2Val,                       /* bounding box bottom right y */
    HP_UInt16 StartPointX,                 /* starting point x */
    HP_UInt16 StartPointY,                 /* starting point y */
    HP_UInt16 EndPointX,                   /* ending point x */
    HP_UInt16 EndPointY)                   /* ending point y */
{
   HP_AttrBoxUInt16(pStream, x1Val, y1Val,x2Val,y2Val);
   HP_AttrId(pStream, HP_BoundingBox);
   HP_AttrXyUInt16(pStream, StartPointX, StartPointY);
   HP_AttrId(pStream, HP_StartPoint);
   HP_AttrXyUInt16(pStream, EndPointX, EndPointY);
   HP_AttrId(pStream, HP_EndPoint);
   HP_Operator(pStream, HP_PiePath);
}



HP_StdFuncPrefix HP_PiePath_3(             /* PiePath signature 3 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_SInt32 x1Val,                       /* bounding box top left x */
    HP_SInt32 y1Val,                       /* bounding box top left y */
    HP_SInt32 x2Val,                       /* bounding box bottom right x */
    HP_SInt32 y2Val,                       /* bounding box bottom right y */
    HP_SInt32 StartPointX,                 /* starting point x */
    HP_SInt32 StartPointY,                 /* starting point y */
    HP_SInt32 EndPointX,                   /* ending point x */
    HP_SInt32 EndPointY)                   /* ending point y */
{
   HP_AttrBoxSInt32(pStream, x1Val, y1Val,x2Val,y2Val);
   HP_AttrId(pStream, HP_BoundingBox);
   HP_AttrXySInt32(pStream, StartPointX, StartPointY);
   HP_AttrId(pStream, HP_StartPoint);
   HP_AttrXySInt32(pStream, EndPointX, EndPointY);
   HP_AttrId(pStream, HP_EndPoint);
   HP_Operator(pStream, HP_PiePath);
}




HP_StdFuncPrefix HP_PiePath_4(             /* PiePath signature 4 */

	HP_StreamHandleType pStream,           /* stream handle */
    HP_Real32 x1Val,                       /* bounding box top left x */
    HP_Real32 y1Val,                       /* bounding box top left y */
    HP_Real32 x2Val,                       /* bounding box bottom right x */
    HP_Real32 y2Val,                       /* bounding box bottom right y */
    HP_Real32 StartPointX,                 /* starting point x */
    HP_Real32 StartPointY,                 /* starting point y */
    HP_Real32 EndPointX,                   /* ending point x */
    HP_Real32 EndPointY)                   /* ending point y */
{
   HP_AttrBoxReal32(pStream, x1Val, y1Val,x2Val,y2Val);
   HP_AttrId(pStream, HP_BoundingBox);
   HP_AttrXyReal32(pStream, StartPointX, StartPointY);
   HP_AttrId(pStream, HP_StartPoint);
   HP_AttrXyReal32(pStream, EndPointX, EndPointY);
   HP_AttrId(pStream, HP_EndPoint);
   HP_Operator(pStream, HP_PiePath);
}




HP_StdFuncPrefix HP_PopGS_1(               /* PopGS signature 1 */
    
    HP_StreamHandleType pStream)           /* stream handle */
{
    HP_Operator(pStream,HP_PopGS);
}




HP_StdFuncPrefix HP_PushGS_1(              /* PushGS signature 1 */

    HP_StreamHandleType pStream)           /* stream handle */ 
{
    HP_Operator(pStream,HP_PushGS);
}


HP_StdFuncPrefix HP_Query_1(               /* Query signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_pUByte queryKey,                    /* array for query string */
    HP_UInt16 queryKeyLen)                 /* length of query string array */
{

    HP_AttrUByteArray(pStream, queryKey, (HP_UInt32) queryKeyLen);
    HP_AttrId(pStream, HP_QueryKey);
    HP_Operator(pStream,HP_Query);
}


HP_StdFuncPrefix HP_ReadChar_1(            /* ReadChar signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UInt16 CharCodeData,                /* character code value */
    HP_UInt32 CharSize)                    /* length of char code data */
{
    HP_AttrUInt16(pStream, CharCodeData);
    HP_AttrId(pStream, HP_CharCode);
    HP_AttrUInt32(pStream,  CharSize);
    HP_AttrId(pStream, HP_CharDataSize);
    HP_Operator(pStream,HP_ReadChar);
}

                                                  
HP_StdFuncPrefix HP_ReadFontHeader_1(      /* ReadFontHeader signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UInt16 FontHeaderLen)               /* font header length */
{
    HP_AttrUInt16(pStream, FontHeaderLen);
    HP_AttrId(pStream, HP_FontHeaderLength);
    HP_Operator(pStream,HP_ReadFontHeader);
}


HP_StdFuncPrefix HP_ReadImage_1(           /* ReadImage signature 1 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_UInt16 startLine,                   /* start line for reading raster */
	HP_UInt16 blockHeight,                 /* line height of block */
	HP_UByte  compressionEnum)             /* compression mode enumeration */
{
   HP_AttrUInt16(pStream, startLine);
   HP_AttrId(pStream,HP_StartLine);
   HP_AttrUInt16(pStream, blockHeight);
   HP_AttrId(pStream, HP_BlockHeight);
   HP_AttrUByte(pStream,compressionEnum);
   HP_AttrId(pStream, HP_CompressMode);
   HP_Operator(pStream, HP_ReadImage);
}



HP_StdFuncPrefix HP_ReadImage_2(           /* ReadImage signature 2 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_UInt32 startLine,                   /* start line for reading raster */
	HP_UInt32 blockHeight,                 /* line height of block */
	HP_UByte  compressionEnum)             /* compression mode enumeration */
{
   HP_AttrUInt32(pStream, startLine);
   HP_AttrId(pStream,HP_StartLine);
   HP_AttrUInt32(pStream, blockHeight);
   HP_AttrId(pStream, HP_BlockHeight);
   HP_AttrUByte(pStream,compressionEnum);
   HP_AttrId(pStream, HP_CompressMode);
   HP_Operator(pStream, HP_ReadImage);
}



HP_StdFuncPrefix HP_ReadRastPattern_1(     /* ReadRastPattern signature 1 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_UInt16 startLine,                   /* start line for reading raster */
	HP_UInt16 blockHeight,                 /* line height of block */
	HP_UByte  compressionEnum)             /* compression mode enumeration */
{
   HP_AttrUInt16(pStream, startLine);
   HP_AttrId(pStream,HP_StartLine);
   HP_AttrUInt16(pStream, blockHeight);
   HP_AttrId(pStream, HP_BlockHeight);
   HP_AttrUByte(pStream,compressionEnum);
   HP_AttrId(pStream, HP_CompressMode);
   HP_Operator(pStream, HP_ReadRastPattern);
}




HP_StdFuncPrefix HP_ReadRastPattern_2(     /* ReadRastPattern signature 2 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_UInt32 startLine,                   /* start line for reading raster */
	HP_UInt32 blockHeight,                 /* line height of block */
	HP_UByte  compressionEnum)             /* compression mode enumeration */
{
   HP_AttrUInt32(pStream, startLine);
   HP_AttrId(pStream,HP_StartLine);
   HP_AttrUInt32(pStream, blockHeight);
   HP_AttrId(pStream, HP_BlockHeight);
   HP_AttrUByte(pStream,compressionEnum);
   HP_AttrId(pStream, HP_CompressMode);
   HP_Operator(pStream, HP_ReadRastPattern);
}


HP_StdFuncPrefix HP_ReadStream_1(          /* ReadStream signature 1 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_UInt32 streamDataLength)            /* stream data length */
{

	HP_AttrUInt32(pStream, streamDataLength);
	HP_AttrId(pStream, HP_StreamDataLength);
	HP_Operator(pStream, HP_ReadStream);

}


HP_StdFuncPrefix HP_ExecStream_1(          /* ExecStream signature 1 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_UInt32 streamDataLength)            /* stream data length */
{

	HP_AttrUInt32(pStream, streamDataLength);
	HP_AttrId(pStream, HP_StreamDataLength);
	HP_Operator(pStream, HP_ExecStream);
}


HP_StdFuncPrefix HP_RemoveStream_1(          /* RemoveStream signature 1 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_UInt32 streamDataLength)            /* stream data length */
{

	HP_AttrUInt32(pStream, streamDataLength);
	HP_AttrId(pStream, HP_StreamDataLength);
	HP_Operator(pStream, HP_RemoveStream);

}


HP_StdFuncPrefix HP_Rectangle_1(           /* Rectangle signature 1 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_SInt16 x1Val,                       /* bounding box top left x */
	HP_SInt16 y1Val,                       /* bounding box top left y */
	HP_SInt16 x2Val,                       /* bounding box bottom right x */
	HP_SInt16 y2Val)                       /* bounding box bottom right y */
{
   HP_AttrBoxSInt16(pStream, x1Val, y1Val, x2Val, y2Val);
   HP_AttrId(pStream, HP_BoundingBox);
   HP_Operator(pStream, HP_Rectangle);
}



HP_StdFuncPrefix HP_Rectangle_2(           /* Rectangle signature 2 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_UInt16 x1Val,                       /* bounding box top left x */
	HP_UInt16 y1Val,                       /* bounding box top left y */
	HP_UInt16 x2Val,                       /* bounding box bottom right x */
	HP_UInt16 y2Val)                       /* bounding box bottom right y */
{
   HP_AttrBoxUInt16(pStream, x1Val, y1Val, x2Val, y2Val);
   HP_AttrId(pStream, HP_BoundingBox);
   HP_Operator(pStream, HP_Rectangle);
}



HP_StdFuncPrefix HP_Rectangle_3(           /* Rectangle signature 3 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_SInt32 x1Val,                       /* bounding box top left x */
    HP_SInt32 y1Val,                       /* bounding box top left y */
    HP_SInt32 x2Val,                       /* bounding box bottom right x */
    HP_SInt32 y2Val)                       /* bounding box bottom right y */
{
   HP_AttrBoxSInt32(pStream, x1Val, y1Val, x2Val, y2Val);
   HP_AttrId(pStream, HP_BoundingBox);
   HP_Operator(pStream, HP_Rectangle);
}

HP_StdFuncPrefix HP_Rectangle_4(           /* Rectangle signature 4 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_Real32 x1Val,                       /* bounding box top left x */
    HP_Real32 y1Val,                       /* bounding box top left y */
	HP_Real32 x2Val,                       /* bounding box bottom right x */
    HP_Real32 y2Val)                       /* bounding box bottom right y */
{
   HP_AttrBoxReal32(pStream, x1Val, y1Val, x2Val, y2Val);
   HP_AttrId(pStream, HP_BoundingBox);
   HP_Operator(pStream, HP_Rectangle);
}



HP_StdFuncPrefix HP_RectanglePath_1(       /* RectanglePath signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_SInt16 x1Val,                       /* bounding box top left x */
    HP_SInt16 y1Val,                       /* bounding box top left y */
    HP_SInt16 x2Val,                       /* bounding box bottom right x */
    HP_SInt16 y2Val)                       /* bounding box bottom right y */
{
   HP_AttrBoxSInt16(pStream, x1Val, y1Val, x2Val, y2Val);
   HP_AttrId(pStream, HP_BoundingBox);
   HP_Operator(pStream, HP_RectanglePath);
}



HP_StdFuncPrefix HP_RectanglePath_2(       /* RectanglePath signature 2 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UInt16 x1Val,                       /* bounding box top left x */
    HP_UInt16 y1Val,                       /* bounding box top left y */
    HP_UInt16 x2Val,                       /* bounding box bottom right x */
    HP_UInt16 y2Val)                       /* bounding box bottom right y */
{
   HP_AttrBoxUInt16(pStream, x1Val, y1Val, x2Val, y2Val);
   HP_AttrId(pStream, HP_BoundingBox);
   HP_Operator(pStream, HP_RectanglePath);
}



HP_StdFuncPrefix HP_RectanglePath_3(       /* RectanglePath signature 3 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_SInt32 x1Val,                       /* bounding box top left x */
    HP_SInt32 y1Val,                       /* bounding box top left y */
	HP_SInt32 x2Val,                       /* bounding box bottom right x */
    HP_SInt32 y2Val)                       /* bounding box bottom right y */
{
   HP_AttrBoxSInt32(pStream, x1Val, y1Val, x2Val, y2Val);
   HP_AttrId(pStream, HP_BoundingBox);
   HP_Operator(pStream, HP_RectanglePath);
}



HP_StdFuncPrefix HP_RectanglePath_4(       /* RectanglePath signature 4 */

    HP_StreamHandleType pStream,           /* stream handle */
	HP_Real32 x1Val,                       /* bounding box top left x */
	HP_Real32 y1Val,                       /* bounding box top left y */
    HP_Real32 x2Val,                       /* bounding box bottom right x */
    HP_Real32 y2Val)                       /* bounding box bottom right y */
{
   HP_AttrBoxReal32(pStream, x1Val, y1Val, x2Val, y2Val);
   HP_AttrId(pStream, HP_BoundingBox);
   HP_Operator(pStream, HP_RectanglePath);
}



HP_StdFuncPrefix HP_RemoveFont_1(          /* RemoveFont signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_pUByte fontName,                    /* array for font name */
    HP_UInt16 fontNameLen)                 /* length of font name array */
{
    HP_AttrUByteArray(pStream, fontName, (HP_UInt32) fontNameLen);
    HP_AttrId(pStream, HP_FontName);
    HP_Operator(pStream,HP_RemoveFont);
}

          


HP_StdFuncPrefix HP_RoundRectangle_1(      /* RoundRectangle signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_SInt16 x1Val,                       /* bounding box top left x */
    HP_SInt16 y1Val,                       /* bounding box top left y */
    HP_SInt16 x2Val,                       /* bounding box bottom right x */
    HP_SInt16 y2Val,                       /* bounding box bottom right y */
    HP_UInt16 xEllipse,                    /* ellipse x dimension */
    HP_UInt16 yEllipse)                    /* ellipse y dimension */
{
   HP_AttrBoxSInt16(pStream, x1Val, y1Val, x2Val, y2Val);
   HP_AttrId(pStream, HP_BoundingBox);                               
   HP_AttrXyUInt16(pStream, xEllipse, yEllipse);
   HP_AttrId(pStream, HP_EllipseDimension);
   HP_Operator(pStream, HP_RoundRectangle);
}



HP_StdFuncPrefix HP_RoundRectangle_2(      /* RoundRectangle signature 2 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UInt16 x1Val,                       /* bounding box top left x */
    HP_UInt16 y1Val,                       /* bounding box top left y */
    HP_UInt16 x2Val,                       /* bounding box bottom right x */
	HP_UInt16 y2Val,                       /* bounding box bottom right y */
	HP_UInt16 xEllipse,                    /* ellipse x dimension */
    HP_UInt16 yEllipse)                    /* ellipse y dimension */
{
   HP_AttrBoxUInt16(pStream, x1Val, y1Val, x2Val, y2Val);
   HP_AttrId(pStream, HP_BoundingBox);                               
   HP_AttrXyUInt16(pStream, xEllipse, yEllipse);
   HP_AttrId(pStream, HP_EllipseDimension);
   HP_Operator(pStream, HP_RoundRectangle);
}




HP_StdFuncPrefix HP_RoundRectangle_3(      /* RoundRectangle signature 2 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_SInt32 x1Val,                       /* bounding box top left x */
    HP_SInt32 y1Val,                       /* bounding box top left y */
	HP_SInt32 x2Val,                       /* bounding box bottom right x */
    HP_SInt32 y2Val,                       /* bounding box bottom right y */
    HP_UInt32 xEllipse,                    /* ellipse x dimension */
    HP_UInt32 yEllipse)                    /* ellipse y dimension */
{
   HP_AttrBoxSInt32(pStream, x1Val, y1Val, x2Val, y2Val);
   HP_AttrId(pStream, HP_BoundingBox);                               
   HP_AttrXyUInt32(pStream, xEllipse, yEllipse);
   HP_AttrId(pStream, HP_EllipseDimension);
   HP_Operator(pStream, HP_RoundRectangle);
}




HP_StdFuncPrefix HP_RoundRectangle_4(      /* RoundRectangle signature 2 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_Real32 x1Val,                       /* bounding box top left x */
	HP_Real32 y1Val,                       /* bounding box top left y */
    HP_Real32 x2Val,                       /* bounding box bottom right x */
    HP_Real32 y2Val,                       /* bounding box bottom right y */
    HP_Real32 xEllipse,                    /* ellipse x dimension */
    HP_Real32 yEllipse)                    /* ellipse y dimension */
{
   HP_AttrBoxReal32(pStream, x1Val, y1Val, x2Val, y2Val);
   HP_AttrId(pStream, HP_BoundingBox);                               
   HP_AttrXyReal32(pStream, xEllipse, yEllipse);
   HP_AttrId(pStream, HP_EllipseDimension);
   HP_Operator(pStream, HP_RoundRectangle);
}



HP_StdFuncPrefix HP_RoundRectanglePath_1(  /* RoundRectanglePath signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_SInt16 x1Val,                       /* bounding box top left x */
	HP_SInt16 y1Val,                       /* bounding box top left y */
    HP_SInt16 x2Val,                       /* bounding box bottom right x */
    HP_SInt16 y2Val,                       /* bounding box bottom right y */
    HP_UInt16 xEllipse,                    /* ellipse x dimension */
    HP_UInt16 yEllipse)                    /* ellipse y dimension */
{
   HP_AttrBoxSInt16(pStream, x1Val, y1Val, x2Val, y2Val);
   HP_AttrId(pStream, HP_BoundingBox);                               
   HP_AttrXyUInt16(pStream, xEllipse, yEllipse);
   HP_AttrId(pStream, HP_EllipseDimension);
   HP_Operator(pStream, HP_RoundRectanglePath);
}



HP_StdFuncPrefix HP_RoundRectanglePath_2(  /* RoundRectanglePath signature 2 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UInt16 x1Val,                       /* bounding box top left x */
    HP_UInt16 y1Val,                       /* bounding box top left y */
    HP_UInt16 x2Val,                       /* bounding box bottom right x */
    HP_UInt16 y2Val,                       /* bounding box bottom right y */
    HP_UInt16 xEllipse,                    /* ellipse x dimension */
    HP_UInt16 yEllipse)                    /* ellipse y dimension */
{
   HP_AttrBoxUInt16(pStream, x1Val, y1Val, x2Val, y2Val);
   HP_AttrId(pStream, HP_BoundingBox);
   HP_AttrXyUInt16(pStream, xEllipse, yEllipse);
   HP_AttrId(pStream, HP_EllipseDimension);
   HP_Operator(pStream, HP_RoundRectanglePath);
}




HP_StdFuncPrefix HP_RoundRectanglePath_3(  /* RoundRectanglePath signature 2 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_SInt32 x1Val,                       /* bounding box top left x */
    HP_SInt32 y1Val,                       /* bounding box top left y */
    HP_SInt32 x2Val,                       /* bounding box bottom right x */
    HP_SInt32 y2Val,                       /* bounding box bottom right y */
    HP_SInt32 xEllipse,                    /* ellipse x dimension */
    HP_SInt32 yEllipse)                    /* ellipse y dimension */
{
   HP_AttrBoxSInt32(pStream, x1Val, y1Val, x2Val, y2Val);
   HP_AttrId(pStream, HP_BoundingBox);
   HP_AttrXySInt32(pStream, xEllipse, yEllipse);
   HP_AttrId(pStream, HP_EllipseDimension);
   HP_Operator(pStream, HP_RoundRectanglePath);
}




HP_StdFuncPrefix HP_RoundRectanglePath_4(  /* RoundRectanglePath signature 2 */

    HP_StreamHandleType pStream,           /* stream handle */
	HP_Real32 x1Val,                       /* bounding box top left x */
    HP_Real32 y1Val,                       /* bounding box top left y */
    HP_Real32 x2Val,                       /* bounding box bottom right x */
    HP_Real32 y2Val,                       /* bounding box bottom right y */
    HP_Real32 xEllipse,                    /* ellipse x dimension */
    HP_Real32 yEllipse)                    /* ellipse y dimension */
{
   HP_AttrBoxReal32(pStream, x1Val, y1Val, x2Val, y2Val);
   HP_AttrId(pStream, HP_BoundingBox);                               
   HP_AttrXyReal32(pStream, xEllipse, yEllipse);
   HP_AttrId(pStream, HP_EllipseDimension);
   HP_Operator(pStream, HP_RoundRectanglePath);
}


#define HP_EMBEDDED_SCAN_PREFIX_LEN 7 /* 7 bytes of prefix in embedded
                                       * data per scan line */                   
#define HP_SCAN_BUFFER_PREFIX_LEN 4 /* 4 words of prefix in buffer per
                                     * scan line */

HP_StdFuncPrefix HP_ScanLineRel_1(         /* ScanLineRel signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_SInt16 yStart,                      /* y start for scan line */
    HP_SInt16 xStart,                      /* x start for scan line */
	HP_UInt16 *XPairArray,                 /* array of x pairs */
    HP_UInt16 numberOfXPairs)              /* number of x pairs in scan line */
{
   int useByteArray = 1;
   
   {
       HP_UInt16 i = 0;

       /* check to see if all x-pairs may be expressed as ubytes: */
       for (i=0; (i < (numberOfXPairs*2)) && (useByteArray); i++)
       {
           if (XPairArray[i]>255) useByteArray = 0;
       } 

   }
   
   /* do ScanLineRel operator: */
   HP_Operator(pStream, HP_ScanLineRel);
                
   /* format embedded data length: */
   HP_EmbeddedDataPrefix(pStream,
           HP_EMBEDDED_SCAN_PREFIX_LEN + 1 /* XStart, YStart data type */ + 
           ((useByteArray) ? (numberOfXPairs * 2) : (numberOfXPairs * 4)));
   
   HP_RawUByte(pStream, HP_eSInt16); /* type of XStart, YStart */ 
   
   /* output one scan line: */
   HP_RawUInt16(pStream, (HP_UInt16) xStart);
   HP_RawUInt16(pStream, (HP_UInt16) yStart);
   HP_RawUInt16(pStream, numberOfXPairs);
   HP_RawUByte(pStream, (HP_UByte) ((useByteArray) ? (HP_eUByte) : (HP_eUInt16)));
       
   if (useByteArray)
   {
       HP_RawUInt16ToByteArray(pStream, XPairArray, numberOfXPairs * 2);
   }
   else
   {
       HP_RawUInt16Array(pStream, XPairArray, numberOfXPairs * 2);
   }


}
#define HP_SCAN_BUFF_LEN_POS 0
#define HP_SCAN_BUFF_INDEX_POS 1
#define HP_SCAN_BUFF_START_POS 2
#define HP_SCAN_BUFF_DATALEN_POS 3
#define HP_SCAN_BUFF_LINES_POS 4

#define HP_SCAN_BUFF_GetBuffLen(scanLineBuff) \
        scanLineBuff[HP_SCAN_BUFF_LEN_POS]  
#define HP_SCAN_BUFF_GetIndex(scanLineBuff) \
        scanLineBuff[HP_SCAN_BUFF_INDEX_POS]  
#define HP_SCAN_BUFF_SetIndex(scanLineBuff, value) \
        scanLineBuff[HP_SCAN_BUFF_INDEX_POS] = value 
#define HP_SCAN_BUFF_GetStartPosition(scanLineBuff) \
        scanLineBuff[HP_SCAN_BUFF_START_POS]   
#define HP_SCAN_BUFF_GetDataLength(scanLineBuff) \
        scanLineBuff[HP_SCAN_BUFF_DATALEN_POS]  
#define HP_SCAN_BUFF_SetDataLength(scanLineBuff, value) \
        scanLineBuff[HP_SCAN_BUFF_DATALEN_POS] = value
#define HP_SCAN_BUFF_SetNumberOfLines(scanLineBuff, value) \
        scanLineBuff[HP_SCAN_BUFF_LINES_POS] = value
#define HP_SCAN_BUFF_GetNumberOfLines(scanLineBuff) \
		scanLineBuff[HP_SCAN_BUFF_LINES_POS]
#define HP_SCAN_BUFF_IncrementNumberOfLines(scanLineBuff) \
             scanLineBuff[HP_SCAN_BUFF_LINES_POS] = scanLineBuff[HP_SCAN_BUFF_LINES_POS] + 1
#define HP_SCAN_BUFF_SetElement(scanLineBuff, value, index) \
             scanLineBuff[index] = value                
#define HP_SCAN_BUFF_GetElement(scanLineBuff, index) \
             scanLineBuff[index]                 
#define HP_SCAN_BUFF_IncrementIndex(scanLineBuff) \
        scanLineBuff[HP_SCAN_BUFF_INDEX_POS] = scanLineBuff[HP_SCAN_BUFF_INDEX_POS] + 1

HP_StdFuncPrefix HP_ScanLineRelBuffInit_1( /* ScanLineRelBuffInit signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
	HP_UInt16 *scanLineBuff,               /* scan line buffer */
    HP_UInt16 scanLineBuffLen)             /* scan line buffer len */
{
#define HP_SCAN_BUFF_SetStartPosition(scanLineBuff) scanLineBuff[2] = 5
#define HP_SCAN_BUFF_SetBuffLen(scanLineBuff, value) scanLineBuff[0] = value 
#define HP_SCAN_BUFF_MIN_LEN 10
    
    if (scanLineBuffLen >= HP_SCAN_BUFF_MIN_LEN)
    {                                                                        
        HP_SCAN_BUFF_SetBuffLen(scanLineBuff, scanLineBuffLen);
        
        HP_SCAN_BUFF_SetStartPosition(scanLineBuff);
    
        HP_SCAN_BUFF_SetIndex(scanLineBuff, 
            HP_SCAN_BUFF_GetStartPosition(scanLineBuff));
        
        HP_SCAN_BUFF_SetDataLength(scanLineBuff, 1 /* XStart, YStart type */);                       
    
        HP_SCAN_BUFF_SetNumberOfLines(scanLineBuff, 0);
    }   
    else
    {
        scanLineBuff[0] = 0;
    }

#undef HP_SCAN_BUFF_MIN_LEN
#undef HP_SCAN_BUFF_SetStartPosition
}

HP_StdFuncPrefix HP_ScanLineRelBuffFlush_1( /* ScanLineRelBuffFlush signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
	HP_UInt16 *scanLineBuff)               /* scan line buffer */
{
    
    HP_UInt16 position = HP_SCAN_BUFF_GetStartPosition(scanLineBuff);
    HP_UInt16 lastPosition = HP_SCAN_BUFF_GetIndex(scanLineBuff);
    HP_UInt16 dataLength = HP_SCAN_BUFF_GetDataLength(scanLineBuff);
	HP_UInt16 numberOfLines = HP_SCAN_BUFF_GetNumberOfLines(scanLineBuff);
	HP_UInt16 scanLineBuffLen = HP_SCAN_BUFF_GetBuffLen(scanLineBuff);
    HP_UInt16 XPairType;                 
    HP_UInt16 numberOfXPairs;
    
    if (numberOfLines && scanLineBuff[0]/*scan line buff initialized?*/)
    {
        HP_UInt16 XStart;
        HP_UInt16 YStart;

        /* do BeginScan operator: */
        HP_Operator(pStream, HP_BeginScan);
                
        /* do NumberOfLines attribute if more than one scan line: */
        if (numberOfLines > 1)
        {
            HP_AttrUInt16(pStream, numberOfLines);
            HP_AttrId(pStream, HP_NumberOfScanLines);
        }                                           
        
		/* do ScanLineRel operator: */
        HP_Operator(pStream, HP_ScanLineRel);
                
        /* do embedded data tag: */
        HP_EmbeddedDataPrefix(pStream, dataLength);
        
		HP_RawUByte(pStream, HP_eSInt16); /* data type of XStart, YStart */
        
        /* scan line data: */
        while (position < lastPosition)
        {
            XStart = HP_SCAN_BUFF_GetElement(scanLineBuff, position++);
            HP_RawUInt16(pStream, XStart);

            YStart = HP_SCAN_BUFF_GetElement(scanLineBuff, position++); 
            HP_RawUInt16(pStream, YStart);
            
            numberOfXPairs = HP_SCAN_BUFF_GetElement(scanLineBuff, position++);
			HP_RawUInt16(pStream, numberOfXPairs);

            XPairType = HP_SCAN_BUFF_GetElement(scanLineBuff, position++);
            HP_RawUByte(pStream, (HP_UByte) XPairType);
            
            if (XPairType == HP_eUByte)
            {
                HP_RawUInt16ToByteArray(pStream, &(scanLineBuff[position]), 
                    numberOfXPairs * 2);
            }
            else
            {
                HP_RawUInt16Array(pStream, &(scanLineBuff[position]), 
				numberOfXPairs * 2);
			}
            
            position = position + (numberOfXPairs * 2);
            
        }                                              
        
		HP_ScanLineRelBuffInit_1(pStream, scanLineBuff,
            HP_SCAN_BUFF_GetBuffLen(scanLineBuff));
        
        /* do EndScan operator: */
        HP_Operator(pStream, HP_EndScan);
                
    }
            
}

HP_StdFuncPrefix HP_ScanLineRelBuffAdd_1( /* ScanLineRelBuffAdd signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UInt16 yStart,                      /* y start for scan line */
    HP_UInt16 xStart,                      /* x start for scan line */         
    HP_UInt16 *XPairArray,                  /* x pair array */
    HP_UInt16 numberOfXPairs,              /* number of x pairs in scan line */
    HP_UInt16 *scanLineBuff)               /* scan line buffer */
{
    
    HP_UInt16 saveIndex = 0;
    HP_UInt16 scanLineBuffLen = HP_SCAN_BUFF_GetBuffLen(scanLineBuff);
    
    if ((scanLineBuffLen - HP_SCAN_BUFF_GetIndex(scanLineBuff)) <= 
        (HP_SCAN_BUFFER_PREFIX_LEN + (numberOfXPairs * 2)))
    {
		HP_ScanLineRelBuffFlush_1(pStream, scanLineBuff);
    } 

    if ((scanLineBuffLen - HP_SCAN_BUFF_GetIndex(scanLineBuff)) > 
        (HP_SCAN_BUFFER_PREFIX_LEN + (numberOfXPairs * 2)))
	{
		HP_UInt16 position = HP_SCAN_BUFF_GetIndex(scanLineBuff);
        
        HP_SCAN_BUFF_SetElement(scanLineBuff, (HP_UInt16) xStart, position++);
        HP_SCAN_BUFF_SetElement(scanLineBuff, (HP_UInt16) yStart, position++);
        HP_SCAN_BUFF_SetElement(scanLineBuff, 
            (HP_UInt16) numberOfXPairs, position++);
        saveIndex = position++; /* save index for setting x-pair type */
         
        {
            HP_UInt16 i;
            int useBytes = 1;
            
			for (i=0; i<(numberOfXPairs*2); i++)
			{
                HP_SCAN_BUFF_SetElement(scanLineBuff, 
                    (HP_UInt16) XPairArray[i], position++);
                    
                if (XPairArray[i] > 255) useBytes = 0;
            }
            
            if (useBytes) 
            {
                /* set the type of x-pairs to the saved index */
                HP_SCAN_BUFF_SetElement(scanLineBuff, HP_eUByte, saveIndex);
                /* increment the embedded data length field for byte pairs */
                HP_SCAN_BUFF_SetDataLength(scanLineBuff,
					   (HP_EMBEDDED_SCAN_PREFIX_LEN +
                       (numberOfXPairs * 2)) + 
                       HP_SCAN_BUFF_GetDataLength(scanLineBuff));
            }
            else
            {
				/* set the type of x-pairs to the saved index */
                HP_SCAN_BUFF_SetElement(scanLineBuff, HP_eUInt16, saveIndex);
                /* increment the embedded data length field for uint16 pairs */
                HP_SCAN_BUFF_SetDataLength(scanLineBuff,
                       (HP_EMBEDDED_SCAN_PREFIX_LEN + 
                       (numberOfXPairs * 4)) + 
                       HP_SCAN_BUFF_GetDataLength(scanLineBuff));
            }
            
        }                                                           
        
        HP_SCAN_BUFF_SetIndex(scanLineBuff, position);
        
        HP_SCAN_BUFF_IncrementNumberOfLines(scanLineBuff);
        
    }
    else
    {
		/* do BeginScan operator: */
        HP_Operator(pStream, HP_BeginScan);
                
        /* scan line buffer not large enough, just output as single scan line */
        HP_ScanLineRel_1(pStream, xStart, yStart, XPairArray, numberOfXPairs);

        /* do EndScan operator: */
        HP_Operator(pStream, HP_EndScan);

    }
    
}     

#undef HP_EMBEDDED_SCAN_PREFIX_LEN

HP_StdFuncPrefix HP_SetBrushSource_Gray1(  /* SetBrushSource Gray sig. 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_Real32 grayLevel)                   /* gray level value (0-1.0) */
{
   HP_AttrReal32(pStream, grayLevel);
   HP_AttrId(pStream, HP_GrayLevel);
   HP_Operator(pStream, HP_SetBrushSource);

}


HP_StdFuncPrefix HP_SetBrushSource_Gray2(  /* SetBrushSource Gray sig. 2 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UByte grayLevel)                   /* gray level value (0-255) */
{
   HP_AttrUByte(pStream, grayLevel);
   HP_AttrId(pStream, HP_GrayLevel);
   HP_Operator(pStream, HP_SetBrushSource);

}


HP_StdFuncPrefix HP_SetBrushSource_Null1(  /* SetBrushSource Null sig. 1 */

    HP_StreamHandleType pStream)           /* stream handle */
{
   HP_AttrUByte(pStream, 0);
   HP_AttrId(pStream, HP_NullBrush);
   HP_Operator(pStream, HP_SetBrushSource);
}



HP_StdFuncPrefix HP_SetBrushSource_Patt1(  /* SetBrushSource Pattern sig. 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_SInt16 patternSelectID)             /* pattern select identifier */
{
   HP_AttrSInt16(pStream, patternSelectID);
   HP_AttrId(pStream, HP_PatternSelectID);
   HP_Operator(pStream, HP_SetBrushSource);

}



HP_StdFuncPrefix HP_SetBrushSource_Patt2(  /* SetBurshSource Pattern sig. 1 */

	HP_StreamHandleType pStream,           /* stream handle */
    HP_SInt16 xExtent,                     /* pattern dest x extent */
    HP_SInt16 yExtent,                     /* pattern dest y extent */
    HP_SInt16 patternSelectID)             /* pattern select identifier */
{
   HP_AttrXyUInt16(pStream, xExtent, yExtent);
   HP_AttrId(pStream, HP_NewDestinationSize);
   HP_AttrSInt16(pStream, patternSelectID);
   HP_AttrId(pStream, HP_PatternSelectID);
   HP_Operator(pStream, HP_SetBrushSource);

}


HP_StdFuncPrefix HP_SetBrushSource_RGB1(   /* SetBrushSource RGB sig. 1 */
    
    HP_StreamHandleType pStream,           /* stream handle */
    HP_Real32 redValue,                    /* red value */
    HP_Real32 greenValue,                  /* green value */
    HP_Real32 blueValue)                   /* blue value */
{
#define HP_RGB_COLOR_LEN 3

   HP_Real32 RGB_Array[HP_RGB_COLOR_LEN];

   RGB_Array[0] = redValue;
   RGB_Array[1] = greenValue;
   RGB_Array[2] = blueValue;

   HP_AttrReal32Array(pStream, RGB_Array, HP_RGB_COLOR_LEN);
   HP_AttrId(pStream, HP_RGBColor);
   HP_Operator(pStream, HP_SetBrushSource);

#undef HP_RGB_COLOR_LEN 
}

HP_StdFuncPrefix HP_SetBrushSource_RGB2(   /* SetBrushSource RGB sig. 2 */

    HP_StreamHandleType pStream,           /* stream handle */
	HP_UByte redValue,                     /* red value */
    HP_UByte greenValue,                   /* green value */
    HP_UByte blueValue)                    /* blue value */
{
#define HP_RGB_COLOR_LEN 3

   HP_UByte RGB_Array[HP_RGB_COLOR_LEN];

   RGB_Array[0] = redValue;
   RGB_Array[1] = greenValue;
   RGB_Array[2] = blueValue;

   HP_AttrUByteArray(pStream, RGB_Array, HP_RGB_COLOR_LEN);
   HP_AttrId(pStream, HP_RGBColor);
   HP_Operator(pStream, HP_SetBrushSource);

#undef HP_RGB_COLOR_LEN
}

/* PCL XL 2.0 */

HP_StdFuncPrefix HP_SetBrushSource_Primary(       /* SetBrushSource Primary */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_UByte colorSpaceEnum,               /* Primary Depth */
	HP_pUByte colorData,                   /* array for color data */
	HP_UInt16 colorDataLen)                /* length of color data array in bytes */
{

  HP_AttrUByte(pStream, colorSpaceEnum);
  HP_AttrId(pStream,  HP_PrimaryDepth);    /* PrimaryDepth */
  HP_AttrUByteArray(pStream, colorData, (HP_UInt32) colorDataLen);
  HP_AttrId(pStream,HP_PrimaryArray);      /* PrimaryArray */
  HP_Operator(pStream,HP_SetBrushSource);
}


HP_StdFuncPrefix HP_SetCharAngle_1(        /* SetCharAngle signature 1 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_SInt16 charAngle)                   /* character angle */
{
   HP_AttrSInt16(pStream, charAngle);
   HP_AttrId(pStream, HP_CharAngle);
   HP_Operator(pStream, HP_SetCharAngle);
}




HP_StdFuncPrefix HP_SetCharAngle_2(        /* SetCharAngle signature 2 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_Real32 charAngle)                   /* character angle */
{
   HP_AttrReal32(pStream, charAngle);
   HP_AttrId(pStream, HP_CharAngle);
   HP_Operator(pStream, HP_SetCharAngle);
}




HP_StdFuncPrefix HP_SetCharScale_1(        /* SetCharScale signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_Real32 xVal,                        /* character x scale value */
    HP_Real32 yVal)                        /* character y scale value */
{
   HP_AttrXyReal32(pStream, xVal, yVal);
   HP_AttrId(pStream, HP_CharScale);
   HP_Operator(pStream, HP_SetCharScale);
}




HP_StdFuncPrefix HP_SetCharShear_1(        /* SetCharShear signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_SInt16 xVal,                        /* character x shear value */
    HP_SInt16 yVal)                        /* character y shear value */
{
   HP_AttrXySInt16(pStream, xVal, yVal);
   HP_AttrId(pStream, HP_CharShear);
   HP_Operator(pStream, HP_SetCharShear);
}




HP_StdFuncPrefix HP_SetCharShear_2(        /* SetCharShear signature 2 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_Real32 xVal,                        /* character x shear value */
    HP_Real32 yVal)                        /* character y shear value */
{
   HP_AttrXyReal32(pStream, xVal, yVal);
   HP_AttrId(pStream, HP_CharShear);
   HP_Operator(pStream, HP_SetCharShear);
}

HP_StdFuncPrefix HP_SetCharAttribute_1(    /* SetCharAttribute signature 1 */
    HP_StreamHandleType pStream,           /* stream handle */
    HP_UByte writingMode)
{
   HP_AttrUByte(pStream, writingMode);
   HP_AttrId(pStream, HP_WritingMode);
   HP_Operator(pStream, HP_SetCharAttribute);
}

HP_StdFuncPrefix HP_SetClipIntersect_1(    /* SetClipIntersect signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UByte clipRegionEnum)               /* clip region enumeration */
{
   HP_AttrUByte(pStream, clipRegionEnum);
   HP_AttrId(pStream, HP_ClipRegion);
   HP_Operator(pStream, HP_SetClipIntersect);
}



HP_StdFuncPrefix HP_SetClipMode_1(         /* SetClipMode signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UByte clipModeEnum )                /* clip mode enumeration */
{
   HP_AttrUByte(pStream, clipModeEnum);
   HP_AttrId(pStream, HP_ClipMode);
   HP_Operator(pStream, HP_SetClipMode);
}



HP_StdFuncPrefix HP_SetClipRectangle_1(    /* SetClipRectangle signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_SInt16 x1Val,                       /* bounding box top left x */
    HP_SInt16 y1Val,                       /* bounding box top left y */
    HP_SInt16 x2Val,                       /* bounding box bottom right x */
    HP_SInt16 y2Val,                       /* bounding box bottom right y */
	HP_UByte clipRegionEnum)               /* clip region enumeration */
{
   HP_AttrBoxSInt16(pStream, x1Val, y1Val,x2Val,y2Val);
   HP_AttrId(pStream, HP_BoundingBox);
   HP_AttrUByte(pStream, clipRegionEnum);
   HP_AttrId(pStream, HP_ClipRegion);
   HP_Operator(pStream, HP_SetClipRectangle);
}




HP_StdFuncPrefix HP_SetClipRectangle_2(    /* SetClipRectangle signature 2 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UInt16 x1Val,                       /* bounding box top left x */
    HP_UInt16 y1Val,                       /* bounding box top left y */
	HP_UInt16 x2Val,                       /* bounding box bottom right x */
    HP_UInt16 y2Val,                       /* bounding box bottom right y */
    HP_UByte clipRegionEnum)               /* clip region enumeration */
{
   HP_AttrBoxUInt16(pStream, x1Val, y1Val,x2Val,y2Val);
   HP_AttrId(pStream, HP_BoundingBox);
   HP_AttrUByte(pStream, clipRegionEnum);
   HP_AttrId(pStream, HP_ClipRegion);
   HP_Operator(pStream, HP_SetClipRectangle);
}




HP_StdFuncPrefix HP_SetClipRectangle_3(    /* SetClipRectangle signature 3 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_SInt32 x1Val,                       /* bounding box top left x */
    HP_SInt32 y1Val,                       /* bounding box top left y */
    HP_SInt32 x2Val,                       /* bounding box bottom right x */
    HP_SInt32 y2Val,                       /* bounding box bottom right y */
    HP_UByte clipRegionEnum)               /* clip region enumeration */
{
   HP_AttrBoxSInt32(pStream, x1Val, y1Val,x2Val,y2Val);
   HP_AttrId(pStream, HP_BoundingBox);
   HP_AttrUByte(pStream, clipRegionEnum);
   HP_AttrId(pStream, HP_ClipRegion);
   HP_Operator(pStream, HP_SetClipRectangle);
}




HP_StdFuncPrefix HP_SetClipRectangle_4(    /* SetClipRectangle signature 4 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_Real32 x1Val,                       /* bounding box top left x */
    HP_Real32 y1Val,                       /* bounding box top left y */
    HP_Real32 x2Val,                       /* bounding box bottom right x */
	HP_Real32 y2Val,                       /* bounding box bottom right y */
    HP_UByte clipRegionEnum)               /* clip region enumeration */
{
   HP_AttrBoxReal32(pStream, x1Val, y1Val,x2Val,y2Val);
   HP_AttrId(pStream, HP_BoundingBox);
   HP_AttrUByte(pStream, clipRegionEnum);
   HP_AttrId(pStream, HP_ClipRegion);
   HP_Operator(pStream, HP_SetClipRectangle);
}


HP_StdFuncPrefix HP_SetClipReplace_1(      /* SetClipReplace signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UByte clipRegionEnum)               /* clip region enumeration */
{
   HP_AttrUByte(pStream, clipRegionEnum);
   HP_AttrId(pStream, HP_ClipRegion);
   HP_Operator(pStream, HP_SetClipReplace);
}



HP_StdFuncPrefix HP_SetClipToPage_1(       /* SetClipToPage signature 1 */

	HP_StreamHandleType pStream)           /* stream handle */
{
    HP_Operator(pStream, HP_SetClipToPage);
}



HP_StdFuncPrefix HP_SetColorSpace_1(       /* SetColorSpace signature 1 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_UByte colorSpaceEnum)               /* color space enumeration */
{

  HP_AttrUByte(pStream, colorSpaceEnum);
  HP_AttrId(pStream, HP_ColorSpace);
  HP_Operator(pStream,HP_SetColorSpace);
}



HP_StdFuncPrefix HP_SetColorSpace_2(       /* SetColorSpace signature 2 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_UByte colorSpaceEnum,               /* color space enumeration */
	HP_UByte paletteDepthEnum,             /* palette depth enumeration */
	HP_pUByte array,                       /* palette data array */
	HP_UInt32 arrayLen)                    /* palette data array length */
{

  HP_AttrUByte(pStream, colorSpaceEnum);
  HP_AttrId(pStream, HP_ColorSpace);
  HP_AttrUByte(pStream, paletteDepthEnum);
  HP_AttrId(pStream, HP_PaletteDepth);
  HP_AttrUByteArray(pStream, array, arrayLen);
  HP_AttrId(pStream, HP_PaletteData);
  HP_Operator(pStream,HP_SetColorSpace);
}

/*
sint16 -10 Lightness
sint16 20 Saturation
SetColorAdjustment
*/

#if 0 //~ MP
HP_StdFuncPrefix HP_SetColorAdjustment_1(       /* SetColorAdjustment Signature 6 */

	HP_StreamHandleType pStream,                /* Stream handle */
	HP_SInt16 lightness,
	HP_SInt16 saturation)
{
  HP_AttrSInt16(pStream, lightness);
  HP_AttrSInt16(pStream, saturation);
  HP_Operator(pStream,HP_SetColorAdjustment);
}
#endif

/*
ubyte eInch Measure
sint16_xy 600 600 UnitsPerMeasure
SetUserUnits
*/

HP_StdFuncPrefix HP_SetUserUnits_1(        /* SetUserUnits signature 1 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_UByte measure,
	HP_Real32 xVal,                        /* absolute x location */
	HP_Real32 yVal)                        /* absolute y location */

{
	HP_AttrUByte(pStream, measure);
	HP_AttrId(pStream, HP_Measure);
	HP_AttrXyReal32(pStream, xVal, yVal);
	HP_AttrId(pStream, HP_UnitsPerMeasure);
	HP_Operator(pStream,HP_SetCursor);
}


HP_StdFuncPrefix HP_SetCursor_1(           /* SetCursor signature 1 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_SInt16 xVal,                        /* absolute x location */
	HP_SInt16 yVal)                        /* absolute y location */

{
	HP_AttrXySInt16(pStream, xVal, yVal);
	HP_AttrId(pStream, HP_Point);
	HP_Operator(pStream,HP_SetCursor);
}


HP_StdFuncPrefix HP_SetCursor_2(           /* SetCursor signature 2 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_UInt16 xVal,                        /* absolute x location */
	HP_UInt16 yVal)                        /* absolute y location */

{
	HP_AttrXyUInt16(pStream, xVal, yVal);
	HP_AttrId(pStream, HP_Point);
	HP_Operator(pStream,HP_SetCursor);
}




HP_StdFuncPrefix HP_SetCursor_3(           /* SetCursor signature 3 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_SInt32 xVal,                        /* absolute x location */
	HP_SInt32 yVal)                        /* absolute y location */

{
	HP_AttrXySInt32(pStream, xVal, yVal);
	HP_AttrId(pStream, HP_Point);
	HP_Operator(pStream,HP_SetCursor);
}




HP_StdFuncPrefix HP_SetCursor_4(           /* SetCursor signature 4 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_Real32 xVal,                        /* absolute x location */
	HP_Real32 yVal)                        /* absolute y location */

{
	HP_AttrXyReal32(pStream, xVal, yVal);
	HP_AttrId(pStream, HP_Point);
	HP_Operator(pStream,HP_SetCursor);
}




HP_StdFuncPrefix HP_SetCursorRel_1(        /* SetCursorRel signature 1 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_SInt16 xVal,                        /* absolute x location */
	HP_SInt16 yVal)                        /* absolute y location */

{
    HP_AttrXySInt16(pStream, xVal, yVal);
    HP_AttrId(pStream, HP_Point);
    HP_Operator(pStream,HP_SetCursorRel);
}




HP_StdFuncPrefix HP_SetCursorRel_2(        /* SetCursorRel signature 2 */
    
    HP_StreamHandleType pStream,           /* stream handle */
	HP_UInt16 xVal,                        /* absolute x location */
	HP_UInt16 yVal)                        /* absolute y location */

{
    HP_AttrXyUInt16(pStream, xVal, yVal);
    HP_AttrId(pStream, HP_Point);
    HP_Operator(pStream,HP_SetCursorRel);
}




HP_StdFuncPrefix HP_SetCursorRel_3(        /* SetCursorRel signature 3 */
    
    HP_StreamHandleType pStream,           /* stream handle */
    HP_SInt32 xVal,                        /* absolute x location */
    HP_SInt32 yVal)                        /* absolute y location */

{
	HP_AttrXySInt32(pStream, xVal, yVal);
	HP_AttrId(pStream, HP_Point);
    HP_Operator(pStream,HP_SetCursorRel);
}




HP_StdFuncPrefix HP_SetCursorRel_4(        /* SetCursorRel signature 4 */
    
	HP_StreamHandleType pStream,           /* stream handle */
    HP_Real32 xVal,                        /* absolute x location */
    HP_Real32 yVal)                        /* absolute y location */

{
    HP_AttrXyReal32(pStream, xVal, yVal);
	HP_AttrId(pStream, HP_Point);
    HP_Operator(pStream,HP_SetCursorRel);
}




HP_StdFuncPrefix HP_SetHalftoneMethod_1(   /* SetHalftoneMethod signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UByte ditherMatrixEnum)             /* dither matrix enumeration */
{
   HP_AttrUByte(pStream, ditherMatrixEnum);
   HP_AttrId(pStream, HP_DeviceMatrix);
   HP_Operator(pStream, HP_SetHalftoneMethod);
}




HP_StdFuncPrefix HP_SetHalftoneMethod_2(     /* SetHalftoneMethod signature 2 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UInt16 matrixSizeX,                 /* Number of matrix X entries */
    HP_UInt16 matrixSizeY,                 /* Number of matrix Y entries */ 
    HP_UByte ditherMatrixDepth,            /* depth of the dither matrix */
	HP_UByte ditherMatrixDataType)         /* dither matrix data type */
{
   HP_AttrXyUInt16(pStream, matrixSizeX, matrixSizeY);
   HP_AttrId(pStream, HP_DitherMatrixSize);
   HP_AttrUByte(pStream, ditherMatrixDepth);
   HP_AttrId(pStream, HP_DitherMatrixDepth);
   HP_AttrUByte(pStream, ditherMatrixDataType);
   HP_AttrId(pStream, HP_DitherMatrixDataType);
   HP_Operator(pStream, HP_SetHalftoneMethod);
}




HP_StdFuncPrefix HP_SetHalftoneMethod_3(  /* SetHalftoneMethod signature 3 */

    HP_StreamHandleType pStream,          /* stream handle */
    HP_UInt16 matrixSizeX,                /* Number of matrix X entries */
    HP_UInt16 matrixSizeY,                /* Number of matrix Y entries */ 
    HP_UByte ditherMatrixDepth,           /* depth of the dither matrix */
    HP_UInt16 xVal,                        /* dither matrix origin x */
    HP_UInt16 yVal,                        /* dither matrix origin x */
    HP_UByte ditherMatrixDataType)         /* dither matrix data type */
{
   HP_AttrXyUInt16(pStream, matrixSizeX, matrixSizeY);
   HP_AttrId(pStream, HP_DitherMatrixSize);
   HP_AttrUByte(pStream, ditherMatrixDepth);
   HP_AttrId(pStream, HP_DitherMatrixDepth);
   HP_AttrUByte(pStream, ditherMatrixDataType);
   HP_AttrId(pStream, HP_DitherMatrixDataType);
   HP_AttrXyUInt16(pStream, xVal, yVal);
   HP_AttrId(pStream, HP_DitherOrigin);
   HP_Operator(pStream, HP_SetHalftoneMethod);
}


HP_StdFuncPrefix HP_SetFillMode_1(         /* SetFillMode signature 1 */

	HP_StreamHandleType pStream,           /* stream handle */
    HP_UByte fillModeEnum)                 /* fill mode enumeration */
{
   HP_AttrUByte(pStream, fillModeEnum);
   HP_AttrId(pStream, HP_FillMode);
   HP_Operator(pStream, HP_SetFillMode);
}


HP_StdFuncPrefix HP_SetFont_1(             /* SetFont signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_pUByte fontName,                    /* font name array */
    HP_UInt16 fontNameLen,                 /* font name array length */
    HP_Real32 charSize,                    /* char size in user units */
    HP_UInt16 symbolSetEnum)               /* symbol set enumeration */
{
   HP_AttrUByteArray(pStream, fontName, fontNameLen);
   HP_AttrId(pStream, HP_FontName);
   HP_AttrReal32(pStream, charSize);
   HP_AttrId(pStream, HP_CharSize);
   HP_AttrUInt16(pStream, symbolSetEnum);
   HP_AttrId(pStream, HP_SymbolSet);
   HP_Operator(pStream, HP_SetFont);
}

HP_StdFuncPrefix HP_SetCharSubMode_1(   /* SetCharSubMode signature 1 */
    HP_StreamHandleType pStream,        /* stream handle */
    HP_pUByte pSubModeArray,            /* substitution modes array */
    HP_UByte subModeArrayLength)        /* substitution modes array length */
{
   HP_AttrUByteArray(pStream, pSubModeArray, (HP_UInt32)subModeArrayLength);
   HP_AttrId(pStream, HP_CharSubModeArray);
   HP_Operator(pStream, HP_SetCharSubMode);
}

HP_StdFuncPrefix HP_SetLineCap_1(          /* SetLineCap signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UByte lineCapEnum)                  /* line cap enumeration */
{
   HP_AttrUByte(pStream, lineCapEnum);
   HP_AttrId(pStream, HP_LineCapStyle);
   HP_Operator(pStream, HP_SetLineCap);
}




HP_StdFuncPrefix HP_SetLineDash_1(         /* SetLineDash signature 1 */

	HP_StreamHandleType pStream,           /* stream handle */
    HP_pUInt16 array,                      /* line dash units array */
    HP_UInt16 arrayLen)                    /* line dash array length */
{
   HP_AttrUInt16Array(pStream, array, arrayLen);
   HP_AttrId(pStream, HP_LineDashStyle);
   HP_Operator(pStream, HP_SetLineDash);
}




HP_StdFuncPrefix HP_SetLineDash_2(         /* SetLineDash signature 2 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_pUInt16 array,                      /* line dash units array */
	HP_UInt16 arrayLen,                    /* line dash array length */
    HP_UInt16 dashOffset)                  /* offset into line dash array */
{
   HP_AttrUInt16Array(pStream, array, arrayLen);
   HP_AttrId(pStream, HP_LineDashStyle);
   HP_AttrUInt16(pStream, dashOffset);
   HP_AttrId(pStream, HP_DashOffset);
   HP_Operator(pStream, HP_SetLineDash);
}




HP_StdFuncPrefix HP_SetLineDash_Solid1(    /* SetLineDash signature Solid1 */

    HP_StreamHandleType pStream)           /* stream handle */
{
   HP_AttrUByte(pStream, 0);
   HP_AttrId(pStream, HP_SolidLine);
   HP_Operator(pStream, HP_SetLineDash);
}




HP_StdFuncPrefix HP_SetLineJoin_1(         /* SetLineJoin signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UByte lineJoinEnum)                 /* line join enumeration */
{
   HP_AttrUByte(pStream, lineJoinEnum);
   HP_AttrId(pStream, HP_LineJoinStyle);
   HP_Operator(pStream,HP_SetLineJoin);
}




HP_StdFuncPrefix HP_SetMiterLimit_1(       /* SetMiterLimit signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UInt16 miterLength)                 /* maximum miter shape length */
{
   HP_AttrUInt16(pStream, miterLength);
   HP_AttrId(pStream, HP_MiterLength);
   HP_Operator(pStream, HP_SetMiterLimit);
}



HP_StdFuncPrefix HP_SetMiterLimit_2(       /* SetMiterLimit signature 2 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_Real32 miterLength)                 /* maximum miter shape length */
{
   HP_AttrReal32(pStream, miterLength);
   HP_AttrId(pStream, HP_MiterLength);
   HP_Operator(pStream, HP_SetMiterLimit);
}




HP_StdFuncPrefix HP_SetPageDefaultCTM_1(   /* SetPageDefault signature 1 */

    HP_StreamHandleType pStream)           /* stream handle */
{
   HP_Operator(pStream, HP_SetPageDefaultCTM);
}




HP_StdFuncPrefix HP_SetPageOrigin_1(       /* SetPageOrigin signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_SInt16 xVal,                        /* new origin x value */
    HP_SInt16 yVal)                        /* new origin y value */
{
   HP_AttrXySInt16(pStream, xVal, yVal);
   HP_AttrId(pStream, HP_PageOrigin);
   HP_Operator(pStream, HP_SetPageOrigin);
}




HP_StdFuncPrefix HP_SetPageOrigin_2(       /* SetPageOrigin signature 2 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UInt16 xVal,                        /* new origin x value */
    HP_UInt16 yVal)                        /* new origin y value */
{
   HP_AttrXyUInt16(pStream, xVal, yVal);
   HP_AttrId(pStream, HP_PageOrigin);
   HP_Operator(pStream, HP_SetPageOrigin);
}




HP_StdFuncPrefix HP_SetPageOrigin_3(       /* SetPageOrigin signature 3 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_SInt32 xVal,                        /* new origin x value */
    HP_SInt32 yVal)                        /* new origin y value */
{
   HP_AttrXySInt32(pStream, xVal, yVal);
   HP_AttrId(pStream, HP_PageOrigin);
   HP_Operator(pStream, HP_SetPageOrigin);
}




HP_StdFuncPrefix HP_SetPageOrigin_4(       /* SetPageOrigin signature 4 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_Real32 xVal,                        /* new origin x value */
    HP_Real32 yVal)                        /* new origin y value */
{
   HP_AttrXyReal32(pStream, xVal, yVal);
   HP_AttrId(pStream, HP_PageOrigin);
   HP_Operator(pStream, HP_SetPageOrigin);
}




HP_StdFuncPrefix HP_SetPageRotation_1(     /* SetPageRotation signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_SInt16 pageAngle)                   /* page angle value */
{
   HP_AttrSInt16(pStream, pageAngle);
   HP_AttrId(pStream, HP_PageAngle);
   HP_Operator(pStream, HP_SetPageRotation);
}




HP_StdFuncPrefix HP_SetPageRotation_2(     /* SetPageRotation signature 2 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_Real32 pageAngle)                   /* page angle value */
{
   HP_AttrReal32(pStream, pageAngle);
   HP_AttrId(pStream, HP_PageAngle);
   HP_Operator(pStream, HP_SetPageRotation);
}




HP_StdFuncPrefix HP_SetPageScale_1(        /* SetPageScale signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_Real32 xVal,                        /* x scale factor */
    HP_Real32 yVal)                        /* y scale factor */
{
   HP_AttrXyReal32(pStream, xVal, yVal);
   HP_AttrId(pStream, HP_PageScale);
   HP_Operator(pStream, HP_SetPageScale);
}




HP_StdFuncPrefix HP_SetPathToClip_1(       /* SetPathToClip signature 1 */

    HP_StreamHandleType pStream)           /* stream handle */
{
    HP_Operator(pStream, HP_SetPathToClip);
}



HP_StdFuncPrefix HP_SetPaintTxMode_1(      /* SetPaintTxMode signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UByte transparancyEnum)             /* transparency mode enumeration */
{
   HP_AttrUByte(pStream, transparancyEnum);
   HP_AttrId(pStream, HP_TxMode);
   HP_Operator(pStream, HP_SetPaintTxMode);
}




HP_StdFuncPrefix HP_SetPenSource_Gray1(    /* SetPenSource Gray sig. 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_Real32 grayLevel)                   /* gray level value (0-1.0) */
{
   HP_AttrReal32(pStream, grayLevel);
   HP_AttrId(pStream, HP_GrayLevel);
   HP_Operator(pStream, HP_SetPenSource);

}


HP_StdFuncPrefix HP_SetPenSource_Gray2(    /* SetPenSource Gray sig. 2 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UByte grayLevel)                   /* gray level value (0-255) */
{
   HP_AttrUByte(pStream, grayLevel);
   HP_AttrId(pStream, HP_GrayLevel);
   HP_Operator(pStream, HP_SetPenSource);

}

/* PCL XL 2.0 */

HP_StdFuncPrefix HP_SetPenSource_Primary(  /* SetPenSource Primary */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_UByte colorSpaceEnum,               /* Primary Depth */
	HP_pUByte colorData,                   /* array for color data */
	HP_UInt16 colorDataLen)                /* length of color data array in bytes */
{

  HP_AttrUByte(pStream, colorSpaceEnum);
  HP_AttrId(pStream,  HP_PrimaryDepth);    /* PrimaryDepth */
  HP_AttrUByteArray(pStream, colorData, (HP_UInt32) colorDataLen);
  HP_AttrId(pStream,HP_PrimaryArray);      /* PrimaryArray */
  HP_Operator(pStream,HP_SetPenSource);
}

HP_StdFuncPrefix HP_SetPenSource_Null1(    /* SetPenSource Null sig. 1 */

    HP_StreamHandleType pStream)           /* stream handle */
{
   HP_AttrUByte(pStream, 0);
   HP_AttrId(pStream, HP_NullPen);
   HP_Operator(pStream, HP_SetPenSource);
}




HP_StdFuncPrefix HP_SetPenSource_Patt1(    /* SetPenSource Pattern sig. 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_SInt16 patternSelectID)             /* pattern select identifier */
{
   HP_AttrSInt16(pStream, patternSelectID);
   HP_AttrId(pStream, HP_PatternSelectID);
   HP_Operator(pStream, HP_SetPenSource);

}

HP_StdFuncPrefix HP_SetPenSource_Patt2(    /* SetPenSource Pattern sig. 1 */

	HP_StreamHandleType pStream,           /* stream handle */
    HP_SInt16 xExtent,                     /* pattern dest x extent */
    HP_SInt16 yExtent,                     /* pattern dest y extent */
    HP_SInt16 patternSelectID)             /* pattern select identifier */
{
   HP_AttrXyUInt16(pStream, xExtent, yExtent);
   HP_AttrId(pStream, HP_NewDestinationSize);
   HP_AttrSInt16(pStream, patternSelectID);
   HP_AttrId(pStream, HP_PatternSelectID);
   HP_Operator(pStream, HP_SetPenSource);

}

HP_StdFuncPrefix HP_SetPenSource_RGB1(     /* SetPenSource RGB sig. 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_Real32 redValue,                    /* red value */
    HP_Real32 greenValue,                  /* green value */
    HP_Real32 blueValue)                   /* blue value */
{
#define HP_RGB_COLOR_LEN 3

   HP_Real32 RGB_Array[HP_RGB_COLOR_LEN];

   RGB_Array[0] = redValue;
   RGB_Array[1] = greenValue;
   RGB_Array[2] = blueValue;

   HP_AttrReal32Array(pStream, RGB_Array, HP_RGB_COLOR_LEN);
   HP_AttrId(pStream, HP_RGBColor);
   HP_Operator(pStream, HP_SetPenSource);

#undef HP_RGB_COLOR_LEN
}

HP_StdFuncPrefix HP_SetPenSource_RGB2(     /* SetPenSource RGB sig. 2 */

    HP_StreamHandleType pStream,           /* stream handle */
	HP_UByte redValue,                     /* red value */
    HP_UByte greenValue,                   /* green value */
    HP_UByte blueValue)                    /* blue value */
{
#define HP_RGB_COLOR_LEN 3

   HP_UByte RGB_Array[HP_RGB_COLOR_LEN];

   RGB_Array[0] = redValue;
   RGB_Array[1] = greenValue;
   RGB_Array[2] = blueValue;

   HP_AttrUByteArray(pStream, RGB_Array, HP_RGB_COLOR_LEN);
   HP_AttrId(pStream, HP_RGBColor);
   HP_Operator(pStream, HP_SetPenSource);

#undef HP_RGB_COLOR_LEN
}



HP_StdFuncPrefix HP_SetPenWidth_1(         /* SetPenWidth signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UInt16 penWidth)                    /* pen width */
{
   HP_AttrUInt16(pStream, penWidth);
   HP_AttrId(pStream, HP_PenWidth);
   HP_Operator(pStream, HP_SetPenWidth);
}



HP_StdFuncPrefix HP_SetPenWidth_2(         /* SetPenWidth signature 2 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_Real32 penWidth)                    /* pen width */
{
   HP_AttrReal32(pStream, penWidth);
   HP_AttrId(pStream, HP_PenWidth);
   HP_Operator(pStream, HP_SetPenWidth);
}




HP_StdFuncPrefix HP_SetROP_1(              /* SetROP signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UByte rop3)                         /* rop 3 value */
{
   HP_AttrUByte(pStream, rop3);
   HP_AttrId(pStream, HP_ROP3);
   HP_Operator(pStream, HP_SetROP);
}




HP_StdFuncPrefix HP_SetSourceTxMode_1(     /* SetSourceTxMode signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UByte transparancyEnum)             /* transparency mode enumeration */
{
   HP_AttrUByte(pStream, transparancyEnum);
   HP_AttrId(pStream, HP_TxMode);
   HP_Operator(pStream, HP_SetSourceTxMode);
}


HP_StdFuncPrefix HP_Text_1(                /* Text signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_pUByte charArray,                   /* character array containing text */
    HP_UInt32 charArrayLen,                /* length of character array */
    HP_pSInt16 xSpaceArray,                /* x escapement array */
    HP_UInt32 xSpaceArrayLen)              /* x escapement array length */
{
   HP_AttrUByteArray(pStream, charArray, charArrayLen);
   HP_AttrId(pStream, HP_TextData);

   {
       int useByteArray = 1;
       HP_UInt32 i = 0;

       /* check to see of all escapements may be expressed as ubytes: */
       for (i=0; (i < xSpaceArrayLen) && (useByteArray); i++)
       {
           if ((xSpaceArray[i]>255) || (xSpaceArray[i]<0))
               useByteArray = 0;
	   }
       
       if (useByteArray)
       {
           HP_AttrSInt16ToByteArray(pStream, xSpaceArray, xSpaceArrayLen);
       }
	   else
	   {
           HP_AttrSInt16Array(pStream, xSpaceArray, xSpaceArrayLen);
       }
   }

   HP_AttrId(pStream, HP_XSpacingData);
   HP_Operator(pStream, HP_Text);
}

HP_StdFuncPrefix HP_Text_2(                /* Text signature 2 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_pUByte charArray,                   /* character array containing text */
	HP_UInt32 charArrayLen,                /* length of character array */
	HP_pSInt16 ySpaceArray,                 /* y escapement array */
    HP_UInt32 ySpaceArrayLen)              /* y escapement array length */
{
   HP_AttrUByteArray(pStream, charArray, charArrayLen);
   HP_AttrId(pStream, HP_TextData);

   {
       int useByteArray = 1;
       HP_UInt32 i = 0;

       /* check to see of all escapements may be expressed as ubytes: */
	   for (i=0; (i < ySpaceArrayLen) && (useByteArray); i++)
       {
           if ((ySpaceArray[i]>255) || (ySpaceArray[i]<0))
               useByteArray = 0;
       } 
       
	   if (useByteArray)
       {
           HP_AttrSInt16ToByteArray(pStream, ySpaceArray, ySpaceArrayLen);
       }
       else
       {
           HP_AttrSInt16Array(pStream, ySpaceArray, ySpaceArrayLen);
       }
   }
   HP_AttrId(pStream, HP_YSpacingData);
   HP_Operator(pStream, HP_Text);
}

HP_StdFuncPrefix HP_Text_3(                /* Text signature 3 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_pUByte charArray,                   /* character array containing text */
    HP_UInt32 charArrayLen,                /* length of character array */
    HP_pSInt16 xSpaceArray,                /* x escapement array */
    HP_UInt32 xSpaceArrayLen,              /* x escapement array length */
	HP_pSInt16 ySpaceArray,                /* y escapement array */
    HP_UInt32 ySpaceArrayLen)              /* y escapement array length */

{
   HP_AttrUByteArray(pStream, charArray, charArrayLen);
   HP_AttrId(pStream, HP_TextData);
   HP_AttrSInt16Array(pStream, xSpaceArray, xSpaceArrayLen);
   HP_AttrId(pStream, HP_XSpacingData);
   HP_AttrSInt16Array(pStream, ySpaceArray, ySpaceArrayLen);
   HP_AttrId(pStream, HP_YSpacingData);
   HP_Operator(pStream, HP_Text);
}

HP_StdFuncPrefix HP_Text_4(                /* Text signature 4 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_pUInt16 charArray,                  /* character array containing text */
	HP_UInt32 charArrayLen,                /* length of character array */
	HP_pSInt16 xSpaceArray,                /* x escapement array */
	HP_UInt32 xSpaceArrayLen)              /* x escapement array length */
{
   HP_AttrUInt16Array(pStream, charArray, charArrayLen);
   HP_AttrId(pStream, HP_TextData);

   {
	   int useByteArray = 1;
	   HP_UInt32 i = 0;

	   /* check to see of all escapements may be expressed as ubytes: */
	   for (i=0; (i < xSpaceArrayLen) && (useByteArray); i++)
	   {
		   if ((xSpaceArray[i]>255) || (xSpaceArray[i]<0))
			   useByteArray = 0;
	   }

	   if (useByteArray)
	   {
		   HP_AttrSInt16ToByteArray(pStream, xSpaceArray, xSpaceArrayLen);
	   }
	   else
	   {
		   HP_AttrSInt16Array(pStream, xSpaceArray, xSpaceArrayLen);
	   }
   }

   HP_AttrId(pStream, HP_XSpacingData);
   HP_Operator(pStream, HP_Text);
}

HP_StdFuncPrefix HP_Text_5(                /* Text signature 5 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_pUInt16 charArray,                  /* character array containing text */
	HP_UInt32 charArrayLen,                /* length of character array */
	HP_pSInt16 ySpaceArray,                /* y escapement array */
	HP_UInt32 ySpaceArrayLen)              /* y escapement array length */
{
   HP_AttrUInt16Array(pStream, charArray, charArrayLen);
   HP_AttrId(pStream, HP_TextData);

   {
	   int useByteArray = 1;
       HP_UInt32 i = 0;

	   /* check to see of all escapements may be expressed as ubytes: */
	   for (i=0; (i < ySpaceArrayLen) && (useByteArray); i++)
       {
		   if ((ySpaceArray[i]>255) || (ySpaceArray[i]<0))
               useByteArray = 0;
	   }

       if (useByteArray)
       {
           HP_AttrSInt16ToByteArray(pStream, ySpaceArray, ySpaceArrayLen);
       }
       else
	   {
           HP_AttrSInt16Array(pStream, ySpaceArray, ySpaceArrayLen);
	   }
   }
   HP_AttrId(pStream, HP_YSpacingData);
   HP_Operator(pStream, HP_Text);
}

HP_StdFuncPrefix HP_Text_6(                /* Text signature 6 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_pUInt16 charArray,                  /* character array containing text */
	HP_UInt32 charArrayLen,                /* length of character array */
	HP_pSInt16 xSpaceArray,                /* x escapement array */
	HP_UInt32 xSpaceArrayLen,              /* x escapement array length */
	HP_pSInt16 ySpaceArray,                /* y escapement array */
	HP_UInt32 ySpaceArrayLen)              /* y escapement array length */

{
   HP_AttrUInt16Array(pStream, charArray, charArrayLen);
   HP_AttrId(pStream, HP_TextData);
   HP_AttrSInt16Array(pStream, xSpaceArray, xSpaceArrayLen);
   HP_AttrId(pStream, HP_XSpacingData);
   HP_AttrSInt16Array(pStream, ySpaceArray, ySpaceArrayLen);
   HP_AttrId(pStream, HP_YSpacingData);
   HP_Operator(pStream, HP_Text);
}

HP_StdFuncPrefix HP_Text_7(                /* Text signature 7 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_pUByte charArray,                   /* character array containing text */
	HP_UInt32 charArrayLen)              /* y escapement array length */
{
   HP_AttrUByteArray(pStream, charArray, charArrayLen);
   HP_AttrId(pStream, HP_TextData);
   HP_Operator(pStream, HP_Text);
}

HP_StdFuncPrefix HP_Text_8(                /* Text signature 8 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_pUInt16 charArray,                  /* character array containing text */
	HP_UInt32 charArrayLen)              /* x escapement array length */
{
   HP_AttrUInt16Array(pStream, charArray, charArrayLen);
   HP_AttrId(pStream, HP_TextData);
   HP_Operator(pStream, HP_Text);
}

HP_StdFuncPrefix HP_TextPath_1(            /* TextPath signature 1 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_pUByte charArray,                   /* character array containing text */
	HP_UInt32 charArrayLen,                /* length of character array */
	HP_pSInt16 xSpaceArray,                /* x escapement array */
    HP_UInt32 xSpaceArrayLen)              /* x escapement array length */
{
   HP_AttrUByteArray(pStream, charArray, charArrayLen);
   HP_AttrId(pStream, HP_TextData);

   {
       int useByteArray = 1;
	   HP_UInt32 i = 0;

       /* check to see of all escapements may be expressed as ubytes: */
       for (i=0; (i < xSpaceArrayLen) && (useByteArray); i++)
       {
		   if ((xSpaceArray[i]>255) || (xSpaceArray[i]<0))
               useByteArray = 0;
       } 
       
       if (useByteArray)
       {
           HP_AttrSInt16ToByteArray(pStream, xSpaceArray, xSpaceArrayLen);
       }
       else
       {
           HP_AttrSInt16Array(pStream, xSpaceArray, xSpaceArrayLen);
	   }
   }

   HP_AttrId(pStream, HP_XSpacingData);
   HP_Operator(pStream, HP_TextPath);
}

HP_StdFuncPrefix HP_TextPath_2(            /* TextPath signature 2 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_pUByte charArray,                   /* character array containing text */
	HP_UInt32 charArrayLen,                /* length of character array */
	HP_pSInt16 ySpaceArray,                /* y escapement array */
	HP_UInt32 ySpaceArrayLen)              /* y escapement array length */
{
   HP_AttrUByteArray(pStream, charArray, charArrayLen);
   HP_AttrId(pStream, HP_TextData);

   {
	   int useByteArray = 1;
	   HP_UInt32 i = 0;

	   /* check to see of all escapements may be expressed as ubytes: */
	   for (i=0; (i < ySpaceArrayLen) && (useByteArray); i++)
	   {
		   if ((ySpaceArray[i]>255) || (ySpaceArray[i]<0))
			   useByteArray = 0;
	   }

	   if (useByteArray)
	   {
		   HP_AttrSInt16ToByteArray(pStream, ySpaceArray, ySpaceArrayLen);
	   }
	   else
	   {
		   HP_AttrSInt16Array(pStream, ySpaceArray, ySpaceArrayLen);
	   }
   }

   HP_AttrId(pStream, HP_YSpacingData);
   HP_Operator(pStream, HP_TextPath);
}

HP_StdFuncPrefix HP_TextPath_3(            /* TextPath signature 3 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_pUByte charArray,                   /* character array containing text */
	HP_UInt32 charArrayLen,                /* length of character array */
	HP_pSInt16 xSpaceArray,                /* x escapement array */
	HP_UInt32 xSpaceArrayLen,              /* x escapement array length */
	HP_pSInt16 ySpaceArray,                /* y escapement array */
	HP_UInt32 ySpaceArrayLen)              /* y escapement array length */

{
   HP_AttrUByteArray(pStream, charArray, charArrayLen);
   HP_AttrId(pStream, HP_TextData);
   HP_AttrSInt16Array(pStream, xSpaceArray, xSpaceArrayLen);
   HP_AttrId(pStream, HP_XSpacingData);
   HP_AttrSInt16Array(pStream, ySpaceArray, ySpaceArrayLen);
   HP_AttrId(pStream, HP_YSpacingData);
   HP_Operator(pStream, HP_TextPath);
}

HP_StdFuncPrefix HP_TextPath_4(            /* TextPath signature 4 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_pUInt16 charArray,                  /* character array containing text */
	HP_UInt32 charArrayLen,                /* length of character array */
	HP_pSInt16 xSpaceArray,                /* x escapement array */
	HP_UInt32 xSpaceArrayLen)              /* x escapement array length */
{
   HP_AttrUInt16Array(pStream, charArray, charArrayLen);
   HP_AttrId(pStream, HP_TextData);

   {
	   int useByteArray = 1;
	   HP_UInt32 i = 0;

	   /* check to see of all escapements may be expressed as ubytes: */
	   for (i=0; (i < xSpaceArrayLen) && (useByteArray); i++)
	   {
		   if ((xSpaceArray[i]>255) || (xSpaceArray[i]<0))
			   useByteArray = 0;
	   }

	   if (useByteArray)
	   {
		   HP_AttrSInt16ToByteArray(pStream, xSpaceArray, xSpaceArrayLen);
	   }
	   else
	   {
		   HP_AttrSInt16Array(pStream, xSpaceArray, xSpaceArrayLen);
	   }
   }

   HP_AttrId(pStream, HP_XSpacingData);
   HP_Operator(pStream, HP_TextPath);
}

HP_StdFuncPrefix HP_TextPath_5(            /* TextPath signature 5 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_pUInt16 charArray,                  /* character array containing text */
	HP_UInt32 charArrayLen,                /* length of character array */
	HP_pSInt16 ySpaceArray,                /* y escapement array */
	HP_UInt32 ySpaceArrayLen)              /* y escapement array length */
{
   HP_AttrUInt16Array(pStream, charArray, charArrayLen);
   HP_AttrId(pStream, HP_TextData);

   {
	   int useByteArray = 1;
	   HP_UInt32 i = 0;

	   /* check to see of all escapements may be expressed as ubytes: */
	   for (i=0; (i < ySpaceArrayLen) && (useByteArray); i++)
	   {
		   if ((ySpaceArray[i]>255) || (ySpaceArray[i]<0))
			   useByteArray = 0;
	   }

	   if (useByteArray)
	   {
		   HP_AttrSInt16ToByteArray(pStream, ySpaceArray, ySpaceArrayLen);
	   }
	   else
	   {
		   HP_AttrSInt16Array(pStream, ySpaceArray, ySpaceArrayLen);
	   }
   }

   HP_AttrId(pStream, HP_YSpacingData);
   HP_Operator(pStream, HP_TextPath);
}

HP_StdFuncPrefix HP_TextPath_6(            /* TextPath signature 6 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_pUInt16 charArray,                  /* character array containing text */
	HP_UInt32 charArrayLen,                /* length of character array */
	HP_pSInt16 xSpaceArray,                /* x escapement array */
	HP_UInt32 xSpaceArrayLen,              /* x escapement array length */
	HP_pSInt16 ySpaceArray,                /* y escapement array */
	HP_UInt32 ySpaceArrayLen)              /* y escapement array length */

{
   HP_AttrUInt16Array(pStream, charArray, charArrayLen);
   HP_AttrId(pStream, HP_TextData);
   HP_AttrSInt16Array(pStream, xSpaceArray, xSpaceArrayLen);
   HP_AttrId(pStream, HP_XSpacingData);
   HP_AttrSInt16Array(pStream, ySpaceArray, ySpaceArrayLen);
   HP_AttrId(pStream, HP_YSpacingData);
   HP_Operator(pStream, HP_TextPath);
}

HP_StdFuncPrefix HP_SetCharBoldValue_1(            /* SetCharBoldValue */
	HP_StreamHandleType pStream,           /* stream handle */
	HP_Real32 boldValue)               /* Bold Value Real */
{
	HP_AttrReal32(pStream, boldValue);
	HP_AttrId(pStream, HP_CharBoldValue);
	HP_Operator(pStream, HP_SetCharBoldValue);
}

HP_StdFuncPrefix HP_TextPath_7(            /* TextPath signature 7 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_pUByte charArray,                   /* character array containing text */
	HP_UInt32 charArrayLen)              /* y escapement array length */
{
   HP_AttrUByteArray(pStream, charArray, charArrayLen);
   HP_AttrId(pStream, HP_TextData);
   HP_Operator(pStream, HP_TextPath);
}

HP_StdFuncPrefix HP_TextPath_8(            /* TextPath signature 8 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_pUInt16 charArray,                  /* character array containing text */
	HP_UInt32 charArrayLen)              /* x escapement array length */
{
   HP_AttrUInt16Array(pStream, charArray, charArrayLen);
   HP_AttrId(pStream, HP_TextData);
   HP_Operator(pStream, HP_TextPath);
}

/*****************************************************************************

	functions:  VendorUnique related

	purpose:    Add vendor unique attributes and operators to the binary stream

*****************************************************************************/

#define hp_PAINT_SETCOLORTREATMENT  0x68701001
#define hp_PAINT_SETHALFTONEMETHOD  0x68701002
#define hp_PAINT_DOWNLOADCOLORTABLE 0x68701003
#define hp_MEDIA_SELECTMEDIASOURCE  0x68702001
#define hp_MEDIA_SELECTMEDIAFINISH  0x68702002
#define hp_MET						0x68703000

HP_StdFuncPrefix HP_media_SelectMediaFinish(/* SelectMediaFinish */

HP_StreamHandleType pStream,           /* stream handle */
HP_UByte mediaFinish)               /* color space enumeration */
{

	HP_AttrUInt32(pStream, hp_MEDIA_SELECTMEDIAFINISH);
	HP_AttrId(pStream, HP_VUExtension);
	HP_AttrUByte(pStream, mediaFinish);
	HP_AttrId(pStream, HP_VUMediaFinish);
	HP_Operator(pStream,HP_VendorUnique);
}

HP_StdFuncPrefix HP_media_SelectMediaSource(/* SelectMediaSource */

HP_StreamHandleType pStream,           /* stream handle */
HP_UByte mediaSource,
HP_UByte mediaType)               /* color space enumeration */
{

	HP_AttrUInt32(pStream, hp_MEDIA_SELECTMEDIASOURCE);
	HP_AttrId(pStream, HP_VUExtension);
	HP_AttrUByte(pStream, mediaSource);
	HP_AttrId(pStream, HP_VUMediaSource);
	HP_AttrUByte(pStream, mediaType);
	HP_AttrId(pStream, HP_VUMediaType);
	HP_Operator(pStream,HP_VendorUnique);
}

HP_StdFuncPrefix HP_paint_SetColorTreatment (/* SetColorTreatment */

HP_StreamHandleType pStream,           /* stream handle */
HP_UByte colorTreatment,
HP_UInt16 colorID)               /* color space enumeration */
{

	HP_AttrUInt32(pStream, hp_PAINT_SETCOLORTREATMENT);
	HP_AttrId(pStream, HP_VUExtension);
	HP_AttrUByte(pStream, colorTreatment);
	HP_AttrId(pStream, HP_VUColorTreatment);
	HP_AttrUInt16(pStream, colorID);
	HP_AttrId(pStream, HP_VUColorTreatmentByID);
	HP_Operator(pStream,HP_VendorUnique);
}

HP_StdFuncPrefix HP_paint_SetHalftoneMethod (/* SetHalftoneMethod */

HP_StreamHandleType pStream,           /* stream handle */
HP_UByte matrix,
HP_UInt16 matrixID)               /* color space enumeration */
{

	HP_AttrUInt32(pStream, hp_PAINT_SETHALFTONEMETHOD);
	HP_AttrId(pStream, HP_VUExtension);
	HP_AttrUByte(pStream, matrix);
	HP_AttrId(pStream, HP_VUDeviceMatrix);
	HP_AttrUInt16(pStream, matrixID);
	HP_AttrId(pStream, HP_VUDeviceMatrixByID);
	HP_Operator(pStream,HP_VendorUnique);
}

HP_StdFuncPrefix HP_paint_DownloadColorTable (/* DownloadColorTable */

HP_StreamHandleType pStream,           /* stream handle */
HP_UByte tableType,
HP_UInt16 tableID,
HP_UInt32 tableSize,
HP_pUByte table)               /* color space enumeration */
{
HP_UInt32 index=0;

	HP_AttrUInt32(pStream, hp_PAINT_DOWNLOADCOLORTABLE);
	HP_AttrId(pStream, HP_VUExtension);
	HP_AttrUByte(pStream, tableType);
	HP_AttrId(pStream, HP_VUTypeOfTable);
	HP_AttrUInt16(pStream, tableID);
	HP_AttrId(pStream, HP_VUColorTableID);
	HP_AttrUInt32(pStream, tableSize);
	HP_AttrId(pStream, HP_VUDataLength);
	HP_Operator(pStream,HP_VendorUnique);

	while ((HP_NoErrors(pStream)) && (index < tableSize))
	{
	   HP_AddToListByte(pStream, (HP_UByte) table[index++]);
	}
}

HP_MET_OPERATORS MetOperators[] =
	{
	hp_MET_NEXT_OP, 5,
	hp_MET_STOP_OP, 5,
	hp_MET_START_CHAR, 5,
	hp_MET_STOP_CHAR, 5,
	hp_MET_THRESHOLD_OP, 4,
	hp_MET_CLIP_SOURCE_OP, 4,
	hp_MET_DECOMP_BUFF_OP, 4,
	hp_MET_PIXEL_CENTER_OP, 4,
	hp_MET_WINDOW_OP, 4,
	hp_MET_LOGICAL_OP, 4,
	hp_MET_PATTERN_OP, 4,
	hp_MET_RACE_BUFF_OP, 4,
	hp_MET_IMAGE_16_OP, 3,
	hp_MET_IMAGE_32_OP, 2,
	hp_MET_OR_IMAGE_32_OP, 3,
	hp_MET_SCALE_IMAGE_OP, 1,
	hp_MET_IMAGE_BLOCK_OP, 2,
	hp_MET_UIP_IMAGE_OP, 1,
	hp_MET_HT_IMAGE_OP, 1,
	hp_MET_POINT_OP, 3,
	hp_MET_XLINE_OP, 3,
	hp_MET_YLINE_OP, 3,
	hp_MET_RUN_OP, 3,
	hp_MET_RULE_OP, 3,
	hp_MET_PATTERN_RULE_OP, 3,
	hp_MET_THIN_VECTOR_OP, 3,
	hp_MET_TRIANGLE_OP, 3,
	hp_MET_VECTOR_OP, 3,
	hp_MET_TRAPEZOID_OP, 3,
	hp_MET_DECOMP_STRIP_OP, 5
	};

HP_StdFuncPrefix HP_met(/* Met Type 4 Operators */

HP_StreamHandleType pStream,           /* stream handle */
HP_UByte Operator,                  /* Operator Number */
HP_SInt16 Xcoord,
HP_SInt16 Ycoord,
HP_SInt16 Height,
HP_SInt16 Width,
HP_UInt32 Count,
HP_UByte  CompType,
HP_Real32 HorizScale,
HP_Real32 VertScale,
HP_UInt16 Terms)               /* color space enumeration */
{

	HP_AttrUInt32(pStream, MetOperators[Operator].Op);
	HP_AttrId(pStream, HP_VUExtension);

	if (MetOperators[Operator].Type < 5) /* 4, 3, 2, 1 */
		{
		HP_AttrUInt32(pStream, Count);
		HP_AttrId(pStream, HP_VUCount);
		}

	if (MetOperators[Operator].Type < 4) /* 3, 2, 1 */
		{
		HP_AttrUInt16(pStream, Width);
		HP_AttrId(pStream, HP_VUWidth);
		HP_AttrUInt16(pStream, Height);
		HP_AttrId(pStream, HP_VUHeight);
		HP_AttrUInt16(pStream, Ycoord);
		HP_AttrId(pStream, HP_VUYcoord);
		HP_AttrUInt16(pStream, Xcoord);
		HP_AttrId(pStream, HP_VUXcoord);
		}

	if (MetOperators[Operator].Type < 3) /* 2, 1 */
		{
		HP_AttrUByte(pStream, CompType);
		HP_AttrId(pStream, HP_VUCompType);
		}

	if (MetOperators[Operator].Type < 2) /* 1 */
		{
		HP_AttrReal32(pStream, VertScale);
		HP_AttrId(pStream, HP_VUVertScale);
		HP_AttrReal32(pStream, HorizScale);
		HP_AttrId(pStream, HP_VUHorizScale);
		}

	if (MetOperators[Operator].Type == 2)
		{
		HP_AttrUInt16(pStream, Terms);
		HP_AttrId(pStream, HP_VUTerms);
		}

	HP_Operator(pStream,HP_VendorUnique);
}

#endif

#define FALSE 0
#define TRUE 1

HP_StdFuncPrefix HP_M2TIFF_CalcCompression(
	HP_pCharHuge lpbytes, /* Pointer to the buffer of bytes to Calculate. */
	HP_SInt32 len,        /* Length of the buffer of bytes. */
	HP_pUInt32 CompSize   /* Memory location to return amount of compression,
							 this will be initialized to zero with each call */
   )
{

	HP_pCharHuge lpfirst;
	HP_pCharHuge lpchar;
	HP_BOOL in_run, FourByteWordPad;
	register HP_UByte count;
	register HP_SInt16 state;
	HP_SInt32    Comp_Count = 0;
	HP_UByte padCount;
	HP_pUByte  lpEnd;

	lpfirst = lpbytes;
	lpEnd =  lpbytes + len;
	in_run = FALSE;
	state = START;
	FourByteWordPad = FALSE;

	*CompSize = 0;
	padCount = (HP_UByte) (4 - (len % 4));

	if(padCount != 4)
	  FourByteWordPad = TRUE;
	else
	  padCount = 0;

    for (;;) {      /* do this until we are DONE */

    switch (state) {

	case IN_RUN:

        if ((lpchar + 1) >= lpEnd) {  /* off the end? */
			state = COMP_DONE;
            break;
		}

        if (*lpchar != *(lpchar + 1)) {
            state = TERM;
            break;
        }

        lpchar++;

        count++;

        if (count >= RUN_LEN)
            state = TERM;
        break;

    case IN_NON_RUN:

        if ((lpchar + 1) >= lpEnd) {  /* off the end? */
            state = COMP_DONE;
            break;
        }

        if (*lpchar == *(lpchar + 1)) {
            lpchar--;   /* back up to give */
            count--;    /* upcomming run more */
			state = TERM;
			break;
        } 

        lpchar++;
        count++;

		if (count >= NON_RUN_LEN)
			state = TERM;
        break;


    case START:

		/* special case, one char (last in string) */
		if ((lpfirst + 1) >= lpEnd) {
            count = 1;      /* minimal case */
            in_run = FALSE;
            state = COMP_DONE;
        } else {
             
			 lpchar = lpfirst + 1;

             count = 2;      /* start with strings of 2 */

            if (*lpchar == *lpfirst){
                in_run = TRUE;
				state = IN_RUN;}
            else{
                in_run = FALSE;
                state = IN_NON_RUN;}
        }
        break;

    case TERM:
    case COMP_DONE:
        if (state == COMP_DONE) {
            if(FourByteWordPad)
              *CompSize += 2;

			if (in_run){
            
                *CompSize += 2;
                return;
                } else {
                
                *CompSize = *CompSize + (count + 1);
                return;
                    } 
           } else {
            if (in_run) 
				*CompSize += 2;
            else 
				*CompSize = *CompSize + (count + 1);
            
            lpfirst = lpchar + 1;
            if(lpfirst >= lpEnd)
              {
                if(FourByteWordPad)
				  *CompSize += 2;
              return;
              }
            state = START;
            }
		break;
		}
	}
    return;
}     


HP_StdFuncPrefix HP_M2TIFF_Compress(
	HP_StreamHandleType lpdv,  /* Pointer to the output buffer */
    HP_UInt32 outputLen,       /* Length of the output buffer in bytes. */
    HP_pCharHuge lpBits,       /* Pointer to the buffer of bytes to Compress */
	HP_SInt32 inputLen,        /* Length of the input buffer of bytes. */
    HP_pUInt32 CompSize        /* Memory location to return amount of compression,
                                  this will be initialized to zero with each call */
	)
{
    HP_pCharHuge lpfirst;
    HP_pCharHuge lpchar;
    HP_BOOL in_run,FourByteWordPad;
    register HP_UByte count;
    register HP_SInt16 state;
	HP_SInt32    Comp_Count = 0;
    HP_pCharHuge   lpEnd;
    HP_SInt32    bitCounter = 0;
    HP_UByte padCount;
    
    lpfirst = lpBits;
	lpEnd = lpBits + inputLen;
    in_run = FALSE;
    state = START;
    *CompSize = 0;
    FourByteWordPad = FALSE;

	padCount = (HP_UByte) (4 - (inputLen % 4));
    if(padCount != 4)
      FourByteWordPad = TRUE;
    else
      padCount = 0;

	for (;;) {      /* do this until we are DONE */
    
    
    switch (state) {

    case IN_RUN: 

		if ((lpchar + 1) >= lpEnd) {  /* off the end? */
            state = COMP_DONE;
            break;
        }

		if (*lpchar != *(lpchar + 1)) {
			state = TERM;
			break;
        }

        lpchar++;

        count++;

        if (count >= RUN_LEN)
            state = TERM;
		break;

    case IN_NON_RUN:

        if ((lpchar + 1) >= lpEnd) {  /* off the end? */
            state = COMP_DONE;
            break;
        }

		if (*lpchar == *(lpchar + 1)) {
            lpchar--;   /* back up to give */
            count--;    /* upcomming run more */
            state = TERM;
            break;
        } 

		lpchar++;
        count++;

        if (count >= NON_RUN_LEN)
            state = TERM;
        break;


    case START:

        /* special case, one char (last in string) */ 
		if ((lpfirst + 1) >= lpEnd) {
            count = 1;      /* minimal case */
            in_run = FALSE;
            state = COMP_DONE;
        } else {
             
             lpchar = lpfirst + 1;

			/* in_run = FALSE; */
            count = 2;      /* start with strings of 2 */

            if (*lpchar == *lpfirst){
                in_run = TRUE;
				state = IN_RUN;}
			else{
				in_run = FALSE;
                state = IN_NON_RUN;}
        }
        break;

    case TERM:
	case COMP_DONE:
        if (state == COMP_DONE) {
            if(FourByteWordPad)
			  *CompSize +=2;
            
            if (in_run){
			   *CompSize += 2;

          if((outputLen > 0) && (*CompSize >= outputLen)){
                 *CompSize = 0;
                  return;
                }
    
                           
          HP_RawUByte(lpdv,(HP_UByte)(0 - (count -1)));
          HP_RawUByte(lpdv,(HP_UByte)*lpfirst);

        if(FourByteWordPad)
          {
		  if(padCount == 1)
            {
              HP_RawUByte(lpdv,0);
              HP_RawUByte(lpdv,0x00);
            }
          else
			{
              HP_RawUByte(lpdv,(HP_UByte)(0 - (padCount - 1)));
              HP_RawUByte(lpdv,0x00);
            }
        }
				return;
                } else {
               
                *CompSize = (*CompSize +(count+1));

                if((outputLen > 0) && (*CompSize >= outputLen)){
                  *CompSize = 0;
                  return;
		}

                HP_RawUByte(lpdv,(HP_UByte)(count -1));
                HP_RawUByteArray(lpdv,lpfirst,(HP_UInt32)count);

		if(FourByteWordPad)
		  {
		  if(padCount == 1)
            {
              HP_RawUByte(lpdv,0);
              HP_RawUByte(lpdv,0x00);
            }
          else
			{
              HP_RawUByte(lpdv,(HP_UByte)(0 - (padCount - 1)));
              HP_RawUByte(lpdv,0x00);
			}
        }
          

                return;    
                }
           } else {
            if (in_run) {
                *CompSize +=2;
                
                if((outputLen > 0) && (*CompSize >= outputLen)){
                  *CompSize = 0;
                  return;
                }
                HP_RawUByte(lpdv,(HP_UByte)(0 - (count - 1)));
                HP_RawUByte(lpdv, *lpfirst);


                } else {
                *CompSize = (*CompSize +(count + 1));
                if((outputLen >= 0) && (*CompSize >= outputLen)){
                  *CompSize = 0;
                  return;
                }
                HP_RawUByte(lpdv,(HP_UByte)(count -1));
                HP_RawUByteArray(lpdv,lpfirst,(HP_UInt32)count);
                    
				 }
            
			lpfirst = lpchar + 1;
            if(lpfirst >= lpEnd)
              {
                if(FourByteWordPad)
                  {
                    *CompSize += 2;
					if(padCount == 1)
                      {
                        HP_RawUByte(lpdv,0);
                        HP_RawUByte(lpdv,0x00);
                      }
					else
					  {
						HP_RawUByte(lpdv,(HP_UByte)(0 - (padCount - 1)));
                        HP_RawUByte(lpdv,0x00);
                      }
                  }
			  return;
			  }
			state = START;
			}
		break;
		}
	}
	return;
}
#undef TRUE
#undef FALSE






















































