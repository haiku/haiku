// BlockingQueue.h
//
// Copyright (c) 2004, Ingo Weinhold (bonefish@cs.tu-berlin.de)
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//
// Except as contained in this notice, the name of a copyright holder shall
// not be used in advertising or otherwise to promote the sale, use or other
// dealings in this Software without prior written authorization of the
// copyright holder.

#ifndef BLOCKING_QUEUE_H
#define BLOCKING_QUEUE_H

#include <AutoLocker.h>
#include <OS.h>

#include "DebugSupport.h"
#include "Locker.h"
#include "Vector.h"

template<typename Element>
class BlockingQueue : public Locker {
public:
								BlockingQueue(const char* name = NULL);
								~BlockingQueue();

			status_t			InitCheck() const;

			status_t			Close(bool deleteElements,
									const Vector<Element*>** elements = NULL);

			status_t			Push(Element* element);
			status_t			Pop(Element** element,
									bigtime_t timeout = B_INFINITE_TIMEOUT);
			status_t			Peek(Element** element);
			status_t			Remove(Element* element);

			int32				Size() const;

private:
			Vector<Element*>	fElements;
			sem_id				fElementSemaphore;
};

// constructor
template<typename Element>
BlockingQueue<Element>::BlockingQueue(const char* name)
	: fElements(),
	  fElementSemaphore(-1)
{
	fElementSemaphore = create_sem(0, (name ? name : "blocking queue"));
}

// destructor
template<typename Element>
BlockingQueue<Element>::~BlockingQueue()
{
	if (fElementSemaphore >= 0)
		delete_sem(fElementSemaphore);
}

// InitCheck
template<typename Element>
status_t
BlockingQueue<Element>::InitCheck() const
{
	return (fElementSemaphore < 0 ? fElementSemaphore : B_OK);
}

// Close
template<typename Element>
status_t
BlockingQueue<Element>::Close(bool deleteElements,
	const Vector<Element*>** elements)
{
	AutoLocker<Locker> _(this);
	status_t error = delete_sem(fElementSemaphore);
	if (error != B_OK)
		return error;
	fElementSemaphore = -1;
	if (elements)
		*elements = &fElements;
	if (deleteElements) {
		int32 count = fElements.Count();
		for (int32 i = 0; i < count; i++)
			delete fElements.ElementAt(i);
	}
	return error;
}

// Push
template<typename Element>
status_t
BlockingQueue<Element>::Push(Element* element)
{
	AutoLocker<Locker> _(this);
	if (fElementSemaphore < 0)
		return B_NO_INIT;
	status_t error = fElements.PushBack(element);
	if (error != B_OK)
		return error;
	error = release_sem(fElementSemaphore);
	if (error != B_OK)
		fElements.Erase(fElements.Count() - 1);
	return error;
}

// Pop
template<typename Element>
status_t
BlockingQueue<Element>::Pop(Element** element, bigtime_t timeout)
{
	status_t error = acquire_sem_etc(fElementSemaphore, 1, B_RELATIVE_TIMEOUT,
		timeout);
	if (error != B_OK)
		return error;
	AutoLocker<Locker> _(this);
	if (fElementSemaphore < 0)
		return B_NO_INIT;
	int32 count = fElements.Count();
	if (count == 0)
		return B_ERROR;
	*element = fElements.ElementAt(0);
	fElements.Erase(0);
	return B_OK;
}

// Peek
template<typename Element>
status_t
BlockingQueue<Element>::Peek(Element** element)
{
	AutoLocker<Locker> _(this);
	if (fElementSemaphore < 0)
		return B_NO_INIT;
	int32 count = fElements.Count();
	if (count == 0)
		return B_ENTRY_NOT_FOUND;
	*element = fElements.ElementAt(0);
	return B_OK;
}

// Remove
template<typename Element>
status_t
BlockingQueue<Element>::Remove(Element* element)
{
	status_t error = acquire_sem_etc(fElementSemaphore, 1,
		B_RELATIVE_TIMEOUT, 0);
	if (error != B_OK)
		return error;
	AutoLocker<Locker> _(this);
	if (fElementSemaphore < 0)
		return B_NO_INIT;
	int32 count = fElements.Remove(element);
	if (count == 0) {
		release_sem(fElementSemaphore);
		return B_ENTRY_NOT_FOUND;
	}
	if (count > 1) {
		ERROR("ERROR: BlockingQueue::Remove(): Removed %ld elements!\n",
			count);
	}
	return error;
}

// Size
template<typename Element>
int32
BlockingQueue<Element>::Size() const
{
	AutoLocker<Locker> _(this);
	return (fElements.Count());
}

#endif	// BLOCKING_QUEUE_H
