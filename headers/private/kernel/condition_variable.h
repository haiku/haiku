/*
 * Copyright 2007-2008, Ingo Weinhold, bonefish@cs.tu-berlin.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_CONDITION_VARIABLE_H
#define _KERNEL_CONDITION_VARIABLE_H


#include <OS.h>

#include <debug.h>

#ifdef __cplusplus

#include <util/DoublyLinkedList.h>
#include <util/OpenHashTable.h>


class ConditionVariable;


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

	inline	ConditionVariable* Variable() const		{ return fVariable; }

private:
	inline	void				AddToVariable(ConditionVariable* variable);

private:
			ConditionVariable*	fVariable;
			struct thread*		fThread;
			status_t			fWaitStatus;

			friend class ConditionVariable;
};


class ConditionVariable : protected HashTableLink<ConditionVariable> {
public:
			void				Init(const void* object,
									const char* objectType);
									// for anonymous (unpublished) cvars

			void				Publish(const void* object,
									const char* objectType);
			void				Unpublish(bool threadsLocked = false);

	inline	void				NotifyOne(bool threadsLocked = false);
	inline	void				NotifyAll(bool threadsLocked = false);

			void				Add(ConditionVariableEntry* entry);

			status_t			Wait(uint32 flags = 0, bigtime_t timeout = 0);
									// all-in one, i.e. doesn't need a
									// ConditionVariableEntry

			const void*			Object() const	{ return fObject; }

	static	void				ListAll();
			void				Dump() const;

private:
			void				_Notify(bool all, bool threadsLocked);
			void				_NotifyChecked(bool all, status_t result);

protected:
			typedef DoublyLinkedList<ConditionVariableEntry> EntryList;

			const void*			fObject;
			const char*			fObjectType;
			EntryList			fEntries;

			friend class ConditionVariableEntry;
			friend class ConditionVariableHashDefinition;
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
ConditionVariable::NotifyOne(bool threadsLocked)
{
	_Notify(false, threadsLocked);
}


inline void
ConditionVariable::NotifyAll(bool threadsLocked)
{
	_Notify(true, threadsLocked);
}


extern "C" {
#endif	// __cplusplus

extern void condition_variable_init();

#ifdef __cplusplus
}	// extern "C"
#endif

#endif	/* _KERNEL_CONDITION_VARIABLE_H */
