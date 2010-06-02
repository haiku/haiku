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
#include <int.h>
#include <util/AutoLock.h>
#include <vm/vm_types.h>


struct VMPageQueue {
public:
	typedef DoublyLinkedList<vm_page,
		DoublyLinkedListMemberGetLink<vm_page, &vm_page::queue_link> > PageList;
	typedef PageList::ConstIterator Iterator;

public:
			void				Init(const char* name);

			const char*			Name() const	{ return fName; }

	inline	void				Append(vm_page* page);
	inline	void				Prepend(vm_page* page);
	inline	void				InsertAfter(vm_page* insertAfter,
									vm_page* page);
	inline	void				Remove(vm_page* page);
	inline	vm_page*			RemoveHead();
	inline	void				Requeue(vm_page* page, bool tail);

	inline	void				AppendUnlocked(vm_page* page);
	inline	void				AppendUnlocked(PageList& pages, uint32 count);
	inline	void				PrependUnlocked(vm_page* page);
	inline	void				RemoveUnlocked(vm_page* page);
	inline	vm_page*			RemoveHeadUnlocked();
	inline	void				RequeueUnlocked(vm_page* page, bool tail);

	inline	vm_page*			Head() const;
	inline	vm_page*			Tail() const;
	inline	vm_page*			Previous(vm_page* page) const;
	inline	vm_page*			Next(vm_page* page) const;

	inline	phys_addr_t			Count() const	{ return fCount; }

	inline	Iterator			GetIterator() const;

	inline	spinlock&			GetLock()	{ return fLock; }

protected:
			const char*			fName;
			spinlock			fLock;
			phys_addr_t			fCount;
			PageList			fPages;
};


// #pragma mark - VMPageQueue


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


void
VMPageQueue::Requeue(vm_page* page, bool tail)
{
#if DEBUG_PAGE_QUEUE
	if (page->queue != this) {
		panic("%p->VMPageQueue::Requeue(): page %p thinks it is in "
			"queue %p", this, page, page->queue);
	}
#endif

	fPages.Remove(page);
	fPages.Add(page, tail);
}


void
VMPageQueue::AppendUnlocked(vm_page* page)
{
	InterruptsSpinLocker locker(fLock);
	Append(page);
}


void
VMPageQueue::AppendUnlocked(PageList& pages, uint32 count)
{
#if DEBUG_PAGE_QUEUE
	for (PageList::Iterator it = pages.GetIterator();
			vm_page* page = it.next();) {
		if (page->queue != NULL) {
			panic("%p->VMPageQueue::AppendUnlocked(): page %p thinks it is "
				"already in queue %p", this, page, page->queue);
		}

		page->queue = this;
	}

#endif	// DEBUG_PAGE_QUEUE

	InterruptsSpinLocker locker(fLock);

	fPages.MoveFrom(&pages);
	fCount += count;
}


void
VMPageQueue::PrependUnlocked(vm_page* page)
{
	InterruptsSpinLocker locker(fLock);
	Prepend(page);
}


void
VMPageQueue::RemoveUnlocked(vm_page* page)
{
	InterruptsSpinLocker locker(fLock);
	return Remove(page);
}


vm_page*
VMPageQueue::RemoveHeadUnlocked()
{
	InterruptsSpinLocker locker(fLock);
	return RemoveHead();
}


void
VMPageQueue::RequeueUnlocked(vm_page* page, bool tail)
{
	InterruptsSpinLocker locker(fLock);
	Requeue(page, tail);
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


#endif	// VM_PAGE_QUEUE_H
