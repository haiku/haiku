#ifndef JETLIB_H
#define JETLIB_H

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

/**************************** NOTICE ***************************************

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

Copyright (c) 1995-1999 Hewlett-Packard Company.  All rights reserved.

*****************************************************************************/

/*
 *  file: jetlib.h
 *
 *  PCL XL Stream library header file
 *
 *  Revision : tracks jetlib.c
 *
 */

#define HP_BYTE_STREAM_PROTOCOL  /* define this for byte stream protocols */
#define OO_INTERFACE /* XXX should be defined at compile time */
#define HP_NoAutoFlush /* XXX should be defined at compile time */

/*  PCL XL Stream library error codes: */
#define     HP_NoError               0
#define     HPERR_InitError         -1
#define     HPERR_UndefinedOp       -2
#define     HPERR_UndefinedAttrId   -3
#define     HPERR_OutOfMemory       -4 
#define     HPERR_ZeroBufferSize    -5
#define     HPERR_OutBufferOverflow -6
#define     HPERR_FlushBufferFailed -7 
#define     HPERR_NullFlushFunction -8

/********************************************************************
 *
 * Some handy typedefs
 *
 *******************************************************************/
typedef unsigned char U8 ;
typedef unsigned int U16 ;
typedef int S16 ;
typedef unsigned long U32 ;
typedef long S32 ;

typedef     unsigned char       HP_UByte;
typedef     signed char         HP_SByte;
typedef     unsigned short int  HP_UInt16;
typedef     short int           HP_SInt16;
typedef     unsigned long       HP_UInt32;
typedef     long                HP_SInt32;
typedef     float               HP_Real32;
typedef     int                 HP_BOOL;

#ifdef HP_WindowsAPI
#include "jetlibhw.h"
#else

/* Added for the Method 2 compression stuff */

typedef     unsigned char       *HP_pCharHuge; /* Added for windows 3.1x compatibility */
typedef     unsigned char       *HP_pChar; /* Added for windows 3.1x compatibility */

/* End of the Method 2 compression stuff */

typedef     unsigned char       *HP_pUByte;
typedef     unsigned short int  *HP_pUInt16;
typedef     unsigned long       *HP_pUInt32;
typedef     short int           *HP_pSInt16;
typedef     long                *HP_pSInt32;
typedef     float               *HP_pReal32;
typedef     HP_SInt16           HP_ErrorCodeType; 
#endif

/* Added for the Method 2 compression stuff */ 

#define START       0   /* states of the compression state machine */
#define IN_RUN      1   /* building a run of repeating chars */
#define IN_NON_RUN  2   /* building a run of non repeating chars */
#define COMP_DONE   3   /* all chars have been processed */
#define TERM        4   /* end of a run or non-run */
#define RUN_LEN     127 /* 129 */
#define NON_RUN_LEN 127 /* 128 */

#define eHP_M2TIFF  3

typedef struct tagCOMPRESSIONINFO {
		HP_UInt32    CompSize;
        HP_SInt16    CompRatio;
        HP_UInt32    CompressionFormat;
        HP_SInt32    biWidth;
        HP_SInt32    biHeight;
        HP_SInt32    biAdjWidth;
        } HP_COMPRESSIONINFO;

/* End of the Method 2 compression stuff */

#ifndef HP_WindowsAPI
/*  UNIX or DOS API */

#ifndef OO_INTERFACE
#define HP_StdFuncPrefix void
#define HP_SInt32FuncPrefix HP_SInt32
#define HP_UInt32FuncPrefix HP_UInt32
#else
/* Use different declarations for C++ */

#define HP_StdFuncPrefix    extern "C" void
#define HP_SInt32FuncPrefix extern "C" HP_SInt32
#define HP_UInt32FuncPrefix extern "C" HP_UInt32

/* This forward declarations is so that pClient can be
 * in the structure below.
 */
class MJetlibClient;
#endif

typedef struct{
	HP_UByte dataType;
	HP_UInt32 length;
	HP_pUByte string;
}CHEETPackagedStringType;

typedef struct HP_StreamHandleStructDef
{
	HP_pUByte   HP_OutBuffer;       /* byte-oriented output buffer for
                                     * byte-stream protocols : referenced
									 * by external programs via .h file
                                     * extern declaration
									 */
	HP_SInt32   HP_OutBufferMaxSize;/* maximum size of when created in
                                     * HP_InitLib function
                                     *
                                     * referenced by external programs via
                                     * .h file extern declaration
									 */
    HP_SInt32   HP_CurrentBufferLen;/* current output buffer index: set to
                                     * zero by HP_InitLib and following
                                     * library routine calls to 
                                     * HP_FlushOutBuffer function.
                                     *
                                     * User's HP_FlushOutBuffer function 
                                     * should
                                     * also set the index to zero.
                                     *
                                     * referenced by external programs via
                                     * .h file extern declaration
                                     */

    HP_SInt32   HP_CurrentStreamLen;/* current stream length in bytes:
									 * set to zero by HP_InitLib
                                     *
                                     */

    HP_ErrorCodeType HP_ErrorCode;  /* Global error code accessed by
                                     * HP_SetErrorCode, HP_GetErrorCode
                                     * macros 
                                     */
                                     
	HP_UInt16 HP_ErrorCount;        /* Counts how many errors were set
                                     */     

#ifndef OO_INTERFACE
    /* pointer to flush buffer function: */                                   
    HP_StdFuncPrefix (*pFlushOutBuffer) (void *, unsigned long, HP_pUByte, HP_SInt32);  
#else
    MJetlibClient *pClient;
#endif

	unsigned long cookie;           /* magic cookie used by callers any way they like
                                     * this cookie is passed to callbacks like
                                     * pFlushOutBuffer
                                     */
                                     
} HP_StreamStructType;

typedef HP_StreamStructType *HP_StreamHandleType;

#ifndef OO_INTERFACE
#define HP_InitFuncPrefix HP_StreamHandleType
typedef HP_StdFuncPrefix (*flushFunctionType) (HP_StreamHandleType, unsigned long, HP_pUByte, HP_SInt32);
#else
#define HP_InitFuncPrefix extern "C" HP_StreamHandleType

/* Also, declare a mixin class that defines an interface that should
 * be used to flush the buffer.  In C++, this is preferred over
 * pointers to functions.
 *
 * Any client class that needs to use Jetlib should publicly derive
 * from MJetlibClient.  The client class then defines the
 * FlushOutBuffer method.
 */
class MJetlibClient
{
public:
    virtual void FlushOutBuffer(HP_StreamHandleType pStream,
                                unsigned long cookie,
                                HP_pUByte pOutBuffer,
                                HP_SInt32 currentBufferLen) = 0;

    /* Compiler generated default constructor, copy constructor, 
     * operator=, and destructor.
     */

};

#endif /* OO_INTERFACE */


#endif 


#ifdef HP_BYTE_STREAM_PROTOCOL
#ifndef HP_WindowsAPI
                                           
#define     HP_SetErrorCode(pStream, value) \
                (pStream)->HP_ErrorCode = (HP_ErrorCodeType) value; \
                (pStream)->HP_ErrorCount++
#define     HP_GetErrorCode(pStream) (pStream)->HP_ErrorCode 
#define     HP_GetErrorCount(pStream) (pStream)->HP_ErrorCount
#define     HP_NoErrors(pStream) (!(pStream)->HP_ErrorCode)
#define     HP_LibErrors(pStream) (pStream->HP_ErrorCode < 0)

#define HP_OutBufferRemaining(pStream) \
        (pStream->HP_OutBufferMaxSize - pStream->HP_CurrentBufferLen)
         
#define HP_BufferOverflow (pStream) \
				 (pStream->HP_CurrentBufferLen > pStream->HP_OutBufferMaxSize)

#endif
#endif

#define HP_BeginSession     0x41
#define HP_EndSession       0x42
#define HP_BeginPage        0x43
#define HP_EndPage          0x44
#define HP_VendorUnique     0x46
#define HP_Comment          0x47
#define HP_OpenDataSource   0x48
#define HP_CloseDataSource  0x49
#define HP_EchoComment      0x4a
#define HP_Query            0x4b
#define HP_Diagnostic3      0x4c

#define HP_BeginFontHeader  0x4f
#define HP_ReadFontHeader   0x50
#define HP_EndFontHeader    0x51
#define HP_BeginChar        0x52
#define HP_ReadChar         0x53
#define HP_EndChar          0x54
#define HP_RemoveFont       0x55
#define HP_SetCharAttribute 0x56

#define HP_SetDefaultGS     0x57
#define HP_SetColorTreatment 0x58
#define HP_SetGlobalAttributes 0x59
#define HP_ClearGlobalAttributes 0x5A
#define HP_BeginStream      0x5b
#define HP_ReadStream       0x5c
#define HP_EndStream        0x5d
#define HP_ExecStream       0x5e
#define HP_RemoveStream     0x5F

#define HP_PopGS            0x60
#define HP_PushGS           0x61
#define HP_SetClipReplace   0x62
#define HP_SetBrushSource   0x63
#define HP_SetCharAngle     0x64
#define HP_SetCharScale     0x65
#define HP_SetCharShear     0x66
#define HP_SetClipIntersect 0x67
#define HP_SetClipRectangle 0x68
#define HP_SetClipToPage    0x69
#define HP_SetColorSpace    0x6a
#define HP_SetCursor        0x6b
#define HP_SetCursorRel     0x6c
#define HP_SetHalftoneMethod 0x6d
#define HP_SetFillMode      0x6e
#define HP_SetFont          0x6f
#define HP_SetLineDash      0x70
#define HP_SetLineCap       0x71
#define HP_SetLineJoin      0x72
#define HP_SetMiterLimit    0x73
#define HP_SetPageDefaultCTM 0x74
#define HP_SetPageOrigin    0x75
#define HP_SetPageRotation  0x76
#define HP_SetPageScale     0x77
#define HP_SetPaintTxMode   0x78
#define HP_SetPenSource     0x79
#define HP_SetPenWidth      0x7a
#define HP_SetROP           0x7b
#define HP_SetSourceTxMode  0x7c
#define HP_SetCharBoldValue 0x7d
#define HP_SetClipMode      0x7f
#define HP_SetPathToClip    0x80
#define HP_SetCharSubMode   0x81
#define HP_BeginUserDefinedLineCap 0x82
#define HP_EndUserDefinedLineCap 0x83
#define HP_CloseSubPath     0x84
#define HP_NewPath          0x85
#define HP_PaintPath        0x86
#define HP_BeginBackground  0x87
#define HP_EndBackground    0x88
#define HP_DrawBackground   0x89
#define HP_RemoveBackground 0x8A
#define HP_BeginForm        0x8B
#define HP_EndForm          0x8C
#define HP_DrawForm         0x8D
#define HP_RemoveForm       0x8E
#define HP_RegisterFormAsPattern 0x8F

#define HP_ArcPath          0x91

#define HP_BezierPath       0x93
#define HP_BezierRelPath    0x95
#define HP_Chord            0x96
#define HP_ChordPath        0x97
#define HP_Ellipse          0x98
#define HP_EllipsePath      0x99

#define HP_LinePath         0x9b

#define HP_LineRelPath      0x9d
#define HP_Pie              0x9e
#define HP_PiePath          0x9f

#define HP_Rectangle        0xa0
#define HP_RectanglePath    0xa1
#define HP_RoundRectangle   0xa2
#define HP_RoundRectanglePath 0xa3

#define HP_Text             0xa8
#define HP_TextPath         0xa9

#define HP_SystemText       0xaa

#define HP_BeginImage       0xb0
#define HP_ReadImage        0xb1
#define HP_EndImage         0xb2
#define HP_BeginRastPattern 0xb3
#define HP_ReadRastPattern  0xb4
#define HP_EndRastPattern   0xb5
#define HP_BeginScan        0xb6
#define HP_EndScan          0xb8
#define HP_ScanLineRel      0xb9

#define HP_PassThrough      0xBA
/* end of #defines for operators */

/* defines for data tags */
#define  HP_UByteData   0xc0
#define  HP_UInt16Data  0xc1
#define  HP_UInt32Data  0xc2
#define  HP_SInt16Data  0xc3
#define  HP_SInt32Data  0xc4
#define  HP_Real32Data  0xc5
					 /* 0xc6     */
#define  HP_String      0xc7
#define  HP_UByteArray  0xc8
#define  HP_UInt16Array 0xc9
#define  HP_UInt32Array 0xca
#define  HP_SInt16Array 0xcb
#define  HP_SInt32Array 0xcc
#define  HP_Real32Array 0xcd
                     /* 0xce     */
                     /* 0xcf     */
#define  HP_UByteXy     0xd0
#define  HP_UInt16Xy    0xd1
#define  HP_UInt32Xy    0xd2
#define  HP_SInt16Xy    0xd3
#define  HP_SInt32Xy    0xd4
#define  HP_Real32Xy    0xd5

#define  HP_UByteBox    0xe0
#define  HP_UInt16Box   0xe1
#define  HP_UInt32Box   0xe2
#define  HP_SInt16Box   0xe3
#define  HP_SInt32Box   0xe4
#define  HP_Real32Box   0xe5
                     /* 0xe6     */
                     /* 0xe7     */

#define   HP_8BitAttrId 0xf8

#define HP_EmbeddedData     0xfa
#define HP_EmbeddedDataByte 0xfb
/* end of #defines for data tags */ 


/* defines for attribute identifier enumerations: */

#define HP_CMYColor          1
#define HP_PaletteDepth      2
#define HP_ColorSpace        3
#define HP_NullBrush         4
#define HP_NullPen           5
#define HP_PaletteData       6
#define HP_PaletteIndex      7
#define HP_PatternSelectID   8
#define HP_GrayLevel         9

#define HP_RGBColor          11
#define HP_PatternOrigin     12
#define HP_NewDestinationSize 13

#define HP_PrimaryArray           14
#define HP_PrimaryDepth           15
#define HP_ColorimetricColorSpace 17
#define HP_XYChromaticities       18
#define HP_WhitePointReference    19
#define HP_CRGBMinMax             20
#define HP_GammaGain              21


#define HP_DeviceMatrix      33
#define HP_DitherMatrixDataType  34
#define HP_DitherOrigin      35
#define HP_MediaDest         36
#define HP_MediaSize         37
#define HP_MediaSource       38
#define HP_MediaType         39
#define HP_Orientation       40
#define HP_PageAngle         41
#define HP_PageOrigin        42
#define HP_PageScale         43
#define HP_ROP3              44
#define HP_TxMode            45
#define HP_CustomMediaSize   47
#define HP_CustomMediaSizeUnits  48
#define HP_PageCopies        49
#define HP_DitherMatrixSize  50
#define HP_DitherMatrixDepth 51
#define HP_SimplexPageMode   52
#define HP_DuplexPageMode    53
#define HP_DuplexPageSide    54

#define HP_LineStartCapStyle 63
#define HP_LineEndCapStyle   64
#define HP_ArcDirection      65
#define HP_BoundingBox       66
#define HP_DashOffset        67
#define HP_EllipseDimension  68
#define HP_EndPoint          69
#define HP_FillMode          70
#define HP_LineCapStyle      71
#define HP_LineJoinStyle     72
#define HP_MiterLength       73
#define HP_LineDashStyle     74
#define HP_PenWidth          75
#define HP_Point             76
#define HP_NumberOfPoints    77
#define HP_SolidLine         78
#define HP_StartPoint        79
#define HP_PointType         80
#define HP_ControlPoint1     81
#define HP_ControlPoint2     82
#define HP_ClipRegion        83
#define HP_ClipMode          84


#define HP_ColorDepthArray   97
#define HP_ColorDepth        98
#define HP_PixelDepth        HP_ColorDepth
#define HP_BlockHeight       99
#define HP_ColorMapping      100
#define HP_PixelEncoding     HP_ColorMapping
#define HP_CompressMode      101
#define HP_DestinationBox    102
#define HP_DestinationSize   103
#define HP_PatternPersistence 104
#define HP_PatternDefineID   105
#define HP_SourceHeight      107
#define HP_SourceWidth       108
#define HP_StartLine         109
#define HP_PadBytesMultiple  110
#define HP_BlockByteLength   111
#define HP_NumberOfScanLines 115

#define HP_ColorTreatment    120
#define HP_FileName          121
#define HP_BackgroundName    122
#define HP_FormName          123
#define HP_FormType          124
#define HP_FormSize          125
#define HP_UDLCName          126

#define HP_CommentData       129
#define HP_DataOrg           130
#define HP_Measure           134
#define HP_SourceType        136
#define HP_UnitsPerMeasure   137
#define HP_QueryKey          138
#define HP_StreamName        139
#define HP_StreamDataLength  140

#define HP_ErrorReport       143
#define HP_IOReadTimeOut     144

#define HP_WritingMode       173

/* Generic VI attributes */

#define HP_VUExtension       145
#define HP_VUDataLength      146

#define HP_VUAttr1           147
#define HP_VUAttr2           148
#define HP_VUAttr3           149
#define HP_VUAttr4           150
#define HP_VUAttr5           151
#define HP_VUAttr6           152
#define HP_VUAttr7           153
#define HP_VUAttr8           154
#define HP_VUAttr9           155
#define HP_VUAttr10          156
#define HP_VUAttr11          157
#define HP_VUAttr12          158

/* Media Source Defines. It is OK that these are the same as VUAttr1-12 */

	#define HP_VUTableSize          146

	#define HP_VUMediaFinish        HP_VUAttr1

	#define HP_VUMediaSource        147
	#define HP_VUMediaType          148

	#define HP_VUColorTableID       147
	#define HP_VUTypeOfTable        148

	#define HP_VUDeviceMatrix       147
	#define HP_VUDeviceMatrixByID   148

	#define HP_VUColorTreatment     147
	#define HP_VUColorTreatmentByID 148


/* MET Defines */

	#define HP_VUXcoord		HP_VUAttr1
	#define HP_VUYcoord		HP_VUAttr2
	#define HP_VUHeight		HP_VUAttr3
	#define HP_VUWidth		HP_VUAttr4
	#define HP_VUCount		HP_VUAttr5
	#define HP_VUCompType	HP_VUAttr6
	#define HP_VUHorizScale	HP_VUAttr7
	#define HP_VUVertScale	HP_VUAttr8
    #define HP_VUTerms      HP_VUAttr9

/********** MET OPERATOR DEFINES ********************/

/* Control Operations */
#define hp_MET_NEXT_OP			0x6870301B
#define hp_MET_STOP_OP			0x6870301C
#define hp_MET_START_CHAR       0x6870301D
#define hp_MET_STOP_CHAR        0x6870301E
/* State Operations */
#define hp_MET_THRESHOLD_OP		0x68703001
#define hp_MET_CLIP_SOURCE_OP	0x68703002
#define hp_MET_DECOMP_BUFF_OP	0x68703003
#define hp_MET_PIXEL_CENTER_OP	0x68703004
#define hp_MET_WINDOW_OP		0x68703005
#define hp_MET_LOGICAL_OP		0x68703006
#define hp_MET_PATTERN_OP		0x68703007
#define hp_MET_RACE_BUFF_OP		0x68703008
/* Marking Operations */
#define hp_MET_IMAGE_16_OP		0x68703009
#define hp_MET_IMAGE_32_OP		0x6870300A
#define hp_MET_OR_IMAGE_32_OP	0x6870300B
#define hp_MET_SCALE_IMAGE_OP	0x6870300C
#define hp_MET_IMAGE_BLOCK_OP	0x6870300D
#define hp_MET_UIP_IMAGE_OP		0x6870300E
#define hp_MET_HT_IMAGE_OP		0x6870300F
#define hp_MET_POINT_OP			0x68703010
#define hp_MET_XLINE_OP			0x68703011
#define hp_MET_YLINE_OP			0x68703012
#define hp_MET_RUN_OP			0x68703013
#define hp_MET_RULE_OP			0x68703014
#define hp_MET_PATTERN_RULE_OP 	0x68703015
#define hp_MET_THIN_VECTOR_OP 	0x68703016
#define hp_MET_TRIANGLE_OP 		0x68703017
#define hp_MET_VECTOR_OP 		0x68703018
#define hp_MET_TRAPEZOID_OP 	0x68703019
#define hp_MET_DECOMP_STRIP_OP	0x6870301A

#define HP_PassThroughCommand 158
#define HP_PassThroughArray   159
#define HP_Diagnostics       160
#define HP_CharAngle         161
#define HP_CharCode          162
#define HP_CharDataSize      163
#define HP_CharScale         164
#define HP_CharShear         165
#define HP_CharSize          166
#define HP_FontHeaderLength  167
#define HP_FontName          168
#define HP_FontFormat        169
#define HP_SymbolSet         170
#define HP_TextData          171
#define HP_CharSubModeArray  172
#define HP_WritingMode       173
#define HP_BitmapCharScale   174
#define HP_XSpacingData      175
#define HP_YSpacingData      176
#define HP_CharBoldValue     177

/* end of attribute identifier enumerations */

/* defines for attribute value enumerations: */

/* Global Attribute Enumerations *******************************************/
#define INVALID_ATTRIBUTE           0
#define VALID_ATTRIBUTE             1
#define GLOBAL_ATTRIBUTE            2

/* enable Enumeration ******************************************************/
#define HP_eOn                      0
#define HP_eOff                     1

/* boolean Enumeration *****************************************************/
#define HP_eFalse                   0
#define HP_eTrue                    1

/* SetCharAttributes Wrint Mode Enumerations ****************************************/
#define HP_eWriteHorizontal        0
#define HP_eWriteVertical          1

/* BitmapCharScaling Enumerations */
#define HP_eDisable                 0
#define HP_eEnable                  1

/* unitOfMeasure Enumeration *******************************************/
#define HP_eInch                    0
#define HP_eMillimeter              1
#define HP_eTenthsOfAMillimeter     2

/* errorReportingEnumeration *********************************************/
#define HP_eNoReporting             0  
#define HP_eBackChannel             1
#define HP_eErrorPage               2
#define HP_eBackChAndErrPage        3
#define HP_eBackChanAndErrPage      3 /* for backward compatibilty only */
#define HP_eNWBackChannel           4
#define HP_eNWErrorPage             5
#define HP_eNWBackChAndErrPage      6

/* dataOrganizationEnumeration ********************************************/
#define HP_eBinaryHighByteFirst     0
#define HP_eBinaryLowByteFirst      1

/* duplexPageModeEnumeration **********************************************/
#define HP_eDuplexHorizontalBinding 0
#define HP_eDuplexVerticalBinding   1

/* duplexPageSideEnumeration **********************************************/
#define HP_eFrontMediaSide          0
#define HP_eBackMediaSide           1

/* simplexPageModeEnumeration *********************************************/
#define HP_eSimplexFrontSide        0

/* orientationEnumeration *************************************************/
#define HP_ePortraitOrientation     0
#define HP_eLandscapeOrientation    1
#define HP_eReversePortrait         2
#define HP_eReverseLandscape        3

/* mediaSize Enumeration ***************************************************/
#define HP_eLetterPaper             0
#define HP_eLegalPaper              1
#define HP_eA4Paper                 2
#define HP_eExecPaper               3
#define HP_eLedgerPaper             4
#define HP_eA3Paper                 5
#define HP_eCOM10Envelope           6
#define HP_eMonarchEnvelope         7
#define HP_eC5Envelope              8
#define HP_eDLEnvelope              9
#define HP_eJB4Paper                10
#define HP_eJB5Paper                11
#define HP_eB5Envelope              12
#define HP_eB5Paper                 13
#define HP_eJPostcard               14
#define HP_eJDoublePostcard         15
#define HP_eA5Paper                 16
#define HP_eA6Paper                 17
#define HP_eJB6Paper                18
#define HP_eJIS8KPaper              19
#define HP_eJIS16KPaper             20
#define HP_eJISExecPaper            21

/* mediaSource Enumeration *************************************************/
#define HP_eDefaultSource          0
#define HP_eAutoSelect             1
#define HP_eManualFeed             2
#define HP_eMultiPurposeTray       3
#define HP_eUpperCassette          4
#define HP_eLowerCassette          5
#define HP_eEnvelopeTray           6
#define HP_eThirdCassette          7

/* mediaDestinationEnumeration ********************************************/
#define HP_eDefaultBin             0
#define HP_eFaceDownBin            1
#define HP_eFaceUpBin              2
#define HP_eJobOffsetBin           3

/* compressionEnumeration *************************************************/
#define HP_eNoCompression          0
#define HP_eRLECompression         1
#define HP_eJPEGCompression        2
#define HP_eDeltaRowCompression    3

/* arcDirectionEnumeration ************************************************/
#define HP_eClockWise              0
#define HP_eCounterClockWise       1

/* fillModeEnumeration ****************************************************/
#define HP_eNonZeroWinding         0
#define HP_eEvenOdd                1

/* lineEndEnumeration *****************************************************/
#define HP_eButtCap                0
#define HP_eRoundCap               1
#define HP_eSquareCap              2
#define HP_eTriangleCap            3

#define HP_eButtEnd                0  /* xxxEnd for backward compatibility */
#define HP_eRoundEnd               1  /* xxxEnd for backward compatibility */
#define HP_eSquareEnd              2  /* xxxEnd for backward compatibility */
#define HP_eTriangleEnd            3  /* xxxEnd for backward compatibility */

/* charSubModeEnumeration *************************************************/
#define HP_eNoSubstitution         0
#define HP_eVerticalSubstitution   1

/* lineJoinEnumeration ****************************************************/
#define HP_eMiterJoin              0
#define HP_eRoundJoin              1
#define HP_eBevelJoin              2
#define HP_eNoJoin                 3

/* ditherMatrixEnumeration ************************************************/
#define HP_eDeviceBest             0
#define HP_eDeviceIndependent      1

/* dataSourceEnumeration **************************************************/
#define HP_eDefaultDataSource      0

/* colorSpaceEnumeration **************************************************/
#define HP_eBiLevel                0
#define HP_eGray                   1
#define HP_eRGB                    2
#define HP_eCMY                    3
#define HP_eCIELab                 4
#define HP_eCRGB                   5
#define HP_eSRGB                   6

/* Form Types Enumeration *************************************************/
#define HP_Quality                 0
#define HP_Performance             1
#define HP_Static                  2
#define HP_Background              3

/* colorDepthEnumeration **************************************************/
#define HP_e1Bit                   0
#define HP_e4Bit                   1 
#define HP_e8Bit                   2  

/* colorMappingEumeration *************************************************/
#define HP_eDirectPixel            0 
#define HP_eIndexedPixel           1
#define HP_eDirectPlane            2 

/* diagnosticEumeration ***************************************************/
#define HP_eNoDiag                 0
#define HP_eFilterDiag             1
#define HP_eCommandsDiag           2  
#define HP_ePersonalityDiag        3  
#define HP_ePageDiag               4
                               
/* clipmodeEnumeration ***************************************************/
#define HP_eInterior               0
#define HP_eExterior               1

/* dataTypeEnumeration ***************************************************/
#define HP_eUByte                  0
#define HP_eSByte                  1
#define HP_eUInt16                 2
#define HP_eSInt16                 3
#define HP_eReal32                 4

/* patternPersistenceEnumeration *****************************************/
#define HP_eTempPattern            0
#define HP_ePagePattern            1
#define HP_eSessionPattern         2

/* transparancyEnumeration ************************************************/
#define HP_eOpaque                 0
#define HP_eTransparent            1

/* charSubModeEnumeration      ***********************************************/
#define HP_eNoSubstitution         0
#define HP_eVerticalSubstitution   1

/* FormType Enumerations *****************************************/
#define HP_eQuality     0
#define HP_ePerformance 1
#define HP_eStatic      2
#define HP_eBackground  3

/* Color Treatment Enumerations *************************************/
#define HP_eNoTreatment 0
#define HP_eVivid       2
#define HP_eScreenMatch 1

/* VendorUnique Extensions    *******************************************/
#define hp_PAINT_SETCOLORTREATMENT  0x68701001
#define hp_PAINT_SETHALFTONEMETHOD  0x68701002
#define hp_PAINT_DOWNLOADCOLORTABLE 0x68701003
#define hp_MEDIA_SELECTMEDIAFINISH  0x68702002
#define hp_SetInternalLUT           0x68702004

#define hp_JR_OPENDATASOURCE        0x68702101
#define hp_MET                      0x68703000
#define MI_MEMORYINFO_ALLOCATE      0x4D491001
#define MI_MEMORYINFO_DEALLOCATE    0x4D491002

#ifndef HP_WindowsAPI

#ifndef OO_INTERFACE
HP_InitFuncPrefix HP_NewStream(HP_SInt32 outBufferMaxSize, flushFunctionType pFlushFunction,
                               unsigned long cookie);
#else
HP_InitFuncPrefix HP_NewStream(HP_SInt32 outBufferMaxSize, MJetlibClient *pClient,
							   unsigned long cookie=0L);
#endif

HP_StdFuncPrefix HP_FlushStreamBuffer(HP_StreamHandleType pStream);

HP_UInt32FuncPrefix HP_FinishStream(HP_StreamHandleType pStream);
#endif

#ifdef HP_TestHarness
extern HP_StreamHandleType HP_GetStreamHandle();
#endif

#define HP_GetStreamBuffLen(pStream) \
        ((HP_UInt32) (pStream)->HP_CurrentBufferLen)
        
#define HP_GetCurrentStreamLen(pStream) \
        ((HP_UInt32) (pStream)->HP_CurrentStreamLen)

#define HP_GetStreamBuffPtr(pStream) \
        (pStream)->HP_OutBuffer

HP_pUByte HP_GetStreamErrorText(HP_SInt16 errorNumber);

HP_StdFuncPrefix HP_Operator(HP_StreamHandleType pStream, HP_UInt16 operatorId);

HP_StdFuncPrefix HP_AttrId(HP_StreamHandleType pStream, HP_UInt16 attrId);

HP_StdFuncPrefix HP_AttrUByte(HP_StreamHandleType pStream, HP_UByte intValue); 

HP_StdFuncPrefix HP_AttrUInt16(HP_StreamHandleType pStream, HP_UInt16 intValue); 

HP_StdFuncPrefix HP_AttrSInt16(HP_StreamHandleType pStream, HP_SInt16 intValue);

HP_StdFuncPrefix HP_AttrUInt32(HP_StreamHandleType pStream, HP_UInt32 intValue); 

HP_StdFuncPrefix HP_AttrSInt32(HP_StreamHandleType pStream, HP_SInt32 intValue);

HP_StdFuncPrefix HP_AttrReal32(HP_StreamHandleType pStream, HP_Real32 realValue); 

HP_StdFuncPrefix HP_AttrXyUByte(HP_StreamHandleType pStream, HP_UByte xValue, HP_UByte yValue); 

HP_StdFuncPrefix HP_AttrXyUInt16(HP_StreamHandleType pStream, HP_UInt16 xValue, HP_UInt16 yValue); 

HP_StdFuncPrefix HP_AttrXySInt16(HP_StreamHandleType pStream, HP_SInt16 xValue, HP_SInt16 yValue); 

HP_StdFuncPrefix HP_AttrXyUInt32(HP_StreamHandleType pStream, HP_UInt32 xValue, HP_UInt32 yValue); 

HP_StdFuncPrefix HP_AttrXySInt32(HP_StreamHandleType pStream, HP_SInt32 xValue, HP_SInt32 yValue); 

HP_StdFuncPrefix HP_AttrXyReal32(HP_StreamHandleType pStream, HP_Real32 xValue, HP_Real32 yValue);

HP_StdFuncPrefix HP_AttrBoxUByte(HP_StreamHandleType pStream, HP_UByte x1Value, HP_UByte y1Value,
        HP_UByte x2Value, HP_UByte y2Value);

HP_StdFuncPrefix HP_AttrBoxUInt16(HP_StreamHandleType pStream, HP_UInt16 x1Value, HP_UInt16 y1Value,
		HP_UInt16 x2Value, HP_UInt16 y2Value);
                        
HP_StdFuncPrefix HP_AttrBoxSInt16(HP_StreamHandleType pStream, HP_SInt16 x1Value, HP_SInt16 y1Value, 
        HP_SInt16 x2Value, HP_SInt16 y2Value);
                        
HP_StdFuncPrefix HP_AttrBoxUInt32(HP_StreamHandleType pStream, HP_UInt32 x1Value, HP_UInt32 y1Value, 
		HP_UInt32 x2Value, HP_UInt32 y2Value);
                        
HP_StdFuncPrefix HP_AttrBoxSInt32(HP_StreamHandleType pStream, HP_SInt32 x1Value, HP_SInt32 y1Value,
        HP_SInt32 x2Value, HP_SInt32 y2Value);
                        
HP_StdFuncPrefix HP_AttrBoxReal32(HP_StreamHandleType pStream, HP_Real32 x1Value, HP_Real32 y1Value, 
		HP_Real32 x2Value, HP_Real32 y2Value);
                        
HP_StdFuncPrefix HP_AttrUByteArray(HP_StreamHandleType pStream, HP_pUByte array, HP_UInt32 arrayLen);

HP_StdFuncPrefix HP_AttrString(HP_StreamHandleType pStream, HP_pUByte array, HP_UInt32 arrayLen);

HP_StdFuncPrefix HP_AttrUInt16Array(HP_StreamHandleType pStream, HP_pUInt16 array, HP_UInt32 arrayLen);
                        
HP_StdFuncPrefix HP_AttrSInt16Array(HP_StreamHandleType pStream, HP_pSInt16 array, HP_UInt32 arrayLen);
                        
HP_StdFuncPrefix HP_AttrUInt32Array(HP_StreamHandleType pStream, HP_pUInt32 array, HP_UInt32 arrayLen);
                        
HP_StdFuncPrefix HP_AttrSInt32Array(HP_StreamHandleType pStream, HP_pSInt32 array, HP_UInt32 arrayLen);
                        
HP_StdFuncPrefix HP_AttrReal32Array(HP_StreamHandleType pStream, HP_pReal32 array, HP_UInt32 arrayLen);

HP_StdFuncPrefix HP_DataUByteArray(HP_StreamHandleType pStream, HP_pUByte array, HP_UInt32 arrayLen);

HP_StdFuncPrefix HP_DataUInt16Array(HP_StreamHandleType pStream, HP_pUInt16 array, HP_UInt32 arrayLen);
                        
HP_StdFuncPrefix HP_DataUInt32Array(HP_StreamHandleType pStream, HP_pUInt32 array, HP_UInt32 arrayLen);

HP_StdFuncPrefix HP_DataReal32Array(HP_StreamHandleType pStream, HP_pReal32 array, HP_UInt32 arrayLen);  

HP_StdFuncPrefix HP_DataUInt16ArrayMSB(HP_StreamHandleType pStream, HP_pUInt16 array, HP_UInt32 arrayLen);
                        
HP_StdFuncPrefix HP_DataUInt32ArrayMSB(HP_StreamHandleType pStream, HP_pUInt32 array, HP_UInt32 arrayLen);

HP_StdFuncPrefix HP_DataReal32ArrayMSB(HP_StreamHandleType pStream, HP_pReal32 array, HP_UInt32 arrayLen);  

HP_StdFuncPrefix HP_EmbeddedDataPrefix(HP_StreamHandleType pStream, HP_UInt32 dataLength); 

HP_StdFuncPrefix HP_EmbeddedDataPrefix32(HP_StreamHandleType pStream, HP_UInt32 dataLength); 

HP_StdFuncPrefix HP_RawUByte(HP_StreamHandleType pStream, HP_UByte intValue);

HP_StdFuncPrefix HP_RawUInt16(HP_StreamHandleType pStream, HP_UInt16 intValue); 

HP_StdFuncPrefix HP_RawUInt16MSB(HP_StreamHandleType pStream, HP_UInt16 intValue);

HP_StdFuncPrefix HP_RawUInt32(HP_StreamHandleType pStream, HP_UInt32 intValue); 

HP_StdFuncPrefix HP_RawUInt32MSB(HP_StreamHandleType pStream, HP_UInt32 intValue); 

HP_StdFuncPrefix HP_RawReal32(HP_StreamHandleType pStream, HP_Real32 realValue);

HP_StdFuncPrefix HP_RawReal32MSB(HP_StreamHandleType pStream, HP_Real32 realValue);

HP_StdFuncPrefix HP_RawUByteArray(HP_StreamHandleType pStream, HP_pUByte array, HP_UInt32 arrayLen);
                    
HP_StdFuncPrefix HP_RawUInt16ToByteArray(HP_StreamHandleType pStream, HP_pUInt16 array, HP_UInt32 arrayLen);

HP_StdFuncPrefix HP_RawHugeUByteArray(HP_StreamHandleType pStream, HP_pCharHuge array, HP_UInt32 arrayLen);
                        
HP_StdFuncPrefix HP_RawUInt16Array(HP_StreamHandleType pStream, HP_pUInt16 array, HP_UInt32 arrayLen);
                        
HP_StdFuncPrefix HP_RawUInt16ArrayMSB(HP_StreamHandleType pStream, HP_pUInt16 array, HP_UInt32 arrayLen);
                        
HP_StdFuncPrefix HP_RawUInt32Array(HP_StreamHandleType pStream, HP_pUInt32 array, HP_UInt32 arrayLen);  

HP_StdFuncPrefix HP_RawUInt32ArrayMSB(HP_StreamHandleType pStream, HP_pUInt32 array, HP_UInt32 arrayLen);  

HP_StdFuncPrefix HP_RawReal32Array(HP_StreamHandleType pStream, HP_pReal32 array, HP_UInt32 arrayLen);

HP_StdFuncPrefix HP_RawReal32ArrayMSB(HP_StreamHandleType pStream, HP_pReal32 array, HP_UInt32 arrayLen);

/****************************************************************************
 *
 * This the the class definition for the ATTRIBUTE structure for the
 * all inclusive call HP_DataStream. Attributes created using this structure
 * carry data about themselves to the HP_DataStream call, are parsed, and
 * turned into the binary data the XL language uses.
 *
 ***************************************************************************/
 
struct ATTRIBUTE
{
    HP_UByte Tag;
    HP_UByte Type;
    HP_UInt32 arrayLen;
    HP_UInt16 cksum;
    union DataValue
    	{
    	HP_UByte   ubyte;
        HP_UInt16  uint16;
        HP_SInt16  sint16;
        HP_UInt32  uint32;
        HP_SInt32  sint32;
        HP_Real32  real32;
        HP_pUByte  ubyte_array;
        HP_pUInt16 uint16_array;
        HP_pSInt16 sint16_array;
        HP_pUInt32 uint32_array;
        HP_pSInt32 sint32_array;
		HP_pReal32 real32_array;

		struct {
        	HP_UByte x, y;
            } UByte_XY;
		struct {
        	HP_UInt16 x, y;
            } UInt16_XY;
		struct {
        	HP_SInt16 x, y;
            } SInt16_XY;
		struct {
        	HP_UInt32 x, y;
            } UInt32_XY;
		struct {
        	HP_SInt32 x, y;
            } SInt32_XY;
		struct {
        	HP_Real32 x, y;
            } Real32_XY;

		struct {
        	HP_UByte x1, y1, x2, y2;
            } UByte_BOX;
		struct {
        	HP_UInt16 x1, y1, x2, y2;
            } UInt16_BOX;
		struct {
        	HP_SInt16 x1, y1, x2, y2;
            } SInt16_BOX;
		struct {
        	HP_UInt32 x1, y1, x2, y2;
            } UInt32_BOX;
		struct {
        	HP_SInt32 x1, y1, x2, y2;
            } SInt32_BOX;
		struct {
        	HP_Real32 x1, y1, x2, y2;
            } Real32_BOX;
        } val;


#ifdef __cplusplus
ATTRIBUTE( HP_UByte aTag, HP_UByte aType )
	{
    Tag = aTag;
    Type = aType;
    arrayLen = 0;
    cksum = (HP_UInt16)aTag * (HP_UInt16)aType;
    }
#endif

};

#ifndef __cplusplus
HP_StdFuncPrefix  HP_InitAttribute(struct ATTRIBUTE *, HP_UByte, HP_UByte);
#endif

HP_StdFuncPrefix  HP_DataStream(HP_StreamHandleType pStream, HP_UByte Operator, ...);



#ifndef JETASM_BUILD
/*------------------------------------------------------------------
 * <PCL XL Stream API Definitions>
 *
 * Descriptions:
 *   Each of the following functions correspond to one of the Cheetah 
 *   operators in the Feature Reference Manual.  All binary binding
 *   stream translations are hidden behind those APIs.  
 *
 * NOTE:
 *   If there are optional sets of parameters for an operator
 *   there will be multiple functions for the each operator.  
 *
 *--------------------------------------------------------------------*/


HP_StdFuncPrefix HP_AddStreamHeader_1(        /* AddStreamHeader signature 1 */
                HP_StreamHandleType pStream,  /* PCL XL Stream Handle */
                HP_UInt16 protocolClass,      /* protocol class number */
                HP_UInt16 protocolRev,        /* protocol class revision */
                HP_pUByte commentArray,       /* any comment desired */
                HP_UInt32 commentArrayLen)    /* length of comment array */
;


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
;




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
;




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
;



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
;




HP_StdFuncPrefix HP_BeginChar_1(           /* BeginChar signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_pUByte fontName,                    /* array for character's font name */
	HP_UInt16 fontNameLen)                 /* length of font name array */
;




HP_StdFuncPrefix HP_BeginFontHeader_1(     /* BeginFontHeader signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_pUByte fontName,                    /* array for font name */
	HP_UInt16 fontNameLen,                 /* length of font name array */
    HP_UByte fontFormat)                   /* number of font format */
;



HP_StdFuncPrefix HP_BeginImage_1(          /* BeginImage signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UByte colorMappingEnum,             /* color mapping enumeration */
    HP_UByte  colorDepthEnum,              /* color depth enumeration */
    HP_UInt16 sourceWidth,                 /* source image width */
    HP_UInt16 sourceHeight,                /* source image height */
    HP_UInt16 destWidth,                   /* destination height in user units */
    HP_UInt16 destHeight)                  /* destination width in user units */

;



HP_StdFuncPrefix HP_BeginImage_2(          /* BeginImage signature 2 */

	HP_StreamHandleType pStream,           /* stream handle */
    HP_UByte colorMappingEnum,             /* color mapping enumeration */
    HP_UByte  colorDepthEnum,              /* color depth enumeration */
    HP_UInt16 sourceWidth,                 /* source image width */
    HP_UInt16 sourceHeight,                /* source image height */
    HP_UInt16 destWidth,                   /* destination height in user units */
	HP_UInt16 destHeight)                  /* destination width in user units */

;




HP_StdFuncPrefix HP_BeginImage_3(          /* BeginImage signature 3 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UByte colorMappingEnum,             /* color mapping enumeration */
    HP_UByte  colorDepthEnum,              /* color depth enumeration */
    HP_UInt32 sourceWidth,                 /* source image width */
    HP_UInt32 sourceHeight,                /* source image height */
	HP_UInt16 destWidth,                   /* destination height in user units */
    HP_UInt16 destHeight)                  /* destination width in user units */

;




HP_StdFuncPrefix HP_BeginPage_1(           /* BeginPage signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UByte orientationEnum,              /* page orientation enumeration */
    HP_UByte mediaSizeEnum)                /* media size enumeration */
;


HP_StdFuncPrefix HP_BeginPage_2(           /* BeginPage signature 2 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UByte orientationEnum,              /* page orientation enumeration */
    HP_UInt16 xSize,                       /* custom media x-axis size */
    HP_UInt16 ySize,                       /* custom media y-axis size */
    HP_UByte unitOfMeasureEnum)            /* unit of measure enumeration */
;


HP_StdFuncPrefix HP_BeginPage_3(           /* BeginPage signature 3 */

    HP_StreamHandleType pStream,           /* stream handle */
	HP_UByte orientationEnum,              /* page orientation enumeration */
    HP_UByte mediaSizeEnum,                /* media size enumeration */
	HP_UByte mediaSourceEnum)              /* media source enumeration */
;

HP_StdFuncPrefix HP_BeginPage_4(           /* BeginPage signature 4 */

    HP_StreamHandleType pStream,           /* stream handle */
	HP_UByte orientationEnum,              /* page orientation enumeration */
    HP_UInt16 xSize,                       /* custom media x-axis size */
    HP_UInt16 ySize,                       /* custom media y-axis size */
    HP_UByte unitOfMeasureEnum,            /* unit of measure enumeration */
    HP_UByte mediaSourceEnum)              /* media Source enumeration */
;

HP_StdFuncPrefix HP_BeginPage_5(           /* BeginPage signature 1 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_UByte orientationEnum,              /* page orientation enumeration */
	HP_UByte mediaSizeEnum,                /* media size enumeration */
    HP_UByte DuplexModeEnum,               /* Set/Clear duplex mode */
    HP_UByte DuplexPageSide)               /* Front/Back to start */
;

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
;

HP_StdFuncPrefix HP_BeginRastPattern_2(    /* BeginRastPattern signature 3 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UByte colorMappingEnum,             /* color mapping enumeration */
    HP_UByte  colorDepthEnum,              /* color depth enumeration */
    HP_UInt32 sourceWidth,                 /* source image width */
    HP_UInt32 sourceHeight,                /* source image height */
    HP_UInt32 xHeight,                     /* destination size x-axis */
    HP_UInt32 yHeight,                     /* destination size y-axis */
    HP_SInt16 patternDefineID)             /* pattern identifier */
;





HP_StdFuncPrefix HP_BeginScan_1(           /* BeginScan signature 1 */

	HP_StreamHandleType pStream)           /* stream handle */

;





HP_StdFuncPrefix HP_BeginSession_1(        /* BeginSession signature 1 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_UInt16     xValue,                  /* x units of measure */
	HP_UInt16     yValue,                  /* y units of measure */
	HP_UByte      MeasureEnum)             /* measure enumeration */
;


HP_StdFuncPrefix HP_BeginSession_2(        /* BeginSession signature 2 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_UInt16     xValue,                  /* x units of measure */
	HP_UInt16     yValue,                  /* y units of measure */
	HP_UByte      MeasureEnum,             /* measure enumeration */
	HP_UByte      errorReportingEnum)      /* error reporting enumeration */
;


HP_StdFuncPrefix HP_BeginStream_1(         /* BeginStream signature 1 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_pUByte streamName,                  /* array for stream name */
	HP_UInt16 streamNameLen)               /* length of stream name array */
;


HP_StdFuncPrefix HP_BezierPath_1(          /* BezierPath signature 1 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_UInt16 numberOfPoints,              /* number of points */
	HP_UByte pointTypeEnum)                /* enum for data type of points */

;




HP_StdFuncPrefix HP_BezierPath_2(          /* BezierPath signature 2 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_SInt16 ControlPointX1,              /* control point x1 */
	HP_SInt16 ControlPointY1,              /* control point y1 */
	HP_SInt16 ControlPointX2,              /* control point x2 */
	HP_SInt16 ControlPointY2,              /* control point y2 */
    HP_SInt16 EndPointX,                   /* ending point x */
	HP_SInt16 EndPointY)                   /* ending point y */
;




HP_StdFuncPrefix HP_BezierPath_3(          /* BezierPath signature 3 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UInt16 ControlPointX1,              /* control point x1 */
    HP_UInt16 ControlPointY1,              /* control point y1 */
    HP_UInt16 ControlPointX2,              /* control point x2 */
    HP_UInt16 ControlPointY2,              /* control point y2 */
    HP_UInt16 EndPointX,                   /* ending point x */
    HP_UInt16 EndPointY)                   /* ending point y */
;



HP_StdFuncPrefix HP_BezierPath_4(          /* BezierPath signature 4 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_SInt32 ControlPointX1,              /* control point x1 */
    HP_SInt32 ControlPointY1,              /* control point y1 */
    HP_SInt32 ControlPointX2,              /* control point x2 */
    HP_SInt32 ControlPointY2,              /* control point y2 */
    HP_SInt32 EndPointX,                   /* ending point x */
    HP_SInt32 EndPointY)                   /* ending point y */
;



HP_StdFuncPrefix HP_BezierPath_5(          /* BezierPath signature 5 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_Real32 ControlPointX1,              /* control point x1 */
    HP_Real32 ControlPointY1,              /* control point y1 */
    HP_Real32 ControlPointX2,              /* control point x2 */
    HP_Real32 ControlPointY2,              /* control point y2 */
    HP_Real32 EndPointX,                   /* ending point x */
    HP_Real32 EndPointY)                   /* ending point y */
;



HP_StdFuncPrefix HP_BezierRelPath_1(       /* BezierRelPath signature 1 */

	HP_StreamHandleType pStream,           /* stream handle */
    HP_UInt16 numberOfPoints,              /* number of points */
	HP_UByte pointTypeEnum)                /* enum for data type of points */

;




HP_StdFuncPrefix HP_BezierRelPath_2(       /* BezierRelPath signature 2 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_SInt16 ControlPointX1,              /* control point x1 */
    HP_SInt16 ControlPointY1,              /* control point y1 */
    HP_SInt16 ControlPointX2,              /* control point x2 */
    HP_SInt16 ControlPointY2,              /* control point y2 */
    HP_SInt16 EndPointX,                   /* ending point x */
    HP_SInt16 EndPointY)                   /* ending point y */
;




HP_StdFuncPrefix HP_BezierRelPath_3(       /* BezierRelPath signature 3 */

	HP_StreamHandleType pStream,           /* stream handle */
    HP_UInt16 ControlPointX1,              /* control point x1 */
    HP_UInt16 ControlPointY1,              /* control point y1 */
    HP_UInt16 ControlPointX2,              /* control point x2 */
    HP_UInt16 ControlPointY2,              /* control point y2 */
    HP_UInt16 EndPointX,                   /* ending point x */
    HP_UInt16 EndPointY)                   /* ending point y */
;



HP_StdFuncPrefix HP_BezierRelPath_4(       /* BezierRelPath signature 4 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_SInt32 ControlPointX1,              /* control point x1 */
    HP_SInt32 ControlPointY1,              /* control point y1 */
    HP_SInt32 ControlPointX2,              /* control point x2 */
    HP_SInt32 ControlPointY2,              /* control point y2 */
    HP_SInt32 EndPointX,                   /* ending point x */
    HP_SInt32 EndPointY)                   /* ending point y */
;



HP_StdFuncPrefix HP_BezierRelPath_5(       /* BezierRelPath signature 5 */

	HP_StreamHandleType pStream,           /* stream handle */
    HP_Real32 ControlPointX1,              /* control point x1 */
	HP_Real32 ControlPointY1,              /* control point y1 */
    HP_Real32 ControlPointX2,              /* control point x2 */
	HP_Real32 ControlPointY2,              /* control point y2 */
    HP_Real32 EndPointX,                   /* ending point x */
    HP_Real32 EndPointY)                   /* ending point y */
;


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
;




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
;



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
;




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
;




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
;




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
;



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
;




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
;




HP_StdFuncPrefix HP_CloseDataSource_1(     /* CloseDataSource signature 1 */

    HP_StreamHandleType pStream)           /* stream handle */
;




HP_StdFuncPrefix HP_CloseSubPath_1(        /* CloseSubPath signature 1 */

    HP_StreamHandleType pStream)           /* stream handle */
;
 




HP_StdFuncPrefix HP_Comment_1(             /* Comment signature 1 */

	HP_StreamHandleType pStream,           /* stream handle */
    HP_pUByte array,                       /* array for comment data/text */
	HP_UInt32 arrayLen)                    /* length of comment data array */
;

HP_StdFuncPrefix HP_Comment_2(             /* Comment signature 1 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_pCharHuge array)                    /* length of comment data array */
;

HP_StdFuncPrefix HP_EchoComment_1(         /* EchoComment signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
	HP_pUByte array,                       /* array for comment data/text */
    HP_UInt32 arrayLen)                    /* length of comment data array */
;

HP_StdFuncPrefix HP_Ellipse_1(             /* Ellipse signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_SInt16 x1Val,                       /* bounding box top left x */
    HP_SInt16 y1Val,                       /* bounding box top left y */
    HP_SInt16 x2Val,                       /* bounding box bottom right x */
    HP_SInt16 y2Val)                       /* bounding box bottom right y */
;




HP_StdFuncPrefix HP_Ellipse_2(             /* Ellipse signature 2 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UInt16 x1Val,                       /* bounding box top left x */
    HP_UInt16 y1Val,                       /* bounding box top left y */
    HP_UInt16 x2Val,                       /* bounding box bottom right x */
    HP_UInt16 y2Val)                       /* bounding box bottom right y */
;



HP_StdFuncPrefix HP_Ellipse_3(             /* Ellipse signature 3 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_SInt32 x1Val,                       /* bounding box top left x */
    HP_SInt32 y1Val,                       /* bounding box top left y */
    HP_SInt32 x2Val,                       /* bounding box bottom right x */
    HP_SInt32 y2Val)                       /* bounding box bottom right y */
;



HP_StdFuncPrefix HP_Ellipse_4(             /* Ellipse signature 4 */

    HP_StreamHandleType pStream,           /* stream handle */
	HP_Real32 x1Val,                       /* bounding box top left x */
    HP_Real32 y1Val,                       /* bounding box top left y */
	HP_Real32 x2Val,                       /* bounding box bottom right x */
    HP_Real32 y2Val)                       /* bounding box bottom right y */
;




HP_StdFuncPrefix HP_EllipsePath_1(         /* EllipsePath signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_SInt16 x1Val,                       /* bounding box top left x */
    HP_SInt16 y1Val,                       /* bounding box top left y */
    HP_SInt16 x2Val,                       /* bounding box bottom right x */
    HP_SInt16 y2Val)                       /* bounding box bottom right y */
;




HP_StdFuncPrefix HP_EllipsePath_2(         /* EllipsePath signature 2 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UInt16 x1Val,                       /* bounding box top left x */
    HP_UInt16 y1Val,                       /* bounding box top left y */
    HP_UInt16 x2Val,                       /* bounding box bottom right x */
    HP_UInt16 y2Val)                       /* bounding box bottom right y */
;



HP_StdFuncPrefix HP_EllipsePath_3(         /* EllipsePath signature 3 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_SInt32 x1Val,                       /* bounding box top left x */
    HP_SInt32 y1Val,                       /* bounding box top left y */
    HP_SInt32 x2Val,                       /* bounding box bottom right x */
    HP_SInt32 y2Val)                       /* bounding box bottom right y */
;



HP_StdFuncPrefix HP_EllipsePath_4(         /* EllipsePath signature 4 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_Real32 x1Val,                       /* bounding box top left x */
    HP_Real32 y1Val,                       /* bounding box top left y */
    HP_Real32 x2Val,                       /* bounding box bottom right x */
    HP_Real32 y2Val)                       /* bounding box bottom right y */
;



HP_StdFuncPrefix HP_EndChar_1(             /* EndChar signature 1 */

    HP_StreamHandleType pStream)           /* stream handle */
;




HP_StdFuncPrefix HP_EndFontHeader_1(       /* EndFontHeader signature 1 */

    HP_StreamHandleType pStream)           /* stream handle */
;




HP_StdFuncPrefix HP_EndImage_1(            /* EndImage signature 1 */

    HP_StreamHandleType pStream)           /* stream handle */
;



HP_StdFuncPrefix HP_EndPage_1(             /* EndPage signature 1 */

    HP_StreamHandleType pStream)           /* stream handle */
;


HP_StdFuncPrefix HP_EndPage_2(             /* EndPage signature 2 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UInt16 pageCopies)                  /* number of copies */
;


HP_StdFuncPrefix HP_EndPage_3(             /* EndPage signature 3 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UInt16 pageCopies,                  /* number of copies */
    HP_UByte mediaSourceEnum)              /* media source enumeration */
;

HP_StdFuncPrefix HP_EndPage_4(             /* EndPage signature 4 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UByte mediaSourceEnum)              /* media source enumeration */
;


HP_StdFuncPrefix HP_EndRastPattern_1(      /* EndRastPattern signature 1 */

	HP_StreamHandleType pStream)           /* stream handle */
;




HP_StdFuncPrefix HP_EndScan_1(             /* EndScan signature 1 */

    HP_StreamHandleType pStream)           /* stream handle */
;



HP_StdFuncPrefix HP_EndSession_1(          /* EndSession signature 1 */

    HP_StreamHandleType pStream)           /* stream handle */
;


HP_StdFuncPrefix HP_EndStream_1(           /* EndStream signature 1 */

    HP_StreamHandleType pStream)           /* stream handle */
;


HP_StdFuncPrefix HP_LinePath_1(            /* LinePath signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UInt16 numberOfPoints,              /* number of points */
    HP_UByte pointTypeEnum)                /* enum for data type of points */

;


HP_StdFuncPrefix HP_LinePath_2(            /* LinePath signature 2 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_SInt16 EndPointX,                   /* ending point x */
    HP_SInt16 EndPointY)                   /* ending point y */

;




HP_StdFuncPrefix HP_LinePath_3(            /* LinePath signature 3 */

	HP_StreamHandleType pStream,           /* stream handle */
    HP_UInt16 EndPointX,                   /* ending point x */
	HP_UInt16 EndPointY)                   /* ending point y */

;




HP_StdFuncPrefix HP_LinePath_4(            /* LinePath signature 4 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_SInt32 EndPointX,                   /* ending point x */
    HP_SInt32 EndPointY)                   /* ending point y */

;




HP_StdFuncPrefix HP_LinePath_5(            /* LinePath signature 5 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_Real32 EndPointX,                   /* ending point x */
    HP_Real32 EndPointY)                   /* ending point y */

;




HP_StdFuncPrefix HP_LineRelPath_1(         /* LineRelPath signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UInt16 numberOfPoints,              /* number of points */
    HP_UByte pointTypeEnum)                /* enum for data type of points */

;




HP_StdFuncPrefix HP_LineRelPath_2(         /* LineRelPath signature 2 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_SInt16 EndPointX,                   /* ending point x */
    HP_SInt16 EndPointY)                   /* ending point y */

;




HP_StdFuncPrefix HP_LineRelPath_3(         /* LineRelPath signature 3 */

	HP_StreamHandleType pStream,           /* stream handle */
    HP_UInt16 EndPointX,                   /* ending point x */
    HP_UInt16 EndPointY)                   /* ending point y */

;




HP_StdFuncPrefix HP_LineRelPath_4(         /* LineRelPath signature 4 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_SInt32 EndPointX,                   /* ending point x */
    HP_SInt32 EndPointY)                   /* ending point y */

;




HP_StdFuncPrefix HP_LineRelPath_5(         /* LineRelPath signature 5 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_Real32 EndPointX,                   /* ending point x */
    HP_Real32 EndPointY)                   /* ending point y */

;




HP_StdFuncPrefix HP_NewPath_1(             /* NewPath signature 1 */

    HP_StreamHandleType pStream)           /* stream handle */
;
 

/* This is a temporary API until the firmware catches up to the Vol. 2 spec */

HP_StdFuncPrefix HP_OpenDataSource_Temp1(  /* OpenDataSource signature Temp1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UByte dataSourceEnum)               /* dataSourceEnumeration */
;


HP_StdFuncPrefix HP_OpenDataSource_1(      /* OpenDataSource signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
	HP_UByte dataSourceEnum,               /* dataSourceEnumeration */
    HP_UByte dataOrgEnum)                  /* dataOrgEnumeration */
;



HP_StdFuncPrefix HP_PaintPath_1(           /* PaintPath signature 1 */
    
	HP_StreamHandleType pStream)           /* stream handle */
;




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
;




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
;



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
;




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
;




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
;




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
;



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
;




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
;




HP_StdFuncPrefix HP_PopGS_1(               /* PopGS signature 1 */
    
    HP_StreamHandleType pStream)           /* stream handle */
;




HP_StdFuncPrefix HP_PushGS_1(              /* PushGS signature 1 */

    HP_StreamHandleType pStream)           /* stream handle */ 
;


HP_StdFuncPrefix HP_Query_1(               /* Query signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_pUByte queryKey,                    /* array for query string */
    HP_UInt16 queryKeyLen)                 /* length of query string array */
;


HP_StdFuncPrefix HP_ReadChar_1(            /* ReadChar signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UInt16 CharCodeData,                /* character code value */
    HP_UInt32 CharSize)                    /* length of char code data */
;

                                                  
HP_StdFuncPrefix HP_ReadFontHeader_1(      /* ReadFontHeader signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UInt16 FontHeaderLen)               /* font header length */
;


HP_StdFuncPrefix HP_ReadImage_1(           /* ReadImage signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UInt16 startLine,                   /* start line for reading raster */
    HP_UInt16 blockHeight,                 /* line height of block */
    HP_UByte  compressionEnum)             /* compression mode enumeration */
;
            


HP_StdFuncPrefix HP_ReadImage_2(           /* ReadImage signature 2 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UInt32 startLine,                   /* start line for reading raster */
    HP_UInt32 blockHeight,                 /* line height of block */
    HP_UByte  compressionEnum)             /* compression mode enumeration */
;
            


HP_StdFuncPrefix HP_ReadRastPattern_1(     /* ReadRastPattern signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UInt16 startLine,                   /* start line for reading raster */
    HP_UInt16 blockHeight,                 /* line height of block */
	HP_UByte  compressionEnum)             /* compression mode enumeration */
;




HP_StdFuncPrefix HP_ReadRastPattern_2(     /* ReadRastPattern signature 2 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UInt32 startLine,                   /* start line for reading raster */
	HP_UInt32 blockHeight,                 /* line height of block */
    HP_UByte  compressionEnum)             /* compression mode enumeration */
;


HP_StdFuncPrefix HP_ReadStream_1(          /* ReadStream signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UInt32 streamDataLength)            /* stream data length */
;
HP_StdFuncPrefix HP_ExecStream_1(          /* ExecStream signature 1 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_UInt32 streamDataLength)            /* stream data length */
;
HP_StdFuncPrefix HP_RemoveStream_1(        /* RemoveStream signature 1 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_UInt32 streamDataLength)            /* stream data length */
;


HP_StdFuncPrefix HP_Rectangle_1(           /* Rectangle signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_SInt16 x1Val,                       /* bounding box top left x */
    HP_SInt16 y1Val,                       /* bounding box top left y */
    HP_SInt16 x2Val,                       /* bounding box bottom right x */
    HP_SInt16 y2Val)                       /* bounding box bottom right y */
;



HP_StdFuncPrefix HP_Rectangle_2(           /* Rectangle signature 2 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UInt16 x1Val,                       /* bounding box top left x */
    HP_UInt16 y1Val,                       /* bounding box top left y */
    HP_UInt16 x2Val,                       /* bounding box bottom right x */
    HP_UInt16 y2Val)                       /* bounding box bottom right y */
;



HP_StdFuncPrefix HP_Rectangle_3(           /* Rectangle signature 3 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_SInt32 x1Val,                       /* bounding box top left x */
    HP_SInt32 y1Val,                       /* bounding box top left y */
    HP_SInt32 x2Val,                       /* bounding box bottom right x */
    HP_SInt32 y2Val)                       /* bounding box bottom right y */
;

HP_StdFuncPrefix HP_Rectangle_4(           /* Rectangle signature 4 */

	HP_StreamHandleType pStream,           /* stream handle */
    HP_Real32 x1Val,                       /* bounding box top left x */
    HP_Real32 y1Val,                       /* bounding box top left y */
    HP_Real32 x2Val,                       /* bounding box bottom right x */
    HP_Real32 y2Val)                       /* bounding box bottom right y */
;



HP_StdFuncPrefix HP_RectanglePath_1(       /* RectanglePath signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_SInt16 x1Val,                       /* bounding box top left x */
    HP_SInt16 y1Val,                       /* bounding box top left y */
    HP_SInt16 x2Val,                       /* bounding box bottom right x */
    HP_SInt16 y2Val)                       /* bounding box bottom right y */
;



HP_StdFuncPrefix HP_RectanglePath_2(       /* RectanglePath signature 2 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UInt16 x1Val,                       /* bounding box top left x */
    HP_UInt16 y1Val,                       /* bounding box top left y */
    HP_UInt16 x2Val,                       /* bounding box bottom right x */
    HP_UInt16 y2Val)                       /* bounding box bottom right y */
;



HP_StdFuncPrefix HP_RectanglePath_3(       /* RectanglePath signature 3 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_SInt32 x1Val,                       /* bounding box top left x */
    HP_SInt32 y1Val,                       /* bounding box top left y */
    HP_SInt32 x2Val,                       /* bounding box bottom right x */
    HP_SInt32 y2Val)                       /* bounding box bottom right y */
;



HP_StdFuncPrefix HP_RectanglePath_4(       /* RectanglePath signature 4 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_Real32 x1Val,                       /* bounding box top left x */
    HP_Real32 y1Val,                       /* bounding box top left y */
	HP_Real32 x2Val,                       /* bounding box bottom right x */
    HP_Real32 y2Val)                       /* bounding box bottom right y */
;



HP_StdFuncPrefix HP_RemoveFont_1(          /* RemoveFont signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_pUByte fontName,                    /* array for font name */
	HP_UInt16 fontNameLen)                 /* length of font name array */
;

          


HP_StdFuncPrefix HP_RoundRectangle_1(      /* RoundRectangle signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_SInt16 x1Val,                       /* bounding box top left x */
    HP_SInt16 y1Val,                       /* bounding box top left y */
    HP_SInt16 x2Val,                       /* bounding box bottom right x */
    HP_SInt16 y2Val,                       /* bounding box bottom right y */
    HP_UInt16 xEllipse,                    /* ellipse x dimension */
    HP_UInt16 yEllipse)                    /* ellipse y dimension */
;



HP_StdFuncPrefix HP_RoundRectangle_2(      /* RoundRectangle signature 2 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UInt16 x1Val,                       /* bounding box top left x */
    HP_UInt16 y1Val,                       /* bounding box top left y */
    HP_UInt16 x2Val,                       /* bounding box bottom right x */
    HP_UInt16 y2Val,                       /* bounding box bottom right y */
    HP_UInt16 xEllipse,                    /* ellipse x dimension */
    HP_UInt16 yEllipse)                    /* ellipse y dimension */
;




HP_StdFuncPrefix HP_RoundRectangle_3(      /* RoundRectangle signature 2 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_SInt32 x1Val,                       /* bounding box top left x */
    HP_SInt32 y1Val,                       /* bounding box top left y */
    HP_SInt32 x2Val,                       /* bounding box bottom right x */
    HP_SInt32 y2Val,                       /* bounding box bottom right y */
    HP_UInt32 xEllipse,                    /* ellipse x dimension */
	HP_UInt32 yEllipse)                    /* ellipse y dimension */
;




HP_StdFuncPrefix HP_RoundRectangle_4(      /* RoundRectangle signature 2 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_Real32 x1Val,                       /* bounding box top left x */
	HP_Real32 y1Val,                       /* bounding box top left y */
    HP_Real32 x2Val,                       /* bounding box bottom right x */
    HP_Real32 y2Val,                       /* bounding box bottom right y */
    HP_Real32 xEllipse,                    /* ellipse x dimension */
    HP_Real32 yEllipse)                    /* ellipse y dimension */
;



HP_StdFuncPrefix HP_RoundRectanglePath_1(  /* RoundRectanglePath signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_SInt16 x1Val,                       /* bounding box top left x */
    HP_SInt16 y1Val,                       /* bounding box top left y */
    HP_SInt16 x2Val,                       /* bounding box bottom right x */
    HP_SInt16 y2Val,                       /* bounding box bottom right y */
    HP_UInt16 xEllipse,                    /* ellipse x dimension */
    HP_UInt16 yEllipse)                    /* ellipse y dimension */
;



HP_StdFuncPrefix HP_RoundRectanglePath_2(  /* RoundRectanglePath signature 2 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UInt16 x1Val,                       /* bounding box top left x */
    HP_UInt16 y1Val,                       /* bounding box top left y */
    HP_UInt16 x2Val,                       /* bounding box bottom right x */
    HP_UInt16 y2Val,                       /* bounding box bottom right y */
    HP_UInt16 xEllipse,                    /* ellipse x dimension */
    HP_UInt16 yEllipse)                    /* ellipse y dimension */
;




HP_StdFuncPrefix HP_RoundRectanglePath_3(  /* RoundRectanglePath signature 2 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_SInt32 x1Val,                       /* bounding box top left x */
    HP_SInt32 y1Val,                       /* bounding box top left y */
	HP_SInt32 x2Val,                       /* bounding box bottom right x */
    HP_SInt32 y2Val,                       /* bounding box bottom right y */
	HP_SInt32 xEllipse,                    /* ellipse x dimension */
    HP_SInt32 yEllipse)                    /* ellipse y dimension */
;




HP_StdFuncPrefix HP_RoundRectanglePath_4(  /* RoundRectanglePath signature 2 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_Real32 x1Val,                       /* bounding box top left x */
    HP_Real32 y1Val,                       /* bounding box top left y */
    HP_Real32 x2Val,                       /* bounding box bottom right x */
    HP_Real32 y2Val,                       /* bounding box bottom right y */
    HP_Real32 xEllipse,                    /* ellipse x dimension */
    HP_Real32 yEllipse)                    /* ellipse y dimension */
;



HP_StdFuncPrefix HP_ScanLineRel_1(         /* ScanLineRel signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_SInt16 yStart,                      /* y start for scan line */
    HP_SInt16 xStart,                      /* x start for scan line */
    HP_UInt16 *XPairArray,                 /* array of x pairs */
    HP_UInt16 numberOfXPairs)              /* number of x pairs in scan line */
;


HP_StdFuncPrefix HP_ScanLineRelBuffInit_1( /* ScanLineRelBuffInit signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UInt16 *scanLineBuff,               /* scan line buffer */
    HP_UInt16 scanLineBuffLen)             /* scan line buffer len */
;


HP_StdFuncPrefix HP_ScanLineRelBuffFlush_1( /* ScanLineRelBuffFlush signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UInt16 *scanLineBuff)               /* scan line buffer */
;
    

HP_StdFuncPrefix HP_ScanLineRelBuffAdd_1( /* ScanLineRelBuffAdd signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UInt16 yStart,                      /* y start for scan line */
	HP_UInt16 xStart,                      /* x start for scan line */
    HP_UInt16 *XPairArray,                  /* x pair array */
	HP_UInt16 numberOfXPairs,              /* number of x pairs in scan line */
    HP_UInt16 *scanLineBuff)               /* scan line buffer */
;
    
HP_StdFuncPrefix HP_SetBrushSource_Gray1(  /* SetBrushSource Gray sig. 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_Real32 grayLevel)                   /* gray level value (0-1.0) */
;



HP_StdFuncPrefix HP_SetBrushSource_Null1(  /* SetBrushSource Null sig. 1 */

    HP_StreamHandleType pStream)           /* stream handle */
;


HP_StdFuncPrefix HP_SetBrushSource_Patt1(  /* SetBrushSource Pattern sig. 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_SInt16 patternSelectID)             /* pattern select identifier */
;


HP_StdFuncPrefix HP_SetBrushSource_Patt2(  /* SetBrushSource Pattern sig. 2 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_SInt16 xExtent,                     /* pattern dest x extent */
    HP_SInt16 yExtent,                     /* pattern dest y extent */
    HP_SInt16 patternSelectID)             /* pattern select identifier */
;



HP_StdFuncPrefix HP_SetBrushSource_RGB1(   /* SetBrushSource RGB sig. 1 */
    
    HP_StreamHandleType pStream,           /* stream handle */
    HP_Real32 redValue,                    /* red value */
    HP_Real32 greenValue,                  /* green value */
    HP_Real32 blueValue)                   /* blue value */
;


HP_StdFuncPrefix HP_SetBrushSource_Primary(/* SetBrushSource Primary */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_UByte colorSpaceEnum,               /* Primary Depth */
	HP_pUByte colorData,                   /* array for color data */
	HP_UInt16 colorDataLen)                /* length of color data array in bytes */
;

HP_StdFuncPrefix HP_SetCharAngle_1(        /* SetCharAngle signature 1 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_SInt16 charAngle)                   /* character angle */
;


HP_StdFuncPrefix HP_SetCharAngle_2(        /* SetCharAngle signature 2 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_Real32 charAngle)                   /* character angle */
;




HP_StdFuncPrefix HP_SetCharScale_1(        /* SetCharScale signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_Real32 xVal,                        /* character x scale value */
    HP_Real32 yVal)                        /* character y scale value */
;




HP_StdFuncPrefix HP_SetCharShear_1(        /* SetCharShear signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_SInt16 xVal,                        /* character x shear value */
    HP_SInt16 yVal)                        /* character y shear value */
;




HP_StdFuncPrefix HP_SetCharShear_2(        /* SetCharShear signature 2 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_Real32 xVal,                        /* character x shear value */
    HP_Real32 yVal)                        /* character y shear value */
;

HP_StdFuncPrefix HP_SetCharAttribute_1(    /* SetCharAttribute signature 1 */
    HP_StreamHandleType pStream,           /* stream handle */
    HP_UByte writingMode)                  /* Writing mode enum */
;

HP_StdFuncPrefix HP_SetClipIntersect_1(    /* SetClipIntersect signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UByte clipRegionEnum)               /* clip region enumeration */
;


HP_StdFuncPrefix HP_SetClipMode_1(         /* SetClipMode signature 1 */

	HP_StreamHandleType pStream,           /* stream handle */
    HP_UByte clipModeEnum )                /* clip mode enumeration */
;



HP_StdFuncPrefix HP_SetClipRectangle_1(    /* SetClipRectangle signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_SInt16 x1Val,                       /* bounding box top left x */
    HP_SInt16 y1Val,                       /* bounding box top left y */
    HP_SInt16 x2Val,                       /* bounding box bottom right x */
    HP_SInt16 y2Val,                       /* bounding box bottom right y */
    HP_UByte clipRegionEnum)               /* clip region enumeration */
;




HP_StdFuncPrefix HP_SetClipRectangle_2(    /* SetClipRectangle signature 2 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UInt16 x1Val,                       /* bounding box top left x */
    HP_UInt16 y1Val,                       /* bounding box top left y */
    HP_UInt16 x2Val,                       /* bounding box bottom right x */
    HP_UInt16 y2Val,                       /* bounding box bottom right y */
    HP_UByte clipRegionEnum)               /* clip region enumeration */
;




HP_StdFuncPrefix HP_SetClipRectangle_3(    /* SetClipRectangle signature 3 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_SInt32 x1Val,                       /* bounding box top left x */
    HP_SInt32 y1Val,                       /* bounding box top left y */
    HP_SInt32 x2Val,                       /* bounding box bottom right x */
    HP_SInt32 y2Val,                       /* bounding box bottom right y */
    HP_UByte clipRegionEnum)               /* clip region enumeration */
;




HP_StdFuncPrefix HP_SetClipRectangle_4(    /* SetClipRectangle signature 4 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_Real32 x1Val,                       /* bounding box top left x */
    HP_Real32 y1Val,                       /* bounding box top left y */
	HP_Real32 x2Val,                       /* bounding box bottom right x */
	HP_Real32 y2Val,                       /* bounding box bottom right y */
    HP_UByte clipRegionEnum)               /* clip region enumeration */
;


HP_StdFuncPrefix HP_SetClipReplace_1(      /* SetClipReplace signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UByte clipRegionEnum)               /* clip region enumeration */
;


HP_StdFuncPrefix HP_SetClipToPage_1(       /* SetClipToPage signature 1 */

    HP_StreamHandleType pStream)           /* stream handle */
;



HP_StdFuncPrefix HP_SetColorSpace_1(       /* SetColorSpace signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UByte colorSpaceEnum)               /* color space enumeration */
;


HP_StdFuncPrefix HP_SetColorSpace_2(       /* SetColorSpace signature 3 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_UByte colorSpaceEnum,               /* color space enumeration */
	HP_UByte paletteDepthEnum,             /* palette depth enumeration */
	HP_pUByte array,                       /* palette data array */
	HP_UInt32 arrayLen)                    /* palette data array length */
;

HP_StdFuncPrefix HP_SetColorSpace_3(       /* SetColorSpace signature 5 */

	HP_StreamHandleType pStream,           /* Stream handle */
	HP_UByte colorSpaceEnum,               /* Colorimetric Enumeration */
	HP_pReal32 xyChroma,                   /* Array for XY Chromiticity Error */
	HP_pReal32 whitePoint,                 /* Array for WhitePointReference */
	HP_pReal32 crgbMinMax,                 /* Array for CRGBMinMax */
	HP_pReal32 gammaGain)                  /* Array for GammaGain */
;

HP_StdFuncPrefix HP_SetColorSpace_4(       /* SetColorSpace Signature 6 */

	HP_StreamHandleType pStream,           /* Stream handle */
	HP_UByte colorSpaceEnum,               /* Colorimetric Enumeration */
	HP_Real32 minL,
	HP_Real32 maxL,
	HP_Real32 minA,
	HP_Real32 maxA,
	HP_Real32 minB,
	HP_Real32 maxB)
;

HP_StdFuncPrefix HP_SetColorAdjustment_1(  /* SetColorAdjustment Signature 6 */

	HP_StreamHandleType pStream,
	HP_SInt16 lightness,
	HP_SInt16 saturation)
;

HP_StdFuncPrefix HP_SetUserUnits_1(        /* SetUserUnits signature 1 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_UByte measure,
	HP_Real32 xVal,                        /* absolute x location */
	HP_Real32 yVal)                        /* absolute y location */

;

HP_StdFuncPrefix HP_SetCursor_1(           /* SetCursor signature 1 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_SInt16 xVal,                        /* absolute x location */
	HP_SInt16 yVal)                        /* absolute y location */

;




HP_StdFuncPrefix HP_SetCursor_2(           /* SetCursor signature 2 */
    
    HP_StreamHandleType pStream,           /* stream handle */
    HP_UInt16 xVal,                        /* absolute x location */
    HP_UInt16 yVal)                        /* absolute y location */

;




HP_StdFuncPrefix HP_SetCursor_3(           /* SetCursor signature 3 */
    
    HP_StreamHandleType pStream,           /* stream handle */
    HP_SInt32 xVal,                        /* absolute x location */
    HP_SInt32 yVal)                        /* absolute y location */

;




HP_StdFuncPrefix HP_SetCursor_4(           /* SetCursor signature 4 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_Real32 xVal,                        /* absolute x location */
    HP_Real32 yVal)                        /* absolute y location */

;




HP_StdFuncPrefix HP_SetCursorRel_1(        /* SetCursorRel signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_SInt16 xVal,                        /* absolute x location */
    HP_SInt16 yVal)                        /* absolute y location */

;




HP_StdFuncPrefix HP_SetCursorRel_2(        /* SetCursorRel signature 2 */
    
    HP_StreamHandleType pStream,           /* stream handle */
    HP_UInt16 xVal,                        /* absolute x location */
	HP_UInt16 yVal)                        /* absolute y location */

;




HP_StdFuncPrefix HP_SetCursorRel_3(        /* SetCursorRel signature 3 */
    
    HP_StreamHandleType pStream,           /* stream handle */
    HP_SInt32 xVal,                        /* absolute x location */
    HP_SInt32 yVal)                        /* absolute y location */

;




HP_StdFuncPrefix HP_SetCursorRel_4(        /* SetCursorRel signature 4 */

	HP_StreamHandleType pStream,           /* stream handle */
    HP_Real32 xVal,                        /* absolute x location */
    HP_Real32 yVal)                        /* absolute y location */

;




HP_StdFuncPrefix HP_SetHalftoneMethod_1(   /* SetHalftoneMethod signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UByte ditherMatrixEnum)             /* dither matrix enumeration */
;




HP_StdFuncPrefix HP_SetHalftoneMethod_2(   /* SetHalftoneMethod signature 2 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UInt16 matrixSizeX,                 /* dither matrix x size */
    HP_UInt16 matrixSizeY,                 /* dither matrix y size */
    HP_UByte ditherMatrixDepth,            /* dither matrix depth */
    HP_UByte ditherMatrixDataType)         /* dither matrix data type */
;




HP_StdFuncPrefix HP_SetHalftoneMethod_3(   /* SetHalftoneMethod signature 3 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UInt16 matrixSizeX,                 /* dither matrix x size */
    HP_UInt16 matrixSizeY,                 /* dither matrix y size */
    HP_UByte ditherMatrixDepth,            /* dither matrix depth */
    HP_UInt16 xVal,                        /* dither matrix origin x */
    HP_UInt16 yVal,                        /* dither matrix origin x */
    HP_UByte ditherMatrixDataType)         /* dither matrix data type */
;




HP_StdFuncPrefix HP_SetFillMode_1(         /* SetFillMode signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UByte fillModeEnum)                 /* fill mode enumeration */
;




HP_StdFuncPrefix HP_SetFont_1(             /* SetFont signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_pUByte fontName,                    /* font name array */
	HP_UInt16 fontNameLen,                 /* font name array length */
    HP_Real32 charSize,                    /* char size in user units */
    HP_UInt16 symbolSetEnum)               /* symbol set enumeration */
;


HP_StdFuncPrefix HP_SetCharSubMode_1(   /* SetCharSubMode signature 1 */
    HP_StreamHandleType pStream,        /* stream handle */
    HP_pUByte pSubModeArray,            /* substitution modes array */
    HP_UByte subModeArrayLength)        /* substitution modes array length */
;

HP_StdFuncPrefix HP_SetLineCap_1(          /* SetLineCap signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UByte lineCapEnum)                  /* line cap enumeration */
;




HP_StdFuncPrefix HP_SetLineDash_1(         /* SetLineDash signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_pUInt16 array,                      /* line dash units array */
    HP_UInt16 arrayLen)                    /* line dash array length */
;




HP_StdFuncPrefix HP_SetLineDash_2(         /* SetLineDash signature 2 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_pUInt16 array,                      /* line dash units array */
    HP_UInt16 arrayLen,                    /* line dash array length */
    HP_UInt16 dashOffset)                  /* offset into line dash array */
;




HP_StdFuncPrefix HP_SetLineDash_Solid1(    /* SetLineDash signature Solid1 */

    HP_StreamHandleType pStream)           /* stream handle */
;




HP_StdFuncPrefix HP_SetLineJoin_1(         /* SetLineJoin signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UByte lineJoinEnum)                 /* line join enumeration */
;




HP_StdFuncPrefix HP_SetMiterLimit_1(       /* SetMiterLimit signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UInt16 miterLength)                 /* maximum miter shape length */
;



HP_StdFuncPrefix HP_SetMiterLimit_2(       /* SetMiterLimit signature 2 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_Real32 miterLength)                 /* maximum miter shape length */
;




HP_StdFuncPrefix HP_SetPageDefaultCTM_1(   /* SetPageDefault signature 1 */

    HP_StreamHandleType pStream)           /* stream handle */
;




HP_StdFuncPrefix HP_SetPageOrigin_1(       /* SetPageOrigin signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_SInt16 xVal,                        /* new origin x value */
    HP_SInt16 yVal)                        /* new origin y value */
;




HP_StdFuncPrefix HP_SetPageOrigin_2(       /* SetPageOrigin signature 2 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UInt16 xVal,                        /* new origin x value */
    HP_UInt16 yVal)                        /* new origin y value */
;

HP_StdFuncPrefix HP_SetPageOrigin_3(       /* SetPageOrigin signature 3 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_SInt32 xVal,                        /* new origin x value */
	HP_SInt32 yVal)                        /* new origin y value */
;

HP_StdFuncPrefix HP_SetPageOrigin_4(       /* SetPageOrigin signature 4 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_Real32 xVal,                        /* new origin x value */
	HP_Real32 yVal)                        /* new origin y value */
;

HP_StdFuncPrefix HP_SetPageRotation_1(     /* SetPageRotation signature 1 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_SInt16 pageAngle)                   /* page angle value */
;

HP_StdFuncPrefix HP_SetPageRotation_2(     /* SetPageRotation signature 2 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_Real32 pageAngle)                   /* page angle value */
;

HP_StdFuncPrefix HP_SetPageScale_1(        /* SetPageScale signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_Real32 xVal,                        /* x scale factor */
    HP_Real32 yVal)                        /* y scale factor */
;

HP_StdFuncPrefix HP_SetPathToClip_1(       /* SetPathToClip signature 1 */

	HP_StreamHandleType pStream)           /* stream handle */
;

HP_StdFuncPrefix HP_SetPaintTxMode_1(      /* SetPaintTxMode signature 1 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_UByte transparancyEnum)             /* transparency mode enumeration */
;

HP_StdFuncPrefix HP_SetPenSource_Gray1(    /* SetPenSource Gray sig. 1 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_Real32 grayLevel)                   /* gray level value (0-1.0) */
;

HP_StdFuncPrefix HP_SetPenSource_Null1(    /* SetPenSource Null sig. 1 */

	HP_StreamHandleType pStream)           /* stream handle */
;

HP_StdFuncPrefix HP_SetPenSource_Primary(  /* SetPenSource Primary */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_UByte colorSpaceEnum,               /* Primary Depth */
	HP_pUByte colorData,                   /* array for color data */
	HP_UInt16 colorDataLen)                /* length of color data array in bytes */
;

HP_StdFuncPrefix HP_SetPenSource_Patt1(    /* SetPenSource Pattern sig. 1 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_SInt16 patternSelectID)              /* pattern select identifier */
;

HP_StdFuncPrefix HP_SetPenSource_Patt2(    /* SetPenSource Pattern sig. 1 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_SInt16 xExtent,                     /* pattern dest x extent */
	HP_SInt16 yExtent,                     /* pattern dest y extent */
	HP_SInt16 patternSelectID)             /* pattern select identifier */
;

HP_StdFuncPrefix HP_SetPenSource_RGB1(     /* SetPenSource RGB sig. 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_Real32 redValue,                    /* red value */
    HP_Real32 greenValue,                  /* green value */
    HP_Real32 blueValue)                   /* blue value */
;


HP_StdFuncPrefix HP_SetPenWidth_1(         /* SetPenWidth signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UInt16 penWidth)                    /* pen width */
;



HP_StdFuncPrefix HP_SetPenWidth_2(         /* SetPenWidth signature 2 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_Real32 penWidth)                    /* pen width */
;




HP_StdFuncPrefix HP_SetROP_1(              /* SetROP signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UByte rop3)                         /* rop 3 value */
;




HP_StdFuncPrefix HP_SetSourceTxMode_1(     /* SetSourceTxMode signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_UByte transparancyEnum)             /* transparency mode enumeration */
;


HP_StdFuncPrefix HP_Text_1(                /* Text signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_pUByte charArray,                   /* character array containing text */
    HP_UInt32 charArrayLen,                /* length of character array */
    HP_pSInt16 xSpaceArray,                /* x escapement array */
    HP_UInt32 xSpaceArrayLen)              /* x escapement array length */
;

HP_StdFuncPrefix HP_Text_2(                /* Text signature 2 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_pUByte charArray,                   /* character array containing text */
    HP_UInt32 charArrayLen,                /* length of character array */
    HP_pSInt16 ySpaceArray,                /* y escapement array */
    HP_UInt32 ySpaceArrayLen)              /* y escapement array length */
;

HP_StdFuncPrefix HP_Text_3(                /* Text signature 3 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_pUByte charArray,                   /* character array containing text */
    HP_UInt32 charArrayLen,                /* length of character array */
    HP_pSInt16 xSpaceArray,                /* x escapement array */
    HP_UInt32 xSpaceArrayLen,              /* x escapement array length */
    HP_pSInt16 ySpaceArray,                /* y escapement array */
    HP_UInt32 ySpaceArrayLen)              /* y escapement array length */
;

HP_StdFuncPrefix HP_Text_4(                /* Text signature 4 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_pUInt16 charArray,                  /* character array containing text */
    HP_UInt32 charArrayLen,                /* length of character array */
    HP_pSInt16 xSpaceArray,                /* x escapement array */
    HP_UInt32 xSpaceArrayLen)              /* x escapement array length */
;

HP_StdFuncPrefix HP_Text_5(                /* Text signature 5 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_pUInt16 charArray,                  /* character array containing text */
    HP_UInt32 charArrayLen,                /* length of character array */
	HP_pSInt16 ySpaceArray,                /* y escapement array */
    HP_UInt32 ySpaceArrayLen)              /* y escapement array length */
;

HP_StdFuncPrefix HP_Text_6(                /* Text signature 6 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_pUInt16 charArray,                  /* character array containing text */
    HP_UInt32 charArrayLen,                /* length of character array */
    HP_pSInt16 xSpaceArray,                /* x escapement array */
    HP_UInt32 xSpaceArrayLen,              /* x escapement array length */
    HP_pSInt16 ySpaceArray,                /* y escapement array */
    HP_UInt32 ySpaceArrayLen)              /* y escapement array length */
;

HP_StdFuncPrefix HP_Text_7(                /* Text signature 7 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_pUByte charArray,                   /* character array containing text */
	HP_UInt32 charArrayLen)              /* y escapement array length */
;

HP_StdFuncPrefix HP_Text_8(                /* Text signature 8 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_pUInt16 charArray,                  /* character array containing text */
	HP_UInt32 charArrayLen)              /* x escapement array length */
;

HP_StdFuncPrefix HP_TextPath_1(            /* TextPath signature 1 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_pUByte charArray,                   /* character array containing text */
    HP_UInt32 charArrayLen,                /* length of character array */
    HP_pSInt16 xSpaceArray,                /* x escapement array */
    HP_UInt32 xSpaceArrayLen)              /* x escapement array length */
;

HP_StdFuncPrefix HP_TextPath_2(            /* TextPath signature 2 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_pUByte charArray,                   /* character array containing text */
    HP_UInt32 charArrayLen,                /* length of character array */
    HP_pSInt16 ySpaceArray,                /* x escapement array */
    HP_UInt32 ySpaceArrayLen)              /* x escapement array length */
;

HP_StdFuncPrefix HP_TextPath_3(            /* TextPath signature 3 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_pUByte charArray,                   /* character array containing text */
    HP_UInt32 charArrayLen,                /* length of character array */
    HP_pSInt16 xSpaceArray,                /* x escapement array */
    HP_UInt32 xSpaceArrayLen,              /* x escapement array length */
    HP_pSInt16 ySpaceArray,                /* y escapement array */
    HP_UInt32 ySpaceArrayLen)              /* y escapement array length */
;

HP_StdFuncPrefix HP_TextPath_4(            /* TextPath signature 4 */

    HP_StreamHandleType pStream,           /* stream handle */
	HP_pUInt16 charArray,                  /* character array containing text */
    HP_UInt32 charArrayLen,                /* length of character array */
    HP_pSInt16 xSpaceArray,                /* x escapement array */
    HP_UInt32 xSpaceArrayLen)              /* x escapement array length */
;

HP_StdFuncPrefix HP_TextPath_5(            /* TextPath signature 5 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_pUInt16 charArray,                  /* character array containing text */
    HP_UInt32 charArrayLen,                /* length of character array */
    HP_pSInt16 ySpaceArray,                /* x escapement array */
    HP_UInt32 ySpaceArrayLen)              /* x escapement array length */
;

HP_StdFuncPrefix HP_TextPath_6(            /* TextPath signature 6 */

    HP_StreamHandleType pStream,           /* stream handle */
    HP_pUInt16 charArray,                  /* character array containing text */
    HP_UInt32 charArrayLen,                /* length of character array */
    HP_pSInt16 xSpaceArray,                /* x escapement array */
    HP_UInt32 xSpaceArrayLen,              /* x escapement array length */
    HP_pSInt16 ySpaceArray,                /* y escapement array */
    HP_UInt32 ySpaceArrayLen)              /* y escapement array length */
;

HP_StdFuncPrefix HP_TextPath_7(            /* TextPath signature 7 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_pUByte charArray,                   /* character array containing text */
	HP_UInt32 charArrayLen)              /* y escapement array length */
;

HP_StdFuncPrefix HP_TextPath_8(            /* TextPath signature 8 */

	HP_StreamHandleType pStream,           /* stream handle */
	HP_pUInt16 charArray,                  /* character array containing text */
	HP_UInt32 charArrayLen)              /* x escapement array length */
;

HP_StdFuncPrefix HP_SetCharBoldValue_1(            /* SetCharBoldValue */

HP_StreamHandleType pStream,           /* stream handle */
HP_Real32 boldValue)
;
			   /* Bold Value Real */
#define hp_PAINT_SETCOLORTREATMENT  0x68701001
#define hp_PAINT_SETHALFTONEMETHOD  0x68701002
#define hp_PAINT_DOWNLOADCOLORTABLE 0x68701003
#define hp_MEDIA_SELECTMEDIASOURCE  0x68702001
#define hp_MEDIA_SELECTMEDIAFINISH  0x68702002
#define hp_MET						0x68703000

HP_StdFuncPrefix HP_media_SelectMediaFinish(/* SelectMediaFinish */

HP_StreamHandleType pStream,           /* stream handle */
HP_UByte mediaFinish)               /* color space enumeration */
;

HP_StdFuncPrefix HP_media_SelectMediaSource(/* SelectMediaSource */

HP_StreamHandleType pStream,           /* stream handle */
HP_UByte mediaSource,
HP_UByte mediaType)               /* color space enumeration */
;

HP_StdFuncPrefix HP_paint_SetColorTreatment (/* SetColorTreatment */

HP_StreamHandleType pStream,           /* stream handle */
HP_UByte colorTreatment,
HP_UInt16 colorID)               /* color space enumeration */
;

HP_StdFuncPrefix HP_paint_SetHalftoneMethod (/* SetHalftoneMethod */

HP_StreamHandleType pStream,           /* stream handle */
HP_UByte matrix,
HP_UInt16 matrixID)               /* color space enumeration */
;

HP_StdFuncPrefix HP_paint_DownloadColorTable (/* DownloadColorTable */

HP_StreamHandleType pStream,           /* stream handle */
HP_UByte tableType,
HP_UInt16 tableID,
HP_UInt32 tableSize,
HP_pUByte table)               /* color space enumeration */
;

typedef struct {
	HP_UInt32 Op;
	HP_UByte   Type;
	} HP_MET_OPERATORS;

;

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
HP_UInt16  Terms)
;
#endif

HP_StdFuncPrefix HP_M2TIFF_CalcCompression(
    HP_pCharHuge lpbytes,     /* Pointer to the buffer of bytes to Calculate. */
    HP_SInt32 len,            /* Length of the buffer of bytes. */
    HP_pUInt32 CompSize       /* Memory location to return amount of compression,
                                 this will be initialized to zero with each call */
   )
;

HP_StdFuncPrefix HP_M2TIFF_Compress(
    HP_StreamHandleType lpdv,  /* Pointer to the output buffer */
    HP_UInt32 outputLen,       /* Length of the output buffer in bytes. */
    HP_pCharHuge lpBits,       /* Pointer to the buffer of bytes to Compress */
    HP_SInt32 inputLen,        /* Length of the input buffer of bytes. */
    HP_pUInt32 CompSize        /* Memory location to return amount of compression,
                                  this will be initialized to zero with each call */

    )
;

#endif


