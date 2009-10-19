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

/* $Id: pc_nocarbon.h 14574 2005-10-29 16:27:43Z bonefish $
 *
 * Header for CodeWarrior to activate non-Carbon builds.
 *
 */

/*
 * This must only be set for non-Carbon builds. It is not used for the
 * standard build.
 */

#define PDF_TARGET_API_MAC_OS8
