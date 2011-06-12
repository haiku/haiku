/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_DPC_H
#define _KERNEL_DPC_H


#include <sys/cdefs.h>

#include <KernelExport.h>

#include <util/DoublyLinkedList.h>

#include <condition_variable.h>


namespace BKernel {


class DPCQueue;


class DPCCallback : public DoublyLinkedListLinkImpl<DPCCallback> {
public:
								DPCCallback();
	virtual						~DPCCallback();

	virtual	void				DoDPC(DPCQueue* queue) = 0;

private:
			friend class DPCQueue;

private:
			DPCQueue*			fInQueue;
};


class FunctionDPCCallback : public DPCCallback {
public:
								FunctionDPCCallback(DPCQueue* owner);

			void				SetTo(void (*function)(void*), void* argument);

	virtual	void				DoDPC(DPCQueue* queue);

private:
			DPCQueue*			fOwner;
			void 				(*fFunction)(void*);
			void*				fArgument;
};


class DPCQueue {
public:
								DPCQueue();
								~DPCQueue();

	static	DPCQueue*			DefaultQueue(int priority);

			status_t			Init(const char* name, int32 priority,
									uint32 reservedSlots);
			void				Close(bool cancelPending);

			status_t			Add(DPCCallback* callback,
									bool schedulerLocked);
			status_t			Add(void (*function)(void*), void* argument,
									bool schedulerLocked);
			bool				Cancel(DPCCallback* callback);

			thread_id			Thread() const
									{ return fThreadID; }

public:
			// conceptually package private
			void				Recycle(FunctionDPCCallback* callback);

private:
			typedef DoublyLinkedList<DPCCallback> CallbackList;

private:
	static	status_t			_ThreadEntry(void* data);
			status_t			_Thread();

			bool				_IsClosed() const
									{ return fThreadID < 0; }

private:
			spinlock			fLock;
			thread_id			fThreadID;
			CallbackList		fCallbacks;
			CallbackList		fUnusedFunctionCallbacks;
			ConditionVariable	fPendingCallbacksCondition;
			DPCCallback*		fCallbackInProgress;
			ConditionVariable*	fCallbackDoneCondition;
};


}	// namespace BKernel


using BKernel::DPCCallback;
using BKernel::DPCQueue;
using BKernel::FunctionDPCCallback;


__BEGIN_DECLS

void		dpc_init();

__END_DECLS


#endif	// _KERNEL_DPC_H
