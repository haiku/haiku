/*------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, Haiku, Inc.
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		bget++.cpp
//	Author:			John Walker <kelvin@fourmilab.ch>
//					DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	public domain BGET pool allocator converted to C++
//					Distributed with Haiku under MIT license
//  
//------------------------------------------------------------------------------*/


// Buffer allocation size quantum: all buffers allocated are a multiple of this size.  '
// This MUST be a power of two.
#define SizeQuant   4

#include <stdio.h>
#include <OS.h>
#include <assert.h>
#include <memory.h>
#include <ctype.h>

//  Declare the interface, including the requested buffer size type, ssize_t.
#include "BGet++.h"

// Queue links
struct qlinks 
{
	struct bfhead *flink;	// Forward link
	struct bfhead *blink;	// Backward link
};

// Header in allocated and free buffers
struct bhead 
{
	// Relative link back to previous free buffer in memory or 0 if previous 
	// buffer is allocated.
    ssize_t prevfree;
    
    // Buffer size: positive if free, negative if allocated.
    ssize_t bsize; 
};

#define BH(p)	((struct bhead *) (p))

//  Header in directly allocated buffers (by acqfcn)
struct bdhead 
{
    ssize_t tsize;		// Total size, including overhead
    struct bhead bh;	// Common header
};

#define BDH(p)	((struct bdhead *) (p))

// Header in free buffers
struct bfhead 
{
	// Common allocated/free header
    struct bhead bh;
    
    // Links on free list
    struct qlinks ql;
};

#define BFH(p)	((struct bfhead *) (p))

static struct bfhead freelist = 
{
	// List of free buffers
	{0, 0},
	{&freelist, &freelist}
};


// Total space currently allocated
//static ssize_t totalloc = 0;

// Number of GetBuffer() and ReleaseBuffer() calls
//static long numget = 0, numrel = 0;

// Number of pool blocks
//static long numpblk = 0;

 // Number of block gets and rels
//static long numpget = 0, numprel = 0;

// Number of direct gets and rels
//static long numdget = 0, numdrel = 0;

// Expansion block size
//static ssize_t exp_incr = 0;

// 0: no AddToPool calls have been made
// -1: not all pool blocks are the same size
// >0: (common) block size for all AddToPool calls made so far
//static ssize_t pool_len = 0;

// Automatic expansion block management functions
//static int (*compfcn)(ssize_t sizereq, int sequence) = NULL;
//static void *(*acqfcn)(ssize_t size) = NULL;
//static void (*relfcn)(void *buf) = NULL;


//  Minimum allocation quantum:
#define QLSize	(sizeof(struct qlinks))
#define SizeQ	((SizeQuant > QLSize) ? SizeQuant : QLSize)

// End sentinel: value placed in bsize field of dummy block delimiting end of
// pool block. The most negative number which will fit in a ssize_t, 
// defined in a way that the compiler will accept.
#define ESent	((ssize_t) (-(((1L << (sizeof(ssize_t) * 8 - 2)) - 1) * 2) - 2))


MemPool::MemPool(void)
 : totalloc(0),
 	numget(0),
 	numrel(0),
 	numpblk(0),
 	numpget(0),
 	numprel(0),
 	numdget(0),
 	numdrel(0),
 	exp_incr(0),
 	pool_len(0)
{
}

MemPool::~MemPool(void)
{
}











//  BGET  --  Allocate a buffer.
void *MemPool::GetBuffer(ssize_t requested_size, bool zero)
{
    ssize_t size = requested_size;
    struct bfhead *b;

    struct bfhead *best;
    void *buf;

	int compactseq = 0;

    assert(size > 0);

    if (size < (ssize_t)SizeQ)
    {
    	// Need at least room for the queue links.
		size = SizeQ;
    }

#if SizeQuant > 1
    size = (size + (SizeQuant - 1)) & (~(SizeQuant - 1));
#endif
	
	// Add overhead in allocated buffer to size required.
    size += sizeof(struct bhead);     

    // If a compact function was provided in the call to bectl(), wrap
    // a loop around the allocation process  to  allow	compaction  to
    // intervene in case we don't find a suitable buffer in the chain.
    while (1) 
    {
		b = freelist.ql.flink;
		best = &freelist;
	
	
		// Scan the free list searching for the first buffer big enough
		// to hold the requested size buffer.
	
		while (b != &freelist) 
		{
		    if (b->bh.bsize >= size) 
		    {
				if ((best == &freelist) || (b->bh.bsize < best->bh.bsize)) 
				{
				    best = b;
				}
		    }
		    
		    b = b->ql.flink;		  // Link to next buffer
		}
		b = best;
		
		while (b != &freelist) 
		{
		    if ((ssize_t) b->bh.bsize >= size) 
		    {
	
				// Buffer is big enough to satisfy the request. Allocate it to the caller.  
				// We must decide whether the buffer is large enough to split into the part 
				// given to the caller and a free buffer that remains on the free list, or
				// whether the entire buffer should be removed from the free list and given 
				// to the caller in its entirety. We only split the buffer if enough room 
				// remains for a header plus the minimum quantum of allocation.
		
				if ((b->bh.bsize - size) > (ssize_t)(SizeQ + (sizeof(struct bhead)))) 
				{
				    struct bhead *ba, *bn;
		
				    ba = BH(((char *) b) + (b->bh.bsize - size));
				    bn = BH(((char *) ba) + size);
				    assert(bn->prevfree == b->bh.bsize);
				    
				    // Subtract size from length of free block.
				    b->bh.bsize -= size;
				    
				    // Link allocated buffer to the previous free buffer.
				    ba->prevfree = b->bh.bsize;
				    
				    // Plug negative size into user buffer.
				    ba->bsize = -(ssize_t) size;
				    
				    // Mark buffer after this one not preceded by free block.
				    bn->prevfree = 0;
		
				    totalloc += size;
				    numget++;		  // Increment number of GetBuffer() calls
					
				    buf = (void *) ((((char *) ba) + sizeof(struct bhead)));
				    return buf;
				}
				else
				{
				    struct bhead *ba;
		
				    ba = BH(((char *) b) + b->bh.bsize);
				    assert(ba->prevfree == b->bh.bsize);
		
					// The buffer isn't big enough to split.  Give  the  whole
					// shebang to the caller and remove it from the free list.
		
				    assert(b->ql.blink->ql.flink == b);
				    assert(b->ql.flink->ql.blink == b);
				    b->ql.blink->ql.flink = b->ql.flink;
				    b->ql.flink->ql.blink = b->ql.blink;
		
				    totalloc += b->bh.bsize;
				    numget++;		  // Increment number of GetBuffer() calls
					
				    // Negate size to mark buffer allocated.
				    b->bh.bsize = -(b->bh.bsize);
		
				    // Zero the back pointer in the next buffer in memory
				    // to indicate that this buffer is allocated.
				    ba->prevfree = 0;
		
				    // Give user buffer starting at queue links.
				    buf =  (void *) &(b->ql);
				    return buf;
				}
		    }
		    b = b->ql.flink;		  // Link to next buffer
		}
		
	
		// We failed to find a buffer.  If there's a compact  function
		// defined,  notify  it  of the size requested.  If it returns
		// TRUE, try the allocation again.
	
		if(!CompactMem(size, ++compactseq))
		    break;
    }

    // No buffer available with requested size free.

    // Don't give up yet -- look in the reserve supply.
	if (size > (ssize_t)(exp_incr - sizeof(struct bhead))) 
	{
	    // Request	is  too  large	to  fit in a single expansion
	    // block.  Try to satisy it by a direct buffer acquisition.
	    struct bdhead *bdh;

	    size += sizeof(struct bdhead) - sizeof(struct bhead);
	    if ((bdh = BDH(AcquireMem((ssize_t) size))) != NULL) 
	    {
			//  Mark the buffer special by setting the size field
			//  of its header to zero.
			bdh->bh.bsize = 0;
			bdh->bh.prevfree = 0;
			bdh->tsize = size;
	
			totalloc += size;
			
			// Increment number of GetBuffer() calls
			numget++;
			
			// Direct GetBuffer() call count
			numdget++;
	
			buf =  (void *) (bdh + 1);
			return buf;
	    }

	}
	else
	{

	    //	Try to obtain a new expansion block
	    void *newpool;

	    if ((newpool = AcquireMem((ssize_t) exp_incr)) != NULL) 
	    {
			AddToPool(newpool, exp_incr);
			
			// This can't, I say, can't get into a loop
            buf =  GetBuffer(requested_size);

			return buf;
	    }
	}

    //	Still no buffer available
    return NULL;

	// Code from bgetz -- zeroing code
/*
	char *buf = (char *) GetBuffer(size);

    if (buf != NULL) 
    {
	   	struct bhead *b;
		ssize_t rsize;
	
		b = BH(buf - sizeof(struct bhead));
		rsize = -(b->bsize);
		
		if (rsize == 0) 
		{
		    struct bdhead *bd;
		
		    bd = BDH(buf - sizeof(struct bdhead));
		    rsize = bd->tsize - sizeof(struct bdhead);
		}
		else 
		{
		    rsize -= sizeof(struct bhead);
		}
		
		assert(rsize >= size);
		memset(buf, 0, (MemSize) rsize);
    }
    return ((void *) buf);
*/

}

// BGETZ  --  Allocate a buffer and clear its contents to zero.  We clear
// the entire contents of the buffer to zero, not just the region 
// requested by the caller.
/*
void *bgetz(ssize_t size)
{
    char *buf = (char *) GetBuffer(size);

    if (buf != NULL) 
    {
	   	struct bhead *b;
		ssize_t rsize;
	
		b = BH(buf - sizeof(struct bhead));
		rsize = -(b->bsize);
		
		if (rsize == 0) 
		{
		    struct bdhead *bd;
		
		    bd = BDH(buf - sizeof(struct bdhead));
		    rsize = bd->tsize - sizeof(struct bdhead);
		}
		else 
		{
		    rsize -= sizeof(struct bhead);
		}
		
		assert(rsize >= size);
		memset(buf, 0, (MemSize) rsize);
    }
    return ((void *) buf);
}
*/

//  BGETR  --  Reallocate a buffer.  This is a minimal implementation,
// simply in terms of ReleaseBuffer()  and  GetBuffer().	 It  could  be
// enhanced to allow the buffer to grow into adjacent free
// blocks and to avoid moving data unnecessarily.

void *MemPool::ReallocateBuffer(void *buf, ssize_t size)
{
    void *nbuf;
    
    // Old size of buffer
    ssize_t osize;
    struct bhead *b;

    if ((nbuf = GetBuffer(size)) == NULL)
    {
    	// Acquire new buffer
		return NULL;
    }
    
    if (buf == NULL) 
    {
		return nbuf;
    }
    b = BH(((char *) buf) - sizeof(struct bhead));
    osize = -b->bsize;

    if (osize == 0) 
    {
		//  Buffer acquired directly through acqfcn.
		struct bdhead *bd;
	
		bd = BDH(((char *) buf) - sizeof(struct bdhead));
		osize = bd->tsize - sizeof(struct bdhead);
    }
    else
		osize -= sizeof(struct bhead);
    assert(osize > 0);
    
    // Copy the data
    memcpy((char *) nbuf, (char *) buf, (int) ((size < osize) ? size : osize));
    ReleaseBuffer(buf);
    return nbuf;
}

//  BREL  --  Release a buffer. 
void MemPool::ReleaseBuffer(void *buf)
{
    struct bfhead *b, *bn;

    b = BFH(((char *) buf) - sizeof(struct bhead));
	
	// Increment number of ReleaseBuffer() calls
    numrel++;

    assert(buf != NULL);
	
	// Directly-acquired buffer?
    if (b->bh.bsize == 0)
    {
		struct bdhead *bdh;
	
		bdh = BDH(((char *) buf) - sizeof(struct bdhead));
		assert(b->bh.prevfree == 0);
	
		totalloc -= bdh->tsize;
		assert(totalloc >= 0);
		
		// Number of direct releases
		numdrel++;
	
		memset((char *) buf, 0x55, (int) (bdh->tsize - sizeof(struct bdhead)));
	
		// Release it directly.
		ReleaseMem((void *) bdh);
		return;
    }


    // Buffer size must be negative, indicating that the buffer is allocated.
    if (b->bh.bsize >= 0) 
    {
		bn = NULL;
    }
    assert(b->bh.bsize < 0);

    //	Back pointer in next buffer must be zero, indicating the same thing:
    assert(BH((char *) b - b->bh.bsize)->prevfree == 0);

    totalloc += b->bh.bsize;
    assert(totalloc >= 0);


    // If the back link is nonzero, the previous buffer is free. 

    if (b->bh.prevfree != 0) 
    {
		// The previous buffer is free.  Consolidate this buffer  with	it
		// by  adding  the  length  of	this  buffer  to the previous free
		// buffer.  Note that we subtract the size  in	the  buffer  being
		// released,  since  it's  negative to indicate that the buffer is allocated.
	
		register ssize_t size = b->bh.bsize;
	
	    // Make the previous buffer the one we're working on.
		assert(BH((char *) b - b->bh.prevfree)->bsize == b->bh.prevfree);
		b = BFH(((char *) b) - b->bh.prevfree);
		b->bh.bsize -= size;
    }
    else 
    {
		// The previous buffer isn't allocated.  Insert this buffer on the 
		// free list as an isolated free block.
		assert(freelist.ql.blink->ql.flink == &freelist);
		assert(freelist.ql.flink->ql.blink == &freelist);
		b->ql.flink = &freelist;
		b->ql.blink = freelist.ql.blink;
		freelist.ql.blink = b;
		b->ql.blink->ql.flink = b;
		b->bh.bsize = -b->bh.bsize;
    }

	// Now we look at the next buffer in memory, located by advancing from
	// the  start  of  this  buffer  by its size, to see if that buffer is
	// free.  If it is, we combine  this  buffer  with	the  next  one	in
	// memory, dechaining the second buffer from the free list.

    bn =  BFH(((char *) b) + b->bh.bsize);
    if (bn->bh.bsize > 0) 
    {
		// The buffer is free.	Remove it from the free list and add
		// its size to that of our buffer.
		assert(BH((char *) bn + bn->bh.bsize)->prevfree == bn->bh.bsize);
		assert(bn->ql.blink->ql.flink == bn);
		assert(bn->ql.flink->ql.blink == bn);
		bn->ql.blink->ql.flink = bn->ql.flink;
		bn->ql.flink->ql.blink = bn->ql.blink;
		b->bh.bsize += bn->bh.bsize;
	
		// Finally,  advance  to   the	buffer	that   follows	the  newly
		// consolidated free block.  We must set its  backpointer  to  the
		// head  of  the  consolidated free block.  We know the next block
		// must be an allocated block because the process of recombination
		// guarantees  that  two  free	blocks will never be contiguous in
		// memory.
		bn = BFH(((char *) b) + b->bh.bsize);
    }

    memset(((char *) b) + sizeof(struct bfhead), 0x55,(int) (b->bh.bsize - sizeof(struct bfhead)));

    assert(bn->bh.bsize < 0);

    // The next buffer is allocated.  Set the backpointer in it  to  point
    // to this buffer; the previous free buffer in memory.
    bn->bh.prevfree = b->bh.bsize;

	// If a block-release function is defined, and this free buffer
	// constitutes the entire block, release it.  Note that  pool_len
	// is  defined  in  such a way that the test will fail unless all
	// pool blocks are the same size.

    if (((ssize_t) b->bh.bsize) == (ssize_t)(pool_len - sizeof(struct bhead))) 
	{
		assert(b->bh.prevfree == 0);
		assert(BH((char *) b + b->bh.bsize)->bsize == ESent);
		assert(BH((char *) b + b->bh.bsize)->prevfree == b->bh.bsize);
		
		//  Unlink the buffer from the free list 
		b->ql.blink->ql.flink = b->ql.flink;
		b->ql.flink->ql.blink = b->ql.blink;
	
		ReleaseMem(b);
		
		// Nr of expansion block releases
		numprel++;
		
		// Total number of blocks
		numpblk--;
		assert(numpblk == numpget - numprel);
    }
}

/*
// BECTL -- Establish automatic pool expansion control
void bectl( int (*compact) (ssize_t sizereq, int sequence), 
	void *(*acquire) (ssize_t size),
	void (*release) (void *buf),
	ssize_t pool_incr )
{
    compfcn = compact;
    acqfcn = acquire;
    relfcn = release;
    exp_incr = pool_incr;
}
*/

// BPOOL -- Add a region of memory to the buffer pool.
void MemPool::AddToPool(void *buf, ssize_t len)
{
    struct bfhead *b = BFH(buf);
    struct bhead *bn;

    len &= ~(SizeQuant - 1);

    if (pool_len == 0) 
    {
		pool_len = len;
    }
    else
    if (len != pool_len)
    {
		pool_len = -1;
    }
	
	// Number of block acquisitions
    numpget++;
    
    // Number of blocks total
    numpblk++;
    assert(numpblk == numpget - numprel);

    // Since the block is initially occupied by a single free  buffer,
    // it  had	better	not  be  (much) larger than the largest buffer
    // whose size we can store in bhead.bsize.
    assert(len - sizeof(struct bhead) <= -((ssize_t) ESent + 1));

    // Clear  the  backpointer at  the start of the block to indicate that
    // there  is  no  free  block  prior  to  this   one.    That   blocks
    // recombination when the first block in memory is released.
    b->bh.prevfree = 0;

    // Chain the new block to the free list.
    assert(freelist.ql.blink->ql.flink == &freelist);
    assert(freelist.ql.flink->ql.blink == &freelist);
    b->ql.flink = &freelist;
    b->ql.blink = freelist.ql.blink;
    freelist.ql.blink = b;
    b->ql.blink->ql.flink = b;

	// Create a dummy allocated buffer at the end of the pool.	This dummy
	// buffer is seen when a buffer at the end of the pool is released and
	// blocks  recombination  of  the last buffer with the dummy buffer at
	// the end.  The length in the dummy buffer  is  set  to  the  largest
	// negative  number  to  denote  the  end  of  the pool for diagnostic
	// routines (this specific value is  not  counted  on  by  the  actual
	// allocation and release functions).

    len -= sizeof(struct bhead);
    b->bh.bsize = (ssize_t) len;

    memset(((char *) b) + sizeof(struct bfhead), 0x55,(int) (len - sizeof(struct bfhead)));

    bn = BH(((char *) b) + len);
    bn->prevfree = (ssize_t) len;
    
    // Definition of ESent assumes two's complement!
    assert((~0) == -1);
    bn->bsize = ESent;
}


//  BSTATS  --	Return buffer allocation free space statistics. 
void MemPool::Stats(ssize_t *curalloc, ssize_t *totfree, ssize_t *maxfree,
	 long *nget, long *nrel)
{
    struct bfhead *b = freelist.ql.flink;

    *nget = numget;
    *nrel = numrel;
    *curalloc = totalloc;
    *totfree = 0;
    *maxfree = -1;
    while (b != &freelist) 
    {
		assert(b->bh.bsize > 0);
		*totfree += b->bh.bsize;
		if (b->bh.bsize > *maxfree) 
		{
		    *maxfree = b->bh.bsize;
		}
		
		// Link to next buffer
		b = b->ql.flink;
    }
}


//  BSTATSE  --  Return extended statistics 
void MemPool::ExtendedStats(ssize_t *pool_incr, long *npool, long *npget, long *nprel, 
	long *ndget, long *ndrel)
{
    *pool_incr = (pool_len < 0) ? -exp_incr : exp_incr;
    *npool = numpblk;
    *npget = numpget;
    *nprel = numprel;
    *ndget = numdget;
    *ndrel = numdrel;
}



//  BUFDUMP  --  Dump the data in a buffer.  This is called with the  user
// data pointer, and backs up to the buffer header.  It will
// dump either a free block or an allocated one.
void MemPool::BufferDump(void *buf)
{
    struct bfhead *b;
    unsigned char *bdump;
    ssize_t bdlen;

    b = BFH(((char *) buf) - sizeof(struct bhead));
    assert(b->bh.bsize != 0);
    if (b->bh.bsize < 0) 
    {
		bdump = (unsigned char *) buf;
		bdlen = (-b->bh.bsize) - sizeof(struct bhead);
    }
    else
    {
		bdump = (unsigned char *) (((char *) b) + sizeof(struct bfhead));
		bdlen = b->bh.bsize - sizeof(struct bfhead);
    }

    while (bdlen > 0) 
    {
		int i, dupes = 0;
		ssize_t l = bdlen;
		char bhex[50], bascii[20];
	
		if (l > 16) 
		{
		    l = 16;
		}
	
		for (i = 0; i < l; i++) 
		{
	        sprintf(bhex + i * 3, "%02X ", bdump[i]);
	        bascii[i] = isprint(bdump[i]) ? bdump[i] : ' ';
		}
		bascii[i] = 0;
	        printf("%-48s   %s\n", bhex, bascii);
		bdump += l;
		bdlen -= l;
		while ((bdlen > 16) && (memcmp((char *) (bdump - 16),(char *) bdump, 16) == 0)) 
		{
		    dupes++;
		    bdump += 16;
		    bdlen -= 16;
		}

		if (dupes > 1) 
		{
		    printf("     (%d lines [%d bytes] identical to above line skipped)\n",
						dupes, dupes * 16);
		}
		else
		{
			if (dupes == 1) 
			{
			    bdump -= 16;
			    bdlen += 16;
			}
	    }
    }
}

//  BPOOLD  --	Dump a buffer pool.  The buffer headers are always listed.
// If DUMPALLOC is nonzero, the contents of allocated buffers
// are  dumped.   If  DUMPFREE  is  nonzero,  free blocks are
// dumped as well.  If FreeWipe  checking	is  enabled,  free
// blocks	which  have  been clobbered will always be dumped.
void MemPool::PoolDump(void *buf, bool dumpalloc, bool dumpfree)
{
    struct bfhead *b = BFH(buf);

    while (b->bh.bsize != ESent) 
    {
		ssize_t bs = b->bh.bsize;
	
		if (bs < 0) 
		{
		    bs = -bs;
	            printf("Allocated buffer: size %6ld bytes.\n", (long) bs);
		    if (dumpalloc) 
		    {
				BufferDump((void *) (((char *) b) + sizeof(struct bhead)));
		    }
		} 
		else 
		{
	            char *lerr = "";
	
		    assert(bs > 0);
		    if ((b->ql.blink->ql.flink != b) ||	(b->ql.flink->ql.blink != b)) 
			{
	                lerr = "  (Bad free list links)";
		    }
	            printf("Free block:       size %6ld bytes.%s\n",
			(long) bs, lerr);
	
		    lerr = ((char *) b) + sizeof(struct bfhead);
		    if ((bs > (ssize_t)sizeof(struct bfhead)) && ((*lerr != 0x55) ||
				(memcmp(lerr, lerr + 1, (int) (bs - (sizeof(struct bfhead) + 1))) != 0))) 
			{
				printf("(Contents of above free block have been overstored.)\n");
				BufferDump((void *) (((char *) b) + sizeof(struct bhead)));
		    } 
		    else
			{
			    if (dumpfree) 
			    {
					BufferDump((void *) (((char *) b) + sizeof(struct bhead)));
			    }
			 }
		}
		b = BFH(((char *) b) + bs);
    }
}

// BPOOLV  --  Validate a buffer pool.
int MemPool::Validate(void *buf)
{
    struct bfhead *b = BFH(buf);

    while (b->bh.bsize != ESent) 
    {
		ssize_t bs = b->bh.bsize;

		if (bs < 0) 
		{
		    bs = -bs;
		}
		else 
		{
            char *lerr = "";
	
		    assert(bs > 0);
		    if (bs <= 0) 
		    {
				return 0;
		    }
		    
		    if ((b->ql.blink->ql.flink != b) || (b->ql.flink->ql.blink != b)) 
		    {
	                printf("Free block: size %6ld bytes.  (Bad free list links)\n",
					     (long) bs);
				assert(0);
				return 0;
		    }
	
		    lerr = ((char *) b) + sizeof(struct bfhead);
		    if ((bs > (ssize_t)sizeof(struct bfhead)) && ((*lerr != 0x55) ||
					(memcmp(lerr, lerr + 1,(int) (bs - (sizeof(struct bfhead) + 1))) != 0))) 
			{
				printf("(Contents of above free block have been overstored.)\n");
				BufferDump((void *) (((char *) b) + sizeof(struct bhead)));
				assert(0);
				return 0;
		    }
	
		}
		b = BFH(((char *) b) + bs);
    }
    return 1;
}

int *MemPool::CompactMem(ssize_t sizereq, int sequence)
{
	return NULL;
}

void *MemPool::AcquireMem(ssize_t size)
{
	return NULL;
}

void MemPool::ReleaseMem(void *buffer)
{
}

AreaPool::AreaPool(void)
{
}

AreaPool::~AreaPool(void)
{
}

void *AreaPool::AcquireMem(ssize_t size)
{
	long areasize=0;
	area_id a;
	int *parea;
	
	if(size<B_PAGE_SIZE)
		areasize=B_PAGE_SIZE;
	else
	{
		if((size % B_PAGE_SIZE)!=0)
			areasize=((long)(size/B_PAGE_SIZE)+1)*B_PAGE_SIZE;
		else
			areasize=size;
	}
	a=create_area("bitmap_area",(void**)&parea,B_ANY_ADDRESS,areasize,
			B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
	if(a==B_BAD_VALUE ||
	   a==B_NO_MEMORY ||
	   a==B_ERROR)
	{
	   	printf("PANIC: BitmapManager couldn't allocate area!!\n");
	   	return NULL;
	}

	return parea;
}

void AreaPool::ReleaseMem(void *buffer)
{
	area_id trash=area_for(buffer);

	if(trash==B_ERROR)
		return;
	delete_area(trash);
}

