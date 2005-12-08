/*
 * Copyright 2001-2005, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		John Walker <kelvin@fourmilab.ch>
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef MEM_POOL_H
#define MEM_POOL_H


#include <SupportDefs.h>


class MemPool {
	public:
						MemPool(int32 incrementSize);
		virtual			~MemPool();
	
		void			AddToPool(void* buffer, ssize_t len);
		void*			GetBuffer(ssize_t size, bool zero = false);
		void*			ReallocateBuffer(void* buffer, ssize_t size);
		void			ReleaseBuffer(void* buffer);

		virtual	int*	CompactMem(ssize_t sizereq, int sequence);
		virtual	void*	AcquireMem(ssize_t size);
		virtual	void	ReleaseMem(void* buffer);

		void			SetPoolIncrement(ssize_t increment);
		ssize_t			PoolIncrement();

							// debugging
		void			Stats(ssize_t* curalloc, ssize_t* totfree,
							ssize_t* maxfree, long* nget, long* nrel);

		void			ExtendedStats(ssize_t* pool_incr, long* npool,
							long* npget, long* nprel,
							long* ndget, long* ndrel);

		void			BufferDump(void* buf);
		void			PoolDump(void* pool, bool dumpalloc,
							bool dumpfree);

		int				Validate(void* buffer);

	private:
		// Header in allocated and free buffers
		struct bhead {
			// Relative link back to previous free buffer in memory or 0 if previous 
			// buffer is allocated.
		    ssize_t prevfree;
		    // Buffer size: positive if free, negative if allocated.
		    ssize_t bsize; 
		};
	
		//  Header in directly allocated buffers (by acqfcn)
		struct bdhead {
		    ssize_t tsize;	// Total size, including overhead
		    bhead bh;		// Common header
		};
	
		// Queue links
		struct bfhead;
		struct qlinks {
			bfhead *flink;	// Forward link
			bfhead *blink;	// Backward link
		};
	
		// Header in free buffers
		struct bfhead {
			// Common allocated/free header
		    bhead bh;
		    // Links on free list
		    qlinks ql;
		};

		ssize_t			fTotalAllocated;
		long			fGetCount;
		long			fReleaseCount;
		long			fNumpblk;
		long			fNumpget;
		long			fNumprel;
		long			fNumdget;
		long			fNumdrel;
		ssize_t			fIncrementSize;
		ssize_t			fPoolLength;
		// List of free buffers
		bfhead			fFreeList;
};

class AreaPool : public MemPool {
	public:
						AreaPool(const char* name, size_t initialSize);
		virtual			~AreaPool();

		virtual void*	AcquireMem(ssize_t size);
		virtual void	ReleaseMem(void* buffer);

	private:
		const char*		fName;
};

#endif	/* MEM_POOL_H */
