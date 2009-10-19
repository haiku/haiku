/* PDFlib GmbH cvsid: $Id: port.h 28305 2008-10-23 21:46:26Z bonefish $ */

#ifndef TIFF_PORT_H
#define TIFF_PORT_H 1

/* not used: PDFlib GmbH:
#define HOST_FILLORDER FILLORDER_LSB2MSB
*/
#define HOST_BIGENDIAN	1

#include <math.h>	/* PDFlib GmbH: */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* PDFlib GmbH: */
#if !defined(_WIN32_WCE)
#if defined(WIN32) || defined(OS2)
#include <fcntl.h>
#include <sys/types.h>
#else
#include <fcntl.h>      /* TODO: fix me */
#endif
#endif /* _WIN32_CE */

typedef unsigned char tif_char;
typedef unsigned short tif_short;
typedef unsigned int tif_int;
typedef unsigned long tif_long;


/* PDFlib GmbH: */
/*
 * This maze of checks controls defines or not the
 * target system has BSD-style typdedefs declared in
 * an include file and/or whether or not to include
 * <unistd.h> to get the SEEK_* definitions.  Some
 * additional includes are also done to pull in the
 * appropriate definitions we're looking for.
 */
#if (defined macintosh || defined __POWERPC__ || \
	defined __CFM68K__ || defined __MC68K__) && \
	!defined MAC && !(defined(__BEOS__) || defined(__HAIKU__))
#  define MAC
#endif

#if defined(WIN32) || defined(OS2)
#  define BSDTYPES
#elif defined(MAC)
#  define BSDTYPES
#  ifndef HAVE_UNISTD_H		/* PDFlib GmbH: avoid warning on OS X */
#  define HAVE_UNISTD_H   0
#  endif
#else
#  include <unistd.h>
#endif


typedef double dblparam_t;

#undef INLINE           /* PDFlib GmbH */
#define INLINE  /* */

#define GLOBALDATA(TYPE,NAME)	extern TYPE NAME

/* to allow the use of PDFlib inside of programs using TIFFlib themselves */
/* Make sure to observe the limit of 31 characters for function names! */
#define LogL10fromY		pdf_LogL10fromY
#define LogL10toY		pdf_LogL10toY
#define LogL16fromY		pdf_LogL16fromY
#define LogL16toY		pdf_LogL16toY
#define LogLuv24fromXYZ		pdf_LogLuv24fromXYZ
#define LogLuv24toXYZ		pdf_LogLuv24toXYZ
#define LogLuv32fromXYZ		pdf_LogLuv32fromXYZ
#define LogLuv32toXYZ		pdf_LogLuv32toXYZ
#define TIFFCheckTile		pdf_TIFFCheckTile
#define TIFFClientOpen		pdf_TIFFClientOpen
#define TIFFClose		pdf_TIFFClose
#define TIFFComputeStrip		pdf_TIFFComputeStrip
#define TIFFComputeTile		pdf_TIFFComputeTile
#define TIFFCreateDirectory		pdf_TIFFCreateDirectory
#define TIFFCurrentDirectory		pdf_TIFFCurrentDirectory
#define TIFFCurrentRow		pdf_TIFFCurrentRow
#define TIFFCurrentStrip		pdf_TIFFCurrentStrip
#define TIFFCurrentTile		pdf_TIFFCurrentTile
#define TIFFDefaultDirectory		pdf_TIFFDefaultDirectory
#define TIFFDefaultStripSize		pdf_TIFFDefaultStripSize
#define TIFFDefaultTileSize		pdf_TIFFDefaultTileSize
#define TIFFError		pdf_TIFFError
#define TIFFFaxBlackCodes		pdf_TIFFFaxBlackCodes
#define TIFFFaxBlackTable		pdf_TIFFFaxBlackTable
#define TIFFFaxMainTable		pdf_TIFFFaxMainTable
#define TIFFFaxWhiteCodes		pdf_TIFFFaxWhiteCodes
#define TIFFFaxWhiteTable		pdf_TIFFFaxWhiteTable
#define TIFFFdOpen		pdf_TIFFFdOpen
#define TIFFFileName		pdf_TIFFFileName
#define TIFFFindCODEC		pdf_TIFFFindCODEC
#define TIFFFlush		pdf_TIFFFlush
#define TIFFFlushData		pdf_TIFFFlushData
#define TIFFFlushData1		pdf_TIFFFlushData1
#define TIFFFreeDirectory		pdf_TIFFFreeDirectory
#define TIFFGetBitRevTable		pdf_TIFFGetBitRevTable
#define TIFFGetField		pdf_TIFFGetField
#define TIFFGetFieldDefaulted		pdf_TIFFGetFieldDefaulted
#define TIFFGetMode		pdf_TIFFGetMode
#define TIFFGetVersion		pdf_TIFFGetVersion
#define TIFFInitCCITTFax3		pdf_TIFFInitCCITTFax3
#define TIFFInitCCITTFax4		pdf_TIFFInitCCITTFax4
#define TIFFInitCCITTRLE		pdf_TIFFInitCCITTRLE
#define TIFFInitCCITTRLEW		pdf_TIFFInitCCITTRLEW
#define TIFFInitDumpMode		pdf_TIFFInitDumpMode
#define TIFFInitLZW		pdf_TIFFInitLZW
#define TIFFInitNeXT		pdf_TIFFInitNeXT
#define TIFFInitPackBits		pdf_TIFFInitPackBits
#define TIFFInitSGILog		pdf_TIFFInitSGILog
#define TIFFInitZIP		pdf_TIFFInitZIP
#define TIFFIsByteSwapped		pdf_TIFFIsByteSwapped
#define TIFFIsMSB2LSB		pdf_TIFFIsMSB2LSB
#define TIFFIsTiled		pdf_TIFFIsTiled
#define TIFFIsUpSampled		pdf_TIFFIsUpSampled
#define TIFFNumberOfStrips		pdf_TIFFNumberOfStrips
#define TIFFNumberOfTiles		pdf_TIFFNumberOfTiles
#define TIFFOpen		pdf_TIFFOpen
#define TIFFPredictorInit		pdf_TIFFPredictorInit
#define TIFFPrintDirectory		pdf_TIFFPrintDirectory
#define TIFFRGBAImageBegin		pdf_TIFFRGBAImageBegin
#define TIFFRGBAImageEnd		pdf_TIFFRGBAImageEnd
#define TIFFRGBAImageGet		pdf_TIFFRGBAImageGet
#define TIFFRGBAImageOK		pdf_TIFFRGBAImageOK
#define TIFFRasterScanlineSize		pdf_TIFFRasterScanlineSize
#define TIFFReadBufferSetup		pdf_TIFFReadBufferSetup
#define TIFFReadDirectory		pdf_TIFFReadDirectory
#define TIFFReadEncodedStrip		pdf_TIFFReadEncodedStrip
#define TIFFReadEncodedTile		pdf_TIFFReadEncodedTile
#define TIFFReadRGBAImage		pdf_TIFFReadRGBAImage
#define TIFFReadRGBAStrip		pdf_TIFFReadRGBAStrip
#define TIFFReadRGBATile		pdf_TIFFReadRGBATile
#define TIFFReadRawStrip		pdf_TIFFReadRawStrip
#define TIFFReadRawTile		pdf_TIFFReadRawTile
#define TIFFReadScanline		pdf_TIFFReadScanline
#define TIFFReadTile		pdf_TIFFReadTile
#define TIFFReassignTagToIgnore		pdf_TIFFReassignTagToIgnore
#define TIFFReverseBits		pdf_TIFFReverseBits
#define TIFFRewriteDirectory		pdf_TIFFRewriteDirectory
#define TIFFScanlineSize		pdf_TIFFScanlineSize
#define TIFFSetCompressionScheme		pdf_TIFFSetCompressionScheme
#define TIFFSetDirectory		pdf_TIFFSetDirectory
#define TIFFSetErrorHandler		pdf_TIFFSetErrorHandler
#define TIFFSetField		pdf_TIFFSetField
#define TIFFSetTagExtender		pdf_TIFFSetTagExtender
#define TIFFSetWarningHandler		pdf_TIFFSetWarningHandler
#define TIFFSetWriteOffset		pdf_TIFFSetWriteOffset
#define TIFFStripSize		pdf_TIFFStripSize
#define TIFFSwabArrayOfDouble		pdf_TIFFSwabArrayOfDouble
#define TIFFSwabArrayOfLong		pdf_TIFFSwabArrayOfLong
#define TIFFSwabArrayOfShort		pdf_TIFFSwabArrayOfShort
#define TIFFSwabDouble		pdf_TIFFSwabDouble
#define TIFFSwabLong		pdf_TIFFSwabLong
#define TIFFSwabShort		pdf_TIFFSwabShort
#define TIFFTileRowSize		pdf_TIFFTileRowSize
#define TIFFTileSize		pdf_TIFFTileSize
#define TIFFVGetField		pdf_TIFFVGetField
#define TIFFVGetFieldDefaulted		pdf_TIFFVGetFieldDefaulted
#define TIFFVSetField		pdf_TIFFVSetField
#define TIFFVStripSize		pdf_TIFFVStripSize
#define TIFFVTileSize		pdf_TIFFVTileSize
#define TIFFWarning		pdf_TIFFWarning
#define TIFFWriteBufferSetup		pdf_TIFFWriteBufferSetup
#define TIFFWriteCheck		pdf_TIFFWriteCheck
#define TIFFWriteDirectory		pdf_TIFFWriteDirectory
#define TIFFWriteEncodedStrip		pdf_TIFFWriteEncodedStrip
#define TIFFWriteEncodedTile		pdf_TIFFWriteEncodedTile
#define TIFFWriteRawStrip		pdf_TIFFWriteRawStrip
#define TIFFWriteRawTile		pdf_TIFFWriteRawTile
#define TIFFWriteScanline		pdf_TIFFWriteScanline
#define TIFFWriteTile		pdf_TIFFWriteTile
#define XYZtoRGB24		pdf_XYZtoRGB24
#define _TIFFBuiltinCODECS		pdf__TIFFBuiltinCODECS
#define _TIFFDefaultStripSize		pdf__TIFFDefaultStripSize
#define _TIFFDefaultTileSize		pdf__TIFFDefaultTileSize
#define _TIFFFax3fillruns		pdf__TIFFFax3fillruns
#define _TIFFFieldWithTag		pdf__TIFFFieldWithTag
#define _TIFFFindFieldInfo		pdf__TIFFFindFieldInfo
#define _TIFFMergeFieldInfo		pdf__TIFFMergeFieldInfo
#define _TIFFNoPostDecode		pdf__TIFFNoPostDecode
#define _TIFFNoPreCode		pdf__TIFFNoPreCode
#define _TIFFNoRowDecode		pdf__TIFFNoRowDecode
#define _TIFFNoRowEncode		pdf__TIFFNoRowEncode
#define _TIFFNoSeek		pdf__TIFFNoSeek
#define _TIFFNoStripDecode		pdf__TIFFNoStripDecode
#define _TIFFNoStripEncode		pdf__TIFFNoStripEncode
#define _TIFFNoTileDecode		pdf__TIFFNoTileDecode
#define _TIFFNoTileEncode		pdf__TIFFNoTileEncode
#define _TIFFPrintFieldInfo		pdf__TIFFPrintFieldInfo
#define _TIFFSampleToTagType		pdf__TIFFSampleToTagType

/* Note: function name shortened to facilitate porting */
#define _TIFFSetDefaultCompressionState	pdf__TIFFSetDefaultCompState

#define _TIFFSetupFieldInfo		pdf__TIFFSetupFieldInfo
#define _TIFFSwab16BitData		pdf__TIFFSwab16BitData
#define _TIFFSwab32BitData		pdf__TIFFSwab32BitData
#define _TIFFSwab64BitData		pdf__TIFFSwab64BitData
#define _TIFFerrorHandler		pdf__TIFFerrorHandler
#define _TIFFfree		pdf__TIFFfree
#define _TIFFgetMode		pdf__TIFFgetMode
#define _TIFFmalloc		pdf__TIFFmalloc
#define _TIFFmemcmp		pdf__TIFFmemcmp
#define _TIFFmemcpy		pdf__TIFFmemcpy
#define _TIFFmemset		pdf__TIFFmemset
#define _TIFFprintAscii		pdf__TIFFprintAscii
#define _TIFFprintAsciiTag		pdf__TIFFprintAsciiTag
#define _TIFFrealloc		pdf__TIFFrealloc
#define _TIFFsetByteArray		pdf__TIFFsetByteArray
#define _TIFFsetDoubleArray		pdf__TIFFsetDoubleArray
#define _TIFFsetFloatArray		pdf__TIFFsetFloatArray
#define _TIFFsetLongArray		pdf__TIFFsetLongArray
#define _TIFFsetNString		pdf__TIFFsetNString
#define _TIFFsetShortArray		pdf__TIFFsetShortArray
#define _TIFFsetString		pdf__TIFFsetString
#define _TIFFwarningHandler		pdf__TIFFwarningHandler
#define tiffDataWidth		pdf_tiffDataWidth
#define uv_decode		pdf_uv_decode
#define uv_encode		pdf_uv_encode

#endif
