/*
 * Copyright 2005-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Erik Jaesler (erik@cgsoftware.com)
 */
#ifndef _FSSH_TYPE_CONSTANTS_H
#define _FSSH_TYPE_CONSTANTS_H


#include "fssh_defs.h"


enum {
	FSSH_B_ANY_TYPE						= 'ANYT',
	FSSH_B_ATOM_TYPE					= 'ATOM',
	FSSH_B_ATOMREF_TYPE					= 'ATMR',
	FSSH_B_BOOL_TYPE					= 'BOOL',
	FSSH_B_CHAR_TYPE					= 'CHAR',
	FSSH_B_COLOR_8_BIT_TYPE				= 'CLRB',
	FSSH_B_DOUBLE_TYPE					= 'DBLE',
	FSSH_B_FLOAT_TYPE					= 'FLOT',
	FSSH_B_GRAYSCALE_8_BIT_TYPE			= 'GRYB',
	FSSH_B_INT16_TYPE					= 'SHRT',
	FSSH_B_INT32_TYPE					= 'LONG',
	FSSH_B_INT64_TYPE					= 'LLNG',
	FSSH_B_INT8_TYPE					= 'BYTE',
	FSSH_B_LARGE_ICON_TYPE				= 'ICON',
	FSSH_B_MEDIA_PARAMETER_GROUP_TYPE	= 'BMCG',
	FSSH_B_MEDIA_PARAMETER_TYPE			= 'BMCT',
	FSSH_B_MEDIA_PARAMETER_WEB_TYPE		= 'BMCW',
	FSSH_B_MESSAGE_TYPE					= 'MSGG',
	FSSH_B_MESSENGER_TYPE				= 'MSNG',
	FSSH_B_MIME_TYPE					= 'MIME',
	FSSH_B_MINI_ICON_TYPE				= 'MICN',
	FSSH_B_MONOCHROME_1_BIT_TYPE		= 'MNOB',
	FSSH_B_OBJECT_TYPE					= 'OPTR',
	FSSH_B_OFF_T_TYPE					= 'OFFT',
	FSSH_B_PATTERN_TYPE					= 'PATN',
	FSSH_B_POINTER_TYPE					= 'PNTR',
	FSSH_B_POINT_TYPE					= 'BPNT',
	FSSH_B_PROPERTY_INFO_TYPE			= 'SCTD',
	FSSH_B_RAW_TYPE						= 'RAWT',
	FSSH_B_RECT_TYPE					= 'RECT',
	FSSH_B_REF_TYPE						= 'RREF',
	FSSH_B_RGB_32_BIT_TYPE				= 'RGBB',
	FSSH_B_RGB_COLOR_TYPE				= 'RGBC',
	FSSH_B_SIZE_T_TYPE					= 'SIZT',
	FSSH_B_SSIZE_T_TYPE					= 'SSZT',
	FSSH_B_STRING_TYPE					= 'CSTR',
	FSSH_B_TIME_TYPE					= 'TIME',
	FSSH_B_UINT16_TYPE					= 'USHT',
	FSSH_B_UINT32_TYPE					= 'ULNG',
	FSSH_B_UINT64_TYPE					= 'ULLG',
	FSSH_B_UINT8_TYPE					= 'UBYT',
	FSSH_B_VECTOR_ICON_TYPE				= 'VICN',
	FSSH_B_XATTR_TYPE					= 'XATR',
	FSSH_B_NETWORK_ADDRESS_TYPE			= 'NWAD',
	FSSH_B_MIME_STRING_TYPE				= 'MIMS',

	// deprecated, do not use
	FSSH_B_ASCII_TYPE					= 'TEXT'	// use B_STRING_TYPE instead
};

//----- System-wide MIME types for handling URL's ------------------------------

extern const char *FSSH_B_URL_HTTP; 	// application/x-vnd.Be.URL.http
extern const char *FSSH_B_URL_HTTPS; 	// application/x-vnd.Be.URL.https
extern const char *FSSH_B_URL_FTP;		// application/x-vnd.Be.URL.ftp
extern const char *FSSH_B_URL_GOPHER; 	// application/x-vnd.Be.URL.gopher
extern const char *FSSH_B_URL_MAILTO; 	// application/x-vnd.Be.URL.mailto
extern const char *FSSH_B_URL_NEWS;		// application/x-vnd.Be.URL.news
extern const char *FSSH_B_URL_NNTP;		// application/x-vnd.Be.URL.nntp
extern const char *FSSH_B_URL_TELNET; 	// application/x-vnd.Be.URL.telnet
extern const char *FSSH_B_URL_RLOGIN; 	// application/x-vnd.Be.URL.rlogin
extern const char *FSSH_B_URL_TN3270; 	// application/x-vnd.Be.URL.tn3270
extern const char *FSSH_B_URL_WAIS;		// application/x-vnd.Be.URL.wais
extern const char *FSSH_B_URL_FILE;		// application/x-vnd.Be.URL.file

#endif	// _FSSH_TYPE_CONSTANTS_H
