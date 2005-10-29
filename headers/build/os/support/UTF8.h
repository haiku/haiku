/******************************************************************************
/
/	File:			UTF8.h
/
/	Description:	UTF-8 conversion functions.
/
/	Copyright 1993-98, Be Incorporated
/
******************************************************************************/


#ifndef _UTF8_H
#define _UTF8_H

#include <BeBuild.h>
#include <InterfaceDefs.h>
#include <SupportDefs.h>

/*------------------------------------------------------------*/
/*------- Conversion Flavors ---------------------------------*/
enum {
	B_ISO1_CONVERSION,				/* ISO 8859-1 */
	B_ISO2_CONVERSION,				/* ISO 8859-2 */
	B_ISO3_CONVERSION,				/* ISO 8859-3 */
	B_ISO4_CONVERSION,				/* ISO 8859-4 */
	B_ISO5_CONVERSION,				/* ISO 8859-5 */
	B_ISO6_CONVERSION,				/* ISO 8859-6 */
	B_ISO7_CONVERSION,				/* ISO 8859-7 */
	B_ISO8_CONVERSION,				/* ISO 8859-8 */
	B_ISO9_CONVERSION,				/* ISO 8859-9 */
	B_ISO10_CONVERSION,				/* ISO 8859-10 */
	B_MAC_ROMAN_CONVERSION,			/* Macintosh Roman */
	B_SJIS_CONVERSION,				/* Shift-JIS */
	B_EUC_CONVERSION,				/* EUC Packed Japanese */
	B_JIS_CONVERSION,				/* JIS X 0208-1990 */
	B_MS_WINDOWS_CONVERSION,		/* MS-Windows Codepage 1252 */
	B_UNICODE_CONVERSION,			/* Unicode 2.0 */
	B_KOI8R_CONVERSION,				/* KOI8-R */
	B_MS_WINDOWS_1251_CONVERSION,	/* MS-Windows Codepage 1251 */
	B_MS_DOS_866_CONVERSION,		/* MS-DOS Codepage 866 */
	B_MS_DOS_CONVERSION,			/* MS-DOS Codepage 437 */
	B_EUC_KR_CONVERSION,				/* EUC Korean */
	B_ISO13_CONVERSION,				/* ISO 8859-13 */
	B_ISO14_CONVERSION,				/* ISO 8859-14 */
	B_ISO15_CONVERSION,				/* ISO 8859-15 */
};

/*-------------------------------------------------------------*/
/*------- Conversion Functions --------------------------------*/

_IMPEXP_TEXTENCODING status_t convert_to_utf8(uint32		srcEncoding, 
						 					  const char	*src, 
						 					  int32			*srcLen, 
											  char			*dst, 
						 					  int32			*dstLen,
											  int32			*state,
											  char			substitute = B_SUBSTITUTE);

_IMPEXP_TEXTENCODING status_t convert_from_utf8(uint32		dstEncoding,
												const char	*src, 
												int32		*srcLen, 
						  						char		*dst, 
						   						int32		*dstLen,
												int32		*state,
												char		substitute = B_SUBSTITUTE);

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

#endif	/* _UTF8_H */
