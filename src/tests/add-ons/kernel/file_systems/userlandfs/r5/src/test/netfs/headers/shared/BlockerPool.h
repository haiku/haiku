// BlockerPool.h

#ifndef BLOCKER_POOL_H
#define BLOCKER_POOL_H

#include "Blocker.h"
#include "Locker.h"

class BlockerPool : public Locker {
public:
								BlockerPool(int32 count = 10);
								~BlockerPool();

			status_t			InitCheck() const;

			Blocker				GetBlocker();
			status_t			PutBlocker(Blocker blocker);

private:
			status_t			_Init(int32 count);
			void				_Unset();

private:
			struct BlockerVector;

			sem_id				fFreeBlockersSemaphore;
			BlockerVector*		fBlockers;
			status_t			fInitStatus;
			
};

// BlockerPutter
class BlockerPutter {
public:
	BlockerPutter(BlockerPool& pool, Blocker blocker)
		: fPool(pool),
		  fBlocker(blocker)
	{
	}

	~BlockerPutter()
	{
		fPool.PutBlocker(fBlocker);
	}

private:
	BlockerPool&	fPool;
	Blocker			fBlocker;
};

#endif	// BLOCKER_POOL_H
