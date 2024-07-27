/*
 * Copyright (c) 2007, Novell Inc.
 *
 * This program is licensed under the BSD license, read LICENSE.BSD
 * for further information
 */

/* pack/unpack functions for key data */

#ifndef LIBSOLV_REPOPACK_H
#define LIBSOLV_REPOPACK_H

static inline unsigned char *
data_read_id(unsigned char *dp, Id *idp)
{
  Id x;
  unsigned char c;
  if (!(dp[0] & 0x80))
    {
      *idp = dp[0];
      return dp + 1;
    }
  if (!(dp[1] & 0x80))
    {
      *idp = dp[0] << 7 ^ dp[1] ^ 0x4000;
      return dp + 2;
    }
  if (!(dp[2] & 0x80))
    {
      *idp = dp[0] << 14 ^ dp[1] << 7 ^ dp[2] ^ 0x204000;
      return dp + 3;
    }
  if (!(dp[3] & 0x80))
    {
      *idp = dp[0] << 21 ^ dp[1] << 14 ^ dp[2] << 7 ^ dp[3] ^ 0x10204000;
      return dp + 4;
    }
  x = dp[0] << 28 ^ dp[1] << 21 ^ dp[2] << 14 ^ dp[3] << 7 ^ dp[4] ^ 0x10204000;
  if (!(dp[4] & 0x80))
    {
      *idp = x;
      return dp + 5;
    }
  x ^= 80;
  dp += 5;
  for (;;)
    {
      c = *dp++;
      if (!(c & 0x80))
        {
          *idp = (x << 7) ^ c;
          return dp;
        }
      x = (x << 7) ^ (c ^ 128);
    }
}

static inline unsigned char *
data_read_num64(unsigned char *dp, unsigned int *low, unsigned int *high)
{
  unsigned long long int x;
  unsigned char c;

  *high = 0;
  if (!(dp[0] & 0x80))
    {
      *low = dp[0];
      return dp + 1;
    }
  if (!(dp[1] & 0x80))
    {
      *low = dp[0] << 7 ^ dp[1] ^ 0x4000;
      return dp + 2;
    }
  if (!(dp[2] & 0x80))
    {
      *low = dp[0] << 14 ^ dp[1] << 7 ^ dp[2] ^ 0x204000;
      return dp + 3;
    }
  if (!(dp[3] & 0x80))
    {
      *low = dp[0] << 21 ^ dp[1] << 14 ^ dp[2] << 7 ^ dp[3] ^ 0x10204000;
      return dp + 4;
    }
  if (!(dp[4] & 0x80))
    {
      *low = dp[0] << 28 ^ dp[1] << 21 ^ dp[2] << 14 ^ dp[3] << 7 ^ dp[4] ^ 0x10204000;
      *high = (dp[0] ^ 0x80) >> 4;
      return dp + 5;
    }
  x = (unsigned long long)(dp[0] ^ 0x80) << 28 ^ (unsigned int)(dp[1] << 21 ^ dp[2] << 14 ^ dp[3] << 7 ^ dp[4] ^ 0x10204080);
  dp += 5;
  for (;;)
    {
      c = *dp++;
      if (!(c & 0x80))
	{
	  x = (x << 7) ^ c;
	  *low = x;
	  *high = x >> 32;
	  return dp;
	}
      x = (x << 7) ^ (c ^ 128);
    }
}

static inline unsigned char *
data_read_ideof(unsigned char *dp, Id *idp, int *eof)
{
  Id x = 0;
  unsigned char c;
  for (;;)
    {
      c = *dp++;
      if (!(c & 0x80))
        {
          if (c & 0x40)
            {
              c ^= 0x40;
              *eof = 0;
            }
          else
            *eof = 1;
          *idp = (x << 6) ^ c;
          return dp;
        }
      x = (x << 7) ^ c ^ 128;
    }
}

static inline unsigned char *
data_read_u32(unsigned char *dp, unsigned int *nump)
{
  *nump = (dp[0] << 24) | (dp[1] << 16) | (dp[2] << 8) | dp[3];
  return dp + 4;
}

static inline unsigned char *
data_fetch(unsigned char *dp, KeyValue *kv, Repokey *key)
{
  kv->eof = 1;
  if (!dp)
    return 0;
  switch (key->type)
    {
    case REPOKEY_TYPE_VOID:
      return dp;
    case REPOKEY_TYPE_CONSTANT:
      kv->num2 = 0;
      kv->num = key->size;
      return dp;
    case REPOKEY_TYPE_CONSTANTID:
      kv->id = key->size;
      return dp;
    case REPOKEY_TYPE_STR:
      kv->str = (const char *)dp;
      return dp + strlen(kv->str) + 1;
    case REPOKEY_TYPE_ID:
    case REPOKEY_TYPE_DIR:
      return data_read_id(dp, &kv->id);
    case REPOKEY_TYPE_NUM:
      return data_read_num64(dp, &kv->num, &kv->num2);
    case REPOKEY_TYPE_U32:
      kv->num2 = 0;
      return data_read_u32(dp, &kv->num);
    case REPOKEY_TYPE_MD5:
      kv->num = 0;	/* not stringified yet */
      kv->str = (const char *)dp;
      return dp + SIZEOF_MD5;
    case REPOKEY_TYPE_SHA1:
      kv->num = 0;	/* not stringified yet */
      kv->str = (const char *)dp;
      return dp + SIZEOF_SHA1;
    case REPOKEY_TYPE_SHA256:
      kv->num = 0;	/* not stringified yet */
      kv->str = (const char *)dp;
      return dp + SIZEOF_SHA256;
    case REPOKEY_TYPE_BINARY:
      dp = data_read_id(dp, (Id *)&kv->num);
      kv->str = (const char *)dp;
      return dp + kv->num;
    case REPOKEY_TYPE_IDARRAY:
      return data_read_ideof(dp, &kv->id, &kv->eof);
    case REPOKEY_TYPE_DIRSTRARRAY:
      dp = data_read_ideof(dp, &kv->id, &kv->eof);
      kv->num = 0;	/* not stringified yet */
      kv->str = (const char *)dp;
      return dp + strlen(kv->str) + 1;
    case REPOKEY_TYPE_DIRNUMNUMARRAY:
      dp = data_read_id(dp, &kv->id);
      dp = data_read_id(dp, (Id *)&kv->num);
      return data_read_ideof(dp, (Id *)&kv->num2, &kv->eof);
    case REPOKEY_TYPE_FIXARRAY:
      dp = data_read_id(dp, (Id *)&kv->num);
      return data_read_id(dp, &kv->id);
    case REPOKEY_TYPE_FLEXARRAY:
      return data_read_id(dp, (Id *)&kv->num);
    default:
      return 0;
    }
}

static inline unsigned char *
data_skip(unsigned char *dp, int type)
{
  unsigned char x;
  switch (type)
    {
    case REPOKEY_TYPE_VOID:
    case REPOKEY_TYPE_CONSTANT:
    case REPOKEY_TYPE_CONSTANTID:
    case REPOKEY_TYPE_DELETED:
      return dp;
    case REPOKEY_TYPE_ID:
    case REPOKEY_TYPE_NUM:
    case REPOKEY_TYPE_DIR:
      while ((*dp & 0x80) != 0)
        dp++;
      return dp + 1;
    case REPOKEY_TYPE_U32:
      return dp + 4;
    case REPOKEY_TYPE_MD5:
      return dp + SIZEOF_MD5;
    case REPOKEY_TYPE_SHA1:
      return dp + SIZEOF_SHA1;
    case REPOKEY_TYPE_SHA256:
      return dp + SIZEOF_SHA256;
    case REPOKEY_TYPE_IDARRAY:
    case REPOKEY_TYPE_REL_IDARRAY:
      while ((*dp & 0xc0) != 0)
        dp++;
      return dp + 1;
    case REPOKEY_TYPE_STR:
      while ((*dp) != 0)
        dp++;
      return dp + 1;
    case REPOKEY_TYPE_BINARY:
      {
	unsigned int len;
	dp = data_read_id(dp, (Id *)&len);
	return dp + len;
      }
    case REPOKEY_TYPE_DIRSTRARRAY:
      for (;;)
        {
          while ((*dp & 0x80) != 0)
            dp++;
          x = *dp++;
          while ((*dp) != 0)
            dp++;
          dp++;
          if (!(x & 0x40))
            return dp;
        }
    case REPOKEY_TYPE_DIRNUMNUMARRAY:
      for (;;)
        {
          while ((*dp & 0x80) != 0)
            dp++;
          dp++;
          while ((*dp & 0x80) != 0)
            dp++;
          dp++;
          while ((*dp & 0x80) != 0)
            dp++;
          if (!(*dp & 0x40))
            return dp + 1;
          dp++;
        }
    default:
      return 0;
    }
}

static inline unsigned char *
data_skip_verify(unsigned char *dp, int type, int maxid, int maxdir)
{
  Id id;
  int eof;

  switch (type)
    {
    case REPOKEY_TYPE_VOID:
    case REPOKEY_TYPE_CONSTANT:
    case REPOKEY_TYPE_CONSTANTID:
    case REPOKEY_TYPE_DELETED:
      return dp;
    case REPOKEY_TYPE_NUM:
      while ((*dp & 0x80) != 0)
        dp++;
      return dp + 1;
    case REPOKEY_TYPE_U32:
      return dp + 4;
    case REPOKEY_TYPE_MD5:
      return dp + SIZEOF_MD5;
    case REPOKEY_TYPE_SHA1:
      return dp + SIZEOF_SHA1;
    case REPOKEY_TYPE_SHA256:
      return dp + SIZEOF_SHA256;
    case REPOKEY_TYPE_ID:
      dp = data_read_id(dp, &id);
      if (id >= maxid)
	return 0;
      return dp;
    case REPOKEY_TYPE_DIR:
      dp = data_read_id(dp, &id);
      if (id >= maxdir)
	return 0;
      return dp;
    case REPOKEY_TYPE_IDARRAY:
      for (;;)
	{
	  dp = data_read_ideof(dp, &id, &eof);
	  if (id >= maxid)
	    return 0;
	  if (eof)
	    return dp;
	}
    case REPOKEY_TYPE_STR:
      while ((*dp) != 0)
        dp++;
      return dp + 1;
    case REPOKEY_TYPE_BINARY:
      {
	unsigned int len;
	dp = data_read_id(dp, (Id *)&len);
	return dp + len;
      }
    case REPOKEY_TYPE_DIRSTRARRAY:
      for (;;)
        {
	  dp = data_read_ideof(dp, &id, &eof);
	  if (id >= maxdir)
	    return 0;
          while ((*dp) != 0)
            dp++;
          dp++;
          if (eof)
            return dp;
        }
    case REPOKEY_TYPE_DIRNUMNUMARRAY:
      for (;;)
        {
	  dp = data_read_id(dp, &id);
	  if (id >= maxdir)
	    return 0;
          while ((*dp & 0x80) != 0)
            dp++;
          dp++;
          while ((*dp & 0x80) != 0)
            dp++;
          if (!(*dp & 0x40))
            return dp + 1;
          dp++;
        }
    default:
      return 0;
    }
}

#endif	/* LIBSOLV_REPOPACK */
