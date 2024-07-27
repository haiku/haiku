/*
 * Copyright (c) 2007-2011, Novell Inc.
 *
 * This program is licensed under the BSD license, read LICENSE.BSD
 * for further information
 */

/*
 * repo_write.c
 *
 * Write Repo data out to a file in solv format
 *
 * See doc/README.format for a description
 * of the binary file format
 *
 */

#include <sys/types.h>
#include <limits.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include "pool.h"
#include "util.h"
#include "repo_write.h"
#include "repopage.h"

/*------------------------------------------------------------------*/
/* Id map optimizations */

typedef struct needid {
  Id need;
  Id map;
} NeedId;


#define RELOFF(id) (needid[0].map + GETRELID(id))

/*
 * increment need Id
 * idarray: array of Ids, ID_NULL terminated
 * needid: array of Id->NeedId
 *
 * return size of array (including trailing zero)
 *
 */

static void
incneedid(Pool *pool, Id id, NeedId *needid)
{
  while (ISRELDEP(id))
    {
      Reldep *rd = GETRELDEP(pool, id);
      needid[RELOFF(id)].need++;
      if (ISRELDEP(rd->evr))
	incneedid(pool, rd->evr, needid);
      else
	needid[rd->evr].need++;
      id = rd->name;
    }
  needid[id].need++;
}

static int
incneedidarray(Pool *pool, Id *idarray, NeedId *needid)
{
  Id id;
  int n = 0;

  if (!idarray)
    return 0;
  while ((id = *idarray++) != 0)
    {
      n++;
      while (ISRELDEP(id))
	{
	  Reldep *rd = GETRELDEP(pool, id);
	  needid[RELOFF(id)].need++;
	  if (ISRELDEP(rd->evr))
	    incneedid(pool, rd->evr, needid);
	  else
	    needid[rd->evr].need++;
	  id = rd->name;
	}
      needid[id].need++;
    }
  return n + 1;
}


/*
 *
 */

static int
needid_cmp_need(const void *ap, const void *bp, void *dp)
{
  const NeedId *a = ap;
  const NeedId *b = bp;
  int r;
  r = b->need - a->need;
  if (r)
    return r;
  return a->map - b->map;
}

static int
needid_cmp_need_s(const void *ap, const void *bp, void *dp)
{
  const NeedId *a = ap;
  const NeedId *b = bp;
  Stringpool *spool = dp;
  const char *as;
  const char *bs;

  int r;
  r = b->need - a->need;
  if (r)
    return r;
  as = spool->stringspace + spool->strings[a->map];
  bs = spool->stringspace + spool->strings[b->map];
  return strcmp(as, bs);
}


/*------------------------------------------------------------------*/
/* output helper routines, used for writing the header */
/* (the data itself is accumulated in memory and written with
 * write_blob) */

/*
 * unsigned 32-bit
 */

static void
write_u32(Repodata *data, unsigned int x)
{
  FILE *fp = data->fp;
  if (data->error)
    return;
  if (putc(x >> 24, fp) == EOF ||
      putc(x >> 16, fp) == EOF ||
      putc(x >> 8, fp) == EOF ||
      putc(x, fp) == EOF)
    {
      data->error = pool_error(data->repo->pool, -1, "write error u32: %s", strerror(errno));
    }
}


/*
 * unsigned 8-bit
 */

static void
write_u8(Repodata *data, unsigned int x)
{
  if (data->error)
    return;
  if (putc(x, data->fp) == EOF)
    {
      data->error = pool_error(data->repo->pool, -1, "write error u8: %s", strerror(errno));
    }
}

/*
 * data blob
 */

static void
write_blob(Repodata *data, void *blob, int len)
{
  if (data->error)
    return;
  if (len && fwrite(blob, len, 1, data->fp) != 1)
    {
      data->error = pool_error(data->repo->pool, -1, "write error blob: %s", strerror(errno));
    }
}

/*
 * Id
 */

static void
write_id(Repodata *data, Id x)
{
  FILE *fp = data->fp;
  if (data->error)
    return;
  if (x >= (1 << 14))
    {
      if (x >= (1 << 28))
	putc((x >> 28) | 128, fp);
      if (x >= (1 << 21))
	putc((x >> 21) | 128, fp);
      putc((x >> 14) | 128, fp);
    }
  if (x >= (1 << 7))
    putc((x >> 7) | 128, fp);
  if (putc(x & 127, fp) == EOF)
    {
      data->error = pool_error(data->repo->pool, -1, "write error id: %s", strerror(errno));
    }
}

static inline void
write_id_eof(Repodata *data, Id x, int eof)
{
  if (x >= 64)
    x = (x & 63) | ((x & ~63) << 1);
  write_id(data, x | (eof ? 0 : 64));
}



static inline void
write_str(Repodata *data, const char *str)
{
  if (data->error)
    return;
  if (fputs(str, data->fp) == EOF || putc(0, data->fp) == EOF)
    {
      data->error = pool_error(data->repo->pool, -1, "write error str: %s", strerror(errno));
    }
}

/*
 * Array of Ids
 */

static void
write_idarray(Repodata *data, Pool *pool, NeedId *needid, Id *ids)
{
  Id id;
  if (!ids)
    return;
  if (!*ids)
    {
      write_u8(data, 0);
      return;
    }
  for (;;)
    {
      id = *ids++;
      if (needid)
        id = needid[ISRELDEP(id) ? RELOFF(id) : id].need;
      if (id >= 64)
	id = (id & 63) | ((id & ~63) << 1);
      if (!*ids)
	{
	  write_id(data, id);
	  return;
	}
      write_id(data, id | 64);
    }
}

static int
cmp_ids(const void *pa, const void *pb, void *dp)
{
  Id a = *(Id *)pa;
  Id b = *(Id *)pb;
  return a - b;
}

#if 0
static void
write_idarray_sort(Repodata *data, Pool *pool, NeedId *needid, Id *ids, Id marker)
{
  int len, i;
  Id lids[64], *sids;

  if (!ids)
    return;
  if (!*ids)
    {
      write_u8(data, 0);
      return;
    }
  for (len = 0; len < 64 && ids[len]; len++)
    {
      Id id = ids[len];
      if (needid)
        id = needid[ISRELDEP(id) ? RELOFF(id) : id].need;
      lids[len] = id;
    }
  if (ids[len])
    {
      for (i = len + 1; ids[i]; i++)
	;
      sids = solv_malloc2(i, sizeof(Id));
      memcpy(sids, lids, 64 * sizeof(Id));
      for (; ids[len]; len++)
	{
	  Id id = ids[len];
	  if (needid)
            id = needid[ISRELDEP(id) ? RELOFF(id) : id].need;
	  sids[len] = id;
	}
    }
  else
    sids = lids;

  /* That bloody solvable:prereqmarker needs to stay in position :-(  */
  if (needid)
    marker = needid[marker].need;
  for (i = 0; i < len; i++)
    if (sids[i] == marker)
      break;
  if (i > 1)
    solv_sort(sids, i, sizeof(Id), cmp_ids, 0);
  if ((len - i) > 2)
    solv_sort(sids + i + 1, len - i - 1, sizeof(Id), cmp_ids, 0);

  Id id, old = 0;

  /* The differencing above produces many runs of ones and twos.  I tried
     fairly elaborate schemes to RLE those, but they give only very mediocre
     improvements in compression, as coding the escapes costs quite some
     space.  Even if they are coded only as bits in IDs.  The best improvement
     was about 2.7% for the whole .solv file.  It's probably better to
     invest some complexity into sharing idarrays, than RLEing.  */
  for (i = 0; i < len - 1; i++)
    {
      id = sids[i];
    /* Ugly PREREQ handling.  A "difference" of 0 is the prereq marker,
       hence all real differences are offsetted by 1.  Otherwise we would
       have to handle negative differences, which would cost code space for
       the encoding of the sign.  We loose the exact mapping of prereq here,
       but we know the result, so we can recover from that in the reader.  */
      if (id == marker)
	id = old = 0;
      else
	{
          id = id - old + 1;
	  old = sids[i];
	}
      /* XXX If difference is zero we have multiple equal elements,
	 we might want to skip writing them out.  */
      if (id >= 64)
	id = (id & 63) | ((id & ~63) << 1);
      write_id(data, id | 64);
    }
  id = sids[i];
  if (id == marker)
    id = 0;
  else
    id = id - old + 1;
  if (id >= 64)
    id = (id & 63) | ((id & ~63) << 1);
  write_id(data, id);
  if (sids != lids)
    solv_free(sids);
}
#endif


struct extdata {
  unsigned char *buf;
  int len;
};

struct cbdata {
  Repo *repo;
  Repodata *target;

  Stringpool *ownspool;
  Dirpool *owndirpool;

  Id *keymap;
  int nkeymap;
  Id *keymapstart;

  NeedId *needid;

  Id *schema;		/* schema construction space */
  Id *sp;		/* pointer in above */
  Id *oldschema, *oldsp;

  Id *solvschemata;
  Id *subschemata;
  int nsubschemata;
  int current_sub;

  struct extdata *extdata;

  Id *dirused;

  Id vstart;

  Id maxdata;
  Id lastlen;

  int doingsolvables;	/* working on solvables data */
};

#define NEEDED_BLOCK 1023
#define SCHEMATA_BLOCK 31
#define SCHEMATADATA_BLOCK 255
#define EXTDATA_BLOCK 4095

static inline void
data_addid(struct extdata *xd, Id sx)
{
  unsigned int x = (unsigned int)sx;
  unsigned char *dp;

  xd->buf = solv_extend(xd->buf, xd->len, 5, 1, EXTDATA_BLOCK);
  dp = xd->buf + xd->len;

  if (x >= (1 << 14))
    {
      if (x >= (1 << 28))
	*dp++ = (x >> 28) | 128;
      if (x >= (1 << 21))
	*dp++ = (x >> 21) | 128;
      *dp++ = (x >> 14) | 128;
    }
  if (x >= (1 << 7))
    *dp++ = (x >> 7) | 128;
  *dp++ = x & 127;
  xd->len = dp - xd->buf;
}

static inline void
data_addideof(struct extdata *xd, Id sx, int eof)
{
  unsigned int x = (unsigned int)sx;
  unsigned char *dp;

  xd->buf = solv_extend(xd->buf, xd->len, 5, 1, EXTDATA_BLOCK);
  dp = xd->buf + xd->len;

  if (x >= (1 << 13))
    {
      if (x >= (1 << 27))
        *dp++ = (x >> 27) | 128;
      if (x >= (1 << 20))
        *dp++ = (x >> 20) | 128;
      *dp++ = (x >> 13) | 128;
    }
  if (x >= (1 << 6))
    *dp++ = (x >> 6) | 128;
  *dp++ = eof ? (x & 63) : (x & 63) | 64;
  xd->len = dp - xd->buf;
}

static void
data_addid64(struct extdata *xd, unsigned int x, unsigned int hx)
{
  if (hx)
    {
      if (hx > 7)
        {
          data_addid(xd, (Id)(hx >> 3));
          xd->buf[xd->len - 1] |= 128;
	  hx &= 7;
        }
      data_addid(xd, (Id)(x | 0x80000000));
      xd->buf[xd->len - 5] = (x >> 28) | (hx << 4) | 128;
    }
  else
    data_addid(xd, (Id)x);
}

static void
data_addidarray_sort(struct extdata *xd, Pool *pool, NeedId *needid, Id *ids, Id marker)
{
  int len, i;
  Id lids[64], *sids;
  Id id, old;

  if (!ids)
    return;
  if (!*ids)
    {
      data_addid(xd, 0);
      return;
    }
  for (len = 0; len < 64 && ids[len]; len++)
    {
      Id id = ids[len];
      if (needid)
        id = needid[ISRELDEP(id) ? RELOFF(id) : id].need;
      lids[len] = id;
    }
  if (ids[len])
    {
      for (i = len + 1; ids[i]; i++)
	;
      sids = solv_malloc2(i, sizeof(Id));
      memcpy(sids, lids, 64 * sizeof(Id));
      for (; ids[len]; len++)
	{
	  Id id = ids[len];
	  if (needid)
            id = needid[ISRELDEP(id) ? RELOFF(id) : id].need;
	  sids[len] = id;
	}
    }
  else
    sids = lids;

  /* That bloody solvable:prereqmarker needs to stay in position :-(  */
  if (needid)
    marker = needid[marker].need;
  for (i = 0; i < len; i++)
    if (sids[i] == marker)
      break;
  if (i > 1)
    solv_sort(sids, i, sizeof(Id), cmp_ids, 0);
  if ((len - i) > 2)
    solv_sort(sids + i + 1, len - i - 1, sizeof(Id), cmp_ids, 0);

  old = 0;

  /* The differencing above produces many runs of ones and twos.  I tried
     fairly elaborate schemes to RLE those, but they give only very mediocre
     improvements in compression, as coding the escapes costs quite some
     space.  Even if they are coded only as bits in IDs.  The best improvement
     was about 2.7% for the whole .solv file.  It's probably better to
     invest some complexity into sharing idarrays, than RLEing.  */
  for (i = 0; i < len - 1; i++)
    {
      id = sids[i];
    /* Ugly PREREQ handling.  A "difference" of 0 is the prereq marker,
       hence all real differences are offsetted by 1.  Otherwise we would
       have to handle negative differences, which would cost code space for
       the encoding of the sign.  We loose the exact mapping of prereq here,
       but we know the result, so we can recover from that in the reader.  */
      if (id == marker)
	id = old = 0;
      else
	{
          id = id - old + 1;
	  old = sids[i];
	}
      /* XXX If difference is zero we have multiple equal elements,
	 we might want to skip writing them out.  */
      data_addideof(xd, id, 0);
    }
  id = sids[i];
  if (id == marker)
    id = 0;
  else
    id = id - old + 1;
  data_addideof(xd, id, 1);
  if (sids != lids)
    solv_free(sids);
}

static inline void
data_addblob(struct extdata *xd, unsigned char *blob, int len)
{
  xd->buf = solv_extend(xd->buf, xd->len, len, 1, EXTDATA_BLOCK);
  memcpy(xd->buf + xd->len, blob, len);
  xd->len += len;
}

static inline void
data_addu32(struct extdata *xd, unsigned int num)
{
  unsigned char d[4];
  d[0] = num >> 24;
  d[1] = num >> 16;
  d[2] = num >> 8;
  d[3] = num;
  data_addblob(xd, d, 4);
}

static Id
putinownpool(struct cbdata *cbdata, Stringpool *ss, Id id)
{
  const char *str = stringpool_id2str(ss, id);
  id = stringpool_str2id(cbdata->ownspool, str, 1);
  if (id >= cbdata->needid[0].map)
    {
      int oldoff = cbdata->needid[0].map;
      int newoff = (id + 1 + NEEDED_BLOCK) & ~NEEDED_BLOCK;
      int nrels = cbdata->repo->pool->nrels;
      cbdata->needid = solv_realloc2(cbdata->needid, newoff + nrels, sizeof(NeedId));
      if (nrels)
	memmove(cbdata->needid + newoff, cbdata->needid + oldoff, nrels * sizeof(NeedId));
      memset(cbdata->needid + oldoff, 0, (newoff - oldoff) * sizeof(NeedId));
      cbdata->needid[0].map = newoff;
    }
  return id;
}

static Id
putinowndirpool(struct cbdata *cbdata, Repodata *data, Dirpool *dp, Id dir)
{
  Id compid, parent;

  parent = dirpool_parent(dp, dir);
  if (parent)
    parent = putinowndirpool(cbdata, data, dp, parent);
  compid = dp->dirs[dir];
  if (cbdata->ownspool && compid > 1)
    compid = putinownpool(cbdata, data->localpool ? &data->spool : &data->repo->pool->ss, compid);
  return dirpool_add_dir(cbdata->owndirpool, parent, compid, 1);
}

/*
 * collect usage information about the dirs
 * 1: dir used, no child of dir used
 * 2: dir used as parent of another used dir
 */
static inline void
setdirused(struct cbdata *cbdata, Dirpool *dp, Id dir)
{
  if (cbdata->dirused[dir])
    return;
  cbdata->dirused[dir] = 1;
  while ((dir = dirpool_parent(dp, dir)) != 0)
    {
      if (cbdata->dirused[dir] == 2)
	return;
      if (cbdata->dirused[dir])
        {
	  cbdata->dirused[dir] = 2;
	  return;
        }
      cbdata->dirused[dir] = 2;
    }
  cbdata->dirused[0] = 2;
}

/*
 * pass 1 callback:
 * collect key/id/dirid usage information, create needed schemas
 */
static int
repo_write_collect_needed(struct cbdata *cbdata, Repo *repo, Repodata *data, Repokey *key, KeyValue *kv)
{
  Id id;
  int rm;

  if (key->name == REPOSITORY_SOLVABLES)
    return SEARCH_NEXT_KEY;	/* we do not want this one */

  /* hack: ignore some keys, see BUGS */
  if (data->repodataid != data->repo->nrepodata - 1)
    if (key->name == REPOSITORY_ADDEDFILEPROVIDES || key->name == REPOSITORY_EXTERNAL || key->name == REPOSITORY_LOCATION || key->name == REPOSITORY_KEYS || key->name == REPOSITORY_TOOLVERSION)
      return SEARCH_NEXT_KEY;

  rm = cbdata->keymap[cbdata->keymapstart[data->repodataid] + (key - data->keys)];
  if (!rm)
    return SEARCH_NEXT_KEY;	/* we do not want this one */

  /* record key in schema */
  if ((key->type != REPOKEY_TYPE_FIXARRAY || kv->eof == 0)
      && (cbdata->sp == cbdata->schema || cbdata->sp[-1] != rm))
    *cbdata->sp++ = rm;

  switch(key->type)
    {
      case REPOKEY_TYPE_ID:
      case REPOKEY_TYPE_IDARRAY:
	id = kv->id;
	if (!ISRELDEP(id) && cbdata->ownspool && id > 1)
	  id = putinownpool(cbdata, data->localpool ? &data->spool : &repo->pool->ss, id);
	incneedid(repo->pool, id, cbdata->needid);
	break;
      case REPOKEY_TYPE_DIR:
      case REPOKEY_TYPE_DIRNUMNUMARRAY:
      case REPOKEY_TYPE_DIRSTRARRAY:
	id = kv->id;
	if (cbdata->owndirpool)
	  putinowndirpool(cbdata, data, &data->dirpool, id);
	else
	  setdirused(cbdata, &data->dirpool, id);
	break;
      case REPOKEY_TYPE_FIXARRAY:
	if (kv->eof == 0)
	  {
	    if (cbdata->oldschema)
	      {
		cbdata->target->error = pool_error(cbdata->repo->pool, -1, "nested fixarray structs not yet implemented");
		return SEARCH_NEXT_KEY;
	      }
	    cbdata->oldschema = cbdata->schema;
	    cbdata->oldsp = cbdata->sp;
	    cbdata->schema = solv_calloc(cbdata->target->nkeys, sizeof(Id));
	    cbdata->sp = cbdata->schema;
	  }
	else if (kv->eof == 1)
	  {
	    cbdata->current_sub++;
	    *cbdata->sp = 0;
	    cbdata->subschemata = solv_extend(cbdata->subschemata, cbdata->nsubschemata, 1, sizeof(Id), SCHEMATA_BLOCK);
	    cbdata->subschemata[cbdata->nsubschemata++] = repodata_schema2id(cbdata->target, cbdata->schema, 1);
#if 0
	    fprintf(stderr, "Have schema %d\n", cbdata->subschemata[cbdata->nsubschemata-1]);
#endif
	    cbdata->sp = cbdata->schema;
	  }
	else
	  {
	    solv_free(cbdata->schema);
	    cbdata->schema = cbdata->oldschema;
	    cbdata->sp = cbdata->oldsp;
	    cbdata->oldsp = cbdata->oldschema = 0;
	  }
	break;
      case REPOKEY_TYPE_FLEXARRAY:
	if (kv->entry == 0)
	  {
	    if (kv->eof != 2)
	      *cbdata->sp++ = 0;	/* mark start */
	  }
	else
	  {
	    /* just finished a schema, rewind */
	    Id *sp = cbdata->sp - 1;
	    *sp = 0;
	    while (sp[-1])
	      sp--;
	    cbdata->subschemata = solv_extend(cbdata->subschemata, cbdata->nsubschemata, 1, sizeof(Id), SCHEMATA_BLOCK);
	    cbdata->subschemata[cbdata->nsubschemata++] = repodata_schema2id(cbdata->target, sp, 1);
	    cbdata->sp = kv->eof == 2 ? sp - 1: sp;
	  }
	break;
      default:
	break;
    }
  return 0;
}

static int
repo_write_cb_needed(void *vcbdata, Solvable *s, Repodata *data, Repokey *key, KeyValue *kv)
{
  struct cbdata *cbdata = vcbdata;
  Repo *repo = data->repo;

#if 0
  if (s)
    fprintf(stderr, "solvable %d (%s): key (%d)%s %d\n", s ? s - repo->pool->solvables : 0, s ? pool_id2str(repo->pool, s->name) : "", key->name, pool_id2str(repo->pool, key->name), key->type);
#endif
  return repo_write_collect_needed(cbdata, repo, data, key, kv);
}


/*
 * pass 2 callback:
 * encode all of the data into the correct buffers
 */

static int
repo_write_adddata(struct cbdata *cbdata, Repodata *data, Repokey *key, KeyValue *kv)
{
  int rm;
  Id id;
  unsigned int u32;
  unsigned char v[4];
  struct extdata *xd;
  NeedId *needid;

  if (key->name == REPOSITORY_SOLVABLES)
    return SEARCH_NEXT_KEY;

  /* hack: ignore some keys, see BUGS */
  if (data->repodataid != data->repo->nrepodata - 1)
    if (key->name == REPOSITORY_ADDEDFILEPROVIDES || key->name == REPOSITORY_EXTERNAL || key->name == REPOSITORY_LOCATION || key->name == REPOSITORY_KEYS || key->name == REPOSITORY_TOOLVERSION)
      return SEARCH_NEXT_KEY;

  rm = cbdata->keymap[cbdata->keymapstart[data->repodataid] + (key - data->keys)];
  if (!rm)
    return SEARCH_NEXT_KEY;	/* we do not want this one */

  if (cbdata->target->keys[rm].storage == KEY_STORAGE_VERTICAL_OFFSET)
    {
      xd = cbdata->extdata + rm;	/* vertical buffer */
      if (cbdata->vstart == -1)
        cbdata->vstart = xd->len;
    }
  else
    xd = cbdata->extdata + 0;		/* incore buffer */
  switch(key->type)
    {
      case REPOKEY_TYPE_VOID:
      case REPOKEY_TYPE_CONSTANT:
      case REPOKEY_TYPE_CONSTANTID:
	break;
      case REPOKEY_TYPE_ID:
	id = kv->id;
	if (!ISRELDEP(id) && cbdata->ownspool && id > 1)
	  id = putinownpool(cbdata, data->localpool ? &data->spool : &data->repo->pool->ss, id);
	needid = cbdata->needid;
	id = needid[ISRELDEP(id) ? RELOFF(id) : id].need;
	data_addid(xd, id);
	break;
      case REPOKEY_TYPE_IDARRAY:
	id = kv->id;
	if (!ISRELDEP(id) && cbdata->ownspool && id > 1)
	  id = putinownpool(cbdata, data->localpool ? &data->spool : &data->repo->pool->ss, id);
	needid = cbdata->needid;
	id = needid[ISRELDEP(id) ? RELOFF(id) : id].need;
	data_addideof(xd, id, kv->eof);
	break;
      case REPOKEY_TYPE_STR:
	data_addblob(xd, (unsigned char *)kv->str, strlen(kv->str) + 1);
	break;
      case REPOKEY_TYPE_MD5:
	data_addblob(xd, (unsigned char *)kv->str, SIZEOF_MD5);
	break;
      case REPOKEY_TYPE_SHA1:
	data_addblob(xd, (unsigned char *)kv->str, SIZEOF_SHA1);
	break;
      case REPOKEY_TYPE_SHA256:
	data_addblob(xd, (unsigned char *)kv->str, SIZEOF_SHA256);
	break;
      case REPOKEY_TYPE_U32:
	u32 = kv->num;
	v[0] = u32 >> 24;
	v[1] = u32 >> 16;
	v[2] = u32 >> 8;
	v[3] = u32;
	data_addblob(xd, v, 4);
	break;
      case REPOKEY_TYPE_NUM:
	data_addid64(xd, kv->num, kv->num2);
	break;
      case REPOKEY_TYPE_DIR:
	id = kv->id;
	if (cbdata->owndirpool)
	  id = putinowndirpool(cbdata, data, &data->dirpool, id);
	id = cbdata->dirused[id];
	data_addid(xd, id);
	break;
      case REPOKEY_TYPE_BINARY:
	data_addid(xd, kv->num);
	if (kv->num)
	  data_addblob(xd, (unsigned char *)kv->str, kv->num);
	break;
      case REPOKEY_TYPE_DIRNUMNUMARRAY:
	id = kv->id;
	if (cbdata->owndirpool)
	  id = putinowndirpool(cbdata, data, &data->dirpool, id);
	id = cbdata->dirused[id];
	data_addid(xd, id);
	data_addid(xd, kv->num);
	data_addideof(xd, kv->num2, kv->eof);
	break;
      case REPOKEY_TYPE_DIRSTRARRAY:
	id = kv->id;
	if (cbdata->owndirpool)
	  id = putinowndirpool(cbdata, data, &data->dirpool, id);
	id = cbdata->dirused[id];
	data_addideof(xd, id, kv->eof);
	data_addblob(xd, (unsigned char *)kv->str, strlen(kv->str) + 1);
	break;
      case REPOKEY_TYPE_FIXARRAY:
	if (kv->eof == 0)
	  {
	    if (kv->num)
	      {
		data_addid(xd, kv->num);
		data_addid(xd, cbdata->subschemata[cbdata->current_sub]);
#if 0
		fprintf(stderr, "writing %d %d\n", kv->num, cbdata->subschemata[cbdata->current_sub]);
#endif
	      }
	  }
	else if (kv->eof == 1)
	  {
	    cbdata->current_sub++;
	  }
	break;
      case REPOKEY_TYPE_FLEXARRAY:
	if (!kv->entry)
	  data_addid(xd, kv->num);
	if (kv->eof != 2)
	  data_addid(xd, cbdata->subschemata[cbdata->current_sub++]);
	if (xd == cbdata->extdata + 0 && !kv->parent && !cbdata->doingsolvables)
	  {
	    if (xd->len - cbdata->lastlen > cbdata->maxdata)
	      cbdata->maxdata = xd->len - cbdata->lastlen;
	    cbdata->lastlen = xd->len;
	  }
	break;
      default:
	cbdata->target->error = pool_error(cbdata->repo->pool, -1, "unknown type for %d: %d\n", key->name, key->type);
	break;
    }
  if (cbdata->target->keys[rm].storage == KEY_STORAGE_VERTICAL_OFFSET && kv->eof)
    {
      /* we can re-use old data in the blob here! */
      data_addid(cbdata->extdata + 0, cbdata->vstart);			/* add offset into incore data */
      data_addid(cbdata->extdata + 0, xd->len - cbdata->vstart);	/* add length into incore data */
      cbdata->vstart = -1;
    }
  return 0;
}

static int
repo_write_cb_adddata(void *vcbdata, Solvable *s, Repodata *data, Repokey *key, KeyValue *kv)
{
  struct cbdata *cbdata = vcbdata;
  return repo_write_adddata(cbdata, data, key, kv);
}

/* traverse through directory with first child "dir" */
static int
traverse_dirs(Dirpool *dp, Id *dirmap, Id n, Id dir, Id *used)
{
  Id sib, child;
  Id parent, lastn;

  parent = n;
  /* special case for '/', which has to come first */
  if (parent == 1)
    dirmap[n++] = 1;
  for (sib = dir; sib; sib = dirpool_sibling(dp, sib))
    {
      if (used && !used[sib])
	continue;
      if (sib == 1 && parent == 1)
	continue;	/* already did that one above */
      dirmap[n++] = sib;
    }

  /* now go through all the siblings we just added and
   * do recursive calls on them */
  lastn = n;
  for (; parent < lastn; parent++)
    {
      sib = dirmap[parent];
      if (used && used[sib] != 2)	/* 2: used as parent */
	continue;
      child = dirpool_child(dp, sib);
      if (child)
	{
	  dirmap[n++] = -parent;	/* start new block */
	  n = traverse_dirs(dp, dirmap, n, child, used);
	}
    }
  return n;
}

static void
write_compressed_page(Repodata *data, unsigned char *page, int len)
{
  int clen;
  unsigned char cpage[REPOPAGE_BLOBSIZE];

  clen = repopagestore_compress_page(page, len, cpage, len - 1);
  if (!clen)
    {
      write_u32(data, len * 2);
      write_blob(data, page, len);
    }
  else
    {
      write_u32(data, clen * 2 + 1);
      write_blob(data, cpage, clen);
    }
}

static Id verticals[] = {
  SOLVABLE_AUTHORS,
  SOLVABLE_DESCRIPTION,
  SOLVABLE_MESSAGEDEL,
  SOLVABLE_MESSAGEINS,
  SOLVABLE_EULA,
  SOLVABLE_DISKUSAGE,
  SOLVABLE_FILELIST,
  SOLVABLE_CHECKSUM,
  DELTA_CHECKSUM,
  DELTA_SEQ_NUM,
  SOLVABLE_PKGID,
  SOLVABLE_HDRID,
  SOLVABLE_LEADSIGID,
  SOLVABLE_CHANGELOG_AUTHOR,
  SOLVABLE_CHANGELOG_TEXT,
  0
};

static char *languagetags[] = {
  "solvable:summary:",
  "solvable:description:",
  "solvable:messageins:",
  "solvable:messagedel:",
  "solvable:eula:",
  0
};

int
repo_write_stdkeyfilter(Repo *repo, Repokey *key, void *kfdata)
{
  const char *keyname;
  int i;

  for (i = 0; verticals[i]; i++)
    if (key->name == verticals[i])
      return KEY_STORAGE_VERTICAL_OFFSET;
  keyname = pool_id2str(repo->pool, key->name);
  for (i = 0; languagetags[i] != 0; i++)
    if (!strncmp(keyname, languagetags[i], strlen(languagetags[i])))
      return KEY_STORAGE_VERTICAL_OFFSET;
  return KEY_STORAGE_INCORE;
}

/*
 * Repo
 */

/*
 * the code works the following way:
 *
 * 1) find which keys should be written
 * 2) collect usage information for keys/ids/dirids, create schema
 *    data
 * 3) use usage information to create mapping tables, so that often
 *    used ids get a lower number
 * 4) encode data into buffers using the mapping tables
 * 5) write everything to disk
 */
int
repo_write_filtered(Repo *repo, FILE *fp, int (*keyfilter)(Repo *repo, Repokey *key, void *kfdata), void *kfdata, Queue *keyq)
{
  Pool *pool = repo->pool;
  int i, j, n;
  Solvable *s;
  NeedId *needid;
  int nstrings, nrels;
  unsigned int sizeid;
  unsigned int solv_flags;
  Reldep *ran;
  Id *idarraydata;

  Id id, *sp;

  Id *dirmap;
  int ndirmap;
  Id *keyused;
  unsigned char *repodataused;
  int anyrepodataused = 0;
  int anysolvableused = 0;

  struct cbdata cbdata;
  int clonepool;
  Repokey *key;
  int poolusage, dirpoolusage, idused, dirused;
  int reloff;

  Repodata *data, *dirpooldata;

  Repodata target;

  Stringpool *spool;
  Dirpool *dirpool;

  Id mainschema;

  struct extdata *xd;

  Id type_constantid = REPOKEY_TYPE_CONSTANTID;


  memset(&cbdata, 0, sizeof(cbdata));
  cbdata.repo = repo;
  cbdata.target = &target;

  repodata_initdata(&target, repo, 1);

  /* go through all repodata and find the keys we need */
  /* also unify keys */
  /*          keymapstart - maps repo number to keymap offset */
  /*          keymap      - maps repo key to my key, 0 -> not used */

  /* start with all KEY_STORAGE_SOLVABLE ids */

  n = ID_NUM_INTERNAL;
  FOR_REPODATAS(repo, i, data)
    n += data->nkeys;
  cbdata.keymap = solv_calloc(n, sizeof(Id));
  cbdata.keymapstart = solv_calloc(repo->nrepodata, sizeof(Id));
  repodataused = solv_calloc(repo->nrepodata, 1);

  clonepool = 0;
  poolusage = 0;

  /* add keys for STORAGE_SOLVABLE */
  for (i = SOLVABLE_NAME; i <= RPM_RPMDBID; i++)
    {
      Repokey keyd;
      keyd.name = i;
      if (i < SOLVABLE_PROVIDES)
        keyd.type = REPOKEY_TYPE_ID;
      else if (i < RPM_RPMDBID)
        keyd.type = REPOKEY_TYPE_REL_IDARRAY;
      else
        keyd.type = REPOKEY_TYPE_NUM;
      keyd.size = 0;
      keyd.storage = KEY_STORAGE_SOLVABLE;
      if (keyfilter)
	{
	  keyd.storage = keyfilter(repo, &keyd, kfdata);
	  if (keyd.storage == KEY_STORAGE_DROPPED)
	    continue;
	  keyd.storage = KEY_STORAGE_SOLVABLE;
	}
      poolusage = 1;
      clonepool = 1;
      cbdata.keymap[keyd.name] = repodata_key2id(&target, &keyd, 1);
    }

  if (repo->nsolvables)
    {
      Repokey keyd;
      keyd.name = REPOSITORY_SOLVABLES;
      keyd.type = REPOKEY_TYPE_FLEXARRAY;
      keyd.size = 0;
      keyd.storage = KEY_STORAGE_INCORE;
      cbdata.keymap[keyd.name] = repodata_key2id(&target, &keyd, 1);
    }

  dirpoolusage = 0;

  spool = 0;
  dirpool = 0;
  dirpooldata = 0;
  n = ID_NUM_INTERNAL;
  FOR_REPODATAS(repo, i, data)
    {
      cbdata.keymapstart[i] = n;
      cbdata.keymap[n++] = 0;	/* key 0 */
      idused = 0;
      dirused = 0;
      if (keyfilter)
	{
	  Repokey keyd;
	  /* check if we want this repodata */
	  memset(&keyd, 0, sizeof(keyd));
	  keyd.name = 1;
	  keyd.type = 1;
	  keyd.size = i;
	  if (keyfilter(repo, &keyd, kfdata) == -1)
	    continue;
	}
      for (j = 1; j < data->nkeys; j++, n++)
	{
	  key = data->keys + j;
	  if (key->name == REPOSITORY_SOLVABLES && key->type == REPOKEY_TYPE_FLEXARRAY)
	    {
	      cbdata.keymap[n] = cbdata.keymap[key->name];
	      continue;
	    }
	  if (key->type == REPOKEY_TYPE_DELETED)
	    {
	      cbdata.keymap[n] = 0;
	      continue;
	    }
	  if (key->type == REPOKEY_TYPE_CONSTANTID && data->localpool)
	    {
	      Repokey keyd = *key;
	      keyd.size = repodata_globalize_id(data, key->size, 1);
	      id = repodata_key2id(&target, &keyd, 0);
	    }
	  else
	    id = repodata_key2id(&target, key, 0);
	  if (!id)
	    {
	      Repokey keyd = *key;
	      keyd.storage = KEY_STORAGE_INCORE;
	      if (keyd.type == REPOKEY_TYPE_CONSTANTID)
		keyd.size = repodata_globalize_id(data, key->size, 1);
	      else if (keyd.type != REPOKEY_TYPE_CONSTANT)
		keyd.size = 0;
	      if (keyfilter)
		{
		  keyd.storage = keyfilter(repo, &keyd, kfdata);
		  if (keyd.storage == KEY_STORAGE_DROPPED)
		    {
		      cbdata.keymap[n] = 0;
		      continue;
		    }
		}
	      id = repodata_key2id(&target, &keyd, 1);
	    }
	  cbdata.keymap[n] = id;
	  /* load repodata if not already loaded */
	  if (data->state == REPODATA_STUB)
	    {
	      if (data->loadcallback)
		data->loadcallback(data);
	      else
		data->state = REPODATA_ERROR;
	      if (data->state != REPODATA_ERROR)
		{
		  /* redo this repodata! */
		  j = 0;
		  n = cbdata.keymapstart[i];
		  continue;
		}
	    }
	  if (data->state == REPODATA_ERROR)
	    {
	      /* too bad! */
	      cbdata.keymap[n] = 0;
	      continue;
	    }

	  repodataused[i] = 1;
	  anyrepodataused = 1;
	  if (key->type == REPOKEY_TYPE_CONSTANTID || key->type == REPOKEY_TYPE_ID ||
              key->type == REPOKEY_TYPE_IDARRAY || key->type == REPOKEY_TYPE_REL_IDARRAY)
	    idused = 1;
	  else if (key->type == REPOKEY_TYPE_DIR || key->type == REPOKEY_TYPE_DIRNUMNUMARRAY || key->type == REPOKEY_TYPE_DIRSTRARRAY)
	    {
	      idused = 1;	/* dirs also use ids */
	      dirused = 1;
	    }
	}
      if (idused)
	{
	  if (data->localpool)
	    {
	      if (poolusage)
		poolusage = 3;	/* need own pool */
	      else
		{
		  poolusage = 2;
		  spool = &data->spool;
		}
	    }
	  else
	    {
	      if (poolusage == 0)
		poolusage = 1;
	      else if (poolusage != 1)
		poolusage = 3;	/* need own pool */
	    }
	}
      if (dirused)
	{
	  if (dirpoolusage)
	    dirpoolusage = 3;	/* need own dirpool */
	  else
	    {
	      dirpoolusage = 2;
	      dirpool = &data->dirpool;
	      dirpooldata = data;
	    }
	}
    }
  cbdata.nkeymap = n;

  /* 0: no pool needed at all */
  /* 1: use global pool */
  /* 2: use repodata local pool */
  /* 3: need own pool */
  if (poolusage == 3)
    {
      spool = &target.spool;
      /* hack: reuse global pool data so we don't have to map pool ids */
      if (clonepool)
	{
	  stringpool_free(spool);
	  stringpool_clone(spool, &pool->ss);
	}
      cbdata.ownspool = spool;
    }
  else if (poolusage == 0 || poolusage == 1)
    {
      poolusage = 1;
      spool = &pool->ss;
    }

  if (dirpoolusage == 3)
    {
      dirpool = &target.dirpool;
      dirpooldata = 0;
      cbdata.owndirpool = dirpool;
    }
  else if (dirpool)
    cbdata.dirused = solv_calloc(dirpool->ndirs, sizeof(Id));


/********************************************************************/
#if 0
fprintf(stderr, "poolusage: %d\n", poolusage);
fprintf(stderr, "dirpoolusage: %d\n", dirpoolusage);
fprintf(stderr, "nkeys: %d\n", target.nkeys);
for (i = 1; i < target.nkeys; i++)
  fprintf(stderr, "  %2d: %s[%d] %d %d %d\n", i, pool_id2str(pool, target.keys[i].name), target.keys[i].name, target.keys[i].type, target.keys[i].size, target.keys[i].storage);
#endif

  /* copy keys if requested */
  if (keyq)
    {
      queue_empty(keyq);
      for (i = 1; i < target.nkeys; i++)
	queue_push2(keyq, target.keys[i].name, target.keys[i].type);
    }

  if (poolusage > 1)
    {
      /* put all the keys we need in our string pool */
      /* put mapped ids right into target.keys */
      for (i = 1, key = target.keys + i; i < target.nkeys; i++, key++)
	{
	  key->name = stringpool_str2id(spool, pool_id2str(pool, key->name), 1);
	  if (key->type == REPOKEY_TYPE_CONSTANTID)
	    {
	      key->type = stringpool_str2id(spool, pool_id2str(pool, key->type), 1);
	      type_constantid = key->type;
	      key->size = stringpool_str2id(spool, pool_id2str(pool, key->size), 1);
	    }
	  else
	    key->type = stringpool_str2id(spool, pool_id2str(pool, key->type), 1);
	}
      if (poolusage == 2)
	stringpool_freehash(spool);	/* free some mem */
    }


/********************************************************************/

  /* set needed count of all strings and rels,
   * find which keys are used in the solvables
   * put all strings in own spool
   */

  reloff = spool->nstrings;
  if (poolusage == 3)
    reloff = (reloff + NEEDED_BLOCK) & ~NEEDED_BLOCK;

  needid = calloc(reloff + pool->nrels, sizeof(*needid));
  needid[0].map = reloff;

  cbdata.needid = needid;
  cbdata.schema = solv_calloc(target.nkeys, sizeof(Id));
  cbdata.sp = cbdata.schema;
  cbdata.solvschemata = solv_calloc(repo->nsolvables, sizeof(Id));

  /* create main schema */
  cbdata.sp = cbdata.schema;
  /* collect all other data from all repodatas */
  /* XXX: merge arrays of equal keys? */
  FOR_REPODATAS(repo, j, data)
    {
      if (!repodataused[j])
	continue;
      repodata_search(data, SOLVID_META, 0, SEARCH_SUB|SEARCH_ARRAYSENTINEL, repo_write_cb_needed, &cbdata);
    }
  sp = cbdata.sp;
  /* add solvables if needed (may revert later) */
  if (repo->nsolvables)
    {
      *sp++ = cbdata.keymap[REPOSITORY_SOLVABLES];
      target.keys[cbdata.keymap[REPOSITORY_SOLVABLES]].size++;
    }
  *sp = 0;
  mainschema = repodata_schema2id(cbdata.target, cbdata.schema, 1);

  idarraydata = repo->idarraydata;

  anysolvableused = 0;
  cbdata.doingsolvables = 1;
  for (i = repo->start, s = pool->solvables + i, n = 0; i < repo->end; i++, s++)
    {
      if (s->repo != repo)
	continue;

      /* set schema info, keep in sync with further down */
      sp = cbdata.schema;
      if (cbdata.keymap[SOLVABLE_NAME])
	{
          *sp++ = cbdata.keymap[SOLVABLE_NAME];
	  needid[s->name].need++;
	}
      if (cbdata.keymap[SOLVABLE_ARCH])
	{
          *sp++ = cbdata.keymap[SOLVABLE_ARCH];
	  needid[s->arch].need++;
	}
      if (cbdata.keymap[SOLVABLE_EVR])
	{
          *sp++ = cbdata.keymap[SOLVABLE_EVR];
	  needid[s->evr].need++;
	}
      if (s->vendor && cbdata.keymap[SOLVABLE_VENDOR])
	{
          *sp++ = cbdata.keymap[SOLVABLE_VENDOR];
	  needid[s->vendor].need++;
	}
      if (s->provides && cbdata.keymap[SOLVABLE_PROVIDES])
        {
          *sp++ = cbdata.keymap[SOLVABLE_PROVIDES];
	  target.keys[cbdata.keymap[SOLVABLE_PROVIDES]].size += incneedidarray(pool, idarraydata + s->provides, needid);
	}
      if (s->obsoletes && cbdata.keymap[SOLVABLE_OBSOLETES])
	{
          *sp++ = cbdata.keymap[SOLVABLE_OBSOLETES];
	  target.keys[cbdata.keymap[SOLVABLE_OBSOLETES]].size += incneedidarray(pool, idarraydata + s->obsoletes, needid);
	}
      if (s->conflicts && cbdata.keymap[SOLVABLE_CONFLICTS])
	{
          *sp++ = cbdata.keymap[SOLVABLE_CONFLICTS];
	  target.keys[cbdata.keymap[SOLVABLE_CONFLICTS]].size += incneedidarray(pool, idarraydata + s->conflicts, needid);
	}
      if (s->requires && cbdata.keymap[SOLVABLE_REQUIRES])
	{
          *sp++ = cbdata.keymap[SOLVABLE_REQUIRES];
	  target.keys[cbdata.keymap[SOLVABLE_REQUIRES]].size += incneedidarray(pool, idarraydata + s->requires, needid);
	}
      if (s->recommends && cbdata.keymap[SOLVABLE_RECOMMENDS])
	{
          *sp++ = cbdata.keymap[SOLVABLE_RECOMMENDS];
	  target.keys[cbdata.keymap[SOLVABLE_RECOMMENDS]].size += incneedidarray(pool, idarraydata + s->recommends, needid);
	}
      if (s->suggests && cbdata.keymap[SOLVABLE_SUGGESTS])
	{
          *sp++ = cbdata.keymap[SOLVABLE_SUGGESTS];
	  target.keys[cbdata.keymap[SOLVABLE_SUGGESTS]].size += incneedidarray(pool, idarraydata + s->suggests, needid);
	}
      if (s->supplements && cbdata.keymap[SOLVABLE_SUPPLEMENTS])
	{
          *sp++ = cbdata.keymap[SOLVABLE_SUPPLEMENTS];
	  target.keys[cbdata.keymap[SOLVABLE_SUPPLEMENTS]].size += incneedidarray(pool, idarraydata + s->supplements, needid);
	}
      if (s->enhances && cbdata.keymap[SOLVABLE_ENHANCES])
	{
          *sp++ = cbdata.keymap[SOLVABLE_ENHANCES];
	  target.keys[cbdata.keymap[SOLVABLE_ENHANCES]].size += incneedidarray(pool, idarraydata + s->enhances, needid);
	}
      if (repo->rpmdbid && cbdata.keymap[RPM_RPMDBID])
	{
          *sp++ = cbdata.keymap[RPM_RPMDBID];
	  target.keys[cbdata.keymap[RPM_RPMDBID]].size++;
	}
      cbdata.sp = sp;

      if (anyrepodataused)
	{
	  FOR_REPODATAS(repo, j, data)
	    {
	      if (!repodataused[j])
		continue;
	      if (i < data->start || i >= data->end)
		continue;
	      repodata_search(data, i, 0, SEARCH_SUB|SEARCH_ARRAYSENTINEL, repo_write_cb_needed, &cbdata);
	      needid = cbdata.needid;
	    }
	}
      *cbdata.sp = 0;
      cbdata.solvschemata[n] = repodata_schema2id(cbdata.target, cbdata.schema, 1);
      if (cbdata.solvschemata[n])
	anysolvableused = 1;
      n++;
    }
  cbdata.doingsolvables = 0;
  assert(n == repo->nsolvables);

  if (repo->nsolvables && !anysolvableused)
    {
      /* strip off solvable from the main schema */
      target.keys[cbdata.keymap[REPOSITORY_SOLVABLES]].size = 0;
      sp = cbdata.schema;
      for (i = 0; target.schemadata[target.schemata[mainschema] + i]; i++)
	{
	  *sp = target.schemadata[target.schemata[mainschema] + i];
	  if (*sp != cbdata.keymap[REPOSITORY_SOLVABLES])
	    sp++;
	}
      assert(target.schemadatalen == target.schemata[mainschema] + i + 1);
      *sp = 0;
      target.schemadatalen = target.schemata[mainschema];
      target.nschemata--;
      repodata_free_schemahash(&target);
      mainschema = repodata_schema2id(cbdata.target, cbdata.schema, 1);
    }

/********************************************************************/

  /* remove unused keys */
  keyused = solv_calloc(target.nkeys, sizeof(Id));
  for (i = 1; i < target.schemadatalen; i++)
    keyused[target.schemadata[i]] = 1;
  keyused[0] = 0;
  for (n = i = 1; i < target.nkeys; i++)
    {
      if (!keyused[i])
	continue;
      keyused[i] = n;
      if (i != n)
	{
	  target.keys[n] = target.keys[i];
	  if (keyq)
	    {
	      keyq->elements[2 * n - 2] = keyq->elements[2 * i - 2];
	      keyq->elements[2 * n - 1] = keyq->elements[2 * i - 1];
	    }
	}
      n++;
    }
  target.nkeys = n;
  if (keyq)
    queue_truncate(keyq, 2 * n - 2);

  /* update schema data to the new key ids */
  for (i = 1; i < target.schemadatalen; i++)
    target.schemadata[i] = keyused[target.schemadata[i]];
  /* update keymap to the new key ids */
  for (i = 0; i < cbdata.nkeymap; i++)
    cbdata.keymap[i] = keyused[cbdata.keymap[i]];
  keyused = solv_free(keyused);

  /* increment needid of the used keys, they are already mapped to
   * the correct string pool  */
  for (i = 1; i < target.nkeys; i++)
    {
      if (target.keys[i].type == type_constantid)
	needid[target.keys[i].size].need++;
      needid[target.keys[i].name].need++;
      needid[target.keys[i].type].need++;
    }

/********************************************************************/

  if (dirpool && cbdata.dirused && !cbdata.dirused[0])
    {
      /* no dirs used at all */
      cbdata.dirused = solv_free(cbdata.dirused);
      dirpool = 0;
    }

  /* increment need id for used dir components */
  if (dirpool)
    {
      /* if we have own dirpool, all entries in it are used.
	 also, all comp ids are already mapped by putinowndirpool(),
	 so we can simply increment needid.
	 (owndirpool != 0, dirused == 0, dirpooldata == 0) */
      /* else we re-use a dirpool of repodata "dirpooldata".
	 dirused tells us which of the ids are used.
	 we need to map comp ids if we generate a new pool.
	 (owndirpool == 0, dirused != 0, dirpooldata != 0) */
      for (i = 1; i < dirpool->ndirs; i++)
	{
#if 0
fprintf(stderr, "dir %d used %d\n", i, cbdata.dirused ? cbdata.dirused[i] : 1);
#endif
	  if (cbdata.dirused && !cbdata.dirused[i])
	    continue;
	  id = dirpool->dirs[i];
	  if (id <= 0)
	    continue;
	  if (dirpooldata && cbdata.ownspool && id > 1)
	    {
	      id = putinownpool(&cbdata, dirpooldata->localpool ? &dirpooldata->spool : &pool->ss, id);
	      needid = cbdata.needid;
	    }
	  needid[id].need++;
	}
    }


/********************************************************************/

  /*
   * create mapping table, new keys are sorted by needid[].need
   *
   * needid[key].need : old key -> new key
   * needid[key].map  : new key -> old key
   */

  /* zero out id 0 and rel 0 just in case */
  reloff = needid[0].map;
  needid[0].need = 0;
  needid[reloff].need = 0;

  for (i = 1; i < reloff + pool->nrels; i++)
    needid[i].map = i;

#if 0
  solv_sort(needid + 1, spool->nstrings - 1, sizeof(*needid), needid_cmp_need_s, spool);
#else
  /* make first entry '' */
  needid[1].need = 1;
  solv_sort(needid + 2, spool->nstrings - 2, sizeof(*needid), needid_cmp_need_s, spool);
#endif
  solv_sort(needid + reloff, pool->nrels, sizeof(*needid), needid_cmp_need, 0);
  /* now needid is in new order, needid[newid].map -> oldid */

  /* calculate string space size, also zero out needid[].need */
  sizeid = 0;
  for (i = 1; i < reloff; i++)
    {
      if (!needid[i].need)
        break;	/* as we have sorted, every entry after this also has need == 0 */
      needid[i].need = 0;
      sizeid += strlen(spool->stringspace + spool->strings[needid[i].map]) + 1;
    }
  nstrings = i;	/* our new string id end */

  /* make needid[oldid].need point to newid */
  for (i = 1; i < nstrings; i++)
    needid[needid[i].map].need = i;

  /* same as above for relations */
  for (i = 0; i < pool->nrels; i++)
    {
      if (!needid[reloff + i].need)
        break;
      needid[reloff + i].need = 0;
    }
  nrels = i;	/* our new rel id end */

  for (i = 0; i < nrels; i++)
    needid[needid[reloff + i].map].need = nstrings + i;

  /* now we have: needid[oldid].need -> newid
                  needid[newid].map  -> oldid
     both for strings and relations  */


/********************************************************************/

  ndirmap = 0;
  dirmap = 0;
  if (dirpool)
    {
      /* create our new target directory structure by traversing through all
       * used dirs. This will concatenate blocks with the same parent
       * directory into single blocks.
       * Instead of components, traverse_dirs stores the old dirids,
       * we will change this in the second step below */
      /* (dirpooldata and dirused are 0 if we have our own dirpool) */
      if (cbdata.dirused && !cbdata.dirused[1])
	cbdata.dirused[1] = 1;	/* always want / entry */
      dirmap = solv_calloc(dirpool->ndirs, sizeof(Id));
      dirmap[0] = 0;
      ndirmap = traverse_dirs(dirpool, dirmap, 1, dirpool_child(dirpool, 0), cbdata.dirused);

      /* (re)create dirused, so that it maps from "old dirid" to "new dirid" */
      /* change dirmap so that it maps from "new dirid" to "new compid" */
      if (!cbdata.dirused)
	cbdata.dirused = solv_malloc2(dirpool->ndirs, sizeof(Id));
      memset(cbdata.dirused, 0, dirpool->ndirs * sizeof(Id));
      for (i = 1; i < ndirmap; i++)
	{
	  if (dirmap[i] <= 0)
	    continue;
	  cbdata.dirused[dirmap[i]] = i;
	  id = dirpool->dirs[dirmap[i]];
	  if (dirpooldata && cbdata.ownspool && id > 1)
	    id = putinownpool(&cbdata, dirpooldata->localpool ? &dirpooldata->spool : &pool->ss, id);
	  dirmap[i] = needid[id].need;
	}
      /* now the new target directory structure is complete (dirmap), and we have
       * dirused[olddirid] -> newdirid */
    }

/********************************************************************/

  /* collect all data
   * we use extdata[0] for incore data and extdata[keyid] for vertical data
   */

  cbdata.extdata = solv_calloc(target.nkeys, sizeof(struct extdata));

  xd = cbdata.extdata;
  cbdata.current_sub = 0;
  /* add main schema */
  cbdata.lastlen = 0;
  data_addid(xd, mainschema);

#if 1
  FOR_REPODATAS(repo, j, data)
    {
      if (!repodataused[j])
	continue;
      repodata_search(data, SOLVID_META, 0, SEARCH_SUB|SEARCH_ARRAYSENTINEL, repo_write_cb_adddata, &cbdata);
    }
#endif

  if (xd->len - cbdata.lastlen > cbdata.maxdata)
    cbdata.maxdata = xd->len - cbdata.lastlen;
  cbdata.lastlen = xd->len;

  if (anysolvableused)
    {
      data_addid(xd, repo->nsolvables);	/* FLEXARRAY nentries */
      cbdata.doingsolvables = 1;
      for (i = repo->start, s = pool->solvables + i, n = 0; i < repo->end; i++, s++)
	{
	  if (s->repo != repo)
	    continue;
	  data_addid(xd, cbdata.solvschemata[n]);
	  if (cbdata.keymap[SOLVABLE_NAME])
	    data_addid(xd, needid[s->name].need);
	  if (cbdata.keymap[SOLVABLE_ARCH])
	    data_addid(xd, needid[s->arch].need);
	  if (cbdata.keymap[SOLVABLE_EVR])
	    data_addid(xd, needid[s->evr].need);
	  if (s->vendor && cbdata.keymap[SOLVABLE_VENDOR])
	    data_addid(xd, needid[s->vendor].need);
	  if (s->provides && cbdata.keymap[SOLVABLE_PROVIDES])
	    data_addidarray_sort(xd, pool, needid, idarraydata + s->provides, SOLVABLE_FILEMARKER);
	  if (s->obsoletes && cbdata.keymap[SOLVABLE_OBSOLETES])
	    data_addidarray_sort(xd, pool, needid, idarraydata + s->obsoletes, 0);
	  if (s->conflicts && cbdata.keymap[SOLVABLE_CONFLICTS])
	    data_addidarray_sort(xd, pool, needid, idarraydata + s->conflicts, 0);
	  if (s->requires && cbdata.keymap[SOLVABLE_REQUIRES])
	    data_addidarray_sort(xd, pool, needid, idarraydata + s->requires, SOLVABLE_PREREQMARKER);
	  if (s->recommends && cbdata.keymap[SOLVABLE_RECOMMENDS])
	    data_addidarray_sort(xd, pool, needid, idarraydata + s->recommends, 0);
	  if (s->suggests && cbdata.keymap[SOLVABLE_SUGGESTS])
	    data_addidarray_sort(xd, pool, needid, idarraydata + s->suggests, 0);
	  if (s->supplements && cbdata.keymap[SOLVABLE_SUPPLEMENTS])
	    data_addidarray_sort(xd, pool, needid, idarraydata + s->supplements, 0);
	  if (s->enhances && cbdata.keymap[SOLVABLE_ENHANCES])
	    data_addidarray_sort(xd, pool, needid, idarraydata + s->enhances, 0);
	  if (repo->rpmdbid && cbdata.keymap[RPM_RPMDBID])
	    data_addid(xd, repo->rpmdbid[i - repo->start]);
	  if (anyrepodataused)
	    {
	      cbdata.vstart = -1;
	      FOR_REPODATAS(repo, j, data)
		{
		  if (!repodataused[j])
		    continue;
		  if (i < data->start || i >= data->end)
		    continue;
		  repodata_search(data, i, 0, SEARCH_SUB|SEARCH_ARRAYSENTINEL, repo_write_cb_adddata, &cbdata);
		}
	    }
	  if (xd->len - cbdata.lastlen > cbdata.maxdata)
	    cbdata.maxdata = xd->len - cbdata.lastlen;
	  cbdata.lastlen = xd->len;
	  n++;
	}
      cbdata.doingsolvables = 0;
    }

  assert(cbdata.current_sub == cbdata.nsubschemata);
  if (cbdata.subschemata)
    {
      cbdata.subschemata = solv_free(cbdata.subschemata);
      cbdata.nsubschemata = 0;
    }

/********************************************************************/

  target.fp = fp;

  /* write header */

  /* write file header */
  write_u32(&target, 'S' << 24 | 'O' << 16 | 'L' << 8 | 'V');
  write_u32(&target, SOLV_VERSION_8);


  /* write counts */
  write_u32(&target, nstrings);
  write_u32(&target, nrels);
  write_u32(&target, ndirmap);
  write_u32(&target, anysolvableused ? repo->nsolvables : 0);
  write_u32(&target, target.nkeys);
  write_u32(&target, target.nschemata);
  solv_flags = 0;
  solv_flags |= SOLV_FLAG_PREFIX_POOL;
  solv_flags |= SOLV_FLAG_SIZE_BYTES;
  write_u32(&target, solv_flags);

  if (nstrings)
    {
      /*
       * calculate prefix encoding of the strings
       */
      unsigned char *prefixcomp = solv_malloc(nstrings);
      unsigned int compsum = 0;
      char *old_str = "";

      prefixcomp[0] = 0;
      for (i = 1; i < nstrings; i++)
	{
	  char *str = spool->stringspace + spool->strings[needid[i].map];
	  int same;
	  for (same = 0; same < 255; same++)
	    if (!old_str[same] || old_str[same] != str[same])
	      break;
	  prefixcomp[i] = same;
	  compsum += same;
	  old_str = str;
	}

      /*
       * write strings
       */
      write_u32(&target, sizeid);
      /* we save compsum bytes but need 1 extra byte for every string */
      write_u32(&target, sizeid + nstrings - 1 - compsum);
      for (i = 1; i < nstrings; i++)
	{
	  char *str = spool->stringspace + spool->strings[needid[i].map];
	  write_u8(&target, prefixcomp[i]);
	  write_str(&target, str + prefixcomp[i]);
	}
      solv_free(prefixcomp);
    }
  else
    {
      write_u32(&target, 0);
      write_u32(&target, 0);
    }

  /*
   * write RelDeps
   */
  for (i = 0; i < nrels; i++)
    {
      ran = pool->rels + (needid[reloff + i].map - reloff);
      write_id(&target, needid[ISRELDEP(ran->name) ? RELOFF(ran->name) : ran->name].need);
      write_id(&target, needid[ISRELDEP(ran->evr) ? RELOFF(ran->evr) : ran->evr].need);
      write_u8(&target, ran->flags);
    }

  /*
   * write dirs (skip both root and / entry)
   */
  for (i = 2; i < ndirmap; i++)
    {
      if (dirmap[i] > 0)
        write_id(&target, dirmap[i]);
      else
        write_id(&target, nstrings - dirmap[i]);
    }
  solv_free(dirmap);

  /*
   * write keys
   */
  for (i = 1; i < target.nkeys; i++)
    {
      write_id(&target, needid[target.keys[i].name].need);
      write_id(&target, needid[target.keys[i].type].need);
      if (target.keys[i].storage != KEY_STORAGE_VERTICAL_OFFSET)
	{
	  if (target.keys[i].type == type_constantid)
            write_id(&target, needid[target.keys[i].size].need);
	  else
            write_id(&target, target.keys[i].size);
	}
      else
        write_id(&target, cbdata.extdata[i].len);
      write_id(&target, target.keys[i].storage);
    }

  /*
   * write schemata
   */
  write_id(&target, target.schemadatalen);	/* XXX -1? */
  for (i = 1; i < target.nschemata; i++)
    write_idarray(&target, pool, 0, repodata_id2schema(&target, i));

/********************************************************************/

  write_id(&target, cbdata.maxdata);
  write_id(&target, cbdata.extdata[0].len);
  if (cbdata.extdata[0].len)
    write_blob(&target, cbdata.extdata[0].buf, cbdata.extdata[0].len);
  solv_free(cbdata.extdata[0].buf);

  /* do we have vertical data? */
  for (i = 1; i < target.nkeys; i++)
    if (cbdata.extdata[i].len)
      break;
  if (i < target.nkeys)
    {
      /* yes, write it in pages */
      unsigned char *dp, vpage[REPOPAGE_BLOBSIZE];
      int l, ll, lpage = 0;

      write_u32(&target, REPOPAGE_BLOBSIZE);
      for (i = 1; i < target.nkeys; i++)
	{
	  if (!cbdata.extdata[i].len)
	    continue;
	  l = cbdata.extdata[i].len;
	  dp = cbdata.extdata[i].buf;
	  while (l)
	    {
	      ll = REPOPAGE_BLOBSIZE - lpage;
	      if (l < ll)
		ll = l;
	      memcpy(vpage + lpage, dp, ll);
	      dp += ll;
	      lpage += ll;
	      l -= ll;
	      if (lpage == REPOPAGE_BLOBSIZE)
		{
		  write_compressed_page(&target, vpage, lpage);
		  lpage = 0;
		}
	    }
	}
      if (lpage)
	write_compressed_page(&target, vpage, lpage);
    }

  for (i = 1; i < target.nkeys; i++)
    solv_free(cbdata.extdata[i].buf);
  solv_free(cbdata.extdata);

  target.fp = 0;
  repodata_freedata(&target);

  solv_free(needid);
  solv_free(cbdata.solvschemata);
  solv_free(cbdata.schema);

  solv_free(cbdata.keymap);
  solv_free(cbdata.keymapstart);
  solv_free(cbdata.dirused);
  solv_free(repodataused);
  return target.error;
}

struct repodata_write_data {
  int (*keyfilter)(Repo *repo, Repokey *key, void *kfdata);
  void *kfdata;
  int repodataid;
};

static int
repodata_write_keyfilter(Repo *repo, Repokey *key, void *kfdata)
{
  struct repodata_write_data *wd = kfdata;

  /* XXX: special repodata selection hack */
  if (key->name == 1 && key->size != wd->repodataid)
    return -1;
  if (key->storage == KEY_STORAGE_SOLVABLE)
    return KEY_STORAGE_DROPPED;	/* not part of this repodata */
  if (wd->keyfilter)
    return (*wd->keyfilter)(repo, key, wd->kfdata);
  return key->storage;
}

int
repodata_write_filtered(Repodata *data, FILE *fp, int (*keyfilter)(Repo *repo, Repokey *key, void *kfdata), void *kfdata, Queue *keyq)
{
  struct repodata_write_data wd;

  wd.keyfilter = keyfilter;
  wd.kfdata = kfdata;
  wd.repodataid = data->repodataid;
  return repo_write_filtered(data->repo, fp, repodata_write_keyfilter, &wd, keyq);
}

int
repodata_write(Repodata *data, FILE *fp)
{
  return repodata_write_filtered(data, fp, repo_write_stdkeyfilter, 0, 0);
}

int
repo_write(Repo *repo, FILE *fp)
{
  return repo_write_filtered(repo, fp, repo_write_stdkeyfilter, 0, 0);
}
