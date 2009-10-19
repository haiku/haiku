/* PDFlib GmbH cvsid: $Id: tiffvers.h 14574 2005-10-29 16:27:43Z bonefish $ */

#define TIFFLIB_VERSION_STR "LIBTIFF, Version 3.5.7\n\
Copyright (c) 1988-1996 Sam Leffler\n\
Copyright (c) 1991-1996 Silicon Graphics, Inc."
/*
 * This define can be used in code that requires
 * compilation-related definitions specific to a
 * version or versions of the library.  Runtime
 * version checking should be done based on the
 * string returned by TIFFGetVersion.
 */
#define TIFFLIB_VERSION 20011123
