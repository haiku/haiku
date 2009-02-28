/*
 * Copyright 2007-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef USERLAND_FS_HAIKU_CONDITION_VARIABLE_H
#define USERLAND_FS_HAIKU_CONDITION_VARIABLE_H

#include <OS.h>

#include <kernel/util/DoublyLinkedList.h>
#include <kernel/util/OpenHashTable.h>


class ConditionVariable;


struct ConditionVariableEntry
	: DoublyLinkedListLinkImpl<ConditionVariableEntry> {
public:
			bool				Add(const void* object);
			status_t			Wait(uint32 flags = 0, bigtime_t timeout = 0);
			status_t			Wait(const void* object, uint32 flags = 0,
									bigtime_t timeout = 0);

	inline	ConditionVariable* Variable() const		{ return fVariable; }

private:
	inline	void				AddToVariable(ConditionVariable* variable);

private:
			ConditionVariable*	fVariable;
			thread_id			fThread;
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

			const void*			Object() const		{ return fObject; }
			const char*			ObjectType() const	{ return fObjectType; }

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


status_t condition_variable_init();


#endif	// USERLAND_FS_HAIKU_CONDITION_VARIABLE_H
