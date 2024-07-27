/*
 * Copyright (c) 2008-2012, Novell Inc.
 *
 * This program is licensed under the BSD license, read LICENSE.BSD
 * for further information
 */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "pool.h"
#include "util.h"
#include "chksum.h"

#include "md5.h"
#include "sha1.h"
#include "sha2.h"

struct ctxhandle {
  Id type;
  int done;
  unsigned char result[64];
  union {
    MD5_CTX md5;
    SHA1_CTX sha1;
    SHA256_CTX sha256;
  } c;
};

void *
solv_chksum_create(Id type)
{
  struct ctxhandle *h;
  h = solv_calloc(1, sizeof(*h));
  h->type = type;
  switch(type)
    {
    case REPOKEY_TYPE_MD5:
      solv_MD5_Init(&h->c.md5);
      return h;
    case REPOKEY_TYPE_SHA1:
      solv_SHA1_Init(&h->c.sha1);
      return h;
    case REPOKEY_TYPE_SHA256:
      solv_SHA256_Init(&h->c.sha256);
      return h;
    default:
      break;
    }
  free(h);
  return 0;
}

int
solv_chksum_len(Id type)
{
  switch (type)
    {   
    case REPOKEY_TYPE_MD5:
      return 16; 
    case REPOKEY_TYPE_SHA1:
      return 20; 
    case REPOKEY_TYPE_SHA256:
      return 32; 
    default:
      return 0;
    }   
}

void *
solv_chksum_create_from_bin(Id type, const unsigned char *buf)
{
  struct ctxhandle *h;
  int l = solv_chksum_len(type);
  if (buf == 0 || l == 0)
    return 0;
  h = solv_calloc(1, sizeof(*h));
  h->type = type;
  h->done = 1;
  memcpy(h->result, buf, l);
  return h;
}

void
solv_chksum_add(void *handle, const void *data, int len)
{
  struct ctxhandle *h = handle;
  if (h->done)
    return;
  switch(h->type)
    {
    case REPOKEY_TYPE_MD5:
      solv_MD5_Update(&h->c.md5, (void *)data, len);
      return;
    case REPOKEY_TYPE_SHA1:
      solv_SHA1_Update(&h->c.sha1, data, len);
      return;
    case REPOKEY_TYPE_SHA256:
      solv_SHA256_Update(&h->c.sha256, data, len);
      return;
    default:
      return;
    }
}

const unsigned char *
solv_chksum_get(void *handle, int *lenp)
{
  struct ctxhandle *h = handle;
  if (h->done)
    {
      if (lenp)
        *lenp = solv_chksum_len(h->type);
      return h->result;
    }
  switch(h->type)
    {
    case REPOKEY_TYPE_MD5:
      solv_MD5_Final(h->result, &h->c.md5);
      h->done = 1;
      if (lenp)
	*lenp = 16;
      return h->result;
    case REPOKEY_TYPE_SHA1:
      solv_SHA1_Final(&h->c.sha1, h->result);
      h->done = 1;
      if (lenp)
	*lenp = 20;
      return h->result;
    case REPOKEY_TYPE_SHA256:
      solv_SHA256_Final(h->result, &h->c.sha256);
      h->done = 1;
      if (lenp)
	*lenp = 32;
      return h->result;
    default:
      if (lenp)
	*lenp = 0;
      return 0;
    }
}

Id
solv_chksum_get_type(void *handle)
{
  struct ctxhandle *h = handle;
  return h->type;
}

int
solv_chksum_isfinished(void *handle)
{
  struct ctxhandle *h = handle;
  return h->done != 0;
}

const char *
solv_chksum_type2str(Id type)
{
  switch(type)
    {
    case REPOKEY_TYPE_MD5:
      return "md5";
    case REPOKEY_TYPE_SHA1:
      return "sha1";
    case REPOKEY_TYPE_SHA256:
      return "sha256";
    default:
      return 0;
    }
}

Id
solv_chksum_str2type(const char *str)
{
  if (!strcasecmp(str, "md5"))
    return REPOKEY_TYPE_MD5;
  if (!strcasecmp(str, "sha") || !strcasecmp(str, "sha1"))
    return REPOKEY_TYPE_SHA1;
  if (!strcasecmp(str, "sha256"))
    return REPOKEY_TYPE_SHA256;
  return 0;
}

void *
solv_chksum_free(void *handle, unsigned char *cp)
{
  if (cp)
    {
      const unsigned char *res;
      int l;
      res = solv_chksum_get(handle, &l);
      if (l && res)
        memcpy(cp, res, l);
    }
  solv_free(handle);
  return 0;
}

