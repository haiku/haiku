/*
 * Copyright 2004-2010, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Author(s):
 *		Daniel Reinhold, danielre@users.sf.net
 *		Axel Dörfler, axeld@pinc-software.de
 *		Ingo Weinhold, ingo_weinhold@gmx.de
 */


#include <SupportDefs.h>

#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include <new>

#include <util/DoublyLinkedList.h>
#include <util/SinglyLinkedList.h>

#include <libroot_private.h>
#include <locks.h>
#include <runtime_loader.h>
#include <syscalls.h>


extern "C" void _IO_cleanup(void);
extern "C" void _thread_do_exit_work(void);


struct AtExitInfoBlock;

struct AtExitInfo : SinglyLinkedListLinkImpl<AtExitInfo> {
	AtExitInfoBlock*	block;
	void				(*hook)(void*);
	void*				data;
	void*				dsoHandle;
};

typedef SinglyLinkedList<AtExitInfo> AtExitInfoList;


struct AtExitInfoBlock : DoublyLinkedListLinkImpl<AtExitInfoBlock> {
	AtExitInfoBlock()
		:
		fFirstUnused(0)
	{
	}

	bool IsEmpty() const
	{
		return fFirstUnused == ATEXIT_MAX && fFreeList.IsEmpty();
	}

	AtExitInfo* AllocateInfo()
	{
		// Handle the likely case -- the block is not fully used yet -- first.
		// Grab the next info from the array.
		if (fFirstUnused < ATEXIT_MAX) {
			AtExitInfo* info = &fInfos[fFirstUnused++];
			info->block = this;
			return info;
		}

		// The block was fully used, but there might be infos in the free list.
		return fFreeList.RemoveHead();
	}

	void FreeInfo(AtExitInfo* info)
	{
		fFreeList.Add(info);
	}

private:
	AtExitInfo		fInfos[ATEXIT_MAX];
	uint32			fFirstUnused;
	AtExitInfoList	fFreeList;
};

typedef DoublyLinkedList<AtExitInfoBlock> AtExitInfoBlockList;


struct DSOPredicate {
	DSOPredicate(void* dsoHandle)
		:
		fDSOHandle(dsoHandle)
	{
	}

	inline bool operator()(const AtExitInfo* info) const
	{
		return info->dsoHandle == fDSOHandle;
	}

private:
	void*	fDSOHandle;
};


struct AddressRangePredicate {
	AddressRangePredicate(addr_t start, size_t size)
		:
		fStart(start),
		fEnd(start + size - 1)
	{
	}

	inline bool operator()(const AtExitInfo* info) const
	{
		addr_t address = (addr_t)info->hook;
		return info->dsoHandle == NULL && address >= fStart && address <= fEnd;
			// Note: We ignore hooks associated with an image (the same one
			// likely), since those will be called anyway when __cxa_finalize()
			// is invoked for that image.
	}

private:
	addr_t	fStart;
	addr_t	fEnd;
};


static AtExitInfoBlock sInitialAtExitInfoBlock;
static AtExitInfoBlockList sAtExitInfoBlocks;
static AtExitInfoList sAtExitInfoStack;
static recursive_lock sAtExitLock = RECURSIVE_LOCK_INITIALIZER("at exit lock");


static void inline
_exit_stack_lock()
{
	recursive_lock_lock(&sAtExitLock);
}


static void inline
_exit_stack_unlock()
{
	recursive_lock_unlock(&sAtExitLock);
}


template<typename Predicate>
static void
call_exit_hooks(const Predicate& predicate)
{
	_exit_stack_lock();

	AtExitInfo* previousInfo = NULL;
	AtExitInfo* info = sAtExitInfoStack.Head();
	while (info != NULL) {
		AtExitInfo* nextInfo = sAtExitInfoStack.GetNext(info);

		if (predicate(info)) {
			// remove info from stack
			sAtExitInfoStack.Remove(previousInfo, info);

			// call the hook
			info->hook(info->data);

			// return the info to the block
			if (info->block->IsEmpty())
				sAtExitInfoBlocks.Add(info->block);

			info->block->FreeInfo(info);
		} else
			previousInfo = info;

		info = nextInfo;
	}

	_exit_stack_unlock();
}


// #pragma mark -- C++ ABI


/*!	exit() hook registration function (mandated by the C++ ABI).
	\param hook Hook function to be called.
	\param data The data to be passed to the hook.
	\param dsoHandle If non-NULL, the hook is associated with the respective
		loaded shared object (aka image) -- the hook will be called either on
		exit() or earlier when the shared object is unloaded. If NULL, the hook
		is called only on exit().
	\return \c 0 on success, another value on failure.
 */
extern "C" int
__cxa_atexit(void (*hook)(void*), void* data, void* dsoHandle)
{
	if (hook == NULL)
		return -1;

	_exit_stack_lock();

	// We need to allocate an info. Get an info block from which to allocate.
	AtExitInfoBlock* block = sAtExitInfoBlocks.Head();
	if (block == NULL) {
		// might be the first call -- check the initial block
		if (!sInitialAtExitInfoBlock.IsEmpty()) {
			block = &sInitialAtExitInfoBlock;
		} else {
			// no empty block -- let's hope libroot is initialized sufficiently
			// for the heap to work
			block = new(std::nothrow) AtExitInfoBlock;
			if (block == NULL) {
				_exit_stack_unlock();
				return -1;
			}
		}

		sAtExitInfoBlocks.Add(block);
	}

	// allocate the info
	AtExitInfo* info = block->AllocateInfo();

	// If the block is empty now, remove it from the list.
	if (block->IsEmpty())
		sAtExitInfoBlocks.Remove(block);

	// init and add the info
	info->hook = hook;
	info->data = data;
	info->dsoHandle = dsoHandle;

	sAtExitInfoStack.Add(info);

	_exit_stack_unlock();

	return 0;
}


/*!	exit() hook calling function (mandated by the C++ ABI).

	Calls the exit() hooks associated with a certain shared object handle,
	respectively calls all hooks when a NULL handle is given. All called
	hooks are removed.

	\param dsoHandle If non-NULL, all hooks associated with that handle are
		called. If NULL, all hooks are called.
 */
extern "C" void
__cxa_finalize(void* dsoHandle)
{
	if (dsoHandle == NULL) {
		// call all hooks
		_exit_stack_lock();

		while (AtExitInfo* info = sAtExitInfoStack.RemoveHead()) {
			// call the hook
			info->hook(info->data);

			// return the info to the block
			if (info->block->IsEmpty())
				sAtExitInfoBlocks.Add(info->block);

			info->block->FreeInfo(info);
		}

		_exit_stack_unlock();
	} else {
		// call all hooks for the respective DSO
		call_exit_hooks(DSOPredicate(dsoHandle));
	}
}


// #pragma mark - private API


void
_call_atexit_hooks_for_range(addr_t start, addr_t size)
{
	call_exit_hooks(AddressRangePredicate(start, size));
}


// #pragma mark - public API


void
abort()
{
	fprintf(stderr, "Abort\n");

	raise(SIGABRT);
	debugger("abort() called");
	exit(EXIT_FAILURE);
}


int
atexit(void (*func)(void))
{
	return __cxa_atexit((void (*)(void*))func, NULL, NULL);
}


void
exit(int status)
{
	// BeOS on exit notification for the main thread
	_thread_do_exit_work();

	// unwind the exit stack, calling the registered functions
	__cxa_finalize(NULL);

	// close all open files
	_IO_cleanup();

	__gRuntimeLoader->call_termination_hooks();

	// exit with status code
	_kern_exit_team(status);
}
