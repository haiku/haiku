/* Copyright 1994 NEC Corporation, Tokyo, Japan.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of NEC
 * Corporation not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission.  NEC Corporation makes no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * NEC CORPORATION DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN 
 * NO EVENT SHALL NEC CORPORATION BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF 
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR 
 * OTHER TORTUOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR 
 * PERFORMANCE OF THIS SOFTWARE. 
 */

/************************************************************************/
/* THIS SOURCE CODE IS MODIFIED FOR TKO BY T.MURAI 1997
/************************************************************************/


/*
static char rcsid[]="$Id: ncache.c 10525 2004-12-23 21:23:50Z korli $";
*/
#include "canna.h"
#include "RK.h"
#include "RKintern.h"

#define	NCHASH		101UL
#define	hash(x)		((int)(((ulong)x)%NCHASH))

static struct ncache	Nchash[NCHASH];
static struct ncache	Ncfree;

inline void
ainserttop(struct ncache *p)
{ 
	p->nc_anext = Ncfree.nc_anext; 
	p->nc_aprev = &Ncfree;
	Ncfree.nc_anext->nc_aprev = p;
	Ncfree.nc_anext = p;
}

inline void
ainsertbottom(struct ncache *p)
{
	p->nc_anext = &Ncfree;
	p->nc_aprev = Ncfree.nc_aprev;
	Ncfree.nc_aprev->nc_anext = p;
	Ncfree.nc_aprev = p;
}

inline void
aremove(struct ncache *p)
{
	p->nc_anext->nc_aprev = p->nc_aprev;
	p->nc_aprev->nc_anext = p->nc_anext;
	p->nc_anext = p->nc_aprev = p;
}

inline void
hremove(struct ncache *p)	
{
	p->nc_hnext->nc_hprev = p->nc_hprev;
	p->nc_hprev->nc_hnext = p->nc_hnext;
	p->nc_hnext = p->nc_hprev = p;
}

static int flushCache(struct DM *dm, struct ncache *cache);

int	
_RkInitializeCache(int size)
{
  struct RkParam	*sx = &SX;
  int				i;

  sx->maxcache = size;
  if (!(sx->cache = (struct ncache *)calloc((unsigned)size, sizeof(struct ncache))))
    return -1;
  for (i = 0; i < size ; i++) {
    sx->cache[i].nc_anext = &sx->cache[i+1];
    sx->cache[i].nc_aprev = &sx->cache[i-1];
    sx->cache[i].nc_hnext = sx->cache[i].nc_hprev = &sx->cache[i];
    sx->cache[i].nc_count = 0;
  };
  Ncfree.nc_anext = &sx->cache[0];
  sx->cache[sx->maxcache - 1].nc_anext = &Ncfree;
  Ncfree.nc_aprev = &sx->cache[sx->maxcache - 1];
  sx->cache[0].nc_aprev = &Ncfree;
  for (i = 0; i < NCHASH; i++) 
    Nchash[i].nc_hnext = Nchash[i].nc_hprev = &Nchash[i];
  return 0;
}

void
_RkFinalizeCache(void)
{
  struct RkParam	*sx = &SX;
  
  if (sx->cache) 
    free(sx->cache);
  sx->cache = (struct ncache *)0;
}

static int
flushCache(struct DM *dm, struct ncache *cache)
{
  if (cache->nc_word) {
    if (dm && (cache->nc_flags & NC_DIRTY)) {
      DST_WRITE(dm, cache);
    };
    cache->nc_flags &= ~NC_DIRTY;
    return 0;
  };
  return -1;
}

inline struct ncache
*newCache(struct DM *ndm, long address)
{
  struct ncache	*newc;

  if ((newc = Ncfree.nc_anext) != &Ncfree) {
    (void)flushCache(newc->nc_dic, newc);
    aremove(newc);
    hremove(newc);
    newc->nc_dic = ndm;
    newc->nc_word = (unsigned char *)NULL;
    newc->nc_flags  = 0;
    newc->nc_address = address;
    newc->nc_count = 0;
    return(newc);
  };
  return (struct ncache *)0;
}

int
_RkRelease(void)
{
  struct ncache	*newc;

  for (newc = Ncfree.nc_anext; newc != &Ncfree; newc = newc->nc_anext) {
    if (!newc->nc_word || (newc->nc_flags & NC_NHEAP))
      continue;
    (void)flushCache(newc->nc_dic, newc);
    hremove(newc);
    newc->nc_dic = (struct DM *)0;
    newc->nc_flags  = (unsigned short)0;
    newc->nc_word = (unsigned char *)0;
    newc->nc_address = (long)0;
    newc->nc_count = (unsigned long)0;
    return 1;
  };
  return 0;
}

/*
int
_RkEnrefCache(cache)
     struct ncache *cache;
{
  static int count = 0;
  fprintf(stderr, "_RkEnrefCache(0x%08x), %d\n", cache, ++count);
  return(cache->nc_count++);
}
*/

void
_RkDerefCache(struct ncache *cache)
{
  struct DM	*dm = cache->nc_dic;
/*
  static int count = 0;
  fprintf(stderr, "_RkDeref(0x%08x), %d\n", cache, ++count);
*/

//  if (cache->nc_count <= 0) {
//    _Rkpanic("wrong cache count %s %d#%d",
//	     dm ? dm->dm_dicname : "-", cache->nc_address, cache->nc_count);
//  };

  if (--cache->nc_count == 0) {
    aremove(cache);
    if (cache->nc_flags & NC_ERROR) {
      ainserttop(cache);
    } else {
      ainsertbottom(cache);
    };
  };
  return;
}

void	
_RkPurgeCache(struct ncache *cache)
{
  hremove(cache);
  aremove(cache);
  ainserttop(cache);
}

void	
_RkKillCache(struct DM *dm)
{
  struct ncache		*cache;
  int			i;

  for (i = 0, cache = SX.cache; i < SX.maxcache; i++, cache++) {
    if (dm == cache->nc_dic) {
      (void)flushCache(dm, cache);
      _RkPurgeCache(cache);
    };
  };
}

#if defined(MMAP) || defined(WIN)
int
_RkDoInvalidateCache(long addr, unsigned long size)
{
  struct ncache	*head, *cache, *tmp;
  int i;
  int found = 0;

  for(i = 0; i < NCHASH; i ++) {
    head = &Nchash[i];
    for (cache = head->nc_hnext; cache != head; )  {
      tmp = cache->nc_hnext;
      if (cache->nc_address >= addr &&
	  cache->nc_address < (long)(addr + size))  {
	found = 1;
        if (cache->nc_count) return(0);

      }
      cache = tmp;
    }
  }

  if (found == 0) return(1);

  for(i = 0; i < NCHASH; i ++) {
    head = &Nchash[i];
    for (cache = head->nc_hnext; cache != head; )  {
      tmp = cache->nc_hnext;
      if (cache->nc_address >= addr &&
	  cache->nc_address < (long)(addr + size))  {
        cache->nc_flags |= NC_ERROR;
        hremove(cache);
        aremove(cache);
        ainserttop(cache);
      }
      cache = tmp;
    }
  }
  return(1);
}
#endif

struct ncache	*
_RkFindCache(struct DM *dm, long addr)
{
  struct ncache	*head, *cache;

  head = &Nchash[hash(addr)];
  for (cache = head->nc_hnext; cache != head; cache = cache->nc_hnext)  
    if (cache->nc_dic == dm && cache->nc_address == addr) 
      return cache;
  return (struct ncache *)0;
}

void
_RkRehashCache(struct ncache *cache, long addr)
{
  struct ncache	*head;

  if ((head = &Nchash[hash(addr)]) != &Nchash[hash(cache->nc_address)]) {
    hremove(cache);
    cache->nc_hnext = head->nc_hnext;
    cache->nc_hprev = head;
    head->nc_hnext->nc_hprev = cache;
    head->nc_hnext = cache;
  };
  cache->nc_address = addr;
}

struct ncache	*
_RkReadCache(struct DM *dm, long addr)
{
  struct ncache	*head, *cache;

  head = &Nchash[hash(addr)];
  for (cache = head->nc_hnext; cache != head; cache = cache->nc_hnext) {
    if (cache->nc_dic == dm && cache->nc_address == addr) {
      aremove(cache);
      if (cache != head->nc_hnext) {
	hremove(cache);
	cache->nc_hnext = head->nc_hnext;
	cache->nc_hprev = head;
	head->nc_hnext->nc_hprev = cache;
	head->nc_hnext = cache;
      }
      _RkEnrefCache(cache);
      return(cache);
    };
  };
  cache = newCache(dm, addr);
  if (cache) {
    if (DST_READ(dm, cache)) {
      ainserttop(cache);
      return (struct ncache *)0;
    } else {
      cache->nc_hnext = head->nc_hnext;
      cache->nc_hprev = head;
      head->nc_hnext->nc_hprev = cache;
      head->nc_hnext = cache;
      _RkEnrefCache(cache);
      return(cache);
    };
  } else {
    return (struct ncache *)0;
  };
}
