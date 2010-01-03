/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef VM_PAGE_QUEUE_H
#define VM_PAGE_QUEUE_H


struct VMPageQueue {
public:
	typedef DoublyLinkedList<vm_page, DoublyLinkedListMemberGetLink<vm_page,
		&vm_page::queue_link> > PageList;
	typedef PageList::ConstIterator Iterator;

public:
	inline						VMPageQueue();

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

	inline	void				MoveFrom(VMPageQueue* from, vm_page* page);

	inline	uint32				Count() const	{ return fCount; }

	inline	Iterator			GetIterator() const;

private:
			PageList			fPages;
			uint32				fCount;
};


VMPageQueue::VMPageQueue()
	:
	fCount(0)
{
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


/*!	Moves a page to the tail of this queue, but only does so if
	the page is currently in another queue.
*/
void
VMPageQueue::MoveFrom(VMPageQueue* from, vm_page* page)
{
	if (from != this) {
		from->Remove(page);
		Append(page);
	}
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
