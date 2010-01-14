// GlobalBlockerPool.cpp

#include <new>

#include "GlobalBlockerPool.h"

// CreateDefault
status_t
GlobalBlockerPool::CreateDefault()
{
	if (sPool)
		return B_OK;

	BlockerPool* pool = new(std::nothrow) BlockerPool;
	if (!pool)
		return B_NO_MEMORY;

	status_t error = pool->InitCheck();
	if (error != B_OK) {
		delete pool;
		return error;
	}

	sPool = pool;
	return B_OK;
}

// DeleteDefault
void
GlobalBlockerPool::DeleteDefault()
{
	if (sPool) {
		delete sPool;
		sPool = NULL;
	}
}

// GetDefault
BlockerPool*
GlobalBlockerPool::GetDefault()
{
	return sPool;
}

// sPool
BlockerPool* GlobalBlockerPool::sPool = NULL;
