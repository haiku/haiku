/*---------------------------------------------------------------------------*
 |              PDFlib - A library for generating PDF on the fly             |
 +---------------------------------------------------------------------------+
 | Copyright (c) 1997-2004 Thomas Merz and PDFlib GmbH. All rights reserved. |
 +---------------------------------------------------------------------------+
 |                                                                           |
 |    This software is subject to the PDFlib license. It is NOT in the       |
 |    public domain. Extended versions and commercial licenses are           |
 |    available, please check http://www.pdflib.com.                         |
 |                                                                           |
 *---------------------------------------------------------------------------*/

/* $Id: pc_generr.h 14574 2005-10-29 16:27:43Z bonefish $
 *
 * PDCORE error messages
 *
 */

#if	pdc_genNames
#define gen(n, num, nam, msg)	PDC_E_##nam = num,
#elif	pdc_genInfo
#define gen(n, num, nam, msg)	{ n, num, msg, (const char *) 0 },

#else
#error	invalid inclusion of generator file
#endif

/* -------------------------------------------------------------------- */
/* Configuration, memory, and I/O			(10xx)		*/
/* -------------------------------------------------------------------- */

gen(1, 1000, MEM_OUT,	"Out of memory in function $1")

gen(1, 1008, IO_ILLFILENAME, "Bad file name '$1'")

gen(2, 1010, IO_RDOPEN,	"Couldn't open $1file '$2' for reading")

gen(2, 1012, IO_WROPEN,	"Couldn't open $1file '$2' for writing")

gen(0, 1014, IO_NOWRITE, "Couldn't write output")

gen(2, 1016, IO_RDOPEN_NF,
    "Couldn't open $1file '$2' for reading (file not found)")

gen(2, 1018, IO_WROPEN_NF,
    "Couldn't open $1file '$2' for writing (no such directory)")

gen(2, 1020, IO_RDOPEN_PD,
    "Couldn't open $1file '$2' for reading (permission denied)")

gen(2, 1022, IO_WROPEN_PD,
    "Couldn't open $1file '$2' for writing (permission denied)")

gen(2, 1024, IO_RDOPEN_TM,
    "Couldn't open $1file '$2' for reading (too many open files)")

gen(2, 1026, IO_WROPEN_TM,
    "Couldn't open $1file '$2' for writing (too many open files)")

gen(2, 1028, IO_RDOPEN_ID,
    "Couldn't open $1file '$2' for reading (is a directory)")

gen(2, 1030, IO_WROPEN_ID,
    "Couldn't open $1file '$2' for writing (is a directory)")

gen(2, 1032, IO_WROPEN_AE,
    "Couldn't open $1file '$2' for writing (file already exists)")

gen(2, 1034, IO_WROPEN_TL,
    "Couldn't open $1file '$2' for writing (file name too long)")

gen(2, 1036, IO_WROPEN_NS,
    "Couldn't open $1file '$2' for writing (no space left on device)")

gen(2, 1038, IO_WROPEN_IS,
    "Couldn't open $1file '$2' for writing (file name syntax incorrect)")

gen(2, 1040, IO_WROPEN_NC,
    "Couldn't open $1file '$2' for writing (file cannot be created)")

gen(2, 1042, IO_WROPEN_NP,
    "Couldn't open $1file '$2' for writing (path not found)")

gen(1, 1050, IO_COMPRESS, "Compression error ($1)")

gen(0, 1052, IO_NOBUFFER, "Don't fetch buffer contents when writing to file")

gen(2, 1054, IO_BADFORMAT, "'$1' does not appear to be a $2 file")

gen(1, 1056, IO_NODATA, "Couldn't read data from file '$1'")

gen(3, 1058, IO_ILLSYNTAX, "$1file '$2': Syntax error in line $3")

gen(1, 1060, PVF_NAMEEXISTS,
    "Couldn't create virtual file '$1' (name already exists)")


/* -------------------------------------------------------------------- */
/* Invalid arguments                                        (11xx)	*/
/* -------------------------------------------------------------------- */

gen(1, 1100, ILLARG_EMPTY, "Parameter '$1' is empty")

gen(1, 1102, ILLARG_POSITIVE, "Parameter '$1' must be positive")

gen(2, 1104, ILLARG_BOOL, "Boolean parameter '$1' has bad value '$2'")

gen(2, 1106, ILLARG_INT, "Integer parameter '$1' has bad value $2")

gen(2, 1108, ILLARG_FLOAT, "Floating-point parameter '$1' has bad value $2")

gen(2, 1110, ILLARG_STRING, "String parameter '$1' has bad value '$2'")

/* Unused. See 1504
gen(1, 1112, ILLARG_UTF,  "Illegal UTF-$1 sequence in string")
 */

gen(2, 1114, ILLARG_MATRIX, "Matrix [$1] is degenerate")

gen(2, 1116, ILLARG_TOOLONG,
    "String parameter '$1' is limited to max. $2 characters'")

gen(2, 1118, ILLARG_HANDLE, "Handle parameter '$1' has bad value $2")


/* -------------------------------------------------------------------- */
/* Parameters and values                                    (12xx)	*/
/* -------------------------------------------------------------------- */

gen(0, 1200, PAR_EMPTYKEY, "Empty key")

gen(1, 1202, PAR_UNKNOWNKEY, "Unknown key '$1'")

gen(0, 1204, PAR_EMPTYVALUE, "Empty parameter value")

gen(2, 1206, PAR_ILLPARAM, "Bad parameter '$1' for key '$2'")

gen(2, 1208, PAR_ILLVALUE, "Bad value $1 for key '$2'")

gen(2, 1210, PAR_SCOPE_GET, "Can't get parameter '$1' in scope '$2'")

gen(2, 1212, PAR_SCOPE_SET, "Can't set parameter '$1' in scope '$2'")

gen(2, 1214, PAR_VERSION, "Parameter '$1' requires PDF $2 or above")




/* -------------------------------------------------------------------- */
/* Options and values                                       (14xx)      */
/* -------------------------------------------------------------------- */

gen(1, 1400, OPT_UNKNOWNKEY, "Unknown option '$1'")

gen(2, 1402, OPT_TOOFEWVALUES, "Option '$1' has too few values (< $2)")

gen(2, 1404, OPT_TOOMANYVALUES, "Option '$1' has too many values (> $2)")

gen(1, 1406, OPT_NOVALUES, "Option '$1' has no value(s)")

gen(2, 1408, OPT_ILLBOOLEAN, "Option '$1' has bad boolean value '$2'")

gen(2, 1410, OPT_ILLINTEGER, "Option '$1' has bad integer value '$2'")

gen(2, 1412, OPT_ILLNUMBER, "Option '$1' has bad number value '$2'")

gen(2, 1414, OPT_ILLKEYWORD, "Option '$1' has bad keyword '$2'")

gen(3, 1416, OPT_TOOSMALLVAL, "Option '$1' has too small value $2 (< $3)")

gen(3, 1418, OPT_TOOBIGVAL, "Option '$1' has too large value $2 (> $3)")

gen(2, 1420, OPT_ZEROVAL, "Option '$1' has bad zero value $2")

gen(3, 1422, OPT_TOOSHORTSTR, "Option '$1' has too short string '$2' (< $3)")

gen(3, 1424, OPT_TOOLONGSTR, "Option '$1' has too long string '$2' (> $3)")

gen(2, 1426, OPT_ILLSPACES,
    "Option '$1' has bad string value '$2' (contains whitespace)")

gen(1, 1428, OPT_NOTFOUND, "Required option '$1' is missing")

gen(1, 1430, OPT_IGNORED, "Option '$1' ignored")

gen(2, 1432, OPT_VERSION, "Option '$1' is not supported in PDF $2")

gen(3, 1434, OPT_ILLHANDLE, "Option '$1' has bad $3 handle $2")

gen(2, 1436, OPT_IGNORE,
    "Option '$1' will be ignored (specified option '$2' dominant)")

gen(1, 1438, OPT_UNSUPP, "Option '$1' not supported in this configuration")

gen(1, 1440, OPT_NOTBAL, "Braces aren't balanced in option list '$1'")


/* -------------------------------------------------------------------- */
/* String conversion functions                              (15xx)      */
/* -------------------------------------------------------------------- */

gen(0, 1500, CONV_ILLUTF16,  "Invalid UTF-16 string (odd byte count)")

gen(0, 1502, CONV_MEMOVERFLOW,  "Out of memory in UTF string conversion")

gen(1, 1504, CONV_ILLUTF, "Invalid UTF-$1 string")


/* -------------------------------------------------------------------- */
/* Internal                                               (19xx)	*/
/* -------------------------------------------------------------------- */

gen(1, 1900, INT_NULLARG, "Invalid NULL argument in function $1")

gen(0, 1902, INT_XSTACK, "Exception stack underflow")

gen(1, 1904, INT_UNUSEDOBJ, "Object $1 allocated but not used")

gen(0, 1906, INT_FLOATTOOLARGE, "Floating point value too large")

gen(2, 1908, INT_BADFORMAT, "Unknown vsprintf() format '$1' ($2)")

gen(1, 1910, INT_ALLOC0,
    "Tried to allocate 0 or negative number of bytes in function $1")

/* Unused. See 1502
gen(1, 1912, INT_UNICODEMEM, "Too few bytes allocated in Unicode function $1")
 */

gen(1, 1914, INT_INVMATRIX, "Matrix [$1] not invertible")


#undef	gen
#undef	pdc_genNames
#undef	pdc_genInfo
