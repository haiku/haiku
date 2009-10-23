/*
 * Copyright 2009, Colin Günther, coling@gmx.de.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _FBSD_COMPAT_SYS_IOCCOM_H_
#define _FBSD_COMPAT_SYS_IOCCOM_H_


/*
 * Ioctl's have the command encoded in the lower word, and the size of
 * any in or out parameters in the upper word.  The high 3 bits of the
 * upper word are used to encode the in/out status of the parameter.
 */
#define	IOCPARM_MASK 0x1fff		/* parameter length, at most 13 bits */

#define	IOC_OUT		0x40000000	/* copy out parameters */
#define	IOC_IN		0x80000000	/* copy in parameters */
#define	IOC_INOUT	(IOC_IN|IOC_OUT)


#define	_IOC(inout,group,num,len) ((unsigned long) \
	((inout) | (((len) & IOCPARM_MASK) << 16) | ((group) << 8) | (num)))

#define	_IOW(g,n,t)	_IOC(IOC_IN,	(g), (n), sizeof(t))
/* this should be _IORW, but stdio got there first */
#define	_IOWR(g,n,t)	_IOC(IOC_INOUT,	(g), (n), sizeof(t))

#endif /* _FBSD_COMPAT_SYS_IOCCOM_H_ */
