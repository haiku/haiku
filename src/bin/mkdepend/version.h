/* $Id: version.h 1.2 Sat, 24 Oct 1998 19:54:13 -0600 lars $ */

/*---------------------------------------------------------------------------
 * Version information for MkDepend.
 *
 * Copyright (c) 1998 by Lars Düning. All Rights Reserved.
 * This file is free software. For terms of use see the file LICENSE.
 *---------------------------------------------------------------------------
 * This macros are exported:
 *
 *   VERSION:     a string of the form "x.y" or "x.y.z", with z being >= 2.
 *   RELEASEDATE: a string with date and time when the current version was
 *                checked in.
 *   PRCS_LEVEL:  the internal minor version of MkDepend, used for <z>
 *                in VERSION when the value is >= 2.
 *
 * The values are created by PRCS.
 *---------------------------------------------------------------------------
 */

#ifndef __VERSION_H__
#define __VERSION_H__

/* $Format: "#define RELEASE_DATE \"$ReleaseDate$\""$ */
#define RELEASE_DATE "Thu, 19 Mar 2009 11:20:00 -0100"

/* $Format: "#define PRCS_LEVEL $ProjectMinorVersion$"$ */
#define PRCS_LEVEL 1

#if PRCS_LEVEL > 1
/* $Format: "#define VERSION \"$ReleaseVersion$.$ProjectMinorVersion$\""$ */
#define VERSION "1.7.1"
#else
/* $Format: "#define VERSION \"$ReleaseVersion$\""$ */
#define VERSION "1.7"
#endif

#endif /* __VERSION_H__ */
