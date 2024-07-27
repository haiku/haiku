/*
 * Copyright (c) 2007-2012, Novell Inc.
 *
 * This program is licensed under the BSD license, read LICENSE.BSD
 * for further information
 */

/*
 * repopage.c
 *
 * Paging and compression functions for the vertical repository data.
 * Vertical data is grouped by key, normal data is grouped by solvable.
 * This makes searching for a string in vertical data fast as there's
 * no need to skip over data if keys we're not interested in.
 *
 * The vertical data is split into pages, each page is compressed with a fast
 * compression algorithm. These pages are read in on demand, not recently used
 * pages automatically get dropped.
 */

#define _XOPEN_SOURCE 500

#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <time.h>

#include "repo.h"
#include "repopage.h"



#define BLOCK_SIZE (65536*1)
#if BLOCK_SIZE <= 65536
typedef uint16_t Ref;
#else
typedef uint32_t Ref;
#endif

/*
   The format is tailored for fast decompression (i.e. only byte based),
   and skewed to ASCII content (highest bit often not set):
   
   a 0LLLLLLL
        - self-describing ASCII character hex L
   b 100lllll <l+1 bytes>
        - literal run of length l+1
   c 101oolll <8o>
        - back ref of length l+2, at offset -(o+1) (o < 1 << 10)
   d 110lllll <8o>
        - back ref of length l+2+8, at offset -(o+1) (o < 1 << 8)
   e 1110llll <8o> <8o>
        - back ref of length l+3, at offset -(o+1) (o < 1 << 16)
  f1 1111llll <8l> <8o> <8o>
        - back ref, length l+19 (l < 1<<12), offset -(o+1) (o < 1<<16)
  f2 11110lll <8l> <8o> <8o>
        - back ref, length l+19 (l < 1<<11), offset -(o+1) (o < 1<<16)
   g 11111lll <8l> <8o> <8o> <8o>
        - back ref, length l+5 (l < 1<<11), offset -(o+1) (o < 1<<24)

   Generally for a literal of length L we need L+1 bytes, hence it is
   better to encode also very short backrefs (2 chars) as backrefs if
   their offset is small, as that only needs two bytes.  Except if we
   already have a literal run, in that case it's better to append there,
   instead of breaking it for a backref.  So given a potential backref
   at offset O, length L the strategy is as follows:

   L < 2 : encode as 1-literal
   L == 2, O > 1024 : encode as 1-literal
   L == 2, have already literals: encode as 1-literal
   O = O - 1
   L >= 2, L <= 9, O < 1024                            : encode as c
   L >= 10, L <= 41, O < 256                           : encode as d
   else we have either O >= 1024, or L >= 42:
   L < 3 : encode as 1-literal
   L >= 3, L <= 18, O < 65536                          : encode as e
   L >= 19, L <= 4095+18, O < 65536                    : encode as f
   else we have either L >= 4096+18 or O >= 65536.
   O >= 65536: encode as 1-literal, too bad
     (with the current block size this can't happen)
   L >= 4096+18, so reduce to 4095+18                  : encode as f
*/


static unsigned int
compress_buf(const unsigned char *in, unsigned int in_len,
	      unsigned char *out, unsigned int out_len)
{
  unsigned int oo = 0;		/* out-offset */
  unsigned int io = 0;		/* in-offset */
#define HS (65536)
  Ref htab[HS];
  Ref hnext[BLOCK_SIZE];
  unsigned int litofs = 0;
  memset(htab, -1, sizeof (htab));
  memset(hnext, -1, sizeof (hnext));
  while (io + 2 < in_len)
    {
      /* Search for a match of the string starting at IN, we have at
         least three characters.  */
      unsigned int hval = in[io] | in[io + 1] << 8 | in[io + 2] << 16;
      unsigned int try, mlen, mofs, tries;
      hval = (hval ^ (hval << 5) ^ (hval >> 5)) - hval * 5;
      hval = hval & (HS - 1);
      try = htab[hval];
      hnext[io] = htab[hval];
      htab[hval] = io;
      mlen = 0;
      mofs = 0;

      for (tries = 0; try != -1 && tries < 12; tries++)
        {
	  if (try < io
	      && in[try] == in[io] && in[try + 1] == in[io + 1])
	    {
	      mlen = 2;
	      mofs = (io - try) - 1;
	      break;
	    }
	  try = hnext[try];
	}
      for (; try != -1 && tries < 12; tries++)
	{
	  /* assert(mlen >= 2); */
	  /* assert(io + mlen < in_len); */
	  /* Try a match starting from [io] with the strings at [try].
	     That's only sensible if TRY actually is before IO (can happen
	     with uninit hash table).  If we have a previous match already
	     we're only going to take the new one if it's longer, hence
	     check the potentially last character.  */
	  if (try < io && in[try + mlen] == in[io + mlen])
	    {
	      unsigned int this_len, this_ofs;
	      if (memcmp(in + try, in + io, mlen))
		goto no_match;
	      this_len = mlen + 1;
	      /* Now try extending the match by more characters.  */
	      for (;
		   io + this_len < in_len
		   && in[try + this_len] == in[io + this_len]; this_len++)
		;
#if 0
	      unsigned int testi;
	      for (testi = 0; testi < this_len; testi++)
		assert(in[try + testi] == in[io + testi]);
#endif
	      this_ofs = (io - try) - 1;
	      /*if (this_ofs > 65535)
		 goto no_match; */
#if 0
	      assert(this_len >= 2);
	      assert(this_len >= mlen);
	      assert(this_len > mlen || (this_len == mlen && this_ofs > mofs));
#endif
	      mlen = this_len, mofs = this_ofs;
	      /* If our match extends up to the end of input, no next
		 match can become better.  This is not just an
		 optimization, it establishes a loop invariant
		 (io + mlen < in_len).  */
	      if (io + mlen >= in_len)
		goto match_done;
	    }
	no_match:
	  try = hnext[try];
	  /*if (io - try - 1 >= 65536)
	    break;*/
	}

match_done:
      if (mlen)
	{
	  /*fprintf(stderr, "%d %d\n", mlen, mofs);*/
	  if (mlen == 2 && (litofs || mofs >= 1024))
	    mlen = 0;
	  /*else if (mofs >= 65536)
	    mlen = 0;*/
	  else if (mofs >= 65536)
	    {
	      if (mlen >= 2048 + 5)
	        mlen = 2047 + 5;
	      else if (mlen < 5)
	        mlen = 0;
	    }
	  else if (mlen < 3)
	    mlen = 0;
	  /*else if (mlen >= 4096 + 19)
	    mlen = 4095 + 19;*/
	  else if (mlen >= 2048 + 19)
	    mlen = 2047 + 19;
	  /* Skip this match if the next character would deliver a better one,
	     but only do this if we have the chance to really extend the
	     length (i.e. our current length isn't yet the (conservative)
	     maximum).  */
	  if (mlen && mlen < (2048 + 5) && io + 3 < in_len)
	    {
	      unsigned int hval =
		in[io + 1] | in[io + 2] << 8 | in[io + 3] << 16;
	      unsigned int try;
	      hval = (hval ^ (hval << 5) ^ (hval >> 5)) - hval * 5;
	      hval = hval & (HS - 1);
	      try = htab[hval];
	      if (try < io + 1
		  && in[try] == in[io + 1] && in[try + 1] == in[io + 2])
		{
		  unsigned int this_len;
		  this_len = 2;
		  for (;
		       io + 1 + this_len < in_len
		       && in[try + this_len] == in[io + 1 + this_len];
		       this_len++)
		    ;
		  if (this_len >= mlen)
		    mlen = 0;
		}
	    }
	}
      if (!mlen)
	{
	  if (!litofs)
	    litofs = io + 1;
	  io++;
	}
      else
	{
	  if (litofs)
	    {
	      unsigned litlen;
	      litofs--;
	      litlen = io - litofs;
	      /* fprintf(stderr, "lit: %d\n", litlen); */
	      while (litlen)
		{
		  unsigned int easy_sz;
		  /* Emit everything we can as self-describers.  As soon as
		     we hit a byte we can't emit as such we're going to emit
		     a length descriptor anyway, so we can as well include
		     bytes < 0x80 which might follow afterwards in that run.  */
		  for (easy_sz = 0;
		       easy_sz < litlen && in[litofs + easy_sz] < 0x80;
		       easy_sz++)
		    ;
		  if (easy_sz)
		    {
		      if (oo + easy_sz >= out_len)
			return 0;
		      memcpy(out + oo, in + litofs, easy_sz);
		      litofs += easy_sz;
		      oo += easy_sz;
		      litlen -= easy_sz;
		      if (!litlen)
			break;
		    }
		  if (litlen <= 32)
		    {
		      if (oo + 1 + litlen >= out_len)
			return 0;
		      out[oo++] = 0x80 | (litlen - 1);
		      while (litlen--)
			out[oo++] = in[litofs++];
		      break;
		    }
		  else
		    {
		      /* Literal length > 32, so chunk it.  */
		      if (oo + 1 + 32 >= out_len)
			return 0;
		      out[oo++] = 0x80 | 31;
		      memcpy(out + oo, in + litofs, 32);
		      oo += 32;
		      litofs += 32;
		      litlen -= 32;
		    }
		}
	      litofs = 0;
	    }

	  /* fprintf(stderr, "ref: %d @ %d\n", mlen, mofs); */

	  if (mlen >= 2 && mlen <= 9 && mofs < 1024)
	    {
	      if (oo + 2 >= out_len)
		return 0;
	      out[oo++] = 0xa0 | ((mofs & 0x300) >> 5) | (mlen - 2);
	      out[oo++] = mofs & 0xff;
	    }
	  else if (mlen >= 10 && mlen <= 41 && mofs < 256)
	    {
	      if (oo + 2 >= out_len)
		return 0;
	      out[oo++] = 0xc0 | (mlen - 10);
	      out[oo++] = mofs;
	    }
	  else if (mofs >= 65536)
	    {
	      assert(mlen >= 5 && mlen < 2048 + 5);
	      if (oo + 5 >= out_len)
	        return 0;
	      out[oo++] = 0xf8 | ((mlen - 5) >> 8);
	      out[oo++] = (mlen - 5) & 0xff;
	      out[oo++] = mofs & 0xff;
	      out[oo++] = (mofs >> 8) & 0xff;
	      out[oo++] = mofs >> 16;
	    }
	  else if (mlen >= 3 && mlen <= 18)
	    {
	      assert(mofs < 65536);
	      if (oo + 3 >= out_len)
		return 0;
	      out[oo++] = 0xe0 | (mlen - 3);
	      out[oo++] = mofs & 0xff;
	      out[oo++] = mofs >> 8;
	    }
	  else
	    {
	      assert(mlen >= 19 && mlen <= 4095 + 19 && mofs < 65536);
	      if (oo + 4 >= out_len)
		return 0;
	      out[oo++] = 0xf0 | ((mlen - 19) >> 8);
	      out[oo++] = (mlen - 19) & 0xff;
	      out[oo++] = mofs & 0xff;
	      out[oo++] = mofs >> 8;
	    }
	  /* Insert the hashes for the compressed run [io..io+mlen-1].
	     For [io] we have it already done at the start of the loop.
	     So it's from [io+1..io+mlen-1], and we need three chars per
	     hash, so the accessed characters will be [io+1..io+mlen-1+2],
	     ergo io+mlen+1 < in_len.  */
	  mlen--;
	  io++;
	  while (mlen--)
	    {
	      if (io + 2 < in_len)
		{
		  unsigned int hval =
		    in[io] | in[io + 1] << 8 | in[io + 2] << 16;
		  hval = (hval ^ (hval << 5) ^ (hval >> 5)) - hval * 5;
		  hval = hval & (HS - 1);
		  hnext[io] = htab[hval];
		  htab[hval] = io;
		}
	      io++;
	    };
	}
    }
  /* We might have some characters left.  */
  if (io < in_len && !litofs)
    litofs = io + 1;
  io = in_len;
  if (litofs)
    {
      unsigned litlen;
      litofs--;
      litlen = io - litofs;
      /* fprintf(stderr, "lit: %d\n", litlen); */
      while (litlen)
	{
	  unsigned int easy_sz;
	  /* Emit everything we can as self-describers.  As soon as we hit a
	     byte we can't emit as such we're going to emit a length
	     descriptor anyway, so we can as well include bytes < 0x80 which
	     might follow afterwards in that run.  */
	  for (easy_sz = 0; easy_sz < litlen && in[litofs + easy_sz] < 0x80;
	       easy_sz++)
	    ;
	  if (easy_sz)
	    {
	      if (oo + easy_sz >= out_len)
		return 0;
	      memcpy(out + oo, in + litofs, easy_sz);
	      litofs += easy_sz;
	      oo += easy_sz;
	      litlen -= easy_sz;
	      if (!litlen)
		break;
	    }
	  if (litlen <= 32)
	    {
	      if (oo + 1 + litlen >= out_len)
		return 0;
	      out[oo++] = 0x80 | (litlen - 1);
	      while (litlen--)
		out[oo++] = in[litofs++];
	      break;
	    }
	  else
	    {
	      /* Literal length > 32, so chunk it.  */
	      if (oo + 1 + 32 >= out_len)
		return 0;
	      out[oo++] = 0x80 | 31;
	      memcpy(out + oo, in + litofs, 32);
	      oo += 32;
	      litofs += 32;
	      litlen -= 32;
	    }
	}
      litofs = 0;
    }
  return oo;
}

static unsigned int
unchecked_decompress_buf(const unsigned char *in, unsigned int in_len,
			  unsigned char *out,
			  unsigned int out_len __attribute__((unused)))
{
  unsigned char *orig_out = out;
  const unsigned char *in_end = in + in_len;
  while (in < in_end)
    {
      unsigned int first = *in++;
      int o;
      switch (first >> 4)
	{
	default:
	  /* This default case can't happen, but GCCs VRP is not strong
	     enough to see this, so make this explicitely not fall to
	     the end of the switch, so that we don't have to initialize
	     o above.  */
	  continue;
	case 0: case 1:
	case 2: case 3:
	case 4: case 5:
	case 6: case 7:
	  /* a 0LLLLLLL */
	  /* fprintf (stderr, "lit: 1\n"); */
	  *out++ = first;
	  continue;
	case 8: case 9:
	  /* b 100lllll <l+1 bytes> */
	  {
	    unsigned int l = first & 31;
	    /* fprintf (stderr, "lit: %d\n", l); */
	    do
	      *out++ = *in++;
	    while (l--);
	    continue;
	  }
	case 10: case 11:
	  /* c 101oolll <8o> */
	  {
	    o = first & (3 << 3);
	    o = (o << 5) | *in++;
	    first = (first & 7) + 2;
	    break;
	  }
	case 12: case 13:
	  /* d 110lllll <8o> */
	  {
	    o = *in++;
	    first = (first & 31) + 10;
	    break;
	  }
	case 14:
	  /* e 1110llll <8o> <8o> */
	  {
	    o = in[0] | (in[1] << 8);
	    in += 2;
	    first = first & 31;
	    first += 3;
	    break;
	  }
	case 15:
	  /* f1 1111llll <8o> <8o> <8l> */
	  /* f2 11110lll <8o> <8o> <8l> */
	  /* g 11111lll <8o> <8o> <8o> <8l> */
	  {
	    first = first & 15;
	    if (first >= 8)
	      {
		first = (((first - 8) << 8) | in[0]) + 5;
		o = in[1] | (in[2] << 8) | (in[3] << 16);
		in += 4;
	      }
	    else
	      {
	        first = ((first << 8) | in[0]) + 19;
		o = in[1] | (in[2] << 8);
		in += 3;
	      }
	    break;
	  }
	}
      /* fprintf(stderr, "ref: %d @ %d\n", first, o); */
      o++;
      o = -o;
#if 0
      /* We know that first will not be zero, and this loop structure is
         better optimizable.  */
      do
	{
	  *out = *(out - o);
	  out++;
	}
      while (--first);
#else
      switch (first)
        {
	  case 18: *out = *(out + o); out++;
	  case 17: *out = *(out + o); out++;
	  case 16: *out = *(out + o); out++;
	  case 15: *out = *(out + o); out++;
	  case 14: *out = *(out + o); out++;
	  case 13: *out = *(out + o); out++;
	  case 12: *out = *(out + o); out++;
	  case 11: *out = *(out + o); out++;
	  case 10: *out = *(out + o); out++;
	  case  9: *out = *(out + o); out++;
	  case  8: *out = *(out + o); out++;
	  case  7: *out = *(out + o); out++;
	  case  6: *out = *(out + o); out++;
	  case  5: *out = *(out + o); out++;
	  case  4: *out = *(out + o); out++;
	  case  3: *out = *(out + o); out++;
	  case  2: *out = *(out + o); out++;
	  case  1: *out = *(out + o); out++;
	  case  0: break;
	  default:
	    /* Duff duff :-) */
	    switch (first & 15)
	      {
		do
		  {
		    case  0: *out = *(out + o); out++;
		    case 15: *out = *(out + o); out++;
		    case 14: *out = *(out + o); out++;
		    case 13: *out = *(out + o); out++;
		    case 12: *out = *(out + o); out++;
		    case 11: *out = *(out + o); out++;
		    case 10: *out = *(out + o); out++;
		    case  9: *out = *(out + o); out++;
		    case  8: *out = *(out + o); out++;
		    case  7: *out = *(out + o); out++;
		    case  6: *out = *(out + o); out++;
		    case  5: *out = *(out + o); out++;
		    case  4: *out = *(out + o); out++;
		    case  3: *out = *(out + o); out++;
		    case  2: *out = *(out + o); out++;
		    case  1: *out = *(out + o); out++;
		  }
		while ((int)(first -= 16) > 0);
	      }
	    break;
	}
#endif
    }
  return out - orig_out;
}

/**********************************************************************/

void repopagestore_init(Repopagestore *store)
{
  memset(store, 0, sizeof(*store));
  store->pagefd = -1;
}

void repopagestore_free(Repopagestore *store)
{
  store->blob_store = solv_free(store->blob_store);
  store->file_pages = solv_free(store->file_pages);
  store->mapped_at = solv_free(store->mapped_at);
  store->mapped = solv_free(store->mapped);
  if (store->pagefd != -1)
    close(store->pagefd);
  store->pagefd = -1;
}


/**********************************************************************/

unsigned char *
repopagestore_load_page_range(Repopagestore *store, unsigned int pstart, unsigned int pend)
{
/* Make sure all pages from PSTART to PEND (inclusive) are loaded,
   and are consecutive.  Return a pointer to the mapping of PSTART.  */
  unsigned char buf[REPOPAGE_BLOBSIZE];
  unsigned int i, best, pnum;

  if (pstart == pend)
    {
      /* Quick check in case the requested page is already mapped */
      if (store->mapped_at[pstart] != -1)
	return store->blob_store + store->mapped_at[pstart];
    }
  else
    {
      /* Quick check in case all pages are already mapped and consecutive.  */
      for (pnum = pstart; pnum <= pend; pnum++)
	if (store->mapped_at[pnum] == -1
	    || (pnum > pstart
		&& store->mapped_at[pnum]
		   != store->mapped_at[pnum-1] + REPOPAGE_BLOBSIZE))
	  break;
      if (pnum > pend)
	return store->blob_store + store->mapped_at[pstart];
    }

  if (store->pagefd == -1 || !store->file_pages)
    return 0;	/* no backing file */

#ifdef DEBUG_PAGING
  fprintf(stderr, "PAGE: want %d pages starting at %d\n", pend - pstart + 1, pstart);
#endif

  /* Ensure that we can map the numbers of pages we need at all.  */
  if (pend - pstart + 1 > store->nmapped)
    {
      unsigned int oldcan = store->nmapped;
      store->nmapped = pend - pstart + 1;
      if (store->nmapped < 4)
        store->nmapped = 4;
      store->mapped = solv_realloc2(store->mapped, store->nmapped, sizeof(store->mapped[0]));
      for (i = oldcan; i < store->nmapped; i++)
	store->mapped[i] = -1;
      store->blob_store = solv_realloc2(store->blob_store, store->nmapped, REPOPAGE_BLOBSIZE);
#ifdef DEBUG_PAGING
      fprintf(stderr, "PAGE: can map %d pages\n", store->nmapped);
#endif
    }

  if (store->mapped_at[pstart] != -1)
    {
      /* assume forward search */
      best = store->mapped_at[pstart] / REPOPAGE_BLOBSIZE;
      if (best + (pend - pstart) >= store->nmapped)
	best = 0;
    }
  else if (store->mapped_at[pend] != -1)
    {
      /* assume backward search */
      best = store->mapped_at[pend] / REPOPAGE_BLOBSIZE;
      if (best < pend - pstart)
	best = store->nmapped - 1;
      best -= pend - pstart;
    }
  else
    {
      /* choose some "random" location to avoid thrashing */
      best = (pstart + store->rr_counter++) % (store->nmapped - pend + pstart);
    }

  /* So we want to map our pages from [best] to [best+pend-pstart].
     Use a very simple strategy, which doesn't make the best use of
     our resources, but works.  Throw away all pages in that range
     (even ours) then copy around ours or read them in.  */
  for (i = best, pnum = pstart; pnum <= pend; i++, pnum++)
    {
      unsigned int pnum_mapped_at;
      unsigned int oldpnum = store->mapped[i];
      if (oldpnum != -1)
	{
	  if (oldpnum == pnum)
	    continue;	/* already have the correct page */
	  /* Evict this page.  */
#ifdef DEBUG_PAGING
	  fprintf(stderr, "PAGE: evict page %d from %d\n", oldpnum, i);
#endif
	  store->mapped[i] = -1;
	  store->mapped_at[oldpnum] = -1;
	}
      /* check if we can copy the correct content (before it gets evicted) */
      pnum_mapped_at = store->mapped_at[pnum];
      if (pnum_mapped_at != -1 && pnum_mapped_at != i * REPOPAGE_BLOBSIZE)
	{
	  void *dest = store->blob_store + i * REPOPAGE_BLOBSIZE;
#ifdef DEBUG_PAGING
	  fprintf(stderr, "PAGECOPY: %d from %d to %d\n", pnum, pnum_mapped_at / REPOPAGE_BLOBSIZE, i);
#endif
	  memcpy(dest, store->blob_store + pnum_mapped_at, REPOPAGE_BLOBSIZE);
	  store->mapped[pnum_mapped_at / REPOPAGE_BLOBSIZE] = -1;
	  store->mapped[i] = pnum;
	  store->mapped_at[pnum] = i * REPOPAGE_BLOBSIZE;
	}
    }

  /* Everything is free now.  Read in or copy the pages we want.  */
  for (i = best, pnum = pstart; pnum <= pend; i++, pnum++)
    {
      void *dest = store->blob_store + i * REPOPAGE_BLOBSIZE;
      if (store->mapped_at[pnum] != -1)
        {
          unsigned int pnum_mapped_at = store->mapped_at[pnum];
	  if (pnum_mapped_at != i * REPOPAGE_BLOBSIZE)
	    {
#ifdef DEBUG_PAGING
	      fprintf(stderr, "PAGECOPY: %d from %d to %d\n", pnum, pnum_mapped_at / REPOPAGE_BLOBSIZE, i);
#endif
	      /* Still mapped somewhere else, so just copy it from there.  */
	      memcpy(dest, store->blob_store + pnum_mapped_at, REPOPAGE_BLOBSIZE);
	      store->mapped[pnum_mapped_at / REPOPAGE_BLOBSIZE] = -1;
	    }
	}
      else
        {
	  Attrblobpage *p = store->file_pages + pnum;
	  unsigned int in_len = p->page_size;
	  unsigned int compressed = in_len & 1;
	  in_len >>= 1;
#ifdef DEBUG_PAGING
	  fprintf(stderr, "PAGEIN: %d to %d", pnum, i);
#endif
          if (pread(store->pagefd, compressed ? buf : dest, in_len, store->file_offset + p->page_offset) != in_len)
	    {
	      perror("mapping pread");
	      return 0;
	    }
	  if (compressed)
	    {
	      unsigned int out_len;
	      out_len = unchecked_decompress_buf(buf, in_len, dest, REPOPAGE_BLOBSIZE);
	      if (out_len != REPOPAGE_BLOBSIZE && pnum < store->num_pages - 1)
	        {
#ifdef DEBUG_PAGING
	          fprintf(stderr, "can't decompress\n");
#endif
		  return 0;
		}
#ifdef DEBUG_PAGING
	      fprintf(stderr, " (expand %d to %d)", in_len, out_len);
#endif
	    }
#ifdef DEBUG_PAGING
	  fprintf(stderr, "\n");
#endif
	}
      store->mapped_at[pnum] = i * REPOPAGE_BLOBSIZE;
      store->mapped[i] = pnum;
    }
  return store->blob_store + best * REPOPAGE_BLOBSIZE;
}

unsigned int
repopagestore_compress_page(unsigned char *page, unsigned int len, unsigned char *cpage, unsigned int max)
{
  return compress_buf(page, len, cpage, max);
}

#define SOLV_ERROR_EOF		3
#define SOLV_ERROR_CORRUPT	6

static inline unsigned int
read_u32(FILE *fp)
{
  int c, i;
  unsigned int x = 0; 

  for (i = 0; i < 4; i++) 
    {    
      c = getc(fp);
      if (c == EOF) 
        return 0;
      x = (x << 8) | c; 
    }    
  return x;
}

/* Try to either setup on-demand paging (using FP as backing
   file), or in case that doesn't work (FP not seekable) slurps in
   all pages and deactivates paging.  */
int
repopagestore_read_or_setup_pages(Repopagestore *store, FILE *fp, unsigned int pagesz, unsigned int blobsz)
{
  unsigned int npages;
  unsigned int i;
  unsigned int can_seek;
  unsigned int cur_page_ofs;
  unsigned char buf[REPOPAGE_BLOBSIZE];

  if (pagesz != REPOPAGE_BLOBSIZE)
    {
      /* We could handle this by slurping in everything.  */
      return SOLV_ERROR_CORRUPT;
    }
  can_seek = 1;
  if ((store->file_offset = ftell(fp)) < 0)
    can_seek = 0;
  clearerr(fp);
  if (can_seek)
    store->pagefd = dup(fileno(fp));
  if (store->pagefd == -1)
    can_seek = 0;
  else
    fcntl(store->pagefd, F_SETFD, FD_CLOEXEC);

#ifdef DEBUG_PAGING
  fprintf(stderr, "can %sseek\n", can_seek ? "" : "NOT ");
#endif
  npages = (blobsz + REPOPAGE_BLOBSIZE - 1) / REPOPAGE_BLOBSIZE;

  store->num_pages = npages;
  store->mapped_at = solv_malloc2(npages, sizeof(*store->mapped_at));

  /* If we can't seek on our input we have to slurp in everything.
   * Otherwise set up file_pages containing offest/length of the
   * pages */
  if (can_seek)
    store->file_pages = solv_malloc2(npages, sizeof(*store->file_pages));
  else
    store->blob_store = solv_malloc2(npages, REPOPAGE_BLOBSIZE);
  cur_page_ofs = 0;
  for (i = 0; i < npages; i++)
    {
      unsigned int in_len = read_u32(fp);
      unsigned int compressed = in_len & 1;
      in_len >>= 1;
#ifdef DEBUG_PAGING
      fprintf(stderr, "page %d: len %d (%scompressed)\n",
      	       i, in_len, compressed ? "" : "not ");
#endif
      if (can_seek)
        {
	  Attrblobpage *p = store->file_pages + i;
          cur_page_ofs += 4;
          store->mapped_at[i] = -1;	/* not mapped yet */
	  p->page_offset = cur_page_ofs;
	  p->page_size = in_len * 2 + compressed;
	  if (fseek(fp, in_len, SEEK_CUR) < 0)
	    {
	      /* We can't fall back to non-seeking behaviour as we already
	         read over some data pages without storing them away.  */
	      close(store->pagefd);
	      store->pagefd = -1;
	      return SOLV_ERROR_EOF;
	    }
	  cur_page_ofs += in_len;
	}
      else
        {
	  unsigned int out_len;
	  void *dest = store->blob_store + i * REPOPAGE_BLOBSIZE;
          store->mapped_at[i] = i * REPOPAGE_BLOBSIZE;
	  /* We can't seek, so suck everything in.  */
	  if (fread(compressed ? buf : dest, in_len, 1, fp) != 1)
	    {
	      perror("fread");
	      return SOLV_ERROR_EOF;
	    }
	  if (compressed)
	    {
	      out_len = unchecked_decompress_buf(buf, in_len, dest, REPOPAGE_BLOBSIZE);
	      if (out_len != REPOPAGE_BLOBSIZE && i < npages - 1)
	        {
		  return SOLV_ERROR_CORRUPT;
	        }
	    }
	}
    }
  return 0;
}

void
repopagestore_disable_paging(Repopagestore *store)
{
  if (store->num_pages)
    repopagestore_load_page_range(store, 0, store->num_pages - 1);
}

#ifdef STANDALONE

static void
transfer_file(FILE * from, FILE * to, int compress)
{
  unsigned char inb[BLOCK_SIZE];
  unsigned char outb[BLOCK_SIZE];
  while (!feof (from) && !ferror (from))
    {
      unsigned int in_len, out_len;
      if (compress)
	{
	  in_len = fread(inb, 1, BLOCK_SIZE, from);
	  if (in_len)
	    {
	      unsigned char *b = outb;
	      out_len = compress_buf(inb, in_len, outb, sizeof (outb));
	      if (!out_len)
		b = inb, out_len = in_len;
	      if (fwrite(&out_len, sizeof (out_len), 1, to) != 1)
		{
		  perror("write size");
		  exit (1);
		}
	      if (fwrite(b, out_len, 1, to) != 1)
		{
		  perror("write data");
		  exit (1);
		}
	    }
	}
      else
	{
	  if (fread(&in_len, sizeof(in_len), 1, from) != 1)
	    {
	      if (feof(from))
		return;
	      perror("can't read size");
	      exit(1);
	    }
	  if (fread(inb, in_len, 1, from) != 1)
	    {
	      perror("can't read data");
	      exit(1);
	    }
	  out_len =
	    unchecked_decompress_buf(inb, in_len, outb, sizeof(outb));
	  if (fwrite(outb, out_len, 1, to) != 1)
	    {
	      perror("can't write output");
	      exit(1);
	    }
	}
    }
}

/* Just for benchmarking purposes.  */
static void
dumb_memcpy(void *dest, const void *src, unsigned int len)
{
  char *d = dest;
  const char *s = src;
  while (len--)
    *d++ = *s++;
}

static void
benchmark(FILE * from)
{
  unsigned char inb[BLOCK_SIZE];
  unsigned char outb[BLOCK_SIZE];
  unsigned int in_len = fread(inb, 1, BLOCK_SIZE, from);
  unsigned int out_len;
  if (!in_len)
    {
      perror("can't read from input");
      exit(1);
    }

  unsigned int calib_loop;
  unsigned int per_loop;
  unsigned int i, j;
  clock_t start, end;
  float seconds;

#if 0
  calib_loop = 1;
  per_loop = 0;
  start = clock();
  while ((clock() - start) < CLOCKS_PER_SEC / 4)
    {
      calib_loop *= 2;
      for (i = 0; i < calib_loop; i++)
	dumb_memcpy(outb, inb, in_len);
      per_loop += calib_loop;
    }

  fprintf(stderr, "memcpy:\nCalibrated to %d iterations per loop\n",
	   per_loop);

  start = clock();
  for (i = 0; i < 10; i++)
    for (j = 0; j < per_loop; j++)
      dumb_memcpy(outb, inb, in_len);
  end = clock();
  seconds = (end - start) / (float) CLOCKS_PER_SEC;
  fprintf(stderr, "%.2f seconds == %.2f MB/s\n", seconds,
	   ((long long) in_len * per_loop * 10) / (1024 * 1024 * seconds));
#endif

  calib_loop = 1;
  per_loop = 0;
  start = clock();
  while ((clock() - start) < CLOCKS_PER_SEC / 4)
    {
      calib_loop *= 2;
      for (i = 0; i < calib_loop; i++)
	compress_buf(inb, in_len, outb, sizeof(outb));
      per_loop += calib_loop;
    }

  fprintf(stderr, "compression:\nCalibrated to %d iterations per loop\n",
	   per_loop);

  start = clock();
  for (i = 0; i < 10; i++)
    for (j = 0; j < per_loop; j++)
      compress_buf(inb, in_len, outb, sizeof(outb));
  end = clock();
  seconds = (end - start) / (float) CLOCKS_PER_SEC;
  fprintf(stderr, "%.2f seconds == %.2f MB/s\n", seconds,
	   ((long long) in_len * per_loop * 10) / (1024 * 1024 * seconds));

  out_len = compress_buf(inb, in_len, outb, sizeof(outb));

  calib_loop = 1;
  per_loop = 0;
  start = clock();
  while ((clock() - start) < CLOCKS_PER_SEC / 4)
    {
      calib_loop *= 2;
      for (i = 0; i < calib_loop; i++)
	unchecked_decompress_buf(outb, out_len, inb, sizeof(inb));
      per_loop += calib_loop;
    }

  fprintf(stderr, "decompression:\nCalibrated to %d iterations per loop\n",
	   per_loop);

  start = clock();
  for (i = 0; i < 10; i++)
    for (j = 0; j < per_loop; j++)
      unchecked_decompress_buf(outb, out_len, inb, sizeof(inb));
  end = clock();
  seconds = (end - start) / (float) CLOCKS_PER_SEC;
  fprintf(stderr, "%.2f seconds == %.2f MB/s\n", seconds,
	   ((long long) in_len * per_loop * 10) / (1024 * 1024 * seconds));
}

int
main(int argc, char *argv[])
{
  int compress = 1;
  if (argc > 1 && !strcmp(argv[1], "-d"))
    compress = 0;
  if (argc > 1 && !strcmp(argv[1], "-b"))
    benchmark(stdin);
  else
    transfer_file(stdin, stdout, compress);
  return 0;
}

#endif

