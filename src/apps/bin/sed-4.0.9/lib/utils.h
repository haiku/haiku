/*  Functions from hack's utils library.
    Copyright (C) 1989, 1990, 1991, 1998, 1999, 2003
    Free Software Foundation, Inc.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. */

#include <stdio.h>

#include "basicdefs.h"

void panic P_((const char *str, ...));

FILE *ck_fopen P_((const char *name, const char *mode, flagT fail));
void ck_fwrite P_((const VOID *ptr, size_t size, size_t nmemb, FILE *stream));
size_t ck_fread P_((VOID *ptr, size_t size, size_t nmemb, FILE *stream));
void ck_fflush P_((FILE *stream));
void ck_fclose P_((FILE *stream));

char *temp_file_template P_((const char *tmpdir, char *program));

VOID *ck_malloc P_((size_t size));
VOID *xmalloc P_((size_t size));
VOID *ck_realloc P_((VOID *ptr, size_t size));
char *ck_strdup P_((const char *str));
VOID *ck_memdup P_((const VOID *buf, size_t len));
void ck_free P_((VOID *ptr));

struct buffer *init_buffer P_((void));
char *get_buffer P_((struct buffer *b));
size_t size_buffer P_((struct buffer *b));
void add_buffer P_((struct buffer *b, const char *p, size_t n));
void add1_buffer P_((struct buffer *b, int ch));
void free_buffer P_((struct buffer *b));

extern const char *myname;
