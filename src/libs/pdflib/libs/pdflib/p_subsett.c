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

/* $Id: p_subsett.c 14574 2005-10-29 16:27:43Z bonefish $
 *
 * PDFlib subsetting routines
 *
 */

#include <time.h>

#include "p_intern.h"
#include "p_font.h"
#include "p_truetype.h"

#include "pc_md5.h"

