/*
 *
 * Copyright 2004, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef _SYS_UTSNAME_H
#define _SYS_UTSNAME_H

#define _SYS_NAMELEN 256

struct utsname {
	char sysname[_SYS_NAMELEN];	/* Name of the OS */
	char nodename[_SYS_NAMELEN];	/* Name of this node (network related) */
	char release[_SYS_NAMELEN];	/* Current release level */
	char version[_SYS_NAMELEN];	/* Current version level */
	char machine[_SYS_NAMELEN];	/* Name of the hardware type */
};

int uname(struct utsname *);

#endif

