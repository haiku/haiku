//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		TypeConstants.h
//	Author:			Erik Jaesler (erik@cgsoftware.com)
//	Description:	Constants that represent distinct data types, as used
//					by BMessage et. al.
//------------------------------------------------------------------------------

#ifndef _TYPE_CONSTANTS_H
#define _TYPE_CONSTANTS_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <BeBuild.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------


// Data Types ------------------------------------------------------------------

enum {
	B_ANY_TYPE 					= 'ANYT',
	B_BOOL_TYPE 				= 'BOOL',
	B_CHAR_TYPE 				= 'CHAR',
	B_COLOR_8_BIT_TYPE 			= 'CLRB',
	B_DOUBLE_TYPE 				= 'DBLE',
	B_FLOAT_TYPE 				= 'FLOT',
	B_GRAYSCALE_8_BIT_TYPE		= 'GRYB',
	B_INT64_TYPE 				= 'LLNG',
	B_INT32_TYPE 				= 'LONG',
	B_INT16_TYPE 				= 'SHRT',
	B_INT8_TYPE 				= 'BYTE',
	B_MESSAGE_TYPE				= 'MSGG',
	B_MESSENGER_TYPE			= 'MSNG',
	B_MIME_TYPE					= 'MIME',
	B_MONOCHROME_1_BIT_TYPE 	= 'MNOB',
	B_OBJECT_TYPE 				= 'OPTR',
	B_OFF_T_TYPE 				= 'OFFT',
	B_PATTERN_TYPE 				= 'PATN',
	B_POINTER_TYPE 				= 'PNTR',
	B_POINT_TYPE 				= 'BPNT',
	B_RAW_TYPE 					= 'RAWT',
	B_RECT_TYPE 				= 'RECT',
	B_REF_TYPE 					= 'RREF',
	B_RGB_32_BIT_TYPE 			= 'RGBB',
	B_RGB_COLOR_TYPE 			= 'RGBC',
	B_SIZE_T_TYPE	 			= 'SIZT',
	B_SSIZE_T_TYPE	 			= 'SSZT',
	B_STRING_TYPE 				= 'CSTR',
	B_TIME_TYPE 				= 'TIME',
	B_UINT64_TYPE				= 'ULLG',
	B_UINT32_TYPE				= 'ULNG',
	B_UINT16_TYPE 				= 'USHT',
	B_UINT8_TYPE 				= 'UBYT',
	B_MEDIA_PARAMETER_TYPE		= 'BMCT',
	B_MEDIA_PARAMETER_WEB_TYPE	= 'BMCW',
	B_MEDIA_PARAMETER_GROUP_TYPE= 'BMCG',

	// deprecated, do not use
	B_ASCII_TYPE 				= 'TEXT'	// use B_STRING_TYPE instead
};

//----- System-wide MIME types for handling URL's ------------------------------

	// To register your application as a handler for a specific URL type,
	// mark it as a handler of the corresponding MIME type from the list
	// below.  When the user clicks on a link in NetPositive that your
	// application is a handler for, you will get a B_ARGV_RECEIVED message
	// with the	full URL as the second argument.
extern _IMPEXP_BE const char *B_URL_HTTP; 				// application/x-vnd.Be.URL.http
extern _IMPEXP_BE const char *B_URL_HTTPS; 				// application/x-vnd.Be.URL.https
extern _IMPEXP_BE const char *B_URL_FTP;				// application/x-vnd.Be.URL.ftp
extern _IMPEXP_BE const char *B_URL_GOPHER; 			// application/x-vnd.Be.URL.gopher
extern _IMPEXP_BE const char *B_URL_MAILTO; 			// application/x-vnd.Be.URL.mailto
extern _IMPEXP_BE const char *B_URL_NEWS;				// application/x-vnd.Be.URL.news
extern _IMPEXP_BE const char *B_URL_NNTP;				// application/x-vnd.Be.URL.nntp
extern _IMPEXP_BE const char *B_URL_TELNET; 			// application/x-vnd.Be.URL.telnet
extern _IMPEXP_BE const char *B_URL_RLOGIN; 			// application/x-vnd.Be.URL.rlogin
extern _IMPEXP_BE const char *B_URL_TN3270; 			// application/x-vnd.Be.URL.tn3270
extern _IMPEXP_BE const char *B_URL_WAIS;				// application/x-vnd.Be.URL.wais
extern _IMPEXP_BE const char *B_URL_FILE;				// application/x-vnd.Be.URL.file

//----- Obsolete; do not use ---------------------------------------------------

enum {
	_DEPRECATED_TYPE_1_			= 'PATH'
};

//------------------------------------------------------------------------------

#endif	// _TYPE_CONSTANTS_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

