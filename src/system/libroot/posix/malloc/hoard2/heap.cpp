///-*-C++-*-//////////////////////////////////////////////////////////////////
//
// Hoard: A Fast, Scalable, and Memory-Efficient Allocator
//        for Shared-Memory Multiprocessors
// Contact author: Emery Berger, http://www.cs.utexas.edu/users/emery
//
// Copyright (c) 1998-2000, The University of Texas at Austin.
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Library General Public License as
// published by the Free Software Foundation, http://www.fsf.org.
//
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
//////////////////////////////////////////////////////////////////////////////

#include "config.h"

#include "heap.h"
#include "processheap.h"
#include "superblock.h"

using namespace BPrivate;

// NB: Use maketable.cpp to update this
//     if SIZE_CLASSES, ALIGNMENT, SIZE_CLASS_BASE, MAX_EMPTY_SUPERBLOCKS,
//     or SUPERBLOCK_SIZE changes.

#if (MAX_INTERNAL_FRAGMENTATION == 2)

size_t hoardHeap::_sizeTable[hoardHeap::SIZE_CLASSES] = {
	8UL, 16UL, 24UL, 32UL, 40UL, 48UL, 56UL, 72UL, 80UL, 96UL, 120UL, 144UL,
	168UL, 200UL, 240UL, 288UL, 344UL, 416UL, 496UL, 592UL, 712UL, 856UL,
	1024UL, 1232UL, 1472UL, 1768UL, 2120UL, 2544UL, 3048UL, 3664UL,
	4392UL, 5272UL, 6320UL, 7584UL, 9104UL, 10928UL, 13112UL, 15728UL,
	18872UL, 22648UL, 27176UL, 32616UL, 39136UL, 46960UL, 56352UL,
	67624UL, 81144UL, 97376UL, 116848UL, 140216UL, 168256UL, 201904UL,
	242288UL, 290744UL, 348896UL, 418672UL, 502408UL, 602888UL, 723464UL,
	868152UL, 1041784UL, 1250136UL, 1500160UL, 1800192UL, 2160232UL,
	2592280UL, 3110736UL, 3732880UL, 4479456UL, 5375344UL, 6450408UL,
	7740496UL, 9288592UL, 11146312UL, 13375568UL, 16050680UL, 19260816UL,
	23112984UL, 27735576UL, 33282688UL, 39939224UL, 47927072UL,
	57512488UL, 69014984UL, 82817976UL, 99381576UL, 119257888UL,
	143109472UL, 171731360UL, 206077632UL, 247293152UL, 296751776UL,
	356102144UL, 427322560UL, 512787072UL, 615344512UL, 738413376UL,
	886096064UL, 1063315264UL
};

size_t hoardHeap::_threshold[hoardHeap::SIZE_CLASSES] = {
	4096UL, 2048UL, 1364UL, 1024UL, 816UL, 680UL, 584UL, 452UL, 408UL,
	340UL, 272UL, 224UL, 192UL, 160UL, 136UL, 112UL, 92UL, 76UL, 64UL,
	52UL, 44UL, 36UL, 32UL, 24UL, 20UL, 16UL, 12UL, 12UL, 8UL, 8UL, 4UL,
	4UL, 4UL, 4UL, 4UL, 4UL, 4UL, 4UL, 4UL, 4UL, 4UL, 4UL, 4UL, 4UL, 4UL,
	4UL, 4UL, 4UL, 4UL, 4UL, 4UL, 4UL, 4UL, 4UL, 4UL, 4UL, 4UL, 4UL, 4UL,
	4UL, 4UL, 4UL, 4UL, 4UL, 4UL, 4UL, 4UL, 4UL, 4UL, 4UL, 4UL, 4UL, 4UL,
	4UL, 4UL, 4UL, 4UL, 4UL, 4UL, 4UL, 4UL, 4UL, 4UL, 4UL, 4UL, 4UL, 4UL,
	4UL, 4UL, 4UL, 4UL, 4UL, 4UL, 4UL, 4UL, 4UL, 4UL, 4UL, 4UL
};

#elif (MAX_INTERNAL_FRAGMENTATION == 6)

size_t hoardHeap::_sizeTable[hoardHeap::SIZE_CLASSES] = {
	8UL, 16UL, 24UL, 32UL, 48UL, 72UL, 112UL, 176UL, 288UL, 456UL, 728UL,
	1160UL, 1848UL, 2952UL, 4728UL, 7560UL, 12096UL, 19344UL, 30952UL,
	49520UL, 79232UL, 126768UL, 202832UL, 324520UL, 519232UL, 830768UL,
	1329232UL, 2126768UL, 3402824UL, 5444520UL, 8711232UL, 13937968UL,
	22300752UL, 35681200UL, 57089912UL, 91343856UL, 146150176UL,
	233840256UL, 374144416UL, 598631040UL, 957809728UL, 1532495488UL
};

size_t hoardHeap::_threshold[hoardHeap::SIZE_CLASSES] = {
	4096UL, 2048UL, 1364UL, 1024UL, 680UL, 452UL, 292UL, 184UL, 112UL, 68UL,
	44UL, 28UL, 16UL, 8UL, 4UL, 4UL, 4UL, 4UL, 4UL, 4UL, 4UL, 4UL, 4UL,
	4UL, 4UL, 4UL, 4UL, 4UL, 4UL, 4UL, 4UL, 4UL, 4UL, 4UL, 4UL, 4UL, 4UL,
	4UL, 4UL, 4UL, 4UL, 4UL
};

#elif (MAX_INTERNAL_FRAGMENTATION == 10)

size_t hoardHeap::_sizeTable[hoardHeap::SIZE_CLASSES] = {
	8UL, 16UL, 32UL, 64UL, 128UL, 256UL, 512UL, 1024UL, 2048UL, 4096UL,
	8192UL, 16384UL, 32768UL, 65536UL, 131072UL, 262144UL, 524288UL,
	1048576UL, 2097152UL, 4194304UL, 8388608UL, 16777216UL, 33554432UL,
	67108864UL, 134217728UL, 268435456UL, 536870912UL, 1073741824UL,
	2147483648UL
};

size_t hoardHeap::_threshold[hoardHeap::SIZE_CLASSES] = {
	4096UL, 2048UL, 1024UL, 512UL, 256UL, 128UL, 64UL, 32UL, 16UL, 8UL, 4UL,
	4UL, 4UL, 4UL, 4UL, 4UL, 4UL, 4UL, 4UL, 4UL, 4UL, 4UL, 4UL, 4UL, 4UL,
	4UL, 4UL, 4UL, 4UL
};

#else
#	error "Undefined size class base."
#endif


int hoardHeap::fMaxThreadHeaps = 1;
int hoardHeap::_numProcessors;
int hoardHeap::_numProcessorsMask;


// Return ceil(log_2(num)).
// num must be positive.

static int
lg(int num)
{
	assert(num > 0);
	int power = 0;
	int n = 1;
	// Invariant: 2^power == n.
	while (n < num) {
		n <<= 1;
		power++;
	}
	return power;
}


//	#pragma mark -


hoardHeap::hoardHeap(void)
	:
	_index(0), _reusableSuperblocks(NULL), _reusableSuperblocksCount(0)
#if HEAP_DEBUG
	, _magic(HEAP_MAGIC)
#endif
{
	initLock();

	for (int i = 0; i < SUPERBLOCK_FULLNESS_GROUP; i++) {
		for (int j = 0; j < SIZE_CLASSES; j++) {
			// Initialize all superblocks lists to empty.
			_superblocks[i][j] = NULL;
		}
	}

	for (int k = 0; k < SIZE_CLASSES; k++) {
		_leastEmptyBin[k] = 0;
	}
}


void
hoardHeap::insertSuperblock(int sizeclass,
	superblock *sb, processHeap *pHeap)
{
	assert(sb->isValid());
	assert(sb->getBlockSizeClass() == sizeclass);
	assert(sb->getPrev() == NULL);
	assert(sb->getNext() == NULL);
	assert(_magic == HEAP_MAGIC);

	// Now it's ours.
	sb->setOwner(this);

	// How full is this superblock?  We'll use this information to put
	// it into the right 'bin'.
	sb->computeFullness();
	int fullness = sb->getFullness();

	// Update the stats.
	incStats(sizeclass, sb->getNumBlocks() - sb->getNumAvailable(),
		sb->getNumBlocks());

	if (fullness == 0
		&& sb->getNumBlocks() > 1
		&& sb->getNumBlocks() == sb->getNumAvailable()) {
		// Recycle this superblock.
#if 0
		removeSuperblock(sb, sizeclass);
		// Update the stats.
		decStats(sizeclass,
				 sb->getNumBlocks() - sb->getNumAvailable(), sb->getNumBlocks());
		// Free it immediately.
		const size_t s = sizeFromClass(sizeclass);
		const int blksize = align(sizeof(block) + s);
#if HEAP_LOG
		// Record the memory deallocation.
		MemoryRequest m;
		m.deallocate((int)sb->getNumBlocks() *
					 (int)sizeFromClass(sb->getBlockSizeClass()));
		pHeap->getLog(getIndex()).append(m);
#endif
#if HEAP_FRAG_STATS

		pHeap->setDeallocated(0, sb->getNumBlocks() * sizeFromClass(sb->getBlockSizeClass()));
#endif

		hoardUnsbrk(sb, align(sizeof(superblock) + blksize));
#else
		recycle(sb);
#endif
	} else {
		// Insert it into the appropriate list.
		superblock *&head = _superblocks[fullness][sizeclass];
		sb->insertBefore(head);
		head = sb;
		assert(head->isValid());

		// Reset the least-empty bin counter.
		_leastEmptyBin[sizeclass] = RESET_LEAST_EMPTY_BIN;
	}
}


superblock *
hoardHeap::removeMaxSuperblock(int sizeclass)
{
	assert(_magic == HEAP_MAGIC);

	superblock *head = NULL;

	// First check the reusable superblocks list.

	head = reuse(sizeclass);
	if (head) {
		// We found one. Since we're removing this superblock, update the
		// stats accordingly.
		decStats(sizeclass,
			head->getNumBlocks() - head->getNumAvailable(),
			head->getNumBlocks());

		return head;
	}

	// Instead of finding the superblock with the most available space
	// (something that would either involve a linear scan through the
	// superblocks or maintaining the superblocks in sorted order), we
	// just pick one that is no more than
	// 1/(SUPERBLOCK_FULLNESS_GROUP-1) more full than the superblock
	// with the most available space.  We start with the emptiest group.

	int i = 0;

	// Note: the last group (SUPERBLOCK_FULLNESS_GROUP - 1) is full, so
	// we never need to check it. But for robustness, we leave it in.
	while (i < SUPERBLOCK_FULLNESS_GROUP) {
		head = _superblocks[i][sizeclass];
		if (head)
			break;

		i++;
	}

	if (!head)
		return NULL;

	// Make sure that this superblock is at least 1/EMPTY_FRACTION
	// empty.
	assert(head->getNumAvailable() * EMPTY_FRACTION >= head->getNumBlocks());

	removeSuperblock(head, sizeclass);

	assert(head->isValid());
	assert(head->getPrev() == NULL);
	assert(head->getNext() == NULL);
	return head;
}


void
hoardHeap::removeSuperblock(superblock *sb, int sizeclass)
{
	assert(_magic == HEAP_MAGIC);

	assert(sb->isValid());
	assert(sb->getOwner() == this);
	assert(sb->getBlockSizeClass() == sizeclass);

	for (int i = 0; i < SUPERBLOCK_FULLNESS_GROUP; i++) {
		if (sb == _superblocks[i][sizeclass]) {
			_superblocks[i][sizeclass] = sb->getNext();
			if (_superblocks[i][sizeclass] != NULL) {
				assert(_superblocks[i][sizeclass]->isValid());
			}
			break;
		}
	}

	sb->remove();
	decStats(sizeclass, sb->getNumBlocks() - sb->getNumAvailable(),
		sb->getNumBlocks());
}


void
hoardHeap::moveSuperblock(superblock *sb,
	int sizeclass, int fromBin, int toBin)
{
	assert(_magic == HEAP_MAGIC);
	assert(sb->isValid());
	assert(sb->getOwner() == this);
	assert(sb->getBlockSizeClass() == sizeclass);
	assert(sb->getFullness() == toBin);

	// Remove the superblock from the old bin.

	superblock *&oldHead = _superblocks[fromBin][sizeclass];
	if (sb == oldHead) {
		oldHead = sb->getNext();
		if (oldHead != NULL) {
			assert(oldHead->isValid());
		}
	}

	sb->remove();

	// Insert the superblock into the new bin.

	superblock *&newHead = _superblocks[toBin][sizeclass];
	sb->insertBefore(newHead);
	newHead = sb;
	assert(newHead->isValid());

	// Reset the least-empty bin counter.
	_leastEmptyBin[sizeclass] = RESET_LEAST_EMPTY_BIN;
}


// The heap lock must be held when this procedure is called.

int
hoardHeap::freeBlock(block * &b, superblock * &sb,
	int sizeclass, processHeap *pHeap)
{
	assert(sb->isValid());
	assert(b->isValid());
	assert(this == sb->getOwner());

	const int oldFullness = sb->getFullness();
	sb->putBlock(b);
	decUStats(sizeclass);
	const int newFullness = sb->getFullness();

	// Free big superblocks.
	if (sb->getNumBlocks() == 1) {
		removeSuperblock(sb, sizeclass);
		const size_t s = sizeFromClass(sizeclass);
		const int blksize = align(sizeof(block) + s);
#if HEAP_LOG
		// Record the memory deallocation.
		MemoryRequest m;
		m.deallocate((int)sb->getNumBlocks()
			* (int)sizeFromClass(sb->getBlockSizeClass()));
		pHeap->getLog(getIndex()).append(m);
#endif
#if HEAP_FRAG_STATS
		pHeap->setDeallocated(0,
			sb->getNumBlocks() * sizeFromClass(sb->getBlockSizeClass()));
#endif
		hoardUnsbrk(sb, align(sizeof(superblock) + blksize));
		return 1;
	}

	// If the fullness value has changed, move the superblock.
	if (newFullness != oldFullness) {
		moveSuperblock(sb, sizeclass, oldFullness, newFullness);
	} else {
		// Move the superblock to the front of its list (to reduce
		// paging).
		superblock *&head = _superblocks[newFullness][sizeclass];
		if (sb != head) {
			sb->remove();
			sb->insertBefore(head);
			head = sb;
		}
	}

	// If the superblock is now empty, recycle it.

	if ((newFullness == 0) && (sb->getNumBlocks() == sb->getNumAvailable())) {
		removeSuperblock(sb, sizeclass);
#if 0
		// Free it immediately.
		const size_t s = sizeFromClass(sizeclass);
		const int blksize = align(sizeof(block) + s);
#if HEAP_LOG
		// Record the memory deallocation.
		MemoryRequest m;
		m.deallocate((int)sb->getNumBlocks()
			* (int)sizeFromClass(sb->getBlockSizeClass()));
		pHeap->getLog(getIndex()).append(m);
#endif
#if HEAP_FRAG_STATS
		pHeap->setDeallocated(0,
			sb->getNumBlocks() * sizeFromClass(sb->getBlockSizeClass()));
#endif

		hoardUnsbrk(sb, align(sizeof(superblock) + blksize));
		return 1;
#else
		recycle(sb);
		// Update the stats.  This restores the stats to their state
		// before the call to removeSuperblock, above.
		incStats(sizeclass,
			sb->getNumBlocks() - sb->getNumAvailable(), sb->getNumBlocks());
#endif
	}

	// If this is the process heap, then we're done.
	if (this == (hoardHeap *)pHeap)
		return 0;

	//
	// Release a superblock, if necessary.
	//

	//
	// Check to see if the amount free exceeds the release threshold
	// (two superblocks worth of blocks for a given sizeclass) and if
	// the heap is sufficiently empty.
	//

	// We never move anything to the process heap if we're on a
	// uniprocessor.
	if (_numProcessors > 1) {
		int inUse, allocated;
		getStats(sizeclass, inUse, allocated);
		if ((inUse < allocated - getReleaseThreshold(sizeclass))
			&& (EMPTY_FRACTION * inUse <
				EMPTY_FRACTION * allocated - allocated)) {

			// We've crossed the magical threshold. Find the superblock with
			// the most free blocks and give it to the process heap.
			superblock *const maxSb = removeMaxSuperblock(sizeclass);
			assert(maxSb != NULL);

			// Update the statistics.

			assert(maxSb->getNumBlocks() >= maxSb->getNumAvailable());

			// Give the superblock back to the process heap.
			pHeap->release(maxSb);
		}
	}

	return 0;
}


void
hoardHeap::initNumProcs(void)
{
	system_info info;
	if (get_system_info(&info) != B_OK)
		hoardHeap::_numProcessors = 1;
	else
		hoardHeap::_numProcessors = info.cpu_count;

	fMaxThreadHeaps = 1 << (lg(_numProcessors) + 1);
	_numProcessorsMask = fMaxThreadHeaps - 1;
}

