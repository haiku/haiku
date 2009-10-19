/* $Id: p_icclib.c 14574 2005-10-29 16:27:43Z bonefish $
 *
 * ICClib routines for PDFlib, slightly modified from the original ICClib.
 * (see below).
 *
 */

/*
 * International Color Consortium Format Library (icclib)
 * For ICC profile version 3.4
 *
 * Author:  Graeme W. Gill
 * Date:    2002/04/22
 * Version: 2.02
 *
 * Copyright 1997 - 2002 Graeme W. Gill
 * See Licence.txt file for conditions of use.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#ifdef __sun
#include <unistd.h>
#endif
#if defined(__IBMC__) && defined(_M_IX86)
#include <float.h>
#endif

/* PDFlib */
#include "pc_core.h"
