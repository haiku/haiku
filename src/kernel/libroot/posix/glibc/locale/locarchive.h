/* Definitions for locale archive handling.
   Copyright (C) 2002 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#ifndef _LOCARCHIVE_H
#define _LOCARCHIVE_H 1

#include <stdint.h>


#define AR_MAGIC 0xde020109

struct locarhead
{
  uint32_t magic;
  /* Serial number.  */
  uint32_t serial;
  /* Name hash table.  */
  uint32_t namehash_offset;
  uint32_t namehash_used;
  uint32_t namehash_size;
  /* String table.  */
  uint32_t string_offset;
  uint32_t string_used;
  uint32_t string_size;
  /* Table with locale records.  */
  uint32_t locrectab_offset;
  uint32_t locrectab_used;
  uint32_t locrectab_size;
  /* MD5 sum hash table.  */
  uint32_t sumhash_offset;
  uint32_t sumhash_used;
  uint32_t sumhash_size;
};


struct namehashent
{
  /* Hash value of the name.  */
  uint32_t hashval;
  /* Offset of the name in the string table.  */
  uint32_t name_offset;
  /* Offset of the locale record.  */
  uint32_t locrec_offset;
};


struct sumhashent
{
  /* MD5 sum.  */
  char sum[16];
  /* Offset of the file in the archive.  */
  uint32_t file_offset;
};

struct locrecent
{
  uint32_t refs;		/* # of namehashent records that point here */
  struct
  {
    uint32_t offset;
    uint32_t len;
  } record[__LC_LAST];
};


struct locarhandle
{
  int fd;
  void *addr;
  size_t len;
};


/* In memory data for the locales with their checksums.  */
typedef struct locale_category_data
{
  off64_t size;
  void *addr;
  char sum[16];
} locale_data_t[__LC_LAST];

#endif	/* locarchive.h */
