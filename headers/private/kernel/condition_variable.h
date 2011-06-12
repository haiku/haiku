/*
 * Copyright 2007-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
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
#if KDEBUG
	inline						ConditionVariableEntry();
	inline						~ConditionVariableEntry();
#endif

			bool				Add(const void* object);
			status_t			Wait(uint32 flags = 0, bigtime_t timeout = 0);
			status_t			Wait(const void* object, uint32 flags = 0,
									bigtime_t timeout = 0);

	inline	status_t			WaitStatus() const { return fWaitStatus; }

	inline	ConditionVariable*	Variable() const { return fVariable; }

private:
	inline	void				AddToVariable(ConditionVariable* variable);

private:
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
			void				Unpublish(bool schedulerLocked = false);

	inline	void				NotifyOne(bool schedulerLocked = false,
									status_t result = B_OK);
	inline	void				NotifyAll(bool schedulerLocked = false,
									status_t result = B_OK);

	static	void				NotifyOne(const void* object,
									bool schedulerLocked = false,
									status_t result = B_OK);
	static	void				NotifyAll(const void* object,
									bool schedulerLocked = false,
									status_t result = B_OK);
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
			void				_Notify(bool all, bool schedulerLocked,
									status_t result);
			void				_NotifyLocked(bool all, status_t result);

protected:
			typedef DoublyLinkedList<ConditionVariableEntry> EntryList;

			const void*			fObject;
			const char*			fObjectType;
			EntryList			fEntries;
			ConditionVariable*	fNext;

			friend struct ConditionVariableEntry;
			friend struct ConditionVariableHashDefinition;
};


#if KDEBUG

inline
ConditionVariableEntry::ConditionVariableEntry()
	: fVariable(NULL)
{
}

inline
ConditionVariableEntry::~ConditionVariableEntry()
{
	if (fVariable != NULL) {
		panic("Destroying condition variable entry %p, but it's still "
			"attached to variable %p\n", this, fVariable);
	}
}

#endif


inline void
ConditionVariable::NotifyOne(bool schedulerLocked, status_t result)
{
	_Notify(false, schedulerLocked, result);
}


inline void
ConditionVariable::NotifyAll(bool schedulerLocked, status_t result)
{
	_Notify(true, schedulerLocked, result);
}


extern "C" {
#endif	// __cplusplus

extern void condition_variable_init();

#ifdef __cplusplus
}	// extern "C"
#endif

#endif	/* _KERNEL_CONDITION_VARIABLE_H */
