/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef VM_PAGE_QUEUE_H
#define VM_PAGE_QUEUE_H


#include <util/DoublyLinkedList.h>

#include <lock.h>
#include <vm/vm_types.h>



struct VMPageQueue {
public:
	typedef DoublyLinkedList<vm_page, DoublyLinkedListMemberGetLink<vm_page,
		&vm_page::queue_link> > PageList;
	typedef PageList::ConstIterator Iterator;

public:
			void				Init(const char* name, int lockingOrder);

			const char*			Name() const	{ return fName; }
			int					LockingOrder() const { return fLockingOrder; }

	inline	bool				Lock();
	inline	void				Unlock();

	inline	void				LockMultiple(VMPageQueue* other);

	inline	void				Append(vm_page* page);
	inline	void				Prepend(vm_page* page);
	inline	void				InsertAfter(vm_page* insertAfter,
									vm_page* page);
	inline	void				Remove(vm_page* page);
	inline	vm_page*			RemoveHead();

	inline	vm_page*			Head() const;
	inline	vm_page*			Tail() const;
	inline	vm_page*			Previous(vm_page* page) const;
	inline	vm_page*			Next(vm_page* page) const;

	inline	uint32				Count() const	{ return fCount; }

	inline	Iterator			GetIterator() const;

private:
			const char*			fName;
			int					fLockingOrder;
			mutex				fLock;
			uint32				fCount;
			PageList			fPages;
};


bool
VMPageQueue::Lock()
{
	return mutex_lock(&fLock) == B_OK;
}


void
VMPageQueue::Unlock()
{
	mutex_unlock(&fLock);
}


void
VMPageQueue::LockMultiple(VMPageQueue* other)
{
	if (fLockingOrder < other->fLockingOrder) {
		Lock();
		other->Lock();
	} else {
		other->Lock();
		Lock();
	}
}


void
VMPageQueue::Append(vm_page* page)
{
#if DEBUG_PAGE_QUEUE
	if (page->queue != NULL) {
		panic("%p->VMPageQueue::Append(page: %p): page thinks it is "
			"already in queue %p", this, page, page->queue);
	}
#endif	// DEBUG_PAGE_QUEUE

	fPages.Add(page);
	fCount++;

#if DEBUG_PAGE_QUEUE
	page->queue = this;
#endif
}


void
VMPageQueue::Prepend(vm_page* page)
{
#if DEBUG_PAGE_QUEUE
	if (page->queue != NULL) {
		panic("%p->VMPageQueue::Prepend(page: %p): page thinks it is "
			"already in queue %p", this, page, page->queue);
	}
#endif	// DEBUG_PAGE_QUEUE

	fPages.Add(page, false);
	fCount++;

#if DEBUG_PAGE_QUEUE
	page->queue = this;
#endif
}


void
VMPageQueue::InsertAfter(vm_page* insertAfter, vm_page* page)
{
#if DEBUG_PAGE_QUEUE
	if (page->queue != NULL) {
		panic("%p->VMPageQueue::InsertAfter(page: %p): page thinks it is "
			"already in queue %p", this, page, page->queue);
	}
#endif	// DEBUG_PAGE_QUEUE

	fPages.InsertAfter(insertAfter, page);
	fCount++;

#if DEBUG_PAGE_QUEUE
	page->queue = this;
#endif
}


void
VMPageQueue::Remove(vm_page* page)
{
#if DEBUG_PAGE_QUEUE
	if (page->queue != this) {
		panic("%p->VMPageQueue::Remove(page: %p): page thinks it "
			"is in queue %p", this, page, page->queue);
	}
#endif	// DEBUG_PAGE_QUEUE

	fPages.Remove(page);
	fCount--;

#if DEBUG_PAGE_QUEUE
	page->queue = NULL;
#endif
}


vm_page*
VMPageQueue::RemoveHead()
{
	vm_page* page = fPages.RemoveHead();
	if (page != NULL) {
		fCount--;

#if DEBUG_PAGE_QUEUE
		if (page->queue != this) {
			panic("%p->VMPageQueue::RemoveHead(): page %p thinks it is in "
				"queue %p", this, page, page->queue);
		}

		page->queue = NULL;
#endif	// DEBUG_PAGE_QUEUE
	}

	return page;
}


vm_page*
VMPageQueue::Head() const
{
	return fPages.Head();
}


vm_page*
VMPageQueue::Tail() const
{
	return fPages.Tail();
}


vm_page*
VMPageQueue::Previous(vm_page* page) const
{
	return fPages.GetPrevious(page);
}


vm_page*
VMPageQueue::Next(vm_page* page) const
{
	return fPages.GetNext(page);
}


VMPageQueue::Iterator
VMPageQueue::GetIterator() const
{
	return fPages.GetIterator();
}


// #pragma mark - VMPageQueuePairLocker


struct VMPageQueuePairLocker {
	VMPageQueuePairLocker()
		:
		fQueue1(NULL),
		fQueue2(NULL)
	{
	}

	VMPageQueuePairLocker(VMPageQueue& queue1, VMPageQueue& queue2)
		:
		fQueue1(&queue1),
		fQueue2(&queue2)
	{
		_Lock();
	}

	~VMPageQueuePairLocker()
	{
		_Unlock();
	}

	void SetTo(VMPageQueue* queue1, VMPageQueue* queue2)
	{
		_Unlock();
		fQueue1 = queue1;
		fQueue2 = queue2;
		_Lock();
	}

	void Unlock()
	{
		if (fQueue1 != NULL) {
			fQueue1->Unlock();
			fQueue1 = NULL;
		}

		if (fQueue2 != NULL) {
			fQueue2->Unlock();
			fQueue2 = NULL;
		}
	}

private:
	void _Lock()
	{
		if (fQueue1 == fQueue2) {
			if (fQueue1 == NULL)
				return;
			fQueue1->Lock();
			fQueue2 = NULL;
		} else {
			if (fQueue1 == NULL) {
				fQueue2->Lock();
			} else if (fQueue2 == NULL) {
				fQueue1->Lock();
			} else if (fQueue1->LockingOrder() < fQueue2->LockingOrder()) {
				fQueue1->Lock();
				fQueue2->Lock();
			} else {
				fQueue2->Lock();
				fQueue1->Lock();
			}
		}
	}

	void _Unlock()
	{
		if (fQueue1 != NULL)
			fQueue1->Unlock();

		if (fQueue2 != NULL)
			fQueue2->Unlock();
	}

private:
	VMPageQueue*	fQueue1;
	VMPageQueue*	fQueue2;
};


#endif	// VM_PAGE_QUEUE_H
