/*
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */
#ifndef TTY_SEMAPHORE_POOL_H
#define TTY_SEMAPHORE_POOL_H

#include <lock.h>
#include <util/DoublyLinkedList.h>

class Semaphore : public DoublyLinkedListLinkImpl<Semaphore> {
public:
	Semaphore(sem_id sem)
		: fSemaphore(sem)
	{
	}

	Semaphore(int32 count, const char *name)
	{
		fSemaphore = create_sem(count, name);
	}

	~Semaphore()
	{
		if (fSemaphore >= 0)
			delete_sem(fSemaphore);
	}

	status_t InitCheck() const
	{
		return (fSemaphore >= 0 ? B_OK : fSemaphore);
	}

	sem_id ID() const
	{
		return fSemaphore;
	}

	status_t ZeroCount();

private:
	sem_id	fSemaphore;

public:
	Semaphore	*fNext;
};

class SemaphorePool {
public:
	SemaphorePool(int32 maxCached);
	~SemaphorePool();

	status_t Init();

	status_t Get(Semaphore *&semaphore);
	void Put(Semaphore *semaphore);

private:
	typedef DoublyLinkedList<Semaphore> SemaphoreList;

	struct mutex	fLock;
	SemaphoreList	fSemaphores;
	int32			fCount;
	int32			fMaxCount;
};

#endif	// TTY_SEMAPHORE_POOL_H
