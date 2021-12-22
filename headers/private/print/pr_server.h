/*****************************************************************************/
// print_server Background Application.
//
// Version: 1.0.0d1
//
// The print_server manages the communication between applications and the
// printer and transport drivers.
//
//
// This application and all source files used in its construction, except 
// where noted, are licensed under the MIT License, and have been written 
// and are:
//
// Copyright (c) 2001 Haiku Project
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included 
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/

#ifndef _PR_SERVER_H_
#define _PR_SERVER_H_

// signature mime types
#define PSRV_SIGNATURE_TYPE			"application/x-vnd.Be-PSRV"
#define PRNT_SIGNATURE_TYPE			"application/x-vnd.Be-PRNT"

// messages understood by print_server
#define PSRV_GET_ACTIVE_PRINTER				'pgcp'
#define PSRV_MAKE_PRINTER_ACTIVE_QUIETLY	'pmaq'
#define PSRV_MAKE_PRINTER_ACTIVE			'pmac'
#define PSRV_MAKE_PRINTER					'selp'
#define PSRV_SHOW_PAGE_SETUP				'pgst'
#define PSRV_SHOW_PRINT_SETUP				'ppst'
#define PSRV_PRINT_SPOOLED_JOB				'psns'
#define PSRV_GET_DEFAULT_SETTINGS      'pdef'

// messages sent to Printers preflet
#define PRINTERS_ADD_PRINTER				'addp'

// mime file types
#define PSRV_PRINTER_FILETYPE		"application/x-vnd.Be.printer"
#define PSRV_SPOOL_FILETYPE			"application/x-vnd.Be.printer-spool"

// spool Attributes 
#define PSRV_SPOOL_ATTR_MIMETYPE	"_spool/MimeType"
#define PSRV_SPOOL_ATTR_PAGECOUNT	"_spool/Page Count"
#define PSRV_SPOOL_ATTR_DESCRIPTION	"_spool/Description"
#define PSRV_SPOOL_ATTR_PRINTER		"_spool/Printer"
#define PSRV_SPOOL_ATTR_STATUS		"_spool/Status"
#define PSRV_SPOOL_ATTR_ERRCODE		"_spool/_errorcode"

// spool file status attribute values
#define PSRV_JOB_STATUS_                ""
#define PSRV_JOB_STATUS_WAITING         "Waiting"
#define PSRV_JOB_STATUS_PROCESSING      "Processing"
#define PSRV_JOB_STATUS_FAILED          "Failed"
#define PSRV_JOB_STATUS_COMPLETED 		"Completed"

// printer attributes
#define PSRV_PRINTER_ATTR_DRV_NAME			"Driver Name"
#define PSRV_PRINTER_ATTR_PRT_NAME			"Printer Name"
#define PSRV_PRINTER_ATTR_COMMENTS			"Comments"
#define PSRV_PRINTER_ATTR_STATE				"state"
#define PSRV_PRINTER_ATTR_TRANSPORT			"transport"
#define PSRV_PRINTER_ATTR_TRANSPORT_ADDR	"transport_address"
#define PSRV_PRINTER_ATTR_CNX				"connection"
#define PSRV_PRINTER_ATTR_PNP				"_PNP"
#define PSRV_PRINTER_ATTR_MDL				"_MDL"

// printer name
#define PSRV_FIELD_CURRENT_PRINTER "current_printer"

// page settings fields
#define PSRV_FIELD_XRES "xres"
#define PSRV_FIELD_YRES "yres"
#define PSRV_FIELD_PAPER_RECT "paper_rect"
#define PSRV_FIELD_PRINTABLE_RECT "printable_rect"

// optional page settings field
#define PSRV_FIELD_ORIENTATION "orientation"
#define PSRV_FIELD_COPIES "copies"
#define PSRV_FIELD_SCALE "scale"
#define PSRV_FIELD_QUALITY "quality"

// job settings fields
#define PSRV_FIELD_FIRST_PAGE "first_page"
#define PSRV_FIELD_LAST_PAGE "last_page"

#endif
