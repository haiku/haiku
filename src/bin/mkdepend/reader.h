/* $Id: reader.h 1.4 Wed, 01 Mar 2000 21:15:40 -0700 lars $ */

/*---------------------------------------------------------------------------
** Copyright (c) 1995-1998 by Lars Düning. All Rights Reserved.
** This file is free software. For terms of use see the file LICENSE.
**---------------------------------------------------------------------------
*/

#ifndef __READER_H__
#define __READER_H__ 1

extern void reader_init (void);
extern int  reader_open (const char *);
extern int  reader_openrw (const char *, const char *);
extern int  reader_close (void);
extern int  reader_eof (void);
extern const char * reader_get (int *);
extern int  reader_writeflush (void);
extern int  reader_writen (const char *, size_t);
extern int  reader_write (const char *);
extern int  reader_copymake (const char *);
extern int  reader_copymake2 (const char *);

#endif
