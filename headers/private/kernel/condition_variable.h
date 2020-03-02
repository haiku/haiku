/*
 * Copyright 2007-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2019, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_CONDITION_VARIABLE_H
#define _KERNEL_CONDITION_VARIABLE_H


#include <OS.h>

#include <debug.h>

#ifdef __cplusplus

#include <util/DoublyLinkedList.h>
#include <util/OpenHashTable.h>


struct ConditionVariable;


struct ConditionVariableEntry
	: DoublyLinkedListLinkImpl<ConditionVariableEntry> {
public:
								ConditionVariableEntry();
								~ConditionVariableEntry();

			bool				Add(const void* object);
			status_t			Wait(uint32 flags = 0, bigtime_t timeout = 0);
			status_t			Wait(const void* object, uint32 flags = 0,
									bigtime_t timeout = 0);

	inline	status_t			WaitStatus() const { return fWaitStatus; }

	inline	ConditionVariable*	Variable() const { return fVariable; }

private:
	inline	void				_AddToLockedVariable(ConditionVariable* variable);
			void				_RemoveFromVariable();

private:
			spinlock			fLock;
			ConditionVariable*	fVariable;
			Thread*				fThread;
			status_t			fWaitStatus;

			friend struct ConditionVariable;
};


struct ConditionVariable {
public:
			void				Init(const void* object,
									const char* objectType);
									// for anonymous (unpublished) cvars

			void				Publish(const void* object,
									const char* objectType);
			void				Unpublish();

	inline	void				NotifyOne(status_t result = B_OK);
	inline	void				NotifyAll(status_t result = B_OK);

	static	void				NotifyOne(const void* object, status_t result);
	static	void				NotifyAll(const void* object, status_t result);
									// (both methods) caller must ensure that
									// the variable is not unpublished
									// concurrently

			void				Add(ConditionVariableEntry* entry);

			status_t			Wait(uint32 flags = 0, bigtime_t timeout = 0);
									// all-in one, i.e. doesn't need a
									// ConditionVariableEntry

			const void*			Object() const		{ return fObject; }
			const char*			ObjectType() const	{ return fObjectType; }

	static	void				ListAll();
			void				Dump() const;

private:
			void				_Notify(bool all, status_t result);
			void				_NotifyLocked(bool all, status_t result);

protected:
			typedef DoublyLinkedList<ConditionVariableEntry> EntryList;

			const void*			fObject;
			const char*			fObjectType;

			spinlock			fLock;
			EntryList			fEntries;
			ConditionVariable*	fNext;

			friend struct ConditionVariableEntry;
			friend struct ConditionVariableHashDefinition;
};


inline void
ConditionVariable::NotifyOne(status_t result)
{
	_Notify(false, result);
}


inline void
ConditionVariable::NotifyAll(status_t result)
{
	_Notify(true, result);
}


extern "C" {
#endif	// __cplusplus

extern void condition_variable_init();

#ifdef __cplusplus
}	// extern "C"
#endif

#endif	/* _KERNEL_CONDITION_VARIABLE_H */
