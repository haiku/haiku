/*
	Copyright 1999-2001, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _ALLOC_H
#define _ALLOC_H

#include <SupportDefs.h>

#if KMALLOC_TRACKING
extern bool			kmalloc_tracking_enabled;
#endif

extern void			init_malloc(void);
extern void			init_smalloc(void);

extern void *		smalloc(unsigned int nbytes);
extern void			sfree(void *ptr);
extern void *		scalloc(unsigned int nobj, unsigned int size);
extern void *		srealloc(void *p, unsigned int newsize);

#endif
